#ifndef VIDEO_LIST_WIDGET_H
#define VIDEO_LIST_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QDebug>
#include "video_list_widget_qml.h"
#include "httprequest.h"
#include "VideoPlayerWindow.h"
#include "play_widget.h"  // 添加PlayWidget头文件
#include "AudioService.h"  // 添加AudioService头文件

/**
 * @brief 在线视频列表窗口
 * 显示服务器上的视频列表，点击视频后获取播放URL并打开播放窗口
 */
class VideoListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoListWidget(PlayWidget* playWidget, QWidget *parent = nullptr)
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
    
    ~VideoListWidget() {
        qDebug() << "[VideoListWidget] Destroyed";
    }

signals:
    void signal_open_video_player(const QString& videoUrl, const QString& videoName);

private slots:
    /**
     * @brief 刷新按钮点击处理
     */
    void onRefreshRequested() {
        qDebug() << "[VideoListWidget] Refresh requested, fetching video list...";
        listWidget->clearAll();
        request->getVideoList();
    }
    
    /**
     * @brief 接收到视频列表
     */
    void onVideoListReceived(const QVariantList& videoList) {
        qDebug() << "[VideoListWidget] Received" << videoList.size() << "videos";
        listWidget->clearAll();
        listWidget->addVideoList(videoList);
    }
    
    /**
     * @brief 视频被选中
     */
    void onVideoSelected(const QString& videoPath, const QString& videoName) {
        qDebug() << "[VideoListWidget] Video selected:" << videoName << "(" << videoPath << ")";
        
        // 保存选中的视频信息
        m_selectedVideoName = videoName;
        
        // 请求视频流URL
        request->getVideoStreamUrl(videoPath);
    }
    
    /**
     * @brief 接收到视频流URL
     */
    void onVideoStreamUrlReceived(const QString& videoUrl) {
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

protected:
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);
        // 窗口显示时自动刷新
        onRefreshRequested();
    }

private:
    VideoListWidgetQml* listWidget;
    HttpRequest* request;
    VideoPlayerWindow* videoPlayerWindow = nullptr;
    QString m_selectedVideoName;
    PlayWidget* m_playWidget;  // 音乐播放器引用
};

#endif // VIDEO_LIST_WIDGET_H
