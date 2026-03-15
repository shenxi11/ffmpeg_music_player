#include "AudioSession.h"
#include <chrono>
#include <QFileInfo>
#include <QCoreApplication>
#include <QEventLoop>
#include <QUrlQuery>
#include <QDateTime>
#include <QDebug>
#include <future>
#include <thread>

namespace {

bool initDecoderWithEventPumping(AudioDecoder* decoder, const QString& source)
{
    if (!decoder) {
        return false;
    }

    auto future = std::async(std::launch::async, [decoder, source]() {
        return decoder->initDecoder(source);
    });

    using namespace std::chrono_literals;
    while (future.wait_for(0ms) != std::future_status::ready) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 8);
        std::this_thread::sleep_for(1ms);
    }

    return future.get();
}

bool isLikelyFlacSource(const QUrl& sourceUrl)
{
    if (sourceUrl.path().toLower().endsWith(".flac")) {
        return true;
    }

    // 本地代理模式：真实源地址放在 src 查询参数中。
    const QUrlQuery query(sourceUrl);
    const QString src = query.queryItemValue(QStringLiteral("src")).toLower();
    return src.contains(QStringLiteral(".flac"));
}

int seekGraceMsForSource(const QUrl& sourceUrl)
{
    const QString host = sourceUrl.host().toLower();
    const bool localProxy = sourceUrl.scheme().startsWith("http")
            && (host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost"));
    const bool remoteHttp = sourceUrl.scheme().startsWith("http");
    const bool flacLike = isLikelyFlacSource(sourceUrl);

    if (localProxy) {
        return flacLike ? 180 : 140;
    }
    if (remoteHttp) {
        return flacLike ? 320 : 220;
    }
    return 120;
}

int seekRetryInitialMsForSource(const QUrl& sourceUrl)
{
    const QString host = sourceUrl.host().toLower();
    const bool localProxy = sourceUrl.scheme().startsWith("http")
            && (host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost"));
    const bool remoteHttp = sourceUrl.scheme().startsWith("http");
    const bool flacLike = isLikelyFlacSource(sourceUrl);

    if (localProxy) {
        return flacLike ? 650 : 500;
    }
    if (remoteHttp) {
        return flacLike ? 900 : 700;
    }
    return 500;
}

} // namespace

AudioSession::AudioSession(const QString& sessionId, QObject* parent)
    : QObject(parent),
      m_sessionId(sessionId),
      m_active(false),
      m_decoder(new AudioDecoder(this)),
      m_player(&AudioPlayer::instance()),
      m_duration(0),
    m_seekGraceTimer(new QTimer(this)),
    m_seekRetryTimer(new QTimer(this)),
    m_seekLatencyTimer()
{
    m_seekGraceTimer->setSingleShot(true);
    connect(m_seekGraceTimer, &QTimer::timeout, this, &AudioSession::onSeekGraceTimeout);

    m_seekRetryTimer->setSingleShot(true);
    connect(m_seekRetryTimer, &QTimer::timeout, this, &AudioSession::onSeekRetryTimeout);

    connectSignals();
}

AudioSession::~AudioSession()
{
    cleanup();
}

void AudioSession::onSeekGraceTimeout()
{
    setSeekPhase(SeekPhase::Idle, "seek_grace_timeout");
    if (m_buffering.decoderPausedByFlowControl) {
        m_buffering.decoderPausedByFlowControl = false;
    }

    const int fillLevel = m_player->bufferFillLevel();
    if (fillLevel < 85 && m_decoder->isDecoding()) {
        m_decoder->startDecode();
        infoLog("SEEK_EVT") << "Ensure decoder running after seek, buffer=" << fillLevel << "%";
    }

    infoLog("SEEK_EVT") << "Seek grace period ended";
    validateState("seek_grace_timeout");
}

bool AudioSession::loadSource(const QUrl& url)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    m_sourceUrl = url;

    // 统一为路径或 URL 字符串，便于从路径提取默认标题。
    QString filePath;
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    } else {
        filePath = url.toString();
    }

    // 默认先根据文件名生成标题，后续可被音频元数据覆盖。
    QFileInfo fileInfo(filePath);
    m_title = fileInfo.completeBaseName();  // 优先使用去掉扩展名后的基础文件名
    if (m_title.isEmpty()) {
        m_title = fileInfo.fileName();  // 兜底使用完整文件名
    }

    // 艺术家先置空，等待解码器标签回填。
    m_artist = "";

    infoLog("SESSION_EVT") << "Extracted title from URL:" << m_title;

    bool result = false;
    if (url.isLocalFile()) {
        // 本地文件直接按本地路径初始化解码器。
        result = m_decoder->initDecoder(url.toLocalFile());
    } else if (url.scheme().startsWith("http")) {
        // 网络音频按 URL 初始化解码器。
        infoLog("SESSION_EVT") << "Loading network URL:" << url.toString();
        // 网络初始化期间持续处理事件，避免主线程阻塞导致请求超时。
        result = initDecoderWithEventPumping(m_decoder, url.toString());
    } else {
        warnLog("SESSION_EVT") << "Unsupported URL scheme:" << url.scheme();
    }

    // 解码器初始化成功后，用音频标签覆盖默认标题与艺术家。
    if (result) {
        QString extractedTitle = m_decoder->extractedTitle();
        QString extractedArtist = m_decoder->extractedArtist();

        // 标题以音频标签为准。
        if (!extractedTitle.isEmpty()) {
            m_title = extractedTitle;
            infoLog("SESSION_EVT") << "Using title from metadata:" << m_title;
        }

        // 艺术家以音频标签为准。
        if (!extractedArtist.isEmpty()) {
            m_artist = extractedArtist;
            infoLog("SESSION_EVT") << "Using artist from metadata:" << m_artist;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    infoLog("SESSION_EVT") << "loadSource initDecoder timing="
                           << std::chrono::duration<double, std::milli>(t1 - t0).count()
                           << "ms result=" << result;

    return result;
}

void AudioSession::play()
{
    auto t0 = std::chrono::high_resolution_clock::now();

    if (!m_active) {
        m_active = true;
        resetRuntimeState();
        emit sessionStarted();
    }
    m_playback.manualPaused = false;

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = shouldTakeOverFromOwner(currentOwner, false);
    pauseSharedPlayerForTakeover(takingOverFromOtherOwner);
    assignSessionAsWriteOwner();
    if (takingOverFromOtherOwner) {
        // Reset shared output state on owner handoff to prevent cross-session audio bleed.
        resetSharedOutputAfterTakeover(0);
    }
    requestDecoderStart();

    auto t1 = std::chrono::high_resolution_clock::now();
    infoLog("SESSION_EVT") << "play startDecode timing="
                           << std::chrono::duration<double, std::milli>(t1 - t0).count()
                           << "ms";

    m_buffering.active = true;
    finalizeControlPath("play");
}

void AudioSession::pause()
{
    if (!m_active) {
        return;
    }

    if (isWriteOwnerActive()) {
        const qint64 pausedPos = m_player->getPlaybackPosition();
        if (pausedPos >= 0) {
            m_clock.lastPausedPlaybackMs = pausedPos;
            m_clock.lastDecodedMs = pausedPos;
        }
    }

    m_playback.manualPaused = true;
    m_buffering.active = false;
    m_decoder->pauseDecode();
    if (isWriteOwnerActive()) {
        m_player->pause();
    }

    // Emit pause immediately from session layer. AudioPlayer's queued pause signal
    // can arrive after ownership has switched to video and would be filtered out.
    emit sessionPaused();
    finalizeControlPath("pause");
}

void AudioSession::resume()
{
    m_playback.resumeSignalPending = true;
    m_playback.manualPaused = false;
    m_playback.internalPausePending = false;
    setDecodePhase(DecodePhase::Running, "resume");

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = handleResumeOwnerTakeover(currentOwner);

    requestDecoderStart();

    if (takingOverFromOtherOwner) {
        return;
    }

    startOrResumePlayerOutput();
    finalizeControlPath("resume");
}

void AudioSession::stop()
{
    resetRuntimeState();
    m_decoder->stopDecode();
    if (isWriteOwnerActive()) {
        m_player->stop();
    }
    m_player->clearWriteOwner(m_sessionId);
    m_active = false;
    emit sessionStopped();
    finalizeControlPath("stop");
}

void AudioSession::seekTo(qint64 positionMs)
{
    if (!m_decoder || !m_player) {
        warnLog("SEEK_EVT") << "seekTo ignored while session destroying";
        return;
    }
    
    infoLog("SEEK_EVT") << "seek start target=" << positionMs << "ms source=" << m_sourceUrl.toString();
    beginSeekLatencyTracking(positionMs);
    applySeekClockBase(positionMs);

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = shouldTakeOverFromOwner(currentOwner, false);
    if (takingOverFromOtherOwner) {
        // owner handoff 场景将 seek 延迟到 resume 阶段执行，避免重复 seek。
        deferSeekUntilOwnerTakeover(positionMs);
        return;
    }
    
    // 杩涘叆 seek 鐘舵€侊紝鎶戝埗閮ㄥ垎鐬椂鍥炶皟
    setSeekPhase(SeekPhase::Seeking, "seekTo");
    
    // 记录 seek 前是否在播放，seek 完成后按原状态恢复。
    const bool wasPlaying = isPlayerActivelyPlaying();
    pausePipelineForLocalSeek(wasPlaying);
    prepareOutputForLocalSeek(positionMs, wasPlaying);
    
    // 启动 seek 宽限窗口，过滤 seek 后短暂抖动。
    enterSeekGraceWindow();
    
    dispatchDecoderSeekForLocalSeek(positionMs);
    
    // m_isSeeking 会在 seekGraceTimer 超时后复位。
    infoLog("SEEK_EVT") << "seek pipeline dispatched";
    finalizeControlPath("seekTo");
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
    if (m_playback.manualPaused) {
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

void AudioSession::onDecodedData(const QByteArray& data, qint64 timestampMs, int seekGeneration)
{
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        return;
    }

    if (seekGeneration < m_clock.minAcceptedDecodeGeneration) {
        static int staleDropCount = 0;
        if (++staleDropCount % 50 == 1) {
            infoLog("STATE_EVT") << "Drop stale decoded frame generation="
                                 << seekGeneration
                                 << " expected>=" << m_clock.minAcceptedDecodeGeneration;
        }
        return;
    }

    qint64 normalizedTimestampMs = timestampMs;
    normalizeDecodedTimestamp(timestampMs, normalizedTimestampMs);
    m_clock.lastDecodedMs = normalizedTimestampMs;
    extendDurationFromDecodedTimestamp(normalizedTimestampMs);

    if (handleSeekLatencyOnDecodedTimestamp(normalizedTimestampMs)) {
        return;
    }

    const bool isRemoteFlac = isRemoteFlacSource();
    const int fillLevelBeforeWrite = m_player->bufferFillLevel();
    applyDecoderFlowControlBeforeWrite(isRemoteFlac, fillLevelBeforeWrite);
    
    // 写入解码数据，携带会话ID保证写入归属。
    m_player->writeAudioData(data, normalizedTimestampMs, m_sessionId);
    
    // 基于写入后的最新水位与字节量判断是否可恢复播放。
    const int fillLevel = m_player->bufferFillLevel();
    const AudioBuffer* buffer = m_player->getBuffer();
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    int requiredLevel = 0;
    int requiredBytes = 0;
    const bool readyToResume = shouldResumeFromDecodedWrite(
            fillLevel, bufferedBytes, isRemoteFlac, requiredLevel, requiredBytes);

    if (m_buffering.active && readyToResume) {
        m_buffering.active = false;

        if (!m_player->isPlaying() && m_active) {
            startPlayerFromDecodedReady(fillLevel);
        } else {
            resumePlayerFromDecodedReady(fillLevel, bufferedBytes, requiredLevel, requiredBytes);
        }
        completeDecodedReadyResume();
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
    // 仅在标签非空时覆盖默认信息。
    if (!title.isEmpty()) {
        m_title = title;
        infoLog("SESSION_EVT") << "Using title from metadata:" << m_title;
    }
    if (!artist.isEmpty()) {
        m_artist = artist;
        infoLog("SESSION_EVT") << "Using artist from metadata:" << m_artist;
    }

    // 标签更新后向外同步最新元数据。
    emit metadataReady(m_title, m_artist, m_duration);
}

void AudioSession::onAlbumArtReady(const QString& imagePath)
{
    m_albumArt = imagePath;
    emit albumArtReady(imagePath);
}

void AudioSession::onDecodeError(const QString& error)
{
    if (isSeekLatencyActiveNow()) {
        warnLog("SEEK_EVT") << "seek failed on decode error desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs
                            << " error=" << error
                            << " elapsed=" << (m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1) << "ms";
        setSeekLatencyPhase(SeekLatencyPhase::Inactive, "decode_error");
    }
    m_seek.fastStartMode = false;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
    validateState("onDecodeError");
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
    
    // 仅在解码完成后，才允许按时长阈值收敛到 finished。
    if (m_duration > 0 && positionMs >= m_duration - 100) {
        if (!isDecodeCompletedNow() && positionMs > m_duration + 500) {
            constexpr qint64 kDurationHeadroomMs = 2500;
            const qint64 desiredDuration = positionMs + kDurationHeadroomMs;
            if (desiredDuration > m_duration) {
                m_duration = desiredDuration;
                emit durationChanged(m_duration);
                infoLog("SESSION_EVT") << "Extend duration by playback position to " << m_duration
                                       << "ms (position=" << positionMs << ")";
            }
        }

        if (isDecodeCompletedNow()) {
            onPlaybackFinished();
        } else {
            // 尾段等待 EOS 时做日志节流，避免刷屏影响问题定位。
            if (!m_buffering.tailWaitLogActive) {
                m_tailWaitLogTimer.restart();
                m_buffering.tailWaitLogActive = true;
                infoLog("SESSION_EVT") << "Position reached metadata duration but decode not completed, waiting for EOS";
            } else if (m_tailWaitLogTimer.elapsed() >= 1000) {
                m_tailWaitLogTimer.restart();
                infoLog("SESSION_EVT") << "Still waiting EOS at tail position=" << positionMs
                                       << " duration=" << m_duration;
            }

            // 安全兜底：若超出元数据时长较多且缓冲极低，按尾段自然结束收敛。
            const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
            const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
            if (positionMs >= m_duration + 2500 && bufferedBytes < 32 * 1024) {
                warnLog("BUFFER_EVT") << "Tail force-finish fallback."
                                      << " position=" << positionMs
                                      << " duration=" << m_duration
                                      << " bufferedBytes=" << bufferedBytes;
                onPlaybackFinished();
            }
        }
    }
}

void AudioSession::onBufferStatusChanged(int fillLevel)
{
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        return;
    }

    m_buffering.percent = fillLevel;
    const bool withinSeekGrace = isSeekGraceActive();
    
    const AudioBuffer* buffer = m_player->getBuffer();
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    const bool isRemoteFlac = isRemoteFlacSource();
    int kBufferingEnterMinBytes = m_seek.fastStartMode ? (24 * 1024) : (96 * 1024);
    int kBufferingResumeMinBytes = m_seek.fastStartMode ? (48 * 1024) : (192 * 1024);
    if (isRemoteFlac) {
        // 远端 FLAC 仍保留缓冲保护，但降低门槛以减少 seek 后等待。
        kBufferingEnterMinBytes = 128 * 1024;
        kBufferingResumeMinBytes = 384 * 1024;
    }

    // 水位回落后恢复此前因流控暂停的解码线程。
    if (fillLevel < 60 && m_buffering.decoderPausedByFlowControl && m_decoder->isDecoding()) {
        requestDecoderStart();
        m_buffering.decoderPausedByFlowControl = false;
        infoLog("BUFFER_EVT") << "Buffer low, resume decoder fill=" << fillLevel << "%";
    }
    
    // 低水位进入 buffering，并暂停输出等待回填。
    // 仅当“百分比低 + 绝对字节也低”时才进入，避免大缓冲容量场景反复抖动。
    if (shouldDebounceBufferingEnter() && !m_buffering.active) {
        // 刚恢复播放后的短窗口内忽略一次低水位回落，避免状态闪烁。
        return;
    }

    if (fillLevel < 10
            && bufferedBytes < kBufferingEnterMinBytes
            && m_player->isPlaying()
            && !withinSeekGrace
            && !m_buffering.active) {
        const qint64 currentPos = m_player->getPlaybackPosition();
        if (handleTailLowBufferFallback(currentPos, fillLevel, bufferedBytes)) {
            return;
        }

        m_buffering.active = true;
        m_playback.internalPausePending = true;
        m_player->pause();
        emit bufferingStarted();
        infoLog("BUFFER_EVT") << "Buffering started fill=" << fillLevel << "%";
    }
    // 水位/字节恢复后退出 buffering（字节门槛优先，百分比兜底）。
    else if (m_buffering.active) {
        if (!canExitBufferingByThreshold(fillLevel, bufferedBytes, isRemoteFlac, kBufferingResumeMinBytes)) {
            return;
        }

        exitBufferingAndResume(fillLevel, bufferedBytes);
    }
}

void AudioSession::onBufferUnderrun()
{
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        return;
    }

    warnLog("BUFFER_EVT") << "Buffer underrun detected";
    
    // 仅在未处于 buffering 状态时进入，避免重复触发。
    if (!m_buffering.active) {
        m_buffering.active = true;
        m_playback.internalPausePending = true;
        m_player->pause();
        emit bufferingStarted();
    }
}

void AudioSession::onDecodeCompleted()
{
    setDecodePhase(DecodePhase::Completed, "decode_completed");
    m_buffering.tailWaitLogActive = false;
    if (m_clock.lastDecodedMs > m_duration) {
        m_duration = m_clock.lastDecodedMs;
        emit durationChanged(m_duration);
        infoLog("SESSION_EVT") << "Final duration corrected to decoded tail=" << m_duration << "ms";
    }
    infoLog("SESSION_EVT") << "Decode completed";
    emit decodeFinished();
    
    // 解码完成仅表示数据生产结束；会话结束仍以实际播放完成事件为准。
}

void AudioSession::onDecodeStarted()
{
    setDecodePhase(DecodePhase::Running, "decode_started");
    infoLog("SESSION_EVT") << "Decode started";
}

void AudioSession::onDecodePaused()
{
    infoLog("SESSION_EVT") << "Decode paused";
}

void AudioSession::onDecodeStopped()
{
    infoLog("SESSION_EVT") << "Decode stopped";
}

void AudioSession::onPlaybackStarted()
{
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    infoLog("SESSION_EVT") << "Playback started";
    finalizeSeekLatency("playback_started");

    if (!m_playback.hasStarted) {
        m_playback.hasStarted = true;
        emitSessionResumedByPolicy(ResumeEmitPolicy::PendingOnly);
        validateState("onPlaybackStarted:first_start");
        return;
    }

    // Owner handoff resume path calls AudioPlayer::start(), which emits
    // playbackStarted (not playbackResumed). Propagate a resumed event so
    // ViewModel/UI can switch back to "playing" and pause video focus.
    emitSessionResumedByPolicy(ResumeEmitPolicy::PendingOrUnsuppressed);
    validateState("onPlaybackStarted");
}

void AudioSession::onPlaybackPaused()
{
    if (!m_active) {
        return;
    }

    if (clearInternalPauseIfOwnershipLost()) {
        return;
    }

    if (consumeInternalPausePending()) {
        return;
    }

    infoLog("SESSION_EVT") << "Playback paused";
    // seek/buffering/manual pause are internal control paths; avoid duplicate UI pause emissions.
    if (!shouldSuppressExternalPauseResumeSignals()) {
        emit sessionPaused();
    }
    validateState("onPlaybackPaused");
}

void AudioSession::onPlaybackResumed()
{
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        return;
    }

    infoLog("SESSION_EVT") << "Playback resumed";
    markPlaybackResumedActivity();
    finalizeSeekLatencyOnPlaybackResumed();

    // seek/buffering 期间不向上游抛 resumed，避免 UI 状态误切换。
    emitSessionResumedByPolicy(ResumeEmitPolicy::PendingOrUnsuppressed);
    validateState("onPlaybackResumed");
}

void AudioSession::onPlaybackStopped()
{
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    infoLog("SESSION_EVT") << "Playback stopped";
}

void AudioSession::onPlaybackFinished()
{
    infoLog("SESSION_EVT") << "Playback finished";
    
    // 完成时先停止解码与输出，确保状态收敛。
    m_decoder->stopDecode();
    m_player->stop();
    
    m_active = false;
    m_playback.hasStarted = false;
    m_buffering.tailWaitLogActive = false;
    validateState("onPlaybackFinished");
    emit sessionFinished();
}

void AudioSession::onSeekRetryTimeout()
{
    if (!m_active || !isWriteOwnerActive() || !isSeekLatencyActiveNow() || isSeekFirstPacketSeenNow() || m_playback.manualPaused) {
        return;
    }

    const bool isHlsSeek = isHlsSeekSource();
    const bool isRemoteFlacSeek = isRemoteFlacSource();
    const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
    const qint64 hardTimeoutMs = calculateSeekHardTimeoutMs(isHlsSeek, isRemoteFlacSeek);
    const qint64 giveUpTimeoutMs = calculateSeekGiveUpTimeoutMs(isHlsSeek, isRemoteFlacSeek, hardTimeoutMs);
    switch (decideSeekRetryAction(elapsedMs, hardTimeoutMs, giveUpTimeoutMs)) {
    case SeekRetryAction::GiveUp:
        handleSeekRetryGiveUp(elapsedMs);
        break;
    case SeekRetryAction::DegradeByHardTimeout:
        handleSeekRetryDegrade(elapsedMs, 1500, "[SEEK_DEGRADE] seek waiting exceeded hard timeout, keep buffering.");
        break;
    case SeekRetryAction::DegradeByRetryBudget:
        handleSeekRetryDegrade(elapsedMs, 1800, "[SEEK_DEGRADE] Slow network while seeking, still buffering.");
        break;
    case SeekRetryAction::RetryNow:
        handleSeekRetryAttempt(elapsedMs, isHlsSeek || isRemoteFlacSeek);
        break;
    }
    finalizeControlPath("onSeekRetryTimeout");
}

bool AudioSession::isWriteOwnerActive() const
{
    return m_player && m_player->writeOwner() == m_sessionId;
}

// ===== Seek 运行时状态与时间轴基础 =====
void AudioSession::resetRuntimeState()
{
    m_playback = PlaybackState{};
    m_buffering = BufferingState{};
    m_clock = ClockState{};
    m_seek = SeekState{};
    m_seekLatency = SeekLatencyState{};
    m_decode = DecodeState{};
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
}

void AudioSession::beginSeekLatencyTracking(qint64 targetMs)
{
    setSeekLatencyPhase(SeekLatencyPhase::WaitingFirstPacket, "beginSeekLatencyTracking");
    m_seekLatency.desiredTargetMs = targetMs;
    m_seekLatency.workingTargetMs = targetMs;
    m_seekLatency.firstPacketTimestampMs = -1;
    m_seekLatency.accuracyRetryCount = 0;
    m_seekLatencyTimer.restart();

    m_seek.fastStartMode = true;
    m_seek.retryCount = 0;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->start(seekRetryInitialMsForSource(m_sourceUrl));
    }
}

void AudioSession::applySeekClockBase(qint64 targetMs)
{
    m_clock.lastDecodedMs = targetMs;
    m_clock.lastPausedPlaybackMs = targetMs;
    m_clock.baseTimestampMs = targetMs;
    m_clock.offsetTimestampMs = 0;
    m_clock.timestampMode = DecoderTimestampMode::ProbePending;
    setDecodePhase(DecodePhase::Running, "applySeekClockBase");
}

void AudioSession::enterSeekGraceWindow()
{
    setSeekPhase(SeekPhase::Grace, "enterSeekGraceWindow");
    if (m_seekGraceTimer) {
        m_seekGraceTimer->start(seekGraceMsForSource(m_sourceUrl));
    }
}

qint64 AudioSession::resolveResumeTimestamp() const
{
    if (m_seek.pendingTakeoverSeekMs.has_value()) {
        return qMax<qint64>(0, m_seek.pendingTakeoverSeekMs.value());
    }
    if (m_clock.lastPausedPlaybackMs > 0) {
        return m_clock.lastPausedPlaybackMs;
    }
    if (m_clock.lastDecodedMs > 0) {
        return m_clock.lastDecodedMs;
    }
    return 0;
}

// ===== 播放事件分发与会话信号控制 =====
bool AudioSession::shouldSuppressExternalPauseResumeSignals() const
{
    return isSeekingNow() || m_buffering.active || m_playback.manualPaused;
}

bool AudioSession::consumeResumeSignalPendingAndEmit()
{
    if (!m_playback.resumeSignalPending) {
        return false;
    }

    m_playback.resumeSignalPending = false;
    emit sessionResumed();
    return true;
}

bool AudioSession::emitSessionResumedByPolicy(ResumeEmitPolicy policy)
{
    if (consumeResumeSignalPendingAndEmit()) {
        return true;
    }

    if (policy == ResumeEmitPolicy::PendingOrUnsuppressed
            && !shouldSuppressExternalPauseResumeSignals()) {
        emit sessionResumed();
        return true;
    }

    return false;
}

bool AudioSession::clearInternalPauseIfOwnershipLost()
{
    if (isWriteOwnerActive()) {
        return false;
    }

    // Owner handoff 后延迟到达的 paused 回调，清掉内部标记并忽略。
    m_playback.internalPausePending = false;
    return true;
}

bool AudioSession::consumeInternalPausePending()
{
    if (!m_playback.internalPausePending) {
        return false;
    }

    m_playback.internalPausePending = false;
    infoLog("STATE_EVT") << "Ignore internal playbackPaused";
    return true;
}

void AudioSession::markPlaybackResumedActivity()
{
    m_lastResumeTimer.restart();
    m_buffering.hasRecentResume = true;
}

void AudioSession::finalizeSeekLatencyOnPlaybackResumed()
{
    if (!isSeekLatencyActiveNow() || isSeekFirstPacketSeenNow()) {
        finalizeSeekLatency("playback_resumed");
        return;
    }

    infoLog("SEEK_EVT") << "skip finalize on playback_resumed before first decoded packet"
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;
}

// ===== Owner 接管与会话切换 =====
bool AudioSession::shouldTakeOverFromOwner(const QString& currentOwner, bool treatEmptyOwnerAsTakeover) const
{
    if (currentOwner == m_sessionId) {
        return false;
    }
    if (currentOwner.isEmpty()) {
        return treatEmptyOwnerAsTakeover;
    }
    return true;
}

void AudioSession::pauseSharedPlayerForTakeover(bool takingOverFromOtherOwner)
{
    if (!takingOverFromOtherOwner) {
        return;
    }
    if (!m_player->isPlaying() || m_player->isPaused()) {
        return;
    }
    m_playback.internalPausePending = true;
    m_player->pause();
}

void AudioSession::assignSessionAsWriteOwner()
{
    m_player->setWriteOwner(m_sessionId);
}

void AudioSession::resetSharedOutputAfterTakeover(qint64 timestampMs)
{
    m_player->stop();
    m_player->resetBuffer();
    m_player->setCurrentTimestamp(qMax<qint64>(0, timestampMs));
}

void AudioSession::deferSeekUntilOwnerTakeover(qint64 positionMs)
{
    m_seek.pendingTakeoverSeekMs = positionMs;
    m_clock.minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration() + 1;
    infoLog("SEEK_EVT") << "Deferring seek until owner handoff resume target=" << positionMs << "ms";
}

bool AudioSession::handleResumeOwnerTakeover(const QString& currentOwner)
{
    const bool takingOverFromOtherOwner = shouldTakeOverFromOwner(currentOwner, true);
    pauseSharedPlayerForTakeover(takingOverFromOtherOwner);
    assignSessionAsWriteOwner();
    if (!takingOverFromOtherOwner) {
        return false;
    }

    const qint64 resumeTimestamp = resolveResumeTimestamp();
    applyResumeTakeoverAtTimestamp(currentOwner, resumeTimestamp);
    return true;
}

void AudioSession::applyResumeTakeoverAtTimestamp(const QString& currentOwner, qint64 resumeTimestamp)
{
    infoLog("SESSION_EVT") << "Resume takeover from owner=" << currentOwner
                           << " resumeTimestamp=" << resumeTimestamp
                           << " lastPaused=" << m_clock.lastPausedPlaybackMs
                           << " lastDecoded=" << m_clock.lastDecodedMs;

    m_seek.pendingTakeoverSeekMs.reset();
    applySeekClockBase(resumeTimestamp);
    enterSeekGraceWindow();
    resetSharedOutputAfterTakeover(resumeTimestamp);
    m_decoder->seekTo(resumeTimestamp);
    m_clock.minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
    beginSeekLatencyTracking(resumeTimestamp);
    m_buffering.active = true;
}

void AudioSession::startOrResumePlayerOutput()
{
    if (!m_player->isPlaying()) {
        m_player->start();
    } else if (m_player->isPaused()) {
        m_player->resume();
    }
}

// ===== 本地 seek 执行链 =====
bool AudioSession::isPlayerActivelyPlaying() const
{
    return m_player->isPlaying() && !m_player->isPaused();
}

void AudioSession::pausePipelineForLocalSeek(bool wasPlaying)
{
    if (!wasPlaying) {
        return;
    }

    m_decoder->pauseDecode();
    m_playback.internalPausePending = true;
    m_player->pause();
}

void AudioSession::prepareOutputForLocalSeek(qint64 positionMs, bool wasPlaying)
{
    m_player->resetBuffer();
    m_player->setCurrentTimestamp(positionMs);

    m_buffering.active = wasPlaying;
    if (m_buffering.active) {
        emit bufferingStarted();
    }
}

void AudioSession::dispatchDecoderSeekForLocalSeek(qint64 positionMs)
{
    m_decoder->seekTo(positionMs);
    m_clock.minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
    requestDecoderStart();
}

// ===== 解码时间轴与缓冲恢复策略 =====
bool AudioSession::isRemoteFlacSource() const
{
    return m_sourceUrl.scheme().startsWith("http") && isLikelyFlacSource(m_sourceUrl);
}

void AudioSession::normalizeDecodedTimestamp(qint64 timestampMs, qint64& normalizedTimestampMs)
{
    if (m_clock.timestampMode == DecoderTimestampMode::ProbePending) {
        const qint64 delta = m_clock.baseTimestampMs - timestampMs;
        if (qAbs(delta) > 3000) {
            warnLog("SEEK_EVT") << "Seek target differs from decoded timeline, trust decoded timestamp."
                                << " expected=" << m_clock.baseTimestampMs
                                << " decoded=" << timestampMs
                                << " delta=" << delta;
            m_clock.baseTimestampMs = timestampMs;
        } else {
            m_clock.baseTimestampMs = timestampMs;
        }
        m_clock.timestampMode = DecoderTimestampMode::Passthrough;
        m_clock.offsetTimestampMs = 0;
    }

    if (m_clock.timestampMode == DecoderTimestampMode::OffsetApplied) {
        normalizedTimestampMs = timestampMs + m_clock.offsetTimestampMs;
    }
}

void AudioSession::extendDurationFromDecodedTimestamp(qint64 normalizedTimestampMs)
{
    if (m_duration > 0 && !isDecodeCompletedNow() && normalizedTimestampMs > m_duration + 500) {
        constexpr qint64 kDurationHeadroomMs = 2500;
        const qint64 desiredDuration = normalizedTimestampMs + kDurationHeadroomMs;
        if (desiredDuration > m_duration) {
            m_duration = desiredDuration;
            emit durationChanged(m_duration);
            infoLog("SESSION_EVT") << "Extend dynamic duration=" << m_duration
                                   << "ms from decoded timestamp=" << normalizedTimestampMs;
        }
    }
}

bool AudioSession::handleSeekLatencyOnDecodedTimestamp(qint64 normalizedTimestampMs)
{
    if (!isSeekLatencyActiveNow() || isSeekFirstPacketSeenNow()) {
        return false;
    }

    const qint64 driftMs = normalizedTimestampMs - m_seekLatency.desiredTargetMs;
    const qint64 absDriftMs = qAbs(driftMs);
    qint64 acceptToleranceMs = m_seekLatency.accuracyToleranceMs;
    if (m_seekLatency.accuracyRetryCount >= 2) {
        acceptToleranceMs = qMax<qint64>(acceptToleranceMs, 8000);
    }
    if (m_seekLatency.accuracyRetryCount >= 4) {
        acceptToleranceMs = qMax<qint64>(acceptToleranceMs, 15000);
    }

    if (absDriftMs > acceptToleranceMs) {
        if (m_seekLatency.accuracyRetryCount < m_seekLatency.accuracyMaxRetries) {
            ++m_seekLatency.accuracyRetryCount;
            const int retryRound = m_seekLatency.accuracyRetryCount;

            qint64 correctedTargetMs = m_seekLatency.desiredTargetMs;
            if (driftMs < 0) {
                qint64 forwardCapMs = 20000 + static_cast<qint64>(retryRound - 1) * 8000;
                forwardCapMs = qMin<qint64>(forwardCapMs, 60000);
                const qint64 forwardStepMs = qBound<qint64>(4000, ((-driftMs) * 2) / 3, forwardCapMs);
                correctedTargetMs = m_seekLatency.desiredTargetMs + forwardStepMs;
            } else {
                qint64 backwardCapMs = 12000 + static_cast<qint64>(retryRound - 1) * 4000;
                backwardCapMs = qMin<qint64>(backwardCapMs, 30000);
                const qint64 backwardStepMs = qBound<qint64>(3000, (driftMs * 2) / 3, backwardCapMs);
                correctedTargetMs = m_seekLatency.desiredTargetMs - backwardStepMs;
            }

            if (correctedTargetMs < 0) {
                correctedTargetMs = 0;
            }
            if (m_duration > 0 && correctedTargetMs > m_duration - 500) {
                correctedTargetMs = qMax<qint64>(0, m_duration - 500);
            }

            m_seekLatency.workingTargetMs = correctedTargetMs;

            warnLog("SEEK_EVT") << "[SEEK_CORRECT] retry="
                                << m_seekLatency.accuracyRetryCount << "/" << m_seekLatency.accuracyMaxRetries
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " working=" << m_seekLatency.workingTargetMs
                                << " decoded=" << normalizedTimestampMs
                                << " drift=" << driftMs
                                << " acceptTolerance=" << acceptToleranceMs;

            setSeekLatencyPhase(SeekLatencyPhase::WaitingFirstPacket, "seek_corrective_reseek");
            m_seekLatency.firstPacketTimestampMs = -1;
            m_decoder->seekTo(m_seekLatency.workingTargetMs);
            m_clock.minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
            m_decoder->startDecode();
            if (m_seekRetryTimer) {
                m_seekRetryTimer->start(900);
            }
            return true;
        }

        warnLog("SEEK_EVT") << "[SEEK_ACCEPT] drift remains after correction retries."
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs
                            << " decoded=" << normalizedTimestampMs
                            << " drift=" << driftMs
                            << " acceptTolerance=" << acceptToleranceMs;
        setSeekLatencyPhase(SeekLatencyPhase::FirstPacketSeen, "seek_accept_nearest_keyframe");
        m_seekLatency.firstPacketTimestampMs = normalizedTimestampMs;
        m_seekLatency.workingTargetMs = normalizedTimestampMs;
        if (m_seekRetryTimer) {
            m_seekRetryTimer->stop();
        }
    }

    if (!isSeekFirstPacketSeenNow()) {
        setSeekLatencyPhase(SeekLatencyPhase::FirstPacketSeen, "seek_first_packet_seen");
        m_seekLatency.firstPacketTimestampMs = normalizedTimestampMs;
        const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
        infoLog("SEEK_EVT") << "seek_to_first_decoded_ms=" << elapsedMs
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs
                            << " decoded_ts=" << normalizedTimestampMs;
        if (m_seekRetryTimer) {
            m_seekRetryTimer->stop();
        }
    }

    return false;
}

void AudioSession::applyDecoderFlowControlBeforeWrite(bool isRemoteFlac, int fillLevelBeforeWrite)
{
    const int flowControlPauseThreshold = isRemoteFlac ? 96 : 85;
    if (fillLevelBeforeWrite > flowControlPauseThreshold
            && !m_buffering.decoderPausedByFlowControl
            && m_decoder->isDecoding()) {
        m_decoder->pauseDecode();
        m_buffering.decoderPausedByFlowControl = true;
        infoLog("BUFFER_EVT") << "Buffer full, pause decoder fill=" << fillLevelBeforeWrite << "%";
    }
}

bool AudioSession::shouldResumeFromDecodedWrite(int fillLevel, int bufferedBytes, bool isRemoteFlac,
                                                int& requiredLevel, int& requiredBytes) const
{
    const bool startupPath = (!m_player->isPlaying() && m_active);
    const bool resumePath = (m_player->isPlaying() && m_player->isPaused());

    requiredLevel = 85;
    requiredBytes = 512 * 1024;
    if (startupPath) {
        requiredLevel = 40;
        requiredBytes = 256 * 1024;
    } else if (resumePath) {
        requiredLevel = 25;
        requiredBytes = 192 * 1024;
    }

    if (m_seek.fastStartMode) {
        if (isRemoteFlac) {
            requiredLevel = 14;
            requiredBytes = 320 * 1024;
        } else {
            requiredLevel = 4;
            requiredBytes = 24 * 1024;
        }
    } else if (isRemoteFlac) {
        if (startupPath) {
            requiredLevel = qMax(requiredLevel, 22);
            requiredBytes = qMax(requiredBytes, 512 * 1024);
        } else if (resumePath) {
            requiredLevel = qMax(requiredLevel, 20);
            requiredBytes = qMax(requiredBytes, 512 * 1024);
        }
    }

    const bool readyByBytes = bufferedBytes >= requiredBytes;
    const bool readyByLevel = fillLevel >= requiredLevel;
    if (!isRemoteFlac) {
        return readyByBytes || readyByLevel;
    }

    return m_seek.fastStartMode ? (readyByBytes || readyByLevel) : (readyByBytes && readyByLevel);
}

void AudioSession::startPlayerFromDecodedReady(int fillLevel)
{
    // Do not anchor to latest decoded timestamp here, otherwise
    // pre-buffered data can push UI progress forward abruptly.
    qint64 startTimestamp = m_clock.baseTimestampMs;
    if (startTimestamp < 0) {
        startTimestamp = 0;
    }
    m_player->setCurrentTimestamp(startTimestamp);
    m_player->start();
    infoLog("BUFFER_EVT") << "Initial playback started (onDataDecoded) fill=" << fillLevel << "%";
}

void AudioSession::resumePlayerFromDecodedReady(int fillLevel, int bufferedBytes, int requiredLevel,
                                                int requiredBytes)
{
    m_player->resume();
    infoLog("BUFFER_EVT") << "Buffer filled, resuming playback bytes="
                          << bufferedBytes << "/" << requiredBytes
                          << " fill=" << fillLevel << "/" << requiredLevel;
}

void AudioSession::completeDecodedReadyResume()
{
    m_lastResumeTimer.restart();
    m_buffering.hasRecentResume = true;
    m_seek.fastStartMode = false;
    emit bufferingFinished();
}

bool AudioSession::shouldDebounceBufferingEnter() const
{
    return m_buffering.hasRecentResume
            && m_lastResumeTimer.isValid()
            && m_lastResumeTimer.elapsed() < 220;
}

bool AudioSession::handleTailLowBufferFallback(qint64 currentPos, int fillLevel, int bufferedBytes)
{
    const qint64 remainingToDuration = (m_duration > 0) ? (m_duration - currentPos) : -1;
    if (remainingToDuration >= 0 && remainingToDuration <= 1500) {
        qint64 finalDuration = m_duration;
        if (m_clock.lastDecodedMs > finalDuration) {
            finalDuration = m_clock.lastDecodedMs;
        }
        if (currentPos > finalDuration) {
            finalDuration = currentPos;
        }
        if (finalDuration > 0 && finalDuration != m_duration) {
            m_duration = finalDuration;
            emit durationChanged(m_duration);
        }
        infoLog("BUFFER_EVT") << "Tail low-buffer finish position=" << currentPos
                              << " duration=" << m_duration
                              << " remaining=" << remainingToDuration
                              << " bufferedBytes=" << bufferedBytes;
        onPlaybackFinished();
        return true;
    }

    if (!isDecodeCompletedNow()
            && m_duration > 0
            && currentPos >= m_duration - 8000
            && m_clock.lastDecodedMs > 0
            && currentPos >= m_clock.lastDecodedMs + 800
            && bufferedBytes < 16 * 1024) {
        warnLog("BUFFER_EVT") << "Tail fallback finish without decodeCompleted."
                              << " position=" << currentPos
                              << " duration=" << m_duration
                              << " lastDecoded=" << m_clock.lastDecodedMs
                              << " bufferedBytes=" << bufferedBytes;
        onPlaybackFinished();
        return true;
    }

    if (isDecodeCompletedNow()) {
        qint64 tailRef = m_duration;
        if (m_clock.lastDecodedMs > tailRef) {
            tailRef = m_clock.lastDecodedMs;
        }
        const qint64 remainingMs = tailRef > 0 ? (tailRef - currentPos) : -1;

        if (fillLevel <= 2 || (remainingMs >= 0 && remainingMs <= 1500)) {
            infoLog("BUFFER_EVT") << "Tail drained after decode complete, finishing playback"
                                  << " fill=" << fillLevel << "% remaining=" << remainingMs << "ms";
            onPlaybackFinished();
            return true;
        }

        infoLog("BUFFER_EVT") << "Decode complete, skip buffering pause at tail"
                              << " fill=" << fillLevel << "% remaining=" << remainingMs << "ms";
        return true;
    }

    return false;
}

bool AudioSession::canExitBufferingByThreshold(int fillLevel, int bufferedBytes, bool isRemoteFlac,
                                               int bufferingResumeMinBytes) const
{
    int resumeLevelThreshold = m_seek.fastStartMode ? 6 : 25;
    if (isRemoteFlac) {
        const int flacLevel = m_seek.fastStartMode ? 16 : 20;
        resumeLevelThreshold = qMax(resumeLevelThreshold, flacLevel);
    }

    if (isRemoteFlac) {
        return fillLevel >= resumeLevelThreshold && bufferedBytes >= bufferingResumeMinBytes;
    }
    return fillLevel >= resumeLevelThreshold || bufferedBytes >= bufferingResumeMinBytes;
}

void AudioSession::exitBufferingAndResume(int fillLevel, int bufferedBytes)
{
    m_buffering.active = false;
    if (!m_player->isPlaying() && m_active) {
        qint64 startTimestamp = m_clock.baseTimestampMs;
        if (startTimestamp < 0) {
            startTimestamp = 0;
        }
        m_player->setCurrentTimestamp(startTimestamp);
        m_player->start();
        infoLog("BUFFER_EVT") << "Initial playback started fill=" << fillLevel << "%";
    } else {
        m_player->resume();
    }
    m_lastResumeTimer.restart();
    m_buffering.hasRecentResume = true;

    m_decoder->startDecode();
    emit bufferingFinished();
    m_seek.fastStartMode = false;
    infoLog("BUFFER_EVT") << "Buffering finished fill=" << fillLevel
                          << "% bufferedBytes=" << bufferedBytes;
}

// ===== Seek 重试与退化策略 =====
bool AudioSession::isHlsSeekSource() const
{
    const QString sourcePath = m_sourceUrl.path().toLower();
    return sourcePath.endsWith(".m3u8") || m_sourceUrl.toString().contains("/hls/");
}

qint64 AudioSession::calculateSeekHardTimeoutMs(bool isHlsSeek, bool isRemoteFlacSeek) const
{
    if (isHlsSeek) {
        return qMin<qint64>(m_seekLatency.hardTimeoutMs, 12000);
    }
    if (isRemoteFlacSeek) {
        return 12000;
    }
    return qMax<qint64>(m_seekLatency.hardTimeoutMs, 30000);
}

qint64 AudioSession::calculateSeekGiveUpTimeoutMs(bool isHlsSeek, bool isRemoteFlacSeek,
                                                  qint64 hardTimeoutMs) const
{
    if (isHlsSeek) {
        return qMax<qint64>(hardTimeoutMs, 45000);
    }
    if (isRemoteFlacSeek) {
        return qMax<qint64>(hardTimeoutMs, 15000);
    }
    return qMax<qint64>(hardTimeoutMs, 60000);
}

AudioSession::SeekRetryAction AudioSession::decideSeekRetryAction(qint64 elapsedMs, qint64 hardTimeoutMs,
                                                                  qint64 giveUpTimeoutMs) const
{
    if (elapsedMs >= giveUpTimeoutMs) {
        return SeekRetryAction::GiveUp;
    }
    if (elapsedMs >= hardTimeoutMs) {
        return SeekRetryAction::DegradeByHardTimeout;
    }
    if (m_seek.retryCount >= m_seek.maxRetries) {
        return SeekRetryAction::DegradeByRetryBudget;
    }
    return SeekRetryAction::RetryNow;
}

void AudioSession::handleSeekRetryGiveUp(qint64 elapsedMs)
{
    warnLog("SEEK_EVT") << "[SEEK_ABORT] elapsed=" << elapsedMs
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;
    finalizeSeekLatency("seek_timeout_no_packet");
    m_buffering.active = false;
    emit bufferingFinished();
    emit sessionError(QStringLiteral("音频缓冲超时，请重试"));
}

void AudioSession::handleSeekRetryDegrade(qint64 elapsedMs, int nextRetryMs, const char* logTag)
{
    warnLog("SEEK_EVT") << logTag
                        << " elapsed=" << elapsedMs
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;
    if (!m_buffering.active) {
        m_buffering.active = true;
        emit bufferingStarted();
    }
    emit bufferingProgress(qMax(1, m_buffering.percent));
    requestDecoderStart();
    if (m_seekRetryTimer) {
        m_seekRetryTimer->start(nextRetryMs);
    }
}

void AudioSession::handleSeekRetryAttempt(qint64 elapsedMs, bool avoidRepeatReseek)
{
    ++m_seek.retryCount;
    warnLog("SEEK_EVT") << "[SEEK_RETRY] retry=" << m_seek.retryCount << "/" << m_seek.maxRetries
                        << " elapsed=" << elapsedMs
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;

    if (!avoidRepeatReseek || m_seek.retryCount <= 1) {
        m_decoder->seekTo(m_seekLatency.workingTargetMs);
        m_clock.minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
    } else {
        warnLog("SEEK_EVT") << "[SEEK_WAIT] skip extra reseek"
                            << " elapsed=" << elapsedMs
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs;
    }

    requestDecoderStart();
    if (!m_buffering.active) {
        m_buffering.active = true;
        emit bufferingStarted();
    }
    if (m_seekRetryTimer) {
        const int nextMs = 650 + m_seek.retryCount * 350;
        m_seekRetryTimer->start(nextMs);
    }
}

// ===== 状态迁移与断言校验 =====
void AudioSession::setSeekPhase(SeekPhase phase, const char* reason)
{
    if (m_seek.phase == phase) {
        return;
    }
    infoLog("STATE_EVT") << "SeekPhase " << static_cast<int>(m_seek.phase)
                         << " -> " << static_cast<int>(phase)
                         << " reason=" << reason;
    m_seek.phase = phase;
}

void AudioSession::setSeekLatencyPhase(SeekLatencyPhase phase, const char* reason)
{
    if (m_seekLatency.phase == phase) {
        return;
    }
    infoLog("STATE_EVT") << "SeekLatencyPhase " << static_cast<int>(m_seekLatency.phase)
                         << " -> " << static_cast<int>(phase)
                         << " reason=" << reason;
    m_seekLatency.phase = phase;
}

void AudioSession::setDecodePhase(DecodePhase phase, const char* reason)
{
    if (m_decode.phase == phase) {
        return;
    }
    infoLog("STATE_EVT") << "DecodePhase " << static_cast<int>(m_decode.phase)
                         << " -> " << static_cast<int>(phase)
                         << " reason=" << reason;
    m_decode.phase = phase;
}

void AudioSession::validateState(const char* where) const
{
#ifndef QT_NO_DEBUG
    const char* checkPoint = where ? where : "unknown";
    Q_ASSERT_X(!(m_playback.manualPaused && m_buffering.active),
               checkPoint, "manualPaused and buffering.active cannot both be true");
    Q_ASSERT_X(!(m_seek.phase == SeekPhase::Idle && m_seekGraceTimer && m_seekGraceTimer->isActive()),
               checkPoint, "seek grace timer active while phase is idle");
    Q_ASSERT_X(!(m_seekLatency.phase == SeekLatencyPhase::Inactive && m_seekRetryTimer && m_seekRetryTimer->isActive()),
               checkPoint, "seek retry timer active while seek latency is inactive");
    if (m_seekLatency.phase == SeekLatencyPhase::FirstPacketSeen) {
        Q_ASSERT_X(m_seekLatency.firstPacketTimestampMs >= 0,
                   checkPoint, "firstPacketTimestampMs must be initialized in FirstPacketSeen phase");
    }
#else
    Q_UNUSED(where);
#endif
}

void AudioSession::cleanup()
{
    stop();
}

void AudioSession::requestDecoderStart()
{
    if (!m_decoder) {
        return;
    }
    m_decoder->startDecode();
}

void AudioSession::finalizeControlPath(const char* where)
{
    validateState(where);
}

QDebug AudioSession::infoLog(const char* domain) const
{
    QDebug dbg = qDebug().noquote().nospace();
    dbg << '[' << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << ']'
        << '[' << domain << ']'
        << "[session=" << m_sessionId << "] ";
    return dbg;
}

QDebug AudioSession::warnLog(const char* domain) const
{
    QDebug dbg = qWarning().noquote().nospace();
    dbg << '[' << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << ']'
        << '[' << domain << ']'
        << "[session=" << m_sessionId << "] ";
    return dbg;
}

void AudioSession::finalizeSeekLatency(const char* stage)
{
    if (!isSeekLatencyActiveNow()) {
        return;
    }

    const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
    const QString metricName = QString("[SEEK_METRIC] seek_to_%1_ms:").arg(QString::fromUtf8(stage));
    infoLog("SEEK_EVT") << metricName << elapsedMs
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs
                        << " first_decoded_seen=" << isSeekFirstPacketSeenNow()
                        << " first_decoded_ts=" << m_seekLatency.firstPacketTimestampMs;
    setSeekLatencyPhase(SeekLatencyPhase::Inactive, "finalizeSeekLatency");
    m_seekLatency.desiredTargetMs = 0;
    m_seek.fastStartMode = false;
    m_seek.retryCount = 0;
    m_seekLatency.accuracyRetryCount = 0;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
}





