#ifndef COVER_CACHE_MANAGER_H
#define COVER_CACHE_MANAGER_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include <functional>

class QNetworkAccessManager;

/**
 * @brief 客户端封面缓存管理器，统一处理封面 URL 规范化、远程落盘和 QML 展示路径。
 */
class CoverCacheManager : public QObject
{
    Q_OBJECT

public:
    using CacheCallback = std::function<void(const QString&)>;

    static CoverCacheManager& instance();

    QString normalizeCoverSource(const QString& rawCover) const;
    QString lookupCachedCover(const QString& rawCover) const;
    QString cachedOrOriginalCover(const QString& rawCover);
    QString imageSourceForCover(const QString& rawCover);
    void cacheRemoteCover(const QString& rawCover, CacheCallback callback = {});

signals:
    void coverCached(const QString& normalizedSource, const QString& localFilePath);

private:
    explicit CoverCacheManager(QObject* parent = nullptr);

    QString cacheDirectory() const;
    QString indexFilePath() const;
    QString cacheFilePathForSource(const QString& normalizedSource) const;
    QString sourceHash(const QString& normalizedSource) const;
    bool isRemoteSource(const QString& source) const;
    QString toImageSource(const QString& normalizedSource) const;
    void updateIndex(const QString& normalizedSource, const QString& rawCover,
                     const QString& localFilePath) const;

    QNetworkAccessManager* m_networkManager = nullptr;
    QHash<QString, QList<CacheCallback>> m_pendingCallbacks;
};

#endif // COVER_CACHE_MANAGER_H
