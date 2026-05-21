#include "AudioPrefetchStream.h"

#include "AudioCacheManager.h"

#include <QVector>
#include <QtDebug>
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
constexpr int kTrackCount = 3;
constexpr int kWorkerIdleWaitMs = 20;
constexpr qint64 kStartupReadGateBytes = 32LL * 1024;
constexpr qint64 kFastSeekReadGateBytes = 16LL * 1024;
constexpr qint64 kFlacColdStartupReadGateBytes = 96LL * 1024;
constexpr qint64 kFlacSeekReadGateBytes = 48LL * 1024;
constexpr qint64 kFlacSteadyReadGateBytes = 96LL * 1024;
constexpr int kStartupReadGateTimeoutMs = 120;
constexpr int kFlacStartupReadGateTimeoutMs = 220;
constexpr int kFastSeekReadGateTimeoutMs = 40;
constexpr int kFlacSeekReadGateTimeoutMs = 90;
constexpr int kFlacSteadyReadGateTimeoutMs = 0;
constexpr qint64 kInflightSeekCoalesceBytes = 1024LL * 1024;
constexpr qint64 kFlacInflightSeekCoalesceBytes = 4LL * 1024 * 1024;
constexpr qint64 kFlacInflightBackwardProbeBytes = 768LL * 1024;
constexpr qint64 kFlacInflightOldIntervalBytes = 4LL * 1024 * 1024;
constexpr qint64 kExactSeekHintToleranceBytes = 256LL * 1024;
constexpr qint64 kFlacExactSeekHintToleranceBytes = 768LL * 1024;
constexpr qint64 kSlowThroughputKiBps = 320;
constexpr qint64 kVerboseStreamLogIntervalMs = 5000;
constexpr bool kVerboseStreamChunkLogs = false;

bool isFlacLikeUrl(const QUrl& url) {
    return url.path().toLower().endsWith(QStringLiteral(".flac"));
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
    if (m_curlBackend) {
        m_curlBackend->setInterruptCallback([this]() {
            return shouldAbort();
        });
    }
}

bool AudioPrefetchStream::open(const QUrl& url) {
    stop();
    m_stopSignal = false;

    if (AudioCurlStreamBackend::isAvailable()) {
        auto curlBackend = std::make_shared<AudioCurlStreamBackend>();
        curlBackend->setInterruptCallback([this]() {
            return shouldAbort();
        });
        if (curlBackend->open(url)) {
            m_curlBackend = std::move(curlBackend);
        } else {
            qWarning() << "[STREAM_EVT][AudioPrefetchStream] curl_backend_unavailable"
                       << "error=" << curlBackend->lastError();
        }
    }
    if (!m_curlBackend) {
        qWarning() << "[STREAM_EVT][AudioPrefetchStream] open_failed"
                   << "error=libcurl backend unavailable";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_url = url;
        m_flacLike = isFlacLikeUrl(url);
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
        m_curlOpenCount = 0;
        m_coalesceCount = 0;
        m_hardSeekCount = 0;
        m_backendAbortCount = 0;
        m_totalBackendBytes = 0;
        m_lastBackendChunkBytes = 0;
        m_lastFetchElapsedMs = -1;
        m_lastFetchTtfbMs = -1;
        m_lastFetchAvgKiBps = 0;
        resetTracksLocked(0);
    }

    m_worker = std::thread([this]() {
        workerLoop();
    });
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] fast_open_no_head"
             << "url=" << url.toString() << "flacLike=" << m_flacLike;
    return true;
}

void AudioPrefetchStream::stop() {
    m_stopSignal = true;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = true;
        m_opened = false;
        ++m_generation;
    }
    if (m_curlBackend) {
        m_curlBackend->abort();
    }
    m_dataCv.notify_all();
    m_spaceCv.notify_all();
    waitForWorkerStopped();
    m_curlBackend.reset();
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

        const qint64 nowMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
        const qint64 availableAhead = trackEnd(track) - m_pos;
        if (shouldHoldReadGateLocked(availableAhead, nowMs) ||
            (m_readGateOffset < 0 &&
             shouldHoldFlacSteadyReadLocked(availableAhead, nowMs, waitStartMs))) {
            m_spaceCv.notify_one();
            if (kVerboseStreamChunkLogs && nowMs - m_lastReadGateLogMs >= kVerboseStreamLogIntervalMs) {
                m_lastReadGateLogMs = nowMs;
                const bool startupGateActive = m_readGateOffset >= 0;
                const qint64 remainingMs =
                    startupGateActive ? (m_readGateDeadlineMs - nowMs)
                                      : (kFlacSteadyReadGateTimeoutMs - (nowMs - waitStartMs));
                const qint64 requiredBytes =
                    startupGateActive ? m_readGateMinBytes
                                      : readGateTargetBytesLocked(ReadGateKind::Steady);
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] read_gate_wait"
                          << "pos=" << m_pos << "availableAhead=" << availableAhead
                          << "required=" << requiredBytes
                          << "kind="
                          << readGateKindName(startupGateActive ? m_readGateKind
                                                                : ReadGateKind::Steady)
                          << "remainingMs=" << remainingMs
                          << "ttfbMs=" << m_lastFetchTtfbMs
                          << "fetchElapsedMs=" << m_lastFetchElapsedMs
                          << "avgKiBps=" << m_lastFetchAvgKiBps;
            }
            m_dataCv.wait_for(lock, std::chrono::milliseconds(kWorkerIdleWaitMs));
            waitedForData = true;
            continue;
        }
        if (m_readGateOffset >= 0) {
            const bool timedOut = m_pos == m_readGateOffset &&
                                  availableAhead < m_readGateMinBytes &&
                                  nowMs >= m_readGateDeadlineMs;
            clearReadGateLocked(timedOut ? "timeout" : "ready", availableAhead);
        }

        const int available = track.data.size() - static_cast<int>(offsetInTrack);
        const int toCopy = qMin(size, available);
        std::memcpy(dst, track.data.constData() + offsetInTrack, static_cast<size_t>(toCopy));
        m_pos += toCopy;

        if (cachedAheadLocked() < lowWaterBytesLocked()) {
            m_spaceCv.notify_one();
        }

        if (waitedForData) {
            const qint64 nowMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
            const qint64 waitedMs = nowMs - waitStartMs;
            if (waitedMs >= 800 && nowMs - m_lastReadWaitLogMs >= kVerboseStreamLogIntervalMs) {
                m_lastReadWaitLogMs = nowMs;
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] read_wait_ms="
                          << waitedMs << "read_partial_bytes=" << toCopy
                          << "requested=" << size
                          << "cacheAhead=" << cachedAheadLocked()
                          << "flacLike=" << m_flacLike;
            }
        }

        return toCopy;
    }
}

bool AudioPrefetchStream::seek(qint64 offset) {
    if (offset < 0) {
        return false;
    }

    std::shared_ptr<AudioCurlStreamBackend> backendToAbort;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_opened || shouldStopWorkerLocked()) {
            m_lastInterrupted = shouldStopWorkerLocked();
            return false;
        }

        offset = resolveSeekOffsetLocked(offset);
        const int cachedIndex = findTrackContainingLocked(offset);
        if (cachedIndex >= 0) {
            m_currentTrack = cachedIndex;
            m_tracks[m_currentTrack].lastUse = ++m_tick;
            m_pos = offset;
            clearReadGateLocked("cache_hit", cachedAheadLocked());
            if (cachedAheadLocked() < lowWaterBytesLocked()) {
                m_spaceCv.notify_one();
            }
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_cache_hit=true"
                     << "offset=" << offset << "track=" << cachedIndex;
            return true;
        }

        if (seekForwardByReadingLocked(offset)) {
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_forward_by_read=true"
                     << "offset=" << offset << "cacheAhead=" << cachedAheadLocked()
                     << "generation=" << m_generation;
            return true;
        }

        if (coalesceInflightSeekLocked(offset)) {
            return true;
        }

        if (!m_canSeek) {
            m_lastError = QStringLiteral("Stream is not seekable");
            return false;
        }

        ++m_generation;
        ++m_hardSeekCount;
        m_eof = false;
        m_error = false;
        m_reconnectPending = true;
        m_abortReplyRequested = true;
        backendToAbort = m_curlBackend;
        if (backendToAbort) {
            backendToAbort->abort();
        }
        ++m_backendAbortCount;
        m_retryCount = 0;
        resetTracksLocked(offset);
        m_replyStartOffset = offset;
        m_lastAppendOffset = offset;
        m_lastAppendLogOffset = offset;
        setReadGateLocked(offset, ReadGateKind::Seek);
        m_spaceCv.notify_one();
        m_dataCv.notify_all();

        qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_cache_hit=false hard_seek=true"
                 << "offset=" << offset << "generation=" << m_generation
                 << "curl_open_count=" << m_curlOpenCount
                 << "hard_seek_count=" << m_hardSeekCount
                 << "coalesce_count=" << m_coalesceCount
                 << "backend_abort_count=" << m_backendAbortCount
                 << "gateBytes=" << m_readGateMinBytes
                 << "gateKind=" << readGateKindName(m_readGateKind);
    }

    return true;
}

void AudioPrefetchStream::prepareSeekHint(qint64 targetMs, qint64 targetByte,
                                          qint64 contentLength, bool timeExact,
                                          bool allowRangeAnchor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (targetMs < 0 || targetByte < 0) {
        m_hasPendingSeekHint = false;
        return;
    }

    m_hasPendingSeekHint = true;
    m_pendingSeekHintTimeExact = timeExact;
    m_pendingSeekHintAllowRangeAnchor = allowRangeAnchor;
    m_pendingSeekHintTargetMs = targetMs;
    m_pendingSeekHintByte = targetByte;
    m_pendingSeekHintContentLength = contentLength;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_hint_prepared"
             << "targetMs=" << targetMs << "targetByte=" << targetByte
             << "contentLength=" << contentLength << "timeExact=" << timeExact
             << "allowRangeAnchor=" << allowRangeAnchor;
}

bool AudioPrefetchStream::forceSeekToHint(qint64 targetMs, qint64 targetByte,
                                          qint64 contentLength, bool timeExact,
                                          bool allowRangeAnchor) {
    if (!timeExact && !allowRangeAnchor) {
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] forced_byte_seek_skipped_non_exact"
                 << "targetMs=" << targetMs << "targetByte=" << targetByte
                 << "contentLength=" << contentLength
                 << "allowRangeAnchor=" << allowRangeAnchor;
        return false;
    }
    if (targetMs < 0 || targetByte < 0) {
        return false;
    }

    std::shared_ptr<AudioCurlStreamBackend> backendToAbort;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_opened || shouldStopWorkerLocked()) {
            m_lastInterrupted = shouldStopWorkerLocked();
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
        backendToAbort = m_curlBackend;
        if (backendToAbort) {
            backendToAbort->abort();
        }
        ++m_backendAbortCount;
        ++m_hardSeekCount;
        m_retryCount = 0;
        m_hasPendingSeekHint = false;
        resetTracksLocked(targetByte);
        m_replyStartOffset = targetByte;
        m_lastAppendOffset = targetByte;
        m_lastAppendLogOffset = targetByte;
        m_firstByteStartMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
        setReadGateLocked(targetByte, ReadGateKind::Seek);
        m_spaceCv.notify_one();
        m_dataCv.notify_all();

        qDebug() << "[STREAM_EVT][AudioPrefetchStream] forced_byte_seek"
                 << "targetMs=" << targetMs << "targetByte=" << targetByte
                 << "contentLength=" << m_contentLength << "timeExact=" << timeExact
                 << "allowRangeAnchor=" << allowRangeAnchor
                 << "flac_range_reopen=" << (allowRangeAnchor && !timeExact)
                 << "generation=" << m_generation
                 << "curl_open_count=" << m_curlOpenCount
                 << "hard_seek_count=" << m_hardSeekCount
                 << "coalesce_count=" << m_coalesceCount
                 << "backend_abort_count=" << m_backendAbortCount
                 << "gateBytes=" << m_readGateMinBytes
                 << "gateKind=" << readGateKindName(m_readGateKind);
        if (allowRangeAnchor && !timeExact) {
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] flac_range_reopen"
                     << "targetMs=" << targetMs
                     << "targetByte=" << targetByte
                     << "contentLength=" << m_contentLength
                     << "generation=" << m_generation
                     << "hard_seek_count=" << m_hardSeekCount;
        }
    }

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
    return m_lastError;
}

void AudioPrefetchStream::workerLoop() {
    if (m_curlBackend) {
        curlWorkerLoop();
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_error = true;
    m_lastError = QStringLiteral("libcurl backend unavailable");
    m_dataCv.notify_all();
}

void AudioPrefetchStream::curlWorkerLoop() {
    m_streamTimer.start();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workerRunning = true;
        m_reconnectPending = true;
        m_retryCount = 0;
        m_firstByteStartMs = m_streamTimer.elapsed();
        m_waitingForAbortClear = false;
        m_lastAbortReconnectLogMs = 0;
    }

    qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_worker_enabled";

    while (true) {
        bool shouldStart = false;
        int waitForReconnectMs = 0;
        qint64 startOffset = 0;
        int startGeneration = 0;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_spaceCv.wait_for(lock, std::chrono::milliseconds(m_workerWaitMs), [this]() {
                const bool waitingForTransientAbort =
                    m_waitingForAbortClear && hasTransientAbortLocked();
                return shouldStopWorkerLocked() ||
                       (m_reconnectPending && !waitingForTransientAbort) ||
                       m_abortReplyRequested ||
                       (m_waitingForAbortClear && !waitingForTransientAbort) ||
                       (!waitingForTransientAbort && !m_eof && !m_error &&
                        cachedAheadLocked() < m_bufferMaxBytes);
            });

            if (shouldStopWorkerLocked()) {
                break;
            }

            if (m_abortReplyRequested) {
                markAbortReconnectPendingLocked("explicit_abort", m_replyStartOffset,
                                                m_replyGeneration);
                m_abortReplyRequested = false;
            }
            if (hasTransientAbortLocked()) {
                markAbortReconnectPendingLocked("wait_abort_clear", m_replyStartOffset,
                                                m_replyGeneration);
                continue;
            }
            clearAbortWaitLocked("abort_cleared_before_open");

            const bool wantsStart =
                !m_eof && !m_error &&
                (m_reconnectPending || cachedAheadLocked() < m_bufferMaxBytes);
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
            std::unique_lock<std::mutex> lock(m_mutex);
            m_dataCv.wait_for(lock, std::chrono::milliseconds(waitForReconnectMs), [this]() {
                return shouldStopWorkerLocked() || m_abortReplyRequested ||
                       (m_waitingForAbortClear && !hasTransientAbortLocked());
            });
            continue;
        }

        if (!shouldStart) {
            continue;
        }

        qint64 curlOpenCount = 0;
        qint64 hardSeekCount = 0;
        qint64 coalesceCount = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (shouldStopWorkerLocked()) {
                break;
            }
            if (hasTransientAbortLocked()) {
                markAbortReconnectPendingLocked("abort_before_open", startOffset,
                                                startGeneration);
                continue;
            }
            if (startGeneration != m_generation || startOffset != m_prefetchOffset) {
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_start_stale_ignored"
                         << "requestGeneration=" << startGeneration
                         << "currentGeneration=" << m_generation
                         << "offset=" << startOffset
                         << "activeOffset=" << m_prefetchOffset;
                continue;
            }
            m_reconnectPending = false;
            m_replyGeneration = startGeneration;
            m_replyStartOffset = startOffset;
            m_lastAppendOffset = startOffset;
            m_lastAppendLogOffset = startOffset;
            m_error = false;
            m_lastError.clear();
            if (cachedAheadLocked() == 0 && startOffset == m_pos) {
                m_firstByteStartMs = m_streamTimer.elapsed();
                setReadGateLocked(startOffset, startOffset == 0 ? ReadGateKind::Startup
                                                                : ReadGateKind::Seek);
            }
            curlOpenCount = ++m_curlOpenCount;
            hardSeekCount = m_hardSeekCount;
            coalesceCount = m_coalesceCount;
        }

        m_curlBackend->clearAbort();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (shouldStopWorkerLocked()) {
                break;
            }
            if (startGeneration != m_generation || startOffset != m_prefetchOffset ||
                m_abortReplyRequested) {
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_open_cancelled_before_fetch"
                         << "requestGeneration=" << startGeneration
                         << "currentGeneration=" << m_generation
                         << "offset=" << startOffset
                         << "activeOffset=" << m_prefetchOffset
                         << "abortRequested=" << m_abortReplyRequested;
                continue;
            }
        }
        const QByteArray rangeHeader = "bytes=" + QByteArray::number(startOffset) + "-";
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_upstream_open"
                 << "offset=" << startOffset << "range=" << rangeHeader
                 << "flacLike=" << m_flacLike
                 << "generation=" << startGeneration
                 << "curl_open_count=" << curlOpenCount
                 << "hard_seek_count=" << hardSeekCount
                 << "coalesce_count=" << coalesceCount;

        AudioCurlStreamBackend::Response response =
            m_curlBackend->fetchFrom(startOffset, [this, startGeneration,
                                                   startOffset](const QByteArray& data) {
                return appendBackendData(data, startGeneration, startOffset);
            });

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (shouldStopWorkerLocked()) {
                break;
            }
            if (m_upstreamPaused && startGeneration == m_generation &&
                startOffset == m_replyStartOffset) {
                m_upstreamPaused = false;
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_upstream_pause"
                         << "offset=" << m_prefetchOffset
                         << "cacheAhead=" << cachedAheadLocked();
                continue;
            }
            if (startGeneration != m_generation || startOffset != m_replyStartOffset) {
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_response_stale_ignored"
                         << "requestGeneration=" << startGeneration
                         << "currentGeneration=" << m_generation
                         << "replyStart=" << startOffset
                         << "activeStart=" << m_replyStartOffset;
                continue;
            }
            if (response.totalLength > 0) {
                m_contentLength = response.totalLength;
                AudioCacheManager::instance().noteTrackContentLength(m_url, response.totalLength);
            }
            m_lastFetchElapsedMs = response.elapsedMs;
            m_lastFetchTtfbMs = response.ttfbMs;
            m_lastFetchAvgKiBps = response.avgKiBps;
            if (response.statusCode == 206) {
                m_canSeek = true;
            }
            if (response.eof ||
                (response.ok && m_contentLength > 0 && m_prefetchOffset >= m_contentLength)) {
                m_eof = true;
                m_retryCount = 0;
                qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_upstream_eof"
                         << "offset=" << m_prefetchOffset
                         << "ttfbMs=" << response.ttfbMs
                         << "elapsedMs=" << response.elapsedMs
                         << "avgKiBps=" << response.avgKiBps
                         << "bytesReceived=" << response.bytesReceived
                         << "bytesDelivered=" << response.bytesDelivered;
                m_dataCv.notify_all();
                continue;
            }
            if (!response.ok || response.interrupted) {
                if (response.interrupted) {
                    m_lastInterrupted = true;
                    markAbortReconnectPendingLocked("fetch_interrupted", startOffset,
                                                    startGeneration);
                    m_dataCv.notify_all();
                    continue;
                }
                const bool madeProgress = m_prefetchOffset > startOffset;
                if (madeProgress) {
                    m_reconnectPending = true;
                    m_reconnectNotBeforeMs = m_streamTimer.elapsed();
                    m_error = false;
                    m_retryCount = 0;
                    qWarning() << "[STREAM_EVT][AudioPrefetchStream] short_read_resume"
                               << "from=" << startOffset << "resumeOffset=" << m_prefetchOffset
                               << "reason=" << response.error
                               << "ttfbMs=" << response.ttfbMs
                               << "elapsedMs=" << response.elapsedMs
                               << "avgKiBps=" << response.avgKiBps
                               << "bytesReceived=" << response.bytesReceived
                               << "bytesDelivered=" << response.bytesDelivered;
                    continue;
                }
                if (m_retryCount < m_maxRetryCount) {
                    scheduleReconnectLocked(response.error);
                    continue;
                }
                m_error = true;
                m_lastError = response.error.isEmpty()
                                  ? QStringLiteral("curl stream finished unexpectedly")
                                  : response.error;
                qWarning() << "[STREAM_EVT][AudioPrefetchStream] curl_upstream_failed"
                           << "offset=" << m_prefetchOffset << "error=" << m_lastError
                           << "ttfbMs=" << response.ttfbMs
                           << "elapsedMs=" << response.elapsedMs
                           << "avgKiBps=" << response.avgKiBps
                           << "bytesReceived=" << response.bytesReceived
                           << "bytesDelivered=" << response.bytesDelivered;
                m_dataCv.notify_all();
                continue;
            }
            m_retryCount = 0;
            qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_upstream_done"
                     << "offset=" << m_prefetchOffset
                     << "replyStart=" << m_replyStartOffset
                     << "generation=" << m_generation
                     << "ttfbMs=" << response.ttfbMs
                     << "elapsedMs=" << response.elapsedMs
                     << "avgKiBps=" << response.avgKiBps
                     << "bytesReceived=" << response.bytesReceived
                     << "bytesDelivered=" << response.bytesDelivered
                     << "totalBackendBytes=" << m_totalBackendBytes;
            m_spaceCv.notify_one();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workerRunning = false;
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] curl_worker_exit"
                 << "reason=" << (m_stopRequested ? "stop" : "loop_end")
                 << "generation=" << m_generation
                 << "prefetchOffset=" << m_prefetchOffset
                 << "waitingForAbortClear=" << m_waitingForAbortClear;
        m_dataCv.notify_all();
    }
}

bool AudioPrefetchStream::appendBackendData(const QByteArray& data, int generation,
                                            qint64 replyStartOffset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (data.isEmpty() || m_stopRequested || shouldAbort()) {
        return false;
    }
    if (cachedAheadLocked() >= m_bufferMaxBytes) {
        m_upstreamPaused = true;
        return false;
    }
    if (generation != m_replyGeneration || generation != m_generation ||
        replyStartOffset != m_replyStartOffset) {
        // Abort the obsolete transfer, but do not let its response mutate the active generation.
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] stale_backend_chunk_ignored"
                 << "bytes=" << data.size()
                 << "replyGeneration=" << generation
                 << "currentGeneration=" << m_generation
                 << "replyStart=" << replyStartOffset
                 << "activeStart=" << m_replyStartOffset
                 << "lastAppend=" << m_lastAppendOffset;
        return false;
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
    m_lastBackendChunkBytes = data.size();
    m_totalBackendBytes += data.size();
    trimTrackLocked(track);
    m_error = false;
    m_retryCount = 0;

    if (m_firstByteStartMs >= 0) {
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] prefetch_first_byte_ms="
                 << (m_streamTimer.elapsed() - m_firstByteStartMs)
                 << "bytes=" << data.size()
                 << "offset=" << track.start
                 << "generation=" << generation
                 << "replyStart=" << replyStartOffset
                 << "flacLike=" << m_flacLike;
        m_firstByteStartMs = -1;
    }
    if (kVerboseStreamChunkLogs &&
        (m_prefetchOffset - m_lastAppendLogOffset >= 2LL * 1024 * 1024 ||
         m_prefetchOffset - m_replyStartOffset <= data.size())) {
        m_lastAppendLogOffset = m_prefetchOffset;
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] append_bytes"
                 << "bytes=" << data.size()
                 << "offset=" << m_prefetchOffset
                 << "cacheAhead=" << cachedAheadLocked()
                 << "flacLike=" << m_flacLike
                 << "totalBackendBytes=" << m_totalBackendBytes
                 << "lastChunkBytes=" << m_lastBackendChunkBytes;
    }
    if (kVerboseStreamChunkLogs &&
        (m_prefetchOffset - m_replyStartOffset) % (8LL * 1024 * 1024) < data.size()) {
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] continuous_bytes"
                 << "replyStart=" << m_replyStartOffset
                 << "offset=" << m_prefetchOffset
                 << "cacheAhead=" << cachedAheadLocked();
    }
    m_dataCv.notify_all();
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

qint64 AudioPrefetchStream::lowWaterBytesLocked() const {
    return m_flacLike ? m_flacLowWaterBytes : m_lowWaterBytes;
}

qint64 AudioPrefetchStream::inflightSeekCoalesceBytesLocked() const {
    return m_flacLike ? kFlacInflightSeekCoalesceBytes : kInflightSeekCoalesceBytes;
}

qint64 AudioPrefetchStream::bufferedOrInflightEndLocked() const {
    qint64 end = m_lastAppendOffset;
    for (const Track& track : m_tracks) {
        if (track.start >= m_replyStartOffset && !track.data.isEmpty()) {
            end = qMax(end, trackEnd(track));
        }
    }
    return qMax(end, m_prefetchOffset);
}

void AudioPrefetchStream::setReadGateLocked(qint64 offset, ReadGateKind kind) {
    const qint64 minBytes = readGateTargetBytesLocked(kind);
    const int timeoutMs = readGateTimeoutMsLocked(kind);
    if (!m_streamTimer.isValid() || offset < 0 || minBytes <= 0 || timeoutMs <= 0) {
        m_readGateOffset = -1;
        m_readGateMinBytes = 0;
        m_readGateDeadlineMs = 0;
        m_readGateKind = ReadGateKind::None;
        return;
    }

    m_readGateOffset = offset;
    m_readGateMinBytes = minBytes;
    m_readGateDeadlineMs = m_streamTimer.elapsed() + timeoutMs;
    m_lastReadGateLogMs = 0;
    m_readGateKind = kind;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] read_gate_set"
             << "offset=" << offset
             << "kind=" << readGateKindName(kind)
             << "required=" << minBytes
             << "timeoutMs=" << timeoutMs
             << "flacLike=" << m_flacLike
             << "ttfbMs=" << m_lastFetchTtfbMs
             << "fetchElapsedMs=" << m_lastFetchElapsedMs
             << "avgKiBps=" << m_lastFetchAvgKiBps
             << "curl_open_count=" << m_curlOpenCount
             << "hard_seek_count=" << m_hardSeekCount;
}

qint64 AudioPrefetchStream::readGateTargetBytesLocked(ReadGateKind kind) const {
    if (!m_flacLike) {
        return kind == ReadGateKind::Startup ? kStartupReadGateBytes : kFastSeekReadGateBytes;
    }

    if (kind == ReadGateKind::Startup) {
        if (m_lastFetchAvgKiBps > 0 && m_lastFetchAvgKiBps < kSlowThroughputKiBps) {
            return 64LL * 1024;
        }
        if (m_lastFetchAvgKiBps >= 1200) {
            return 64LL * 1024;
        }
        return kFlacColdStartupReadGateBytes;
    }
    if (kind == ReadGateKind::Seek) {
        if (m_hasPendingSeekHint && m_pendingSeekHintTimeExact) {
            return 32LL * 1024;
        }
        if (m_lastFetchAvgKiBps >= 1200) {
            return 40LL * 1024;
        }
        return kFlacSeekReadGateBytes;
    }
    if (kind == ReadGateKind::Steady) {
        return 0;
    }
    return 0;
}

int AudioPrefetchStream::readGateTimeoutMsLocked(ReadGateKind kind) const {
    if (!m_flacLike) {
        return kind == ReadGateKind::Startup ? kStartupReadGateTimeoutMs
                                             : kFastSeekReadGateTimeoutMs;
    }

    if (kind == ReadGateKind::Startup) {
        if (m_lastFetchAvgKiBps > 0 && m_lastFetchAvgKiBps < kSlowThroughputKiBps) {
            return 120;
        }
        if (m_lastFetchAvgKiBps >= 1200) {
            return 140;
        }
        return kFlacStartupReadGateTimeoutMs;
    }
    if (kind == ReadGateKind::Seek) {
        if (m_hasPendingSeekHint && m_pendingSeekHintTimeExact) {
            return 55;
        }
        if (m_lastFetchAvgKiBps >= 1200) {
            return 65;
        }
        return kFlacSeekReadGateTimeoutMs;
    }
    if (kind == ReadGateKind::Steady) {
        return kFlacSteadyReadGateTimeoutMs;
    }
    return 0;
}

bool AudioPrefetchStream::shouldHoldReadGateLocked(qint64 availableAhead, qint64 nowMs) const {
    if (m_readGateOffset < 0 || m_pos != m_readGateOffset) {
        return false;
    }
    if (availableAhead >= m_readGateMinBytes) {
        return false;
    }
    return nowMs > 0 && nowMs < m_readGateDeadlineMs;
}

bool AudioPrefetchStream::shouldHoldFlacSteadyReadLocked(qint64 availableAhead, qint64 nowMs,
                                                         qint64 waitStartMs) const {
    if (!m_flacLike || availableAhead <= 0 || nowMs <= 0) {
        return false;
    }
    const qint64 targetBytes = readGateTargetBytesLocked(ReadGateKind::Steady);
    const qint64 timeoutMs = readGateTimeoutMsLocked(ReadGateKind::Steady);
    if (targetBytes <= 0 || timeoutMs <= 0) {
        return false;
    }
    return availableAhead < targetBytes && (nowMs - waitStartMs) < timeoutMs;
}

void AudioPrefetchStream::clearReadGateLocked(const char* reason, qint64 availableAhead) {
    if (m_readGateOffset < 0) {
        return;
    }

    qDebug() << "[STREAM_EVT][AudioPrefetchStream] read_gate_release"
             << "reason=" << reason
             << "pos=" << m_pos
             << "availableAhead=" << availableAhead
             << "required=" << m_readGateMinBytes
             << "kind=" << readGateKindName(m_readGateKind)
             << "ttfbMs=" << m_lastFetchTtfbMs
             << "fetchElapsedMs=" << m_lastFetchElapsedMs
             << "avgKiBps=" << m_lastFetchAvgKiBps;
    m_readGateOffset = -1;
    m_readGateMinBytes = 0;
    m_readGateDeadlineMs = 0;
    m_readGateKind = ReadGateKind::None;
}

const char* AudioPrefetchStream::readGateKindName(ReadGateKind kind) const {
    switch (kind) {
    case ReadGateKind::Startup:
        return "startup";
    case ReadGateKind::Seek:
        return "seek";
    case ReadGateKind::Steady:
        return "steady";
    case ReadGateKind::None:
        break;
    }
    return "none";
}

bool AudioPrefetchStream::seekForwardByReadingLocked(qint64 offset) {
    const int trackIndex = findTrackAppendableLocked(m_pos);
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        return false;
    }

    const Track& current = m_tracks[trackIndex];
    const qint64 end = trackEnd(current);
    if (offset < m_pos || offset > end + m_seekThresholdBytes) {
        return false;
    }

    m_currentTrack = trackIndex;
    m_pos = offset;
    m_eof = false;
    if (cachedAheadLocked() < lowWaterBytesLocked()) {
        m_spaceCv.notify_one();
    }
    m_dataCv.notify_all();
    return true;
}

bool AudioPrefetchStream::coalesceInflightSeekLocked(qint64 offset) {
    if (m_replyStartOffset < 0 || m_eof || m_error || shouldStopWorkerLocked() ||
        m_abortReplyRequested || m_waitingForAbortClear || hasTransientAbortLocked()) {
        return false;
    }
    const bool activeReply = m_replyGeneration == m_generation && !m_reconnectPending &&
                             !m_abortReplyRequested;
    if (!activeReply) {
        return false;
    }
    if (offset < m_replyStartOffset) {
        return false;
    }

    const qint64 inflightEnd = bufferedOrInflightEndLocked();
    const qint64 coalesceBytes = inflightSeekCoalesceBytesLocked();
    const bool cacheMissBehind =
        offset < m_prefetchOffset && findTrackContainingLocked(offset) < 0;
    bool flacBackwardProbe = false;
    if (cacheMissBehind) {
        const qint64 behindBytes = m_prefetchOffset - offset;
        const qint64 fromReplyStart = offset - m_replyStartOffset;
        flacBackwardProbe = m_flacLike && behindBytes <= kFlacInflightBackwardProbeBytes &&
                            fromReplyStart <= kFlacInflightOldIntervalBytes;
        if (!flacBackwardProbe) {
            return false;
        }
        m_reconnectPending = true;
        m_abortReplyRequested = true;
        if (m_curlBackend) {
            m_curlBackend->abort();
        }
        ++m_backendAbortCount;
        m_retryCount = 0;
        m_replyStartOffset = offset;
        m_prefetchOffset = offset;
        m_lastAppendOffset = offset;
        m_lastAppendLogOffset = offset;
        m_upstreamPaused = false;
        m_reconnectNotBeforeMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
    }
    if (!cacheMissBehind && offset > inflightEnd + coalesceBytes) {
        return false;
    }

    m_pos = offset;
    m_eof = false;
    clearReadGateLocked("coalesce_inflight_seek", cachedAheadLocked());
    setReadGateLocked(offset, ReadGateKind::Seek);
    if (cachedAheadLocked() < lowWaterBytesLocked()) {
        m_spaceCv.notify_one();
    }
    m_dataCv.notify_all();
    ++m_coalesceCount;

    qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_coalesced_inflight=true"
             << "offset=" << offset
             << "replyStart=" << m_replyStartOffset
             << "inflightEnd=" << inflightEnd
             << "flacBackwardProbe=" << flacBackwardProbe
             << "hard_seek=false"
             << "generation=" << m_generation
             << "curl_open_count=" << m_curlOpenCount
             << "hard_seek_count=" << m_hardSeekCount
             << "coalesce_count=" << m_coalesceCount;
    return true;
}

qint64 AudioPrefetchStream::resolveSeekOffsetLocked(qint64 requestedOffset) {
    if (!m_hasPendingSeekHint || m_pendingSeekHintByte < 0) {
        return requestedOffset;
    }

    if (!m_pendingSeekHintTimeExact && !m_pendingSeekHintAllowRangeAnchor) {
        m_hasPendingSeekHint = false;
        qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_hint_skipped_non_exact"
                 << "requested=" << requestedOffset
                 << "hinted=" << m_pendingSeekHintByte
                 << "targetMs=" << m_pendingSeekHintTargetMs
                 << "allowRangeAnchor=" << m_pendingSeekHintAllowRangeAnchor;
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

    const qint64 tolerance = m_pendingSeekHintTimeExact
                                 ? (m_flacLike ? kFlacExactSeekHintToleranceBytes
                                               : kExactSeekHintToleranceBytes)
                                 : inflightSeekCoalesceBytesLocked();
    if (qAbs(requestedOffset - hintedOffset) > tolerance) {
        return requestedOffset;
    }

    m_hasPendingSeekHint = false;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream] seek_hint_byte_applied"
             << "requested=" << requestedOffset << "applied=" << hintedOffset
             << "targetMs=" << m_pendingSeekHintTargetMs
             << "timeExact=" << m_pendingSeekHintTimeExact
             << "allowRangeAnchor=" << m_pendingSeekHintAllowRangeAnchor;
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

bool AudioPrefetchStream::shouldStopWorkerLocked() const {
    return m_stopRequested || m_stopSignal.load();
}

bool AudioPrefetchStream::hasTransientAbortLocked() const {
    return !m_stopSignal.load() && m_interruptCallback && m_interruptCallback();
}

void AudioPrefetchStream::markAbortReconnectPendingLocked(const char* stage,
                                                          qint64 interruptedStart,
                                                          int interruptedGeneration) {
    m_reconnectPending = true;
    m_reconnectNotBeforeMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
    m_waitingForAbortClear = true;
    m_error = false;
    m_lastError.clear();

    const qint64 nowMs = m_streamTimer.isValid() ? m_streamTimer.elapsed() : 0;
    if (nowMs - m_lastAbortReconnectLogMs >= 250) {
        m_lastAbortReconnectLogMs = nowMs;
        qDebug() << "[STREAM_EVT][AudioPrefetchStream]"
                 << "curl_reconnect_pending_after_interrupt"
                 << "stage=" << stage
                 << "interruptedStart=" << interruptedStart
                 << "interruptedGeneration=" << interruptedGeneration
                 << "currentGeneration=" << m_generation
                 << "prefetchOffset=" << m_prefetchOffset;
    }
}

void AudioPrefetchStream::clearAbortWaitLocked(const char* stage) {
    if (!m_waitingForAbortClear) {
        return;
    }

    m_waitingForAbortClear = false;
    qDebug() << "[STREAM_EVT][AudioPrefetchStream]"
             << "curl_abort_cleared"
             << "stage=" << stage
             << "generation=" << m_generation
             << "prefetchOffset=" << m_prefetchOffset;
}

bool AudioPrefetchStream::shouldAbort() const {
    return m_stopSignal.load() || (m_interruptCallback && m_interruptCallback());
}

void AudioPrefetchStream::waitForWorkerStopped() {
    if (m_worker.joinable()) {
        m_worker.join();
    }
}
