#include "AudioCacheManager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QVector>
#include <QtConcurrent>
#include <QtDebug>
#include <algorithm>
#include <memory>

namespace {

bool isRemoteHttpUrl(const QUrl& url) {
    if (!url.isValid()) {
        return false;
    }
    const QString scheme = url.scheme().toLower();
    return scheme == "http" || scheme == "https";
}

bool hasSupportedAudioExtension(const QString& pathLower) {
    return pathLower.endsWith(".mp3") || pathLower.endsWith(".flac") ||
           pathLower.endsWith(".wav") || pathLower.endsWith(".ogg") || pathLower.endsWith(".m4a") ||
           pathLower.endsWith(".aac");
}

bool hasSeekIndexSupportedAudioExtension(const QString& pathLower) {
    return pathLower.endsWith(".mp3") || pathLower.endsWith(".flac") ||
           pathLower.endsWith(".m4a") || pathLower.endsWith(".aac");
}

bool shouldFullCacheByAudioType(const QUrl& url) {
    const QString pathLower = url.path().toLower();
    return pathLower.endsWith(".mp3") || pathLower.endsWith(".ogg") || pathLower.endsWith(".wav");
}

bool isFlacUrl(const QUrl& url) {
    return url.path().toLower().endsWith(".flac");
}

bool isFlacMusicPath(const QString& path) {
    return path.toLower().endsWith(QStringLiteral(".flac"));
}

bool isLocalProxyUrl(const QUrl& url) {
    if (!url.isValid()) {
        return false;
    }

    const QString scheme = url.scheme().toLower();
    if (scheme != "http") {
        return false;
    }

    const QString host = url.host().toLower();
    return host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost");
}

QUrl resolveSeekIndexSourceUrl(const QUrl& playbackUrl) {
    if (isLocalProxyUrl(playbackUrl)) {
        const QUrlQuery query(playbackUrl);
        const QUrl sourceUrl(query.queryItemValue(QStringLiteral("src")));
        if (sourceUrl.isValid() && !sourceUrl.isEmpty()) {
            return sourceUrl;
        }
    }

    return playbackUrl;
}

bool containsTraversalSegment(const QString& normalizedPath) {
    const QStringList segments = normalizedPath.split('/', Qt::SkipEmptyParts);
    for (const QString& segment : segments) {
        if (segment == QStringLiteral("..")) {
            return true;
        }
    }
    return false;
}

QString headerValue(const QMap<QString, QString>& headers, const QString& name) {
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        if (it.key().compare(name, Qt::CaseInsensitive) == 0) {
            return it.value();
        }
    }
    return QString();
}

QByteArray headerValueBytes(const QMap<QString, QString>& headers, const QString& name) {
    return headerValue(headers, name).toLatin1();
}

qint64 contentLengthFromHeaders(const QMap<QString, QString>& headers) {
    bool ok = false;
    const qint64 value =
        headerValue(headers, QStringLiteral("Content-Length")).toLongLong(&ok);
    return ok ? value : -1;
}

constexpr qint64 kLocalSeekSinglePointWindowMs = 5000;
constexpr qint64 kLocalSeekMaxBracketWindowMs = 45000;

bool isLikelyAudioResponse(int statusCode, const QMap<QString, QString>& headers) {
    if (statusCode != 0 && statusCode != 200 && statusCode != 206) {
        return false;
    }

    QByteArray contentType =
        headerValueBytes(headers, QStringLiteral("Content-Type")).toLower().trimmed();
    const int semicolonPos = contentType.indexOf(';');
    if (semicolonPos >= 0) {
        contentType = contentType.left(semicolonPos).trimmed();
    }

    return contentType.isEmpty() || contentType.startsWith("audio/") ||
           contentType == "application/octet-stream";
}

QByteArray audioContentTypeForUrl(const QUrl& url) {
    const QString pathLower = url.path().toLower();
    if (pathLower.endsWith(".mp3")) {
        return "audio/mpeg";
    }
    if (pathLower.endsWith(".flac")) {
        return "audio/flac";
    }
    if (pathLower.endsWith(".ogg")) {
        return "audio/ogg";
    }
    if (pathLower.endsWith(".wav")) {
        return "audio/wav";
    }
    if (pathLower.endsWith(".m4a")) {
        return "audio/mp4";
    }
    if (pathLower.endsWith(".aac")) {
        return "audio/aac";
    }
    return "application/octet-stream";
}

qint64 parseContentRangeLength(const QString& contentRange) {
    // format: bytes start-end/total
    static const QRegularExpression re(QStringLiteral(R"(bytes\s+\d+-\d+/(\d+))"),
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(contentRange.trimmed());
    if (!m.hasMatch()) {
        return -1;
    }

    bool ok = false;
    const qint64 total = m.captured(1).toLongLong(&ok);
    return ok ? total : -1;
}

qint64 parseContentRangeStart(const QByteArray& contentRange) {
    static const QRegularExpression re(QStringLiteral(R"(bytes\s+(\d+)-\d+/)"),
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(QString::fromUtf8(contentRange).trimmed());
    if (!m.hasMatch()) {
        return -1;
    }

    bool ok = false;
    const qint64 start = m.captured(1).toLongLong(&ok);
    return ok ? start : -1;
}

int findHttpHeaderEnd(const QByteArray& data, int* delimiterLen) {
    const int crlfEnd = data.indexOf("\r\n\r\n");
    if (crlfEnd >= 0) {
        if (delimiterLen) {
            *delimiterLen = 4;
        }
        return crlfEnd;
    }

    const int lfEnd = data.indexOf("\n\n");
    if (lfEnd >= 0) {
        if (delimiterLen) {
            *delimiterLen = 2;
        }
        return lfEnd;
    }

    if (delimiterLen) {
        *delimiterLen = 0;
    }
    return -1;
}

} // namespace

AudioCacheManager& AudioCacheManager::instance() {
    static AudioCacheManager manager;
    return manager;
}

AudioCacheManager::AudioCacheManager(QObject* parent) : QObject(parent) {
    const QString defaultBase =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString defaultPath = QDir(defaultBase.isEmpty() ? QDir::currentPath() : defaultBase)
                                    .absoluteFilePath(QStringLiteral("audio_cache"));
    m_cacheDirectory = ensureCacheDirectory(defaultPath);
    m_chunkRootDirectory = QDir(m_cacheDirectory).filePath(QStringLiteral("chunks"));
    QDir().mkpath(m_chunkRootDirectory);
}

void AudioCacheManager::setCacheDirectory(const QString& directoryPath) {
    const QString normalized = ensureCacheDirectory(directoryPath);
    if (normalized.isEmpty()) {
        return;
    }

    if (m_cacheDirectory != normalized) {
        m_cacheDirectory = normalized;
        m_chunkRootDirectory = QDir(m_cacheDirectory).filePath(QStringLiteral("chunks"));
        QDir().mkpath(m_chunkRootDirectory);
        qDebug() << "[AudioCache] Using cache directory:" << m_cacheDirectory;
    }
}

QString AudioCacheManager::cacheDirectory() const {
    return m_cacheDirectory;
}

bool AudioCacheManager::clearCache(QString* errorMessage, qint64* removedBytes,
                                   qint64* removedFiles) {
    qint64 bytes = 0;
    qint64 files = 0;

    // Abort pending downloads to avoid writing new cache files during cleanup.
    const QList<quint64> activeRequests = m_downloadTasks.keys();
    for (quint64 requestId : activeRequests) {
        abortRequest(requestId);
    }
    m_downloadTasks.clear();
    m_seekIndexRequests.clear();
    m_abortFlags.clear();
    m_inflightChunkKeys.clear();
    m_proxyReadBuffers.clear();
    m_memoryRangeWindows.clear();
    m_seekHints.clear();
    {
        QMutexLocker locker(&m_metaMutex);
        m_trackMetaCache.clear();
    }

    QDir cacheRoot(m_cacheDirectory);
    if (!cacheRoot.exists()) {
        QDir().mkpath(m_cacheDirectory);
        m_chunkRootDirectory = QDir(m_cacheDirectory).filePath(QStringLiteral("chunks"));
        QDir().mkpath(m_chunkRootDirectory);
        if (removedBytes) {
            *removedBytes = 0;
        }
        if (removedFiles) {
            *removedFiles = 0;
        }
        return true;
    }

    QDirIterator countIt(m_cacheDirectory, QDir::Files, QDirIterator::Subdirectories);
    while (countIt.hasNext()) {
        countIt.next();
        const QFileInfo info = countIt.fileInfo();
        bytes += qMax<qint64>(0, info.size());
        ++files;
    }

    bool allRemoved = true;
    QStringList failedPaths;
    const QFileInfoList entries = cacheRoot.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        bool ok = false;
        if (entry.isDir()) {
            QDir subDir(entry.absoluteFilePath());
            ok = subDir.removeRecursively();
        } else {
            ok = QFile::remove(entry.absoluteFilePath());
        }

        if (!ok) {
            allRemoved = false;
            failedPaths.append(entry.absoluteFilePath());
        }
    }

    QDir().mkpath(m_cacheDirectory);
    m_chunkRootDirectory = QDir(m_cacheDirectory).filePath(QStringLiteral("chunks"));
    QDir().mkpath(m_chunkRootDirectory);

    if (removedBytes) {
        *removedBytes = bytes;
    }
    if (removedFiles) {
        *removedFiles = files;
    }

    if (!allRemoved && errorMessage) {
        *errorMessage =
            QStringLiteral("以下路径删除失败：%1").arg(failedPaths.join(QStringLiteral("; ")));
    }
    return allRemoved;
}

QUrl AudioCacheManager::resolvePlaybackUrl(const QUrl& originalUrl) const {
    if (!canCacheUrl(originalUrl)) {
        return originalUrl;
    }

    if (hasValidFullCache(originalUrl)) {
        const QString fullPath = legacyFullCachePathForUrl(originalUrl);
        qDebug() << "[AudioCache] Full cache hit:" << fullPath;
        return QUrl::fromLocalFile(fullPath);
    }

    if (isFlacUrl(originalUrl)) {
        qDebug() << "[AudioCache] Proxy bypass for remote FLAC:" << originalUrl.toString();
        return originalUrl;
    }

    const_cast<AudioCacheManager*>(this)->startProxyServerIfNeeded();
    if (m_proxyPort == 0) {
        return originalUrl;
    }

    QUrl proxyUrl;
    proxyUrl.setScheme(QStringLiteral("http"));
    proxyUrl.setHost(QStringLiteral("127.0.0.1"));
    proxyUrl.setPort(static_cast<int>(m_proxyPort));
    proxyUrl.setPath(QStringLiteral("/audio"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("src"), originalUrl.toString(QUrl::FullyEncoded));
    proxyUrl.setQuery(query);
    return proxyUrl;
}

void AudioCacheManager::warmupCache(const QUrl& originalUrl) {
    if (!canCacheUrl(originalUrl)) {
        return;
    }

    if (isFlacUrl(originalUrl)) {
        const QString cacheKey = makeCacheKey(originalUrl);
        ensureTrackMeta(originalUrl, cacheKey);
        TrackMeta meta;
        loadTrackMeta(cacheKey, &meta);
        ensureSeekIndex(originalUrl, cacheKey, meta.durationMs);
        qDebug() << "[AudioCache] Warmup seek-index only for direct FLAC stream:"
                 << originalUrl.toString();
        return;
    }

    const QString cacheKey = makeCacheKey(originalUrl);
    ensureTrackMeta(originalUrl, cacheKey);

    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    meta.lastAccessMs = currentEpochMs();
    updateTrackMeta(cacheKey, meta.contentLength, meta.durationMs);

    if (shouldFullCacheByAudioType(originalUrl)) {
        qDebug() << "[AudioCache] Full cache queued by audio type:"
                 << "bytes=" << meta.contentLength << "url=" << originalUrl.toString();
        queueFullFileDownload(originalUrl, cacheKey, meta.contentLength,
                              CacheRequestPriority::High);
        return;
    }
    qDebug() << "[AudioCache] Full cache skip-type:"
             << "bytes=" << meta.contentLength << "url=" << originalUrl.toString();

    // 远端播放主链路由本地代理负责顺序拉流；后台预热只补一小段，避免抢占 FLAC 实时带宽。
    const int startupChunks = isFlacUrl(originalUrl) ? 1 : qMin(m_startupPrefetchChunks, 2);

    for (int i = 0; i < startupChunks; ++i) {
        const qint64 chunkIndex = i;
        const qint64 start = chunkIndex * m_chunkSizeBytes;
        qint64 end = start + m_chunkSizeBytes - 1;
        if (meta.contentLength > 0) {
            end = qMin(end, meta.contentLength - 1);
        }
        if (end < start) {
            continue;
        }
        queueChunkDownload(originalUrl, cacheKey, chunkIndex, start, end,
                           CacheRequestPriority::Low);
    }
}

void AudioCacheManager::prefetchForSeek(const QUrl& originalUrl, qint64 targetMs,
                                        qint64 durationMs) {
    if (!canCacheUrl(originalUrl)) {
        return;
    }

    const QString cacheKey = makeCacheKey(originalUrl);
    ensureTrackMeta(originalUrl, cacheKey);

    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    if (durationMs > 0 && meta.durationMs <= 0) {
        meta.durationMs = durationMs;
    }
    if (meta.contentLength <= 0 && meta.seekIndexFileSize > 0) {
        meta.contentLength = meta.seekIndexFileSize;
    }
    meta.lastAccessMs = currentEpochMs();
    {
        QMutexLocker locker(&m_metaMutex);
        m_trackMetaCache.insert(cacheKey, meta);
    }
    persistTrackMeta(cacheKey, meta);

    ensureSeekIndex(originalUrl, cacheKey, durationMs > 0 ? durationMs : meta.durationMs);

    loadTrackMeta(cacheKey, &meta);
    if (meta.contentLength <= 0 && meta.seekIndexFileSize > 0) {
        meta.contentLength = meta.seekIndexFileSize;
    }

    SeekIndexEstimate seekEstimate = estimateCenterByteFromSeekIndex(meta, targetMs);
    const bool seekIndexHit = seekEstimate.hit;

    if (meta.contentLength <= 0 || (!seekIndexHit && meta.durationMs <= 0) || targetMs < 0) {
        qDebug() << "[AudioCache] Seek prefetch skipped, metadata incomplete:"
                 << "targetMs=" << targetMs << "durationMs=" << meta.durationMs
                 << "contentLength=" << meta.contentLength << "url=" << originalUrl.toString();
        return;
    }

    qint64 centerByte = seekEstimate.correctedTargetByte;
    if (!seekIndexHit) {
        const double ratio =
            qBound(0.0, static_cast<double>(targetMs) / static_cast<double>(meta.durationMs), 1.0);
        centerByte = static_cast<qint64>(ratio * static_cast<double>(meta.contentLength - 1));
        seekEstimate.hit = false;
        seekEstimate.exact = false;
        seekEstimate.confidenceScore = 20;
        seekEstimate.confidenceReason = QStringLiteral("duration_ratio_fallback");
        seekEstimate.rawTargetByte = centerByte;
        seekEstimate.correctedTargetByte = centerByte;
        seekEstimate.correctionBytes = 0;
        seekEstimate.anchorTimeMs = targetMs;
        seekEstimate.preRollMs = 0;
    }
    if (centerByte < 0) {
        centerByte = 0;
    }
    registerSeekHint(originalUrl, cacheKey, targetMs, meta.contentLength, seekEstimate);
    const bool flac = isFlacUrl(originalUrl);
    if (flac) {
        qDebug() << "[AudioCache] Direct FLAC seek hint prepared:"
                 << "seek_target_ms=" << targetMs << "seek_index_hit=" << seekIndexHit
                 << "targetByte=" << centerByte << "contentLength=" << meta.contentLength
                 << "exact=" << seekEstimate.exact
                 << "confidenceScore=" << seekEstimate.confidenceScore
                 << "confidenceReason=" << seekEstimate.confidenceReason
                 << "anchorTimeMs=" << seekEstimate.anchorTimeMs
                 << "preRollMs=" << seekEstimate.preRollMs
                 << "rawTargetByte=" << seekEstimate.rawTargetByte
                 << "correctedTargetByte=" << seekEstimate.correctedTargetByte
                 << "url=" << originalUrl.toString();
        return;
    }
    const qint64 centerChunk = centerByte / m_chunkSizeBytes;
    const qint64 seekWindow = seekIndexHit ? 1 : 0;

    // 以“中心块优先，再向前后扩展”的顺序预取，降低首包等待。
    QVector<qint64> offsets;
    offsets.reserve(static_cast<int>(seekWindow * 2 + 1));
    offsets.push_back(0);
    for (qint64 i = 1; i <= seekWindow; ++i) {
        offsets.push_back(i);
        offsets.push_back(-i);
    }

    for (qint64 offset : offsets) {
        const qint64 chunkIndex = centerChunk + offset;
        if (chunkIndex < 0) {
            continue;
        }
        const qint64 start = chunkIndex * m_chunkSizeBytes;
        qint64 end = start + m_chunkSizeBytes - 1;
        end = qMin(end, meta.contentLength - 1);
        if (end < start) {
            continue;
        }
        CacheRequestPriority priority = CacheRequestPriority::Low;
        if (offset == 0) {
            priority = CacheRequestPriority::High;
        } else if (qAbs(offset) <= 1) {
            priority = CacheRequestPriority::Normal;
        }
        queueChunkDownload(originalUrl, cacheKey, chunkIndex, start, end, priority);
    }

    qDebug() << "[AudioCache] Seek prefetch queued:"
             << "seek_target_ms=" << targetMs << "seek_index_hit=" << seekIndexHit
             << "seek_prefetch_center_byte=" << centerByte
             << "seek_hint_exact=" << seekEstimate.exact
             << "seek_confidence_score=" << seekEstimate.confidenceScore
             << "seek_confidence_reason=" << seekEstimate.confidenceReason
             << "seek_raw_target_byte=" << seekEstimate.rawTargetByte
             << "seek_corrected_target_byte=" << seekEstimate.correctedTargetByte
             << "seek_correction_bytes=" << seekEstimate.correctionBytes << "targetMs=" << targetMs
             << "durationMs=" << meta.durationMs << "contentLength=" << meta.contentLength
             << "centerChunk=" << centerChunk << "window=" << seekWindow << "isFlac=" << flac;
}

void AudioCacheManager::noteTrackDuration(const QUrl& originalUrl, qint64 durationMs) {
    if (!canCacheUrl(originalUrl) || durationMs <= 0) {
        return;
    }

    const QString cacheKey = makeCacheKey(originalUrl);
    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    if (meta.durationMs == durationMs) {
        return;
    }

    meta.durationMs = durationMs;
    meta.lastAccessMs = currentEpochMs();
    updateTrackMeta(cacheKey, meta.contentLength, meta.durationMs);
}

void AudioCacheManager::noteTrackContentLength(const QUrl& originalUrl, qint64 contentLength) {
    if (!canCacheUrl(originalUrl) || contentLength <= 0) {
        return;
    }

    updateTrackMeta(makeCacheKey(originalUrl), contentLength, -1);
}

qint64 AudioCacheManager::knownTrackDuration(const QUrl& originalUrl) const {
    if (!canCacheUrl(originalUrl)) {
        return -1;
    }

    TrackMeta meta;
    if (!loadTrackMeta(makeCacheKey(originalUrl), &meta)) {
        return -1;
    }
    return meta.durationMs;
}

void AudioCacheManager::noteSeekIndexObservation(const QUrl& originalUrl, qint64 targetMs,
                                                 qint64 hintedByte, qint64 firstDecodedMs,
                                                 bool accepted, int generation) {
    if (!canCacheUrl(originalUrl) || targetMs < 0 || hintedByte < 0 || firstDecodedMs < 0) {
        return;
    }

    const QString cacheKey = makeCacheKey(originalUrl);
    TrackMeta meta;
    if (!loadTrackMeta(cacheKey, &meta)) {
        return;
    }
    if (meta.contentLength <= 0 && meta.seekIndexFileSize > 0) {
        meta.contentLength = meta.seekIndexFileSize;
    }

    const bool flacObservation = isFlacUrl(originalUrl) || isFlacMusicPath(meta.seekIndexMusicPath);
    if (flacObservation) {
        const SeekHint hint = m_seekHints.value(cacheKey);
        const bool generationMatches = generation <= 0 || hint.generation <= 0 ||
                                       hint.generation == generation;
        const bool currentHintMatches =
            hint.targetByte == hintedByte && qAbs(hint.targetMs - targetMs) <= 1500 &&
            hint.expiresAtMs >= currentEpochMs() && generationMatches;
        const bool durationRatioFallback =
            hint.confidenceReason == QStringLiteral("duration_ratio_fallback");
        if (!currentHintMatches || !hint.seekIndexHit || durationRatioFallback) {
            qDebug() << "[AudioCache] Skip FLAC seek observation:"
                     << "targetMs=" << targetMs << "hintedByte=" << hintedByte
                     << "firstDecodedMs=" << firstDecodedMs
                     << "currentHintMatches=" << currentHintMatches
                     << "generationMatches=" << generationMatches
                     << "hintGeneration=" << hint.generation
                     << "observationGeneration=" << generation
                     << "seekIndexHit=" << hint.seekIndexHit
                     << "confidenceScore=" << hint.confidenceScore
                     << "confidenceReason=" << hint.confidenceReason;
            return;
        }
    }

    SeekIndexEstimate targetEstimate;
    const qint64 targetRawByte = estimateRawByteFromSeekIndex(meta, targetMs, &targetEstimate);
    const qint64 decodedRawByte = estimateRawByteFromSeekIndex(meta, firstDecodedMs);
    const qint64 driftMs = firstDecodedMs - targetMs;
    qint64 inferredCorrectionBytes = 0;
    if (targetRawByte >= 0 && decodedRawByte >= 0) {
        inferredCorrectionBytes = targetRawByte - decodedRawByte;
    }
    const bool goodSample = accepted && qAbs(driftMs) <= 1500;
    if (goodSample) {
        ++meta.seekIndexGoodSamples;
    } else {
        ++meta.seekIndexBadSamples;
    }

    if (!goodSample) {
        if (meta.seekIndexCorrectionUpdatedAtMs > 0) {
            meta.seekIndexGlobalBiasBytes =
                (meta.seekIndexGlobalBiasBytes * 3 + inferredCorrectionBytes) / 4;
            meta.seekIndexGlobalBiasMs = (meta.seekIndexGlobalBiasMs * 3 + driftMs) / 4;
        } else {
            meta.seekIndexGlobalBiasBytes = inferredCorrectionBytes;
            meta.seekIndexGlobalBiasMs = driftMs;
        }
        meta.seekIndexCorrectionUpdatedAtMs = currentEpochMs();
    } else if (qAbs(meta.seekIndexGlobalBiasBytes) <= 64 * 1024 &&
               qAbs(meta.seekIndexGlobalBiasMs) <= 1500) {
        meta.seekIndexGlobalBiasBytes = 0;
        meta.seekIndexGlobalBiasMs = 0;
        meta.seekIndexCorrectionUpdatedAtMs = currentEpochMs();
    }

    if (accepted) {
        qint64 learnedByte = hintedByte;
        if (meta.contentLength > 0) {
            learnedByte = qBound<qint64>(0, learnedByte, meta.contentLength - 1);
        }
        const qint64 learnedTime = qMax<qint64>(0, firstDecodedMs);
        bool merged = false;
        for (TrackMeta::SeekIndexPoint& point : meta.localSeekPoints) {
            if (qAbs(point.timeMs - learnedTime) <= 2500) {
                point.timeMs = (point.timeMs + learnedTime) / 2;
                point.byteOffset = (point.byteOffset * 3 + learnedByte) / 4;
                merged = true;
                break;
            }
        }
        if (!merged) {
            meta.localSeekPoints.push_back(TrackMeta::SeekIndexPoint{learnedTime, learnedByte});
        }
        std::sort(meta.localSeekPoints.begin(), meta.localSeekPoints.end(),
                  [](const TrackMeta::SeekIndexPoint& lhs,
                     const TrackMeta::SeekIndexPoint& rhs) {
                      if (lhs.timeMs == rhs.timeMs) {
                          return lhs.byteOffset < rhs.byteOffset;
                      }
                      return lhs.timeMs < rhs.timeMs;
                  });
        while (meta.localSeekPoints.size() > 64) {
            meta.localSeekPoints.removeFirst();
        }
    }

    meta.lastAccessMs = currentEpochMs();
    {
        QMutexLocker locker(&m_metaMutex);
        m_trackMetaCache.insert(cacheKey, meta);
    }
    persistTrackMeta(cacheKey, meta);

    qDebug() << "[AudioCache] Seek index observation:"
             << "targetMs=" << targetMs << "hintedByte=" << hintedByte
             << "firstDecodedMs=" << firstDecodedMs << "driftMs=" << driftMs
             << "accepted=" << accepted << "generation=" << generation
             << "targetRawByte=" << targetRawByte << "decodedRawByte=" << decodedRawByte
             << "inferredCorrectionBytes=" << inferredCorrectionBytes
             << "localPoints=" << meta.localSeekPoints.size()
             << "globalBiasBytes=" << meta.seekIndexGlobalBiasBytes
             << "globalBiasMs=" << meta.seekIndexGlobalBiasMs
             << "goodSamples=" << meta.seekIndexGoodSamples
             << "badSamples=" << meta.seekIndexBadSamples;
}

bool AudioCacheManager::hasResolvedSeekIndexForPlaybackUrl(const QUrl& playbackUrl) const {
    const QUrl originalUrl = resolveSeekIndexSourceUrl(playbackUrl);
    if (!canUseSeekIndexForUrl(originalUrl)) {
        return false;
    }

    TrackMeta meta;
    if (!loadTrackMeta(makeCacheKey(originalUrl), &meta)) {
        return false;
    }

    return meta.seekIndexResolved && meta.seekIndexSupported && !meta.seekIndexPoints.isEmpty();
}

bool AudioCacheManager::hasRecentProxySeekTakeoverForPlaybackUrl(const QUrl& playbackUrl,
                                                                 qint64 targetMs) const {
    const QUrl originalUrl = resolveSeekIndexSourceUrl(playbackUrl);
    if (!canCacheUrl(originalUrl) || targetMs < 0) {
        return false;
    }

    const SeekHint hint = m_seekHints.value(makeCacheKey(originalUrl));
    if (hint.targetMs < 0 || hint.takeoverAtMs <= 0) {
        return false;
    }
    if (qAbs(hint.targetMs - targetMs) > 1500) {
        return false;
    }
    return currentEpochMs() - hint.takeoverAtMs <= 12000;
}

AudioCacheManager::SeekHintStatus AudioCacheManager::seekHintStatusForPlaybackUrl(
    const QUrl& playbackUrl, qint64 targetMs) const {
    SeekHintStatus status;
    const QUrl originalUrl = resolveSeekIndexSourceUrl(playbackUrl);
    if (!canCacheUrl(originalUrl) || targetMs < 0) {
        return status;
    }

    const QString cacheKey = makeCacheKey(originalUrl);
    const SeekHint hint = m_seekHints.value(cacheKey);
    if (hint.targetByte < 0 || hint.expiresAtMs < currentEpochMs()) {
        return status;
    }

    const qint64 nowMs = currentEpochMs();
    status.hasHint = true;
    status.targetMatches = qAbs(hint.targetMs - targetMs) <= 1500;
    status.seekIndexHit = hint.seekIndexHit;
    status.exact = hint.exact;
    status.confidenceScore = hint.confidenceScore;
    status.confidenceReason = hint.confidenceReason;
    status.takeoverStarted = hint.takeoverAtMs > 0;
    status.takeoverHeadersSent = hint.takeoverHeadersSentAtMs > 0;
    status.generation = hint.generation;
    status.targetMs = hint.targetMs;
    status.targetByte = hint.targetByte;
    status.rawTargetByte = hint.rawTargetByte;
    status.correctedTargetByte = hint.correctedTargetByte;
    status.correctionBytes = hint.correctionBytes;
    status.anchorTimeMs = hint.anchorTimeMs;
    status.preRollMs = hint.preRollMs;
    status.takeoverAgeMs = hint.takeoverAtMs > 0 ? nowMs - hint.takeoverAtMs : -1;

    for (auto it = m_proxyStreamStates.cbegin(); it != m_proxyStreamStates.cend(); ++it) {
        const ProxyStreamState& stream = it.value();
        if (stream.cacheKey != cacheKey || stream.seekHintGeneration != hint.generation ||
            !stream.seekHintTakeover) {
            continue;
        }

        ++status.activeProxyStreams;
        status.proxyOriginalRangeStart = stream.proxyOriginalRangeStart;
        status.proxyEffectiveRangeStart = stream.proxyEffectiveRangeStart;
        if (!stream.headerSent) {
            status.takeoverPending = true;
        } else {
            status.takeoverHeadersSent = true;
        }
    }

    return status;
}

AudioCacheManager::DirectSeekHint AudioCacheManager::directSeekHintForPlaybackUrl(
    const QUrl& playbackUrl, qint64 targetMs) const {
    DirectSeekHint result;
    const QUrl originalUrl = resolveSeekIndexSourceUrl(playbackUrl);
    if (!canCacheUrl(originalUrl) || targetMs < 0) {
        return result;
    }

    const QString cacheKey = makeCacheKey(originalUrl);
    const qint64 nowMs = currentEpochMs();
    const SeekHint registered = m_seekHints.value(cacheKey);
    if (registered.targetByte >= 0 && registered.expiresAtMs >= nowMs &&
        qAbs(registered.targetMs - targetMs) <= 1500) {
        result.valid = true;
        result.seekIndexHit = registered.seekIndexHit;
        result.exact = registered.exact;
        result.registeredHint = true;
        result.confidenceScore = registered.confidenceScore;
        result.confidenceReason = registered.confidenceReason;
        result.generation = registered.generation;
        result.targetMs = registered.targetMs;
        result.targetByte = registered.targetByte;
        result.rawTargetByte = registered.rawTargetByte;
        result.correctedTargetByte = registered.correctedTargetByte;
        result.correctionBytes = registered.correctionBytes;
        result.anchorTimeMs = registered.anchorTimeMs;
        result.preRollMs = registered.preRollMs;
        result.contentLength = registered.contentLength;
        return result;
    }

    TrackMeta meta;
    if (!loadTrackMeta(cacheKey, &meta)) {
        return result;
    }
    if (meta.contentLength <= 0 && meta.seekIndexFileSize > 0) {
        meta.contentLength = meta.seekIndexFileSize;
    }

    SeekIndexEstimate estimate = estimateCenterByteFromSeekIndex(meta, targetMs);
    qint64 targetByte = estimate.correctedTargetByte;
    const bool seekIndexHit = estimate.hit;
    if (!seekIndexHit && meta.contentLength > 0 && meta.durationMs > 0) {
        const double ratio =
            qBound(0.0, static_cast<double>(targetMs) / static_cast<double>(meta.durationMs), 1.0);
        targetByte = static_cast<qint64>(ratio * static_cast<double>(meta.contentLength - 1));
        estimate.confidenceScore = 20;
        estimate.confidenceReason = QStringLiteral("duration_ratio_fallback");
        estimate.rawTargetByte = targetByte;
        estimate.correctedTargetByte = targetByte;
        estimate.correctionBytes = 0;
        estimate.anchorTimeMs = targetMs;
        estimate.preRollMs = 0;
    }

    if (targetByte < 0 || meta.contentLength <= 0) {
        return result;
    }

    result.valid = true;
    result.seekIndexHit = estimate.hit;
    result.exact = estimate.exact;
    result.confidenceScore = estimate.confidenceScore;
    result.confidenceReason = estimate.confidenceReason;
    result.targetMs = targetMs;
    result.targetByte = qBound<qint64>(0, targetByte, meta.contentLength - 1);
    result.rawTargetByte = estimate.rawTargetByte;
    result.correctedTargetByte = result.targetByte;
    result.correctionBytes = estimate.correctionBytes;
    result.anchorTimeMs = estimate.anchorTimeMs;
    result.preRollMs = estimate.preRollMs;
    result.contentLength = meta.contentLength;
    return result;
}

QString AudioCacheManager::musicPathForSeekIndexUrl(const QUrl& url) const {
    if (!isRemoteHttpUrl(url)) {
        return QString();
    }

    const QString path = url.path();
    if (!path.startsWith(QStringLiteral("/uploads/"), Qt::CaseInsensitive)) {
        return QString();
    }

    QString musicPath =
        QUrl::fromPercentEncoding(path.mid(QStringLiteral("/uploads/").size()).toUtf8()).trimmed();
    musicPath.replace('\\', '/');
    if (musicPath.isEmpty() || musicPath == QStringLiteral(".") || musicPath.startsWith('/') ||
        QDir::isAbsolutePath(musicPath) || containsTraversalSegment(musicPath)) {
        return QString();
    }

    musicPath = QDir::cleanPath(musicPath);
    if (musicPath.isEmpty() || musicPath == QStringLiteral(".")) {
        return QString();
    }
    if (musicPath.startsWith('/') || QDir::isAbsolutePath(musicPath) ||
        containsTraversalSegment(musicPath)) {
        return QString();
    }

    return musicPath;
}

bool AudioCacheManager::shouldRefreshSeekIndex(const TrackMeta& meta) const {
    const qint64 nowMs = currentEpochMs();
    if (meta.seekIndexMusicPath.isEmpty()) {
        return true;
    }

    if (!meta.seekIndexResolved) {
        return meta.seekIndexFetchedAtMs <= 0 || nowMs - meta.seekIndexFetchedAtMs >= 60 * 1000;
    }

    if (!meta.seekIndexSupported) {
        return meta.seekIndexFetchedAtMs <= 0 ||
               nowMs - meta.seekIndexFetchedAtMs >= 6LL * 60 * 60 * 1000;
    }

    if (meta.seekIndexPoints.isEmpty() || meta.seekIndexVersion.isEmpty() ||
        meta.seekIndexFileSize <= 0) {
        return true;
    }

    return nowMs - meta.seekIndexFetchedAtMs >= 24LL * 60 * 60 * 1000;
}

void AudioCacheManager::ensureSeekIndex(const QUrl& originalUrl, const QString& cacheKey,
                                        qint64 knownDurationMs) {
    if (!canUseSeekIndexForUrl(originalUrl)) {
        return;
    }

    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);

    const QString musicPath = musicPathForSeekIndexUrl(originalUrl);
    if (musicPath.isEmpty()) {
        return;
    }

    bool metaChanged = false;
    if (knownDurationMs > 0 && meta.durationMs <= 0) {
        meta.durationMs = knownDurationMs;
        metaChanged = true;
    }
    if (meta.seekIndexMusicPath != musicPath) {
        meta.seekIndexResolved = false;
        meta.seekIndexSupported = false;
        meta.seekIndexFetchedAtMs = 0;
        meta.seekIndexFileSize = -1;
        meta.seekIndexVersion.clear();
        meta.seekIndexMusicPath = musicPath;
        meta.seekIndexPoints.clear();
        meta.seekIndexGoodSamples = 0;
        meta.seekIndexBadSamples = 0;
        meta.seekIndexGlobalBiasBytes = 0;
        meta.seekIndexGlobalBiasMs = 0;
        meta.seekIndexCorrectionUpdatedAtMs = 0;
        metaChanged = true;
    }

    if (metaChanged) {
        meta.lastAccessMs = currentEpochMs();
        {
            QMutexLocker locker(&m_metaMutex);
            m_trackMetaCache.insert(cacheKey, meta);
        }
        persistTrackMeta(cacheKey, meta);
    }

    if (!shouldRefreshSeekIndex(meta)) {
        return;
    }

    for (auto it = m_seekIndexRequests.cbegin(); it != m_seekIndexRequests.cend(); ++it) {
        if (it.value().cacheKey == cacheKey) {
            return;
        }
    }

    QUrl seekIndexUrl;
    seekIndexUrl.setScheme(originalUrl.scheme());
    seekIndexUrl.setHost(originalUrl.host());
    if (originalUrl.port() > 0) {
        seekIndexUrl.setPort(originalUrl.port());
    }
    seekIndexUrl.setPath(QStringLiteral("/music/local/seek-index"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("music_path"), musicPath);
    seekIndexUrl.setQuery(query);

    CurlNetworkTransport::Request request;
    request.url = seekIndexUrl.toString(QUrl::FullyEncoded);
    request.method = QStringLiteral("GET");
    request.userAgent = QStringLiteral("FFmpegMusicPlayer/1.0");
    request.timeoutMs = 8000;

    const quint64 requestId = nextRequestId();
    m_seekIndexRequests.insert(requestId, SeekIndexRequest{cacheKey, musicPath});
    launchSeekIndexRequest(requestId, SeekIndexRequest{cacheKey, musicPath}, request);

    qDebug() << "[AudioCache] Request seek index:"
             << "musicPath=" << musicPath << "url=" << seekIndexUrl.toString();
}

qint64 AudioCacheManager::estimateRawByteFromSeekIndex(const TrackMeta& meta, qint64 targetMs,
                                                       SeekIndexEstimate* estimate) const {
    if (estimate) {
        *estimate = SeekIndexEstimate{};
    }
    const bool hasServerIndex =
        meta.seekIndexResolved && meta.seekIndexSupported && !meta.seekIndexPoints.isEmpty();
    const bool usingLocalLearning = !hasServerIndex;
    const QVector<TrackMeta::SeekIndexPoint>& points =
        hasServerIndex ? meta.seekIndexPoints : meta.localSeekPoints;
    if (points.isEmpty() || targetMs < 0) {
        return -1;
    }

    if (usingLocalLearning) {
        if (points.size() == 1) {
            const TrackMeta::SeekIndexPoint& point = points.first();
            if (qAbs(targetMs - point.timeMs) > kLocalSeekSinglePointWindowMs) {
                if (estimate) {
                    estimate->confidenceReason =
                        QStringLiteral("local_seek_single_point_out_of_window");
                }
                return -1;
            }
        } else if (targetMs < points.first().timeMs || targetMs > points.last().timeMs) {
            if (estimate) {
                estimate->confidenceReason = QStringLiteral("local_seek_out_of_bracket");
            }
            return -1;
        }
    }

    const bool flacIndex = hasServerIndex && isFlacMusicPath(meta.seekIndexMusicPath);
    qint64 centerByte = points.first().byteOffset;
    if (targetMs <= points.first().timeMs) {
        centerByte = points.first().byteOffset;
        if (estimate) {
            estimate->previousTimeMs = points.first().timeMs;
            estimate->nextTimeMs = points.first().timeMs;
            estimate->previousByte = points.first().byteOffset;
            estimate->nextByte = points.first().byteOffset;
            estimate->anchorTimeMs = points.first().timeMs;
            estimate->preRollMs = qMax<qint64>(0, targetMs - points.first().timeMs);
        }
    } else if (targetMs >= points.last().timeMs) {
        centerByte = points.last().byteOffset;
        if (estimate) {
            estimate->previousTimeMs = points.last().timeMs;
            estimate->nextTimeMs = points.last().timeMs;
            estimate->previousByte = points.last().byteOffset;
            estimate->nextByte = points.last().byteOffset;
            estimate->anchorTimeMs = points.last().timeMs;
            estimate->preRollMs = qMax<qint64>(0, targetMs - points.last().timeMs);
        }
    } else {
        for (int i = 1; i < points.size(); ++i) {
            const TrackMeta::SeekIndexPoint& previous = points.at(i - 1);
            const TrackMeta::SeekIndexPoint& next = points.at(i);
            if (targetMs > next.timeMs) {
                continue;
            }

            if (next.timeMs <= previous.timeMs || next.byteOffset <= previous.byteOffset) {
                centerByte = previous.byteOffset;
                if (estimate) {
                    estimate->previousTimeMs = previous.timeMs;
                    estimate->nextTimeMs = next.timeMs;
                    estimate->previousByte = previous.byteOffset;
                    estimate->nextByte = next.byteOffset;
                    estimate->anchorTimeMs = previous.timeMs;
                    estimate->preRollMs = qMax<qint64>(0, targetMs - previous.timeMs);
                }
                break;
            }

            if (usingLocalLearning &&
                next.timeMs - previous.timeMs > kLocalSeekMaxBracketWindowMs) {
                if (estimate) {
                    estimate->confidenceReason = QStringLiteral("local_seek_sparse_bracket");
                }
                return -1;
            }

            if (flacIndex) {
                centerByte = previous.byteOffset;
            } else {
                const double ratio = static_cast<double>(targetMs - previous.timeMs) /
                                     static_cast<double>(next.timeMs - previous.timeMs);
                centerByte =
                    previous.byteOffset +
                    static_cast<qint64>(
                        ratio * static_cast<double>(next.byteOffset - previous.byteOffset));
            }
            if (estimate) {
                estimate->previousTimeMs = previous.timeMs;
                estimate->nextTimeMs = next.timeMs;
                estimate->previousByte = previous.byteOffset;
                estimate->nextByte = next.byteOffset;
                estimate->anchorTimeMs = flacIndex ? previous.timeMs : targetMs;
                estimate->preRollMs = flacIndex ? qMax<qint64>(0, targetMs - previous.timeMs) : 0;
            }
            break;
        }
    }

    const qint64 fileSize = qMax(meta.seekIndexFileSize, meta.contentLength);
    if (fileSize <= 0) {
        return -1;
    }
    const qint64 maxByte = qMax<qint64>(0, fileSize - 1);
    const qint64 boundedByte = qBound<qint64>(0, centerByte, maxByte);
    if (estimate) {
        estimate->hit = true;
        estimate->rawTargetByte = boundedByte;
        estimate->correctedTargetByte = boundedByte;
        if (estimate->anchorTimeMs < 0) {
            estimate->anchorTimeMs = targetMs;
            estimate->preRollMs = 0;
        }
        if (!hasServerIndex) {
            estimate->exact = false;
            const bool likelyFlac = isFlacMusicPath(meta.seekIndexMusicPath) ||
                                    meta.seekIndexMusicPath.isEmpty();
            const int upperBound = likelyFlac ? 55 : 78;
            estimate->confidenceScore = qBound(35, 45 + meta.seekIndexGoodSamples * 6 -
                                                       meta.seekIndexBadSamples * 8,
                                               upperBound);
            estimate->confidenceReason =
                likelyFlac ? QStringLiteral("local_flac_learning_approx")
                           : QStringLiteral("local_seek_learning_index");
        }
    }
    return boundedByte;
}

AudioCacheManager::SeekIndexEstimate AudioCacheManager::estimateCenterByteFromSeekIndex(
    const TrackMeta& meta, qint64 targetMs) const {
    SeekIndexEstimate estimate;
    const qint64 rawByte = estimateRawByteFromSeekIndex(meta, targetMs, &estimate);
    if (rawByte < 0) {
        estimate.confidenceReason = QStringLiteral("seek_index_unavailable");
        return estimate;
    }

    const qint64 fileSize = qMax(meta.seekIndexFileSize, meta.contentLength);
    if (fileSize <= 0) {
        estimate.hit = false;
        estimate.confidenceReason = QStringLiteral("missing_file_size");
        return estimate;
    }

    const bool serverIndex =
        meta.seekIndexResolved && meta.seekIndexSupported && !meta.seekIndexPoints.isEmpty();
    const bool flacIndex = serverIndex && isFlacMusicPath(meta.seekIndexMusicPath);
    const bool localLearning = !serverIndex;
    const bool localFlacLearning =
        localLearning && (isFlacMusicPath(meta.seekIndexMusicPath) ||
                          meta.seekIndexMusicPath.isEmpty());
    estimate.hit = true;
    estimate.rawTargetByte = rawByte;
    estimate.correctionBytes = localFlacLearning ? 0 : meta.seekIndexGlobalBiasBytes;
    estimate.correctedTargetByte =
        qBound<qint64>(0, rawByte + estimate.correctionBytes, qMax<qint64>(0, fileSize - 1));
    int confidence = flacIndex ? 90 : (serverIndex ? 55 : qMax(40, estimate.confidenceScore));
    QString reason = flacIndex ? QStringLiteral("flac_server_frame_anchor") :
                     (serverIndex ? QStringLiteral("server_index_default_non_exact") :
                                    (localFlacLearning
                                         ? QStringLiteral("local_flac_learning_approx")
                                         : QStringLiteral("local_seek_learning_index")));
    if (flacIndex && estimate.preRollMs > 3000) {
        confidence -= 20;
        reason = QStringLiteral("flac_anchor_preroll_large");
    }
    if (meta.seekIndexGoodSamples > 0 || meta.seekIndexBadSamples > 0) {
        confidence += qMin(25, meta.seekIndexGoodSamples * 8);
        confidence -= qMin(45, meta.seekIndexBadSamples * 10);
        reason = localFlacLearning ? QStringLiteral("local_flac_learning_feedback")
                                   : QStringLiteral("seek_observation_feedback");
    }
    if (qAbs(meta.seekIndexGlobalBiasMs) > 6500 ||
        qAbs(meta.seekIndexGlobalBiasBytes) > m_chunkSizeBytes) {
        confidence -= 25;
        reason = QStringLiteral("large_bias_observed");
    }
    confidence = qBound(0, confidence, 100);
    if (localFlacLearning) {
        confidence = qMin(confidence, 55);
    }

    estimate.confidenceScore = confidence;
    estimate.confidenceReason = reason;
    if (localFlacLearning) {
        estimate.exact = false;
    } else {
        estimate.exact = flacIndex
                             ? (confidence >= 85 && estimate.preRollMs <= 3000 &&
                                meta.seekIndexBadSamples == 0 &&
                                qAbs(meta.seekIndexGlobalBiasMs) <= 1500)
                             : (confidence >= 85 && meta.seekIndexGoodSamples >= 2 &&
                                meta.seekIndexBadSamples == 0 &&
                                qAbs(meta.seekIndexGlobalBiasMs) <= 1500);
    }
    return estimate;
}

void AudioCacheManager::registerSeekHint(const QUrl& originalUrl, const QString& cacheKey,
                                         qint64 targetMs, qint64 contentLength,
                                         const SeekIndexEstimate& estimate) {
    const qint64 targetByte = estimate.correctedTargetByte;
    if (!canCacheUrl(originalUrl) || cacheKey.isEmpty() || targetMs < 0 || targetByte < 0) {
        return;
    }

    SeekHint hint;
    hint.targetMs = targetMs;
    hint.targetByte = targetByte;
    hint.contentLength = contentLength;
    hint.seekIndexHit = estimate.hit;
    hint.exact = estimate.exact;
    hint.confidenceScore = estimate.confidenceScore;
    hint.confidenceReason = estimate.confidenceReason;
    hint.rawTargetByte = estimate.rawTargetByte;
    hint.correctedTargetByte = estimate.correctedTargetByte;
    hint.correctionBytes = estimate.correctionBytes;
    hint.anchorTimeMs = estimate.anchorTimeMs;
    hint.preRollMs = estimate.preRollMs;
    hint.generation = ++m_seekHintGeneration;
    hint.expiresAtMs = currentEpochMs() + 15000;
    m_seekHints.insert(cacheKey, hint);

    cancelProxyStreamsForCacheKey(cacheKey);
    cancelBackgroundRangesForCacheKey(cacheKey);

    qDebug() << "[AudioCache] Seek hint registered:"
             << "generation=" << hint.generation << "targetMs=" << targetMs
             << "targetByte=" << targetByte << "contentLength=" << contentLength
             << "seekIndexHit=" << hint.seekIndexHit << "exact=" << hint.exact
             << "confidenceScore=" << hint.confidenceScore
             << "confidenceReason=" << hint.confidenceReason
             << "rawTargetByte=" << hint.rawTargetByte
             << "correctedTargetByte=" << hint.correctedTargetByte
             << "correctionBytes=" << hint.correctionBytes
             << "anchorTimeMs=" << hint.anchorTimeMs << "preRollMs=" << hint.preRollMs
             << "url=" << originalUrl.toString();
}

bool AudioCacheManager::resolveSeekHintForRange(const QString& cacheKey, qint64 requestStart,
                                                qint64 knownTotal, SeekHint* out) const {
    if (cacheKey.isEmpty() || requestStart < 0 || !out) {
        return false;
    }

    const SeekHint hint = m_seekHints.value(cacheKey);
    if (hint.targetByte < 0 || hint.expiresAtMs < currentEpochMs()) {
        return false;
    }
    if (hint.takeoverHeadersSentAtMs > 0) {
        return false;
    }
    const qint64 total = knownTotal > 0 ? knownTotal : hint.contentLength;
    if (total > 0 && hint.targetByte >= total) {
        return false;
    }

    const qint64 tolerance =
        hint.exact ? 128 * 1024 : qMax<qint64>(384 * 1024, m_chunkSizeBytes);
    if (qAbs(requestStart - hint.targetByte) <= tolerance) {
        qDebug() << "[AudioCache] Seek hint range already near target:"
                 << "requestStart=" << requestStart << "targetByte=" << hint.targetByte
                 << "exact=" << hint.exact << "confidenceScore=" << hint.confidenceScore
                 << "confidenceReason=" << hint.confidenceReason
                 << "rawTargetByte=" << hint.rawTargetByte
                 << "correctedTargetByte=" << hint.correctedTargetByte
                 << "correctionBytes=" << hint.correctionBytes << "tolerance=" << tolerance;
        return false;
    }

    *out = hint;
    qDebug() << "[AudioCache] Seek hint range takeover:"
             << "requestStart=" << requestStart << "targetByte=" << hint.targetByte
             << "exact=" << hint.exact << "confidenceScore=" << hint.confidenceScore
             << "confidenceReason=" << hint.confidenceReason
             << "rawTargetByte=" << hint.rawTargetByte
             << "correctedTargetByte=" << hint.correctedTargetByte
             << "correctionBytes=" << hint.correctionBytes << "tolerance=" << tolerance;
    return true;
}

void AudioCacheManager::markSeekHintTakeover(const QString& cacheKey, const SeekHint& hint) {
    if (cacheKey.isEmpty()) {
        return;
    }

    SeekHint current = m_seekHints.value(cacheKey);
    if (current.generation != hint.generation) {
        return;
    }
    if (current.takeoverAtMs <= 0) {
        current.takeoverAtMs = currentEpochMs();
    }
    ++current.takeoverCount;
    m_seekHints.insert(cacheKey, current);
}

void AudioCacheManager::markSeekHintHeadersSent(const QString& cacheKey, int generation) {
    if (cacheKey.isEmpty() || generation <= 0) {
        return;
    }

    SeekHint current = m_seekHints.value(cacheKey);
    if (current.generation != generation) {
        return;
    }
    current.takeoverHeadersSentAtMs = currentEpochMs();
    m_seekHints.insert(cacheKey, current);
}

void AudioCacheManager::cancelProxyStreamsForCacheKey(const QString& cacheKey) {
    if (cacheKey.isEmpty()) {
        return;
    }

    const QList<quint64> requests = m_proxyStreamStates.keys();
    int cancelled = 0;
    for (quint64 requestId : requests) {
        const ProxyStreamState state = m_proxyStreamStates.value(requestId);
        if (state.cacheKey != cacheKey) {
            continue;
        }
        m_proxyStreamStates.remove(requestId);
        if (!state.socket.isNull()) {
            state.socket->disconnectFromHost();
        }
        abortRequest(requestId);
        ++cancelled;
    }
    if (cancelled > 0) {
        qDebug() << "[AudioCacheProxy] Cancel proxy streams for seek hint:"
                 << "cacheKey=" << cacheKey << "count=" << cancelled;
    }
}

void AudioCacheManager::cancelBackgroundRangesForCacheKey(const QString& cacheKey) {
    if (cacheKey.isEmpty()) {
        return;
    }

    const QList<quint64> requests = m_downloadTasks.keys();
    int cancelled = 0;
    for (quint64 requestId : requests) {
        const DownloadTask task = m_downloadTasks.value(requestId);
        if (task.cacheKey != cacheKey || task.metadataProbe || task.fullFileDownload) {
            continue;
        }
        abortRequest(requestId);
        ++cancelled;
    }
    if (cancelled > 0) {
        qDebug() << "[AudioCache] Cancel background range downloads for seek hint:"
                 << "cacheKey=" << cacheKey << "count=" << cancelled;
    }
}

quint64 AudioCacheManager::nextRequestId() {
    return m_nextRequestId++;
}

std::shared_ptr<std::atomic_bool> AudioCacheManager::registerAbortFlag(quint64 requestId) {
    auto flag = std::make_shared<std::atomic_bool>(false);
    m_abortFlags.insert(requestId, flag);
    return flag;
}

void AudioCacheManager::abortRequest(quint64 requestId) {
    const auto flag = m_abortFlags.value(requestId);
    if (flag) {
        flag->store(true);
    }
}

void AudioCacheManager::launchSeekIndexRequest(quint64 requestId,
                                               const SeekIndexRequest& requestInfo,
                                               const CurlNetworkTransport::Request& request) {
    auto abortFlag = registerAbortFlag(requestId);
    CurlNetworkTransport::Request transportRequest = request;
    transportRequest.shouldAbort = [abortFlag]() {
        return abortFlag && abortFlag->load();
    };
    QtConcurrent::run([this, requestId, requestInfo, transportRequest]() {
        Q_UNUSED(requestInfo);
        const CurlNetworkTransport::Response response =
            CurlNetworkTransport::perform(transportRequest);
        QMetaObject::invokeMethod(
            this,
            [this, requestId, response]() {
                handleSeekIndexResponse(requestId, response);
            },
            Qt::QueuedConnection);
    });
}

void AudioCacheManager::handleSeekIndexResponse(
    quint64 requestId, const CurlNetworkTransport::Response& response) {
    const auto it = m_seekIndexRequests.find(requestId);
    if (it == m_seekIndexRequests.end()) {
        m_abortFlags.remove(requestId);
        return;
    }

    const SeekIndexRequest request = it.value();
    m_seekIndexRequests.erase(it);
    m_abortFlags.remove(requestId);

    TrackMeta meta;
    loadTrackMeta(request.cacheKey, &meta);
    const bool hadUsableIndex =
        meta.seekIndexResolved && meta.seekIndexSupported && !meta.seekIndexPoints.isEmpty();

    const qint64 nowMs = currentEpochMs();
    meta.lastAccessMs = nowMs;
    meta.seekIndexFetchedAtMs = nowMs;
    if (meta.seekIndexMusicPath.isEmpty()) {
        meta.seekIndexMusicPath = request.musicPath;
    }

    bool parsedResponse = false;
    const int statusCode = response.statusCode;
    if (response.error == CurlNetworkTransport::Error::NoError && statusCode == 200) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(response.body, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject obj = doc.object();
            const bool supported = obj.value(QStringLiteral("supported")).toBool(false);
            const QString responseMusicPath =
                obj.value(QStringLiteral("music_path")).toString().trimmed();
            const QString resolvedMusicPath =
                responseMusicPath.isEmpty() ? request.musicPath : responseMusicPath;
            const qint64 fileSize =
                static_cast<qint64>(obj.value(QStringLiteral("file_size")).toDouble(-1));
            const qint64 durationMs =
                static_cast<qint64>(obj.value(QStringLiteral("duration_ms")).toDouble(-1));
            const QString version = obj.value(QStringLiteral("version")).toString().trimmed();

            QVector<TrackMeta::SeekIndexPoint> points;
            const QJsonArray pointsArray = obj.value(QStringLiteral("points")).toArray();
            points.reserve(pointsArray.size());
            for (const QJsonValue& value : pointsArray) {
                const QJsonObject pointObj = value.toObject();
                const qint64 timeMs =
                    static_cast<qint64>(pointObj.value(QStringLiteral("time_ms")).toDouble(-1));
                const qint64 byteOffset =
                    static_cast<qint64>(pointObj.value(QStringLiteral("byte_offset")).toDouble(-1));
                if (timeMs < 0 || byteOffset < 0) {
                    continue;
                }
                points.push_back(TrackMeta::SeekIndexPoint{timeMs, byteOffset});
            }
            std::sort(
                points.begin(), points.end(),
                [](const TrackMeta::SeekIndexPoint& lhs, const TrackMeta::SeekIndexPoint& rhs) {
                    if (lhs.timeMs == rhs.timeMs) {
                        return lhs.byteOffset < rhs.byteOffset;
                    }
                    return lhs.timeMs < rhs.timeMs;
                });

            const QString previousMusicPath = meta.seekIndexMusicPath;
            const QString previousVersion = meta.seekIndexVersion;
            const qint64 previousFileSize = meta.seekIndexFileSize;
            const bool indexIdentityChanged = previousMusicPath != resolvedMusicPath ||
                                              previousVersion != version ||
                                              previousFileSize != fileSize;
            meta.seekIndexResolved = true;
            meta.seekIndexSupported = supported && !points.isEmpty();
            meta.seekIndexMusicPath = resolvedMusicPath;
            meta.seekIndexFileSize = fileSize;
            meta.seekIndexVersion = version;
            meta.seekIndexPoints =
                meta.seekIndexSupported ? points : QVector<TrackMeta::SeekIndexPoint>{};
            if (indexIdentityChanged) {
                meta.seekIndexGoodSamples = 0;
                meta.seekIndexBadSamples = 0;
                meta.seekIndexGlobalBiasBytes = 0;
                meta.seekIndexGlobalBiasMs = 0;
                meta.seekIndexCorrectionUpdatedAtMs = 0;
                meta.localSeekPoints.clear();
            }
            if (durationMs > 0) {
                meta.durationMs = durationMs;
            }
            if (fileSize > 0) {
                meta.contentLength = fileSize;
            }

            parsedResponse = true;
            qDebug() << "[AudioCache] Seek index response:"
                     << "musicPath=" << meta.seekIndexMusicPath
                     << "supported=" << meta.seekIndexSupported
                     << "points=" << meta.seekIndexPoints.size()
                     << "version=" << meta.seekIndexVersion;
        }
    }

    if (!parsedResponse) {
        if (!hadUsableIndex) {
            meta.seekIndexResolved = false;
            meta.seekIndexSupported = false;
            meta.seekIndexFileSize = -1;
            meta.seekIndexVersion.clear();
            meta.seekIndexPoints.clear();
        }
        qWarning() << "[AudioCache] Seek index request failed:"
                   << "musicPath=" << request.musicPath << "status=" << statusCode
                   << "error=" << response.errorString;
    }

    {
        QMutexLocker locker(&m_metaMutex);
        m_trackMetaCache.insert(request.cacheKey, meta);
    }
    persistTrackMeta(request.cacheKey, meta);
}

void AudioCacheManager::launchDownloadTask(quint64 requestId, const DownloadTask& task,
                                           const CurlNetworkTransport::Request& request) {
    auto abortFlag = registerAbortFlag(requestId);
    CurlNetworkTransport::Request transportRequest = request;
    transportRequest.shouldAbort = [abortFlag]() {
        return abortFlag && abortFlag->load();
    };
    QtConcurrent::run([this, requestId, task, transportRequest]() {
        Q_UNUSED(task);
        const CurlNetworkTransport::Response response =
            CurlNetworkTransport::perform(transportRequest);
        QMetaObject::invokeMethod(
            this,
            [this, requestId, response]() {
                handleDownloadResponse(requestId, response);
            },
            Qt::QueuedConnection);
    });
}

void AudioCacheManager::handleDownloadResponse(
    quint64 requestId, const CurlNetworkTransport::Response& response) {
    const auto it = m_downloadTasks.find(requestId);
    if (it == m_downloadTasks.end()) {
        m_abortFlags.remove(requestId);
        return;
    }

    const DownloadTask task = it.value();
    m_downloadTasks.erase(it);
    m_abortFlags.remove(requestId);
    if (task.fullFileDownload) {
        m_inflightChunkKeys.remove(task.cacheKey + ":full");
    } else {
        m_inflightChunkKeys.remove(task.cacheKey + ":" + QString::number(task.chunkIndex));
    }
    if (task.metadataProbe) {
        m_inflightChunkKeys.remove(task.cacheKey + ":probe");
    }

    const QByteArray payload = response.body;
    const bool responseLooksAudio = isLikelyAudioResponse(response.statusCode, response.headers);
    const bool ok = (response.error == CurlNetworkTransport::Error::NoError) &&
                    !payload.isEmpty() &&
                    (task.metadataProbe || responseLooksAudio);

    if (ok && task.fullFileDownload) {
        const QString partPath = task.targetFilePath + QStringLiteral(".part");
        QFile out(partPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(payload);
            out.close();

            QFile::remove(task.targetFilePath);
            if (QFile::rename(partPath, task.targetFilePath)) {
                QFile(task.targetFilePath)
                    .setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                    QFileDevice::ReadUser | QFileDevice::WriteUser);
                qDebug() << "[AudioCache] Full cache completed:"
                         << "bytes=" << payload.size() << "path=" << task.targetFilePath;
                emit fullFileCacheReady(task.originalUrl, QUrl::fromLocalFile(task.targetFilePath));
            } else {
                QFile::remove(partPath);
                qWarning() << "[AudioCache] Full cache rename failed:" << task.targetFilePath;
            }
        } else {
            qWarning() << "[AudioCache] Cannot write full cache file:" << partPath;
        }
    } else if (ok && !task.metadataProbe) {
        const QString partPath = task.targetFilePath + QStringLiteral(".part");
        QFile out(partPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(payload);
            out.close();

            QFile::remove(task.targetFilePath);
            if (QFile::rename(partPath, task.targetFilePath)) {
                QFile(task.targetFilePath)
                    .setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                    QFileDevice::ReadUser | QFileDevice::WriteUser);
                qDebug() << "[AudioCache] Chunk cached:" << task.chunkIndex
                         << "bytes:" << payload.size();
            } else {
                QFile::remove(partPath);
                qWarning() << "[AudioCache] Chunk rename failed:" << task.targetFilePath;
            }
        } else {
            qWarning() << "[AudioCache] Cannot write chunk cache file:" << partPath;
        }
    } else if (!ok) {
        qWarning() << (task.fullFileDownload ? "[AudioCache] Full cache download failed:"
                                             : "[AudioCache] Chunk download failed:")
                   << "chunk=" << task.chunkIndex << "url=" << task.originalUrl.toString()
                   << "status=" << response.statusCode
                   << "contentType=" << headerValueBytes(response.headers,
                                                           QStringLiteral("Content-Type"))
                   << "error=" << response.errorString;
    }

    // Update metadata from response headers.
    qint64 contentLength = contentLengthFromHeaders(response.headers);
    const QString contentRange = headerValue(response.headers, QStringLiteral("Content-Range"));
    const qint64 rangeTotal = parseContentRangeLength(contentRange);
    if (rangeTotal > 0) {
        contentLength = rangeTotal;
    }

    TrackMeta meta;
    loadTrackMeta(task.cacheKey, &meta);
    if (contentLength > 0) {
        meta.contentLength = contentLength;
    }
    meta.lastAccessMs = currentEpochMs();
    updateTrackMeta(task.cacheKey, meta.contentLength, meta.durationMs);

    if (task.metadataProbe && shouldFullCacheByAudioType(task.originalUrl) &&
        meta.contentLength > 0) {
        queueFullFileDownload(task.originalUrl, task.cacheKey, meta.contentLength,
                              CacheRequestPriority::High);
    }

    maybeRunLruCleanup();
}

void AudioCacheManager::startProxyServerIfNeeded() {
    if (m_proxyServer && m_proxyServer->isListening()) {
        return;
    }

    if (!m_proxyServer) {
        m_proxyServer = new QTcpServer(this);
        connect(m_proxyServer, &QTcpServer::newConnection, this,
                &AudioCacheManager::onProxyNewConnection);
    }

    if (!m_proxyServer->listen(QHostAddress::LocalHost, 0)) {
        qWarning() << "[AudioCacheProxy] Listen failed:" << m_proxyServer->errorString();
        m_proxyPort = 0;
        return;
    }

    m_proxyPort = m_proxyServer->serverPort();
    qDebug() << "[AudioCacheProxy] Listening on 127.0.0.1:" << m_proxyPort;
}

void AudioCacheManager::onProxyNewConnection() {
    if (!m_proxyServer) {
        return;
    }

    while (m_proxyServer->hasPendingConnections()) {
        QTcpSocket* socket = m_proxyServer->nextPendingConnection();
        if (!socket) {
            continue;
        }

        const auto destroyedSignal = static_cast<void (QObject::*)(QObject*)>(&QObject::destroyed);
        const auto readyReadSlot =
            static_cast<void (AudioCacheManager::*)()>(&AudioCacheManager::onProxySocketReadyRead);
        connect(socket, destroyedSignal, this, &AudioCacheManager::onProxySocketDestroyed);
        connect(socket, &QTcpSocket::readyRead, this, readyReadSlot);
        connect(socket, &QTcpSocket::disconnected, this,
                &AudioCacheManager::onProxySocketDisconnected);
    }
}

void AudioCacheManager::onProxySocketReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    onProxySocketReadyRead(socket);
}

void AudioCacheManager::onProxySocketDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    m_proxyReadBuffers.remove(socket);
    socket->deleteLater();
}

void AudioCacheManager::onProxySocketDestroyed(QObject* obj) {
    auto* socket = qobject_cast<QTcpSocket*>(obj);
    if (!socket) {
        return;
    }
    m_proxyReadBuffers.remove(socket);
}

void AudioCacheManager::onProxySocketReadyRead(QTcpSocket* socket) {
    if (!socket) {
        return;
    }

    QByteArray& buffer = m_proxyReadBuffers[socket];
    buffer.append(socket->readAll());
    if (buffer.size() > 64 * 1024) {
        sendProxyError(socket, 413, "Request Header Too Large");
        return;
    }

    int delimiterLen = 0;
    const int endPos = findHttpHeaderEnd(buffer, &delimiterLen);
    if (endPos < 0) {
        return;
    }

    const QByteArray headers = buffer.left(endPos + delimiterLen);
    m_proxyReadBuffers.remove(socket);
    handleProxyRequest(socket, headers);
}

void AudioCacheManager::handleProxyRequest(QTcpSocket* socket, const QByteArray& requestHeaders) {
    if (!socket) {
        return;
    }

    const QList<QByteArray> lines = requestHeaders.split('\n');
    if (lines.isEmpty()) {
        sendProxyError(socket, 400, "Bad Request");
        return;
    }

    const QByteArray requestLine = lines.first().trimmed();
    const QList<QByteArray> requestParts = requestLine.split(' ');
    if (requestParts.size() < 2 || requestParts.first() != "GET") {
        sendProxyError(socket, 405, "Method Not Allowed");
        return;
    }

    const QUrl requestUrl(QStringLiteral("http://127.0.0.1") +
                          QString::fromUtf8(requestParts.at(1)));
    const QUrlQuery query(requestUrl);
    const QString srcText = query.queryItemValue(QStringLiteral("src"));
    const QUrl srcUrl(srcText);
    if (!srcUrl.isValid() || srcUrl.isEmpty()) {
        sendProxyError(socket, 400, "Invalid Source");
        return;
    }

    qint64 rangeStart = -1;
    qint64 rangeEnd = -1;
    bool hasRange = false;
    QByteArray rangeHeaderRaw;
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        if (line.toLower().startsWith("range:")) {
            const QByteArray value = line.mid(line.indexOf(':') + 1).trimmed();
            if (parseHttpRange(value, &rangeStart, &rangeEnd)) {
                hasRange = true;
                rangeHeaderRaw = value;
            }
            break;
        }
    }

    const QString cacheKey = makeCacheKey(srcUrl);
    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    const qint64 knownTotal = qMax(meta.contentLength, meta.seekIndexFileSize);
    if (knownTotal <= 0) {
        ensureTrackMeta(srcUrl, cacheKey);
    }
    const QByteArray fallbackContentType = audioContentTypeForUrl(srcUrl);
    const QString fullCachePath = legacyFullCachePathForUrl(srcUrl);
    const QFileInfo fullCacheInfo(fullCachePath);
    if (fullCacheInfo.exists() && fullCacheInfo.isFile() && fullCacheInfo.size() > 0) {
        QFile fullFile(fullCachePath);
        if (fullFile.open(QIODevice::ReadOnly)) {
            const qint64 fileSize = fullCacheInfo.size();
            qint64 responseStart = hasRange && rangeStart >= 0 ? rangeStart : 0;
            qint64 responseEnd = hasRange && rangeEnd >= rangeStart ? rangeEnd : fileSize - 1;
            responseStart = qBound<qint64>(0, responseStart, fileSize - 1);
            responseEnd = qBound<qint64>(responseStart, responseEnd, fileSize - 1);
            const qint64 responseLength = responseEnd - responseStart + 1;
            if (responseLength > 0 && fullFile.seek(responseStart)) {
                const QByteArray payload = fullFile.read(responseLength);
                QByteArray extra;
                if (hasRange) {
                    extra += QByteArray("Content-Range: bytes ") +
                             QByteArray::number(responseStart) + "-" +
                             QByteArray::number(responseStart + payload.size() - 1) + "/" +
                             QByteArray::number(fileSize) + "\r\n";
                }
                socket->write(
                    buildHttpHeaders(hasRange ? 206 : 200,
                                     hasRange ? QByteArray("Partial Content") : QByteArray("OK"),
                                     fallbackContentType, payload.size(), extra));
                socket->write(payload);
                socket->disconnectFromHost();
                qDebug()
                    << "[AudioCacheProxy] Full cache response:"
                    << "range="
                    << QString("%1-%2").arg(responseStart).arg(responseStart + payload.size() - 1)
                    << "bytes=" << payload.size() << "path=" << fullCachePath;
                return;
            }
        }
    }

    if (hasRange && rangeStart >= 0) {
        qint64 normalizedRangeEnd = rangeEnd;
        if (normalizedRangeEnd < rangeStart) {
            normalizedRangeEnd = rangeStart + (m_chunkSizeBytes * 4) - 1;
        }
        if (knownTotal > 0) {
            normalizedRangeEnd = qMin(normalizedRangeEnd, knownTotal - 1);
        }

        if (tryServeMemoryRange(socket, cacheKey, fallbackContentType, rangeStart,
                                normalizedRangeEnd, knownTotal)) {
            return;
        }
        if (tryServeCachedRange(socket, cacheKey, fallbackContentType, rangeStart,
                                normalizedRangeEnd, knownTotal)) {
            return;
        }
    }

    const qint64 proxyOriginalRangeStart = hasRange && rangeStart >= 0 ? rangeStart : -1;
    qint64 upstreamStart = hasRange && rangeStart >= 0 ? rangeStart : 0;
    const qint64 upstreamEnd = hasRange && rangeEnd >= rangeStart ? rangeEnd : -1;
    SeekHint seekHint;
    bool seekHintTakeover = false;
    if (hasRange && rangeStart >= 0 &&
        resolveSeekHintForRange(cacheKey, rangeStart, knownTotal, &seekHint)) {
        upstreamStart = seekHint.targetByte;
        seekHintTakeover = true;
        markSeekHintTakeover(cacheKey, seekHint);
    }

    qDebug() << "[AudioCacheProxy] Request:"
             << "src=" << srcUrl.toString() << "hasRange=" << hasRange
             << "rangeStart=" << rangeStart << "rangeEnd=" << rangeEnd
             << "upstreamStart=" << upstreamStart << "upstreamEnd=" << upstreamEnd
             << "seekHintTakeover=" << seekHintTakeover
             << "seekTargetMs=" << seekHint.targetMs
             << "seekHintExact=" << seekHint.exact
             << "seekConfidenceScore=" << seekHint.confidenceScore
             << "seekConfidenceReason=" << seekHint.confidenceReason
             << "seekRawTargetByte=" << seekHint.rawTargetByte
             << "seekCorrectedTargetByte=" << seekHint.correctedTargetByte
             << "seekCorrectionBytes=" << seekHint.correctionBytes
             << "proxy_original_range_start=" << proxyOriginalRangeStart
             << "proxy_effective_range_start=" << upstreamStart;

    ProxyStreamState state;
    state.socket = socket;
    state.sourceUrl = srcUrl;
    state.rangeHeaderRaw = rangeHeaderRaw;
    state.hasRange = hasRange;
    state.rangeStart = rangeStart;
    state.rangeEnd = rangeEnd;
    state.upstreamStartByte = upstreamStart;
    state.upstreamEndByte = upstreamEnd;
    state.proxyOriginalRangeStart = proxyOriginalRangeStart;
    state.proxyEffectiveRangeStart = upstreamStart;
    state.seekHintTakeover = seekHintTakeover;
    state.seekTargetMs = seekHint.targetMs;
    state.seekHintGeneration = seekHint.generation;
    state.cacheKey = cacheKey;
    startProxyUpstreamRequest(state);
}

void AudioCacheManager::startProxyUpstreamRequest(const ProxyStreamState& state) {
    if (state.socket.isNull() || state.socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }

    qint64 upstreamStart = state.upstreamStartByte >= 0 ? state.upstreamStartByte : 0;
    QByteArray upstreamRange = QByteArray("bytes=") + QByteArray::number(upstreamStart) + "-";
    if (state.upstreamEndByte >= upstreamStart) {
        upstreamRange += QByteArray::number(state.upstreamEndByte);
    }

    ProxyStreamState nextState = state;
    nextState.upstreamStartByte = upstreamStart;
    nextState.upstreamRangeHeaderRaw = upstreamRange;
    nextState.upstreamHeadersValidated = false;
    nextState.upstreamBytesForwarded = 0;
    if (nextState.upstreamEndByte >= upstreamStart) {
        nextState.expectedUpstreamBytes = nextState.upstreamEndByte - upstreamStart + 1;
    } else {
        nextState.expectedUpstreamBytes = -1;
    }
    if (!nextState.headerSent) {
        nextState.responseStartByte = -1;
        nextState.nextCacheByte = -1;
        nextState.nextMemoryByte = -1;
    }
    const quint64 requestId = nextRequestId();
    m_proxyStreamStates.insert(requestId, nextState);

    auto abortFlag = registerAbortFlag(requestId);
    if (!nextState.socket.isNull()) {
        connect(nextState.socket.data(), &QObject::destroyed, this, [this, requestId]() {
            abortRequest(requestId);
        });
    }

    CurlNetworkTransport::Request request;
    request.url = nextState.sourceUrl.toString(QUrl::FullyEncoded);
    request.method = QStringLiteral("GET");
    request.userAgent = QStringLiteral("FFmpegMusicPlayer/1.0");
    request.timeoutMs = 120000;
    request.headers.insert(QStringLiteral("Accept"), QStringLiteral("*/*"));
    request.headers.insert(QStringLiteral("Connection"), QStringLiteral("keep-alive"));
    request.headers.insert(QStringLiteral("Range"), QString::fromLatin1(upstreamRange));
    request.shouldAbort = [abortFlag]() {
        return abortFlag && abortFlag->load();
    };
    request.onHeaders = [this, requestId](int statusCode, const QMap<QString, QString>& headers) {
        QMetaObject::invokeMethod(
            this,
            [this, requestId, statusCode, headers]() {
                handleProxyHeaders(requestId, statusCode, headers);
            },
            Qt::QueuedConnection);
    };
    request.onData = [this, requestId](const QByteArray& chunk) {
        QMetaObject::invokeMethod(
            this,
            [this, requestId, chunk]() {
                handleProxyData(requestId, chunk);
            },
            Qt::QueuedConnection);
        return true;
    };

    QtConcurrent::run([this, requestId, request]() {
        const CurlNetworkTransport::Response response = CurlNetworkTransport::perform(request);
        QMetaObject::invokeMethod(
            this,
            [this, requestId, response]() {
                handleProxyFinished(requestId, response);
            },
            Qt::QueuedConnection);
    });

    qDebug() << "[AudioCacheProxy] Upstream request:"
             << "retry=" << nextState.retryCount << "range=" << nextState.upstreamRangeHeaderRaw
             << "seekHintTakeover=" << nextState.seekHintTakeover
             << "seekTargetMs=" << nextState.seekTargetMs
             << "proxy_original_range_start=" << nextState.proxyOriginalRangeStart
             << "proxy_effective_range_start=" << nextState.proxyEffectiveRangeStart
             << "src=" << nextState.sourceUrl.toString();
}

void AudioCacheManager::handleProxyHeaders(quint64 requestId, int statusCode,
                                           const QMap<QString, QString>& headers) {
    auto it = m_proxyStreamStates.find(requestId);
    if (it == m_proxyStreamStates.end()) {
        return;
    }
    ProxyStreamState& state = it.value();
    ensureProxyHeadersSent(statusCode, headers, &state);
}

void AudioCacheManager::handleProxyData(quint64 requestId, const QByteArray& chunk) {
    auto it = m_proxyStreamStates.find(requestId);
    if (it == m_proxyStreamStates.end()) {
        return;
    }
    ProxyStreamState& state = it.value();
    if (chunk.isEmpty() || state.socket.isNull()) {
        return;
    }
    if (!state.headerSent) {
        state.pendingBody.append(chunk);
        return;
    }

    forwardProxyChunk(&state, chunk);
}

void AudioCacheManager::forwardProxyChunk(ProxyStreamState* state, const QByteArray& chunk) {
    if (!state || chunk.isEmpty() || state->socket.isNull()) {
        return;
    }

    state->bytesForwarded += chunk.size();
    state->upstreamBytesForwarded += chunk.size();
    state->socket->write(chunk);

    if (state->hasRange && state->rangeStart >= 0 && state->rangeEnd >= state->rangeStart &&
        (state->rangeStart % m_chunkSizeBytes == 0) &&
        (state->rangeEnd - state->rangeStart + 1) <= m_chunkSizeBytes &&
        state->collectedForCache.size() < m_chunkSizeBytes) {
        state->collectedForCache.append(chunk);
    }
    appendMemoryRangeData(state, chunk);
    appendProxyCacheData(state, chunk);
}

void AudioCacheManager::handleProxyFinished(
    quint64 requestId, const CurlNetworkTransport::Response& response) {
    auto it = m_proxyStreamStates.find(requestId);
    if (it == m_proxyStreamStates.end()) {
        m_abortFlags.remove(requestId);
        return;
    }
    ProxyStreamState state = it.value();
    m_proxyStreamStates.erase(it);
    m_abortFlags.remove(requestId);

    if (state.socket.isNull() || state.socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }

    const int statusCode = response.statusCode;
    const bool retryableStatus = statusCode == 0 || statusCode == 408 || statusCode == 429 ||
                                 (statusCode >= 500 && statusCode <= 599);
    const bool networkError = response.error != CurlNetworkTransport::Error::NoError;
    const bool failedOrRetryable = networkError || retryableStatus;
    if (failedOrRetryable) {
        if (!state.headerSent && (retryableStatus || networkError) &&
            state.retryCount < state.maxRetries) {
            ++state.retryCount;
            state.bytesForwarded = 0;
            state.upstreamBytesForwarded = 0;
            state.pendingBody.clear();
            state.collectedForCache.clear();
            state.cacheWriteBuffer.clear();
            state.responseStartByte = -1;
            state.nextCacheByte = -1;
            state.nextMemoryByte = -1;
            qWarning() << "[AudioCacheProxy] Upstream retry:"
                       << "retry=" << state.retryCount << "/" << state.maxRetries
                       << "status=" << statusCode << "range=" << state.upstreamRangeHeaderRaw
                       << "error=" << response.errorString
                       << "src=" << state.sourceUrl.toString();
            startProxyUpstreamRequest(state);
            return;
        }
        if (state.headerSent && tryResumeProxyUpstream(&state, response.errorString)) {
            return;
        }

        qWarning() << "[AudioCacheProxy] Upstream error:"
                   << "status=" << statusCode << "retry=" << state.retryCount << "/"
                   << state.maxRetries << "range=" << state.upstreamRangeHeaderRaw
                   << "error=" << response.errorString;
        if (!state.headerSent) {
            sendProxyError(state.socket, 502, "Bad Gateway");
        } else {
            state.socket->disconnectFromHost();
        }
        return;
    }

    if (!ensureProxyHeadersSent(statusCode, response.headers, &state)) {
        return;
    }

    const bool shortUpstream =
        state.expectedUpstreamBytes > 0 && state.upstreamBytesForwarded < state.expectedUpstreamBytes;
    if (state.headerSent && shortUpstream &&
        tryResumeProxyUpstream(&state, QStringLiteral("short upstream response"))) {
        return;
    }

    state.socket->flush();
    state.socket->disconnectFromHost();

    persistAlignedProxyChunk(state);
    updateTrackMetaFromProxyHeaders(state.cacheKey, response.headers);
    maybeRunLruCleanup();

    qDebug() << "[AudioCacheProxy] Response finished, bytes:" << state.bytesForwarded;
}

bool AudioCacheManager::ensureProxyHeadersSent(int upstreamStatus,
                                               const QMap<QString, QString>& headers,
                                               ProxyStreamState* state) {
    if (!state) {
        return false;
    }

    if (state->headerSent && state->upstreamHeadersValidated) {
        return !state->socket.isNull() &&
               state->socket->state() != QAbstractSocket::UnconnectedState;
    }

    if (state->socket.isNull() || state->socket->state() == QAbstractSocket::UnconnectedState) {
        return false;
    }

    const bool clientRequestedRange = state->hasRange && state->rangeStart >= 0;
    const int defaultStatus = clientRequestedRange ? 206 : 200;
    const int statusCode = upstreamStatus > 0 ? upstreamStatus : defaultStatus;
    const bool upstreamHttpError = statusCode == 408 || statusCode == 429 || statusCode >= 500;
    if (upstreamHttpError) {
        return false;
    }
    if (state->upstreamStartByte > 0 && statusCode == 200) {
        qWarning() << "[AudioCacheProxy] Upstream ignored non-zero Range:"
                   << "requested=" << state->upstreamRangeHeaderRaw
                   << "src=" << state->sourceUrl.toString();
        return false;
    }

    const QByteArray outboundReason =
        clientRequestedRange ? QByteArray("Partial Content") : QByteArray("OK");

    QByteArray contentType = headerValueBytes(headers, QStringLiteral("Content-Type"));
    if (contentType.isEmpty()) {
        contentType = audioContentTypeForUrl(state->sourceUrl);
    }

    qint64 contentLength = contentLengthFromHeaders(headers);
    if (state->expectedUpstreamBytes <= 0 && contentLength > 0) {
        state->expectedUpstreamBytes = contentLength;
    }
    QByteArray extraHeaders;
    const QByteArray contentRange = headerValueBytes(headers, QStringLiteral("Content-Range"));
    if (!contentRange.isEmpty()) {
        const qint64 responseStart = parseContentRangeStart(contentRange);
        if (responseStart >= 0 && responseStart != state->upstreamStartByte) {
            qWarning() << "[AudioCacheProxy] Upstream Content-Range start mismatch:"
                       << "expected=" << state->upstreamStartByte << "actual=" << responseStart
                       << "range=" << contentRange;
            return false;
        }
        if (clientRequestedRange) {
            if (state->seekHintTakeover && state->proxyOriginalRangeStart >= 0) {
                const qint64 parsedTotal = parseContentRangeLength(contentRange);
                qint64 virtualEnd = state->rangeEnd;
                if (virtualEnd < state->proxyOriginalRangeStart && contentLength > 0) {
                    virtualEnd = state->proxyOriginalRangeStart + contentLength - 1;
                }
                if (parsedTotal > 0 && virtualEnd >= parsedTotal) {
                    virtualEnd = parsedTotal - 1;
                }
                if (virtualEnd >= state->proxyOriginalRangeStart) {
                    extraHeaders += QByteArray("Content-Range: bytes ") +
                                    QByteArray::number(state->proxyOriginalRangeStart) + "-" +
                                    QByteArray::number(virtualEnd) + "/" +
                                    (parsedTotal > 0 ? QByteArray::number(parsedTotal)
                                                     : QByteArray("*")) +
                                    "\r\n";
                }
            } else {
                extraHeaders += "Content-Range: " + contentRange + "\r\n";
            }
        }
        const qint64 parsed = parseContentRangeLength(contentRange);
        if (parsed > 0) {
            TrackMeta meta;
            loadTrackMeta(state->cacheKey, &meta);
            meta.contentLength = parsed;
            meta.lastAccessMs = currentEpochMs();
            updateTrackMeta(state->cacheKey, meta.contentLength, meta.durationMs);
        }

        if (responseStart >= 0) {
            state->responseStartByte = responseStart;
            state->nextCacheByte = responseStart;
            state->nextMemoryByte = responseStart;
        }
    } else if (clientRequestedRange) {
        TrackMeta meta;
        loadTrackMeta(state->cacheKey, &meta);
        const qint64 knownTotal = qMax(meta.contentLength, meta.seekIndexFileSize);
        if (knownTotal > 0) {
            qint64 responseEnd = state->rangeEnd;
            if (responseEnd < state->rangeStart && contentLength > 0) {
                responseEnd = state->rangeStart + contentLength - 1;
            }
            if (responseEnd >= state->rangeStart) {
                responseEnd = qMin(responseEnd, knownTotal - 1);
                extraHeaders += QByteArray("Content-Range: bytes ") +
                                QByteArray::number(state->rangeStart) + "-" +
                                QByteArray::number(responseEnd) + "/" +
                                QByteArray::number(knownTotal) + "\r\n";
                contentLength = responseEnd - state->rangeStart + 1;
            }
        }
        state->responseStartByte = state->rangeStart;
        state->nextCacheByte = state->rangeStart;
        state->nextMemoryByte = state->rangeStart;
    } else {
        state->responseStartByte = 0;
        state->nextCacheByte = 0;
        state->nextMemoryByte = 0;
    }

    if (state->headerSent) {
        state->upstreamHeadersValidated = true;
        qDebug() << "[AudioCacheProxy] Resumed upstream headers validated:"
                 << "status=" << statusCode << "range=" << state->upstreamRangeHeaderRaw
                 << "contentRange=" << contentRange
                 << "retry=" << state->retryCount;
        return true;
    }

    qDebug() << "[AudioCacheProxy] Headers sent:"
             << "status=" << (clientRequestedRange ? 206 : 200)
             << "upstreamStatus=" << statusCode << "range=" << state->upstreamRangeHeaderRaw
             << "contentLength=" << contentLength
             << "contentRange=" << contentRange
             << "seekHintTakeover=" << state->seekHintTakeover
             << "seekTargetMs=" << state->seekTargetMs
             << "proxy_original_range_start=" << state->proxyOriginalRangeStart
             << "proxy_effective_range_start=" << state->proxyEffectiveRangeStart
             << "retry=" << state->retryCount;

    if (state->seekHintTakeover) {
        markSeekHintHeadersSent(state->cacheKey, state->seekHintGeneration);
    }

    state->socket->write(buildHttpHeaders(clientRequestedRange ? 206 : 200, outboundReason,
                                          contentType, contentLength, extraHeaders));
    state->headerSent = true;
    state->upstreamHeadersValidated = true;
    if (!state->pendingBody.isEmpty()) {
        const QByteArray pending = state->pendingBody;
        state->pendingBody.clear();
        forwardProxyChunk(state, pending);
    }
    return true;
}

bool AudioCacheManager::tryResumeProxyUpstream(ProxyStreamState* state, const QString& reason) {
    if (!state || state->socket.isNull() ||
        state->socket->state() == QAbstractSocket::UnconnectedState) {
        return false;
    }
    if (state->retryCount >= state->maxRetries) {
        return false;
    }

    qint64 resumeOffset = state->nextCacheByte;
    if (resumeOffset < 0) {
        resumeOffset = state->upstreamStartByte + state->upstreamBytesForwarded;
    }
    if (resumeOffset <= state->upstreamStartByte) {
        return false;
    }
    if (state->upstreamEndByte >= 0 && resumeOffset > state->upstreamEndByte) {
        return false;
    }

    ++state->retryCount;
    state->upstreamStartByte = resumeOffset;
    state->upstreamBytesForwarded = 0;
    state->upstreamHeadersValidated = false;
    state->pendingBody.clear();
    state->collectedForCache.clear();
    state->cacheWriteBuffer.clear();

    qWarning() << "[AudioCacheProxy] Resume upstream:"
               << "retry=" << state->retryCount << "/" << state->maxRetries
               << "offset=" << resumeOffset << "end=" << state->upstreamEndByte
               << "reason=" << reason << "src=" << state->sourceUrl.toString();
    startProxyUpstreamRequest(*state);
    return true;
}

bool AudioCacheManager::tryServeCachedRange(QTcpSocket* socket, const QString& cacheKey,
                                            const QByteArray& contentType, qint64 rangeStart,
                                            qint64 rangeEnd, qint64 knownTotal) {
    if (!socket || rangeStart < 0 || rangeEnd < rangeStart) {
        return false;
    }

    QByteArray payload;
    qint64 cursor = rangeStart;
    const qint64 firstChunkIndex = rangeStart / m_chunkSizeBytes;
    qint64 lastChunkIndex = rangeEnd / m_chunkSizeBytes;
    if (lastChunkIndex < firstChunkIndex) {
        lastChunkIndex = firstChunkIndex;
    }

    for (qint64 chunkIndex = firstChunkIndex; chunkIndex <= lastChunkIndex; ++chunkIndex) {
        QFileInfo chunkInfo(chunkFilePath(cacheKey, chunkIndex));
        if (!chunkInfo.exists() || !chunkInfo.isFile() || chunkInfo.size() <= 0) {
            break;
        }

        const qint64 chunkStart = chunkIndex * m_chunkSizeBytes;
        const qint64 expectedSize =
            (knownTotal > 0 && chunkStart + m_chunkSizeBytes >= knownTotal)
                ? knownTotal - chunkStart
                : m_chunkSizeBytes;
        if (expectedSize <= 0 || chunkInfo.size() != expectedSize) {
            break;
        }

        QFile chunkFile(chunkInfo.absoluteFilePath());
        if (!chunkFile.open(QIODevice::ReadOnly)) {
            break;
        }

        const qint64 offsetInChunk = qMax<qint64>(0, cursor - chunkStart);
        if (offsetInChunk >= chunkFile.size() || !chunkFile.seek(offsetInChunk)) {
            break;
        }

        const qint64 availableInChunk = chunkFile.size() - offsetInChunk;
        const qint64 requested = rangeEnd - cursor + 1;
        const qint64 toRead = qMin(availableInChunk, requested);
        if (toRead <= 0) {
            break;
        }

        const QByteArray data = chunkFile.read(toRead);
        if (data.isEmpty()) {
            break;
        }

        payload.append(data);
        cursor += data.size();
        if (cursor > rangeEnd || data.size() < toRead) {
            break;
        }
    }

    if (payload.isEmpty()) {
        return false;
    }

    const qint64 responseEnd = rangeStart + payload.size() - 1;
    QByteArray extra = QByteArray("Content-Range: bytes ") + QByteArray::number(rangeStart) + "-" +
                       QByteArray::number(responseEnd) + "/";
    extra += knownTotal > 0 ? QByteArray::number(knownTotal) : QByteArray("*");
    extra += "\r\n";

    socket->write(buildHttpHeaders(206, "Partial Content", contentType, payload.size(), extra));
    socket->write(payload);
    socket->disconnectFromHost();

    QFile(chunkFilePath(cacheKey, firstChunkIndex))
        .setFileTime(QDateTime::currentDateTimeUtc(), QFileDevice::FileAccessTime);
    qDebug() << "[AudioCacheProxy] Cached range hit:"
             << "range=" << QString("%1-%2").arg(rangeStart).arg(responseEnd)
             << "bytes=" << payload.size() << "knownTotal=" << knownTotal;
    return true;
}

bool AudioCacheManager::tryServeMemoryRange(QTcpSocket* socket, const QString& cacheKey,
                                            const QByteArray& contentType, qint64 rangeStart,
                                            qint64 rangeEnd, qint64 knownTotal) {
    if (!socket || cacheKey.isEmpty() || rangeStart < 0 || rangeEnd < rangeStart) {
        return false;
    }

    for (int i = m_memoryRangeWindows.size() - 1; i >= 0; --i) {
        MemoryRangeWindow& window = m_memoryRangeWindows[i];
        if (window.cacheKey != cacheKey || window.startByte < 0 || window.data.isEmpty()) {
            continue;
        }

        const qint64 windowEnd = window.startByte + window.data.size() - 1;
        if (rangeStart < window.startByte || rangeStart > windowEnd) {
            continue;
        }

        const qint64 offset = rangeStart - window.startByte;
        const qint64 available = window.data.size() - offset;
        const qint64 requested = rangeEnd - rangeStart + 1;
        const qint64 toSend = qMin(available, requested);
        if (toSend <= 0) {
            continue;
        }

        const QByteArray payload =
            window.data.mid(static_cast<int>(offset), static_cast<int>(toSend));
        const qint64 responseEnd = rangeStart + payload.size() - 1;
        QByteArray extra = QByteArray("Content-Range: bytes ") + QByteArray::number(rangeStart) +
                           "-" + QByteArray::number(responseEnd) + "/";
        extra += knownTotal > 0 ? QByteArray::number(knownTotal) : QByteArray("*");
        extra += "\r\n";

        socket->write(
            buildHttpHeaders(206, "Partial Content", contentType, payload.size(), extra));
        socket->write(payload);
        socket->disconnectFromHost();

        if (i != m_memoryRangeWindows.size() - 1) {
            const MemoryRangeWindow hit = window;
            m_memoryRangeWindows.removeAt(i);
            m_memoryRangeWindows.push_back(hit);
        }
        qDebug() << "[AudioCacheProxy] Memory range hit:"
                 << "range=" << QString("%1-%2").arg(rangeStart).arg(responseEnd)
                 << "bytes=" << payload.size() << "knownTotal=" << knownTotal;
        return true;
    }

    return false;
}

void AudioCacheManager::appendMemoryRangeData(ProxyStreamState* state, const QByteArray& data) {
    if (!state || data.isEmpty() || state->cacheKey.isEmpty() || state->nextMemoryByte < 0) {
        return;
    }

    const qint64 dataStart = state->nextMemoryByte;
    MemoryRangeWindow* target = nullptr;
    for (MemoryRangeWindow& window : m_memoryRangeWindows) {
        if (window.cacheKey != state->cacheKey) {
            continue;
        }
        const qint64 windowEnd = window.startByte + window.data.size();
        if (windowEnd == dataStart) {
            target = &window;
            break;
        }
    }

    if (!target) {
        MemoryRangeWindow window;
        window.cacheKey = state->cacheKey;
        window.startByte = dataStart;
        m_memoryRangeWindows.push_back(window);
        target = &m_memoryRangeWindows.last();
    }

    target->data.append(data);
    state->nextMemoryByte += data.size();
    if (target->data.size() > m_memoryWindowMaxBytes) {
        const qint64 trimBytes = target->data.size() - m_memoryWindowMaxBytes;
        target->data.remove(0, static_cast<int>(trimBytes));
        target->startByte += trimBytes;
    }

    while (m_memoryRangeWindows.size() > m_memoryWindowMaxCount) {
        m_memoryRangeWindows.removeFirst();
    }
}

void AudioCacheManager::appendProxyCacheData(ProxyStreamState* state, const QByteArray& data) {
    if (!state || data.isEmpty() || state->nextCacheByte < 0 || state->cacheKey.isEmpty()) {
        return;
    }

    qint64 dataOffset = 0;
    while (dataOffset < data.size()) {
        const qint64 chunkIndex = state->nextCacheByte / m_chunkSizeBytes;
        const qint64 offsetInChunk = state->nextCacheByte % m_chunkSizeBytes;

        if (offsetInChunk == 0 && state->cacheWriteBuffer.isEmpty() &&
            hasChunk(state->cacheKey, chunkIndex)) {
            const qint64 skip = qMin<qint64>(m_chunkSizeBytes, data.size() - dataOffset);
            state->nextCacheByte += skip;
            dataOffset += skip;
            continue;
        }

        if (offsetInChunk != state->cacheWriteBuffer.size()) {
            state->cacheWriteBuffer.clear();
            const qint64 skip = qMin<qint64>(m_chunkSizeBytes - offsetInChunk,
                                             data.size() - dataOffset);
            state->nextCacheByte += skip;
            dataOffset += skip;
            continue;
        }

        const qint64 writable = qMin<qint64>(m_chunkSizeBytes - offsetInChunk,
                                            data.size() - dataOffset);
        state->cacheWriteBuffer.append(data.constData() + dataOffset,
                                       static_cast<int>(writable));
        state->nextCacheByte += writable;
        dataOffset += writable;

        if (state->cacheWriteBuffer.size() == m_chunkSizeBytes) {
            persistProxyCacheChunk(state->cacheKey, chunkIndex, state->cacheWriteBuffer);
            state->cacheWriteBuffer.clear();
        }
    }
}

void AudioCacheManager::persistAlignedProxyChunk(const ProxyStreamState& state) {
    if (!state.hasRange || state.rangeStart < 0 || state.rangeEnd < state.rangeStart) {
        return;
    }
    if (state.rangeStart % m_chunkSizeBytes != 0) {
        return;
    }

    const qint64 expected = state.rangeEnd - state.rangeStart + 1;
    if (expected != state.collectedForCache.size() || expected > m_chunkSizeBytes) {
        return;
    }

    const qint64 chunkIndex = state.rangeStart / m_chunkSizeBytes;
    QDir().mkpath(trackCacheDir(state.cacheKey));
    const QString finalPath = chunkFilePath(state.cacheKey, chunkIndex);
    const QString partPath = finalPath + ".part";
    QFile file(partPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write(state.collectedForCache);
    file.close();
    QFile::remove(finalPath);
    if (QFile::rename(partPath, finalPath)) {
        qDebug() << "[AudioCacheProxy] Persisted aligned chunk from passthrough:" << chunkIndex;
    } else {
        QFile::remove(partPath);
    }
}

void AudioCacheManager::persistProxyCacheChunk(const QString& cacheKey, qint64 chunkIndex,
                                               const QByteArray& payload) {
    if (cacheKey.isEmpty() || chunkIndex < 0 || payload.size() != m_chunkSizeBytes) {
        return;
    }

    QDir().mkpath(trackCacheDir(cacheKey));
    const QString finalPath = chunkFilePath(cacheKey, chunkIndex);
    const QString partPath = finalPath + QStringLiteral(".part");

    QFile file(partPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    const qint64 written = file.write(payload);
    file.close();
    if (written != payload.size()) {
        QFile::remove(partPath);
        return;
    }

    QFile::remove(finalPath);
    if (QFile::rename(partPath, finalPath)) {
        QFile(finalPath).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                        QFileDevice::ReadUser | QFileDevice::WriteUser);
        qDebug() << "[AudioCacheProxy] Persisted streamed chunk:"
                 << "chunk=" << chunkIndex << "bytes=" << payload.size();
    } else {
        QFile::remove(partPath);
    }
}

void AudioCacheManager::updateTrackMetaFromProxyHeaders(
    const QString& cacheKey, const QMap<QString, QString>& headers) {
    if (cacheKey.isEmpty()) {
        return;
    }

    qint64 contentLength = contentLengthFromHeaders(headers);
    const qint64 parsedLength =
        parseContentRangeLength(headerValue(headers, QStringLiteral("Content-Range")));
    if (parsedLength > 0) {
        contentLength = parsedLength;
    }
    if (contentLength <= 0) {
        return;
    }

    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    meta.contentLength = contentLength;
    meta.lastAccessMs = currentEpochMs();
    updateTrackMeta(cacheKey, meta.contentLength, meta.durationMs);
}

bool AudioCacheManager::parseHttpRange(const QByteArray& rangeHeader, qint64* start,
                                       qint64* end) const {
    if (!start || !end) {
        return false;
    }

    static const QRegularExpression re(QStringLiteral(R"(bytes=(\d+)-(\d*) )").trimmed(),
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(QString::fromUtf8(rangeHeader).trimmed());
    if (!match.hasMatch()) {
        // Compatible parse: bytes=123-456 or bytes=123-
        const QString text = QString::fromUtf8(rangeHeader).trimmed();
        if (!text.startsWith(QStringLiteral("bytes="), Qt::CaseInsensitive)) {
            return false;
        }
        const QString value = text.mid(QStringLiteral("bytes=").size());
        const int dash = value.indexOf('-');
        if (dash <= 0) {
            return false;
        }
        bool okStart = false;
        *start = value.left(dash).toLongLong(&okStart);
        if (!okStart) {
            return false;
        }
        const QString endText = value.mid(dash + 1).trimmed();
        if (endText.isEmpty()) {
            *end = -1;
            return true;
        }
        bool okEnd = false;
        *end = endText.toLongLong(&okEnd);
        return okEnd;
    }

    bool okStart = false;
    *start = match.captured(1).toLongLong(&okStart);
    if (!okStart) {
        return false;
    }
    const QString endText = match.captured(2).trimmed();
    if (endText.isEmpty()) {
        *end = -1;
        return true;
    }
    bool okEnd = false;
    *end = endText.toLongLong(&okEnd);
    return okEnd;
}

void AudioCacheManager::sendProxyError(QTcpSocket* socket, int statusCode,
                                       const QByteArray& reason) const {
    if (!socket) {
        return;
    }

    const QByteArray body = reason + "\n";
    const QByteArray header = buildHttpHeaders(statusCode, reason, "text/plain", body.size());
    socket->write(header);
    socket->write(body);
    socket->disconnectFromHost();
}

QByteArray AudioCacheManager::buildHttpHeaders(int statusCode, const QByteArray& reason,
                                               const QByteArray& contentType, qint64 contentLength,
                                               const QByteArray& extra) const {
    QByteArray headers;
    headers += "HTTP/1.1 " + QByteArray::number(statusCode) + " " + reason + "\r\n";
    headers += "Connection: close\r\n";
    headers += "Accept-Ranges: bytes\r\n";
    headers += "Content-Type: " + contentType + "\r\n";
    if (contentLength >= 0) {
        headers += "Content-Length: " + QByteArray::number(contentLength) + "\r\n";
    }
    if (!extra.isEmpty()) {
        headers += extra;
    }
    headers += "\r\n";
    return headers;
}

bool AudioCacheManager::canCacheUrl(const QUrl& url) const {
    if (!isRemoteHttpUrl(url)) {
        return false;
    }

    const QString pathLower = url.path().toLower();
    if (!hasSupportedAudioExtension(pathLower)) {
        return false;
    }

    if (pathLower.endsWith(".m3u8")) {
        return false;
    }

    return true;
}

bool AudioCacheManager::canUseSeekIndexForUrl(const QUrl& url) const {
    if (!isRemoteHttpUrl(url)) {
        return false;
    }

    const QString pathLower = url.path().toLower();
    if (!pathLower.startsWith(QStringLiteral("/uploads/"))) {
        return false;
    }
    if (!hasSeekIndexSupportedAudioExtension(pathLower)) {
        return false;
    }

    return !musicPathForSeekIndexUrl(url).isEmpty();
}

QString AudioCacheManager::makeCacheKey(const QUrl& url) const {
    const QByteArray raw = url.toString(QUrl::FullyEncoded).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex());
}

QString AudioCacheManager::ensureCacheDirectory(const QString& requestedPath) const {
    QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(requestedPath.trimmed()));
    if (normalized.isEmpty()) {
        return QString();
    }

    QDir dir;
    if (!dir.mkpath(normalized)) {
        qWarning() << "[AudioCache] Failed to create cache directory:" << normalized;
        return QString();
    }
    return normalized;
}

QString AudioCacheManager::legacyFullCachePathForUrl(const QUrl& url) const {
    QString ext;
    const QFileInfo info(url.path());
    if (!info.suffix().isEmpty()) {
        ext = QStringLiteral(".") + info.suffix().toLower();
    } else {
        ext = QStringLiteral(".bin");
    }
    return QDir(m_cacheDirectory).filePath(makeCacheKey(url) + ext);
}

bool AudioCacheManager::hasCompleteFileCache(const QUrl& originalUrl) const {
    return canCacheUrl(originalUrl) && hasValidFullCache(originalUrl);
}

bool AudioCacheManager::hasValidFullCache(const QUrl& url, qint64 expectedSize) const {
    const QFileInfo info(legacyFullCachePathForUrl(url));
    if (!info.exists() || !info.isFile() || info.size() <= 0) {
        return false;
    }
    return expectedSize <= 0 || info.size() == expectedSize;
}

QString AudioCacheManager::trackCacheDir(const QString& cacheKey) const {
    return QDir(m_chunkRootDirectory).filePath(cacheKey);
}

QString AudioCacheManager::chunkFilePath(const QString& cacheKey, qint64 chunkIndex) const {
    return QDir(trackCacheDir(cacheKey))
        .filePath(QString::number(chunkIndex) + QStringLiteral(".bin"));
}

bool AudioCacheManager::hasChunk(const QString& cacheKey, qint64 chunkIndex) const {
    const QFileInfo info(chunkFilePath(cacheKey, chunkIndex));
    if (!info.exists() || !info.isFile() || info.size() <= 0) {
        return false;
    }

    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    const qint64 chunkStart = chunkIndex * m_chunkSizeBytes;
    const qint64 expectedSize =
        (meta.contentLength > 0 && chunkStart + m_chunkSizeBytes >= meta.contentLength)
            ? meta.contentLength - chunkStart
            : m_chunkSizeBytes;
    return expectedSize > 0 && info.size() == expectedSize;
}

void AudioCacheManager::queueFullFileDownload(const QUrl& url, const QString& cacheKey,
                                              qint64 contentLength,
                                              CacheRequestPriority priority) {
    Q_UNUSED(priority);
    if (!shouldFullCacheByAudioType(url) || contentLength <= 0) {
        return;
    }

    if (hasValidFullCache(url, contentLength)) {
        QFile(legacyFullCachePathForUrl(url))
            .setFileTime(QDateTime::currentDateTimeUtc(), QFileDevice::FileAccessTime);
        qDebug() << "[AudioCache] Full cache already complete:"
                 << "bytes=" << contentLength << "url=" << url.toString();
        return;
    }

    const QString inflightKey = cacheKey + ":full";
    if (m_inflightChunkKeys.contains(inflightKey)) {
        return;
    }

    QDir().mkpath(m_cacheDirectory);
    CurlNetworkTransport::Request request;
    request.url = url.toString(QUrl::FullyEncoded);
    request.method = QStringLiteral("GET");
    request.userAgent = QStringLiteral("FFmpegMusicPlayer/1.0");
    request.timeoutMs = 120000;

    const quint64 requestId = nextRequestId();
    const DownloadTask task{cacheKey, url, 0, contentLength - 1, -1,
                            legacyFullCachePathForUrl(url), false, true};
    m_inflightChunkKeys.insert(inflightKey);
    m_downloadTasks.insert(requestId, task);
    launchDownloadTask(requestId, task, request);

    qDebug() << "[AudioCache] Full cache queued:"
             << "bytes=" << contentLength << "url=" << url.toString();
}

void AudioCacheManager::queueChunkDownload(const QUrl& url, const QString& cacheKey,
                                           qint64 chunkIndex, qint64 startByte, qint64 endByte,
                                           CacheRequestPriority priority) {
    Q_UNUSED(priority);
    if (chunkIndex < 0 || endByte < startByte) {
        return;
    }

    QDir().mkpath(trackCacheDir(cacheKey));
    const QString targetFile = chunkFilePath(cacheKey, chunkIndex);
    if (hasChunk(cacheKey, chunkIndex)) {
        QFile(targetFile).setFileTime(QDateTime::currentDateTimeUtc(), QFileDevice::FileAccessTime);
        return;
    }

    const QString inflightKey = cacheKey + ":" + QString::number(chunkIndex);
    if (m_inflightChunkKeys.contains(inflightKey)) {
        return;
    }

    CurlNetworkTransport::Request request;
    request.url = url.toString(QUrl::FullyEncoded);
    request.method = QStringLiteral("GET");
    request.userAgent = QStringLiteral("FFmpegMusicPlayer/1.0");
    request.timeoutMs = 30000;
    request.headers.insert(QStringLiteral("Range"),
                           QStringLiteral("bytes=%1-%2").arg(startByte).arg(endByte));

    const quint64 requestId = nextRequestId();
    const DownloadTask task{cacheKey, url, startByte, endByte, chunkIndex, targetFile, false};
    m_inflightChunkKeys.insert(inflightKey);
    m_downloadTasks.insert(requestId, task);
    launchDownloadTask(requestId, task, request);

    qDebug() << "[AudioCache] Queue range:"
             << "chunk=" << chunkIndex << "range=" << QString("%1-%2").arg(startByte).arg(endByte)
             << "url=" << url.toString();
}

void AudioCacheManager::ensureTrackMeta(const QUrl& url, const QString& cacheKey) {
    TrackMeta meta;
    if (loadTrackMeta(cacheKey, &meta) && meta.contentLength > 0) {
        return;
    }

    // Initialize empty metadata and fire a tiny probe range to get content-length.
    if (meta.contentLength <= 0) {
        meta.lastAccessMs = currentEpochMs();
        updateTrackMeta(cacheKey, meta.contentLength, meta.durationMs);
    }

    const QString inflightKey = cacheKey + ":probe";
    if (m_inflightChunkKeys.contains(inflightKey)) {
        return;
    }

    CurlNetworkTransport::Request request;
    request.url = url.toString(QUrl::FullyEncoded);
    request.method = QStringLiteral("GET");
    request.userAgent = QStringLiteral("FFmpegMusicPlayer/1.0");
    request.timeoutMs = 8000;
    request.headers.insert(QStringLiteral("Range"), QStringLiteral("bytes=0-0"));

    const quint64 requestId = nextRequestId();
    const DownloadTask task{cacheKey, url, 0, 0, -1, QString(), true};
    m_inflightChunkKeys.insert(inflightKey);
    m_downloadTasks.insert(requestId, task);
    launchDownloadTask(requestId, task, request);
}

void AudioCacheManager::updateTrackMeta(const QString& cacheKey, qint64 contentLength,
                                        qint64 durationMs) {
    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    if (contentLength > 0) {
        meta.contentLength = contentLength;
    }
    if (durationMs > 0) {
        meta.durationMs = durationMs;
    }
    meta.lastAccessMs = currentEpochMs();
    {
        QMutexLocker locker(&m_metaMutex);
        m_trackMetaCache.insert(cacheKey, meta);
    }
    persistTrackMeta(cacheKey, meta);
}

bool AudioCacheManager::loadTrackMeta(const QString& cacheKey, TrackMeta* out) const {
    if (!out) {
        return false;
    }

    {
        QMutexLocker locker(&m_metaMutex);
        if (m_trackMetaCache.contains(cacheKey)) {
            *out = m_trackMetaCache.value(cacheKey);
            return true;
        }
    }

    const QString metaPath = trackMetaFilePath(cacheKey);
    QFile file(metaPath);
    if (!file.exists()) {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QJsonObject obj = doc.object();
    TrackMeta meta;
    meta.contentLength =
        static_cast<qint64>(obj.value(QStringLiteral("content_length")).toDouble(-1));
    meta.durationMs = static_cast<qint64>(obj.value(QStringLiteral("duration_ms")).toDouble(-1));
    meta.lastAccessMs =
        static_cast<qint64>(obj.value(QStringLiteral("last_access_ms")).toDouble(0));
    meta.seekIndexResolved = obj.value(QStringLiteral("seek_index_resolved")).toBool(false);
    meta.seekIndexSupported = obj.value(QStringLiteral("seek_index_supported")).toBool(false);
    meta.seekIndexFetchedAtMs =
        static_cast<qint64>(obj.value(QStringLiteral("seek_index_fetched_at_ms")).toDouble(0));
    meta.seekIndexFileSize =
        static_cast<qint64>(obj.value(QStringLiteral("seek_index_file_size")).toDouble(-1));
    meta.seekIndexVersion = obj.value(QStringLiteral("seek_index_version")).toString();
    meta.seekIndexMusicPath = obj.value(QStringLiteral("seek_index_music_path")).toString();
    meta.seekIndexGoodSamples =
        obj.value(QStringLiteral("seek_index_good_samples")).toInt(0);
    meta.seekIndexBadSamples = obj.value(QStringLiteral("seek_index_bad_samples")).toInt(0);
    meta.seekIndexGlobalBiasBytes =
        static_cast<qint64>(obj.value(QStringLiteral("seek_index_global_bias_bytes")).toDouble(0));
    meta.seekIndexGlobalBiasMs =
        static_cast<qint64>(obj.value(QStringLiteral("seek_index_global_bias_ms")).toDouble(0));
    meta.seekIndexCorrectionUpdatedAtMs = static_cast<qint64>(
        obj.value(QStringLiteral("seek_index_correction_updated_at_ms")).toDouble(0));
    const QJsonArray pointsArray = obj.value(QStringLiteral("seek_index_points")).toArray();
    meta.seekIndexPoints.reserve(pointsArray.size());
    for (const QJsonValue& value : pointsArray) {
        const QJsonObject pointObj = value.toObject();
        const qint64 timeMs =
            static_cast<qint64>(pointObj.value(QStringLiteral("time_ms")).toDouble(-1));
        const qint64 byteOffset =
            static_cast<qint64>(pointObj.value(QStringLiteral("byte_offset")).toDouble(-1));
        if (timeMs < 0 || byteOffset < 0) {
            continue;
        }
        meta.seekIndexPoints.push_back(TrackMeta::SeekIndexPoint{timeMs, byteOffset});
    }
    const QJsonArray localPointsArray =
        obj.value(QStringLiteral("local_seek_points")).toArray();
    meta.localSeekPoints.reserve(localPointsArray.size());
    for (const QJsonValue& value : localPointsArray) {
        const QJsonObject pointObj = value.toObject();
        const qint64 timeMs =
            static_cast<qint64>(pointObj.value(QStringLiteral("time_ms")).toDouble(-1));
        const qint64 byteOffset =
            static_cast<qint64>(pointObj.value(QStringLiteral("byte_offset")).toDouble(-1));
        if (timeMs < 0 || byteOffset < 0) {
            continue;
        }
        meta.localSeekPoints.push_back(TrackMeta::SeekIndexPoint{timeMs, byteOffset});
    }

    {
        QMutexLocker locker(&m_metaMutex);
        const_cast<AudioCacheManager*>(this)->m_trackMetaCache.insert(cacheKey, meta);
    }
    *out = meta;
    return true;
}

void AudioCacheManager::persistTrackMeta(const QString& cacheKey, const TrackMeta& meta) const {
    QDir().mkpath(trackCacheDir(cacheKey));
    const QString metaPath = trackMetaFilePath(cacheKey);
    QFile file(metaPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("content_length"), static_cast<double>(meta.contentLength));
    obj.insert(QStringLiteral("duration_ms"), static_cast<double>(meta.durationMs));
    obj.insert(QStringLiteral("last_access_ms"), static_cast<double>(meta.lastAccessMs));
    obj.insert(QStringLiteral("seek_index_resolved"), meta.seekIndexResolved);
    obj.insert(QStringLiteral("seek_index_supported"), meta.seekIndexSupported);
    obj.insert(QStringLiteral("seek_index_fetched_at_ms"),
               static_cast<double>(meta.seekIndexFetchedAtMs));
    obj.insert(QStringLiteral("seek_index_file_size"), static_cast<double>(meta.seekIndexFileSize));
    obj.insert(QStringLiteral("seek_index_version"), meta.seekIndexVersion);
    obj.insert(QStringLiteral("seek_index_music_path"), meta.seekIndexMusicPath);
    obj.insert(QStringLiteral("seek_index_good_samples"), meta.seekIndexGoodSamples);
    obj.insert(QStringLiteral("seek_index_bad_samples"), meta.seekIndexBadSamples);
    obj.insert(QStringLiteral("seek_index_global_bias_bytes"),
               static_cast<double>(meta.seekIndexGlobalBiasBytes));
    obj.insert(QStringLiteral("seek_index_global_bias_ms"),
               static_cast<double>(meta.seekIndexGlobalBiasMs));
    obj.insert(QStringLiteral("seek_index_correction_updated_at_ms"),
               static_cast<double>(meta.seekIndexCorrectionUpdatedAtMs));

    QJsonArray pointsArray;
    for (const TrackMeta::SeekIndexPoint& point : meta.seekIndexPoints) {
        QJsonObject pointObj;
        pointObj.insert(QStringLiteral("time_ms"), static_cast<double>(point.timeMs));
        pointObj.insert(QStringLiteral("byte_offset"), static_cast<double>(point.byteOffset));
        pointsArray.append(pointObj);
    }
    obj.insert(QStringLiteral("seek_index_points"), pointsArray);

    QJsonArray localPointsArray;
    for (const TrackMeta::SeekIndexPoint& point : meta.localSeekPoints) {
        QJsonObject pointObj;
        pointObj.insert(QStringLiteral("time_ms"), static_cast<double>(point.timeMs));
        pointObj.insert(QStringLiteral("byte_offset"), static_cast<double>(point.byteOffset));
        localPointsArray.append(pointObj);
    }
    obj.insert(QStringLiteral("local_seek_points"), localPointsArray);

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    file.close();
}

QString AudioCacheManager::trackMetaFilePath(const QString& cacheKey) const {
    return QDir(trackCacheDir(cacheKey)).filePath(QStringLiteral("meta.json"));
}

qint64 AudioCacheManager::currentEpochMs() const {
    return QDateTime::currentMSecsSinceEpoch();
}

void AudioCacheManager::maybeRunLruCleanup() {
    const qint64 now = currentEpochMs();
    if (now - m_lruLastRunMs < 10000) {
        return;
    }
    m_lruLastRunMs = now;

    const qint64 total = totalChunkBytes();
    if (total <= m_maxCacheBytes) {
        return;
    }

    QDir root(m_chunkRootDirectory);
    if (!root.exists()) {
        return;
    }

    QFileInfoList files;
    const QFileInfoList trackDirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& dirInfo : trackDirs) {
        QDir d(dirInfo.absoluteFilePath());
        const QFileInfoList chunkFiles =
            d.entryInfoList(QStringList() << "*.bin", QDir::Files, QDir::Time);
        for (const QFileInfo& fi : chunkFiles) {
            files.push_back(fi);
        }
    }

    std::sort(files.begin(), files.end(), [](const QFileInfo& a, const QFileInfo& b) {
        return a.lastModified() < b.lastModified();
    });

    qint64 currentTotal = total;
    const qint64 target = (m_maxCacheBytes * 8) / 10;
    for (const QFileInfo& fi : files) {
        if (currentTotal <= target) {
            break;
        }
        const qint64 size = fi.size();
        QFile::remove(fi.absoluteFilePath());
        currentTotal -= size;
    }

    qDebug() << "[AudioCache] LRU cleanup:"
             << "before=" << total << "after=" << currentTotal << "limit=" << m_maxCacheBytes;
}

qint64 AudioCacheManager::totalChunkBytes() const {
    QDir root(m_chunkRootDirectory);
    if (!root.exists()) {
        return 0;
    }

    qint64 total = 0;
    const QFileInfoList trackDirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& dirInfo : trackDirs) {
        QDir d(dirInfo.absoluteFilePath());
        const QFileInfoList chunkFiles = d.entryInfoList(QStringList() << "*.bin", QDir::Files);
        for (const QFileInfo& fi : chunkFiles) {
            total += fi.size();
        }
    }
    return total;
}
