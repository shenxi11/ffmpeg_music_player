#include "playlist_widget.h"

#include <QMetaObject>
#include <QDebug>

namespace {

qint64 parseTrackIdValue(const QVariantMap& rawItem)
{
    qint64 trackId = rawItem.value(QStringLiteral("track_id")).toLongLong();
    if (trackId > 0) {
        return trackId;
    }

    trackId = rawItem.value(QStringLiteral("music_id")).toLongLong();
    if (trackId > 0) {
        return trackId;
    }

    trackId = rawItem.value(QStringLiteral("id")).toLongLong();
    if (trackId > 0) {
        return trackId;
    }

    return rawItem.value(QStringLiteral("trackId")).toLongLong();
}

QString normalizeCoverValue(const QVariant& value)
{
    const QString cover = value.toString().trimmed();
    if (cover.compare(QStringLiteral("null"), Qt::CaseInsensitive) == 0 ||
        cover.compare(QStringLiteral("undefined"), Qt::CaseInsensitive) == 0 ||
        cover.compare(QStringLiteral("none"), Qt::CaseInsensitive) == 0) {
        return QString();
    }
    return cover;
}

QString derivePlaylistCoverFromItems(const QVariantList& items)
{
    if (items.isEmpty()) {
        return QString();
    }

    const QVariantMap firstItem = items.constFirst().toMap();
    const QStringList candidateKeys = {
        QStringLiteral("cover_art_url"),
        QStringLiteral("cover_url"),
        QStringLiteral("cover_art_path")
    };
    for (const QString& key : candidateKeys) {
        const QString cover = normalizeCoverValue(firstItem.value(key));
        if (!cover.isEmpty()) {
            return cover;
        }
    }
    return QString();
}

}

/*
模块名: PlaylistWidget 桥接实现
功能概述: 统一连接 QML 歌单页信号，并向 C++ 业务层转发。
对外接口: PlaylistWidget 构造与数据更新函数。
依赖关系: PlaylistWidget.qml
输入输出: 输入为歌单数据，输出为用户操作事件。
异常与错误: 若 rootObject 为空，调用会安全返回。
维护说明: 该文件不应直接发网络请求。
*/

PlaylistWidget::PlaylistWidget(QWidget* parent)
    : QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/components/library/PlaylistWidget.qml"));
    setAttribute(Qt::WA_TranslucentBackground);

    QQuickItem* root = rootObject();
    if (!root) {
        qWarning() << "[PlaylistWidget] rootObject is null";
        return;
    }

    connect(root, SIGNAL(loginRequested()),
            this, SIGNAL(loginRequested()));
    connect(root, SIGNAL(refreshRequested()),
            this, SIGNAL(refreshRequested()));
    connect(root, SIGNAL(openPlaylistRequested(QVariant)),
            this, SLOT(handleOpenPlaylistRequested(QVariant)));
    connect(root, SIGNAL(createPlaylistRequested(QString,QString)),
            this, SIGNAL(createPlaylistRequested(QString,QString)));
    connect(root, SIGNAL(updatePlaylistRequested(QVariant,QString,QString)),
            this, SLOT(handleUpdatePlaylistRequested(QVariant,QString,QString)));
    connect(root, SIGNAL(deletePlaylistRequested(QVariant)),
            this, SLOT(handleDeletePlaylistRequested(QVariant)));
    connect(root, SIGNAL(removePlaylistItemsRequested(QVariant,QVariant)),
            this, SLOT(handleRemovePlaylistItemsRequested(QVariant,QVariant)));
    connect(root, SIGNAL(reorderPlaylistItemsRequested(QVariant,QVariant)),
            this, SLOT(handleReorderPlaylistItemsRequested(QVariant,QVariant)));
    connect(root, SIGNAL(addCurrentSongRequested(QVariant)),
            this, SLOT(handleAddCurrentSongRequested(QVariant)));
    connect(root, SIGNAL(playMusicWithMetadata(QString,QString,QString,QString)),
            this, SIGNAL(playMusicWithMetadata(QString,QString,QString,QString)));
    connect(root, SIGNAL(songActionRequested(QString,QVariant)),
            this, SLOT(handleSongActionRequested(QString,QVariant)));
}

void PlaylistWidget::setLoggedIn(bool loggedIn, const QString& userAccount)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("isLoggedIn", loggedIn);
    root->setProperty("userAccount", userAccount);
}

void PlaylistWidget::loadPlaylists(const QVariantList& playlists, int page, int pageSize, int total)
{
    QVariantList normalizedPlaylists;
    normalizedPlaylists.reserve(playlists.size());
    for (const QVariant& value : playlists) {
        QVariantMap item = value.toMap();
        const qint64 playlistId = item.value(QStringLiteral("id")).toLongLong();
        if (playlistId > 0 && m_cachedPlaylistCoverUrls.contains(playlistId)) {
            item.insert(QStringLiteral("cover_url"), m_cachedPlaylistCoverUrls.value(playlistId));
        }
        normalizedPlaylists.push_back(item);
    }

    m_lastPlaylists = normalizedPlaylists;
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "loadPlaylists",
                              Q_ARG(QVariant, QVariant::fromValue(normalizedPlaylists)),
                              Q_ARG(QVariant, page),
                              Q_ARG(QVariant, pageSize),
                              Q_ARG(QVariant, total));
}

void PlaylistWidget::loadPlaylistDetail(const QVariantMap& detail)
{
    const QVariantMap normalizedDetail = normalizePlaylistDetailForCover(detail);
    m_lastPlaylistDetail = normalizedDetail;
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "loadPlaylistDetail",
                              Q_ARG(QVariant, QVariant::fromValue(normalizedDetail)));
}

void PlaylistWidget::updatePlaylistCoverFromDetail(const QVariantMap& detail)
{
    const QVariantMap normalizedDetail = normalizePlaylistDetailForCover(detail);
    const qint64 playlistId = normalizedDetail.value(QStringLiteral("id")).toLongLong();
    if (playlistId <= 0) {
        return;
    }

    const QString coverUrl = normalizedDetail.value(QStringLiteral("cover_url")).toString().trimmed();
    updateCachedPlaylistCover(playlistId, coverUrl);

    if (!m_lastPlaylistDetail.isEmpty() &&
        m_lastPlaylistDetail.value(QStringLiteral("id")).toLongLong() == playlistId) {
        m_lastPlaylistDetail = normalizedDetail;
    }

    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "updatePlaylistCover",
                              Q_ARG(QVariant, playlistId),
                              Q_ARG(QVariant, coverUrl));
}

void PlaylistWidget::setFavoritePaths(const QStringList& favoritePaths)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("favoritePaths", QVariant::fromValue(favoritePaths));
}

void PlaylistWidget::setCurrentPlayingPath(const QString& filePath)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    root->setProperty("currentPlayingPath", filePath);
}

void PlaylistWidget::setPlayingState(const QString& filePath, bool playing)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "setPlayingState",
                              Q_ARG(QVariant, filePath),
                              Q_ARG(QVariant, playing));
}

void PlaylistWidget::clearData()
{
    m_lastPlaylists.clear();
    m_lastPlaylistDetail.clear();
    m_cachedPlaylistCoverUrls.clear();
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "clearData");
}

void PlaylistWidget::openCreatePlaylistDialog()
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "openCreatePlaylistDialog");
}

qint64 PlaylistWidget::selectedPlaylistIdValue() const
{
    QQuickItem* root = rootObject();
    if (!root) {
        return -1;
    }
    return root->property("selectedPlaylistId").toLongLong();
}

QVariantMap PlaylistWidget::currentPlaylistDetailSnapshot() const
{
    if (!m_lastPlaylistDetail.isEmpty()) {
        return m_lastPlaylistDetail;
    }
    QQuickItem* root = rootObject();
    if (!root) {
        return {};
    }
    return root->property("currentPlaylistDetail").toMap();
}

QVariantList PlaylistWidget::currentPlaylistTrackIds() const
{
    const QVariantMap detail = currentPlaylistDetailSnapshot();
    const QVariantList items = detail.value(QStringLiteral("items")).toList();

    QVariantList trackIds;
    trackIds.reserve(items.size());
    for (const QVariant& value : items) {
        const qint64 trackId = parseTrackIdValue(value.toMap());
        if (trackId > 0) {
            trackIds.push_back(trackId);
        }
    }
    return trackIds;
}

QVariantList PlaylistWidget::ownedPlaylistsSnapshot() const
{
    QVariantList items;
    for (const QVariant& value : m_lastPlaylists) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("ownership")).toString().trimmed().compare(
                QStringLiteral("subscribed"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        items.push_back(item);
    }
    return items;
}

QVariantList PlaylistWidget::subscribedPlaylistsSnapshot() const
{
    QVariantList items;
    for (const QVariant& value : m_lastPlaylists) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("ownership")).toString().trimmed().compare(
                QStringLiteral("subscribed"), Qt::CaseInsensitive) == 0) {
            items.push_back(item);
        }
    }
    return items;
}

QVariantMap PlaylistWidget::normalizePlaylistDetailForCover(const QVariantMap& detail)
{
    QVariantMap normalizedDetail = detail;
    const qint64 playlistId = normalizedDetail.value(QStringLiteral("id")).toLongLong();
    const QString derivedCover = derivePlaylistCoverFromItems(
        normalizedDetail.value(QStringLiteral("items")).toList());
    normalizedDetail.insert(QStringLiteral("cover_url"), derivedCover);
    if (playlistId > 0) {
        updateCachedPlaylistCover(playlistId, derivedCover);
    }
    return normalizedDetail;
}

void PlaylistWidget::updateCachedPlaylistCover(qint64 playlistId, const QString& coverUrl)
{
    if (playlistId <= 0) {
        return;
    }

    m_cachedPlaylistCoverUrls.insert(playlistId, coverUrl);
    for (QVariant& value : m_lastPlaylists) {
        QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("id")).toLongLong() != playlistId) {
            continue;
        }
        item.insert(QStringLiteral("cover_url"), coverUrl);
        value = item;
        break;
    }
}

void PlaylistWidget::handleOpenPlaylistRequested(const QVariant& playlistIdValue)
{
    emit openPlaylistRequested(playlistIdValue.toLongLong());
}

void PlaylistWidget::handleUpdatePlaylistRequested(const QVariant& playlistIdValue,
                                                   const QString& name,
                                                   const QString& description)
{
    emit updatePlaylistRequested(playlistIdValue.toLongLong(), name, description);
}

void PlaylistWidget::handleDeletePlaylistRequested(const QVariant& playlistIdValue)
{
    emit deletePlaylistRequested(playlistIdValue.toLongLong());
}

void PlaylistWidget::handleRemovePlaylistItemsRequested(const QVariant& playlistIdValue, const QVariant& musicPathsValue)
{
    QStringList musicPaths;
    const QVariantList rawList = musicPathsValue.toList();
    for (const QVariant& item : rawList) {
        const QString path = item.toString().trimmed();
        if (!path.isEmpty()) {
            musicPaths.append(path);
        }
    }
    emit removePlaylistItemsRequested(playlistIdValue.toLongLong(), musicPaths);
}

void PlaylistWidget::handleReorderPlaylistItemsRequested(const QVariant& playlistIdValue, const QVariant& orderedItemsValue)
{
    emit reorderPlaylistItemsRequested(playlistIdValue.toLongLong(), orderedItemsValue.toList());
}

void PlaylistWidget::handleAddCurrentSongRequested(const QVariant& playlistIdValue)
{
    emit addCurrentSongRequested(playlistIdValue.toLongLong());
}

void PlaylistWidget::handleSongActionRequested(const QString& action, const QVariant& payload)
{
    emit songActionRequested(action, payload.toMap());
}
