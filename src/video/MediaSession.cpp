#include "MediaSession.h"
#include "VideoSession.h"
#include "VideoBuffer.h"
#include "VideoRenderer.h"
#include "AudioService.h"
#include "AudioPlayer.h"
#include <QDebug>
#include <QFileInfo>
#include <QPair>
#include <QUuid>
#include <QVector>
#include <QtGlobal>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

MediaSession::MediaSession(QObject* parent)
    : QObject(parent)
    , m_audioSession(nullptr)
    , m_videoSession(nullptr)
    , m_formatContext(nullptr)
    , m_audioStreamIndex(-1)
    , m_videoStreamIndex(-1)
    , m_audioCodecCtx(nullptr)
    , m_audioFrame(nullptr)
    , m_audioFilterGraph(nullptr)
    , m_audioFilterSrcCtx(nullptr)
    , m_audioFilterSinkCtx(nullptr)
    , m_audioOutputSampleRate(44100)
    , m_duration(0)
    , m_state(Stopped)
    , m_playbackRate(1.0)
    , m_syncTimer(nullptr)
    , m_syncEnabled(true)
    , m_masterClock(0)
    , m_demuxThread(nullptr)
    , m_demuxState(DemuxState::Stopped)
    , m_pendingAudioClockSyncMs(std::nullopt)
    , m_audioWriteOwnerId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_positionTimer(nullptr)
{
    qDebug() << "[MediaSession] Created";
    
    m_syncTimer = new QTimer(this);
    m_syncTimer->setInterval(10);
    connect(m_syncTimer, &QTimer::timeout, this, &MediaSession::syncAudioVideo);
    
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
    
    QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    
    if (!initDemuxer(filePath)) {
        qWarning() << "[MediaSession] Failed to init demuxer";
        setState(Error);
        return false;
    }
    
    m_currentUrl = url.toString();
    
    if (m_audioStreamIndex >= 0) {
        qDebug() << "[MediaSession] Initializing audio decoder for stream" << m_audioStreamIndex;
        
        AVStream* audioStream = m_formatContext->streams[m_audioStreamIndex];
        if (!initAudioDecoder(audioStream)) {
            qWarning() << "[MediaSession] Failed to init audio decoder";
        } else {
            qDebug() << "[MediaSession] Audio stream - codec:" 
                     << avcodec_get_name(audioStream->codecpar->codec_id)
                     << "sample_rate:" << audioStream->codecpar->sample_rate;
        }
    }
    
    if (m_videoStreamIndex >= 0) {
        qDebug() << "[MediaSession] Creating VideoSession for stream" << m_videoStreamIndex;
        m_videoSession = new VideoSession(this);
        
        // connect(m_videoSession, &VideoSession::frameRendered, 
        //         this, &MediaSession::onVideoFrameRendered);
        
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
    
    QString title = QFileInfo(filePath).fileName();
    emit metadataReady(title, "", m_duration);
    emit durationChanged(m_duration);
    
    setState(Stopped);
    return true;
}

void MediaSession::unloadSource()
{
    qDebug() << "[MediaSession] Unloading source";
    setPlaybackRate(1.0);
    
    stopDemux();
    
    if (m_audioSession) {
        delete m_audioSession;
        m_audioSession = nullptr;
    }
    
    if (m_videoSession) {
        delete m_videoSession;
        m_videoSession = nullptr;
    }
    
    cleanupAudioDecoder();
    
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

    qint64 resumePos = m_masterClock;
    bool takingOverFromOtherOwner = false;

    // Start or resume audio playback
    if (m_audioCodecCtx) {
        auto& player = AudioPlayer::instance();
        const QString previousOwner = player.writeOwner();
        takingOverFromOtherOwner = !previousOwner.isEmpty() && previousOwner != m_audioWriteOwnerId;
        if (resumePos <= 0 && m_videoSession) {
            const qint64 videoPos = m_videoSession->getCurrentPTS();
            if (videoPos > 0) {
                resumePos = videoPos;
            }
        }
    }

    if (resumePos < 0) {
        resumePos = 0;
    }

    // On cross-owner resume, force a keyframe-aligned seek first to avoid
    // reference-frame corruption and frozen/black frames after resume.
    // If an explicit seek has just happened, skip the extra
    // handoff seek to avoid duplicate demux restart.
    if (m_state == Paused && takingOverFromOtherOwner && m_videoSession && !m_pendingAudioClockSyncMs.has_value()) {
        qDebug() << "[MediaSession] Owner handoff resume, forcing keyframe seek to" << resumePos << "ms";
        seekTo(resumePos);
    }

    if (m_audioCodecCtx) {
        AudioPlayer::instance().setWriteOwner(m_audioWriteOwnerId);
    }

    // Start video decoding/rendering before demux to avoid dropping the first
    // keyframe due to session state races during startup/resume.
    if (m_videoSession) {
        m_videoSession->start();
        qDebug() << "[MediaSession] Video playback started";
    }

    // Start demux loop
    startDemux();

    if (m_audioCodecCtx) {
        auto& player = AudioPlayer::instance();
        player.setPlaybackRate(m_playbackRate.load());

        if (m_state == Paused) {
            if (takingOverFromOtherOwner) {
                player.stop();
                player.resetBuffer();
                player.setCurrentTimestamp(resumePos);
                m_pendingAudioClockSyncMs = resumePos;
                player.start();
                qDebug() << "[MediaSession] Audio playback restarted after owner handoff at" << resumePos << "ms";
            } else if (!player.isPlaying()) {
                player.resetBuffer();
                player.setCurrentTimestamp(resumePos);
                player.start();
                qDebug() << "[MediaSession] Audio playback restarted from paused state";
            } else if (player.isPaused()) {
                player.resume();
                qDebug() << "[MediaSession] Audio playback resumed";
            }
        } else {
            // Ensure we never inherit stale audio clock/state from music playback.
            player.stop();
            player.resetBuffer();
            player.setCurrentTimestamp(0);
            m_pendingAudioClockSyncMs = 0;
            m_masterClock = 0;
            player.start();
            qDebug() << "[MediaSession] Audio playback started (fresh clock)";
        }
    }

    // Start sync loop
    if (m_syncEnabled && hasVideo() && hasAudio()) {
        m_syncTimer->start();
    }

    // Start position update loop
    m_positionTimer->start();

    setState(Playing);
}

void MediaSession::pause()
{
    qDebug() << "[MediaSession] Pause";

    if (m_state != Playing) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
        if (isDemuxRunning()) {
            setDemuxState(DemuxState::Paused);
        }
    }

    // Snapshot current playback clock before pausing so resume can continue
    // from the correct timeline even after owner handoff.
    m_masterClock = getCurrentPosition();

    // Pause audio playback
    if (m_audioCodecCtx) {
        auto& player = AudioPlayer::instance();
        if (player.writeOwner() == m_audioWriteOwnerId) {
            player.pause();
        }
    }

    // Pause video rendering
    if (m_videoSession) {
        m_videoSession->pause();
    }

    // Stop sync & position timers
    m_syncTimer->stop();
    m_positionTimer->stop();

    setState(Paused);
}

void MediaSession::stop()
{
    qDebug() << "[MediaSession] Stop";

    {
        std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
        if (isDemuxRunning()) {
            setDemuxState(DemuxState::Running);
        }
    }
    m_demuxPauseCv.notify_all();

    // Stop demux loop
    stopDemux();

    // Stop audio output
    if (m_audioCodecCtx) {
        auto& player = AudioPlayer::instance();
        if (player.writeOwner() == m_audioWriteOwnerId) {
            player.setPlaybackRate(1.0);
            player.stop();
        }
        player.clearWriteOwner(m_audioWriteOwnerId);
    }

    // Stop video playback
    if (m_videoSession) {
        m_videoSession->stop();
    }

    // Stop timers
    m_syncTimer->stop();
    m_positionTimer->stop();

    setPlaybackRate(1.0);

    setState(Stopped);
}

void MediaSession::seekTo(qint64 positionMs)
{
    qDebug() << "[MediaSession] Seek to:" << positionMs << "ms";
    
    if (!m_formatContext) {
        return;
    }
    
    bool wasRunning = isDemuxRunning();
    if (wasRunning) {
        setDemuxState(DemuxState::Stopped);
        {
            std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
            setDemuxState(DemuxState::Stopped);
        }
        m_demuxPauseCv.notify_all();
        
        if (m_demuxThread && !m_demuxThread->isFinished()) {
            qDebug() << "[MediaSession] Waiting for demux thread to stop...";
            m_demuxThread->wait(1000);
            qDebug() << "[MediaSession] Demux thread stopped";
        }
    }
    
    int64_t timestamp = positionMs * 1000;
    
    // FFmpeg seek
    int ret = av_seek_frame(m_formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        qWarning() << "[MediaSession] Seek failed";
        if (wasRunning) {
            setDemuxState(DemuxState::Running);
        }
        return;
    }
    
    if (m_audioCodecCtx) {
        avcodec_flush_buffers(m_audioCodecCtx);
        AudioPlayer::instance().resetBuffer();
        m_pendingAudioClockSyncMs = positionMs;
        {
            std::lock_guard<std::mutex> lock(m_audioResampleMutex);
            recreateAudioResampler();
        }
        qDebug() << "[MediaSession] Audio buffers flushed, waiting for first frame to sync clock";
    }
    
    if (m_videoSession) {
        m_videoSession->flush();
        qDebug() << "[MediaSession] Video buffers flushed";
        
        if (m_videoSession->videoRenderer()) {
            QMetaObject::invokeMethod(m_videoSession->videoRenderer(), "startBuffering");
        }
    }
    
    m_masterClock = positionMs;
    
    if (wasRunning) {
        setDemuxState(DemuxState::Running);
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

void MediaSession::setPlaybackRate(double rate)
{
    const double clampedRate = qBound(0.5, rate, 2.0);
    const double currentRate = m_playbackRate.load();
    if (qFuzzyCompare(currentRate, clampedRate)) {
        return;
    }

    m_playbackRate.store(clampedRate);

    if (m_audioCodecCtx) {
        auto& player = AudioPlayer::instance();
        if (player.writeOwner() == m_audioWriteOwnerId) {
            player.setPlaybackRate(clampedRate);
        }

        std::lock_guard<std::mutex> lock(m_audioResampleMutex);
        if (!recreateAudioResampler()) {
            qWarning() << "[MediaSession] Failed to apply playback rate, audio resampler unavailable";
        }
    }

    emit playbackRateChanged(clampedRate);
    qDebug() << "[MediaSession] Playback rate set to" << clampedRate << "x";
}

double MediaSession::playbackRate() const
{
    return m_playbackRate.load();
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
    if (m_audioCodecCtx) {
        const auto& player = AudioPlayer::instance();
        if (player.writeOwner() == m_audioWriteOwnerId && player.isPlaying()) {
            return player.getPlaybackPosition();
        }
    }
    
    if (m_videoSession) {
        qint64 videoPTS = m_videoSession->getCurrentPTS();
        if (videoPTS > 0) {
            return videoPTS;
        }
    }
    
    return m_masterClock;
}

void MediaSession::onAudioPositionChanged(qint64 audioPos)
{
    m_masterClock = audioPos;
    
    emit positionChanged(audioPos);
}

void MediaSession::onVideoFrameRendered(qint64 videoPts)
{
    Q_UNUSED(videoPts);
}

void MediaSession::syncAudioVideo()
{
    if (!m_syncEnabled || !m_videoSession || !m_audioCodecCtx) {
        return;
    }
    
    qint64 audioPos = m_masterClock;
    
    // TODO: qint64 videoPos = m_videoSession->getCurrentPTS();
    qint64 videoPos = audioPos;
    
    int offset = videoPos - audioPos;
    
    if (qAbs(offset) > 40) {
        qDebug() << "[MediaSession] Sync error:" << offset << "ms (audio:" << audioPos << "video:" << videoPos << ")";
        
        if (offset > 0) {
            // TODO: m_videoSession->holdFrame();
        } else {
            // TODO: m_videoSession->skipNonKeyFrames();
        }
        
        emit syncError(offset);
    }
}

void MediaSession::updatePosition()
{
    if (m_state == Playing) {
        qint64 pos = getCurrentPosition();
        if (pos > 0) {
            m_masterClock = pos;
        }
        emit positionChanged(pos);
    }
}

bool MediaSession::initDemuxer(const QString& filePath)
{
    int ret = avformat_open_input(&m_formatContext, filePath.toUtf8().data(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "[MediaSession] Failed to open input:" << errbuf;
        return false;
    }
    
    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to find stream info";
        avformat_close_input(&m_formatContext);
        return false;
    }
    
    m_audioStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, 
                                              -1, -1, nullptr, 0);
    
    m_videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, 
                                              -1, -1, nullptr, 0);
    
    qDebug() << "[MediaSession] Stream indices - Audio:" << m_audioStreamIndex 
             << "Video:" << m_videoStreamIndex;
    
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = m_formatContext->duration / 1000;
    } else {
        m_duration = 0;
    }
    
    qDebug() << "[MediaSession] Duration:" << m_duration << "ms";
    
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

    // 线程对象存在且仍在运行：根据状态决定“恢复”还是“已在运行”。
    if (m_demuxThread && !m_demuxThread->isFinished()) {
        if (isDemuxPaused()) {
            qDebug() << "[MediaSession] Resuming demux thread";
            {
                std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
                setDemuxState(DemuxState::Running);
            }
            m_demuxPauseCv.notify_all();
        } else {
            qDebug() << "[MediaSession] Demux already running";
        }
        return;
    }

    // 线程对象已结束，清理悬挂指针并重置状态。
    if (m_demuxThread && m_demuxThread->isFinished()) {
        qDebug() << "[MediaSession] Previous demux thread finished, will restart";
        m_demuxThread = nullptr;
        setDemuxState(DemuxState::Stopped);
    }

    qDebug() << "[MediaSession] Starting demux thread";
    setDemuxState(DemuxState::Running);

    m_demuxThread = QThread::create([this]() {
        this->demuxLoop();
    });

    connect(m_demuxThread, &QThread::finished, m_demuxThread, &QThread::deleteLater);
    m_demuxThread->start();
}

void MediaSession::stopDemux()
{
    if (!isDemuxRunning()) {
        return;
    }

    qDebug() << "[MediaSession] Stopping demux thread";

    setDemuxState(DemuxState::Stopped);
    {
        std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
        setDemuxState(DemuxState::Stopped);
    }
    m_demuxPauseCv.notify_all();

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
    
    while (isDemuxRunning()) {
        {
            std::unique_lock<std::mutex> lock(m_demuxPauseMutex);
            if (isDemuxPaused()) {
                m_demuxPauseCv.wait(lock, [this]() {
                    return !isDemuxPaused() || !isDemuxRunning();
                });
            }
        }

        if (!isDemuxRunning()) {
            break;
        }
        int audioBufferedBytes = 0;
        if (m_audioCodecCtx) {
            AudioBuffer* audioBuffer = AudioPlayer::instance().getBuffer();
            if (audioBuffer) {
                audioBufferedBytes = audioBuffer->availableBytes();
            }
        }
        constexpr int kAudioLowWaterBytes = 96 * 1024;    // ~0.55s PCM
        constexpr int kAudioHighWaterBytes = 384 * 1024;  // ~2.2s PCM

        // 音频背压：优先限制“读太快跑到EOF”，而不是依赖大容量环形缓冲百分比。
        if (audioBufferedBytes >= kAudioHighWaterBytes) {
            QThread::msleep(4);
            continue;
        }

        bool shouldThrottleVideo = false;
        if (m_videoSession && m_videoSession->videoBuffer()) {
            const int bufferSize = m_videoSession->videoBuffer()->size();
            const int bufferCapacity = m_videoSession->videoBuffer()->capacity();
            shouldThrottleVideo = (bufferCapacity > 0 && bufferSize >= bufferCapacity * 0.8);
        }

        // 当视频缓冲已高位且音频有最低安全余量时，暂停 demux 让渲染追上。
        // 这里不用“丢视频包”，避免 H264 包序破坏造成画面卡顿。
        if (shouldThrottleVideo && audioBufferedBytes >= kAudioLowWaterBytes) {
            QThread::msleep(3);
            continue;
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
        
        if (pkt->stream_index == m_audioStreamIndex && m_audioCodecCtx) {
            audioPacketCount++;
            int ret = avcodec_send_packet(m_audioCodecCtx, pkt);
            if (ret == 0) {
                static int audioFrameCount = 0;
                while (avcodec_receive_frame(m_audioCodecCtx, m_audioFrame) == 0) {
                    audioFrameCount++;
                    if (audioFrameCount % 100 == 0) {
                        qDebug() << "[MediaSession] Decoded" << audioFrameCount << "audio frames";
                    }

                    const int64_t rawPts = (m_audioFrame->best_effort_timestamp == AV_NOPTS_VALUE)
                        ? m_audioFrame->pts
                        : m_audioFrame->best_effort_timestamp;

                    qint64 inputPtsMs = 0;
                    if (rawPts != AV_NOPTS_VALUE) {
                        inputPtsMs = static_cast<qint64>(
                            rawPts * av_q2d(m_formatContext->streams[m_audioStreamIndex]->time_base) * 1000.0
                        );
                    }

                    if (m_pendingAudioClockSyncMs.has_value()) {
                        AudioPlayer::instance().setCurrentTimestamp(inputPtsMs);
                        m_pendingAudioClockSyncMs.reset();
                        qDebug() << "[MediaSession] Seek completed - clock synced to actual PTS:" << inputPtsMs << "ms";
                    }

                    QVector<QPair<QByteArray, qint64>> outputFrames;
                    {
                        std::lock_guard<std::mutex> lock(m_audioResampleMutex);
                        if (!m_audioFilterSrcCtx || !m_audioFilterSinkCtx) {
                            continue;
                        }

                        const int addRet = av_buffersrc_add_frame_flags(
                            m_audioFilterSrcCtx,
                            m_audioFrame,
                            AV_BUFFERSRC_FLAG_KEEP_REF
                        );
                        if (addRet < 0) {
                            qWarning() << "[MediaSession] Failed to push frame to audio filter graph:" << addRet;
                            continue;
                        }

                        AVFrame* filteredFrame = av_frame_alloc();
                        if (!filteredFrame) {
                            qWarning() << "[MediaSession] Failed to allocate filtered audio frame";
                            continue;
                        }

                        while (true) {
                            const int sinkRet = av_buffersink_get_frame(m_audioFilterSinkCtx, filteredFrame);
                            if (sinkRet == AVERROR(EAGAIN) || sinkRet == AVERROR_EOF) {
                                break;
                            }
                            if (sinkRet < 0) {
                                qWarning() << "[MediaSession] Failed to pull frame from audio filter graph:" << sinkRet;
                                break;
                            }

                            const int dataSize = av_samples_get_buffer_size(
                                nullptr, 2, filteredFrame->nb_samples, AV_SAMPLE_FMT_S16, 1
                            );
                            if (dataSize > 0 && filteredFrame->data[0]) {
                                qint64 filteredPtsMs = inputPtsMs;
                                if (filteredFrame->pts != AV_NOPTS_VALUE) {
                                    const AVRational sinkTimeBase = av_buffersink_get_time_base(m_audioFilterSinkCtx);
                                    filteredPtsMs = av_rescale_q(filteredFrame->pts, sinkTimeBase, AVRational{1, 1000});
                                }
                                outputFrames.push_back(qMakePair(
                                    QByteArray(reinterpret_cast<const char*>(filteredFrame->data[0]), dataSize),
                                    filteredPtsMs
                                ));
                            }

                            av_frame_unref(filteredFrame);
                        }

                        av_frame_free(&filteredFrame);
                    }

                    for (const auto& frameData : outputFrames) {
                        AudioPlayer::instance().writeAudioData(frameData.first, frameData.second, m_audioWriteOwnerId);
                    }

                    if (audioFrameCount == 1 && !outputFrames.isEmpty()) {
                        qDebug() << "[MediaSession] First audio frame sent - pts:" << outputFrames.first().second
                                 << "size:" << outputFrames.first().first.size();
                    }
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
            AVPacket* clonedPkt = av_packet_clone(pkt);
            if (clonedPkt) {
                m_videoSession->pushPacket(clonedPkt);
                videoPacketCount++;
                if (videoPacketCount <= 10) {
                    qDebug() << "[MediaSession] Video packet" << videoPacketCount << "sent to decoder";
                }
            }
        } else if (pkt->stream_index == m_audioStreamIndex) {
            // 音频解码链初始化失败时，静默丢弃音频包，避免刷屏日志干扰视频播放。
        } else {
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
    
    setDemuxState(DemuxState::Stopped);
    
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
    
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        qWarning() << "[MediaSession] Audio codec not found";
        return false;
    }
    
    m_audioCodecCtx = avcodec_alloc_context3(codec);
    if (!m_audioCodecCtx) {
        qWarning() << "[MediaSession] Failed to allocate audio codec context";
        return false;
    }
    
    if (avcodec_parameters_to_context(m_audioCodecCtx, stream->codecpar) < 0) {
        qWarning() << "[MediaSession] Failed to copy audio codec parameters";
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }
    
    if (avcodec_open2(m_audioCodecCtx, codec, nullptr) < 0) {
        qWarning() << "[MediaSession] Failed to open audio codec";
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }
    
    m_audioFrame = av_frame_alloc();
    if (!m_audioFrame) {
        qWarning() << "[MediaSession] Failed to allocate audio frame";
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }

    if (!recreateAudioResampler()) {
        qWarning() << "[MediaSession] Failed to initialize audio resampler";
        av_frame_free(&m_audioFrame);
        avcodec_free_context(&m_audioCodecCtx);
        return false;
    }
    
    qDebug() << "[MediaSession] Audio decoder initialized successfully";
    return true;
}

void MediaSession::cleanupAudioDecoder()
{
    cleanupAudioFilterGraph();
    
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }
    
    if (m_audioCodecCtx) {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
    }

    m_audioOutputSampleRate = 44100;
}

void MediaSession::cleanupAudioFilterGraph()
{
    m_audioFilterSrcCtx = nullptr;
    m_audioFilterSinkCtx = nullptr;
    if (m_audioFilterGraph) {
        avfilter_graph_free(&m_audioFilterGraph);
        m_audioFilterGraph = nullptr;
    }
}

bool MediaSession::recreateAudioResampler()
{
    if (!m_audioCodecCtx) {
        return false;
    }

    cleanupAudioFilterGraph();

    const char* sampleFmtName = av_get_sample_fmt_name(m_audioCodecCtx->sample_fmt);
    if (!sampleFmtName) {
        qWarning() << "[MediaSession] Invalid input audio sample format";
        return false;
    }

    const uint64_t inputLayout = m_audioCodecCtx->channel_layout
        ? m_audioCodecCtx->channel_layout
        : static_cast<uint64_t>(av_get_default_channel_layout(m_audioCodecCtx->channels));
    if (inputLayout == 0 || m_audioCodecCtx->sample_rate <= 0) {
        qWarning() << "[MediaSession] Invalid input audio parameters for filter graph";
        return false;
    }

    const double rate = qBound(0.5, m_playbackRate.load(), 2.0);
    m_audioOutputSampleRate = 44100;

    m_audioFilterGraph = avfilter_graph_alloc();
    if (!m_audioFilterGraph) {
        qWarning() << "[MediaSession] Failed to allocate audio filter graph";
        return false;
    }

    const AVFilter* abuffer = avfilter_get_by_name("abuffer");
    const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffer || !abuffersink) {
        qWarning() << "[MediaSession] Required audio filters not found";
        cleanupAudioFilterGraph();
        return false;
    }

    const QString sourceArgs = QStringLiteral("time_base=1/%1:sample_rate=%1:sample_fmt=%2:channel_layout=0x%3")
        .arg(m_audioCodecCtx->sample_rate)
        .arg(QString::fromLatin1(sampleFmtName))
        .arg(QString::number(inputLayout, 16));
    const QByteArray sourceArgsUtf8 = sourceArgs.toUtf8();

    int ret = avfilter_graph_create_filter(
        &m_audioFilterSrcCtx,
        abuffer,
        "in",
        sourceArgsUtf8.constData(),
        nullptr,
        m_audioFilterGraph
    );
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to create abuffer:" << ret;
        cleanupAudioFilterGraph();
        return false;
    }

    ret = avfilter_graph_create_filter(
        &m_audioFilterSinkCtx,
        abuffersink,
        "out",
        nullptr,
        nullptr,
        m_audioFilterGraph
    );
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to create abuffersink:" << ret;
        cleanupAudioFilterGraph();
        return false;
    }

    const int64_t sinkSampleFmts[] = { AV_SAMPLE_FMT_S16, -1 };
    const int64_t sinkChannelLayouts[] = { static_cast<int64_t>(AV_CH_LAYOUT_STEREO), -1 };
    const int64_t sinkSampleRates[] = { 44100, -1 };

    ret = av_opt_set_int_list(
        m_audioFilterSinkCtx, "sample_fmts", sinkSampleFmts, -1, AV_OPT_SEARCH_CHILDREN
    );
    ret = (ret < 0) ? ret : av_opt_set_int_list(
        m_audioFilterSinkCtx, "channel_layouts", sinkChannelLayouts, -1, AV_OPT_SEARCH_CHILDREN
    );
    ret = (ret < 0) ? ret : av_opt_set_int_list(
        m_audioFilterSinkCtx, "sample_rates", sinkSampleRates, -1, AV_OPT_SEARCH_CHILDREN
    );
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to configure abuffersink:" << ret;
        cleanupAudioFilterGraph();
        return false;
    }

    const QString filterDesc = QStringLiteral(
        "aresample=44100,atempo=%1,aformat=sample_fmts=s16:channel_layouts=stereo:sample_rates=44100"
    ).arg(QString::number(rate, 'f', 3));
    const QByteArray filterDescUtf8 = filterDesc.toUtf8();

    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    if (!outputs || !inputs) {
        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
        qWarning() << "[MediaSession] Failed to allocate filter graph endpoints";
        cleanupAudioFilterGraph();
        return false;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = m_audioFilterSrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = m_audioFilterSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    ret = avfilter_graph_parse_ptr(m_audioFilterGraph, filterDescUtf8.constData(), &inputs, &outputs, nullptr);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to parse filter graph:" << ret;
        cleanupAudioFilterGraph();
        return false;
    }

    ret = avfilter_graph_config(m_audioFilterGraph, nullptr);
    if (ret < 0) {
        qWarning() << "[MediaSession] Failed to config filter graph:" << ret;
        cleanupAudioFilterGraph();
        return false;
    }

    qDebug() << "[MediaSession] Audio filter graph rebuilt, playbackRate:" << rate
             << "chain:" << filterDesc;
    return true;
}
