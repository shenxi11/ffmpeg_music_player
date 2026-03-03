#include "VideoRenderer.h"
#include "VideoBuffer.h"
#include "AudioPlayer.h"
#include <QPaintEvent>
#include <QDebug>
#include <QDateTime>

VideoRenderer::VideoRenderer(QWidget* parent)
    : QWidget(parent)
    , m_playing(false)
    , m_buffering(false)
    , m_bufferingThreshold(15)  // seek后等待15帧再播放
    , m_lastPTS(0)
    , m_renderTimer(nullptr)
    , m_targetFPS(30)
    , m_frameInterval(33)  // ~30fps
    , m_buffer(nullptr)
{
    qDebug() << "[VideoRenderer] Created";
    
    // 设置背景色
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(0, 0, 0));
    setAutoFillBackground(true);
    setPalette(palette);
    
    // 创建渲染定时器
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &VideoRenderer::renderNextFrame);
    
    setMinimumSize(320, 180);
}

VideoRenderer::~VideoRenderer()
{
    qDebug() << "[VideoRenderer] Destroying...";
    
    stop();
    
    if (m_renderTimer) {
        delete m_renderTimer;
        m_renderTimer = nullptr;
    }
    
    qDebug() << "[VideoRenderer] Destroyed";
}

void VideoRenderer::start()
{
    qDebug() << "[VideoRenderer] Start";
    
    m_playing = true;
    m_playbackStartTime = QDateTime::currentMSecsSinceEpoch();
    m_videoPTS = 0;
    m_renderTimer->start(m_frameInterval);
}

void VideoRenderer::pause()
{
    qDebug() << "[VideoRenderer] Pause";
    
    m_playing = false;
    m_buffering = false;
    m_renderTimer->stop();
}

void VideoRenderer::startBuffering()
{
    qDebug() << "[VideoRenderer] Start buffering (waiting for" << m_bufferingThreshold << "frames)";
    m_buffering = true;
}

void VideoRenderer::stop()
{
    qDebug() << "[VideoRenderer] Stop";
    
    m_playing = false;
    m_renderTimer->stop();
    
    // 清空当前帧
    m_currentFrame = QImage();
    m_lastPTS = 0;
    m_playbackStartTime = 0;
    m_videoPTS = 0;
    
    update();
}

void VideoRenderer::renderFrame(VideoFrame* frame)
{
    if (!frame) {
        return;
    }
    
    // 注意：VideoFrame已移除QImage，此软件渲染器已不可用
    // 现在统一使用VideoRendererGL进行OpenGL硬件渲染
    // m_currentFrame = frame->image;
    m_lastPTS = frame->pts;
    
    // 更新视频尺寸
    if (m_videoSize != frame->size) {
        m_videoSize = frame->size;
        emit videoSizeChanged(m_videoSize);
    }
    
    // 触发重绘
    // update();
    
    // 发送渲染完成信号
    emit frameRendered(frame->pts);
    
    // 释放帧
    delete frame;
}

void VideoRenderer::setVideoBuffer(VideoBuffer* buffer)
{
    m_buffer = buffer;
}

void VideoRenderer::setTargetFPS(int fps)
{
    m_targetFPS = fps;
    m_frameInterval = 1000 / fps;
    
    if (m_renderTimer->isActive()) {
        m_renderTimer->setInterval(m_frameInterval);
    }
    
    qDebug() << "[VideoRenderer] Target FPS set to:" << fps 
             << "Interval:" << m_frameInterval << "ms";
}

void VideoRenderer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // 绘制背景
    painter.fillRect(rect(), Qt::black);
    
    // 如果有当前帧，绘制视频
    if (!m_currentFrame.isNull()) {
        QRect targetRect = calculateAspectRatioRect();
        painter.drawImage(targetRect, m_currentFrame);
    } else {
        // 绘制占位文字
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "无视频帧");
    }
}

void VideoRenderer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void VideoRenderer::renderNextFrame()
{
    if (!m_playing || !m_buffer) {
        return;
    }
    
    // 检查缓冲状态：seek后等待缓冲区填充
    if (m_buffering) {
        int bufferSize = m_buffer->size();
        if (bufferSize >= m_bufferingThreshold) {
            m_buffering = false;
            qDebug() << "[VideoRenderer] Buffering complete, buffer size:" << bufferSize;
        } else {
            // 继续等待缓冲
            return;
        }
    }
    
    // 使用音频时钟作为主时钟（音视频同步）
    qint64 audioClock = AudioPlayer::instance().getPlaybackPosition();
    
    // 从缓冲区查看下一帧
    VideoFrame* frame = m_buffer->peek();
    if (!frame) {
        return;  // 缓冲区空，等待更多帧
    }
    
    // 调试：定期打印时间信息
    static int debugCounter = 0;
    static qint64 lastRenderTime = 0;
    if (debugCounter++ % 30 == 0) {  // 每30帧打印一次
        qint64 diff = frame->pts - audioClock;
        qDebug() << "[VideoRenderer] audioClock:" << audioClock 
                 << "frame->pts:" << frame->pts 
                 << "diff:" << diff
                 << "buffer_size:" << m_buffer->size();
    }
    
    // 检查帧的PTS是否到达显示时间（允许40ms误差）
    if (frame->pts > audioClock + 40) {
        // 帧还没到显示时间，等待
        return;
    }
    
    // 如果视频落后太多（超过100ms），跳帧
    if (audioClock > frame->pts + 100) {
        qDebug() << "[VideoRenderer] Skip frame - video lagging, pts:" << frame->pts << "audio:" << audioClock;
        frame = m_buffer->pop();
        delete frame;
        return;
    }
    
    // 从缓冲区取出并渲染
    frame = m_buffer->pop();
    if (frame) {
        lastRenderTime = audioClock;
        renderFrame(frame);
    }
}

QRect VideoRenderer::calculateAspectRatioRect() const
{
    if (m_currentFrame.isNull()) {
        return rect();
    }
    
    QSize imageSize = m_currentFrame.size();
    QSize widgetSize = size();
    
    // 计算缩放比例（保持宽高比）
    float imageAspect = (float)imageSize.width() / imageSize.height();
    float widgetAspect = (float)widgetSize.width() / widgetSize.height();
    
    int targetWidth, targetHeight;
    
    if (imageAspect > widgetAspect) {
        // 图像更宽，以宽度为准
        targetWidth = widgetSize.width();
        targetHeight = targetWidth / imageAspect;
    } else {
        // 图像更高，以高度为准
        targetHeight = widgetSize.height();
        targetWidth = targetHeight * imageAspect;
    }
    
    // 居中显示
    int x = (widgetSize.width() - targetWidth) / 2;
    int y = (widgetSize.height() - targetHeight) / 2;
    
    return QRect(x, y, targetWidth, targetHeight);
}
