#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(QObject* parent)
    : QObject(parent)
    , m_codecCtx(nullptr)
    , m_codec(nullptr)
    , m_swsCtx(nullptr)
    , m_frame(nullptr)
    , m_running(false)
{
    qDebug() << "[VideoDecoder] Created";
}

VideoDecoder::~VideoDecoder()
{
    qDebug() << "[VideoDecoder] Destroying...";
    cleanup();
    qDebug() << "[VideoDecoder] Destroyed";
}

bool VideoDecoder::init(AVStream* stream)
{
    if (!stream) {
        qWarning() << "[VideoDecoder] Invalid stream";
        return false;
    }
    
    // 查找解码器
    m_codec = const_cast<AVCodec*>(avcodec_find_decoder(stream->codecpar->codec_id));
    if (!m_codec) {
        qWarning() << "[VideoDecoder] Codec not found";
        return false;
    }
    
    // 创建解码器上下文
    m_codecCtx = avcodec_alloc_context3(m_codec);
    if (!m_codecCtx) {
        qWarning() << "[VideoDecoder] Failed to allocate codec context";
        return false;
    }
    
    // 复制参数
    int ret = avcodec_parameters_to_context(m_codecCtx, stream->codecpar);
    if (ret < 0) {
        qWarning() << "[VideoDecoder] Failed to copy codec parameters";
        cleanup();
        return false;
    }
    
    // 打开解码器
    ret = avcodec_open2(m_codecCtx, m_codec, nullptr);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[VideoDecoder] Failed to open codec:" << errbuf;
        cleanup();
        return false;
    }
    
    // 分配帧
    m_frame = av_frame_alloc();
    if (!m_frame) {
        qWarning() << "[VideoDecoder] Failed to allocate frame";
        cleanup();
        return false;
    }
    
    // 记录视频尺寸和时间基准
    m_videoSize = QSize(m_codecCtx->width, m_codecCtx->height);
    m_timeBase = stream->time_base;
    
    m_running = true;
    
    qDebug() << "[VideoDecoder] Initialized - Codec:" << m_codec->name
             << "Size:" << m_videoSize
             << "Time base:" << m_timeBase.num << "/" << m_timeBase.den;
    
    return true;
}

bool VideoDecoder::init(AVCodecContext* codecCtx)
{
    if (!codecCtx) {
        qWarning() << "[VideoDecoder] Invalid codec context";
        return false;
    }
    
    m_codecCtx = codecCtx;
    m_codec = const_cast<AVCodec*>(m_codecCtx->codec);
    
    // 分配帧
    m_frame = av_frame_alloc();
    if (!m_frame) {
        qWarning() << "[VideoDecoder] Failed to allocate frame";
        return false;
    }
    
    m_videoSize = QSize(m_codecCtx->width, m_codecCtx->height);
    m_running = true;
    
    qDebug() << "[VideoDecoder] Initialized with existing context - Size:" << m_videoSize;
    
    return true;
}

void VideoDecoder::decodePacket(AVPacket* packet)
{
    if (!m_running || !m_codecCtx || !packet) {
        if (packet) {
            av_packet_free(&packet);
        }
        return;
    }
    
    // 发送packet到解码器
    int ret = avcodec_send_packet(m_codecCtx, packet);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[VideoDecoder] Failed to send packet:" << errbuf;
        emit decodingError(QString("Send packet failed: %1").arg(errbuf));
        av_packet_free(&packet);
        return;
    }
    
    // 接收解码后的帧
    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // 需要更多数据或已到结尾
            break;
        }
        
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qWarning() << "[VideoDecoder] Failed to receive frame:" << errbuf;
            emit decodingError(QString("Receive frame failed: %1").arg(errbuf));
            break;
        }
        
        // 转换帧格式并发送信号
        VideoFrame* videoFrame = convertFrame(m_frame);
        if (videoFrame) {
            emit frameDecoded(videoFrame);
        }
    }
    
    av_packet_free(&packet);
}

void VideoDecoder::flush()
{
    qDebug() << "[VideoDecoder] Flush";
    
    if (m_codecCtx) {
        avcodec_flush_buffers(m_codecCtx);
    }
}

void VideoDecoder::start()
{
    qDebug() << "[VideoDecoder] Start";
    m_running = true;
}

void VideoDecoder::stop()
{
    qDebug() << "[VideoDecoder] Stop";
    m_running = false;
}

VideoFrame* VideoDecoder::convertFrame(AVFrame* srcFrame)
{
    if (!srcFrame) {
        return nullptr;
    }
    
    // 创建 sws 上下文（如果还没创建）
    if (!m_swsCtx) {
        m_swsCtx = sws_getContext(
            srcFrame->width, srcFrame->height,
            (AVPixelFormat)srcFrame->format,
            srcFrame->width, srcFrame->height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
        
        if (!m_swsCtx) {
            qWarning() << "[VideoDecoder] Failed to create sws context";
            return nullptr;
        }
    }
    
    // 创建 VideoFrame
    VideoFrame* frame = new VideoFrame();
    frame->size = QSize(srcFrame->width, srcFrame->height);
    frame->width = srcFrame->width;
    frame->height = srcFrame->height;
    
    // 复制YUV原始数据（用于OpenGL渲染）
    for (int i = 0; i < 3; i++) {
        int planeHeight = (i == 0) ? srcFrame->height : srcFrame->height / 2;
        int planeWidth = (i == 0) ? srcFrame->width : srcFrame->width / 2;
        frame->linesize[i] = planeWidth;
        
        // 分配内存并复制数据
        frame->data[i] = new uint8_t[planeWidth * planeHeight];
        for (int y = 0; y < planeHeight; y++) {
            memcpy(frame->data[i] + y * planeWidth,
                   srcFrame->data[i] + y * srcFrame->linesize[i],
                   planeWidth);
        }
    }
    
    // 计算时间戳（转换为毫秒）
    if (srcFrame->pts != AV_NOPTS_VALUE) {
        frame->pts = srcFrame->pts * av_q2d(m_timeBase) * 1000;
    } else {
        frame->pts = 0;
    }
    
    // 计算帧持续时间
    if (srcFrame->pkt_duration > 0) {
        frame->duration = srcFrame->pkt_duration * av_q2d(m_timeBase) * 1000;
    } else {
        // 默认按30fps计算
        frame->duration = 33;  // ~30fps
    }
    
    // 是否关键帧
    frame->isKeyFrame = srcFrame->key_frame;
    
    return frame;
}

void VideoDecoder::cleanup()
{
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    
    m_codec = nullptr;
    m_running = false;
}
