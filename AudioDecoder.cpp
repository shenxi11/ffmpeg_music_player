#include "AudioDecoder.h"
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QUrl>
#include <chrono>

AudioDecoder::AudioDecoder(QObject *parent)
    : QObject(parent),
      m_formatCtx(nullptr),
      m_codecCtx(nullptr),
      m_swrCtx(nullptr),
      m_frame(nullptr),
      m_packet(nullptr),
      m_audioStreamIndex(-1),
      m_duration(0),
      m_outSampleRate(44100),
      m_outChannels(2),
      m_outSampleFormat(AV_SAMPLE_FMT_S16),
      m_decoding(false),
      m_paused(false),
      m_stopRequested(false),
      m_seekRequested(false),
      m_seekTarget(0),
      m_currentPts(0),
      m_firstPts(0),
      m_firstPtsSet(false)
{
    qDebug() << "AudioDecoder created in thread:" << QThread::currentThreadId();
}

AudioDecoder::~AudioDecoder()
{
    stopDecode();
    
    // 等待解码线程结束
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }
    
    cleanup();
}

bool AudioDecoder::initDecoder(const QString& filePath)
{
    auto t_start = std::chrono::high_resolution_clock::now();
    
    cleanup(); // 清理旧资源
    
    m_filePath = filePath;
    
    auto t0 = std::chrono::high_resolution_clock::now();
    if (!openFile(filePath)) {
        emit decodeError("Failed to open file");
        return false;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::openFile:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    if (!setupDecoder()) {
        emit decodeError("Failed to setup decoder");
        return false;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::setupDecoder:" 
             << std::chrono::duration<double, std::milli>(t2 - t1).count() << "ms";
    
    if (!setupResampler()) {
        emit decodeError("Failed to setup resampler");
        return false;
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::setupResampler:" 
             << std::chrono::duration<double, std::milli>(t3 - t2).count() << "ms";
    
    // 获取音频时长
    m_duration = m_formatCtx->duration * av_q2d(av_make_q(1, AV_TIME_BASE)) * 1000; // 转换为毫秒
    
    // 发送元数据
    int sampleRate = m_codecCtx->sample_rate;
    int channels = 2; // 重采样后的通道数
    emit metadataReady(m_duration, sampleRate, channels);
    
    // 提取封面
    extractAlbumArt();
    
    auto t_end = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioDecoder::initDecoder TOTAL:" 
             << std::chrono::duration<double, std::milli>(t_end - t_start).count() << "ms";
    
    qDebug() << "AudioDecoder initialized successfully. Duration:" << m_duration << "ms";
    return true;
}

void AudioDecoder::startDecode()
{
    if (m_decoding) {
        // 检查是否真的需要resume
        bool wasPaused = m_paused.load();
        if (wasPaused) {
            qDebug() << "AudioDecoder: Resuming from pause";
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_paused = false;
            }
            m_cv.notify_all();
        }
        return;
    }
    
    m_decoding = true;
    m_paused = false;
    m_stopRequested = false;
    m_firstPtsSet = false;  // 重置基准时间戳标志
    
    // 启动独立的解码线程
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }
    
    m_decodeThread = std::thread([this]() {
        qDebug() << "AudioDecoder: Decode thread started in std::thread";
        this->decode();
    });
    
    emit decodeStarted();
    qDebug() << "AudioDecoder: Decode thread launched";
}

void AudioDecoder::pauseDecode()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_paused = true;
    }
    m_cv.notify_all();
    emit decodePaused();
}

void AudioDecoder::stopDecode()
{
    m_stopRequested = true;
    m_decoding = false;
    m_cv.notify_all();
    
    // 立即等待解码线程结束（确保同步停止）
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
        qDebug() << "AudioDecoder: Decode thread stopped synchronously";
    }
    
    emit decodeStopped();
}

void AudioDecoder::seekTo(qint64 positionMs)
{
    if (!m_formatCtx || m_audioStreamIndex < 0) return;
    
    qDebug() << "AudioDecoder: Seeking to" << positionMs << "ms";
    
    // 设置seek标志（加锁保护）
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_seekTarget = positionMs;
        m_seekRequested = true;
    }
    
    // 毫秒转微秒 (FFmpeg使用微秒)
    int64_t targetTimestamp = static_cast<int64_t>(positionMs) * 1000;
    
    // 转换为流时间基准
    AVRational time_base = m_formatCtx->streams[m_audioStreamIndex]->time_base;
    int64_t seek_target = av_rescale_q(targetTimestamp, {1, AV_TIME_BASE}, time_base);
    
    // 执行seek操作（使用音频流索引和BACKWARD标志）
    int ret = av_seek_frame(m_formatCtx, m_audioStreamIndex, seek_target, AVSEEK_FLAG_BACKWARD);
    
    // 如果失败，尝试使用全局seek（stream index = -1）
    if (ret < 0) {
        qDebug() << "AudioDecoder: Stream seek failed, trying global seek";
        ret = av_seek_frame(m_formatCtx, -1, targetTimestamp, AVSEEK_FLAG_BACKWARD);
    }
    
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "AudioDecoder: Seek failed, error:" << errbuf;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_seekRequested = false;
        return;
    }
    
    // 清空解码器缓冲
    if (m_codecCtx) {
        avcodec_flush_buffers(m_codecCtx);
    }
    
    // 对于网络流，清空IO缓冲区
    bool isNetworkStream = m_filePath.startsWith("http");
    if (isNetworkStream && m_formatCtx->pb) {
        avio_flush(m_formatCtx->pb);
    }
    
    qDebug() << "AudioDecoder: Seek completed";
}

void AudioDecoder::decode()
{
    qDebug() << "AudioDecoder::decode thread started in std::thread";
    
    while (!m_stopRequested) {
        // 只有在暂停时才等待
        if (m_paused) {
            std::unique_lock<std::mutex> lock(m_mutex);
            qDebug() << "AudioDecoder: Paused, waiting for resume signal";
            m_cv.wait(lock, [this]() { return !m_paused || m_stopRequested; });
            qDebug() << "AudioDecoder: Resumed";
            
            if (m_stopRequested) {
                qDebug() << "AudioDecoder: Stop requested during pause";
                break;
            }
        }
        
        if (m_stopRequested) break;
        
        // 处理seek请求
        if (m_seekRequested) {
            qDebug() << "AudioDecoder: Processing seek request";
            m_seekRequested = false;
            continue;
        }
        
        // 读取音频包
        int ret = av_read_frame(m_formatCtx, m_packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                // 到达文件末尾
                qDebug() << "AudioDecoder: End of stream reached";
                emit decodeCompleted();
                break;
            } else {
                // 其他错误（如网络中断），记录但继续尝试
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                qDebug() << "AudioDecoder: Read frame error:" << errbuf << ", will retry";
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;  // 继续循环，不退出
            }
        }
        
        // 只处理音频流
        if (m_packet->stream_index == m_audioStreamIndex) {
            // 发送数据包到解码器
            if (avcodec_send_packet(m_codecCtx, m_packet) >= 0) {
                // 接收解码后的帧
                while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0) {
                    // 计算重采样后的样本数
                    int dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(m_swrCtx, m_codecCtx->sample_rate) + m_frame->nb_samples,
                        m_outSampleRate,
                        m_codecCtx->sample_rate,
                        AV_ROUND_UP
                    );
                    
                    // 计算缓冲区大小
                    int buffer_size = av_samples_get_buffer_size(
                        nullptr, 
                        m_outChannels, 
                        dst_nb_samples, 
                        m_outSampleFormat, 
                        1
                    );
                    
                    if (buffer_size <= 0) {
                        qDebug() << "Error calculating buffer size:" << buffer_size;
                        continue;
                    }
                    
                    // 分配缓冲区
                    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
                    if (!buffer) {
                        qDebug() << "Failed to allocate memory for buffer";
                        continue;
                    }
                    
                    // 执行重采样
                    int converted_samples = swr_convert(
                        m_swrCtx, 
                        &buffer,
                        dst_nb_samples,
                        (const uint8_t**)m_frame->data,
                        m_frame->nb_samples
                    );
                    
                    // 计算实际转换后的数据大小
                    int converted_buffer_size = av_samples_get_buffer_size(
                        nullptr, 
                        m_outChannels,
                        converted_samples,
                        m_outSampleFormat, 
                        1
                    );
                    
                    // 获取时间戳
                    qint64 timestamp_ms = 0;
                    if (m_packet->pts != AV_NOPTS_VALUE) {
                        AVRational time_base = m_formatCtx->streams[m_audioStreamIndex]->time_base;
                        qint64 raw_timestamp_ms = av_rescale_q(m_packet->pts, time_base, {1, 1000});
                        
                        // 记录第一个数据包的PTS作为基准
                        if (!m_firstPtsSet) {
                            m_firstPts = raw_timestamp_ms;
                            m_firstPtsSet = true;
                            qDebug() << "AudioDecoder: First PTS set to" << m_firstPts << "ms";
                        }
                        
                        // 减去基准时间戳，确保从0开始
                        timestamp_ms = raw_timestamp_ms - m_firstPts;
                        m_currentPts = timestamp_ms;
                    }
                    
                    // 创建QByteArray并发送信号
                    QByteArray pcmData(reinterpret_cast<const char*>(buffer), converted_buffer_size);
                    free(buffer);
                    
                    // 在发送数据前再次检查是否停止（避免发送旧数据到新Session）
                    if (m_stopRequested) {
                        qDebug() << "AudioDecoder: Stop requested, discarding decoded frame";
                        break;
                    }
                    
                    emit decodedData(pcmData, timestamp_ms);
                    emit progressUpdated(timestamp_ms);
                    
                    // 每100帧打印一次日志，避免日志过多
                    static int frameCount = 0;
                    if (++frameCount % 100 == 0) {
                        qDebug() << "AudioDecoder: Decoded" << frameCount << "frames, current timestamp:" << timestamp_ms << "ms";
                    }
                    
                    // 给主线程时间处理信号，避免解码过快导致信号堆积
                    // 每10帧yield一次，让主线程有机会处理decodedData信号
                    if (frameCount % 10 == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                }
            }
        }
        
        // 释放数据包
        av_packet_unref(m_packet);
    }
    
    // 解码线程结束，重置状态标志
    m_decoding = false;
    qDebug() << "AudioDecoder::decode thread finished, m_decoding reset to false";
}

void AudioDecoder::cleanup()
{
    // TODO: 清理FFmpeg资源
    if(m_frame)
        av_frame_free(&m_frame);
    if(m_packet)
    {
        av_packet_unref(m_packet);
        av_packet_free(&m_packet);
    }
    if(m_codecCtx)
        avcodec_free_context(&m_codecCtx);
    if(m_formatCtx)
        avformat_close_input(&m_formatCtx);
    if(m_swrCtx)
        swr_free(&m_swrCtx);
}

void AudioDecoder::extractAlbumArt()
{
    if (!m_formatCtx) return;
    
    // 查找附加图片流（封面）
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket *pkt = &m_formatCtx->streams[i]->attached_pic;
            
            // 保存封面到临时文件
            QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + "/album_cover_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg";
            
            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write((const char*)pkt->data, pkt->size);
                file.close();
                
                // 转换为 file:/// URL 格式供 QML 使用
                QString fileUrl = QUrl::fromLocalFile(tempPath).toString();
                
                qDebug() << "Album cover extracted to:" << fileUrl;
                emit albumArtReady(fileUrl);
            } else {
                qDebug() << "Failed to save album cover";
                emit albumArtReady("qrc:/new/prefix1/icon/maxresdefault.jpg");
            }
            return;
        }
    }
    
    // 没有找到封面，使用默认图片
    qDebug() << "No album cover found, using default image";
    emit albumArtReady("qrc:/new/prefix1/icon/maxresdefault.jpg");
}

bool AudioDecoder::openFile(const QString& filePath)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    
    QByteArray utf8Data = filePath.toUtf8();
    const char* inputUrl = utf8Data.constData();
    
    AVDictionary* options = nullptr;
    // 网络超时设置（30秒），应对网络不稳定
    av_dict_set(&options, "timeout", "30000000", 0);  // 30秒超时
    av_dict_set(&options, "reconnect", "1", 0);  // 启用自动重连
    av_dict_set(&options, "reconnect_streamed", "1", 0);  // 流式重连
    av_dict_set(&options, "reconnect_delay_max", "5", 0);  // 最大重连延迟5秒
    
    int ret = avformat_open_input(&m_formatCtx, inputUrl, nullptr, &options);
    av_dict_free(&options);
    
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING]   avformat_open_input:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "Could not open input file:" << errbuf << "Error code:" << ret;
        return false;
    }
    
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        qDebug() << "Could not find stream information";
        avformat_close_input(&m_formatCtx);
        return false;
    }
    
    auto t2 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING]   avformat_find_stream_info:" 
             << std::chrono::duration<double, std::milli>(t2 - t1).count() << "ms";
    
    // 查找音频流
    m_audioStreamIndex = -1;
    for (int i = 0; i < (int)m_formatCtx->nb_streams; ++i) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            break;
        }
    }
    
    if (m_audioStreamIndex == -1) {
        qDebug() << "Could not find an audio stream";
        avformat_close_input(&m_formatCtx);
        return false;
    }
    
    qDebug() << "Audio stream found at index:" << m_audioStreamIndex;
    return true;
}

bool AudioDecoder::setupDecoder()
{
    if (!m_formatCtx || m_audioStreamIndex < 0) return false;
    
    // 查找解码器
    AVCodec* codec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    if (!codec) {
        qDebug() << "Codec not found";
        return false;
    }
    
    // 分配解码器上下文
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qDebug() << "Could not allocate codec context";
        return false;
    }
    
    // 复制参数
    if (avcodec_parameters_to_context(m_codecCtx, m_formatCtx->streams[m_audioStreamIndex]->codecpar) < 0) {
        qDebug() << "Failed to copy codec parameters to codec context";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    // 打开解码器
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        qDebug() << "Could not open codec";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    // 分配帧和包
    m_frame = av_frame_alloc();
    if (!m_frame) {
        qDebug() << "Could not allocate frame";
        return false;
    }
    
    m_packet = av_packet_alloc();
    if (!m_packet) {
        qDebug() << "Could not allocate packet";
        av_frame_free(&m_frame);
        return false;
    }
    
    qDebug() << "Decoder setup successfully. Sample rate:" << m_codecCtx->sample_rate
             << "Channels:" << m_codecCtx->channels;
    return true;
}

bool AudioDecoder::setupResampler()
{
    if (!m_codecCtx) return false;
    
    // 获取输入格式
    int64_t input_channel_layout = m_codecCtx->channel_layout;
    if (input_channel_layout == 0) {
        input_channel_layout = av_get_default_channel_layout(m_codecCtx->channels);
    }
    
    // 分配重采样上下文
    m_swrCtx = swr_alloc();
    if (!m_swrCtx) {
        qDebug() << "Could not allocate SwrContext";
        return false;
    }
    
    // 配置重采样参数
    int64_t output_channel_layout = AV_CH_LAYOUT_STEREO;
    
    av_opt_set_int(m_swrCtx, "in_channel_layout", input_channel_layout, 0);
    av_opt_set_int(m_swrCtx, "out_channel_layout", output_channel_layout, 0);
    av_opt_set_int(m_swrCtx, "in_sample_rate", m_codecCtx->sample_rate, 0);
    av_opt_set_int(m_swrCtx, "out_sample_rate", m_outSampleRate, 0);
    av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_codecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", m_outSampleFormat, 0);
    
    // 初始化重采样器
    if (swr_init(m_swrCtx) < 0) {
        qDebug() << "Could not initialize resampler";
        swr_free(&m_swrCtx);
        return false;
    }
    
    qDebug() << "Resampler setup successfully";
    return true;
}
