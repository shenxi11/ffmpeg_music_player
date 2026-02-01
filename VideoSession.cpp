#include "VideoSession.h"
#include "VideoDecoder.h"
#include "VideoRenderer.h"
#include "VideoBuffer.h"
#include <QDebug>

VideoSession::VideoSession(QObject* parent)
    : QObject(parent)
    , m_decoder(nullptr)
    , m_renderer(nullptr)
    , m_buffer(nullptr)
    , m_decodeThread(nullptr)
    , m_running(false)
    , m_holdFrame(false)
    , m_currentPTS(0)
    , m_stream(nullptr)
{
    qDebug() << "[VideoSession] Created";
    
    // 创建视频缓冲区（不设置parent，手动管理生命周期）
    m_buffer = new VideoBuffer();
    
    // 创建视频解码器
    m_decoder = new VideoDecoder(this);
    
    // 连接解码器信号
    connect(m_decoder, &VideoDecoder::frameDecoded, 
            this, &VideoSession::onFrameDecoded);
    connect(m_decoder, &VideoDecoder::decodingError, 
            this, &VideoSession::decodingError);
    
    // 渲染器由外部设置，不在这里创建
    // m_renderer = new VideoRenderer();
}

VideoSession::~VideoSession()
{
    qDebug() << "[VideoSession] Destroying...";
    
    stop();
    
    if (m_decoder) {
        delete m_decoder;
        m_decoder = nullptr;
    }
    
    // 渲染器由外部管理，不在这里删除
    // if (m_renderer) {
    //     delete m_renderer;
    //     m_renderer = nullptr;
    // }
    
    if (m_buffer) {
        delete m_buffer;
        m_buffer = nullptr;
    }
    
    qDebug() << "[VideoSession] Destroyed";
}

void VideoSession::setVideoRenderer(QWidget* renderer)
{
    m_renderer = renderer;
    
    // 设置 VideoBuffer 给渲染器（通过动态调用）
    if (m_renderer && m_buffer) {
        // 尝试调用setVideoBuffer方法（VideoRenderer和VideoRendererGL都有此方法）
        QMetaObject::invokeMethod(m_renderer, "setVideoBuffer", Q_ARG(VideoBuffer*, m_buffer));
        
        // 根据视频流的帧率设置渲染器FPS
        if (m_stream) {
            AVRational frameRate = av_guess_frame_rate(nullptr, m_stream, nullptr);
            if (frameRate.num > 0 && frameRate.den > 0) {
                // 使用四舍五入获取正确的FPS（59.94 -> 60）
                int fps = (int)((double)frameRate.num / frameRate.den + 0.5);
                QMetaObject::invokeMethod(m_renderer, "setTargetFPS", Q_ARG(int, fps));
                qDebug() << "[VideoSession] Set renderer FPS to" << fps << "(" << frameRate.num << "/" << frameRate.den << ")";
            }
        }
        
        qDebug() << "[VideoSession] VideoRenderer set and connected to buffer";
    }
}

bool VideoSession::initVideoStream(AVStream* stream)
{
    if (!stream) {
        qWarning() << "[VideoSession] Invalid stream";
        return false;
    }
    
    m_stream = stream;
    
    // 获取视频尺寸
    m_videoSize = QSize(stream->codecpar->width, stream->codecpar->height);
    emit videoSizeChanged(m_videoSize);
    
    qDebug() << "[VideoSession] Initialized - Size:" << m_videoSize
             << "Codec:" << avcodec_get_name(stream->codecpar->codec_id);
    
    // 初始化解码器
    if (!m_decoder->init(stream)) {
        qWarning() << "[VideoSession] Failed to init decoder";
        return false;
    }
    
    return true;
}

void VideoSession::start()
{
    qDebug() << "[VideoSession] Start";
    
    m_running = true;
    m_holdFrame = false;
    
    // 启动解码器
    if (m_decoder) {
        m_decoder->start();
    }
    
    // 启动渲染器
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "start");
    }
}

void VideoSession::pause()
{
    qDebug() << "[VideoSession] Pause";
    
    m_running = false;
    
    // 暂停渲染器
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "pause");
    }
}

void VideoSession::stop()
{
    qDebug() << "[VideoSession] Stop";
    
    m_running = false;
    
    // 停止解码器
    if (m_decoder) {
        m_decoder->stop();
    }
    
    // 停止渲染器
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "stop");
    }
    
    // 清空缓冲区
    if (m_buffer) {
        m_buffer->clear();
    }
}

void VideoSession::seekTo(qint64 positionMs)
{
    qDebug() << "[VideoSession] Seek to:" << positionMs << "ms";
    
    // 清空缓冲区
    flush();
    
    m_currentPTS = positionMs;
}

void VideoSession::flush()
{
    qDebug() << "[VideoSession] Flush buffers";
    
    if (m_decoder) {
        m_decoder->flush();
    }
    
    if (m_buffer) {
        m_buffer->clear();
    }
}

void VideoSession::pushPacket(AVPacket* packet)
{
    if (!m_decoder || !m_running) {
        av_packet_free(&packet);  // 如果不解码，需要释放克隆的包
        return;
    }
    
    // 传递给解码器（解码器会负责释放packet）
    m_decoder->decodePacket(packet);
}

qint64 VideoSession::getCurrentPTS() const
{
    // 从渲染器获取最后渲染帧的PTS
    if (m_renderer) {
        qint64 pts = 0;
        QMetaObject::invokeMethod(m_renderer, "lastPTS", Qt::DirectConnection, Q_RETURN_ARG(qint64, pts));
        // qDebug() << "[VideoSession] getCurrentPTS from renderer:" << pts;
        return pts;
    }
    
    return m_currentPTS;
}

void VideoSession::holdFrame()
{
    qDebug() << "[VideoSession] Hold frame (sync wait)";
    m_holdFrame = true;
    
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "pause", Qt::QueuedConnection);
    }
}

void VideoSession::skipNonKeyFrames()
{
    qDebug() << "[VideoSession] Skip non-key frames";
    
    if (m_buffer) {
        m_buffer->clearNonKeyFrames();
    }
}

void VideoSession::resumeRendering()
{
    qDebug() << "[VideoSession] Resume rendering";
    m_holdFrame = false;
    
    if (m_renderer && m_running) {
        QMetaObject::invokeMethod(m_renderer, "start", Qt::QueuedConnection);
    }
}

void VideoSession::onFrameDecoded(VideoFrame* frame)
{
    if (!frame) {
        return;
    }
    
    // 更新当前PTS
    m_currentPTS = frame->pts;
    
    // 添加到缓冲区
    if (m_buffer) {
        m_buffer->push(frame);
        
        // 检查缓冲区状态
        if (m_buffer->isFull()) {
            emit bufferFull();
        }
    }
}
