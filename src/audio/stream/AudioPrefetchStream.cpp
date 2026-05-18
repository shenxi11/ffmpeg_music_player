#include "AudioPrefetchStream.h"

#include "AudioCacheManager.h"

#include <QEventLoop>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QVector>
#include <QtDebug>
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
constexpr int kTrackCount = 3;
constexpr int kWorkerIdleWaitMs = 20;
constexpr int kHeaderWaitMs = 5000;

bool parseContentRange(const QByteArray& value, qint64* start, qint64* end, qint64* total) {
    if (!start || !end || !total || value.isEmpty()) {
        return false;
    }

    static const QRegularExpression re(
        QStringLiteral(R"(^\s*bytes\s+(\d+)-(\d+)/(\d+|\*)\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(QString::fromLatin1(value));
    if (!match.hasMatch()) {
        return false;
    }

    bool okStart = false;
    bool okEnd = false;
    *start = match.captured(1).toLongLong(&okStart);
    *end = match.captured(2).toLongLong(&okEnd);
    if (match.captured(3) == QStringLiteral("*")) {
        *total = -1;
    } else {
        bool okTotal = false;
        *total = match.captured(3).toLongLong(&okTotal);
        if (!okTotal) {
            return false;
        }
    }
    return okStart && okEnd;
}
}

AudioPrefetchStream::AudioPrefetchStream() {
    m_tracks.resize(kTrackCount);
}

AudioPrefetchStream::~AudioPrefetchStream() {
    stop();
}

void AudioPrefetchStream::setInterruptCallback(InterruptCallback callback) {
    m_interruptCallback = std::move(callback);
    if (m_access) {
        m_access->setInterruptCallback([this]() {
            return shouldAbort();
        });
    }
}

bool AudioPrefetchStream::open(const QUrl& url) {
    stop();

    m_access = std::make_unique<AudioStreamAccess>();
    m_access->setInterruptCallback([this]() {
        return shouldAbort();
    });

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_url = url;
        m_opened = true;
        m_stopRequested = false;
        m_eof = false;
        m_error = false;
        m_upstreamPaused = false;
        m_canSeek = true;
        m_contentLength = -1;
        m_lastError.clear();
        m_generation = 0;
        m_replyGeneration = -1;
        resetTracksLocked(0);
    }

    m_worker = std::thread([this]() {
        workerLoop();
    });
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] fast_open_no_head"
             << "url=" << url.toString();
    return true;
}

void AudioPrefetchStream::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = true;
        m_opened = false;
        ++m_generation;
    }
    m_dataCv.notify_all();
    m_spaceCv.notify_all();
    waitForWorkerStopped();
    m_access.reset();
}

int AudioPrefetchStream::read(uint8_t* dst, int size) {
    if (!dst || size <= 0) {
        return ReadError;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    m_lastInterrupted = false;
    const qint64 waitStartMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
    bool waitedForData = false;

    while (true) {
        if (shouldAbort()) {
            m_lastInterrupted = true;
            return ReadInterrupted;
        }

        int trackIndex = findTrackContainingLocked(m_pos);
        while (trackIndex < 0 && !m_eof && !m_error && !m_stopRequested && !shouldAbort()) {
            m_spaceCv.notify_one();
            m_dataCv.wait_for(lock, std::chrono::milliseconds(kWorkerIdleWaitMs));
            waitedForData = true;
            trackIndex = findTrackContainingLocked(m_pos);
        }

        if (shouldAbort() || m_stopRequested) {
            m_lastInterrupted = true;
            return ReadInterrupted;
        }
        if (trackIndex < 0) {
            if (m_eof) {
                return 0;
            }
            return ReadError;
        }

        m_currentTrack = trackIndex;
        Track& track = m_tracks[m_currentTrack];
        track.lastUse = ++m_tick;
        const qint64 offsetInTrack = m_pos - track.start;
        if (offsetInTrack < 0 || offsetInTrack >= track.data.size()) {
            m_dataCv.wait_for(lock, std::chrono::milliseconds(kWorkerIdleWaitMs));
            waitedForData = true;
            continue;
        }

        const int available = track.data.size() - static_cast<int>(offsetInTrack);
        const int toCopy = qMin(size, available);
        std::memcpy(dst, track.data.constData() + offsetInTrack, static_cast<size_t>(toCopy));
        m_pos += toCopy;

        if (cachedAheadLocked() < m_lowWaterBytes) {
            m_spaceCv.notify_one();
        }

        if (waitedForData) {
            const qint64 nowMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
            const qint64 waitedMs = nowMs - waitStartMs;
            if (waitedMs >= 100 || nowMs - m_lastReadWaitLogMs >= 1000) {
                m_lastReadWaitLogMs = nowMs;
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] read_wait_ms="
                         << waitedMs << "read_partial_bytes=" << toCopy
                         << "requested=" << size
                         << "cacheAhead=" << cachedAheadLocked();
            }
        }

        return toCopy;
    }
}

bool AudioPrefetchStream::seek(qint64 offset) {
    if (offset < 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_opened || shouldAbort()) {
        m_lastInterrupted = shouldAbort();
        return false;
    }

    offset = resolveSeekOffsetLocked(offset);
    const int cachedIndex = findTrackContainingLocked(offset);
    if (cachedIndex >= 0) {
        m_currentTrack = cachedIndex;
        m_tracks[m_currentTrack].lastUse = ++m_tick;
        m_pos = offset;
        if (cachedAheadLocked() < m_lowWaterBytes) {
            m_spaceCv.notify_one();
        }
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_cache_hit=true"
                 << "offset=" << offset << "track=" << cachedIndex;
        return true;
    }

    const Track& current = m_tracks[m_currentTrack];
    const bool canSkipForward = offset >= trackEnd(current) &&
                                offset <= trackEnd(current) + m_seekThresholdBytes;
    if (!m_canSeek && !canSkipForward) {
        m_lastError = QStringLiteral("Stream is not seekable");
        return false;
    }

    ++m_generation;
    m_eof = false;
    m_error = false;
    m_reconnectPending = true;
    m_abortReplyRequested = true;
    m_retryCount = 0;
    resetTracksLocked(offset);
    m_replyStartOffset = offset;
    m_lastAppendOffset = offset;
    m_lastAppendLogOffset = offset;
    m_spaceCv.notify_one();
    m_dataCv.notify_all();

    qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_cache_hit=false hard_seek=true"
             << "offset=" << offset << "generation=" << m_generation;
    return true;
}

void AudioPrefetchStream::prepareSeekHint(qint64 targetMs, qint64 targetByte,
                                          qint64 contentLength, bool exact) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (targetMs < 0 || targetByte < 0) {
        m_hasPendingSeekHint = false;
        return;
    }

    m_hasPendingSeekHint = true;
    m_pendingSeekHintExact = exact;
    m_pendingSeekHintTargetMs = targetMs;
    m_pendingSeekHintByte = targetByte;
    m_pendingSeekHintContentLength = contentLength;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_hint_prepared"
             << "targetMs=" << targetMs << "targetByte=" << targetByte
             << "contentLength=" << contentLength << "exact=" << exact;
}

bool AudioPrefetchStream::forceSeekToHint(qint64 targetMs, qint64 targetByte,
                                          qint64 contentLength, bool exact) {
    if (targetMs < 0 || targetByte < 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_opened || shouldAbort()) {
        m_lastInterrupted = shouldAbort();
        return false;
    }

    if (contentLength > 0) {
        m_contentLength = contentLength;
        AudioCacheManager::instance().noteTrackContentLength(m_url, contentLength);
    }
    if (m_contentLength > 0 && targetByte >= m_contentLength) {
        targetByte = m_contentLength - 1;
    }
    if (targetByte < 0) {
        targetByte = 0;
    }

    ++m_generation;
    m_eof = false;
    m_error = false;
    m_canSeek = true;
    m_reconnectPending = true;
    m_abortReplyRequested = true;
    m_retryCount = 0;
    m_hasPendingSeekHint = false;
    resetTracksLocked(targetByte);
    m_replyStartOffset = targetByte;
    m_lastAppendOffset = targetByte;
    m_lastAppendLogOffset = targetByte;
    m_firstByteStartMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
    m_spaceCv.notify_one();
    m_dataCv.notify_all();

    qDebug() << "[STREAM_EVT][AudioPrefetchStream] forced_byte_seek"
             << "targetMs=" << targetMs << "targetByte=" << targetByte
             << "contentLength=" << m_contentLength << "exact=" << exact
             << "generation=" << m_generation;
    return true;
}

qint64 AudioPrefetchStream::position() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pos;
}

qint64 AudioPrefetchStream::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_contentLength;
}

bool AudioPrefetchStream::canSeek() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_canSeek;
}

QString AudioPrefetchStream::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_lastError.isEmpty()) {
        return m_lastError;
    }
    return m_access ? m_access->lastError() : QString();
}

void AudioPrefetchStream::workerLoop() {
    m_streamTimer.start();
    m_networkManager = std::make_unique<QNetworkAccessManager>();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workerRunning = true;
        m_reconnectPending = true;
        m_retryCount = 0;
        m_firstByteStartMs = m_streamTimer.elapsed();
    }

    while (true) {
        bool shouldStart = false;
        bool shouldPauseUpstream = false;
        bool shouldAbortReply = false;
        bool hasActiveReply = false;
        int waitForReconnectMs = 0;
        qint64 startOffset = 0;
        int startGeneration = 0;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_reply) {
                m_spaceCv.wait_for(lock, std::chrono::milliseconds(m_workerWaitMs), [this]() {
                    return m_stopRequested || shouldAbort() || m_reconnectPending ||
                           m_abortReplyRequested ||
                           (!m_eof && !m_error && !m_reply &&
                            cachedAheadLocked() < m_bufferMaxBytes);
                });
            }

            if (m_stopRequested || shouldAbort()) {
                break;
            }

            hasActiveReply = m_reply != nullptr;
            shouldAbortReply = m_abortReplyRequested;
            m_abortReplyRequested = false;
            shouldPauseUpstream = m_reply && cachedAheadLocked() >= m_bufferMaxBytes;
            const bool wantsStart = !m_reply && !m_eof && !m_error &&
                                    (m_reconnectPending ||
                                     cachedAheadLocked() < m_bufferMaxBytes);
            if (wantsStart && m_reconnectNotBeforeMs > m_streamTimer.elapsed()) {
                waitForReconnectMs = static_cast<int>(
                    qMin<qint64>(m_reconnectNotBeforeMs - m_streamTimer.elapsed(), 250));
            } else {
                shouldStart = wantsStart;
            }
            startOffset = m_prefetchOffset;
            startGeneration = m_generation;
        }

        if (waitForReconnectMs > 0) {
            pumpNetworkEvents(waitForReconnectMs);
            continue;
        }

        if (shouldAbortReply) {
            closeContinuousRequest(true);
            continue;
        }

        if (shouldPauseUpstream) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_upstreamPaused = true;
            }
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] upstream_pause"
                     << "offset=" << startOffset;
            closeContinuousRequest(true);
            continue;
        }

        if (shouldStart && !startContinuousRequest(startOffset, startGeneration)) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_error = true;
            m_dataCv.notify_all();
            continue;
        }

        pumpNetworkEvents(hasActiveReply ? m_workerWaitMs : 0);

        bool finished = false;
        bool retry = false;
        bool eof = false;
        QString retryReason;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            finished = m_reply && m_replyFinished;
            if (finished) {
                const bool noNetworkError = m_reply->error() == QNetworkReply::NoError;
                const int status = m_statusCode;
                eof = noNetworkError && (status == 200 || status == 206 || status == 0) &&
                      (m_contentLength < 0 || m_prefetchOffset >= m_contentLength);
                if (noNetworkError && status == 416) {
                    eof = true;
                }
                retry = !eof && shouldRetryStatus(status);
                if (!retry && !eof && !noNetworkError) {
                    retry = shouldRetryStatus(0);
                }
                retryReason = noNetworkError
                                  ? QStringLiteral("status=%1").arg(status)
                                  : m_reply->errorString();
            }
        }

        if (!finished) {
            continue;
        }

        closeContinuousRequest(false);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopRequested || shouldAbort()) {
                break;
            }
            if (eof) {
                m_eof = true;
                m_retryCount = 0;
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] upstream_eof"
                         << "offset=" << m_prefetchOffset;
                m_dataCv.notify_all();
                continue;
            }
            if (retry && m_retryCount < m_maxRetryCount) {
                scheduleReconnectLocked(retryReason);
                continue;
            }
            m_error = true;
            m_lastError = retryReason.isEmpty()
                              ? QStringLiteral("Continuous stream finished unexpectedly")
                              : retryReason;
            qWarning() << "[STREAM_EVT][AudioPrefetchStream] upstream_failed"
                       << "offset=" << m_prefetchOffset << "error=" << m_lastError;
            m_dataCv.notify_all();
        }
    }

    closeContinuousRequest(true);
    m_networkManager.reset();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workerRunning = false;
        m_dataCv.notify_all();
    }
}

bool AudioPrefetchStream::startContinuousRequest(qint64 offset, int generation) {
    if (!m_networkManager || offset < 0) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (generation != m_generation) {
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] stale_start_ignored"
                     << "requestGeneration=" << generation
                     << "currentGeneration=" << m_generation
                     << "offset=" << offset;
            return true;
        }
        if (m_contentLength >= 0 && offset >= m_contentLength) {
            m_eof = true;
            m_dataCv.notify_all();
            return true;
        }
    }

    QNetworkRequest request(m_url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("FFmpegMusicPlayer/2.0"));
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Range", "bytes=" + QByteArray::number(offset) + "-");

    QNetworkReply* reply = m_networkManager->get(request);
    bool wasPaused = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stopRequested || shouldAbort()) {
            reply->abort();
            reply->deleteLater();
            return true;
        }
        if (generation != m_generation) {
            reply->abort();
            reply->deleteLater();
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] stale_start_aborted"
                     << "requestGeneration=" << generation
                     << "currentGeneration=" << m_generation
                     << "offset=" << offset;
            return true;
        }
        m_reply = reply;
        m_replyGeneration = generation;
        wasPaused = m_upstreamPaused;
        m_upstreamPaused = false;
        m_replyStartOffset = offset;
        m_lastAppendOffset = offset;
        m_lastAppendLogOffset = offset;
        m_replyHeadersValidated = false;
        m_replyFinished = false;
        m_reconnectPending = false;
        m_abortReplyRequested = false;
        m_reconnectNotBeforeMs = 0;
        m_statusCode = 0;
        m_error = false;
        m_lastError.clear();
        if (cachedAheadLocked() == 0 && offset == m_pos) {
            m_firstByteStartMs = m_streamTimer.elapsed();
        }
    }

    const int replyGeneration = generation;
    const qint64 replyStartOffset = offset;
    QObject::connect(reply, &QNetworkReply::readyRead, reply,
                     [this, reply, replyGeneration, replyStartOffset]() {
        bool abortReply = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (reply != m_reply || replyGeneration != m_replyGeneration ||
                replyGeneration != m_generation ||
                replyStartOffset != m_replyStartOffset) {
                const QByteArray discarded = reply->readAll();
                if (!discarded.isEmpty()) {
                    qDebug() << "[STREAM_EVT][AudioPrefetchStream] stale_reply_ignored"
                             << "bytes=" << discarded.size()
                             << "replyGeneration=" << replyGeneration
                             << "currentGeneration=" << m_generation
                             << "replyStart=" << replyStartOffset
                             << "activeStart=" << m_replyStartOffset;
                }
                return;
            }
            if (!m_replyHeadersValidated && !validateContinuousHeaders()) {
                abortReply = true;
            }
        }
        if (abortReply) {
            reply->abort();
            return;
        }

        const QByteArray data = reply->readAll();
        if (!data.isEmpty()) {
            appendNetworkData(data, replyGeneration, replyStartOffset);
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, reply,
                     [this, reply, replyGeneration, replyStartOffset]() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (reply == m_reply && replyGeneration == m_replyGeneration &&
                replyGeneration == m_generation && replyStartOffset == m_replyStartOffset) {
                if (!m_replyHeadersValidated) {
                    validateContinuousHeaders();
                }
                m_statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                m_replyFinished = true;
            } else {
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] stale_reply_finished_ignored"
                         << "replyGeneration=" << replyGeneration
                         << "currentGeneration=" << m_generation
                         << "replyStart=" << replyStartOffset
                         << "activeStart=" << m_replyStartOffset;
            }
        }
        m_dataCv.notify_all();
        m_spaceCv.notify_all();
    });

    qDebug() << "[STREAM_EVT][AudioPrefetchStream]"
             << (wasPaused ? "upstream_resume" : "upstream_open")
             << "offset=" << offset << "range=bytes=" + QByteArray::number(offset) + "-";
    return true;
}

void AudioPrefetchStream::closeContinuousRequest(bool abortReply) {
    QNetworkReply* reply = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        reply = m_reply;
        m_reply = nullptr;
        m_replyGeneration = -1;
        m_replyHeadersValidated = false;
        m_replyFinished = false;
        m_statusCode = 0;
    }

    if (!reply) {
        return;
    }
    if (abortReply && reply->isRunning()) {
        reply->abort();
    }
    reply->deleteLater();
    pumpNetworkEvents(0);
}

void AudioPrefetchStream::pumpNetworkEvents(int maxWaitMs) {
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(qMax(0, maxWaitMs));
    loop.exec();
}

void AudioPrefetchStream::appendNetworkData(const QByteArray& data, int generation,
                                            qint64 replyStartOffset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_reply || data.isEmpty() || m_stopRequested || shouldAbort()) {
        return;
    }
    if (generation != m_replyGeneration || generation != m_generation ||
        replyStartOffset != m_replyStartOffset) {
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] stale_append_ignored"
                 << "bytes=" << data.size()
                 << "replyGeneration=" << generation
                 << "currentGeneration=" << m_generation
                 << "replyStart=" << replyStartOffset
                 << "activeStart=" << m_replyStartOffset
                 << "lastAppend=" << m_lastAppendOffset;
        return;
    }

    int trackIndex = findTrackAppendableLocked(m_lastAppendOffset);
    if (trackIndex < 0) {
        trackIndex = chooseReusableTrackLocked();
        m_tracks[trackIndex].start = m_lastAppendOffset;
        m_tracks[trackIndex].data.clear();
    }

    Track& track = m_tracks[trackIndex];
    if (trackEnd(track) != m_lastAppendOffset) {
        track.start = m_lastAppendOffset;
        track.data.clear();
    }
    track.data.append(data);
    track.lastUse = ++m_tick;
    m_lastAppendOffset += data.size();
    m_prefetchOffset = m_lastAppendOffset;
    trimTrackLocked(track);
    m_error = false;
    m_retryCount = 0;

    if (m_firstByteStartMs >= 0) {
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] prefetch_first_byte_ms="
                 << (m_streamTimer.elapsed() - m_firstByteStartMs)
                 << "bytes=" << data.size()
                 << "offset=" << track.start;
        m_firstByteStartMs = -1;
    }
    if (m_prefetchOffset - m_lastAppendLogOffset >= 256LL * 1024 ||
        m_prefetchOffset - m_replyStartOffset <= data.size()) {
        m_lastAppendLogOffset = m_prefetchOffset;
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] append_bytes"
                 << "bytes=" << data.size()
                 << "offset=" << m_prefetchOffset
                 << "cacheAhead=" << cachedAheadLocked();
    }
    if ((m_prefetchOffset - m_replyStartOffset) % (2LL * 1024 * 1024) < data.size()) {
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] continuous_bytes"
                 << "replyStart=" << m_replyStartOffset
                 << "offset=" << m_prefetchOffset
                 << "cacheAhead=" << cachedAheadLocked();
    }
    m_dataCv.notify_all();
}

bool AudioPrefetchStream::validateContinuousHeaders() {
    if (!m_reply || m_replyHeadersValidated) {
        return true;
    }

    m_statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (m_statusCode == 206) {
        qint64 rangeStart = -1;
        qint64 rangeEnd = -1;
        qint64 rangeTotal = -1;
        if (!parseContentRange(m_reply->rawHeader("Content-Range"), &rangeStart, &rangeEnd,
                               &rangeTotal) ||
            rangeStart != m_replyStartOffset) {
            m_error = true;
            m_lastError = QStringLiteral("Content-Range start mismatch");
            qWarning() << "[STREAM_EVT][AudioPrefetchStream] upstream_header_invalid"
                       << "requested=" << m_replyStartOffset
                       << "contentRange=" << m_reply->rawHeader("Content-Range");
            m_dataCv.notify_all();
            return false;
        }
        m_canSeek = true;
        if (rangeTotal > 0) {
            m_contentLength = rangeTotal;
            AudioCacheManager::instance().noteTrackContentLength(m_url, rangeTotal);
        }
    } else if (m_statusCode == 200) {
        if (m_replyStartOffset != 0) {
            m_error = true;
            m_lastError = QStringLiteral("Server ignored Range request");
            qWarning() << "[STREAM_EVT][AudioPrefetchStream] upstream_header_invalid"
                       << "requested=" << m_replyStartOffset << "status=200";
            m_dataCv.notify_all();
            return false;
        }
        m_canSeek = false;
    } else if (m_statusCode >= 400) {
        return true;
    }

    m_replyHeadersValidated = true;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] upstream_headers"
             << "status=" << m_statusCode
             << "start=" << m_replyStartOffset
             << "contentRange=" << m_reply->rawHeader("Content-Range");
    return true;
}

bool AudioPrefetchStream::shouldRetryStatus(int statusCode) const {
    return statusCode == 0 || statusCode == 408 || statusCode == 429 ||
           statusCode == 500 || statusCode == 502 || statusCode == 503 ||
           statusCode == 504;
}

void AudioPrefetchStream::scheduleReconnectLocked(const QString& reason) {
    ++m_retryCount;
    m_reconnectPending = true;
    const qint64 delayMs = qMin<qint64>(2000, 120LL * (1LL << qMin(m_retryCount - 1, 4)));
    m_reconnectNotBeforeMs = m_streamTimer.elapsed() + delayMs;
    m_error = false;
    qWarning() << "[STREAM_EVT][AudioPrefetchStream] upstream_reconnect"
               << "retry=" << m_retryCount << "/" << m_maxRetryCount
               << "offset=" << m_prefetchOffset << "delayMs=" << delayMs
               << "reason=" << reason;
}

void AudioPrefetchStream::resetTracksLocked(qint64 offset) {
    for (Track& track : m_tracks) {
        track.start = offset;
        track.data.clear();
        track.lastUse = 0;
    }
    m_currentTrack = 0;
    m_tracks[m_currentTrack].lastUse = ++m_tick;
    m_pos = offset;
    m_prefetchOffset = offset;
    m_replyStartOffset = offset;
    m_lastAppendOffset = offset;
    m_lastAppendLogOffset = offset;
}

int AudioPrefetchStream::findTrackContainingLocked(qint64 offset) const {
    for (int i = 0; i < m_tracks.size(); ++i) {
        const Track& track = m_tracks[i];
        if (!track.data.isEmpty() && track.start <= offset && offset < trackEnd(track)) {
            return i;
        }
    }
    return -1;
}

int AudioPrefetchStream::findTrackAppendableLocked(qint64 offset) const {
    for (int i = 0; i < m_tracks.size(); ++i) {
        const Track& track = m_tracks[i];
        if (!track.data.isEmpty() && offset >= track.start && offset <= trackEnd(track)) {
            return i;
        }
    }
    return -1;
}

int AudioPrefetchStream::chooseReusableTrackLocked() const {
    int candidate = 0;
    qint64 oldest = std::numeric_limits<qint64>::max();
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (i == m_currentTrack && m_tracks.size() > 1) {
            continue;
        }
        if (m_tracks[i].lastUse < oldest) {
            oldest = m_tracks[i].lastUse;
            candidate = i;
        }
    }
    return candidate;
}

qint64 AudioPrefetchStream::trackEnd(const Track& track) const {
    return track.start + track.data.size();
}

qint64 AudioPrefetchStream::cachedAheadLocked() const {
    const int trackIndex = findTrackContainingLocked(m_pos);
    if (trackIndex < 0) {
        return 0;
    }
    return qMax<qint64>(0, trackEnd(m_tracks[trackIndex]) - m_pos);
}

qint64 AudioPrefetchStream::resolveSeekOffsetLocked(qint64 requestedOffset) {
    if (!m_hasPendingSeekHint || m_pendingSeekHintByte < 0) {
        return requestedOffset;
    }

    const qint64 hintedOffset = m_pendingSeekHintByte;
    const qint64 total = m_pendingSeekHintContentLength > 0
                             ? m_pendingSeekHintContentLength
                             : m_contentLength;
    if (total > 0 && hintedOffset >= total) {
        m_hasPendingSeekHint = false;
        return requestedOffset;
    }

    const qint64 tolerance = m_pendingSeekHintExact ? 16LL * 1024 * 1024 : 2LL * 1024 * 1024;
    if (qAbs(requestedOffset - hintedOffset) > tolerance) {
        return requestedOffset;
    }

    m_hasPendingSeekHint = false;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_hint_byte_applied"
             << "requested=" << requestedOffset << "applied=" << hintedOffset
             << "targetMs=" << m_pendingSeekHintTargetMs << "exact=" << m_pendingSeekHintExact;
    return hintedOffset;
}

void AudioPrefetchStream::trimTrackLocked(Track& track) {
    if (track.data.size() <= m_trackMaxBytes) {
        return;
    }

    const qint64 maxRemovableHistory = qMax<qint64>(0, m_pos - track.start);
    const int removeBytes = static_cast<int>(
        qMin<qint64>(track.data.size() - m_trackMaxBytes, maxRemovableHistory));
    if (removeBytes <= 0) {
        return;
    }
    track.data.remove(0, removeBytes);
    track.start += removeBytes;
}

bool AudioPrefetchStream::shouldAbort() const {
    return m_interruptCallback && m_interruptCallback();
}

void AudioPrefetchStream::waitForWorkerStopped() {
    if (m_worker.joinable()) {
        m_worker.join();
    }
}
