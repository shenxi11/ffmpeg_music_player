#include "video_list_widget.h"

VideoListWidget::VideoListWidget(PlayWidget* playWidget, QWidget *parent)
    : QWidget(parent)
    , m_playWidget(playWidget)
{
    // 内嵌控件，不需要设置窗口标题和独立窗口属性
    // setWindowTitle 和 setFixedSize 将在 MainWidget 中设置
    
    // 创建 QML 视频列表控件
    listWidget = new VideoListWidgetQml(this);
    
    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(listWidget);
    setLayout(layout);
    
    // 获取 HttpRequest 实例
    request = HttpRequestPool::getInstance().getRequest();
    
    // 连接刷新信号
    connect(listWidget, &VideoListWidgetQml::signal_refresh_requested, 
            this, &VideoListWidget::onRefreshRequested);
    
    // 连接视频选择信号
    connect(listWidget, &VideoListWidgetQml::signal_video_selected,
            this, &VideoListWidget::onVideoSelected);
    
    // 连接视频列表响应
    connect(request, &HttpRequest::signal_videoList,
            this, &VideoListWidget::onVideoListReceived);
    
    // 连接视频流URL响应
    connect(request, &HttpRequest::signal_videoStreamUrl,
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
    
    // 保存选中的视频信息
    m_selectedVideoName = videoName;
    
    // 请求视频流URL
    request->getVideoStreamUrl(videoPath);
}

void VideoListWidget::onVideoStreamUrlReceived(const QString& videoUrl)
{
    qDebug() << "[VideoListWidget] Received video stream URL:" << videoUrl;
    
    // 创建或显示视频播放窗口
    if (!videoPlayerWindow) {
        videoPlayerWindow = new VideoPlayerWindow(nullptr);
        videoPlayerWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        
        // 连接视频播放信号，当视频开始播放时暂停音乐
        connect(videoPlayerWindow, &VideoPlayerWindow::playStateChanged, this, [this](bool isPlaying){
            if (isPlaying && m_playWidget) {
                qDebug() << "[VideoListWidget] Video started, pausing music";
                // 暂停音乐（不停止，保留状态）
                AudioService::instance().pause();
            }
        });
    }
    
    // 加载视频URL（直接传入QString）
    videoPlayerWindow->loadVideo(videoUrl);
    
    // 显示窗口
    videoPlayerWindow->show();
    videoPlayerWindow->raise();
    videoPlayerWindow->activateWindow();
    
    // 隐藏视频列表窗口（可选）
    // this->hide();
}

void VideoListWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    // 窗口显示时自动刷新
    onRefreshRequested();
}
