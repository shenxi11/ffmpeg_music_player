#ifndef PLAYLIST_WIDGET_H
#define PLAYLIST_WIDGET_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>

/*
模块名: PlaylistWidget
功能概述: 封装“我的歌单”QML页面并向上层转发歌单交互信号。
对外接口: setLoggedIn/loadPlaylists/loadPlaylistDetail/setCurrentPlayingPath/setPlayingState
依赖关系: qml/components/library/PlaylistWidget.qml
输入输出: 输入歌单列表与详情数据，输出新建/删除/加歌/删歌/播放等用户意图。
异常与错误: QML根对象为空时跳过调用并记录日志。
维护说明: 仅负责 UI 桥接，不承载网络请求逻辑。
*/
class PlaylistWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget* parent = nullptr);

    void setLoggedIn(bool loggedIn, const QString& userAccount = QString());
    void loadPlaylists(const QVariantList& playlists, int page, int pageSize, int total);
    void loadPlaylistDetail(const QVariantMap& detail);
    void updatePlaylistCoverFromDetail(const QVariantMap& detail);
    void setFavoritePaths(const QStringList& favoritePaths);
    void setCurrentPlayingPath(const QString& filePath);
    void setPlayingState(const QString& filePath, bool playing);
    void clearData();
    void openCreatePlaylistDialog();
    qint64 selectedPlaylistIdValue() const;
    QVariantMap currentPlaylistDetailSnapshot() const;
    QVariantList currentPlaylistTrackIds() const;
    QVariantList ownedPlaylistsSnapshot() const;
    QVariantList subscribedPlaylistsSnapshot() const;

signals:
    void loginRequested();
    void refreshRequested();
    void openPlaylistRequested(qint64 playlistId);
    void createPlaylistRequested(const QString& name, const QString& description);
    void updatePlaylistRequested(qint64 playlistId, const QString& name, const QString& description);
    void deletePlaylistRequested(qint64 playlistId);
    void removePlaylistItemsRequested(qint64 playlistId, const QStringList& musicPaths);
    void reorderPlaylistItemsRequested(qint64 playlistId, const QVariantList& orderedItems);
    void addCurrentSongRequested(qint64 playlistId);
    void playMusicWithMetadata(const QString& filePath,
                               const QString& title,
                               const QString& artist,
                               const QString& cover);
    void songActionRequested(const QString& action, const QVariantMap& songData);

private slots:
    void handleOpenPlaylistRequested(const QVariant& playlistIdValue);
    void handleUpdatePlaylistRequested(const QVariant& playlistIdValue,
                                       const QString& name,
                                       const QString& description);
    void handleDeletePlaylistRequested(const QVariant& playlistIdValue);
    void handleRemovePlaylistItemsRequested(const QVariant& playlistIdValue, const QVariant& musicPathsValue);
    void handleReorderPlaylistItemsRequested(const QVariant& playlistIdValue, const QVariant& orderedItemsValue);
    void handleAddCurrentSongRequested(const QVariant& playlistIdValue);
    void handleSongActionRequested(const QString& action, const QVariant& payload);

private:
    QVariantMap normalizePlaylistDetailForCover(const QVariantMap& detail);
    void updateCachedPlaylistCover(qint64 playlistId, const QString& coverUrl);
    QVariantList m_lastPlaylists;
    QVariantMap m_lastPlaylistDetail;
    QHash<qint64, QString> m_cachedPlaylistCoverUrls;
};

#endif // PLAYLIST_WIDGET_H
