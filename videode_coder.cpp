#include "videode_coder.h"
#include <QDebug>
#include <QDateTime>

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject{parent}
    , formatCtx(nullptr)
    , videoCodecCtx(nullptr)
    , audioCodecCtx(nullptr)
    , videoFrame(nullptr)
    , rgbFrame(nullptr)
    , audioFrame(nullptr)
    , packet(nullptr)
    , swsCtx(nullptr)
    , swrCtx(nullptr)
    , videoStreamIndex(-1)
    , audioStreamIndex(-1)
    , videoWidth(0)
    , videoHeight(0)
    , videoFPS(0.0)
    , videoDuration(0)
    , isRunning_(false)
    , shouldDecode_(false)
    , shouldStop_(false)
    , rgbBuffer(nullptr)
    , rgbBufferSize(0)
    , startTime(0)
    , pauseTime(0)
    , isPaused(false)
    , frameInterval(0)
{
    qDebug() << "VideoDecoder created in thread:" << QThread::currentThreadId();
    
    // 连接内部信号
    connect(this, &VideoDecoder::signal_open_video, this, &VideoDecoder::openVideo, Qt::QueuedConnection);
    connect(this, &VideoDecoder::signal_start_decode, this, &VideoDecoder::startDecoding, Qt::QueuedConnection);
    
    // 启动解码线程
    isRunning_ = true;
    decodeThread_ = std::thread(&VideoDecoder::decodeThread, this);
}

VideoDecoder::~VideoDecoder()
{
    qDebug() << "VideoDecoder destructor called";
    
    // 停止解码线程
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isRunning_ = false;
        shouldStop_ = true;
    }
    cv_.notify_all();
    
    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }
    
    cleanupResources();
}

void VideoDecoder::openVideo(QString videoPath)
{
    qDebug() << "Opening video:" << videoPath;
    
    cleanupResources();
    
    // 打开视频文件
    if (avformat_open_input(&formatCtx, videoPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("无法打开视频文件: " + videoPath);
        return;
    }
    
    // 获取流信息
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        emit errorOccurred("无法获取视频流信息");
        avformat_close_input(&formatCtx);
        return;
    }
    
    // 查找视频流和音频流
    videoStreamIndex = -1;
    audioStreamIndex = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex == -1) {
            videoStreamIndex = i;
        } else if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex == -1) {
            audioStreamIndex = i;
        }
    }
    
    if (videoStreamIndex == -1) {
        emit errorOccurred("未找到视频流");
        avformat_close_input(&formatCtx);
        return;
    }
    
    AVStream* videoStream = formatCtx->streams[videoStreamIndex];
    AVCodecParameters* codecpar = videoStream->codecpar;
    
    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        emit errorOccurred("未找到视频解码器");
        avformat_close_input(&formatCtx);
        return;
    }
    
    // 创建解码器上下文
    videoCodecCtx = avcodec_alloc_context3(codec);
    if (!videoCodecCtx) {
        emit errorOccurred("无法创建解码器上下文");
        avformat_close_input(&formatCtx);
        return;
    }
    
    // 复制解码器参数
    if (avcodec_parameters_to_context(videoCodecCtx, codecpar) < 0) {
        emit errorOccurred("无法复制解码器参数");
        avcodec_free_context(&videoCodecCtx);
        avformat_close_input(&formatCtx);
        return;
    }
    
    // 打开解码器
    if (avcodec_open2(videoCodecCtx, codec, nullptr) < 0) {
        emit errorOccurred("无法打开解码器");
        avcodec_free_context(&videoCodecCtx);
        avformat_close_input(&formatCtx);
        return;
    }
    
    // 获取视频信息
    videoWidth = videoCodecCtx->width;
    videoHeight = videoCodecCtx->height;
    timeBase = videoStream->time_base;
    
    // 计算帧率
    if (videoStream->avg_frame_rate.den != 0) {
        videoFPS = av_q2d(videoStream->avg_frame_rate);
    } else if (videoStream->r_frame_rate.den != 0) {
        videoFPS = av_q2d(videoStream->r_frame_rate);
    } else {
        videoFPS = 25.0;  // 默认帧率
    }
    
    // 计算时长（转换为毫秒）
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        videoDuration = formatCtx->duration;  // 微秒
    } else if (videoStream->duration != AV_NOPTS_VALUE) {
        videoDuration = av_rescale_q(videoStream->duration, timeBase, {1, AV_TIME_BASE});
    } else {
        videoDuration = 0;
    }
    
    qDebug() << "Video info - Size:" << videoWidth << "x" << videoHeight
             << "FPS:" << videoFPS
             << "Duration:" << (videoDuration / 1000) << "ms"
             << "TimeBase:" << timeBase.num << "/" << timeBase.den;
    
    // 分配帧
    videoFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    packet = av_packet_alloc();
    
    if (!videoFrame || !rgbFrame || !packet) {
        emit errorOccurred("无法分配帧内存");
        cleanupResources();
        return;
    }
    
    // 创建图像转换上下文（YUV转RGB）
    swsCtx = sws_getContext(
        videoWidth, videoHeight, videoCodecCtx->pix_fmt,
        videoWidth, videoHeight, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    
    if (!swsCtx) {
        emit errorOccurred("无法创建图像转换上下文");
        cleanupResources();
        return;
    }
    
    // 分配RGB缓冲区
    rgbBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoWidth, videoHeight, 1);
    rgbBuffer = (uint8_t*)av_malloc(rgbBufferSize);
    
    if (!rgbBuffer) {
        emit errorOccurred("无法分配RGB缓冲区");
        cleanupResources();
        return;
    }
    
    // 设置RGB帧数据指针
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbBuffer,
                        AV_PIX_FMT_RGB24, videoWidth, videoHeight, 1);
    
    // 计算帧间隔（毫秒）
    frameInterval = (videoFPS > 0) ? (1000.0 / videoFPS) : 40.0;  // 默认25fps
    qDebug() << "Frame interval:" << frameInterval << "ms";
    
    // 初始化音频解码器
    if (audioStreamIndex >= 0) {
        AVStream* audioStream = formatCtx->streams[audioStreamIndex];
        const AVCodec* audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
        
        if (audioCodec) {
            audioCodecCtx = avcodec_alloc_context3(audioCodec);
            avcodec_parameters_to_context(audioCodecCtx, audioStream->codecpar);
            
            if (avcodec_open2(audioCodecCtx, audioCodec, nullptr) >= 0) {
                audioFrame = av_frame_alloc();
                
                // 创建音频重采样上下文（转换为PCM）
                swrCtx = swr_alloc_set_opts(nullptr,
                    AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,
                    audioCodecCtx->channel_layout, audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate,
                    0, nullptr);
                swr_init(swrCtx);
                
                qDebug() << "Audio decoder initialized - Sample rate:" << audioCodecCtx->sample_rate
                         << "Channels:" << audioCodecCtx->channels;
            }
        }
    }
    
    emit videoOpened(videoWidth, videoHeight, videoFPS, videoDuration / 1000);
    qDebug() << "Video opened successfully";
}

void VideoDecoder::seekToPosition(qint64 timestampMs)
{
    if (!formatCtx || videoStreamIndex < 0) {
        qDebug() << "Cannot seek: video not opened";
        return;
    }
    
    qDebug() << "Seeking to:" << timestampMs << "ms";
    
    // 暂停解码
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shouldDecode_ = false;
    }
    
    // 转换时间戳（毫秒 -> 微秒）
    int64_t timestamp = timestampMs * 1000;
    
    // 执行seek
    if (av_seek_frame(formatCtx, videoStreamIndex, 
                     av_rescale_q(timestamp, {1, AV_TIME_BASE}, timeBase), 
                     AVSEEK_FLAG_BACKWARD) < 0) {
        qDebug() << "Seek failed";
    }
    
    // 清空解码器缓冲
    if (videoCodecCtx) {
        avcodec_flush_buffers(videoCodecCtx);
    }
}

void VideoDecoder::startDecoding()
{
    qDebug() << "Starting decoding";
    startTime = QDateTime::currentMSecsSinceEpoch();  // 重置开始时间
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shouldDecode_ = true;
    }
    cv_.notify_one();
    emit decodingStarted();
}

void VideoDecoder::pauseDecoding()
{
    qDebug() << "Pausing decoding";
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shouldDecode_ = false;
    }
    emit decodingPaused();
}

void VideoDecoder::stopDecoding()
{
    qDebug() << "Stopping decoding";
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shouldDecode_ = false;
        shouldStop_ = true;
    }
    cv_.notify_one();
    emit decodingStopped();
}

void VideoDecoder::decodeThread()
{
    qDebug() << "Decode thread started, ID:" << QThread::currentThreadId();
    
    while (isRunning_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return shouldDecode_ || !isRunning_; });
        
        if (!isRunning_ || shouldStop_) {
            break;
        }
        
        lock.unlock();
        
        // 解码一帧
        if (shouldDecode_ && formatCtx && videoCodecCtx) {
            if (!decodeOneFrame()) {
                // 解码结束
                qDebug() << "Decoding finished";
                {
                    std::lock_guard<std::mutex> l(mutex_);
                    shouldDecode_ = false;
                }
                emit decodingFinished();
            }
        }
    }
    
    qDebug() << "Decode thread exiting";
}

bool VideoDecoder::decodeOneFrame()
{
    if (!formatCtx || !videoCodecCtx) {
        return false;
    }
    
    // 读取数据包
    int ret = av_read_frame(formatCtx, packet);
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            qDebug() << "End of file reached";
        }
        return false;
    }
    
    // 处理音频流
    if (packet->stream_index == audioStreamIndex && audioCodecCtx) {
        ret = avcodec_send_packet(audioCodecCtx, packet);
        if (ret >= 0) {
            while (avcodec_receive_frame(audioCodecCtx, audioFrame) == 0) {
                // 重采样音频数据
                int out_samples = av_rescale_rnd(swr_get_delay(swrCtx, audioCodecCtx->sample_rate) + audioFrame->nb_samples,
                                                44100, audioCodecCtx->sample_rate, AV_ROUND_UP);
                uint8_t* output[2];
                int out_linesize;
                av_samples_alloc(output, &out_linesize, 2, out_samples, AV_SAMPLE_FMT_S16, 0);
                
                int converted = swr_convert(swrCtx, output, out_samples,
                                          (const uint8_t**)audioFrame->data, audioFrame->nb_samples);
                
                int data_size = av_samples_get_buffer_size(&out_linesize, 2, converted, AV_SAMPLE_FMT_S16, 1);
                QByteArray audioData((const char*)output[0], data_size);
                
                qint64 audioPts = av_rescale_q(audioFrame->pts, formatCtx->streams[audioStreamIndex]->time_base, {1, 1000});
                qDebug() << "[AUDIO] Emitting audio data, size:" << audioData.size() << "bytes, PTS:" << audioPts << "ms";
                emit audioDataReady(audioData, audioPts);
                
                av_freep(&output[0]);
                av_frame_unref(audioFrame);
            }
        }
        av_packet_unref(packet);
        return true;
    }
    
    // 处理视频流
    if (packet->stream_index != videoStreamIndex) {
        av_packet_unref(packet);
        return true;  // 继续读取下一帧
    }
    
    // 发送数据包到解码器
    ret = avcodec_send_packet(videoCodecCtx, packet);
    if (ret < 0) {
        qDebug() << "Error sending packet to decoder";
        av_packet_unref(packet);
        return false;
    }
    
    // 接收解码后的帧
    while (ret >= 0) {
        ret = avcodec_receive_frame(videoCodecCtx, videoFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            qDebug() << "Error receiving frame from decoder";
            av_packet_unref(packet);
            return false;
        }
        
        // 转换YUV到RGB
        sws_scale(swsCtx, videoFrame->data, videoFrame->linesize, 0, videoHeight,
                 rgbFrame->data, rgbFrame->linesize);
        
        // 转换为QImage
        QImage image = convertFrameToImage(rgbFrame);
        
        // 计算PTS（转换为毫秒）
        qint64 ptsMs = 0;
        if (videoFrame->pts != AV_NOPTS_VALUE) {
            ptsMs = av_rescale_q(videoFrame->pts, timeBase, {1, 1000});
        }
        
        // 帧率控制：根据PTS延迟
        if (startTime == 0) {
            startTime = QDateTime::currentMSecsSinceEpoch();
        }
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch() - startTime;
        qint64 delay = ptsMs - currentTime;
        
        if (delay > 0 && delay < 1000) {  // 最多延迟1秒
            QThread::msleep(delay);
        }
        
        // 发送帧
        emit videoFrameReady(image, ptsMs);
        
        av_frame_unref(videoFrame);
    }
    
    av_packet_unref(packet);
    return true;
}

QImage VideoDecoder::convertFrameToImage(AVFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return QImage();
    }
    
    // 创建QImage（RGB24格式）
    QImage image(frame->data[0], videoWidth, videoHeight, 
                frame->linesize[0], QImage::Format_RGB888);
    
    // 深拷贝，避免数据被释放
    return image.copy();
}

void VideoDecoder::cleanupResources()
{
    qDebug() << "Cleaning up video decoder resources";
    
    if (rgbBuffer) {
        av_free(rgbBuffer);
        rgbBuffer = nullptr;
    }
    
    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    
    if (swrCtx) {
        swr_free(&swrCtx);
    }
    
    if (videoFrame) {
        av_frame_free(&videoFrame);
    }
    
    if (rgbFrame) {
        av_frame_free(&rgbFrame);
    }
    
    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
    
    if (packet) {
        av_packet_free(&packet);
    }
    
    if (videoCodecCtx) {
        avcodec_free_context(&videoCodecCtx);
    }
    
    if (audioCodecCtx) {
        avcodec_free_context(&audioCodecCtx);
    }
    
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
    
    videoStreamIndex = -1;
    audioStreamIndex = -1;
    videoWidth = 0;
    videoHeight = 0;
    startTime = 0;
}
