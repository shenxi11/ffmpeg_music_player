#ifndef VIDEO_LIST_WIDGET_H
#define VIDEO_LIST_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QDebug>
#include "video_list_widget_qml.h"
#include "httprequest_v2.h"
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
    explicit VideoListWidget(PlayWidget* playWidget, QWidget *parent = nullptr);
    ~VideoListWidget();
    VideoPlayerWindow* playerWindow() const { return videoPlayerWindow; }

signals:
    void signal_open_video_player(const QString& videoUrl, const QString& videoName);
    void videoPlayerWindowReady(VideoPlayerWindow* window);
    void videoPlaybackStateChanged(bool isPlaying);

public slots:
    void pauseVideoPlayback();

private slots:
    /**
     * @brief 刷新按钮点击处理
     */
    void onRefreshRequested();
    
    /**
     * @brief 接收到视频列表
     */
    void onVideoListReceived(const QVariantList& videoList);
    
    /**
     * @brief 视频被选中
     */
    void onVideoSelected(const QString& videoPath, const QString& videoName);
    
    /**
     * @brief 接收到视频流URL
     */
    void onVideoStreamUrlReceived(const QString& videoUrl);

protected:
    void showEvent(QShowEvent* event) override;

private:
    VideoListWidgetQml* listWidget;
    HttpRequestV2* request;
    VideoPlayerWindow* videoPlayerWindow = nullptr;
    QString m_selectedVideoName;
    PlayWidget* m_playWidget;  // 音乐播放器引用
};

#endif // VIDEO_LIST_WIDGET_H
