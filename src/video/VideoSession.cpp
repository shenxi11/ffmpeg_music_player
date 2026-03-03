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
    , m_sampleAspectRatio({1, 1})
{
    qDebug() << "[VideoSession] Created";
    
    m_buffer = new VideoBuffer();
    
    m_decoder = new VideoDecoder(this);
    
    connect(m_decoder, &VideoDecoder::frameDecoded, 
            this, &VideoSession::onFrameDecoded);
    connect(m_decoder, &VideoDecoder::decodingError, 
            this, &VideoSession::decodingError);
    
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
    
    if (m_renderer && m_buffer) {
        QMetaObject::invokeMethod(m_renderer, "setVideoBuffer", Q_ARG(VideoBuffer*, m_buffer));
        
        if (m_stream) {
            AVRational frameRate = av_guess_frame_rate(nullptr, m_stream, nullptr);
            if (frameRate.num > 0 && frameRate.den > 0) {
                int fps = (int)((double)frameRate.num / frameRate.den + 0.5);
                QMetaObject::invokeMethod(m_renderer, "setTargetFPS", Q_ARG(int, fps));
                qDebug() << "[VideoSession] Set renderer FPS to" << fps << "(" << frameRate.num << "/" << frameRate.den << ")";
            }

            QMetaObject::invokeMethod(m_renderer, "setPixelAspectRatio",
                                      Q_ARG(int, m_sampleAspectRatio.num),
                                      Q_ARG(int, m_sampleAspectRatio.den));
            qDebug() << "[VideoSession] Set renderer PAR to"
                     << m_sampleAspectRatio.num << "/" << m_sampleAspectRatio.den;
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

    // Prefer stream SAR, fallback to codec parameters SAR.
    m_sampleAspectRatio = stream->sample_aspect_ratio;
    if (m_sampleAspectRatio.num <= 0 || m_sampleAspectRatio.den <= 0) {
        m_sampleAspectRatio = stream->codecpar->sample_aspect_ratio;
    }
    if (m_sampleAspectRatio.num <= 0 || m_sampleAspectRatio.den <= 0) {
        m_sampleAspectRatio = AVRational{1, 1};
    }

    // Video coded size from stream metadata.
    m_videoSize = QSize(stream->codecpar->width, stream->codecpar->height);
    emit videoSizeChanged(m_videoSize);

    qDebug() << "[VideoSession] Initialized - Size:" << m_videoSize
             << "Codec:" << avcodec_get_name(stream->codecpar->codec_id)
             << "SAR:" << m_sampleAspectRatio.num << "/" << m_sampleAspectRatio.den;
    
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
    
    if (m_decoder) {
        m_decoder->start();
    }
    
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "start");
    }
}

void VideoSession::pause()
{
    qDebug() << "[VideoSession] Pause";
    
    m_running = false;
    
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "pause");
    }
}

void VideoSession::stop()
{
    qDebug() << "[VideoSession] Stop";
    
    m_running = false;
    
    if (m_decoder) {
        m_decoder->stop();
    }
    
    if (m_renderer) {
        QMetaObject::invokeMethod(m_renderer, "stop");
    }
    
    if (m_buffer) {
        m_buffer->clear();
    }
}

void VideoSession::seekTo(qint64 positionMs)
{
    qDebug() << "[VideoSession] Seek to:" << positionMs << "ms";
    
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
    // Avoid dropping startup keyframes when play/pause state and demux start
    // transition concurrently.
    if (!m_decoder) {
        av_packet_free(&packet);
        return;
    }
    
    m_decoder->decodePacket(packet);
}

qint64 VideoSession::getCurrentPTS() const
{
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
    
    m_currentPTS = frame->pts;
    
    if (m_buffer) {
        m_buffer->push(frame);
        
        if (m_buffer->isFull()) {
            emit bufferFull();
        }
    }
}
