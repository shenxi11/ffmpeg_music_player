#include "video_list_widget.h"

/*
模块名称: VideoListWidget 连接配置
功能概述: 统一管理视频列表与 ViewModel、播放器窗口之间的信号连接。
对外接口: setupConnections / setupVideoPlayerConnections
维护说明: 连接逻辑集中维护，减少构造函数复杂度。
*/

void VideoListWidget::setupConnections()
{
    connect(listWidget,
            &VideoListWidgetQml::signalRefreshRequested,
            this,
            &VideoListWidget::onRefreshRequested);

    connect(listWidget,
            &VideoListWidgetQml::signalVideoSelected,
            this,
            &VideoListWidget::onVideoSelected);

    connect(m_viewModel,
            &VideoListViewModel::videoListUpdated,
            this,
            &VideoListWidget::onVideoListReceived);

    connect(m_viewModel,
            &VideoListViewModel::videoStreamResolved,
            this,
            &VideoListWidget::onVideoStreamResolved);
}

void VideoListWidget::setupVideoPlayerConnections()
{
    if (!videoPlayerWindow) {
        return;
    }

    connect(videoPlayerWindow, &VideoPlayerWindow::playStateChanged,
            this, &VideoListWidget::onVideoPlayerPlayStateChanged);
}

void VideoListWidget::onVideoStreamResolved(const QString& videoUrl, const QString& videoName)
{
    Q_UNUSED(videoName);
    onVideoStreamUrlReceived(videoUrl);
}

void VideoListWidget::onVideoPlayerPlayStateChanged(bool isPlaying)
{
    emit videoPlaybackStateChanged(isPlaying);
}
