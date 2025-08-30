#include "movie_decoder.h"

MovieDecoder::MovieDecoder(QObject *parent) : QObject(parent) {
    avformat_network_init();
//    moveToThread(&m_decodeThread);
//    connect(&m_decodeThread, &QThread::finished, this, &MovieDecoder::deleteLater);
//    m_decodeThread.start();
}

MovieDecoder::~MovieDecoder() {
    /*
    m_decodeThread.quit();
    m_decodeThread.wait();*/
}
void MovieDecoder::startDecode(QString url){
    open(url);
    start();
}
bool MovieDecoder::open(const QString &url) {
    QMutexLocker locker(&m_mutex);
    cleanup();

    // 打开输入文件
    if (avformat_open_input(&m_fmtCtx, url.toUtf8().constData(), nullptr, nullptr) != 0) {
        emit errorOccurred(tr("无法打开输入文件"));
        return false;
    }

    // 获取流信息
    if (avformat_find_stream_info(m_fmtCtx, nullptr) < 0) {
        emit errorOccurred(tr("无法获取流信息"));
        return false;
    }

    // 查找视频流
    m_videoStream = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStream < 0) {
        emit errorOccurred(tr("未找到视频流"));
        return false;
    }

    // 初始化解码器
    AVCodecParameters *codecPar = m_fmtCtx->streams[m_videoStream]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit errorOccurred(tr("找不到解码器"));
        return false;
    }

    // 创建解码上下文
    m_codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(m_codecCtx, codecPar) < 0) {
        emit errorOccurred(tr("无法初始化解码上下文"));
        return false;
    }

    // 硬件加速初始化
    if (!initHWAccel()) {
        qWarning() << "硬件加速初始化失败，回退到软件解码";
    }

    // 打开解码器
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        emit errorOccurred(tr("无法打开解码器"));
        return false;
    }

    // 获取视频信息
    m_videoSize.setWidth(m_codecCtx->width);
    m_videoSize.setHeight(m_codecCtx->height);
    m_frameRate = av_q2d(m_fmtCtx->streams[m_videoStream]->avg_frame_rate);
    emit signal_resize(m_codecCtx->width, m_codecCtx->height);
    return true;
}

void MovieDecoder::start() {
    if (!m_fmtCtx || !m_codecCtx) return;
    m_paused = false;
    if (!m_running) {
        m_running = true;
        QMetaObject::invokeMethod(this, "decodeLoop", Qt::QueuedConnection);
    }
}

bool MovieDecoder::isRunning() const { return m_running; }

QSize MovieDecoder::videoSize() const { return m_videoSize; }

double MovieDecoder::frameRate() const { return m_frameRate; }

void MovieDecoder::decodeLoop() {
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    while (m_running) {
        if (m_paused) {
            QThread::msleep(10);
            continue;
        }

        // 读取数据包
        int ret = av_read_frame(m_fmtCtx, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                emit errorOccurred(tr("视频播放结束"));
                break;
            }
            continue;
        }

        if (pkt->stream_index == m_videoStream) {
            if (avcodec_send_packet(m_codecCtx, pkt) < 0) {
                av_packet_unref(pkt);
                continue;
            }

            while (avcodec_receive_frame(m_codecCtx, frame) == 0) {
                // 创建新 AVFrame 并增加引用计数
                AVFrame* new_frame = av_frame_alloc();
                av_frame_ref(new_frame, frame);

                // 发送信号（注意跨线程需队列连接）
                emit signal_frameDecoded(new_frame);
            }
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_frame_free(&frame);
    m_running = false;
}
QImage MovieDecoder::convertAVFrameToQImage(AVFrame *frame) {

    QImage img(frame->width, frame->height, QImage::Format_RGB888);
    for (int y = 0; y < frame->height; y++) {
        memcpy(img.scanLine(y),
               frame->data[0] + y * frame->linesize[0],
               frame->width * 3);
    }
   return img;
}
bool MovieDecoder::initHWAccel() {
#if defined(Q_OS_WIN)
    const AVHWDeviceType type = AV_HWDEVICE_TYPE_D3D11VA;
#elif defined(Q_OS_LINUX)
    const AVHWDeviceType type = AV_HWDEVICE_TYPE_VAAPI;
#else
    const AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
#endif

    if (type != AV_HWDEVICE_TYPE_NONE) {
        if (av_hwdevice_ctx_create(&m_hwDeviceCtx, type, nullptr, nullptr, 0) == 0) {
            m_codecCtx->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);
            return true;
        }
    }
    return false;
}

void MovieDecoder::cleanup() {
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
    if (m_fmtCtx) {
        avformat_close_input(&m_fmtCtx);
    }
    if (m_hwDeviceCtx) {
        av_buffer_unref(&m_hwDeviceCtx);
    }
}
