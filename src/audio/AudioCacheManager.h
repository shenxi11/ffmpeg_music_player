#ifndef AUDIO_CACHE_MANAGER_H
#define AUDIO_CACHE_MANAGER_H

#include <QObject>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QUrl>
#include <QMutex>

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

    // Save known duration to improve byte-range estimation.
    void noteTrackDuration(const QUrl& originalUrl, qint64 durationMs);

private slots:
    void onDownloadFinished(QNetworkReply* reply);

private:
    struct DownloadTask {
        QString cacheKey;
        QUrl originalUrl;
        qint64 startByte = 0;
        qint64 endByte = 0;
        qint64 chunkIndex = -1;
        QString targetFilePath;
        bool metadataProbe = false;
    };

    struct TrackMeta {
        qint64 contentLength = -1;
        qint64 durationMs = -1;
        qint64 lastAccessMs = 0;
    };

    AudioCacheManager(QObject* parent = nullptr);
    AudioCacheManager(const AudioCacheManager&) = delete;
    AudioCacheManager& operator=(const AudioCacheManager&) = delete;

    bool canCacheUrl(const QUrl& url) const;
    QString makeCacheKey(const QUrl& url) const;
    QString ensureCacheDirectory(const QString& requestedPath) const;

    QString legacyFullCachePathForUrl(const QUrl& url) const;

    QString trackCacheDir(const QString& cacheKey) const;
    QString chunkFilePath(const QString& cacheKey, qint64 chunkIndex) const;
    bool hasChunk(const QString& cacheKey, qint64 chunkIndex) const;
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
    QSet<QString> m_inflightChunkKeys;
    QHash<QString, TrackMeta> m_trackMetaCache;

    qint64 m_chunkSizeBytes = 512 * 1024;      // 512 KB
    int m_startupPrefetchChunks = 4;           // 2 MB warmup
    qint64 m_maxCacheBytes = 512LL * 1024 * 1024; // 512 MB
    qint64 m_lruLastRunMs = 0;
    mutable QTcpServer* m_proxyServer = nullptr;
    mutable quint16 m_proxyPort = 0;
    mutable QHash<QTcpSocket*, QByteArray> m_proxyReadBuffers;
    mutable QMutex m_metaMutex;
};

#endif // AUDIO_CACHE_MANAGER_H
