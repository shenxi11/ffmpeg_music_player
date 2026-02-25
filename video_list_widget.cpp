#include "video_list_widget.h"

VideoListWidget::VideoListWidget(PlayWidget* playWidget, QWidget *parent)
    : QWidget(parent)
    , m_playWidget(playWidget)
{
    
    listWidget = new VideoListWidgetQml(this);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(listWidget);
    setLayout(layout);
    
    request = new HttpRequestV2(this);
    
    connect(listWidget, &VideoListWidgetQml::signal_refresh_requested, 
            this, &VideoListWidget::onRefreshRequested);
    
    connect(listWidget, &VideoListWidgetQml::signal_video_selected,
            this, &VideoListWidget::onVideoSelected);
    
    connect(request, &HttpRequestV2::signal_videoList,
            this, &VideoListWidget::onVideoListReceived);
    
    connect(request, &HttpRequestV2::signal_videoStreamUrl,
            this, &VideoListWidget::onVideoStreamUrlReceived);
}

VideoListWidget::~VideoListWidget()
{
    qDebug() << "[VideoListWidget] Destroyed";
}

void VideoListWidget::onRefreshRequested()
{
    qDebug() << "[VideoListWidget] Refresh requested, fetching video list...";
    listWidget->clearAll();
    request->getVideoList();
}

void VideoListWidget::onVideoListReceived(const QVariantList& videoList)
{
    qDebug() << "[VideoListWidget] Received" << videoList.size() << "videos";
    listWidget->clearAll();
    listWidget->addVideoList(videoList);
}

void VideoListWidget::onVideoSelected(const QString& videoPath, const QString& videoName)
{
    qDebug() << "[VideoListWidget] Video selected:" << videoName << "(" << videoPath << ")";
    
    m_selectedVideoName = videoName;
    
    request->getVideoStreamUrl(videoPath);
}

void VideoListWidget::onVideoStreamUrlReceived(const QString& videoUrl)
{
    qDebug() << "[VideoListWidget] Received video stream URL:" << videoUrl;
    
    if (!videoPlayerWindow) {
        videoPlayerWindow = new VideoPlayerWindow(nullptr);
        videoPlayerWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        emit videoPlayerWindowReady(videoPlayerWindow);
        
        connect(videoPlayerWindow, &VideoPlayerWindow::playStateChanged, this, [this](bool isPlaying){
            emit videoPlaybackStateChanged(isPlaying);
        });
    }
    
    videoPlayerWindow->loadVideo(videoUrl);
    
    videoPlayerWindow->show();
    videoPlayerWindow->raise();
    videoPlayerWindow->activateWindow();
    
    // this->hide();
}

void VideoListWidget::pauseVideoPlayback()
{
    if (videoPlayerWindow) {
        videoPlayerWindow->pausePlayback();
    }
}

void VideoListWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    onRefreshRequested();
}
