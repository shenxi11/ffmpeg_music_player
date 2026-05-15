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
#include <QMutexLocker>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QVector>
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
    return pathLower.endsWith(".mp3") || pathLower.endsWith(".m4a") || pathLower.endsWith(".aac");
}

bool shouldFullCacheByAudioType(const QUrl& url) {
    const QString pathLower = url.path().toLower();
    return pathLower.endsWith(".mp3") || pathLower.endsWith(".ogg") || pathLower.endsWith(".wav");
}

bool isFlacUrl(const QUrl& url) {
    return url.path().toLower().endsWith(".flac");
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

bool isLikelyAudioResponse(const QNetworkReply* reply) {
    if (!reply) {
        return false;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 0 && statusCode != 200 && statusCode != 206) {
        return false;
    }

    QByteArray contentType =
        reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().toLower().trimmed();
    const int semicolonPos = contentType.indexOf(';');
    if (semicolonPos >= 0) {
        contentType = contentType.left(semicolonPos).trimmed();
    }

    return contentType.isEmpty() || contentType.startsWith("audio/") ||
           contentType == "application/octet-stream";
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

    connect(&m_networkManager, &QNetworkAccessManager::finished, this,
            &AudioCacheManager::onDownloadFinished);
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
    const QList<QNetworkReply*> activeReplies = m_downloadTasks.keys();
    for (QNetworkReply* reply : activeReplies) {
        if (!reply) {
            continue;
        }
        reply->abort();
        reply->deleteLater();
    }
    m_downloadTasks.clear();
    m_inflightChunkKeys.clear();
    m_proxyReadBuffers.clear();
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

    const QString cacheKey = makeCacheKey(originalUrl);
    ensureTrackMeta(originalUrl, cacheKey);

    TrackMeta meta;
    loadTrackMeta(cacheKey, &meta);
    meta.lastAccessMs = currentEpochMs();
    updateTrackMeta(cacheKey, meta.contentLength, meta.durationMs);

    if (shouldFullCacheByAudioType(originalUrl) && meta.contentLength > 0) {
        queueFullFileDownload(originalUrl, cacheKey, meta.contentLength,
                              QNetworkRequest::HighPriority);
        return;
    }
    if (shouldFullCacheByAudioType(originalUrl)) {
        qDebug() << "[AudioCache] Full cache waiting-size:"
                 << "url=" << originalUrl.toString();
        return;
    }
    if (!shouldFullCacheByAudioType(originalUrl)) {
        qDebug() << "[AudioCache] Full cache skip-type:"
                 << "bytes=" << meta.contentLength << "url=" << originalUrl.toString();
    }

    int startupChunks = m_startupPrefetchChunks;
    if (isFlacUrl(originalUrl)) {
        // FLAC 启动预热略高于普通音频，但避免一次性请求过多抢占实时流。
        startupChunks = qMax(startupChunks, 6);
    }

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
                           QNetworkRequest::LowPriority);
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

    const qint64 indexedCenterByte = estimateCenterByteFromSeekIndex(meta, targetMs);
    const bool seekIndexHit = indexedCenterByte >= 0;

    if (meta.contentLength <= 0 || (!seekIndexHit && meta.durationMs <= 0) || targetMs < 0) {
        // Unknown total length/duration: fallback to startup prefetch.
        warmupCache(originalUrl);
        return;
    }

    qint64 centerByte = indexedCenterByte;
    if (!seekIndexHit) {
        const double ratio =
            qBound(0.0, static_cast<double>(targetMs) / static_cast<double>(meta.durationMs), 1.0);
        centerByte = static_cast<qint64>(ratio * static_cast<double>(meta.contentLength - 1));
    }
    if (centerByte < 0) {
        centerByte = 0;
    }
    const qint64 centerChunk = centerByte / m_chunkSizeBytes;
    const bool flac = isFlacUrl(originalUrl);
    const qint64 seekWindow = flac ? 4 : (seekIndexHit ? 1 : 2);

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
        QNetworkRequest::Priority priority = QNetworkRequest::LowPriority;
        if (offset == 0) {
            priority = QNetworkRequest::HighPriority;
        } else if (qAbs(offset) <= 1) {
            priority = QNetworkRequest::NormalPriority;
        }
        queueChunkDownload(originalUrl, cacheKey, chunkIndex, start, end, priority);
    }

    qDebug() << "[AudioCache] Seek prefetch queued:"
             << "seek_target_ms=" << targetMs << "seek_index_hit=" << seekIndexHit
             << "seek_prefetch_center_byte=" << centerByte << "targetMs=" << targetMs
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

    QNetworkRequest request(seekIndexUrl);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("FFmpegMusicPlayer/1.0"));
    request.setPriority(QNetworkRequest::HighPriority);

    QNetworkReply* reply = m_networkManager.get(request);
    m_seekIndexRequests.insert(reply, SeekIndexRequest{cacheKey, musicPath});
    connect(reply, &QNetworkReply::finished, this, &AudioCacheManager::onSeekIndexReplyFinished);

    qDebug() << "[AudioCache] Request seek index:"
             << "musicPath=" << musicPath << "url=" << seekIndexUrl.toString();
}

qint64 AudioCacheManager::estimateCenterByteFromSeekIndex(const TrackMeta& meta,
                                                          qint64 targetMs) const {
    if (!meta.seekIndexResolved || !meta.seekIndexSupported || meta.seekIndexPoints.isEmpty() ||
        targetMs < 0) {
        return -1;
    }

    const auto& points = meta.seekIndexPoints;
    qint64 centerByte = points.first().byteOffset;
    if (targetMs <= points.first().timeMs) {
        centerByte = points.first().byteOffset;
    } else if (targetMs >= points.last().timeMs) {
        centerByte = points.last().byteOffset;
    } else {
        for (int i = 1; i < points.size(); ++i) {
            const TrackMeta::SeekIndexPoint& previous = points.at(i - 1);
            const TrackMeta::SeekIndexPoint& next = points.at(i);
            if (targetMs > next.timeMs) {
                continue;
            }

            if (next.timeMs <= previous.timeMs || next.byteOffset <= previous.byteOffset) {
                centerByte = previous.byteOffset;
                break;
            }

            const double ratio = static_cast<double>(targetMs - previous.timeMs) /
                                 static_cast<double>(next.timeMs - previous.timeMs);
            centerByte = previous.byteOffset +
                         static_cast<qint64>(
                             ratio * static_cast<double>(next.byteOffset - previous.byteOffset));
            break;
        }
    }

    const qint64 fileSize = qMax(meta.seekIndexFileSize, meta.contentLength);
    if (fileSize <= 0) {
        return -1;
    }
    const qint64 maxByte = qMax<qint64>(0, fileSize - 1);
    return qBound<qint64>(0, centerByte, maxByte);
}

void AudioCacheManager::onSeekIndexReplyFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const auto it = m_seekIndexRequests.find(reply);
    if (it == m_seekIndexRequests.end()) {
        reply->deleteLater();
        return;
    }

    const SeekIndexRequest request = it.value();
    m_seekIndexRequests.erase(it);

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
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError && statusCode == 200) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
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

            meta.seekIndexResolved = true;
            meta.seekIndexSupported = supported && !points.isEmpty();
            meta.seekIndexMusicPath = resolvedMusicPath;
            meta.seekIndexFileSize = fileSize;
            meta.seekIndexVersion = version;
            meta.seekIndexPoints =
                meta.seekIndexSupported ? points : QVector<TrackMeta::SeekIndexPoint>{};
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
                   << "error=" << reply->errorString();
    }

    {
        QMutexLocker locker(&m_metaMutex);
        m_trackMetaCache.insert(request.cacheKey, meta);
    }
    persistTrackMeta(request.cacheKey, meta);
    reply->deleteLater();
}

void AudioCacheManager::onDownloadFinished(QNetworkReply* reply) {
    if (!reply) {
        return;
    }

    if (m_seekIndexRequests.contains(reply)) {
        return;
    }

    const auto it = m_downloadTasks.find(reply);
    if (it == m_downloadTasks.end()) {
        reply->deleteLater();
        return;
    }

    const DownloadTask task = it.value();
    m_downloadTasks.erase(it);
    if (task.fullFileDownload) {
        m_inflightChunkKeys.remove(task.cacheKey + ":full");
    } else {
        m_inflightChunkKeys.remove(task.cacheKey + ":" + QString::number(task.chunkIndex));
    }
    if (task.metadataProbe) {
        m_inflightChunkKeys.remove(task.cacheKey + ":probe");
    }

    const QByteArray payload = reply->readAll();
    const bool responseLooksAudio = isLikelyAudioResponse(reply);
    const bool ok = (reply->error() == QNetworkReply::NoError) && !payload.isEmpty() &&
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
                   << "chunk=" << task.chunkIndex << "url=" << reply->url().toString() << "status="
                   << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                   << "contentType="
                   << reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()
                   << "error=" << reply->errorString();
    }

    // Update metadata from response headers.
    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    const QString contentRange = reply->rawHeader("Content-Range");
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

    if ((task.metadataProbe || !task.fullFileDownload) &&
        shouldFullCacheByAudioType(task.originalUrl) && meta.contentLength > 0) {
        queueFullFileDownload(task.originalUrl, task.cacheKey, meta.contentLength,
                              QNetworkRequest::HighPriority);
    }

    maybeRunLruCleanup();
    reply->deleteLater();
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
                                     "audio/mpeg", payload.size(), extra));
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

    // Fast path: single-chunk hit can be returned directly without upstream fetch.
    // Supports unaligned byte-range to improve FLAC seek hit rate.
    if (hasRange && rangeStart >= 0) {
        qint64 normalizedRangeEnd = rangeEnd;
        if (normalizedRangeEnd < rangeStart) {
            normalizedRangeEnd = rangeStart + m_chunkSizeBytes - 1;
        }

        const qint64 chunkIndexStart = rangeStart / m_chunkSizeBytes;
        const qint64 chunkIndexEnd = normalizedRangeEnd / m_chunkSizeBytes;
        if (chunkIndexStart == chunkIndexEnd && hasChunk(cacheKey, chunkIndexStart)) {
            const qint64 chunkStart = chunkIndexStart * m_chunkSizeBytes;
            const qint64 offsetInChunk = rangeStart - chunkStart;
            QFile chunkFile(chunkFilePath(cacheKey, chunkIndexStart));
            if (chunkFile.open(QIODevice::ReadOnly)) {
                qint64 toRead = chunkFile.size() - offsetInChunk;
                QByteArray payload;
                if (toRead > 0) {
                    const qint64 requested = normalizedRangeEnd - rangeStart + 1;
                    if (requested > 0) {
                        toRead = qMin(toRead, requested);
                    }
                    if (offsetInChunk > 0 && offsetInChunk < chunkFile.size()) {
                        chunkFile.seek(offsetInChunk);
                    }
                    payload = chunkFile.read(toRead);
                }
                chunkFile.close();
                if (!payload.isEmpty()) {
                    const QByteArray extra =
                        QByteArray("Content-Range: bytes ") + QByteArray::number(rangeStart) + "-" +
                        QByteArray::number(rangeStart + payload.size() - 1) + "/*\r\n";
                    socket->write(buildHttpHeaders(206, "Partial Content", "audio/mpeg",
                                                   payload.size(), extra));
                    socket->write(payload);
                    socket->disconnectFromHost();
                    qDebug() << "[AudioCacheProxy] Chunk hit response, chunk:" << chunkIndexStart
                             << "offset:" << offsetInChunk << "bytes:" << payload.size();
                    return;
                }
            }
        }
    }

    qDebug() << "[AudioCacheProxy] Request:"
             << "src=" << srcUrl.toString() << "hasRange=" << hasRange
             << "rangeStart=" << rangeStart << "rangeEnd=" << rangeEnd;

    // Upstream async passthrough: non-blocking and stream-on-read.
    QNetworkRequest upstreamRequest(srcUrl);
    upstreamRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    upstreamRequest.setHeader(QNetworkRequest::UserAgentHeader,
                              QStringLiteral("FFmpegMusicPlayer/1.0"));
    upstreamRequest.setPriority(QNetworkRequest::HighPriority);
    if (hasRange && !rangeHeaderRaw.isEmpty()) {
        upstreamRequest.setRawHeader("Range", rangeHeaderRaw);
    }

    QNetworkReply* reply = m_networkManager.get(upstreamRequest);
    ProxyStreamState state;
    state.socket = socket;
    state.hasRange = hasRange;
    state.rangeStart = rangeStart;
    state.rangeEnd = rangeEnd;
    state.cacheKey = cacheKey;
    m_proxyStreamStates.insert(reply, state);

    const auto destroyedSignal = static_cast<void (QObject::*)(QObject*)>(&QObject::destroyed);
    connect(socket, destroyedSignal, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::readyRead, this, &AudioCacheManager::onProxyUpstreamReadyRead);
    connect(reply, &QNetworkReply::finished, this, &AudioCacheManager::onProxyUpstreamFinished);
}

void AudioCacheManager::onProxyUpstreamReadyRead() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    auto it = m_proxyStreamStates.find(reply);
    if (it == m_proxyStreamStates.end()) {
        return;
    }
    ProxyStreamState& state = it.value();

    if (!ensureProxyHeadersSent(reply, &state)) {
        return;
    }

    const QByteArray chunk = reply->readAll();
    if (chunk.isEmpty() || state.socket.isNull()) {
        return;
    }

    state.bytesForwarded += chunk.size();
    state.socket->write(chunk);

    if (state.hasRange && state.rangeStart >= 0 && state.rangeEnd >= state.rangeStart &&
        (state.rangeStart % m_chunkSizeBytes == 0) &&
        (state.rangeEnd - state.rangeStart + 1) <= m_chunkSizeBytes &&
        state.collectedForCache.size() < m_chunkSizeBytes) {
        state.collectedForCache.append(chunk);
    }
}

void AudioCacheManager::onProxyUpstreamFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    auto it = m_proxyStreamStates.find(reply);
    if (it == m_proxyStreamStates.end()) {
        reply->deleteLater();
        return;
    }
    ProxyStreamState state = it.value();
    m_proxyStreamStates.erase(it);

    if (state.socket.isNull() || state.socket->state() == QAbstractSocket::UnconnectedState) {
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[AudioCacheProxy] Upstream error:" << reply->errorString();
        if (!state.headerSent) {
            sendProxyError(state.socket, 502, "Bad Gateway");
        } else {
            state.socket->disconnectFromHost();
        }
        reply->deleteLater();
        return;
    }

    if (!ensureProxyHeadersSent(reply, &state)) {
        reply->deleteLater();
        return;
    }

    const QByteArray tail = reply->readAll();
    if (!tail.isEmpty()) {
        state.bytesForwarded += tail.size();
        state.socket->write(tail);
        if (state.hasRange && state.rangeStart >= 0 && state.rangeEnd >= state.rangeStart &&
            (state.rangeStart % m_chunkSizeBytes == 0) &&
            (state.rangeEnd - state.rangeStart + 1) <= m_chunkSizeBytes &&
            state.collectedForCache.size() < m_chunkSizeBytes) {
            state.collectedForCache.append(tail);
        }
    }

    state.socket->flush();
    state.socket->disconnectFromHost();

    persistAlignedProxyChunk(state);
    updateTrackMetaFromProxyReply(state.cacheKey, reply);
    maybeRunLruCleanup();

    qDebug() << "[AudioCacheProxy] Response finished, bytes:" << state.bytesForwarded;
    reply->deleteLater();
}

bool AudioCacheManager::ensureProxyHeadersSent(QNetworkReply* reply, ProxyStreamState* state) {
    if (!reply || !state) {
        return false;
    }

    if (state->headerSent) {
        return !state->socket.isNull() &&
               state->socket->state() != QAbstractSocket::UnconnectedState;
    }

    if (state->socket.isNull() || state->socket->state() == QAbstractSocket::UnconnectedState) {
        return false;
    }

    const int upstreamStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const int statusCode = upstreamStatus > 0 ? upstreamStatus : (state->hasRange ? 206 : 200);
    QByteArray reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray();
    if (reason.isEmpty()) {
        reason = (statusCode == 206) ? QByteArray("Partial Content") : QByteArray("OK");
    }

    QByteArray contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
    if (contentType.isEmpty()) {
        contentType = "audio/mpeg";
    }

    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    QByteArray extraHeaders;
    const QByteArray contentRange = reply->rawHeader("Content-Range");
    if (!contentRange.isEmpty()) {
        extraHeaders += "Content-Range: " + contentRange + "\r\n";
        const qint64 parsed = parseContentRangeLength(contentRange);
        if (parsed > 0) {
            TrackMeta meta;
            loadTrackMeta(state->cacheKey, &meta);
            meta.contentLength = parsed;
            meta.lastAccessMs = currentEpochMs();
            updateTrackMeta(state->cacheKey, meta.contentLength, meta.durationMs);
        }
    }

    state->socket->write(
        buildHttpHeaders(statusCode, reason, contentType, contentLength, extraHeaders));
    state->headerSent = true;
    return true;
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

void AudioCacheManager::updateTrackMetaFromProxyReply(const QString& cacheKey,
                                                      QNetworkReply* reply) {
    if (!reply) {
        return;
    }

    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    const qint64 parsedLength = parseContentRangeLength(reply->rawHeader("Content-Range"));
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
    return info.exists() && info.isFile() && info.size() > 0;
}

void AudioCacheManager::queueFullFileDownload(const QUrl& url, const QString& cacheKey,
                                              qint64 contentLength,
                                              QNetworkRequest::Priority priority) {
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
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("FFmpegMusicPlayer/1.0"));
    request.setPriority(priority);

    QNetworkReply* reply = m_networkManager.get(request);
    m_inflightChunkKeys.insert(inflightKey);
    m_downloadTasks.insert(reply, DownloadTask{cacheKey, url, 0, contentLength - 1, -1,
                                               legacyFullCachePathForUrl(url), false, true});

    qDebug() << "[AudioCache] Full cache queued:"
             << "bytes=" << contentLength << "url=" << url.toString();
}

void AudioCacheManager::queueChunkDownload(const QUrl& url, const QString& cacheKey,
                                           qint64 chunkIndex, qint64 startByte, qint64 endByte,
                                           QNetworkRequest::Priority priority) {
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

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("FFmpegMusicPlayer/1.0"));
    request.setPriority(priority);
    request.setRawHeader("Range", QByteArray("bytes=") + QByteArray::number(startByte) +
                                      QByteArray("-") + QByteArray::number(endByte));

    QNetworkReply* reply = m_networkManager.get(request);
    m_inflightChunkKeys.insert(inflightKey);
    m_downloadTasks.insert(
        reply, DownloadTask{cacheKey, url, startByte, endByte, chunkIndex, targetFile, false});

    qDebug() << "[AudioCache] Queue range:"
             << "chunk=" << chunkIndex << "range=" << QString("%1-%2").arg(startByte).arg(endByte)
             << "url=" << url.toString();
}

void AudioCacheManager::ensureTrackMeta(const QUrl& url, const QString& cacheKey) {
    TrackMeta meta;
    if (loadTrackMeta(cacheKey, &meta)) {
        return;
    }

    // Initialize empty metadata and fire a tiny probe range to get content-length.
    meta.lastAccessMs = currentEpochMs();
    updateTrackMeta(cacheKey, -1, -1);

    const QString inflightKey = cacheKey + ":probe";
    if (m_inflightChunkKeys.contains(inflightKey)) {
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("FFmpegMusicPlayer/1.0"));
    request.setPriority(QNetworkRequest::HighPriority);
    request.setRawHeader("Range", QByteArray("bytes=0-0"));

    QNetworkReply* reply = m_networkManager.get(request);
    m_inflightChunkKeys.insert(inflightKey);
    m_downloadTasks.insert(reply, DownloadTask{cacheKey, url, 0, 0, -1, QString(), true});
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

    QJsonArray pointsArray;
    for (const TrackMeta::SeekIndexPoint& point : meta.seekIndexPoints) {
        QJsonObject pointObj;
        pointObj.insert(QStringLiteral("time_ms"), static_cast<double>(point.timeMs));
        pointObj.insert(QStringLiteral("byte_offset"), static_cast<double>(point.byteOffset));
        pointsArray.append(pointObj);
    }
    obj.insert(QStringLiteral("seek_index_points"), pointsArray);

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
