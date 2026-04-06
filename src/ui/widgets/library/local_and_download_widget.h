#ifndef LOCAL_AND_DOWNLOAD_WIDGET_H
#define LOCAL_AND_DOWNLOAD_WIDGET_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include "download_task_model.h"
#include "local_music_model.h"

/**
 * @brief 本地和下载页面包装类
 * 包含三个Tab：本地音乐、下载音乐、正在下载
 */
class LocalAndDownloadWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit LocalAndDownloadWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 设置当前播放的歌曲路径（用于高亮显示）
     */
    void setCurrentPlayingPath(const QString& path);
    void setAvailablePlaylists(const QVariantList& playlists);
    void setFavoritePaths(const QStringList& favoritePaths);
    
signals:
    void playMusic(const QString& filename);
    void deleteMusic(const QString& filename);
    void addLocalMusicRequested();
    void addToFavorite(const QString& path, const QString& title, const QString& artist, const QString& duration);
    void songActionRequested(const QString& action, const QVariantMap& songData);

private slots:
    void onPlayMusic(const QString& filename);
    void onDeleteMusic(const QString& filename);
    void onSongActionRequested(const QString& action, const QVariant& payload);

private:
    DownloadTaskModel* m_downloadTaskModel;
    LocalMusicModel* m_localMusicModel;
};

#endif // LOCAL_AND_DOWNLOAD_WIDGET_H
