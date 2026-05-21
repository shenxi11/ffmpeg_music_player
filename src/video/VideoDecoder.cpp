#include "VideoDecoder.h"
#include <QDebug>

namespace {
constexpr AVPixelFormat kNoHardwarePixelFormat = AV_PIX_FMT_NONE;
}

VideoDecoder::VideoDecoder(QObject* parent)
    : QObject(parent)
    , m_codecCtx(nullptr)
    , m_codec(nullptr)
    , m_swsCtx(nullptr)
    , m_swsSrcFormat(AV_PIX_FMT_NONE)
    , m_frame(nullptr)
    , m_yuvFrame(nullptr)
    , m_hwTransferFrame(nullptr)
    , m_hwDeviceCtx(nullptr)
    , m_hwPixelFormat(kNoHardwarePixelFormat)
    , m_decodeOptions()
    , m_hwDecodeActive(false)
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
    return init(stream, VideoDecodeOptions());
}

bool VideoDecoder::init(AVStream* stream, const VideoDecodeOptions& options)
{
    if (!stream) {
        qWarning() << "[VideoDecoder] Invalid stream";
        return false;
    }

    m_decodeOptions = options;
    
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

    initHardwareDevice(m_codec, m_codecCtx);
    
    // 打开解码器
    ret = avcodec_open2(m_codecCtx, m_codec, nullptr);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[VideoDecoder] Failed to open codec:" << errbuf;
        if (m_hwDecodeActive) {
            qWarning() << "[VideoDecoder] Retrying without hardware decode";
            releaseHardwareDevice();
            avcodec_free_context(&m_codecCtx);
            m_codecCtx = avcodec_alloc_context3(m_codec);
            if (!m_codecCtx ||
                avcodec_parameters_to_context(m_codecCtx, stream->codecpar) < 0 ||
                avcodec_open2(m_codecCtx, m_codec, nullptr) < 0) {
                cleanup();
                return false;
            }
        } else {
            cleanup();
            return false;
        }
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
             << "Time base:" << m_timeBase.num << "/" << m_timeBase.den
             << "Hardware decode:" << (m_hwDecodeActive ? "D3D11VA" : "software");
    
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
    m_hwDecodeActive = false;
    
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
        
        AVFrame* convertSource = softwareFrameForConvert(m_frame);
        if (!convertSource) {
            av_frame_unref(m_frame);
            continue;
        }

        // 转换帧格式并发送信号
        VideoFrame* videoFrame = convertFrame(convertSource);
        if (videoFrame) {
            emit frameDecoded(videoFrame);
        }
        if (convertSource == m_hwTransferFrame) {
            av_frame_unref(m_hwTransferFrame);
        }
        av_frame_unref(m_frame);
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

    if (!ensureSwsContext(srcFrame)) {
        return nullptr;
    }

    int ret = sws_scale(m_swsCtx, srcFrame->data, srcFrame->linesize, 0, srcFrame->height,
                        m_yuvFrame->data, m_yuvFrame->linesize);
    if (ret <= 0) {
        qWarning() << "[VideoDecoder] Failed to convert frame to YUV420P";
        return nullptr;
    }
    
    // 创建 VideoFrame
    VideoFrame* frame = new VideoFrame();
    frame->size = QSize(srcFrame->width, srcFrame->height);
    frame->width = srcFrame->width;
    frame->height = srcFrame->height;
    
    // 复制统一后的YUV420P原始数据（用于OpenGL渲染）。
    for (int i = 0; i < 3; i++) {
        int planeHeight = (i == 0) ? srcFrame->height : (srcFrame->height + 1) / 2;
        int planeWidth = (i == 0) ? srcFrame->width : (srcFrame->width + 1) / 2;
        QByteArray planeData;
        planeData.resize(planeWidth * planeHeight);
        if (planeData.size() != planeWidth * planeHeight) {
            qWarning() << "[VideoDecoder] Failed to allocate YUV plane" << i;
            delete frame;
            return nullptr;
        }

        for (int y = 0; y < planeHeight; y++) {
            memcpy(planeData.data() + y * planeWidth,
                   m_yuvFrame->data[i] + y * m_yuvFrame->linesize[i],
                   planeWidth);
        }
        frame->setPlane(i, std::move(planeData), planeWidth);
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

bool VideoDecoder::ensureSwsContext(AVFrame* srcFrame)
{
    if (!srcFrame || srcFrame->width <= 0 || srcFrame->height <= 0) {
        return false;
    }

    const AVPixelFormat srcFormat = static_cast<AVPixelFormat>(srcFrame->format);
    if (m_yuvFrame && m_yuvFrame->width == srcFrame->width &&
        m_yuvFrame->height == srcFrame->height && m_yuvFrame->format == AV_PIX_FMT_YUV420P &&
        m_swsCtx && m_swsSrcFormat == srcFormat) {
        return true;
    }

    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    if (m_yuvFrame) {
        av_frame_free(&m_yuvFrame);
    }

    m_swsSrcFormat = AV_PIX_FMT_NONE;
    m_swsCtx = sws_getContext(srcFrame->width, srcFrame->height, srcFormat, srcFrame->width,
                              srcFrame->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr,
                              nullptr, nullptr);
    if (!m_swsCtx) {
        qWarning() << "[VideoDecoder] Failed to create sws context for pixel format"
                   << srcFrame->format;
        return false;
    }

    m_yuvFrame = av_frame_alloc();
    if (!m_yuvFrame) {
        qWarning() << "[VideoDecoder] Failed to allocate YUV conversion frame";
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
        return false;
    }

    m_yuvFrame->format = AV_PIX_FMT_YUV420P;
    m_yuvFrame->width = srcFrame->width;
    m_yuvFrame->height = srcFrame->height;

    const int ret = av_frame_get_buffer(m_yuvFrame, 32);
    if (ret < 0) {
        qWarning() << "[VideoDecoder] Failed to allocate YUV frame buffer:" << ret;
        av_frame_free(&m_yuvFrame);
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
        return false;
    }

    qDebug() << "[VideoDecoder] Conversion target initialized:"
             << srcFrame->width << "x" << srcFrame->height
             << "srcPixelFormat:" << srcFrame->format << "dstPixelFormat:YUV420P";
    m_swsSrcFormat = srcFormat;
    return true;
}

bool VideoDecoder::initHardwareDevice(const AVCodec* codec, AVCodecContext* codecCtx)
{
    releaseHardwareDevice();

    if (!m_decodeOptions.enableHardwareDecode || !codec || !codecCtx) {
        return false;
    }

    for (int i = 0;; ++i) {
        const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
        if (!config) {
            break;
        }

        if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
            config->device_type == m_decodeOptions.preferredHardwareDevice) {
            m_hwPixelFormat = config->pix_fmt;
            break;
        }
    }

    if (m_hwPixelFormat == kNoHardwarePixelFormat) {
        qDebug() << "[VideoDecoder] D3D11VA config unavailable for codec" << codec->name;
        return false;
    }

    const int ret =
        av_hwdevice_ctx_create(&m_hwDeviceCtx, m_decodeOptions.preferredHardwareDevice, nullptr,
                               nullptr, 0);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[VideoDecoder] Failed to create D3D11VA device:" << errbuf;
        releaseHardwareDevice();
        return false;
    }

    codecCtx->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);
    if (!codecCtx->hw_device_ctx) {
        qWarning() << "[VideoDecoder] Failed to reference D3D11VA device";
        releaseHardwareDevice();
        return false;
    }

    codecCtx->opaque = this;
    codecCtx->get_format = &VideoDecoder::chooseHardwareFormat;
    m_hwDecodeActive = true;
    qDebug() << "[VideoDecoder] D3D11VA hardware decode enabled";
    return true;
}

void VideoDecoder::releaseHardwareDevice()
{
    if (m_codecCtx) {
        m_codecCtx->get_format = nullptr;
        m_codecCtx->opaque = nullptr;
        av_buffer_unref(&m_codecCtx->hw_device_ctx);
    }
    av_buffer_unref(&m_hwDeviceCtx);
    m_hwPixelFormat = kNoHardwarePixelFormat;
    m_hwDecodeActive = false;
}

AVFrame* VideoDecoder::softwareFrameForConvert(AVFrame* srcFrame)
{
    if (!srcFrame) {
        return nullptr;
    }

    if (!m_hwDecodeActive || srcFrame->format != m_hwPixelFormat) {
        return srcFrame;
    }

    if (!m_hwTransferFrame) {
        m_hwTransferFrame = av_frame_alloc();
        if (!m_hwTransferFrame) {
            qWarning() << "[VideoDecoder] Failed to allocate hardware transfer frame";
            return nullptr;
        }
    }

    av_frame_unref(m_hwTransferFrame);
    const int ret = av_hwframe_transfer_data(m_hwTransferFrame, srcFrame, 0);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[VideoDecoder] Failed to transfer D3D11VA frame:" << errbuf;
        return nullptr;
    }

    m_hwTransferFrame->pts = srcFrame->pts;
    m_hwTransferFrame->pkt_duration = srcFrame->pkt_duration;
    m_hwTransferFrame->best_effort_timestamp = srcFrame->best_effort_timestamp;
    m_hwTransferFrame->key_frame = srcFrame->key_frame;
    return m_hwTransferFrame;
}

AVPixelFormat VideoDecoder::chooseHardwareFormat(AVCodecContext* ctx, const AVPixelFormat* formats)
{
    auto* self = static_cast<VideoDecoder*>(ctx ? ctx->opaque : nullptr);
    const AVPixelFormat preferred = self ? self->m_hwPixelFormat : kNoHardwarePixelFormat;

    for (const AVPixelFormat* fmt = formats; fmt && *fmt != AV_PIX_FMT_NONE; ++fmt) {
        if (*fmt == preferred) {
            return *fmt;
        }
    }

    qWarning() << "[VideoDecoder] D3D11VA pixel format not offered, falling back to software";
    if (self) {
        self->m_hwDecodeActive = false;
    }
    return avcodec_default_get_format(ctx, formats);
}

void VideoDecoder::cleanup()
{
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_yuvFrame) {
        av_frame_free(&m_yuvFrame);
        m_yuvFrame = nullptr;
    }

    if (m_hwTransferFrame) {
        av_frame_free(&m_hwTransferFrame);
        m_hwTransferFrame = nullptr;
    }
    
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    m_swsSrcFormat = AV_PIX_FMT_NONE;
    
    releaseHardwareDevice();

    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    
    m_codec = nullptr;
    m_running = false;
}
