#include "AudioSession.h"
#include <chrono>
#include <QFileInfo>

AudioSession::AudioSession(const QString& sessionId, QObject* parent)
    : QObject(parent),
      m_sessionId(sessionId),
      m_active(false),
      m_decoder(new AudioDecoder(this)),
      m_player(&AudioPlayer::instance()),  // 使用全局单例AudioPlayer
      m_duration(0),
      m_isBuffering(false),
      m_bufferingPercent(0),
      m_decoderPausedByFlowControl(false),
      m_seekGracePeriod(false),
      m_seekGraceTimer(new QTimer(this)),
      m_isSeeking(false)
{
    m_seekGraceTimer->setSingleShot(true);
    connect(m_seekGraceTimer, &QTimer::timeout, this, [this]() {
        m_seekGracePeriod = false;
        m_isSeeking = false;  // 宽限期结束时才清除seek标志
        
        // 清除流控暂停标志，并确保解码器处于正确状态
        if (m_decoderPausedByFlowControl) {
            m_decoderPausedByFlowControl = false;
        }
        
        // 检查buffer状态，确保解码器在合理的buffer水平下运行
        int fillLevel = m_player->bufferFillLevel();
        if (fillLevel < 85 && m_decoder->isDecoding()) {
            // Buffer未满，确保解码器运行
            m_decoder->startDecode();
            qDebug() << "AudioSession: Ensuring decoder is running after seek, buffer at" << fillLevel << "%";
        }
        
        qDebug() << "AudioSession: Seek grace period ended";
    });
    
    connectSignals();
}

AudioSession::~AudioSession()
{
    cleanup();
}

bool AudioSession::loadSource(const QUrl& url)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    m_sourceUrl = url;

    // 从URL提取歌曲名称
    QString filePath;
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    } else {
        filePath = url.toString();
    }

    // 提取文件名作为标题（去除扩展名）
    QFileInfo fileInfo(filePath);
    m_title = fileInfo.completeBaseName();  // 不包含扩展名的文件名
    if (m_title.isEmpty()) {
        m_title = fileInfo.fileName();  // 如果completeBaseName为空，使用完整文件名
    }

    // 艺术家默认为空，后续可以从元数据中提取
    m_artist = "";

    qDebug() << "AudioSession: Extracted title from URL:" << m_title;

    bool result = false;
    if (url.isLocalFile()) {
        // 本地文件
        result = m_decoder->initDecoder(url.toLocalFile());
    } else if (url.scheme().startsWith("http")) {
        // 网络URL（HTTP/HTTPS）
        qDebug() << "AudioSession: Loading network URL:" << url.toString();
        result = m_decoder->initDecoder(url.toString());
    } else {
        qDebug() << "AudioSession: Unsupported URL scheme:" << url.scheme();
    }

    // 同步获取解码器提取的元数据（标题和艺术家）
    // 这样可以确保在 playbackStarted 信号发出时，元数据已经可用
    if (result) {
        QString extractedTitle = m_decoder->extractedTitle();
        QString extractedArtist = m_decoder->extractedArtist();

        // 如果从文件元数据中提取到了标题，则使用它
        if (!extractedTitle.isEmpty()) {
            m_title = extractedTitle;
            qDebug() << "AudioSession: Using title from file metadata:" << m_title;
        }

        // 如果从文件元数据中提取到了艺术家，则使用它
        if (!extractedArtist.isEmpty()) {
            m_artist = extractedArtist;
            qDebug() << "AudioSession: Using artist from file metadata:" << m_artist;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioSession::loadSource (initDecoder):"
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms"
             << "result:" << result;

    return result;
}

void AudioSession::play()
{
    auto t0 = std::chrono::high_resolution_clock::now();
    
    if (!m_active) {
        m_active = true;
        emit sessionStarted();
    }
    
    // 注意：resetBuffer由AudioService在switchToSession中调用
    // 这里不再调用，避免时序问题

    // 声明当前会话为共享AudioPlayer的写入者，防止与视频会话串流
    m_player->setWriteOwner(m_sessionId);
    
    // 先启动解码器预填充缓冲区，但不立即播放
    m_decoder->startDecode();
    
    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioSession::play - startDecode call:" 
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";
    
    // 设置初始缓冲标志，等待缓冲区达到一定水平再开始播放
    m_isBuffering = true;
    
    // 注意：播放器会在 checkBufferLevel() 中检测到缓冲区达到25%时自动开始播放
}

void AudioSession::pause()
{
    m_decoder->pauseDecode();
    m_player->pause();
    // sessionPaused 信号由 onPlaybackPaused 发射
}

void AudioSession::resume()
{
    // 恢复时重新声明写入所有权
    m_player->setWriteOwner(m_sessionId);

    // 恢复解码
    m_decoder->startDecode();
    // 恢复播放
    m_player->resume();
    // sessionResumed 信号由 onPlaybackResumed 发射
}

void AudioSession::stop()
{
    m_decoder->stopDecode();
    m_player->stop();
    m_player->clearWriteOwner(m_sessionId);
    m_active = false;
    emit sessionStopped();
}

void AudioSession::seekTo(qint64 positionMs)
{
    if (!m_decoder || !m_player) {
        qDebug() << "AudioSession::seekTo - Session is being destroyed, ignoring seek request";
        return;
    }
    
    qDebug() << "AudioSession: Seeking to" << positionMs << "ms";
    
    // 设置seek标志，避免内部pause/resume操作误触发UI状态更新
    m_isSeeking = true;
    
    // 记录是否在播放
    bool wasPlaying = m_player->isPlaying() && !m_player->isPaused();
    
    // 暂停播放和解码
    if (wasPlaying && m_decoder) {
        m_decoder->pauseDecode();
        m_player->pause();
    }
    
    // 清空播放器缓冲区（不停止线程），保留已扩展的容量以容纳大帧
    AudioBuffer* buffer = m_player->getBuffer();
    if (buffer) {
        buffer->clear();  // 只清除数据，保留容量避免频繁扩展
    }
    
    // 立即设置播放器的当前时间戳，确保进度条显示准确
    // 注意：必须在resume之前设置，否则resume会用旧的pausedPosition覆盖
    m_player->setCurrentTimestamp(positionMs);
    
    // 重置buffering状态，避免seek后误判
    m_isBuffering = false;
    
    // 启动seek宽限期（800ms），在此期间不检查buffering（FLAC等大音频需要更多时间）
    m_seekGracePeriod = true;
    if (m_seekGraceTimer) {
        m_seekGraceTimer->start(800);
    }
    
    // 执行解码器跳转
    if (m_decoder) {
        m_decoder->seekTo(positionMs);
    }
    
    // 立即启动解码，快速填充buffer
    if (m_decoder) {
        m_decoder->startDecode();
    }
    
    // 如果之前在播放，恢复播放（buffering逻辑会自动处理）
    if (wasPlaying && m_player) {
        m_player->resume();
    }
    
    // 注意：m_isSeeking标志会在宽限期结束时自动清除（QTimer timeout）
    
    qDebug() << "AudioSession: Seek completed";
}

void AudioSession::setVolume(int volume)
{
    m_player->setVolume(volume);
}

bool AudioSession::isPlaying() const
{
    return m_player->isPlaying();
}

bool AudioSession::isPaused() const
{
    return m_player->isPaused();
}

qint64 AudioSession::duration() const
{
    return m_duration;
}

qint64 AudioSession::position() const
{
    return m_player->getCurrentTimestamp();
}

void AudioSession::onDecodedData(const QByteArray& data, qint64 timestampMs)
{
    // 检查buffer是否已满（>85%），暂停解码等待消耗
    int fillLevel = m_player->bufferFillLevel();
    if (fillLevel > 85 && !m_decoderPausedByFlowControl && m_decoder->isDecoding()) {
        m_decoder->pauseDecode();
        m_decoderPausedByFlowControl = true;
        qDebug() << "AudioSession: Buffer full (" << fillLevel << "%), pausing decoder";
    }
    
    // 将解码数据传递给播放器
    m_player->writeAudioData(data, timestampMs, m_sessionId);
    
    // 如果正在缓冲，检查是否可以开始播放
    // 初始播放需要 40% 即可开始（网络音频需要更多缓冲），中途缓冲需要 85% 才恢复
    int requiredLevel = (!m_player->isPlaying() && m_active) ? 40 : 85;
    
    if (m_isBuffering && fillLevel >= requiredLevel) {
        m_isBuffering = false;
        
        // 如果播放器还没启动（初始播放），则启动播放器
        if (!m_player->isPlaying() && m_active) {
            m_player->start();
            qDebug() << "AudioSession: Initial playback started (onDataDecoded), fill level:" << fillLevel << "%";
        } else {
            m_player->resume();
            qDebug() << "AudioSession: Buffer filled, resuming playback";
        }
        
        emit bufferingFinished();
    }
}

void AudioSession::onMetadataReady(qint64 durationMs, int sampleRate, int channels)
{
    m_duration = durationMs;
    emit durationChanged(durationMs);
    emit metadataReady(m_title, m_artist, durationMs);
}

void AudioSession::onAudioTagsReady(const QString& title, const QString& artist)
{
    // 如果从文件元数据中提取到了标题和艺术家，则使用它们
    // 否则保留从URL提取的标题
    if (!title.isEmpty()) {
        m_title = title;
        qDebug() << "AudioSession: Using title from file metadata:" << m_title;
    }
    if (!artist.isEmpty()) {
        m_artist = artist;
        qDebug() << "AudioSession: Using artist from file metadata:" << m_artist;
    }

    // 重新发送元数据信号，因为标题和艺术家可能已更新
    emit metadataReady(m_title, m_artist, m_duration);
}

void AudioSession::onAlbumArtReady(const QString& imagePath)
{
    m_albumArt = imagePath;
    emit albumArtReady(imagePath);
}

void AudioSession::onDecodeError(const QString& error)
{
    emit sessionError(error);
}

void AudioSession::onPlaybackError(const QString& error)
{
    emit sessionError(error);
}

void AudioSession::onPositionChanged(qint64 positionMs)
{
    emit positionChanged(positionMs);
    
    // 检查是否播放完成（允许100ms误差）
    if (m_duration > 0 && positionMs >= m_duration - 100) {
        onPlaybackFinished();
    }
}

void AudioSession::onBufferStatusChanged(int fillLevel)
{
    m_bufferingPercent = fillLevel;
    
    // seek宽限期内不检查buffering，给解码器时间填充buffer
    if (m_seekGracePeriod) {
        return;
    }
    
    // Buffer消耗到<60%且解码器被流控暂停时，恢复解码器
    if (fillLevel < 60 && m_decoderPausedByFlowControl && m_decoder->isDecoding()) {
        m_decoder->startDecode();
        m_decoderPausedByFlowControl = false;
        qDebug() << "AudioSession: Buffer low (" << fillLevel << "%), resuming decoder";
    }
    
    // 缓冲区低于10%时，暂停播放等待缓冲(降低阈值减少触发频率)
    if (fillLevel < 10 && m_player->isPlaying() && !m_isBuffering) {
        m_isBuffering = true;
        m_player->pause();
        emit bufferingStarted();
        qDebug() << "AudioSession: Buffering started, fill level:" << fillLevel << "%";
    }
    // 缓冲区恢复到25%以上时，恢复播放和解码（降低阈值快速恢复）
    else if (fillLevel >= 25 && m_isBuffering) {
        m_isBuffering = false;
        
        // 如果播放器还没启动（初始播放），则启动播放器
        if (!m_player->isPlaying() && m_active) {
            m_player->start();
            qDebug() << "AudioSession: Initial playback started, fill level:" << fillLevel << "%";
        } else {
            m_player->resume();
        }
        
        // 确保解码器恢复
        m_decoder->startDecode();
        emit bufferingFinished();
        qDebug() << "AudioSession: Buffering finished, fill level:" << fillLevel << "%";
    }
}

void AudioSession::onBufferUnderrun()
{
    qDebug() << "AudioSession: Buffer underrun detected";
    
    // 缓冲区饥饿时自动暂停等待数据
    if (!m_isBuffering) {
        m_isBuffering = true;
        m_player->pause();
        emit bufferingStarted();
    }
}

void AudioSession::onDecodeCompleted()
{
    qDebug() << "AudioSession: Decode completed";
    emit decodeFinished();
    
    // 注意：不要立即停止播放，等待缓冲区数据播放完毕
    // 播放完成由 onPositionChanged 检测
}

void AudioSession::onDecodeStarted()
{
    qDebug() << "AudioSession: Decode started";
}

void AudioSession::onDecodePaused()
{
    qDebug() << "AudioSession: Decode paused";
}

void AudioSession::onDecodeStopped()
{
    qDebug() << "AudioSession: Decode stopped";
}

void AudioSession::onPlaybackStarted()
{
    qDebug() << "AudioSession: Playback started";
}

void AudioSession::onPlaybackPaused()
{
    qDebug() << "AudioSession: Playback paused";
    // seek期间或buffering期间的暂停是内部操作，不通知UI
    if (!m_isSeeking && !m_isBuffering) {
        emit sessionPaused();
    }
}

void AudioSession::onPlaybackResumed()
{
    qDebug() << "AudioSession: Playback resumed";
    // seek期间或buffering恢复的是内部操作，不通知UI
    if (!m_isSeeking && !m_isBuffering) {
        emit sessionResumed();
    }
}

void AudioSession::onPlaybackStopped()
{
    qDebug() << "AudioSession: Playback stopped";
}

void AudioSession::onPlaybackFinished()
{
    qDebug() << "AudioSession: Playback finished";
    
    // 停止解码器和播放器
    m_decoder->stopDecode();
    m_player->stop();
    
    m_active = false;
    emit sessionFinished();
}

void AudioSession::connectSignals()
{
    // 连接解码器信号 - 显式使用Qt::QueuedConnection，因为AudioDecoder在std::thread中emit
    connect(m_decoder, &AudioDecoder::decodedData, this, &AudioSession::onDecodedData, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::metadataReady, this, &AudioSession::onMetadataReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::audioTagsReady, this, &AudioSession::onAudioTagsReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::albumArtReady, this, &AudioSession::onAlbumArtReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeError, this, &AudioSession::onDecodeError, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeCompleted, this, &AudioSession::onDecodeCompleted, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeStarted, this, &AudioSession::onDecodeStarted, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodePaused, this, &AudioSession::onDecodePaused, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeStopped, this, &AudioSession::onDecodeStopped, Qt::QueuedConnection);
    
    // 连接播放器信号 - 播放器信号也在std::thread中emit
    connect(m_player, &AudioPlayer::positionChanged, this, &AudioSession::onPositionChanged, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackError, this, &AudioSession::onPlaybackError, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::bufferStatusChanged, this, &AudioSession::onBufferStatusChanged, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::bufferUnderrun, this, &AudioSession::onBufferUnderrun, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackStarted, this, &AudioSession::onPlaybackStarted, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackPaused, this, &AudioSession::onPlaybackPaused, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackResumed, this, &AudioSession::onPlaybackResumed, Qt::QueuedConnection);
    connect(m_player, &AudioPlayer::playbackStopped, this, &AudioSession::onPlaybackStopped, Qt::QueuedConnection);
}

void AudioSession::cleanup()
{
    stop();
}
