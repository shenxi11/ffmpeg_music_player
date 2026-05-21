#include "AudioAvioBridge.h"

#include "AudioCacheManager.h"

#include <QtDebug>
#include <utility>

namespace {
constexpr int kAvioBufferSize = 32 * 1024;
constexpr qint64 kSeekGuardOperationBudget = 96;
constexpr qint64 kSeekGuardNearTargetToleranceBytes = 24LL * 1024 * 1024;
constexpr qint64 kSeekGuardEofProbeBytes = 2LL * 1024 * 1024;
constexpr qint64 kFlacApproxAnchorBackoffBytes = 1024LL * 1024;
constexpr qint64 kFlacApproxAnchorBeforeBytes = 256LL * 1024;
constexpr qint64 kFlacApproxAnchorAfterBytes = 4LL * 1024 * 1024;

bool canUseFlacRangeAnchor(const AudioCacheManager::DirectSeekHint& hint) {
    if (hint.exact) {
        return true;
    }
    if (!hint.seekIndexHit) {
        return false;
    }
    if (hint.confidenceReason == QStringLiteral("duration_ratio_fallback")) {
        return false;
    }
    if (hint.confidenceReason == QStringLiteral("local_flac_learning_feedback")) {
        return hint.confidenceScore >= 30;
    }
    if (hint.confidenceReason == QStringLiteral("local_flac_learning_approx")) {
        return hint.confidenceScore >= 35;
    }
    return hint.confidenceScore >= 45;
}
}

AudioAvioBridge::AudioAvioBridge() = default;

AudioAvioBridge::~AudioAvioBridge() {
    if (m_avioContext) {
        avio_context_free(&m_avioContext);
        m_avioBuffer = nullptr;
    } else if (m_avioBuffer) {
        av_freep(&m_avioBuffer);
    }
}

void AudioAvioBridge::setInterruptCallback(InterruptCallback callback) {
    m_interruptCallback = std::move(callback);
    if (m_stream) {
        m_stream->setInterruptCallback(m_interruptCallback);
    }
}

bool AudioAvioBridge::open(const QUrl& url) {
    m_lastError.clear();
    m_url = url;
    resetActiveSeekGuard();
    m_stream = std::make_unique<AudioPrefetchStream>();
    m_stream->setInterruptCallback(m_interruptCallback);
    if (!m_stream->open(url)) {
        m_lastError = m_stream->lastError();
        return false;
    }

    return true;
}

bool AudioAvioBridge::attachTo(AVFormatContext* formatContext) {
    if (!formatContext || !m_stream) {
        m_lastError = QStringLiteral("Invalid AVIO attach state");
        return false;
    }

    m_avioBuffer = static_cast<unsigned char*>(av_malloc(kAvioBufferSize));
    if (!m_avioBuffer) {
        m_lastError = QStringLiteral("Failed to allocate AVIO buffer");
        return false;
    }

    m_avioContext = avio_alloc_context(m_avioBuffer, kAvioBufferSize, 0, this,
                                       &AudioAvioBridge::readPacket, nullptr,
                                       &AudioAvioBridge::seek);
    if (!m_avioContext) {
        av_freep(&m_avioBuffer);
        m_lastError = QStringLiteral("Failed to allocate AVIO context");
        return false;
    }

    m_avioContext->seekable = m_stream->canSeek() ? AVIO_SEEKABLE_NORMAL : 0;
    formatContext->pb = m_avioContext;
    formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    qDebug() << "[AudioAvioBridge] Custom AVIO attached:"
             << "length=" << contentLength() << "canSeek=" << canSeek();
    return true;
}

qint64 AudioAvioBridge::contentLength() const {
    return m_stream ? m_stream->size() : -1;
}

bool AudioAvioBridge::canSeek() const {
    return m_stream && m_stream->canSeek();
}

bool AudioAvioBridge::prepareSeek(qint64 targetMs) {
    if (!m_stream || targetMs < 0) {
        return false;
    }

    ++m_activeSeekSerial;
    resetActiveSeekGuard();

    const bool allowApproximateAnchor = shouldUseFlacApproximateAnchor();
    const AudioCacheManager::DirectSeekHint hint =
        AudioCacheManager::instance().directSeekHintForPlaybackUrl(m_url, targetMs);
    if (!hint.valid) {
        qDebug() << "[AudioAvioBridge] direct_seek_hint_unavailable"
                 << "targetMs=" << targetMs << "url=" << m_url.toString()
                 << "flacApproxAnchor=" << allowApproximateAnchor;
        return false;
    }

    const bool exactHint = hint.exact;
    const bool allowTrustedFlacAnchor =
        allowApproximateAnchor && canUseFlacRangeAnchor(hint);
    const bool approximateFlacAnchor = allowTrustedFlacAnchor && !exactHint;
    const qint64 rangeAnchorByte =
        approximateFlacAnchor
            ? qMax<qint64>(0, hint.targetByte - kFlacApproxAnchorBackoffBytes)
            : hint.targetByte;
    const bool allowRangeAnchor = allowTrustedFlacAnchor && rangeAnchorByte >= 0;
    m_stream->prepareSeekHint(hint.targetMs, rangeAnchorByte, hint.contentLength,
                              exactHint, allowRangeAnchor);
    const bool shouldForceRangeAnchor = exactHint || allowRangeAnchor;
    const bool forced = shouldForceRangeAnchor &&
                        m_stream->forceSeekToHint(hint.targetMs, rangeAnchorByte,
                                                  hint.contentLength, exactHint,
                                                  allowRangeAnchor);
    m_activeSeekTargetMs = hint.targetMs;
    m_activeSeekTargetByte = forced && allowRangeAnchor ? rangeAnchorByte : -1;
    m_activeSeekContentLength = hint.contentLength;
    m_activeSeekGuardBudget = forced && allowRangeAnchor ? kSeekGuardOperationBudget : 0;
    m_activeSeekApproximateAnchor = forced && allowRangeAnchor && !exactHint;
    qDebug() << "[AudioAvioBridge] direct_seek_hint"
             << "targetMs=" << hint.targetMs << "targetByte=" << rangeAnchorByte
             << "rawTargetByte=" << hint.targetByte
             << "contentLength=" << hint.contentLength
             << "seekIndexHit=" << hint.seekIndexHit
             << "exact=" << exactHint
             << "anchorTimeMs=" << hint.anchorTimeMs
             << "preRollMs=" << hint.preRollMs
             << "registered=" << hint.registeredHint
             << "generation=" << hint.generation
             << "flacApproxAnchor=" << allowApproximateAnchor
             << "trustedFlacAnchor=" << allowTrustedFlacAnchor
             << "confidenceScore=" << hint.confidenceScore
             << "confidenceReason=" << hint.confidenceReason
             << "allowRangeAnchor=" << allowRangeAnchor
             << "forceRangeAnchor=" << shouldForceRangeAnchor
             << "forced=" << forced;
    if (forced && allowRangeAnchor) {
        qDebug() << "[AudioAvioBridge] flac_anchor_byte"
                 << "targetMs=" << hint.targetMs
                 << "targetByte=" << rangeAnchorByte
                 << "rawTargetByte=" << hint.targetByte
                 << "contentLength=" << hint.contentLength
                 << "backoffBytes=" << (hint.targetByte - rangeAnchorByte)
                 << "anchorTimeMs=" << hint.anchorTimeMs
                 << "preRollMs=" << hint.preRollMs
                 << "exact=" << exactHint
                 << "seekIndexHit=" << hint.seekIndexHit
                 << "generation=" << hint.generation;
    }
    return forced && allowRangeAnchor;
}

int AudioAvioBridge::readPacket(void* opaque, uint8_t* buffer, int bufferSize) {
    auto* self = static_cast<AudioAvioBridge*>(opaque);
    if (!self) {
        return AVERROR(EIO);
    }
    return self->readPacket(buffer, bufferSize);
}

int64_t AudioAvioBridge::seek(void* opaque, int64_t offset, int whence) {
    auto* self = static_cast<AudioAvioBridge*>(opaque);
    if (!self) {
        return AVERROR(EIO);
    }
    return self->seek(offset, whence);
}

int AudioAvioBridge::readPacket(uint8_t* buffer, int bufferSize) {
    if (m_interruptCallback && m_interruptCallback()) {
        return AVERROR_EXIT;
    }
    const int ret = m_stream ? m_stream->read(buffer, bufferSize) : AudioPrefetchStream::ReadError;
    if (ret > 0) {
        consumeActiveSeekGuardBudget("read");
        return ret;
    }
    if (ret == 0) {
        return AVERROR_EOF;
    }
    if (ret == AudioPrefetchStream::ReadInterrupted ||
        (m_interruptCallback && m_interruptCallback())) {
        return AVERROR_EXIT;
    }
    return AVERROR(EIO);
}

int64_t AudioAvioBridge::seek(int64_t offset, int whence) {
    if (!m_stream) {
        return AVERROR(EIO);
    }
    if (m_interruptCallback && m_interruptCallback()) {
        return AVERROR_EXIT;
    }
    if (whence == AVSEEK_SIZE) {
        return contentLength();
    }

    qint64 target = offset;
    const int origin = whence & ~AVSEEK_FORCE;
    if (origin == SEEK_CUR) {
        target = m_stream->position() + offset;
    } else if (origin == SEEK_END) {
        const qint64 length = contentLength();
        if (length < 0) {
            return AVERROR(ENOSYS);
        }
        target = length + offset;
    } else if (origin != SEEK_SET) {
        return AVERROR(EINVAL);
    }

    QString rejectReason;
    const qint64 length = contentLength();
    const qint64 activeSeekTargetByte = m_activeSeekTargetByte;
    const qint64 activeSeekTargetMs = m_activeSeekTargetMs;
    const qint64 activeSeekSerial = m_activeSeekSerial;
    if (shouldRejectInternalSeek(target, origin, length, &rejectReason)) {
        const qint64 current = m_stream->position();
        qDebug() << "[AudioAvioBridge] avio_seek_rejected"
                 << "requestedOffset=" << offset << "whence=" << whence
                 << "target=" << target << "current=" << current
                 << "length=" << length
                 << "activeTargetByte=" << activeSeekTargetByte
                 << "activeTargetMs=" << activeSeekTargetMs
                 << "serial=" << activeSeekSerial
                 << "reason=" << rejectReason;
        return current;
    }

    if (!m_stream->seek(target)) {
        return m_stream->wasLastReadInterrupted() ? AVERROR_EXIT : AVERROR(EIO);
    }
    qDebug() << "[AudioAvioBridge] avio_seek_accepted"
             << "requestedOffset=" << offset << "whence=" << whence
             << "target=" << target << "length=" << length
             << "activeTargetByte=" << m_activeSeekTargetByte
             << "activeTargetMs=" << m_activeSeekTargetMs
             << "serial=" << m_activeSeekSerial;
    return m_stream->position();
}

bool AudioAvioBridge::shouldRejectInternalSeek(qint64 target, int origin, qint64 length,
                                               QString* reason) {
    if (m_activeSeekTargetByte < 0 || m_activeSeekGuardBudget <= 0) {
        return false;
    }

    auto reject = [this, reason](const QString& rejectReason) {
        if (reason) {
            *reason = rejectReason;
        }
        consumeActiveSeekGuardBudget("rejected_seek");
        return true;
    };

    if (target < 0) {
        return reject(QStringLiteral("negative_target"));
    }

    const qint64 total = m_activeSeekContentLength > 0 ? m_activeSeekContentLength : length;
    if (m_activeSeekApproximateAnchor) {
        const qint64 lowerBound =
            qMax<qint64>(0, m_activeSeekTargetByte - kFlacApproxAnchorBeforeBytes);
        const qint64 upperBound = m_activeSeekTargetByte + kFlacApproxAnchorAfterBytes;
        if (target >= lowerBound && target <= upperBound) {
            return false;
        }
        return reject(QStringLiteral("far_from_flac_approx_anchor"));
    }

    const qint64 distanceFromTarget = qAbs(target - m_activeSeekTargetByte);
    if (distanceFromTarget <= kSeekGuardNearTargetToleranceBytes) {
        return false;
    }

    if (origin == SEEK_END) {
        return reject(QStringLiteral("seek_end_probe_during_user_seek"));
    }

    if (total > 0 && target >= qMax<qint64>(0, total - kSeekGuardEofProbeBytes)) {
        return reject(QStringLiteral("near_eof_probe_during_user_seek"));
    }

    return reject(QStringLiteral("far_from_user_seek_target"));
}

void AudioAvioBridge::consumeActiveSeekGuardBudget(const char* reason) {
    if (m_activeSeekGuardBudget <= 0) {
        return;
    }

    --m_activeSeekGuardBudget;
    if (m_activeSeekGuardBudget > 0) {
        return;
    }

    qDebug() << "[AudioAvioBridge] seek_guard_expired"
             << "reason=" << reason
             << "activeTargetByte=" << m_activeSeekTargetByte
             << "activeTargetMs=" << m_activeSeekTargetMs
             << "approximateFlacAnchor=" << m_activeSeekApproximateAnchor
             << "serial=" << m_activeSeekSerial;
    if (m_activeSeekApproximateAnchor) {
        // Remote FLAC approximate anchors are intentionally narrow. Letting the guard expire
        // would allow FFmpeg EOF probes to drag the input to a wrong byte range before the
        // first packet is validated.
        m_activeSeekGuardBudget = kSeekGuardOperationBudget;
        return;
    }
    resetActiveSeekGuard();
}

void AudioAvioBridge::resetActiveSeekGuard() {
    m_activeSeekTargetMs = -1;
    m_activeSeekTargetByte = -1;
    m_activeSeekContentLength = -1;
    m_activeSeekGuardBudget = 0;
    m_activeSeekApproximateAnchor = false;
}

bool AudioAvioBridge::shouldUseFlacApproximateAnchor() const {
    const QString scheme = m_url.scheme().toLower();
    return scheme.startsWith(QStringLiteral("http")) &&
           m_url.path().toLower().endsWith(QStringLiteral(".flac"));
}
