#include "video_list_widget.h"

VideoListWidget::VideoListWidget(PlayWidget* playWidget, QWidget *parent)
    : QWidget(parent)
    , m_playWidget(playWidget)
    , m_viewModel(new VideoListViewModel(this))
{
    
    listWidget = new VideoListWidgetQml(this);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(listWidget);
    setLayout(layout);

    setupConnections();
}

VideoListWidget::~VideoListWidget()
{
    qDebug() << "[VideoListWidget] Destroyed";
}

void VideoListWidget::onRefreshRequested()
{
    qDebug() << "[VideoListWidget] Refresh requested, fetching video list...";
    listWidget->clearAll();
    m_viewModel->refresh();
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
    m_viewModel->resolveVideoStream(videoPath, videoName);
}

void VideoListWidget::onVideoStreamUrlReceived(const QString& videoUrl)
{
    qDebug() << "[VideoListWidget] Received video stream URL:" << videoUrl;
    
    if (!videoPlayerWindow) {
        videoPlayerWindow = new VideoPlayerWindow(nullptr);
        videoPlayerWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        emit videoPlayerWindowReady(videoPlayerWindow);

        setupVideoPlayerConnections();
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
