#include "cover_cache_manager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

namespace {

bool isWindowsDrivePath(const QString& path) {
    return path.size() >= 3 && path.at(1) == QLatin1Char(':') && path.at(0).isLetter() &&
           (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\'));
}

QString collapseDuplicateUploads(QString text) {
    text = QDir::fromNativeSeparators(text.trimmed());
    while (text.contains(QStringLiteral("/uploads/uploads/"), Qt::CaseInsensitive)) {
        text.replace(QStringLiteral("/uploads/uploads/"), QStringLiteral("/uploads/"),
                     Qt::CaseInsensitive);
    }
    while (text.startsWith(QStringLiteral("uploads/uploads/"), Qt::CaseInsensitive)) {
        text = QStringLiteral("uploads/") + text.mid(QStringLiteral("uploads/uploads/").size());
    }
    return text;
}

QString stripUploadsPrefix(QString path) {
    path = collapseDuplicateUploads(path);
    while (path.startsWith(QLatin1Char('/'))) {
        path.remove(0, 1);
    }
    while (path.startsWith(QStringLiteral("uploads/"), Qt::CaseInsensitive)) {
        path = path.mid(QStringLiteral("uploads/").size());
    }
    return path;
}

} // namespace

CoverCacheManager& CoverCacheManager::instance() {
    static CoverCacheManager manager;
    return manager;
}

CoverCacheManager::CoverCacheManager(QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
    QDir().mkpath(cacheDirectory());
}

QString CoverCacheManager::normalizeCoverSource(const QString& rawCover) const {
    QString cover = collapseDuplicateUploads(rawCover);
    const QString lower = cover.toLower();
    if (cover.isEmpty() || lower == QStringLiteral("null") ||
        lower == QStringLiteral("undefined") || lower == QStringLiteral("none") ||
        lower == QStringLiteral("(null)")) {
        return QString();
    }

    if (cover.startsWith(QStringLiteral("qrc:/"), Qt::CaseInsensitive)) {
        return QStringLiteral(":") + cover.mid(4);
    }

    if (cover.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        const QUrl localUrl(cover);
        return localUrl.isLocalFile() ? QDir::fromNativeSeparators(localUrl.toLocalFile())
                                      : QString();
    }

    if (isRemoteSource(cover)) {
        const QUrl url(cover);
        if (!url.isValid()) {
            return QString();
        }

        const QString localCandidate =
            stripUploadsPrefix(QUrl::fromPercentEncoding(url.path().toUtf8()));
        if (isWindowsDrivePath(localCandidate) || QFileInfo(localCandidate).isAbsolute()) {
            return localCandidate;
        }
        return cover;
    }

    QString localCandidate = stripUploadsPrefix(cover);
    if (isWindowsDrivePath(localCandidate) || QFileInfo(localCandidate).isAbsolute()) {
        return localCandidate;
    }
    return collapseDuplicateUploads(cover);
}

QString CoverCacheManager::lookupCachedCover(const QString& rawCover) const {
    const QString normalizedSource = normalizeCoverSource(rawCover);
    if (normalizedSource.isEmpty()) {
        return QString();
    }
    if (normalizedSource.startsWith(QStringLiteral(":/")) || QFileInfo::exists(normalizedSource)) {
        return normalizedSource;
    }
    if (!isRemoteSource(normalizedSource)) {
        return QString();
    }

    const QString cachePath = cacheFilePathForSource(normalizedSource);
    return QFileInfo::exists(cachePath) ? cachePath : QString();
}

QString CoverCacheManager::cachedOrOriginalCover(const QString& rawCover) {
    const QString cached = lookupCachedCover(rawCover);
    if (!cached.isEmpty()) {
        return cached;
    }

    const QString normalizedSource = normalizeCoverSource(rawCover);
    if (isRemoteSource(normalizedSource)) {
        cacheRemoteCover(normalizedSource);
    }
    return normalizedSource;
}

QString CoverCacheManager::imageSourceForCover(const QString& rawCover) {
    return toImageSource(cachedOrOriginalCover(rawCover));
}

void CoverCacheManager::cacheRemoteCover(const QString& rawCover, CacheCallback callback) {
    const QString normalizedSource = normalizeCoverSource(rawCover);
    if (!isRemoteSource(normalizedSource)) {
        if (callback) {
            callback(normalizedSource);
        }
        return;
    }

    const QString cachePath = cacheFilePathForSource(normalizedSource);
    if (QFileInfo::exists(cachePath)) {
        updateIndex(normalizedSource, rawCover, cachePath);
        if (callback) {
            callback(cachePath);
        }
        return;
    }

    const bool alreadyPending = m_pendingCallbacks.contains(normalizedSource);
    if (callback) {
        m_pendingCallbacks[normalizedSource].append(callback);
    } else if (!alreadyPending) {
        m_pendingCallbacks.insert(normalizedSource, {});
    }
    if (alreadyPending) {
        return;
    }

    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl(normalizedSource)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, normalizedSource, rawCover]() {
        QString localFilePath;
        const QByteArray payload = reply->readAll();
        if (reply->error() == QNetworkReply::NoError && !payload.isEmpty()) {
            const QString cachePath = cacheFilePathForSource(normalizedSource);
            QDir().mkpath(QFileInfo(cachePath).absolutePath());
            QSaveFile file(cachePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(payload);
                if (file.commit()) {
                    localFilePath = cachePath;
                    updateIndex(normalizedSource, rawCover, localFilePath);
                    emit coverCached(normalizedSource, localFilePath);
                }
            }
        }

        const QList<CacheCallback> callbacks = m_pendingCallbacks.take(normalizedSource);
        for (const CacheCallback& cb : callbacks) {
            if (cb) {
                cb(localFilePath);
            }
        }
        reply->deleteLater();
    });
}

QString CoverCacheManager::cacheDirectory() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(base.isEmpty() ? QDir::currentPath() : base)
        .absoluteFilePath(QStringLiteral("cover_cache"));
}

QString CoverCacheManager::indexFilePath() const {
    return QDir(cacheDirectory()).absoluteFilePath(QStringLiteral("index.json"));
}

QString CoverCacheManager::cacheFilePathForSource(const QString& normalizedSource) const {
    return QDir(cacheDirectory())
        .absoluteFilePath(sourceHash(normalizedSource) + QStringLiteral(".img"));
}

QString CoverCacheManager::sourceHash(const QString& normalizedSource) const {
    const QByteArray digest =
        QCryptographicHash::hash(normalizedSource.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

bool CoverCacheManager::isRemoteSource(const QString& source) const {
    return source.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           source.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

QString CoverCacheManager::toImageSource(const QString& normalizedSource) const {
    if (normalizedSource.isEmpty()) {
        return QString();
    }
    if (normalizedSource.startsWith(QStringLiteral(":/"))) {
        return QStringLiteral("qrc") + normalizedSource.mid(1);
    }
    if (!isRemoteSource(normalizedSource) &&
        (isWindowsDrivePath(normalizedSource) || QFileInfo(normalizedSource).isAbsolute())) {
        return QUrl::fromLocalFile(normalizedSource).toString();
    }
    return normalizedSource;
}

void CoverCacheManager::updateIndex(const QString& normalizedSource, const QString& rawCover,
                                    const QString& localFilePath) const {
    QJsonObject index;
    QFile in(indexFilePath());
    if (in.open(QIODevice::ReadOnly)) {
        const QJsonDocument document = QJsonDocument::fromJson(in.readAll());
        if (document.isObject()) {
            index = document.object();
        }
    }

    QJsonObject entry;
    entry.insert(QStringLiteral("originalSource"), rawCover);
    entry.insert(QStringLiteral("normalizedSource"), normalizedSource);
    entry.insert(QStringLiteral("localFilePath"), QDir::fromNativeSeparators(localFilePath));
    entry.insert(QStringLiteral("cachedAt"),
                 QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    index.insert(sourceHash(normalizedSource), entry);

    QDir().mkpath(cacheDirectory());
    QSaveFile out(indexFilePath());
    if (!out.open(QIODevice::WriteOnly)) {
        return;
    }
    out.write(QJsonDocument(index).toJson(QJsonDocument::Compact));
    out.commit();
}
