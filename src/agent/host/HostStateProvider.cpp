#include "HostStateProvider.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

#include "MainShellViewModel.h"
#include "AudioService.h"
#include "AudioSession.h"
#include "local_music_cache.h"
#include "music.h"

namespace {

qint64 parseDurationSeconds(const QString& durationText)
{
    const QString trimmed = durationText.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    if (trimmed.contains(':')) {
        const QStringList parts = trimmed.split(':');
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

}

HostStateProvider::HostStateProvider(QObject* parent)
    : QObject(parent)
{
}

void HostStateProvider::setMainShellViewModel(MainShellViewModel* shellViewModel)
{
    m_shellViewModel = shellViewModel;
}

QString HostStateProvider::currentUserAccount() const
{
    if (!m_shellViewModel) {
        return QString();
    }
    return m_shellViewModel->currentUserAccount().trimmed();
}

QVariantMap HostStateProvider::currentTrackSnapshot() const
{
    QVariantMap result;

    AudioSession* session = AudioService::instance().currentSession();
    const QUrl currentUrl = AudioService::instance().currentUrl();
    if (!session || currentUrl.isEmpty()) {
        result.insert(QStringLiteral("playing"), false);
        return result;
    }

    const QString path = currentUrl.toString();
    const QString title = session->title().trimmed().isEmpty()
        ? QFileInfo(currentUrl.path()).completeBaseName()
        : session->title().trimmed();
    const QString artist = session->artist().trimmed().isEmpty()
        ? QStringLiteral("未知艺术家")
        : session->artist().trimmed();
    const qint64 durationMs = qMax<qint64>(0, session->duration());
    const qint64 positionMs = qMax<qint64>(0, session->position());

    result.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
    result.insert(QStringLiteral("musicPath"), path);
    result.insert(QStringLiteral("title"), title);
    result.insert(QStringLiteral("artist"), artist);
    result.insert(QStringLiteral("album"), QString());
    result.insert(QStringLiteral("playlistId"), QString());
    result.insert(QStringLiteral("durationMs"), durationMs);
    result.insert(QStringLiteral("positionMs"), positionMs);
    result.insert(QStringLiteral("playing"), AudioService::instance().isPlaying());
    result.insert(QStringLiteral("coverUrl"), session->albumArt());
    return result;
}

QVariantList HostStateProvider::convertMusicList(const QList<Music>& musics, int limit) const
{
    QVariantList items;
    const int bounded = (limit <= 0) ? musics.size() : qMin(limit, musics.size());
    for (int i = 0; i < bounded; ++i) {
        items.append(convertMusicItem(musics.at(i)));
    }
    return items;
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
                  QVariantMap{
                      {QStringLiteral("playlistId"), playlistId},
                      {QStringLiteral("name"), detail.value(QStringLiteral("name")).toString()},
                      {QStringLiteral("trackCount"), trackCount}
                  });

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
            ? QStringLiteral("未知艺术家")
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
    const QList<LocalMusicInfo> locals = LocalMusicCache::instance().getMusicList();
    const int bounded = (limit <= 0) ? locals.size() : qMin(limit, locals.size());

    QVariantList items;
    items.reserve(bounded);
    for (int i = 0; i < bounded; ++i) {
        const LocalMusicInfo& local = locals.at(i);
        QVariantMap item;
        const QString path = local.filePath.trimmed();
        const QString title = local.fileName.trimmed().isEmpty()
            ? QFileInfo(path).completeBaseName()
            : local.fileName.trimmed();
        const QString artist = local.artist.trimmed().isEmpty()
            ? QStringLiteral("未知艺术家")
            : local.artist.trimmed();
        const qint64 durationSec = parseDurationSeconds(local.duration);
        item.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
        item.insert(QStringLiteral("musicPath"), path);
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("artist"), artist);
        item.insert(QStringLiteral("album"), QString());
        item.insert(QStringLiteral("durationMs"), durationSec * 1000);
        item.insert(QStringLiteral("isFavorite"), false);
        item.insert(QStringLiteral("coverUrl"), local.coverUrl);
        item.insert(QStringLiteral("isLocal"), true);
        items.push_back(item);
    }

    return items;
}

QVariantMap HostStateProvider::convertMusicItem(const Music& music) const
{
    QVariantMap item;
    const QString path = music.getSongPath();
    const QString title = music.getSongName().trimmed().isEmpty()
        ? QFileInfo(path).completeBaseName()
        : music.getSongName().trimmed();
    const QString artist = music.getSinger().trimmed().isEmpty()
        ? QStringLiteral("未知艺术家")
        : music.getSinger().trimmed();
    const qint64 durationMs = normalizeDurationMs(music.getDuration());

    item.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
    item.insert(QStringLiteral("musicPath"), path);
    item.insert(QStringLiteral("title"), title);
    item.insert(QStringLiteral("artist"), artist);
    item.insert(QStringLiteral("album"), QString());
    item.insert(QStringLiteral("durationMs"), durationMs);
    item.insert(QStringLiteral("isFavorite"), false);
    item.insert(QStringLiteral("coverUrl"), music.getPicPath());
    return item;
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
        ? QStringLiteral("未知艺术家")
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
