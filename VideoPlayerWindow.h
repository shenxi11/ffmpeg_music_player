#ifndef VIDEOPLAYERWINDOW_H
#define VIDEOPLAYERWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QTime>
#include <QTimer>
#include "MediaService.h"
#include "MediaSession.h"
#include "VideoRendererGL.h"

// 视频渲染区域占位控件（后续替换为 QOpenGLWidget）
class VideoRenderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoRenderWidget(QWidget *parent = nullptr);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    
private:
    QString m_placeholderText;
};

// 视频播放窗口
class VideoPlayerWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit VideoPlayerWindow(QWidget *parent = nullptr);
    ~VideoPlayerWindow();
    
    // 加载视频文件
    void loadVideo(const QString& filePath);
    
    // 暂停播放（用于外部控制）
    void pausePlayback();
    
signals:
    // 播放状态改变信号
    void playStateChanged(bool isPlaying);
    // 进度改变信号
    void progressChanged(qint64 position);
    // 视频文件加载信号
    void videoLoaded(const QString& filePath);
    
public slots:
    void onPlayPauseClicked();
    void onOpenFileClicked();
    void onSliderPressed();
    void onSliderReleased();
    void onSliderValueChanged(int value);
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackFinished();
    
private:
    void setupUI();
    void updateTimeLabel(qint64 currentMs, qint64 totalMs);
    QString formatTime(qint64 ms);
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
private:
    // UI 控件
    VideoRendererGL* m_renderWidget;        // 视频渲染区域（OpenGL硬件加速）
    QPushButton* m_playPauseBtn;            // 播放/暂停按钮
    QPushButton* m_openFileBtn;             // 打开文件按钮
    QSlider* m_progressSlider;              // 进度条
    QLabel* m_timeLabel;                    // 时间显示
    QLabel* m_fileNameLabel;                // 文件名显示
    
    // 媒体服务
    MediaSession* m_mediaSession;           // 媒体会话
    
    // 播放状态
    bool m_isPlaying;                       // 是否正在播放
    bool m_sliderPressed;                   // 进度条是否被按下
    QString m_currentFilePath;              // 当前视频文件路径
    qint64 m_duration;                      // 视频总时长(ms)
    qint64 m_currentPosition;               // 当前播放位置(ms)
};

#endif // VIDEOPLAYERWINDOW_H
