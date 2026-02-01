#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QPainter>
#include "VideoDecoder.h"

// 前向声明
class VideoBuffer;

/**
 * @brief 视频渲染器 - QWidget版本（预留OpenGL升级）
 * 
 * 设计思路：
 * - 当前使用QWidget + QPainter渲染
 * - 后续可替换为QOpenGLWidget
 * - 帧率控制（30/60fps）
 * - 自动缩放适应窗口
 */
class VideoRenderer : public QWidget
{
    Q_OBJECT
    
public:
    explicit VideoRenderer(QWidget* parent = nullptr);
    ~VideoRenderer() override;
    
    // ===== 渲染控制 =====
    void start();
    void pause();
    void stop();
    void startBuffering();  // 开始缓冲（seek后调用）
    void renderFrame(VideoFrame* frame);
    
    // ===== 设置 =====
    void setVideoBuffer(VideoBuffer* buffer);
    void setTargetFPS(int fps);
    
    // ===== 状态查询 =====
    bool isPlaying() const { return m_playing; }
    QSize videoSize() const { return m_videoSize; }
    qint64 lastPTS() const { return m_lastPTS; }
    
signals:
    void frameRendered(qint64 pts);
    void videoSizeChanged(QSize size);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    void renderNextFrame();
    
private:
    QRect calculateAspectRatioRect() const;
    
private:
    // 渲染状态
    bool m_playing;
    bool m_buffering;  // 缓冲中（seek后等待帧填充）
    int m_bufferingThreshold;  // 缓冲阈值（帧数）
    QImage m_currentFrame;
    QSize m_videoSize;
    qint64 m_lastPTS;
    
    // 帧率控制
    QTimer* m_renderTimer;
    int m_targetFPS;
    int m_frameInterval;  // 帧间隔（ms）
    
    // 时间同步
    qint64 m_playbackStartTime;  // 播放开始的系统时间
    qint64 m_videoPTS;           // 视频的当前PTS
    
    // 缓冲区（可选，也可以直接接收帧）
    VideoBuffer* m_buffer;
};

#endif // VIDEORENDERER_H
