#include "AudioAvioBridge.h"

#include "AudioCacheManager.h"

#include <QtDebug>

namespace {
constexpr int kAvioBufferSize = 32 * 1024;
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

void AudioAvioBridge::prepareSeek(qint64 targetMs) {
    if (!m_stream || targetMs < 0) {
        return;
    }

    const AudioCacheManager::DirectSeekHint hint =
        AudioCacheManager::instance().directSeekHintForPlaybackUrl(m_url, targetMs);
    if (!hint.valid) {
        return;
    }

    m_stream->prepareSeekHint(hint.targetMs, hint.targetByte, hint.contentLength,
                              hint.seekIndexHit);
    const bool forced = m_stream->forceSeekToHint(hint.targetMs, hint.targetByte,
                                                  hint.contentLength, hint.seekIndexHit);
    qDebug() << "[AudioAvioBridge] direct_seek_hint"
             << "targetMs=" << hint.targetMs << "targetByte=" << hint.targetByte
             << "contentLength=" << hint.contentLength
             << "seekIndexHit=" << hint.seekIndexHit
             << "registered=" << hint.registeredHint
             << "generation=" << hint.generation
             << "forced=" << forced;
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

    if (!m_stream->seek(target)) {
        return m_stream->wasLastReadInterrupted() ? AVERROR_EXIT : AVERROR(EIO);
    }
    return m_stream->position();
}
