#include "video_list_widget.h"

/*
模块名称: VideoListWidget 连接配置
功能概述: 统一管理视频列表与 ViewModel、内嵌播放器页之间的信号连接。
对外接口: setupConnections / setupVideoPlayerConnections
维护说明: 连接逻辑集中维护，减少构造函数复杂度。
*/

void VideoListWidget::setupConnections()
{
    connect(listWidget, &VideoListWidgetQml::signalRefreshRequested, this,
            &VideoListWidget::onRefreshRequested);

    connect(listWidget, &VideoListWidgetQml::signalVideoSelected, this,
            &VideoListWidget::onVideoSelected);

    connect(m_viewModel, &VideoListViewModel::videoListUpdated, this,
            &VideoListWidget::onVideoListReceived);

    connect(m_viewModel, &VideoListViewModel::videoStreamResolved, this,
            &VideoListWidget::onVideoStreamResolved);
}

void VideoListWidget::setupVideoPlayerConnections()
{
    if (!m_playerPage) {
        return;
    }

    connect(m_playerPage, &VideoPlayerPage::playStateChanged, this,
            &VideoListWidget::onVideoPlayerPlayStateChanged);
    connect(m_playerPage, &VideoPlayerPage::backRequested, this,
            &VideoListWidget::onReturnToListRequested);
}

void VideoListWidget::onVideoStreamResolved(const QString& videoUrl, const QString& videoName)
{
    Q_UNUSED(videoName);

    if (!m_playerPage) {
        return;
    }

    m_playerPage->setVideoInfo(m_selectedVideoName, m_selectedVideoPath, m_selectedVideoSize);
    m_playerPage->loadVideo(videoUrl);
    showPlayerPage();
    m_playerPage->resumePlayback();
}

void VideoListWidget::onVideoPlayerPlayStateChanged(bool isPlaying)
{
    emit videoPlaybackStateChanged(isPlaying);
}

void VideoListWidget::onReturnToListRequested()
{
    pauseVideoPlayback();
    showListPage();
}
