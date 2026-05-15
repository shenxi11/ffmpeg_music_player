#ifndef AUDIO_CACHE_MANAGER_H
#define AUDIO_CACHE_MANAGER_H

#include <QObject>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QUrl>
#include <QMutex>
#include <QPointer>
#include <QVector>

class QTcpServer;
class QTcpSocket;

class AudioCacheManager : public QObject
{
    Q_OBJECT
public:
    static AudioCacheManager& instance();

    void setCacheDirectory(const QString& directoryPath);
    QString cacheDirectory() const;
    bool clearCache(QString* errorMessage = nullptr,
                    qint64* removedBytes = nullptr,
                    qint64* removedFiles = nullptr);

    // Keep compatibility with existing service call path.
    // In segmented cache mode this may return localhost proxy URL.
    QUrl resolvePlaybackUrl(const QUrl& originalUrl) const;

    // Prefetch startup chunks for this URL.
    void warmupCache(const QUrl& originalUrl);

    // Prefetch chunks around a seek target to reduce first-hit delay.
    void prefetchForSeek(const QUrl& originalUrl, qint64 targetMs, qint64 durationMs);

    bool hasCompleteFileCache(const QUrl& originalUrl) const;

    // Save known duration to improve byte-range estimation.
    void noteTrackDuration(const QUrl& originalUrl, qint64 durationMs);

    bool hasResolvedSeekIndexForPlaybackUrl(const QUrl& playbackUrl) const;

signals:
    void fullFileCacheReady(const QUrl& originalUrl, const QUrl& localFileUrl);

private slots:
    void onDownloadFinished(QNetworkReply* reply);
    void onSeekIndexReplyFinished();
    void onProxySocketReadyRead();
    void onProxySocketDisconnected();
    void onProxySocketDestroyed(QObject* obj);
    void onProxyUpstreamReadyRead();
    void onProxyUpstreamFinished();

private:
    struct DownloadTask {
        QString cacheKey;
        QUrl originalUrl;
        qint64 startByte = 0;
        qint64 endByte = 0;
        qint64 chunkIndex = -1;
        QString targetFilePath;
        bool metadataProbe = false;
        bool fullFileDownload = false;
    };

    struct TrackMeta {
        qint64 contentLength = -1;
        qint64 durationMs = -1;
        qint64 lastAccessMs = 0;
        bool seekIndexResolved = false;
        bool seekIndexSupported = false;
        qint64 seekIndexFetchedAtMs = 0;
        qint64 seekIndexFileSize = -1;
        QString seekIndexVersion;
        QString seekIndexMusicPath;
        struct SeekIndexPoint {
            qint64 timeMs = 0;
            qint64 byteOffset = 0;
        };
        QVector<SeekIndexPoint> seekIndexPoints;
    };

    struct SeekIndexRequest {
        QString cacheKey;
        QString musicPath;
    };

    struct ProxyStreamState {
        QPointer<QTcpSocket> socket;
        bool headerSent = false;
        qint64 bytesForwarded = 0;
        QByteArray collectedForCache;
        bool hasRange = false;
        qint64 rangeStart = -1;
        qint64 rangeEnd = -1;
        QString cacheKey;
    };

    AudioCacheManager(QObject* parent = nullptr);
    AudioCacheManager(const AudioCacheManager&) = delete;
    AudioCacheManager& operator=(const AudioCacheManager&) = delete;

    bool canCacheUrl(const QUrl& url) const;
    bool canUseSeekIndexForUrl(const QUrl& url) const;
    QString makeCacheKey(const QUrl& url) const;
    QString ensureCacheDirectory(const QString& requestedPath) const;
    QString musicPathForSeekIndexUrl(const QUrl& url) const;
    bool shouldRefreshSeekIndex(const TrackMeta& meta) const;
    void ensureSeekIndex(const QUrl& originalUrl, const QString& cacheKey, qint64 knownDurationMs);
    qint64 estimateCenterByteFromSeekIndex(const TrackMeta& meta, qint64 targetMs) const;

    QString legacyFullCachePathForUrl(const QUrl& url) const;
    bool hasValidFullCache(const QUrl& url, qint64 expectedSize = -1) const;

    QString trackCacheDir(const QString& cacheKey) const;
    QString chunkFilePath(const QString& cacheKey, qint64 chunkIndex) const;
    bool hasChunk(const QString& cacheKey, qint64 chunkIndex) const;
    void queueFullFileDownload(const QUrl& url,
                               const QString& cacheKey,
                               qint64 contentLength,
                               QNetworkRequest::Priority priority = QNetworkRequest::LowPriority);
    void queueChunkDownload(const QUrl& url,
                            const QString& cacheKey,
                            qint64 chunkIndex,
                            qint64 startByte,
                            qint64 endByte,
                            QNetworkRequest::Priority priority = QNetworkRequest::LowPriority);

    void ensureTrackMeta(const QUrl& url, const QString& cacheKey);
    void updateTrackMeta(const QString& cacheKey, qint64 contentLength, qint64 durationMs);
    bool loadTrackMeta(const QString& cacheKey, TrackMeta* out) const;
    void persistTrackMeta(const QString& cacheKey, const TrackMeta& meta) const;
    QString trackMetaFilePath(const QString& cacheKey) const;
    qint64 currentEpochMs() const;

    void maybeRunLruCleanup();
    qint64 totalChunkBytes() const;
    void startProxyServerIfNeeded();
    void onProxyNewConnection();
    void onProxySocketReadyRead(QTcpSocket* socket);
    void handleProxyRequest(QTcpSocket* socket, const QByteArray& requestHeaders);
    bool ensureProxyHeadersSent(QNetworkReply* reply, ProxyStreamState* state);
    void persistAlignedProxyChunk(const ProxyStreamState& state);
    void updateTrackMetaFromProxyReply(const QString& cacheKey, QNetworkReply* reply);
    bool parseHttpRange(const QByteArray& rangeHeader, qint64* start, qint64* end) const;
    void sendProxyError(QTcpSocket* socket, int statusCode, const QByteArray& reason) const;
    QByteArray buildHttpHeaders(int statusCode,
                                const QByteArray& reason,
                                const QByteArray& contentType,
                                qint64 contentLength,
                                const QByteArray& extra = QByteArray()) const;

private:
    QString m_cacheDirectory;
    QString m_chunkRootDirectory;
    QNetworkAccessManager m_networkManager;
    QHash<QNetworkReply*, DownloadTask> m_downloadTasks;
    QHash<QNetworkReply*, SeekIndexRequest> m_seekIndexRequests;
    QSet<QString> m_inflightChunkKeys;
    QHash<QString, TrackMeta> m_trackMetaCache;

    qint64 m_chunkSizeBytes = 128 * 1024;      // 128 KB，降低大幅 seek 的字节定位误差
    int m_startupPrefetchChunks = 6;           // 768 KB warmup
    qint64 m_maxCacheBytes = 512LL * 1024 * 1024; // 512 MB
    qint64 m_lruLastRunMs = 0;
    mutable QTcpServer* m_proxyServer = nullptr;
    mutable quint16 m_proxyPort = 0;
    mutable QHash<QTcpSocket*, QByteArray> m_proxyReadBuffers;
    QHash<QNetworkReply*, ProxyStreamState> m_proxyStreamStates;
    mutable QMutex m_metaMutex;
};

#endif // AUDIO_CACHE_MANAGER_H
