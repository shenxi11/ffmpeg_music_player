#include "playlist_widget.h"

#include <QMetaObject>
#include <QDebug>

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
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "loadPlaylists",
                              Q_ARG(QVariant, QVariant::fromValue(playlists)),
                              Q_ARG(QVariant, page),
                              Q_ARG(QVariant, pageSize),
                              Q_ARG(QVariant, total));
}

void PlaylistWidget::loadPlaylistDetail(const QVariantMap& detail)
{
    QQuickItem* root = rootObject();
    if (!root) {
        return;
    }
    QMetaObject::invokeMethod(root, "loadPlaylistDetail",
                              Q_ARG(QVariant, QVariant::fromValue(detail)));
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
