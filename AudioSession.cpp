#include "AudioSession.h"
#include <chrono>
#include <QFileInfo>

AudioSession::AudioSession(const QString& sessionId, QObject* parent)
    : QObject(parent),
      m_sessionId(sessionId),
      m_active(false),
      m_decoder(new AudioDecoder(this)),
      m_player(&AudioPlayer::instance()),  // 共享播放器实例（跨会话复用）
      m_duration(0),
      m_isBuffering(false),
      m_bufferingPercent(0),
      m_decoderPausedByFlowControl(false),
      m_hasStartedPlayback(false),
      m_manualPaused(false),
      m_lastDecodedTimestampMs(0),
      m_lastPausedPlaybackMs(0),
      m_decoderTimestampBaseMs(0),
      m_decoderTimestampOffsetMs(0),
      m_decoderTimestampNeedsProbe(false),
      m_decoderTimestampHasOffset(false),
      m_pendingResumeSignal(false),
      m_seekGracePeriod(false),
      m_seekGraceTimer(new QTimer(this)),
      m_isSeeking(false)
{
    m_seekGraceTimer->setSingleShot(true);
    connect(m_seekGraceTimer, &QTimer::timeout, this, [this]() {
        m_seekGracePeriod = false;
        m_isSeeking = false;  // seek 宽限结束，恢复正常状态判断
        
        // 清理流控暂停标记，避免后续误判
        if (m_decoderPausedByFlowControl) {
            m_decoderPausedByFlowControl = false;
        }
        
        // seek 后按缓冲水位兜底检查解码器是否在运行
        int fillLevel = m_player->bufferFillLevel();
        if (fillLevel < 85 && m_decoder->isDecoding()) {
            // 触发解码，尽快回填缓冲
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

    // 统一成本地路径/URL 字符串，便于从路径提取标题
    QString filePath;
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    } else {
        filePath = url.toString();
    }

    // 默认先根据文件名生成标题，后续可被文件元数据覆盖
    QFileInfo fileInfo(filePath);
    m_title = fileInfo.completeBaseName();  // 优先去掉扩展名后的基础名
    if (m_title.isEmpty()) {
        m_title = fileInfo.fileName();  // 兜底使用完整文件名
    }

    // 艺术家先置空，等待解码器标签回填
    m_artist = "";

    qDebug() << "AudioSession: Extracted title from URL:" << m_title;

    bool result = false;
    if (url.isLocalFile()) {
        // 本地文件直接按本地路径初始化解码器
        result = m_decoder->initDecoder(url.toLocalFile());
    } else if (url.scheme().startsWith("http")) {
        // 网络音频按 URL 初始化解码器
        qDebug() << "AudioSession: Loading network URL:" << url.toString();
        result = m_decoder->initDecoder(url.toString());
    } else {
        qDebug() << "AudioSession: Unsupported URL scheme:" << url.scheme();
    }

    // 解码器初始化成功后，用提取到的音频标签覆盖默认标题/艺术家
    // 这样本地与网络来源都能显示更准确的曲目信息
    if (result) {
        QString extractedTitle = m_decoder->extractedTitle();
        QString extractedArtist = m_decoder->extractedArtist();

        // 标题以音频标签为准
        if (!extractedTitle.isEmpty()) {
            m_title = extractedTitle;
            qDebug() << "AudioSession: Using title from file metadata:" << m_title;
        }

        // 艺术家以音频标签为准
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
        m_hasStartedPlayback = false;
        m_lastDecodedTimestampMs = 0;
        m_lastPausedPlaybackMs = 0;
        m_decoderTimestampBaseMs = 0;
        m_decoderTimestampOffsetMs = 0;
        m_decoderTimestampNeedsProbe = false;
        m_decoderTimestampHasOffset = false;
        m_pendingResumeSignal = false;
        emit sessionStarted();
    }
    m_manualPaused = false;

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = !currentOwner.isEmpty() && currentOwner != m_sessionId;
    if (takingOverFromOtherOwner && m_player->isPlaying() && !m_player->isPaused()) {
        m_player->pause();
    }

    m_player->setWriteOwner(m_sessionId);
    if (takingOverFromOtherOwner) {
        // Reset shared output state on owner handoff to prevent cross-session audio bleed.
        m_player->stop();
        m_player->resetBuffer();
        m_player->setCurrentTimestamp(0);
    }
    m_decoder->startDecode();

    auto t1 = std::chrono::high_resolution_clock::now();
    qDebug() << "[TIMING] AudioSession::play - startDecode call:"
             << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";

    m_isBuffering = true;
}

void AudioSession::pause()
{
    if (!m_active) {
        return;
    }

    if (isWriteOwnerActive()) {
        const qint64 pausedPos = m_player->getPlaybackPosition();
        if (pausedPos >= 0) {
            m_lastPausedPlaybackMs = pausedPos;
            m_lastDecodedTimestampMs = pausedPos;
        }
    }

    m_manualPaused = true;
    m_isBuffering = false;
    m_decoder->pauseDecode();
    if (isWriteOwnerActive()) {
        m_player->pause();
    }

    // Emit pause immediately from session layer. AudioPlayer's queued pause signal
    // can arrive after ownership has switched to video and would be filtered out.
    emit sessionPaused();
}

void AudioSession::resume()
{
    m_pendingResumeSignal = true;
    m_manualPaused = false;

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = !currentOwner.isEmpty() && currentOwner != m_sessionId;
    if (takingOverFromOtherOwner && m_player->isPlaying() && !m_player->isPaused()) {
        m_player->pause();
    }

    m_player->setWriteOwner(m_sessionId);

    if (takingOverFromOtherOwner) {
        // Let decoder refill from this session's timeline after takeover.
        qint64 resumeTimestamp = m_lastPausedPlaybackMs > 0
                ? m_lastPausedPlaybackMs
                : m_lastDecodedTimestampMs;
        if (resumeTimestamp < 0) {
            resumeTimestamp = 0;
        }

        m_lastDecodedTimestampMs = resumeTimestamp;
        m_decoderTimestampBaseMs = resumeTimestamp;
        m_decoderTimestampOffsetMs = 0;
        m_decoderTimestampNeedsProbe = true;
        m_decoderTimestampHasOffset = false;
        m_player->stop();
        m_player->resetBuffer();
        m_player->setCurrentTimestamp(resumeTimestamp);
        m_decoder->seekTo(resumeTimestamp);
        m_isBuffering = true;
    }

    m_decoder->startDecode();

    if (takingOverFromOtherOwner) {
        return;
    }

    if (!m_player->isPlaying()) {
        m_player->start();
    } else if (m_player->isPaused()) {
        m_player->resume();
    }
}

void AudioSession::stop()
{
    m_manualPaused = false;
    m_isBuffering = false;
    m_hasStartedPlayback = false;
    m_lastDecodedTimestampMs = 0;
    m_lastPausedPlaybackMs = 0;
    m_decoderTimestampBaseMs = 0;
    m_decoderTimestampOffsetMs = 0;
    m_decoderTimestampNeedsProbe = false;
    m_decoderTimestampHasOffset = false;
    m_pendingResumeSignal = false;
    m_decoder->stopDecode();
    if (isWriteOwnerActive()) {
        m_player->stop();
    }
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
    m_lastDecodedTimestampMs = positionMs;
    m_lastPausedPlaybackMs = positionMs;
    m_decoderTimestampBaseMs = positionMs;
    m_decoderTimestampOffsetMs = 0;
    m_decoderTimestampNeedsProbe = true;
    m_decoderTimestampHasOffset = false;
    
    // 进入 seek 状态，抑制部分瞬时回调
    m_isSeeking = true;
    
    // 记录 seek 前是否在播放，seek 完成后按原状态恢复
    bool wasPlaying = m_player->isPlaying() && !m_player->isPaused();
    
    // 播放中 seek：先暂停解码与播放，避免旧时间轴数据继续输出
    if (wasPlaying && m_decoder) {
        m_decoder->pauseDecode();
        m_player->pause();
    }
    
    // 清空共享缓冲，防止 seek 前旧帧混入
    AudioBuffer* buffer = m_player->getBuffer();
    if (buffer) {
        buffer->clear();  // 立即丢弃旧 PCM 数据
    }
    
    // 先对齐播放器时钟到目标位置
    // 后续写入的解码数据将基于该时间轴继续
    m_player->setCurrentTimestamp(positionMs);
    
    // seek 期间由宽限逻辑管理缓冲状态，先清除当前 buffering 标志
    m_isBuffering = false;
    
    // 启动 seek 宽限窗口，过滤 seek 后短暂抖动
    m_seekGracePeriod = true;
    if (m_seekGraceTimer) {
        m_seekGraceTimer->start(800);
    }
    
    // 通知解码器跳转到目标时间
    if (m_decoder) {
        m_decoder->seekTo(positionMs);
    }
    
    // seek 后立即恢复解码线程
    if (m_decoder) {
        m_decoder->startDecode();
    }
    
    // 若 seek 前处于播放态，则恢复输出
    if (wasPlaying && m_player) {
        m_player->resume();
    }
    
    // m_isSeeking 将在 seekGraceTimer 超时后复位
    
    qDebug() << "AudioSession: Seek completed";
}

void AudioSession::setVolume(int volume)
{
    m_player->setVolume(volume);
}

bool AudioSession::isPlaying() const
{
    // Session-level playing must be ownership-aware.
    return m_active
            && isWriteOwnerActive()
            && m_player->isPlaying()
            && !m_player->isPaused();
}

bool AudioSession::isPaused() const
{
    if (!m_active) {
        return false;
    }

    // Explicit pause from service/UI takes highest priority.
    if (m_manualPaused) {
        return true;
    }

    // Active but not current write owner means this session is backgrounded.
    if (!isWriteOwnerActive()) {
        return true;
    }

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
    if (!m_active || m_manualPaused || !isWriteOwnerActive()) {
        return;
    }

    qint64 normalizedTimestampMs = timestampMs;
    if (m_decoderTimestampNeedsProbe) {
        // After seek/owner handoff, decoder timestamp can be offset.
        // Probe once and apply a stable offset if the delta is significant.
        const qint64 delta = m_decoderTimestampBaseMs - timestampMs;
        if (qAbs(delta) > 5000) {
            m_decoderTimestampHasOffset = true;
            m_decoderTimestampOffsetMs = delta;
            qDebug() << "AudioSession: Decoder timestamp offset applied. expected:"
                     << m_decoderTimestampBaseMs << "raw:" << timestampMs
                     << "offset:" << m_decoderTimestampOffsetMs;
        } else {
            m_decoderTimestampHasOffset = false;
            m_decoderTimestampOffsetMs = 0;
        }
        m_decoderTimestampNeedsProbe = false;
    }

    if (m_decoderTimestampHasOffset) {
        normalizedTimestampMs = timestampMs + m_decoderTimestampOffsetMs;
    }

    m_lastDecodedTimestampMs = normalizedTimestampMs;

    // 缓冲过高时暂停解码，避免无意义堆积
    int fillLevel = m_player->bufferFillLevel();
    if (fillLevel > 85 && !m_decoderPausedByFlowControl && m_decoder->isDecoding()) {
        m_decoder->pauseDecode();
        m_decoderPausedByFlowControl = true;
        qDebug() << "AudioSession: Buffer full (" << fillLevel << "%), pausing decoder";
    }
    
    // 写入解码数据，携带会话 ID 以保证写入归属
    m_player->writeAudioData(data, normalizedTimestampMs, m_sessionId);
    
    // 初次起播门槛可低一些，减少启动等待
    // 正常播放时门槛更高，提升稳定性
    int requiredLevel = (!m_player->isPlaying() && m_active) ? 40 : 85;
    
    if (m_isBuffering && fillLevel >= requiredLevel) {
        m_isBuffering = false;
        
        // 缓冲达到阈值后启动或恢复播放器
        if (!m_player->isPlaying() && m_active) {
            // Do not anchor to latest decoded timestamp here, otherwise
            // pre-buffered data can push UI progress forward abruptly.
            qint64 startTimestamp = m_decoderTimestampBaseMs;
            if (startTimestamp < 0) {
                startTimestamp = 0;
            }
            m_player->setCurrentTimestamp(startTimestamp);
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
    // 仅在标签非空时覆盖默认信息
    // 避免空标签把已有标题/艺术家清空
    if (!title.isEmpty()) {
        m_title = title;
        qDebug() << "AudioSession: Using title from file metadata:" << m_title;
    }
    if (!artist.isEmpty()) {
        m_artist = artist;
        qDebug() << "AudioSession: Using artist from file metadata:" << m_artist;
    }

    // 标签更新后向外同步最新元数据
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
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    emit positionChanged(positionMs);
    
    // 接近总时长末尾时，主动触发完成逻辑
    if (m_duration > 0 && positionMs >= m_duration - 100) {
        onPlaybackFinished();
    }
}

void AudioSession::onBufferStatusChanged(int fillLevel)
{
    if (!m_active || m_manualPaused || !isWriteOwnerActive()) {
        return;
    }

    m_bufferingPercent = fillLevel;
    
    // seek 宽限期内忽略水位波动，避免误触发 buffering 切换
    if (m_seekGracePeriod) {
        return;
    }
    
    // 水位回落后恢复此前因流控暂停的解码线程
    if (fillLevel < 60 && m_decoderPausedByFlowControl && m_decoder->isDecoding()) {
        m_decoder->startDecode();
        m_decoderPausedByFlowControl = false;
        qDebug() << "AudioSession: Buffer low (" << fillLevel << "%), resuming decoder";
    }
    
    // 低水位进入 buffering，并暂停输出等待回填
    if (fillLevel < 10 && m_player->isPlaying() && !m_isBuffering) {
        m_isBuffering = true;
        m_player->pause();
        emit bufferingStarted();
        qDebug() << "AudioSession: Buffering started, fill level:" << fillLevel << "%";
    }
    // 水位恢复后退出 buffering
    else if (fillLevel >= 25 && m_isBuffering) {
        m_isBuffering = false;
        
        // 输出端不在播放时执行一次显式启动
        if (!m_player->isPlaying() && m_active) {
            qint64 startTimestamp = m_decoderTimestampBaseMs;
            if (startTimestamp < 0) {
                startTimestamp = 0;
            }
            m_player->setCurrentTimestamp(startTimestamp);
            m_player->start();
            qDebug() << "AudioSession: Initial playback started, fill level:" << fillLevel << "%";
        } else {
            m_player->resume();
        }
        
        // 退出 buffering 后确保解码继续推进
        m_decoder->startDecode();
        emit bufferingFinished();
        qDebug() << "AudioSession: Buffering finished, fill level:" << fillLevel << "%";
    }
}

void AudioSession::onBufferUnderrun()
{
    if (!m_active || m_manualPaused || !isWriteOwnerActive()) {
        return;
    }

    qDebug() << "AudioSession: Buffer underrun detected";
    
    // 仅在未处于 buffering 状态时进入，避免重复触发
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
    
    // 解码完成仅表示数据生产结束
    // 会话结束仍以实际播放完成事件为准
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
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    qDebug() << "AudioSession: Playback started";

    if (!m_hasStartedPlayback) {
        m_hasStartedPlayback = true;
        if (m_pendingResumeSignal) {
            m_pendingResumeSignal = false;
            emit sessionResumed();
            return;
        }
        return;
    }

    if (m_pendingResumeSignal) {
        m_pendingResumeSignal = false;
        emit sessionResumed();
        return;
    }

    // Owner handoff resume path calls AudioPlayer::start(), which emits
    // playbackStarted (not playbackResumed). Propagate a resumed event so
    // ViewModel/UI can switch back to "playing" and pause video focus.
    if (!m_isSeeking && !m_isBuffering && !m_manualPaused) {
        emit sessionResumed();
    }
}

void AudioSession::onPlaybackPaused()
{
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    qDebug() << "AudioSession: Playback paused";
    // seek/buffering/manual pause are internal control paths; avoid duplicate UI pause emissions.
    if (!m_isSeeking && !m_isBuffering && !m_manualPaused) {
        emit sessionPaused();
    }
}

void AudioSession::onPlaybackResumed()
{
    if (!m_active || m_manualPaused || !isWriteOwnerActive()) {
        return;
    }

    qDebug() << "AudioSession: Playback resumed";
    if (m_pendingResumeSignal) {
        m_pendingResumeSignal = false;
        emit sessionResumed();
        return;
    }

    // seek/buffering 期间不向上游抛 resumed，避免 UI 状态误切换
    if (!m_isSeeking && !m_isBuffering) {
        emit sessionResumed();
    }
}

void AudioSession::onPlaybackStopped()
{
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    qDebug() << "AudioSession: Playback stopped";
}

void AudioSession::onPlaybackFinished()
{
    qDebug() << "AudioSession: Playback finished";
    
    // 完成时先停止解码与输出，确保状态收敛
    m_decoder->stopDecode();
    m_player->stop();
    
    m_active = false;
    m_hasStartedPlayback = false;
    emit sessionFinished();
}

bool AudioSession::isWriteOwnerActive() const
{
    return m_player && m_player->writeOwner() == m_sessionId;
}

void AudioSession::connectSignals()
{
    // 解码器信号来自工作线程，使用队列连接切回会话线程处理
    connect(m_decoder, &AudioDecoder::decodedData, this, &AudioSession::onDecodedData, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::metadataReady, this, &AudioSession::onMetadataReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::audioTagsReady, this, &AudioSession::onAudioTagsReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::albumArtReady, this, &AudioSession::onAlbumArtReady, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeError, this, &AudioSession::onDecodeError, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeCompleted, this, &AudioSession::onDecodeCompleted, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeStarted, this, &AudioSession::onDecodeStarted, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodePaused, this, &AudioSession::onDecodePaused, Qt::QueuedConnection);
    connect(m_decoder, &AudioDecoder::decodeStopped, this, &AudioSession::onDecodeStopped, Qt::QueuedConnection);
    
    // 播放器信号同样使用队列连接，避免跨线程直接回调
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

