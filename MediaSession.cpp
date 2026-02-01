#include "MediaSession.h"
#include "VideoSession.h"
#include "VideoBuffer.h"
#include "VideoRenderer.h"
#include "AudioService.h"
#include "AudioPlayer.h"
#include <QDebug>
#include <QFileInfo>

extern "C" {
#include <libswresample/swresample.h>
}

MediaSession::MediaSession(QObject* parent)
    : QObject(parent)
    , m_audioSession(nullptr)
    , m_videoSession(nullptr)
    , m_formatContext(nullptr)
    , m_audioStreamIndex(-1)
    , m_videoStreamIndex(-1)
    , m_audioCodecCtx(nullptr)
    , m_audioSwrCtx(nullptr)
    , m_audioFrame(nullptr)
    , m_duration(0)
    , m_state(Stopped)
    , m_syncTimer(nullptr)
    , m_syncEnabled(true)
    , m_masterClock(0)
    , m_demuxThread(nullptr)
    , m_demuxRunning(false)
    , m_seekPending(false)
    , m_positionTimer(nullptr)
{
    qDebug() << "[MediaSession] Created";
    
    // 创建同步定时器（每10ms检查一次同步）
    m_syncTimer = new QTimer(this);
    m_syncTimer->setInterval(10);
    connect(m_syncTimer, &QTimer::timeout, this, &MediaSession::syncAudioVideo);
    
    // 创建位置更新定时器（每100ms更新一次位置）
    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(100);
    connect(m_positionTimer, &QTimer::timeout, this, &MediaSession::updatePosition);
}

MediaSession::~MediaSession()
{
    qDebug() << "[MediaSession] Destroying...";
    
    stop();
    unloadSource();
    
    if (m_syncTimer) {
        m_syncTimer->stop();
        delete m_syncTimer;
    }
    
    if (m_positionTimer) {
        m_positionTimer->stop();
        delete m_positionTimer;
    }
    
    qDebug() << "[MediaSession] Destroyed";
}

bool MediaSession::loadSource(const QUrl& url)
{
    qDebug() << "[MediaSession] Loading source:" << url;
    
    // 重置音频播放器的时钟和缓冲区（重要：避免上次播放的时钟影响新视频）
    AudioPlayer::instance().resetBuffer();
    
    QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    
    // 初始化解复用器
    if (!initDemuxer(filePath)) {
        qWarning() << "[MediaSession] Failed to init demuxer";
        setState(Error);
        return false;
    }
    
    m_currentUrl = url.toString();
    
    // 初始化音频解码器（如果有音频流）
    if (m_audioStreamIndex >= 0) {
        qDebug() << "[MediaSession] Initializing audio decoder for stream" << m_audioStreamIndex;
        
        // 初始化音频解码器（直接在MediaSession中处理，不使用AudioSession）
        AVStream* audioStream = m_formatContext->streams[m_audioStreamIndex];
        if (!initAudioDecoder(audioStream)) {
            qWarning() << "[MediaSession] Failed to init audio decoder";
        } else {
            qDebug() << "[MediaSession] Audio stream - codec:" 
                     << avcodec_get_name(audioStream->codecpar->codec_id)
                     << "sample_rate:" << audioStream->codecpar->sample_rate;
        }
    }
    
    // 创建视频会话（如果有视频流）
    if (m_videoStreamIndex >= 0) {
        qDebug() << "[MediaSession] Creating VideoSession for stream" << m_videoStreamIndex;
        m_videoSession = new VideoSession(this);
        
        // 连接视频帧渲染信号（用于同步）
        // connect(m_videoSession, &VideoSession::frameRendered, 
        //         this, &MediaSession::onVideoFrameRendered);
        
        // 初始化 VideoDecoder with stream info
        AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
        if (!m_videoSession->initVideoStream(videoStream)) {
            qWarning() << "[MediaSession] Failed to init video stream";
            delete m_videoSession;
            m_videoSession = nullptr;
        } else {
            qDebug() << "[MediaSession] Video stream - codec:" 
                     << avcodec_get_name(videoStream->codecpar->codec_id)
                     << "resolution:" << videoStream->codecpar->width 
                     << "x" << videoStream->codecpar->height;
        }
    }
    
    // 发送元数据信号
    QString title = QFileInfo(filePath).fileName();
    emit metadataReady(title, "", m_duration);
    emit durationChanged(m_duration);
    
    setState(Stopped);
    return true;
}

void MediaSession::unloadSource()
{
    qDebug() << "[MediaSession] Unloading source";
    
    stopDemux();
    
    // 删除子会话
    if (m_audioSession) {
        delete m_audioSession;
        m_audioSession = nullptr;
    }
    
    if (m_videoSession) {
        delete m_videoSession;
        m_videoSession = nullptr;
    }
    
    // 清理音频解码器
    cleanupAudioDecoder();
    
    // 清理解复用器
    cleanupDemuxer();
    
    m_currentUrl.clear();
    m_duration = 0;
}

void MediaSession::play()
{
    qDebug() << "[MediaSession] Play";
    
    if (m_state == Playing) {
        qDebug() << "[MediaSession] Already playing";
        return;
    }
    
    // 启动解复用
    startDemux();
    
    // 启动音频播放：区分首次播放和从暂停恢复
    if (m_audioCodecCtx) {
        if (m_state == Paused) {
            // 从暂停恢夏，使用resume()
            AudioPlayer::instance().resume();
            qDebug() << "[MediaSession] Audio playback resumed";
        } else {
            // 首次播放，使用start()
            AudioPlayer::instance().start();
            qDebug() << "[MediaSession] Audio playback started";
        }
    }
    
    // 启动视频渲染
    if (m_videoSession) {
        m_videoSession->start();
        qDebug() << "[MediaSession] Video playback started";
    }
    
    // 启动同步
    if (m_syncEnabled && hasVideo() && hasAudio()) {
        m_syncTimer->start();
    }
    
    // 启动位置更新
    m_positionTimer->start();
    
    setState(Playing);
}

void MediaSession::pause()
{
    qDebug() << "[MediaSession] Pause";
    
    if (m_state != Playing) {
        return;
    }
    
    // 暂停音频播放
    if (m_audioCodecCtx) {
        AudioPlayer::instance().pause();
    }
    
    // 暂停视频
    if (m_videoSession) {
        m_videoSession->pause();
    }
    
    // 停止同步和位置更新
    m_syncTimer->stop();
    m_positionTimer->stop();
    
    setState(Paused);
}

void MediaSession::stop()
{
    qDebug() << "[MediaSession] Stop";
    
    // 停止解复用
    stopDemux();
    
    // 停止音频播放器
    if (m_audioCodecCtx) {
        AudioPlayer::instance().stop();
    }
    
    // 停止视频
    if (m_videoSession) {
        m_videoSession->stop();
    }
    
    // 停止定时器
    m_syncTimer->stop();
    m_positionTimer->stop();
    
    setState(Stopped);
}

void MediaSession::seekTo(qint64 positionMs)
{
    qDebug() << "[MediaSession] Seek to:" << positionMs << "ms";
    
    if (!m_formatContext) {
        return;
    }
    
    // 暂停解复用，避免在 seek 过程中继续读取数据
    bool wasRunning = m_demuxRunning;
    if (wasRunning) {
        m_demuxRunning = false;
        
        // 等待 demux 线程完全停止
        if (m_demuxThread && !m_demuxThread->isFinished()) {
            qDebug() << "[MediaSession] Waiting for demux thread to stop...";
            m_demuxThread->wait(1000);  // 最多等待1秒
            qDebug() << "[MediaSession] Demux thread stopped";
        }
    }
    
    // 计算时间戳（AV_TIME_BASE 单位，微秒）
    int64_t timestamp = positionMs * 1000;  // 转为微秒
    
    // FFmpeg seek
    int ret = av_seek_frame(m_formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        qWarning() << "[MediaSession] Seek failed";
        if (wasRunning) {
            m_demuxRunning = true;
        }
        return;
    }
    
    // 清空音频缓冲区和解码器
    if (m_audioCodecCtx) {
        avcodec_flush_buffers(m_audioCodecCtx);
        AudioPlayer::instance().resetBuffer();
        // 标记seek后等待首帧，用实际的PTS同步时钟
        m_seekPending = true;
        qDebug() << "[MediaSession] Audio buffers flushed, waiting for first frame to sync clock";
    }
    
    // 清空视频缓冲区和解码器
    if (m_videoSession) {
        m_videoSession->flush();
        qDebug() << "[MediaSession] Video buffers flushed";
        
        // 启动视频缓冲，等待足够的帧再开始渲染
        if (m_videoSession->videoRenderer()) {
            QMetaObject::invokeMethod(m_videoSession->videoRenderer(), "startBuffering");
        }
    }
    
    // 更新主时钟
    m_masterClock = positionMs;
    
    // 恢复解复用：由于设置 m_demuxRunning=false 会导致 demuxLoop 退出，
    // 需要重新启动解复用线程
    if (wasRunning) {
        m_demuxRunning = true;
        // 如果线程已经退出，需要重新启动
        if (!m_demuxThread || m_demuxThread->isFinished()) {
            qDebug() << "[MediaSession] Restarting demux thread after seek";
            m_demuxThread = QThread::create([this]() {
                this->demuxLoop();
            });
            connect(m_demuxThread, &QThread::finished, m_demuxThread, &QThread::deleteLater);
            m_demuxThread->start();
        }
    }
    
    emit positionChanged(positionMs);
}

void MediaSession::enableSync(bool enable)
{
    m_syncEnabled = enable;
    
    if (!enable) {
        m_syncTimer->stop();
    } else if (m_state == Playing && hasVideo() && hasAudio()) {
        m_syncTimer->start();
    }
}

qint64 MediaSession::getCurrentPosition() const
{
    // 优先使用 AudioPlayer 的播放位置（最准确的主时钟）
    if (m_audioCodecCtx && AudioPlayer::instance().isPlaying()) {
        return AudioPlayer::instance().getPlaybackPosition();
    }
    
    // 如果音频未播放，使用视频PTS
    if (m_videoSession) {
        qint64 videoPTS = m_videoSession->getCurrentPTS();
        return videoPTS;
    }
    
    // 最后才使用 masterClock
    return m_masterClock;
}

void MediaSession::onAudioPositionChanged(qint64 audioPos)
{
    // 更新主时钟
    m_masterClock = audioPos;
    
    // 转发位置信号
    emit positionChanged(audioPos);
}

void MediaSession::onVideoFrameRendered(qint64 videoPts)
{
    // 视频帧渲染回调（用于同步检测）
    // 当前实现在 syncAudioVideo 中主动查询，也可以用此信号驱动
    Q_UNUSED(videoPts);
}

void MediaSession::syncAudioVideo()
{
    if (!m_syncEnabled || !m_videoSession || !m_audioSession) {
        return;
    }
    
    // 获取音频当前位置（主时钟）
    qint64 audioPos = m_masterClock;
    
    // 获取视频当前PTS
    // TODO: qint64 videoPos = m_videoSession->getCurrentPTS();
    qint64 videoPos = audioPos;  // 临时
    
    // 计算偏差
    int offset = videoPos - audioPos;
    
    // 允许误差范围：±40ms
    if (qAbs(offset) > 40) {
        qDebug() << "[MediaSession] Sync error:" << offset << "ms (audio:" << audioPos << "video:" << videoPos << ")";
        
        if (offset > 0) {
            // 视频超前，需要延迟渲染
            // TODO: m_videoSession->holdFrame();
        } else {
            // 视频落后，需要跳帧加速
            // TODO: m_videoSession->skipNonKeyFrames();
        }
        
        emit syncError(offset);
    }
}

void MediaSession::updatePosition()
{
    if (m_state == Playing) {
        qint64 pos = getCurrentPosition();
        emit positionChanged(pos);
    }
}

bool MediaSession::initDemuxer(const QString& filePath)
{
    // 打开输入文件
    int ret = avformat_open_input(&m_formatContext, filePath.toUtf8().data(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[MediaSession] Failed to open input:" << errbuf;
        return false;
    }
    
    // 获取流信息
    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to find stream info";
        avformat_close_input(&m_formatContext);
        return false;
    }
    
    // 查找音频流
    m_audioStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, 
                                              -1, -1, nullptr, 0);
    
    // 查找视频流
    m_videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, 
                                              -1, -1, nullptr, 0);
    
    qDebug() << "[MediaSession] Stream indices - Audio:" << m_audioStreamIndex 
             << "Video:" << m_videoStreamIndex;
    
    // 计算总时长
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = m_formatContext->duration / 1000;  // 转为毫秒
    } else {
        m_duration = 0;
    }
    
    qDebug() << "[MediaSession] Duration:" << m_duration << "ms";
    
    // 打印文件信息（调试用）
    av_dump_format(m_formatContext, 0, filePath.toUtf8().data(), 0);
    
    return m_audioStreamIndex >= 0 || m_videoStreamIndex >= 0;
}

void MediaSession::cleanupDemuxer()
{
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }
    
    m_audioStreamIndex = -1;
    m_videoStreamIndex = -1;
}

void MediaSession::startDemux()
{
    if (!m_formatContext) {
        return;
    }
    
    // 检查线程是否已经结束（播放完成或错误）
    // 注意：deleteLater() 后指针仍然有效，但对象可能已删除
    // 安全的做法是先检查 m_demuxRunning 状态
    if (m_demuxRunning) {
        // 如果标记为运行中，但线程实际已结束，重置状态
        if (m_demuxThread && m_demuxThread->isFinished()) {
            qDebug() << "[MediaSession] Previous demux thread finished, will restart";
            m_demuxRunning = false;
            m_demuxThread = nullptr;  // 清空指针，让deleteLater完成清理
        } else {
            qDebug() << "[MediaSession] Demux already running";
            return;
        }
    }
    
    qDebug() << "[MediaSession] Starting demux thread";
    
    m_demuxRunning = true;
    
    // 创建解复用线程
    m_demuxThread = QThread::create([this]() {
        this->demuxLoop();
    });
    
    connect(m_demuxThread, &QThread::finished, m_demuxThread, &QThread::deleteLater);
    m_demuxThread->start();
}

void MediaSession::stopDemux()
{
    if (!m_demuxRunning) {
        return;
    }
    
    qDebug() << "[MediaSession] Stopping demux thread";
    
    m_demuxRunning = false;
    
    if (m_demuxThread) {
        m_demuxThread->quit();
        m_demuxThread->wait(1000);
        m_demuxThread = nullptr;
    }
}

void MediaSession::demuxLoop()
{
    qDebug() << "[MediaSession] Demux loop started";
    qDebug() << "[MediaSession] Stream indices - Audio:" << m_audioStreamIndex << "Video:" << m_videoStreamIndex;
    qDebug() << "[MediaSession] VideoSession:" << (m_videoSession ? "valid" : "nullptr");
    
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        qWarning() << "[MediaSession] Failed to allocate packet";
        return;
    }
    
    int videoPacketCount = 0;
    int audioPacketCount = 0;
    
    while (m_demuxRunning) {
        // 检查视频缓冲区大小，避免解码速度过快
        if (m_videoSession && m_videoSession->videoBuffer()) {
            int bufferSize = m_videoSession->videoBuffer()->size();
            int bufferCapacity = m_videoSession->videoBuffer()->capacity();
            
            // 如果缓冲区超过80%，暂停读取，等待渲染消费
            if (bufferSize >= bufferCapacity * 0.8) {
                QThread::msleep(10);  // 等待10ms让渲染器消费帧
                continue;
            }
        }
        
        int ret = av_read_frame(m_formatContext, pkt);
        
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                qDebug() << "[MediaSession] Demux reached EOF";
                emit playbackFinished();
            } else {
                char errbuf[128];
                av_strerror(ret, errbuf, sizeof(errbuf));
                qWarning() << "[MediaSession] Read frame error:" << errbuf;
            }
            break;
        }
        
        // 分发packet到对应的解码器
        if (pkt->stream_index == m_audioStreamIndex && m_audioCodecCtx) {
            // 解码音频
            int ret = avcodec_send_packet(m_audioCodecCtx, pkt);
            if (ret == 0) {
                static int audioFrameCount = 0;
                while (avcodec_receive_frame(m_audioCodecCtx, m_audioFrame) == 0) {
                    audioFrameCount++;
                    if (audioFrameCount % 100 == 0) {
                        qDebug() << "[MediaSession] Decoded" << audioFrameCount << "audio frames";
                    }
                    
                    // 重采样到 44100Hz stereo s16
                    uint8_t* outBuffer = nullptr;
                    int outSamples = av_rescale_rnd(
                        swr_get_delay(m_audioSwrCtx, m_audioCodecCtx->sample_rate) + m_audioFrame->nb_samples,
                        44100,
                        m_audioCodecCtx->sample_rate,
                        AV_ROUND_UP
                    );
                    
                    av_samples_alloc(&outBuffer, nullptr, 2, outSamples, AV_SAMPLE_FMT_S16, 0);
                    
                    int convertedSamples = swr_convert(
                        m_audioSwrCtx,
                        &outBuffer,
                        outSamples,
                        (const uint8_t**)m_audioFrame->data,
                        m_audioFrame->nb_samples
                    );
                    
                    if (convertedSamples > 0) {
                        // 计算PTS（毫秒）
                        qint64 pts = m_audioFrame->pts * av_q2d(m_formatContext->streams[m_audioStreamIndex]->time_base) * 1000;
                        
                        // Seek后的首帧：用实际PTS同步AudioPlayer时钟
                        if (m_seekPending) {
                            AudioPlayer::instance().setCurrentTimestamp(pts);
                            m_seekPending = false;
                            qDebug() << "[MediaSession] Seek completed - clock synced to actual PTS:" << pts << "ms";
                        }
                        
                        // 发送到音频播放器
                        int dataSize = convertedSamples * 2 * 2;  // samples * channels * bytes_per_sample
                        QByteArray audioData(reinterpret_cast<const char*>(outBuffer), dataSize);
                        AudioPlayer::instance().writeAudioData(audioData, pts);
                        
                        if (audioFrameCount == 1) {
                            qDebug() << "[MediaSession] First audio frame sent - pts:" << pts 
                                     << "samples:" << convertedSamples << "size:" << dataSize;
                        }
                    }
                    
                    av_freep(&outBuffer);
                }
            } else {
                static bool errorLogged = false;
                if (!errorLogged) {
                    qWarning() << "[MediaSession] Audio decode error:" << ret;
                    errorLogged = true;
                }
            }
        } 
        else if (pkt->stream_index == m_videoStreamIndex && m_videoSession) {
            // 克隆packet传递给解码器（解码器会负责释放）
            AVPacket* clonedPkt = av_packet_clone(pkt);
            if (clonedPkt) {
                m_videoSession->pushPacket(clonedPkt);
                videoPacketCount++;
                if (videoPacketCount <= 10) {
                    qDebug() << "[MediaSession] Video packet" << videoPacketCount << "sent to decoder";
                }
            }
        } else {
            // 记录未处理的packet
            static int unknownCount = 0;
            unknownCount++;
            if (unknownCount <= 10) {
                qDebug() << "[MediaSession] Unhandled packet - stream_index:" << pkt->stream_index 
                         << "audio_index:" << m_audioStreamIndex << "video_index:" << m_videoStreamIndex;
            }
        }
        
        av_packet_unref(pkt);
    }
    
    av_packet_free(&pkt);
    
    m_demuxRunning = false;  // 标记解复用已停止
    
    qDebug() << "[MediaSession] Demux loop finished - Audio packets:" << audioPacketCount 
             << "Video packets:" << videoPacketCount;
    emit demuxFinished();
}

void MediaSession::setState(PlaybackState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

bool MediaSession::initAudioDecoder(AVStream* stream)
{
    if (!stream) {
        qWarning() << "[MediaSession] Invalid audio stream";
        return false;
    }
    
    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        qWarning() << "[MediaSession] Audio codec not found";
        return false;
    }
    
    // 创建解码器上下文
    m_audioCodecCtx = avcodec_alloc_context3(codec);
    if (!m_audioCodecCtx) {
        qWarning() << "[MediaSession] Failed to allocate audio codec context";
        return false;
    }
    
    // 复制codec参数
    if (avcodec_parameters_to_context(m_audioCodecCtx, stream->codecpar) < 0) {
        qWarning() << "[MediaSession] Failed to copy audio codec parameters";
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }
    
    // 打开解码器
    if (avcodec_open2(m_audioCodecCtx, codec, nullptr) < 0) {
        qWarning() << "[MediaSession] Failed to open audio codec";
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }
    
    // 创建AVFrame
    m_audioFrame = av_frame_alloc();
    if (!m_audioFrame) {
        qWarning() << "[MediaSession] Failed to allocate audio frame";
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }
    
    // 初始化重采样器 (转换为 44100Hz, stereo, s16)
    m_audioSwrCtx = swr_alloc_set_opts(nullptr,
                                       AV_CH_LAYOUT_STEREO,     // 输出：立体声
                                       AV_SAMPLE_FMT_S16,       // 输出：16位整数
                                       44100,                   // 输出：44100Hz
                                       m_audioCodecCtx->channel_layout ? m_audioCodecCtx->channel_layout : AV_CH_LAYOUT_STEREO,
                                       m_audioCodecCtx->sample_fmt,
                                       m_audioCodecCtx->sample_rate,
                                       0, nullptr);
    
    if (!m_audioSwrCtx || swr_init(m_audioSwrCtx) < 0) {
        qWarning() << "[MediaSession] Failed to initialize audio resampler";
        av_frame_free(&m_audioFrame);
        avcodec_free_context(&m_audioCodecCtx);
        if (m_audioSwrCtx) swr_free(&m_audioSwrCtx);
        return false;
    }
    
    qDebug() << "[MediaSession] Audio decoder initialized successfully";
    return true;
}

void MediaSession::cleanupAudioDecoder()
{
    if (m_audioSwrCtx) {
        swr_free(&m_audioSwrCtx);
        m_audioSwrCtx = nullptr;
    }
    
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }
    
    if (m_audioCodecCtx) {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
    }
}
