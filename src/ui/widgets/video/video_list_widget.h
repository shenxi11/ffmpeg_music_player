#ifndef VIDEO_LIST_WIDGET_H
#define VIDEO_LIST_WIDGET_H

#include "video_list_widget_qml.h"
#include "video_player_page.h"
#include "viewmodels/VideoListViewModel.h"

#include <QHash>
#include <QWidget>

class QStackedLayout;

/**
 * @brief 在线视频列表窗口
 * 在主页面内切换视频列表与视频详情播放页。
 */
class VideoListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoListWidget(QWidget* parent = nullptr);
    ~VideoListWidget() override;

signals:
    void videoPlaybackStateChanged(bool isPlaying);

public slots:
    void pauseVideoPlayback();

private slots:
    void onRefreshRequested();
    void onVideoListReceived(const QVariantList& videoList);
    void onVideoSelected(const QString& videoPath, const QString& videoName);
    void onVideoStreamResolved(const QString& videoUrl, const QString& videoName);
    void onVideoPlayerPlayStateChanged(bool isPlaying);
    void onReturnToListRequested();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void setupConnections();
    void setupVideoPlayerConnections();
    void showListPage();
    void showPlayerPage();

    VideoListWidgetQml* listWidget = nullptr;
    VideoPlayerPage* m_playerPage = nullptr;
    QStackedLayout* m_pageLayout = nullptr;
    VideoListViewModel* m_viewModel = nullptr;
    QHash<QString, QVariantMap> m_videoEntriesByPath;
    QString m_selectedVideoPath;
    QString m_selectedVideoName;
    qint64 m_selectedVideoSize = 0;
};

#endif // VIDEO_LIST_WIDGET_H
