#include "HostStateProvider.h"

#include <QCryptographicHash>
#include <QMetaObject>
#include <QStringList>

namespace {

template <typename Return, typename... Args>
Return invokeValue(QObject* obj, const char* method, const Return& fallback, Args&&... args)
{
    if (!obj) {
        return fallback;
    }

    Return result = fallback;
    const bool ok = QMetaObject::invokeMethod(obj,
                                              method,
                                              Qt::DirectConnection,
                                              Q_RETURN_ARG(Return, result),
                                              Q_ARG(std::decay_t<Args>, std::forward<Args>(args))...);
    return ok ? result : fallback;
}

qint64 parseDurationSeconds(const QString& durationText)
{
    const QString trimmed = durationText.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    if (trimmed.contains(QLatin1Char(':'))) {
        const QStringList parts = trimmed.split(QLatin1Char(':'));
        if (parts.size() >= 2) {
            bool okMin = false;
            bool okSec = false;
            const qint64 minutes = parts.at(0).toLongLong(&okMin);
            const qint64 seconds = parts.at(1).toLongLong(&okSec);
            if (okMin && okSec) {
                return qMax<qint64>(0, minutes * 60 + seconds);
            }
        }
    }

    bool ok = false;
    const qint64 value = trimmed.toLongLong(&ok);
    return ok ? qMax<qint64>(0, value) : 0;
}

qint64 parsePlaylistIdValue(const QVariantMap& raw)
{
    qint64 playlistId = raw.value(QStringLiteral("playlist_id")).toLongLong();
    if (playlistId > 0) {
        return playlistId;
    }
    playlistId = raw.value(QStringLiteral("id")).toLongLong();
    if (playlistId > 0) {
        return playlistId;
    }
    return raw.value(QStringLiteral("playlistId")).toLongLong();
}

QVariantList normalizeStringVariantList(const QVariantList& rawItems)
{
    QVariantList normalized;
    normalized.reserve(rawItems.size());
    for (const QVariant& raw : rawItems) {
        const QString text = raw.toString().trimmed();
        if (!text.isEmpty()) {
            normalized.push_back(text);
        }
    }
    return normalized;
}

} // namespace

HostStateProvider::HostStateProvider(QObject* parent)
    : QObject(parent)
{
}

void HostStateProvider::setHostContext(QObject* hostContext)
{
    m_hostContext = hostContext;
}

QObject* HostStateProvider::hostService() const
{
    if (!m_hostContext) {
        return nullptr;
    }

    QObject* rawService = nullptr;
    QMetaObject::invokeMethod(m_hostContext,
                              "service",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QObject*, rawService),
                              Q_ARG(QString, QStringLiteral("clientAutomationHost")));
    return rawService;
}

QString HostStateProvider::currentUserAccount() const
{
    return invokeValue<QString>(hostService(), "currentUserAccount", QString()).trimmed();
}

QVariantMap HostStateProvider::currentTrackSnapshot() const
{
    QVariantMap result = invokeValue<QVariantMap>(hostService(),
                                                  "currentTrackSnapshot",
                                                  QVariantMap());
    if (result.isEmpty()) {
        return {{QStringLiteral("playing"), false}};
    }

    const QString path = result.value(QStringLiteral("musicPath")).toString();
    const QString title = result.value(QStringLiteral("title")).toString();
    const QString artist = result.value(QStringLiteral("artist")).toString();
    result.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
    result.insert(QStringLiteral("album"), result.value(QStringLiteral("album")).toString());
    result.insert(QStringLiteral("playlistId"), result.value(QStringLiteral("playlistId")).toString());
    return result;
}

QVariantMap HostStateProvider::hostContextSnapshot() const
{
    QVariantMap payload = invokeValue<QVariantMap>(hostService(),
                                                   "hostContextSnapshot",
                                                   QVariantMap());
    payload.insert(QStringLiteral("selectedTrackIds"),
                   normalizeStringVariantList(payload.value(QStringLiteral("selectedTrackIds")).toList()));
    return payload;
}

QVariantList HostStateProvider::convertHistoryList(const QVariantList& history, int limit) const
{
    QVariantList items;
    const int bounded = (limit <= 0) ? history.size() : qMin(limit, history.size());
    for (int i = 0; i < bounded; ++i) {
        items.append(convertHistoryItem(history.at(i).toMap()));
    }
    return items;
}

QVariantList HostStateProvider::convertFavoriteList(const QVariantList& favorites, int limit) const
{
    QVariantList items;
    const int bounded = (limit <= 0) ? favorites.size() : qMin(limit, favorites.size());
    for (int i = 0; i < bounded; ++i) {
        items.append(convertHistoryItem(favorites.at(i).toMap()));
    }
    return items;
}

QVariantList HostStateProvider::convertPlaylistList(const QVariantList& playlists) const
{
    QVariantList items;
    for (const QVariant& value : playlists) {
        const QVariantMap raw = value.toMap();
        QVariantMap item;
        const qint64 playlistId = parsePlaylistIdValue(raw);
        item.insert(QStringLiteral("playlistId"), playlistId);
        item.insert(QStringLiteral("name"), raw.value(QStringLiteral("name")).toString());
        item.insert(QStringLiteral("trackCount"), raw.value(QStringLiteral("track_count")).toInt());
        item.insert(QStringLiteral("description"), raw.value(QStringLiteral("description")).toString());
        item.insert(QStringLiteral("coverUrl"), raw.value(QStringLiteral("cover_url")).toString());
        items.append(item);
    }
    return items;
}

QVariantMap HostStateProvider::convertPlaylistDetail(const QVariantMap& detail) const
{
    QVariantMap result;
    const qint64 playlistId = parsePlaylistIdValue(detail);
    const QVariantList rawItems = detail.value(QStringLiteral("items")).toList();
    const int trackCount = detail.contains(QStringLiteral("track_count"))
        ? detail.value(QStringLiteral("track_count")).toInt()
        : rawItems.size();
    result.insert(QStringLiteral("playlist"),
                  QVariantMap{{QStringLiteral("playlistId"), playlistId},
                              {QStringLiteral("name"), detail.value(QStringLiteral("name")).toString()},
                              {QStringLiteral("trackCount"), trackCount}});

    QVariantList items;
    for (const QVariant& value : rawItems) {
        const QVariantMap raw = value.toMap();
        QVariantMap item;
        const QString path = raw.value(QStringLiteral("music_path")).toString().trimmed().isEmpty()
            ? raw.value(QStringLiteral("path")).toString()
            : raw.value(QStringLiteral("music_path")).toString();
        const QString title = raw.value(QStringLiteral("title")).toString().trimmed().isEmpty()
            ? raw.value(QStringLiteral("music_title")).toString()
            : raw.value(QStringLiteral("title")).toString();
        const QString artist = raw.value(QStringLiteral("artist")).toString().trimmed().isEmpty()
            ? QStringLiteral("?????")
            : raw.value(QStringLiteral("artist")).toString().trimmed();
        qint64 durationMs = normalizeDurationMs(raw.value(QStringLiteral("duration_sec")).toLongLong());
        if (durationMs <= 0) {
            durationMs = parseDurationSeconds(raw.value(QStringLiteral("duration")).toString()) * 1000;
        }

        item.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
        item.insert(QStringLiteral("musicPath"), path);
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("artist"), artist);
        item.insert(QStringLiteral("album"), raw.value(QStringLiteral("album")).toString());
        item.insert(QStringLiteral("durationMs"), durationMs);
        item.insert(QStringLiteral("isFavorite"), false);
        QString coverUrl = raw.value(QStringLiteral("cover_art_url")).toString();
        if (coverUrl.trimmed().isEmpty()) {
            coverUrl = raw.value(QStringLiteral("cover_url")).toString();
        }
        item.insert(QStringLiteral("coverUrl"), coverUrl);
        items.append(item);
    }
    result.insert(QStringLiteral("items"), items);
    return result;
}

QVariantList HostStateProvider::convertLocalMusicList(int limit) const
{
    return invokeValue<QVariantList>(hostService(), "localMusicItems", QVariantList(), limit);
}

QVariantMap HostStateProvider::convertHistoryItem(const QVariantMap& raw) const
{
    QVariantMap item;
    const QString path = raw.value(QStringLiteral("path")).toString().trimmed().isEmpty()
        ? raw.value(QStringLiteral("music_path")).toString()
        : raw.value(QStringLiteral("path")).toString();
    const QString title = raw.value(QStringLiteral("title")).toString().trimmed().isEmpty()
        ? raw.value(QStringLiteral("music_title")).toString()
        : raw.value(QStringLiteral("title")).toString();
    const QString artist = raw.value(QStringLiteral("artist")).toString().trimmed().isEmpty()
        ? QStringLiteral("?????")
        : raw.value(QStringLiteral("artist")).toString().trimmed();

    item.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
    item.insert(QStringLiteral("musicPath"), path);
    item.insert(QStringLiteral("title"), title);
    item.insert(QStringLiteral("artist"), artist);
    item.insert(QStringLiteral("album"), raw.value(QStringLiteral("album")).toString());
    qint64 durationMs = normalizeDurationMs(raw.value(QStringLiteral("duration_sec")).toLongLong());
    if (durationMs <= 0) {
        durationMs = parseDurationSeconds(raw.value(QStringLiteral("duration")).toString()) * 1000;
    }
    item.insert(QStringLiteral("durationMs"), durationMs);
    item.insert(QStringLiteral("isFavorite"), false);
    item.insert(QStringLiteral("coverUrl"), raw.value(QStringLiteral("cover_art_url")).toString());
    return item;
}

qint64 HostStateProvider::normalizeDurationMs(qint64 rawDuration)
{
    if (rawDuration <= 0) {
        return 0;
    }
    if (rawDuration < 24 * 60 * 60) {
        return rawDuration * 1000;
    }
    return rawDuration;
}

QString HostStateProvider::fallbackTrackId(const QString& path, const QString& title, const QString& artist)
{
    const QString base = QStringLiteral("%1|%2|%3").arg(path, title, artist);
    const QByteArray digest = QCryptographicHash::hash(base.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}
