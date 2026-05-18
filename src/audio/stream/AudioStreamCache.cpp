#include "AudioStreamCache.h"

#include <QtDebug>
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
constexpr int kTrackCount = 3;
constexpr int kFetchTimeoutMs = 4500;
constexpr qint64 kMinFetchBlockBytes = 128LL * 1024;
constexpr qint64 kSeekFetchBlockBytes = 256LL * 1024;
constexpr qint64 kSequentialFetchBlockBytes = 1024LL * 1024;
constexpr qint64 kSequentialMinReturnBytes = 512LL * 1024;
constexpr qint64 kSeekMinReturnBytes = 128LL * 1024;
} // namespace

AudioStreamCache::AudioStreamCache(AudioNetworkAccess* access) : m_access(access) {
    m_tracks.resize(kTrackCount);
    for (Track& track : m_tracks) {
        track.start = 0;
    }
}

int AudioStreamCache::read(uint8_t* dst, int size) {
    if (!dst || size <= 0 || !m_access) {
        return ReadError;
    }

    m_lastInterrupted = false;
    int totalCopied = 0;
    while (totalCopied < size) {
        int index = findTrackContaining(m_pos);
        if (index >= 0) {
            m_currentTrack = index;
        } else if (!seek(m_pos)) {
            return totalCopied > 0 ? totalCopied
                                   : (m_lastInterrupted ? ReadInterrupted : ReadError);
        }

        Track& track = m_tracks[m_currentTrack];
        track.lastUse = ++m_tick;
        if (m_pos >= trackEnd(track)) {
            if (m_eof) {
                break;
            }
            if (!fetchIntoTrack(track, m_pos + 1)) {
                return totalCopied > 0 ? totalCopied
                                       : (m_lastInterrupted ? ReadInterrupted : ReadError);
            }
            if (track.data.isEmpty() && m_eof) {
                break;
            }
        }

        const qint64 offsetInTrack = m_pos - track.start;
        if (offsetInTrack < 0 || offsetInTrack >= track.data.size()) {
            continue;
        }

        const int available = track.data.size() - static_cast<int>(offsetInTrack);
        const int toCopy = qMin(size - totalCopied, available);
        std::memcpy(dst + totalCopied, track.data.constData() + offsetInTrack,
                    static_cast<size_t>(toCopy));
        totalCopied += toCopy;
        m_pos += toCopy;

        if (totalCopied > 0 && m_skipWarmReadCount <= 0) {
            warmSequentialWindow(track);
        } else if (totalCopied > 0 && m_skipWarmReadCount > 0) {
            --m_skipWarmReadCount;
        }
    }

    return totalCopied;
}

bool AudioStreamCache::seek(qint64 offset) {
    if (offset < 0 || !m_access) {
        return false;
    }

    m_lastInterrupted = false;
    offset = resolveSeekOffset(offset);
    const int cachedIndex = findTrackContaining(offset);
    if (cachedIndex >= 0) {
        m_currentTrack = cachedIndex;
        m_tracks[m_currentTrack].lastUse = ++m_tick;
        m_pos = offset;
        qDebug() << "[AudioStreamCache] stream_cache_hit=true"
                 << "offset=" << offset << "track=" << cachedIndex;
        return true;
    }

    Track& current = m_tracks[m_currentTrack];
    const qint64 currentEnd = trackEnd(current);
    const qint64 skipThreshold = m_access->canSeek() ? m_fetchBlockBytes : std::numeric_limits<int>::max();
    if (offset >= currentEnd && offset <= currentEnd + skipThreshold) {
        if (fetchIntoTrack(current, offset + 1)) {
            m_pos = offset;
            qDebug() << "[AudioStreamCache] stream_cache_hit=false hard_seek=false"
                     << "offset=" << offset << "from=" << currentEnd;
            return true;
        }
    }

    if (!m_access->canSeek() && offset < currentEnd) {
        return false;
    }

    const int replaceIndex = chooseReusableTrack();
    Track& track = m_tracks[replaceIndex];
    qint64 hardSeekStart = offset;
    const qint64 totalLength = size();
    if (totalLength > 0 && offset > totalLength - 64 * 1024) {
        hardSeekStart = qMax<qint64>(0, totalLength - 64 * 1024);
    }
    track.start = hardSeekStart;
    track.data.clear();
    track.lastUse = ++m_tick;
    m_currentTrack = replaceIndex;
    m_pos = offset;
    m_eof = false;
    m_skipWarmReadCount = 4;
    qDebug() << "[AudioStreamCache] stream_cache_hit=false hard_seek=true"
             << "offset=" << offset << "fetchStart=" << hardSeekStart
             << "track=" << replaceIndex;
    return true;
}

void AudioStreamCache::prepareSeekHint(qint64 targetMs, qint64 targetByte, qint64 contentLength,
                                       bool exact) {
    if (targetMs < 0 || targetByte < 0) {
        m_hasPendingSeekHint = false;
        return;
    }

    m_hasPendingSeekHint = true;
    m_pendingSeekHintExact = exact;
    m_pendingSeekHintTargetMs = targetMs;
    m_pendingSeekHintByte = targetByte;
    m_pendingSeekHintContentLength = contentLength;
    m_skipWarmReadCount = 6;
    qDebug() << "[AudioStreamCache] seek_hint_prepared"
             << "targetMs=" << targetMs << "targetByte=" << targetByte
             << "contentLength=" << contentLength << "exact=" << exact;
}

qint64 AudioStreamCache::size() const {
    return m_access ? m_access->contentLength() : -1;
}

bool AudioStreamCache::canSeek() const {
    return m_access && m_access->canSeek();
}

qint64 AudioStreamCache::trackEnd(const Track& track) const {
    return track.start + track.data.size();
}

int AudioStreamCache::findTrackContaining(qint64 offset) const {
    for (int i = 0; i < m_tracks.size(); ++i) {
        const Track& track = m_tracks[i];
        if (!track.data.isEmpty() && track.start <= offset && offset < trackEnd(track)) {
            return i;
        }
    }
    return -1;
}

int AudioStreamCache::chooseReusableTrack() const {
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

bool AudioStreamCache::fetchIntoTrack(Track& track, qint64 minEndExclusive) {
    if (!m_access) {
        return false;
    }

    while (trackEnd(track) < minEndExclusive) {
        const qint64 fetchOffset = trackEnd(track);
        const qint64 requestedBytes =
            fetchBlockBytesForOffset(track, fetchOffset, minEndExclusive);
        const qint64 minReturnBytes = minReturnBytesForFetch(track, requestedBytes);
        AudioNetworkAccess::RangeResult result =
            m_access->fetchRange(fetchOffset, requestedBytes, kFetchTimeoutMs, minReturnBytes);
        m_lastInterrupted = result.interrupted;
        if (result.eof) {
            m_eof = true;
            return true;
        }
        if (!result.ok) {
            qWarning() << "[AudioStreamCache] fetch failed"
                       << "offset=" << fetchOffset << "error=" << result.error;
            return false;
        }
        if (result.data.isEmpty()) {
            m_eof = true;
            return true;
        }
        if (result.responseStart >= 0 && result.responseStart != fetchOffset) {
            qWarning() << "[AudioStreamCache] response start mismatch"
                       << "expected=" << fetchOffset << "actual=" << result.responseStart;
            return false;
        }

        track.data.append(result.data);
        if (result.partial) {
            qDebug() << "[AudioStreamCache] partial_range_accepted"
                     << "offset=" << fetchOffset << "requested=" << requestedBytes
                     << "received=" << result.data.size() << "timedOut=" << result.timedOut;
        }
        trimTrack(track);
    }
    return true;
}

qint64 AudioStreamCache::fetchBlockBytesForOffset(const Track& track, qint64 fetchOffset,
                                                  qint64 minEndExclusive) const {
    const qint64 requiredBytes = qMax<qint64>(1, minEndExclusive - fetchOffset);
    qint64 blockBytes = kSequentialFetchBlockBytes;
    const bool trackEmpty = track.data.isEmpty();
    const bool nearReadCursor = qAbs(fetchOffset - m_pos) <= 512LL * 1024;

    if (trackEmpty || m_skipWarmReadCount > 0) {
        blockBytes = kSeekFetchBlockBytes;
    } else if (nearReadCursor) {
        blockBytes = qMin<qint64>(m_fetchBlockBytes, kSequentialFetchBlockBytes);
    }

    return qBound(kMinFetchBlockBytes, qMax(requiredBytes, blockBytes), m_fetchBlockBytes);
}

qint64 AudioStreamCache::minReturnBytesForFetch(const Track& track,
                                                qint64 requestedBytes) const {
    if (track.data.isEmpty() || m_skipWarmReadCount > 0) {
        return qMin(requestedBytes, kSeekMinReturnBytes);
    }
    return qMin(requestedBytes, kSequentialMinReturnBytes);
}

void AudioStreamCache::warmSequentialWindow(Track& track) {
    if (!m_access || m_eof || track.data.isEmpty()) {
        return;
    }
    if (m_skipWarmReadCount > 0) {
        return;
    }

    const qint64 cachedAhead = trackEnd(track) - m_pos;
    const qint64 warmBlockBytes = qMin<qint64>(m_fetchBlockBytes, kSequentialFetchBlockBytes);
    if (cachedAhead >= (warmBlockBytes / 2)) {
        return;
    }

    const qint64 targetEnd = qMin(track.start + m_trackMaxBytes,
                                  trackEnd(track) + qMin(warmBlockBytes, m_readAheadBytes));
    if (targetEnd <= trackEnd(track)) {
        return;
    }

    fetchIntoTrack(track, targetEnd);
}

void AudioStreamCache::trimTrack(Track& track) {
    if (track.data.size() <= m_trackMaxBytes) {
        return;
    }

    const qint64 removeBytes = track.data.size() - m_trackMaxBytes;
    track.data.remove(0, static_cast<int>(removeBytes));
    track.start += removeBytes;
}

qint64 AudioStreamCache::resolveSeekOffset(qint64 requestedOffset) {
    if (!m_hasPendingSeekHint || m_pendingSeekHintByte < 0) {
        return requestedOffset;
    }

    const qint64 hintedOffset = m_pendingSeekHintByte;
    const qint64 total = m_pendingSeekHintContentLength > 0 ? m_pendingSeekHintContentLength
                                                            : size();
    if (total > 0 && hintedOffset >= total) {
        m_hasPendingSeekHint = false;
        return requestedOffset;
    }

    const qint64 tolerance = m_pendingSeekHintExact ? 16LL * 1024 * 1024 : 2LL * 1024 * 1024;
    const bool closeEnough = qAbs(requestedOffset - hintedOffset) <= tolerance;
    if (!closeEnough) {
        return requestedOffset;
    }

    m_hasPendingSeekHint = false;
    qDebug() << "[AudioStreamCache] seek_hint_byte_applied"
             << "requested=" << requestedOffset << "applied=" << hintedOffset
             << "targetMs=" << m_pendingSeekHintTargetMs << "exact=" << m_pendingSeekHintExact
             << "contentLength=" << total;
    return hintedOffset;
}
