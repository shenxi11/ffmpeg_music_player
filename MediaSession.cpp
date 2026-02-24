#include "MediaSession.h"
#include "VideoSession.h"
#include "VideoBuffer.h"
#include "VideoRenderer.h"
#include "AudioService.h"
#include "AudioPlayer.h"
#include <QDebug>
#include <QFileInfo>
#include <QUuid>

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
    , m_demuxPaused(false)
    , m_seekPending(false)
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

    {
        std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
        m_demuxPaused = false;
    }
    m_demuxPauseCv.notify_all();

    // Start demux loop
    startDemux();

    // Start or resume audio playback
    if (m_audioCodecCtx) {
        AudioPlayer::instance().setWriteOwner(m_audioWriteOwnerId);
        if (m_state == Paused) {
            AudioPlayer::instance().resume();
            qDebug() << "[MediaSession] Audio playback resumed";
        } else {
            // Fresh playback should discard stale packets from previous source.
            AudioPlayer::instance().resetBuffer();
            AudioPlayer::instance().start();
            qDebug() << "[MediaSession] Audio playback started";
        }
    }

    // Start video rendering/decoding
    if (m_videoSession) {
        m_videoSession->start();
        qDebug() << "[MediaSession] Video playback started";
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
        m_demuxPaused = true;
    }

    // Pause audio playback
    if (m_audioCodecCtx) {
        AudioPlayer::instance().pause();
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
        m_demuxPaused = false;
    }
    m_demuxPauseCv.notify_all();

    // Stop demux loop
    stopDemux();

    // Stop audio output
    if (m_audioCodecCtx) {
        AudioPlayer::instance().stop();
        AudioPlayer::instance().clearWriteOwner(m_audioWriteOwnerId);
    }

    // Stop video playback
    if (m_videoSession) {
        m_videoSession->stop();
    }

    // Stop timers
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
    
    bool wasRunning = m_demuxRunning;
    if (wasRunning) {
        m_demuxRunning = false;
        {
            std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
            m_demuxPaused = false;
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
            m_demuxRunning = true;
        }
        return;
    }
    
    if (m_audioCodecCtx) {
        avcodec_flush_buffers(m_audioCodecCtx);
        AudioPlayer::instance().resetBuffer();
        m_seekPending = true;
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
        m_demuxRunning = true;
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
    if (m_audioCodecCtx && AudioPlayer::instance().isPlaying()) {
        return AudioPlayer::instance().getPlaybackPosition();
    }
    
    if (m_videoSession) {
        qint64 videoPTS = m_videoSession->getCurrentPTS();
        return videoPTS;
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
    
    if (m_demuxRunning) {
        if (m_demuxThread && m_demuxThread->isFinished()) {
            qDebug() << "[MediaSession] Previous demux thread finished, will restart";
            m_demuxRunning = false;
            m_demuxThread = nullptr;
        } else {
            qDebug() << "[MediaSession] Demux already running";
            return;
        }
    }
    
    qDebug() << "[MediaSession] Starting demux thread";
    
    m_demuxRunning = true;
    
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
    {
        std::lock_guard<std::mutex> lock(m_demuxPauseMutex);
        m_demuxPaused = false;
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
    
    while (m_demuxRunning) {
        {
            std::unique_lock<std::mutex> lock(m_demuxPauseMutex);
            if (m_demuxPaused) {
                m_demuxPauseCv.wait(lock, [this]() {
                    return !m_demuxPaused || !m_demuxRunning;
                });
            }
        }

        if (!m_demuxRunning) {
            break;
        }
        if (m_videoSession && m_videoSession->videoBuffer()) {
            int bufferSize = m_videoSession->videoBuffer()->size();
            int bufferCapacity = m_videoSession->videoBuffer()->capacity();
            
            if (bufferSize >= bufferCapacity * 0.8) {
                QThread::msleep(10);
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
        
        if (pkt->stream_index == m_audioStreamIndex && m_audioCodecCtx) {
            int ret = avcodec_send_packet(m_audioCodecCtx, pkt);
            if (ret == 0) {
                static int audioFrameCount = 0;
                while (avcodec_receive_frame(m_audioCodecCtx, m_audioFrame) == 0) {
                    audioFrameCount++;
                    if (audioFrameCount % 100 == 0) {
                        qDebug() << "[MediaSession] Decoded" << audioFrameCount << "audio frames";
                    }
                    
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
                        qint64 pts = m_audioFrame->pts * av_q2d(m_formatContext->streams[m_audioStreamIndex]->time_base) * 1000;
                        
                        if (m_seekPending) {
                            AudioPlayer::instance().setCurrentTimestamp(pts);
                            m_seekPending = false;
                            qDebug() << "[MediaSession] Seek completed - clock synced to actual PTS:" << pts << "ms";
                        }
                        
                        int dataSize = convertedSamples * 2 * 2;  // samples * channels * bytes_per_sample
                        QByteArray audioData(reinterpret_cast<const char*>(outBuffer), dataSize);
                        AudioPlayer::instance().writeAudioData(audioData, pts, m_audioWriteOwnerId);
                        
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
            AVPacket* clonedPkt = av_packet_clone(pkt);
            if (clonedPkt) {
                m_videoSession->pushPacket(clonedPkt);
                videoPacketCount++;
                if (videoPacketCount <= 10) {
                    qDebug() << "[MediaSession] Video packet" << videoPacketCount << "sent to decoder";
                }
            }
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
    
    m_demuxRunning = false;
    
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
    
    m_audioSwrCtx = swr_alloc_set_opts(nullptr,
                                       AV_CH_LAYOUT_STEREO,
                                       AV_SAMPLE_FMT_S16,
                                       44100,
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

