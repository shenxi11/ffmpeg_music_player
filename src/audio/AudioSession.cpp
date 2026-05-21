#include "AudioSession.h"

#include "AudioCacheManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>
#include <QUrlQuery>
#include <chrono>
#include <future>
#include <thread>

namespace {

bool initDecoderWithEventPumping(AudioDecoder* decoder, const QString& source) {
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

bool isLikelyFlacSource(const QUrl& sourceUrl) {
    if (sourceUrl.path().toLower().endsWith(".flac")) {
        return true;
    }

    // 本地代理模式：真实源地址放在 src 查询参数中。
    const QUrlQuery query(sourceUrl);
    const QString src = query.queryItemValue(QStringLiteral("src")).toLower();
    return src.contains(QStringLiteral(".flac"));
}

bool isLikelyMp3Source(const QUrl& sourceUrl) {
    if (sourceUrl.path().toLower().endsWith(".mp3")) {
        return true;
    }

    const QUrlQuery query(sourceUrl);
    const QString src = query.queryItemValue(QStringLiteral("src")).toLower();
    return src.contains(QStringLiteral(".mp3"));
}

bool isLocalProxySource(const QUrl& sourceUrl) {
    const QString host = sourceUrl.host().toLower();
    return sourceUrl.scheme().startsWith("http") &&
           (host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost"));
}

QUrl seekObservationSourceUrl(const QUrl& playbackUrl) {
    if (!isLocalProxySource(playbackUrl)) {
        return playbackUrl;
    }

    const QUrlQuery query(playbackUrl);
    const QUrl sourceUrl(query.queryItemValue(QStringLiteral("src")));
    return sourceUrl.isValid() && !sourceUrl.isEmpty() ? sourceUrl : playbackUrl;
}

int seekGraceMsForSource(const QUrl& sourceUrl) {
    const bool localProxy = isLocalProxySource(sourceUrl);
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

int seekRetryInitialMsForSource(const QUrl& sourceUrl) {
    const bool localProxy = isLocalProxySource(sourceUrl);
    const bool remoteHttp = sourceUrl.scheme().startsWith("http");
    const bool flacLike = isLikelyFlacSource(sourceUrl);

    if (localProxy) {
        return flacLike ? 5000 : 3200;
    }
    if (remoteHttp) {
        return flacLike ? 1800 : 1200;
    }
    return 500;
}

double seekHintConfidenceFromScore(int confidenceScore) {
    return qBound(0.0, static_cast<double>(confidenceScore) / 100.0, 1.0);
}

constexpr int kOutputPcmBytesPerSecond = 44100 * 2 * 2;
constexpr int kRemoteFlacEnterBufferMs = 1200;
constexpr int kRemoteFlacStartupBufferMs = 2500;
constexpr int kRemoteFlacSeekResumeBufferMs = 700;
constexpr int kRemoteFlacSeekResumeBufferMsWithHint = 450;
constexpr int kRemoteFlacResumeBufferMs = 2000;

int pcmBytesForMs(int milliseconds) {
    return static_cast<int>((static_cast<qint64>(kOutputPcmBytesPerSecond) *
                             qMax(milliseconds, 1) + 999) /
                            1000);
}

int pcmBufferedMsFromBytes(int bufferedBytes) {
    return static_cast<int>((static_cast<qint64>(qMax(bufferedBytes, 0)) * 1000) /
                            kOutputPcmBytesPerSecond);
}

int remoteFlacResumeBufferMs(bool startupPath, bool fastStartMode, bool firstPacketSeen,
                             bool hasSeekHint) {
    if (startupPath) {
        return kRemoteFlacStartupBufferMs;
    }
    if (fastStartMode) {
        if (firstPacketSeen && hasSeekHint) {
            return kRemoteFlacSeekResumeBufferMsWithHint;
        }
        return kRemoteFlacSeekResumeBufferMs;
    }
    return kRemoteFlacResumeBufferMs;
}

} // namespace

AudioSession::AudioSession(const QString& sessionId, QObject* parent)
    : QObject(parent), m_sessionId(sessionId), m_active(false), m_decoder(new AudioDecoder(this)),
      m_player(&AudioPlayer::instance()), m_duration(0), m_seekGraceTimer(new QTimer(this)),
      m_seekRetryTimer(new QTimer(this)), m_bufferingWatchdogTimer(new QTimer(this)),
      m_seekLatencyTimer() {
    m_seekGraceTimer->setSingleShot(true);
    connect(m_seekGraceTimer, &QTimer::timeout, this, &AudioSession::onSeekGraceTimeout);

    m_seekRetryTimer->setSingleShot(true);
    connect(m_seekRetryTimer, &QTimer::timeout, this, &AudioSession::onSeekRetryTimeout);

    m_bufferingWatchdogTimer->setInterval(300);
    connect(m_bufferingWatchdogTimer, &QTimer::timeout, this,
            &AudioSession::onBufferingWatchdogTimeout);

    connectSignals();
}

AudioSession::~AudioSession() {
    cleanup();
}

void AudioSession::onSeekGraceTimeout() {
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

bool AudioSession::loadSource(const QUrl& url) {
    auto t0 = std::chrono::high_resolution_clock::now();

    m_sourceUrl = url;
    m_duration = 0;
    m_hasStableDuration = false;

    // 统一为路径或 URL 字符串，便于从路径提取默认标题。
    QString filePath;
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    } else {
        filePath = url.toString();
    }

    // 默认先根据文件名生成标题，后续可被音频元数据覆盖。
    QFileInfo fileInfo(filePath);
    m_title = fileInfo.completeBaseName(); // 优先使用去掉扩展名后的基础文件名
    if (m_title.isEmpty()) {
        m_title = fileInfo.fileName(); // 兜底使用完整文件名
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

void AudioSession::play() {
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
                           << std::chrono::duration<double, std::milli>(t1 - t0).count() << "ms";

    if (resumeFailedRemoteFlacSeekIfNeeded("play")) {
        finalizeControlPath("play_flac_seek_recovery");
        return;
    }

    enterBufferingWaitForDecodedWrite("play", false);
    finalizeControlPath("play");
}

void AudioSession::pause() {
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
    m_buffering.decodedWriteSinceEnter = false;
    stopBufferingWatchdog();
    m_decoder->pauseDecode();
    if (isWriteOwnerActive()) {
        m_player->pause();
    }

    // Emit pause immediately from session layer. AudioPlayer's queued pause signal
    // can arrive after ownership has switched to video and would be filtered out.
    emit sessionPaused();
    finalizeControlPath("pause");
}

void AudioSession::resume() {
    m_playback.resumeSignalPending = true;
    m_playback.manualPaused = false;
    m_playback.internalPausePending = false;
    setDecodePhase(DecodePhase::Running, "resume");

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = handleResumeOwnerTakeover(currentOwner);

    requestDecoderStart();

    if (resumeFailedRemoteFlacSeekIfNeeded(
            takingOverFromOtherOwner ? "resume_owner_takeover" : "resume")) {
        finalizeControlPath(takingOverFromOtherOwner ? "resume_owner_takeover_flac_seek_recovery"
                                                     : "resume_flac_seek_recovery");
        return;
    }

    if (takingOverFromOtherOwner) {
        finalizeControlPath("resume_owner_takeover");
        return;
    }

    if (shouldHoldResumeForDecodedWrite("resume")) {
        finalizeControlPath("resume_wait_decoded_write");
        return;
    }

    startOrResumePlayerOutput();
    finalizeControlPath("resume");
}

void AudioSession::stop() {
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

void AudioSession::cancelFailedRemoteFlacRecoveryForUserSeek() {
    clearRemoteFlacSeekRecovery("user_seek");
}

void AudioSession::seekTo(qint64 positionMs) {
    if (!m_decoder || !m_player) {
        warnLog("SEEK_EVT") << "seekTo ignored while session destroying";
        return;
    }

    clearRemoteFlacSeekRecovery("new_seek");
    beginSeekLatencyTracking(positionMs);
    infoLog("SEEK_EVT") << "seek start target=" << positionMs
                        << "ms seek_id=" << m_seekLatency.seekId
                        << "click=1 source=" << m_sourceUrl.toString();
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

    const AudioDecoder::SeekMode seekMode =
        isRemoteFastSeekSource() ? AudioDecoder::SeekMode::Fast : AudioDecoder::SeekMode::Precise;
    dispatchDecoderSeekForLocalSeek(positionMs, seekMode);

    // m_isSeeking 会在 seekGraceTimer 超时后复位。
    infoLog("SEEK_EVT") << "seek pipeline dispatched";
    finalizeControlPath("seekTo");
}

void AudioSession::setVolume(int volume) {
    m_player->setVolume(volume);
}

bool AudioSession::isPlaying() const {
    // Session-level playing must be ownership-aware.
    return m_active && isWriteOwnerActive() && m_player->isPlaying() && !m_player->isPaused();
}

bool AudioSession::isPaused() const {
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

qint64 AudioSession::duration() const {
    return m_duration;
}

qint64 AudioSession::position() const {
    return m_player->getCurrentTimestamp();
}

void AudioSession::onDecodedData(const QByteArray& data, qint64 timestampMs, int seekGeneration) {
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        return;
    }

    if (seekGeneration < m_clock.minAcceptedDecodeGeneration) {
        static int staleDropCount = 0;
        if (++staleDropCount % 50 == 1) {
            infoLog("STATE_EVT") << "Drop stale decoded frame generation=" << seekGeneration
                                 << " expected>=" << m_clock.minAcceptedDecodeGeneration
                                 << " seek_id=" << m_seek.activeSeekId
                                 << " dispatched_generation=" << m_seek.dispatchedGeneration;
        }
        return;
    }
    if (m_flacSeekRecovery.failed &&
        seekGeneration <= m_flacSeekRecovery.rejectedGeneration) {
        static int flacRecoveryDropCount = 0;
        if (++flacRecoveryDropCount % 25 == 1) {
            logRemoteFlacSeekRecoveryState("drop_failed_generation", "onDecodedData");
        }
        return;
    }
    if (m_flacSeekRecovery.recoveryInProgress && !m_buffering.active) {
        logRemoteFlacSeekRecoveryState("drop_until_recovery_buffering", "onDecodedData");
        return;
    }

    qint64 normalizedTimestampMs = timestampMs;
    normalizeDecodedTimestamp(timestampMs, normalizedTimestampMs);
    m_clock.lastDecodedMs = normalizedTimestampMs;
    extendDurationFromDecodedTimestamp(normalizedTimestampMs);

    if (handleSeekLatencyOnDecodedTimestamp(timestampMs, normalizedTimestampMs, seekGeneration)) {
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
    markBufferingDecodedWrite(normalizedTimestampMs, bufferedBytes, seekGeneration);

    int requiredLevel = 0;
    int requiredBytes = 0;
    const bool readyToResume = shouldResumeFromDecodedWrite(fillLevel, bufferedBytes, isRemoteFlac,
                                                            requiredLevel, requiredBytes);

    if (m_buffering.active && readyToResume) {
        m_buffering.active = false;

        if (!m_player->isPlaying() && m_active) {
            startPlayerFromDecodedReady(fillLevel);
        } else {
            resumePlayerFromDecodedReady(fillLevel, bufferedBytes, requiredLevel, requiredBytes);
        }
        completeDecodedReadyResume();
    }

    if (!m_buffering.active && isSeekLatencyActiveNow() && isSeekFirstPacketSeenNow()) {
        finalizeSeekLatency("decoded_write_no_buffering");
    }
}

void AudioSession::onMetadataReady(qint64 durationMs, int sampleRate, int channels) {
    Q_UNUSED(sampleRate)
    Q_UNUSED(channels)

    if (durationMs > 0) {
        m_hasStableDuration = true;
        publishDuration(durationMs, "ffmpeg");
        emit metadataReady(m_title, m_artist, m_duration);
        return;
    }

    const qint64 cachedDurationMs = fallbackDurationFromCache();
    if (cachedDurationMs > 0) {
        m_hasStableDuration = true;
        publishDuration(cachedDurationMs, "cache");
        emit metadataReady(m_title, m_artist, m_duration);
        return;
    }

    warnLog("SESSION_EVT") << "Ignore invalid metadata duration=" << durationMs
                            << " source=" << m_sourceUrl.toString();
    emit metadataReady(m_title, m_artist, m_duration);
}

void AudioSession::onAudioTagsReady(const QString& title, const QString& artist) {
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

void AudioSession::onAlbumArtReady(const QString& imagePath) {
    m_albumArt = imagePath;
    emit albumArtReady(imagePath);
}

void AudioSession::onDecodeError(const QString& error) {
    if (isRemoteFlacSeekFirstPacketError(error)) {
        failRemoteFlacSeek("flac_seek_first_packet_error",
                           QStringLiteral("FLAC seek failed: %1").arg(error));
        validateState("onDecodeError:flac_seek_first_packet_error");
        return;
    }

    if (isSeekLatencyActiveNow()) {
        warnLog("SEEK_EVT") << "seek failed on decode error desired="
                            << m_seekLatency.desiredTargetMs
                            << " seek_id=" << m_seekLatency.seekId
                            << " working=" << m_seekLatency.workingTargetMs << " error=" << error
                            << " elapsed="
                            << (m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1)
                            << "ms";
        setSeekLatencyPhase(SeekLatencyPhase::Inactive, "decode_error");
    }
    if (m_buffering.active) {
        m_buffering.active = false;
        m_buffering.decodedWriteSinceEnter = false;
        stopBufferingWatchdog();
        emit bufferingFinished();
    }
    m_seek.fastStartMode = false;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
    validateState("onDecodeError");
    emit sessionError(error);
}

void AudioSession::onPlaybackError(const QString& error) {
    emit sessionError(error);
}

void AudioSession::onPositionChanged(qint64 positionMs) {
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    emit positionChanged(positionMs);

    // 仅在解码完成后，才允许按时长阈值收敛到 finished。
    if (m_duration > 0 && positionMs >= m_duration - 100) {
        if (!m_hasStableDuration && !isDecodeCompletedNow() && positionMs > m_duration + 500) {
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
                infoLog("SESSION_EVT") << "Position reached metadata duration but decode not "
                                          "completed, waiting for EOS";
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
                                      << " position=" << positionMs << " duration=" << m_duration
                                      << " bufferedBytes=" << bufferedBytes;
                onPlaybackFinished();
            }
        }
    }
}

void AudioSession::onBufferStatusChanged(int fillLevel) {
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
    int enterThresholdMs = 0;
    int resumeThresholdMs = 0;
    if (isRemoteFlac) {
        enterThresholdMs = kRemoteFlacEnterBufferMs;
        resumeThresholdMs =
            remoteFlacResumeBufferMs(false, m_seek.fastStartMode, isSeekFirstPacketSeenNow(),
                                     m_seekLatency.seekIndexHit || m_seekLatency.hintedByte >= 0);
        kBufferingEnterMinBytes = pcmBytesForMs(enterThresholdMs);
        kBufferingResumeMinBytes = pcmBytesForMs(resumeThresholdMs);
    }

    // 水位回落后恢复此前因流控暂停的解码线程。
    const int bufferedMs = pcmBufferedMsFromBytes(bufferedBytes);
    const bool flowResumeReady =
        isRemoteFlac ? bufferedMs <= 1800 : fillLevel < 60;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool flowHoldElapsed =
        m_buffering.lastFlowControlChangeMs <= 0 ||
        nowMs - m_buffering.lastFlowControlChangeMs >= (isRemoteFlac ? 180 : 80);
    if (flowResumeReady && flowHoldElapsed && m_buffering.decoderPausedByFlowControl &&
        m_decoder->isDecoding()) {
        requestDecoderStart();
        m_buffering.decoderPausedByFlowControl = false;
        m_buffering.lastFlowControlChangeMs = nowMs;
        infoLog("BUFFER_EVT") << "Buffer low, resume decoder fill=" << fillLevel
                              << "% pcmBufferedMs=" << bufferedMs;
    }

    // 低水位进入 buffering，并暂停输出等待回填。
    // 仅当“百分比低 + 绝对字节也低”时才进入，避免大缓冲容量场景反复抖动。
    if (shouldDebounceBufferingEnter() && !m_buffering.active) {
        // 刚恢复播放后的短窗口内忽略一次低水位回落，避免状态闪烁。
        return;
    }

    if (fillLevel < 10 && bufferedBytes < kBufferingEnterMinBytes && m_player->isPlaying() &&
        !withinSeekGrace && !m_buffering.active) {
        const qint64 currentPos = m_player->getPlaybackPosition();
        if (handleTailLowBufferFallback(currentPos, fillLevel, bufferedBytes)) {
            return;
        }

        enterBufferingWaitForDecodedWrite("low_buffer_enter", true);
        m_playback.internalPausePending = true;
        m_player->pause();
        kickBufferingRecovery("low_buffer_enter", fillLevel, bufferedBytes);
        infoLog("BUFFER_EVT") << "Buffering started fill=" << fillLevel << "%"
                              << " bufferedBytes=" << bufferedBytes
                              << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                              << " enterThresholdMs=" << enterThresholdMs;
    }
    // 水位/字节恢复后退出 buffering（字节门槛优先，百分比兜底）。
    else if (m_buffering.active) {
        if (!canExitBufferingAfterDecodedWrite("buffer_status", fillLevel, bufferedBytes)) {
            return;
        }
        if (!canExitBufferingByThreshold(fillLevel, bufferedBytes, isRemoteFlac,
                                         kBufferingResumeMinBytes)) {
            return;
        }

        exitBufferingAndResume(fillLevel, bufferedBytes);
    }
}

void AudioSession::onBufferUnderrun() {
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        return;
    }

    warnLog("BUFFER_EVT") << "Buffer underrun detected";

    // 仅在未处于 buffering 状态时进入，避免重复触发。
    if (!m_buffering.active) {
        enterBufferingWaitForDecodedWrite("buffer_underrun", true);
        m_playback.internalPausePending = true;
        m_player->pause();
        const AudioBuffer* buffer = m_player->getBuffer();
        const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
        kickBufferingRecovery("buffer_underrun", m_player->bufferFillLevel(), bufferedBytes);
    }
}

void AudioSession::onDecodeCompleted() {
    setDecodePhase(DecodePhase::Completed, "decode_completed");
    m_buffering.tailWaitLogActive = false;
    if (!m_hasStableDuration && m_clock.lastDecodedMs > m_duration) {
        m_duration = m_clock.lastDecodedMs;
        emit durationChanged(m_duration);
        infoLog("SESSION_EVT") << "Final duration corrected to decoded tail=" << m_duration << "ms";
    }
    infoLog("SESSION_EVT") << "Decode completed";
    emit decodeFinished();

    // 解码完成仅表示数据生产结束；会话结束仍以实际播放完成事件为准。
}

void AudioSession::onDecodeStarted() {
    setDecodePhase(DecodePhase::Running, "decode_started");
    infoLog("SESSION_EVT") << "Decode started";
}

void AudioSession::onDecodePaused() {
    infoLog("SESSION_EVT") << "Decode paused";
}

void AudioSession::onDecodeStopped() {
    infoLog("SESSION_EVT") << "Decode stopped";
}

void AudioSession::onPlaybackStarted() {
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    infoLog("SESSION_EVT") << "Playback started";
    if (isSeekLatencyActiveNow()) {
        finalizeSeekLatencyOnPlaybackResumed();
    }

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

void AudioSession::onPlaybackPaused() {
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

void AudioSession::onPlaybackResumed() {
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

void AudioSession::onPlaybackStopped() {
    if (!m_active || !isWriteOwnerActive()) {
        return;
    }

    infoLog("SESSION_EVT") << "Playback stopped";
}

void AudioSession::onPlaybackFinished() {
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

void AudioSession::onSeekRetryTimeout() {
    if (!m_active || !isWriteOwnerActive() || !isSeekLatencyActiveNow() ||
        isSeekFirstPacketSeenNow() || m_playback.manualPaused) {
        return;
    }

    const AudioCacheManager::SeekHintStatus hintStatus =
        AudioCacheManager::instance().seekHintStatusForPlaybackUrl(
            m_sourceUrl, m_seekLatency.desiredTargetMs);
    const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
    if (hintStatus.hasHint) {
        infoLog("SEEK_EVT") << "seek_hint_status"
                            << " target_matches=" << hintStatus.targetMatches
                            << " seek_index_hit=" << hintStatus.seekIndexHit
                            << " takeover_started=" << hintStatus.takeoverStarted
                            << " takeover_pending=" << hintStatus.takeoverPending
                            << " takeover_headers_sent=" << hintStatus.takeoverHeadersSent
                            << " generation=" << hintStatus.generation
                            << " active_proxy_streams=" << hintStatus.activeProxyStreams
                            << " target_byte=" << hintStatus.targetByte
                            << " proxy_original_range_start="
                            << hintStatus.proxyOriginalRangeStart
                            << " proxy_effective_range_start="
                            << hintStatus.proxyEffectiveRangeStart
                            << " takeover_age_ms=" << hintStatus.takeoverAgeMs;
    }
    if (hintStatus.targetMatches && hintStatus.takeoverPending) {
        warnLog("SEEK_EVT") << "[SEEK_WAIT] retry_suppressed_by_proxy"
                            << " elapsed=" << elapsedMs
                            << " seek_hint_generation=" << hintStatus.generation
                            << " active_proxy_streams=" << hintStatus.activeProxyStreams
                            << " proxy_original_range_start="
                            << hintStatus.proxyOriginalRangeStart
                            << " proxy_effective_range_start="
                            << hintStatus.proxyEffectiveRangeStart
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " target_byte=" << hintStatus.targetByte;
        requestDecoderStart();
        enterBufferingWaitForDecodedWrite("seek_proxy_pending", !m_buffering.active, false);
        emit bufferingProgress(qMax(1, m_buffering.percent));
        if (m_seekRetryTimer) {
            m_seekRetryTimer->start(hintStatus.takeoverAgeMs >= 3000 ? 900 : 650);
        }
        finalizeControlPath("onSeekRetryTimeout.proxyPending");
        return;
    }

    const bool isHlsSeek = isHlsSeekSource();
    const bool isRemoteFlacSeek = isRemoteFlacSource();
    const bool indexedLocalProxySeek = m_seekLatency.seekIndexHit;
    const qint64 hardTimeoutMs = calculateSeekHardTimeoutMs(isHlsSeek, isRemoteFlacSeek);
    const qint64 giveUpTimeoutMs =
        calculateSeekGiveUpTimeoutMs(isHlsSeek, isRemoteFlacSeek, hardTimeoutMs);
    switch (decideSeekRetryAction(elapsedMs, hardTimeoutMs, giveUpTimeoutMs)) {
        case SeekRetryAction::GiveUp:
            handleSeekRetryGiveUp(elapsedMs);
            break;
        case SeekRetryAction::DegradeByHardTimeout:
            handleSeekRetryDegrade(
                elapsedMs, 1500,
                "[SEEK_DEGRADE] seek waiting exceeded hard timeout, keep buffering.");
            break;
        case SeekRetryAction::DegradeByRetryBudget:
            handleSeekRetryDegrade(elapsedMs, 1800,
                                   "[SEEK_DEGRADE] Slow network while seeking, still buffering.");
            break;
        case SeekRetryAction::RetryNow:
            handleSeekRetryAttempt(elapsedMs,
                                   indexedLocalProxySeek || isHlsSeek || isRemoteFlacSeek);
            break;
    }
    finalizeControlPath("onSeekRetryTimeout");
}

void AudioSession::onBufferingWatchdogTimeout() {
    if (!m_buffering.active) {
        stopBufferingWatchdog();
        return;
    }

    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive()) {
        stopBufferingWatchdog();
        return;
    }

    const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    const int fillLevel = m_player ? m_player->bufferFillLevel() : 0;
    const bool isRemoteFlac = isRemoteFlacSource();
    const int resumeThresholdMs =
        isRemoteFlac
            ? remoteFlacResumeBufferMs(false, m_seek.fastStartMode, isSeekFirstPacketSeenNow(),
                                       m_seekLatency.seekIndexHit || m_seekLatency.hintedByte >= 0)
            : 0;
    const int resumeMinBytes =
        isRemoteFlac ? pcmBytesForMs(resumeThresholdMs) : (64 * 1024);

    kickBufferingRecovery("buffering_watchdog", fillLevel, bufferedBytes);

    if (!canExitBufferingAfterDecodedWrite("buffering_watchdog", fillLevel, bufferedBytes)) {
        return;
    }

    if (canExitBufferingByThreshold(fillLevel, bufferedBytes, isRemoteFlac, resumeMinBytes)) {
        infoLog("BUFFER_EVT") << "buffering_watchdog_exit fill=" << fillLevel
                              << "% bufferedBytes=" << bufferedBytes
                              << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                              << " seek_id=" << m_seek.activeSeekId
                              << " decoded_resume_generation="
                              << m_buffering.decodedWriteGeneration
                              << " resumeThresholdMs=" << resumeThresholdMs;
        exitBufferingAndResume(fillLevel, bufferedBytes);
        return;
    }

    const bool nearResumeThreshold =
        resumeMinBytes > 0 && bufferedBytes >= qMax(0, resumeMinBytes - pcmBytesForMs(250));
    if (isSeekLatencyActiveNow() || nearResumeThreshold) {
        infoLog("BUFFER_EVT") << "buffering_watchdog_wait fill=" << fillLevel
                              << "% bufferedBytes=" << bufferedBytes
                              << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                              << " seek_id=" << m_seek.activeSeekId
                              << " resumeThresholdMs=" << resumeThresholdMs
                              << " decoderPausedByFlowControl="
                              << m_buffering.decoderPausedByFlowControl
                              << " manualPaused=" << m_playback.manualPaused
                              << " isDecoding=" << (m_decoder ? m_decoder->isDecoding() : false);
    }
}

bool AudioSession::isWriteOwnerActive() const {
    return m_player && m_player->writeOwner() == m_sessionId;
}

// ===== Seek 运行时状态与时间轴基础 =====
void AudioSession::resetRuntimeState() {
    m_playback = PlaybackState{};
    m_buffering = BufferingState{};
    m_clock = ClockState{};
    m_seek = SeekState{};
    m_seekLatency = SeekLatencyState{};
    m_flacSeekRecovery = FlacSeekRecoveryState{};
    m_decode = DecodeState{};
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
    stopBufferingWatchdog();
}

void AudioSession::beginSeekLatencyTracking(qint64 targetMs) {
    setSeekLatencyPhase(SeekLatencyPhase::WaitingFirstPacket, "beginSeekLatencyTracking");
    m_seek.activeSeekId = ++m_seek.nextSeekId;
    m_seekLatency.seekId = m_seek.activeSeekId;
    const AudioCacheManager::SeekHintStatus hintStatus =
        AudioCacheManager::instance().seekHintStatusForPlaybackUrl(m_sourceUrl, targetMs);
    m_seekLatency.seekIndexHit =
        (hintStatus.targetMatches && hintStatus.seekIndexHit) ||
        AudioCacheManager::instance().hasResolvedSeekIndexForPlaybackUrl(m_sourceUrl);
    m_seekLatency.desiredTargetMs = targetMs;
    m_seekLatency.workingTargetMs = targetMs;
    m_seekLatency.firstPacketTimestampMs = -1;
    m_seekLatency.rawFirstPacketTimestampMs = -1;
    m_seekLatency.hintedByte = hintStatus.targetMatches ? hintStatus.targetByte : -1;
    m_seekLatency.seekHintGeneration = hintStatus.targetMatches ? hintStatus.generation : 0;
    m_seekLatency.seekHintExact = hintStatus.targetMatches && hintStatus.exact;
    m_seekLatency.seekHintConfidence =
        hintStatus.targetMatches ? seekHintConfidenceFromScore(hintStatus.confidenceScore) : 0.0;
    if (m_seekLatency.seekHintConfidence <= 0.0 && m_seekLatency.seekIndexHit) {
        m_seekLatency.seekHintConfidence = 0.55;
    }
    m_seekLatency.proxySeekTakeover = false;
    m_seekLatency.largeDriftLogged = false;
    m_seekLatency.accuracyRetryCount = 0;
    m_seekLatency.accuracyMaxRetries =
        (m_seekLatency.seekIndexHit || isRemoteFastSeekSource()) ? 0 : 1;
    m_seekLatency.accuracyToleranceMs = m_seekLatency.seekIndexHit ? 6500 : 9000;
    m_seekLatency.hardTimeoutMs = m_seekLatency.seekIndexHit ? 6500 : 9000;
    m_seekLatencyTimer.restart();

    m_seek.fastStartMode = true;
    m_seek.retryCount = 0;
    m_seek.maxRetries = (m_seekLatency.seekIndexHit || isRemoteFastSeekSource()) ? 0 : 2;
    m_seek.dispatchedGeneration = 0;
    m_seek.dispatchedTargetMs = -1;
    m_buffering.decodedWriteSinceEnter = false;
    m_buffering.decodedWriteGeneration = 0;
    m_buffering.decodedWriteTimestampMs = -1;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->start(seekRetryInitialMsForSource(m_sourceUrl));
    }

    infoLog("SEEK_EVT") << "seek_target_ms=" << targetMs
                        << " seek_id=" << m_seekLatency.seekId
                        << " seek_index_hit=" << m_seekLatency.seekIndexHit
                        << " hinted_byte=" << m_seekLatency.hintedByte
                        << " seek_hint_generation=" << m_seekLatency.seekHintGeneration
                        << " seek_hint_exact=" << m_seekLatency.seekHintExact
                        << " seek_hint_confidence=" << m_seekLatency.seekHintConfidence
                        << " source=" << m_sourceUrl.toString();
}

void AudioSession::applySeekClockBase(qint64 targetMs) {
    m_clock.lastDecodedMs = targetMs;
    m_clock.lastPausedPlaybackMs = targetMs;
    m_clock.baseTimestampMs = targetMs;
    m_clock.offsetTimestampMs = 0;
    m_clock.timestampMode = DecoderTimestampMode::ProbePending;
    setDecodePhase(DecodePhase::Running, "applySeekClockBase");
}

void AudioSession::enterSeekGraceWindow() {
    setSeekPhase(SeekPhase::Grace, "enterSeekGraceWindow");
    if (m_seekGraceTimer) {
        m_seekGraceTimer->start(seekGraceMsForSource(m_sourceUrl));
    }
}

qint64 AudioSession::resolveResumeTimestamp() const {
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
bool AudioSession::shouldSuppressExternalPauseResumeSignals() const {
    return isSeekingNow() || m_buffering.active || m_playback.manualPaused;
}

bool AudioSession::consumeResumeSignalPendingAndEmit() {
    if (!m_playback.resumeSignalPending) {
        return false;
    }

    m_playback.resumeSignalPending = false;
    emit sessionResumed();
    return true;
}

bool AudioSession::emitSessionResumedByPolicy(ResumeEmitPolicy policy) {
    if (consumeResumeSignalPendingAndEmit()) {
        return true;
    }

    if (policy == ResumeEmitPolicy::PendingOrUnsuppressed &&
        !shouldSuppressExternalPauseResumeSignals()) {
        emit sessionResumed();
        return true;
    }

    return false;
}

bool AudioSession::clearInternalPauseIfOwnershipLost() {
    if (isWriteOwnerActive()) {
        return false;
    }

    // Owner handoff 后延迟到达的 paused 回调，清掉内部标记并忽略。
    m_playback.internalPausePending = false;
    return true;
}

bool AudioSession::consumeInternalPausePending() {
    if (!m_playback.internalPausePending) {
        return false;
    }

    m_playback.internalPausePending = false;
    infoLog("STATE_EVT") << "Ignore internal playbackPaused";
    return true;
}

void AudioSession::markPlaybackResumedActivity() {
    m_lastResumeTimer.restart();
    m_buffering.hasRecentResume = true;
}

void AudioSession::finalizeSeekLatencyOnPlaybackResumed() {
    if (!isSeekLatencyActiveNow()) {
        return;
    }

    if (isSeekFirstPacketSeenNow() && m_buffering.decodedWriteSinceEnter) {
        finalizeSeekLatency("playback_resumed");
        return;
    }

    infoLog("SEEK_EVT") << "skip finalize on playback_resumed before decoded write"
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs
                        << " first_decoded_seen=" << isSeekFirstPacketSeenNow()
                        << " decoded_write_since_enter=" << m_buffering.decodedWriteSinceEnter;
}

bool AudioSession::resumeFailedRemoteFlacSeekIfNeeded(const char* reason) {
    if (!m_flacSeekRecovery.failed || !isRemoteFlacSource()) {
        return false;
    }

    beginRemoteFlacSeekRecovery(reason);
    return true;
}

void AudioSession::beginRemoteFlacSeekRecovery(const char* reason) {
    const qint64 targetMs =
        qMax<qint64>(0, m_flacSeekRecovery.targetMs >= 0 ? m_flacSeekRecovery.targetMs
                                                         : resolveResumeTimestamp());
    m_flacSeekRecovery.failed = false;
    m_flacSeekRecovery.recoveryInProgress = true;
    m_flacSeekRecovery.targetMs = targetMs;
    logRemoteFlacSeekRecoveryState("resume_reseek", reason);

    if (m_player->isPlaying() && !m_player->isPaused()) {
        m_playback.internalPausePending = true;
        m_player->pause();
        m_playback.internalPausePending = false;
    }

    m_seek.pendingTakeoverSeekMs.reset();
    m_player->resetBuffer();
    m_player->setCurrentTimestamp(targetMs);
    beginSeekLatencyTracking(targetMs);
    m_flacSeekRecovery.seekId = m_seekLatency.seekId;
    applySeekClockBase(targetMs);
    setSeekPhase(SeekPhase::Seeking, "flac_seek_recovery_resume");
    enterSeekGraceWindow();
    enterBufferingWaitForDecodedWrite("flac_seek_recovery_resume", true);
    dispatchDecoderSeekForLocalSeek(targetMs, AudioDecoder::SeekMode::Fast);
}

bool AudioSession::shouldHoldResumeForDecodedWrite(const char* reason) {
    const bool waitingBufferWrite = m_buffering.active && !m_buffering.decodedWriteSinceEnter;
    const bool waitingSeekPacket = isSeekLatencyActiveNow() && !isSeekFirstPacketSeenNow();
    if (!waitingBufferWrite && !waitingSeekPacket) {
        return false;
    }

    if (!m_buffering.active) {
        enterBufferingWaitForDecodedWrite(reason, true);
    }

    const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    const int fillLevel = m_player ? m_player->bufferFillLevel() : 0;
    kickBufferingRecovery(reason, fillLevel, bufferedBytes);
    if (isRemoteFlacSource()) {
        logRemoteFlacSeekRecoveryState("resume_wait_decoded_write", reason);
    }
    return true;
}

// ===== Owner 接管与会话切换 =====
bool AudioSession::shouldTakeOverFromOwner(const QString& currentOwner,
                                           bool treatEmptyOwnerAsTakeover) const {
    if (currentOwner == m_sessionId) {
        return false;
    }
    if (currentOwner.isEmpty()) {
        return treatEmptyOwnerAsTakeover;
    }
    return true;
}

void AudioSession::pauseSharedPlayerForTakeover(bool takingOverFromOtherOwner) {
    if (!takingOverFromOtherOwner) {
        return;
    }
    if (!m_player->isPlaying() || m_player->isPaused()) {
        return;
    }
    m_playback.internalPausePending = true;
    m_player->pause();
}

void AudioSession::assignSessionAsWriteOwner() {
    m_player->setWriteOwner(m_sessionId);
}

void AudioSession::resetSharedOutputAfterTakeover(qint64 timestampMs) {
    m_player->stop();
    m_player->resetBuffer();
    m_player->setCurrentTimestamp(qMax<qint64>(0, timestampMs));
}

void AudioSession::deferSeekUntilOwnerTakeover(qint64 positionMs) {
    m_seek.pendingTakeoverSeekMs = positionMs;
    m_clock.minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration() + 1;
    infoLog("SEEK_EVT") << "Deferring seek until owner handoff resume target=" << positionMs
                        << "ms";
}

bool AudioSession::handleResumeOwnerTakeover(const QString& currentOwner) {
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

void AudioSession::applyResumeTakeoverAtTimestamp(const QString& currentOwner,
                                                  qint64 resumeTimestamp) {
    infoLog("SESSION_EVT") << "Resume takeover from owner=" << currentOwner
                           << " resumeTimestamp=" << resumeTimestamp
                           << " lastPaused=" << m_clock.lastPausedPlaybackMs
                           << " lastDecoded=" << m_clock.lastDecodedMs;

    m_seek.pendingTakeoverSeekMs.reset();
    applySeekClockBase(resumeTimestamp);
    enterSeekGraceWindow();
    resetSharedOutputAfterTakeover(resumeTimestamp);
    beginSeekLatencyTracking(resumeTimestamp);
    dispatchDecoderSeekForLocalSeek(resumeTimestamp,
                                    isRemoteFastSeekSource() ? AudioDecoder::SeekMode::Fast
                                                             : AudioDecoder::SeekMode::Precise);
    enterBufferingWaitForDecodedWrite("resume_owner_takeover", false);
}

void AudioSession::startOrResumePlayerOutput() {
    if (!m_player->isPlaying()) {
        m_player->start();
    } else if (m_player->isPaused()) {
        m_player->resume();
    }
}

// ===== 本地 seek 执行链 =====
bool AudioSession::isPlayerActivelyPlaying() const {
    return m_player->isPlaying() && !m_player->isPaused();
}

void AudioSession::pausePipelineForLocalSeek(bool wasPlaying) {
    if (!wasPlaying) {
        return;
    }

    m_decoder->pauseDecode();
    m_playback.internalPausePending = true;
    m_player->pause();
}

void AudioSession::prepareOutputForLocalSeek(qint64 positionMs, bool wasPlaying) {
    m_player->resetBuffer();
    m_player->setCurrentTimestamp(positionMs);

    if (wasPlaying) {
        enterBufferingWaitForDecodedWrite("local_seek", true);
    } else {
        m_buffering.decodedWriteSinceEnter = false;
    }
}

void AudioSession::dispatchDecoderSeekForLocalSeek(qint64 positionMs, AudioDecoder::SeekMode mode) {
    if (m_seek.activeSeekId <= 0) {
        m_seek.activeSeekId = ++m_seek.nextSeekId;
    }
    const int seekGeneration = m_decoder->seekTo(positionMs, mode, m_seek.activeSeekId);
    m_clock.minAcceptedDecodeGeneration = seekGeneration;
    m_seek.dispatchedGeneration = m_clock.minAcceptedDecodeGeneration;
    m_seek.dispatchedTargetMs = positionMs;
    requestDecoderStart();
}

// ===== 解码时间轴与缓冲恢复策略 =====
bool AudioSession::isRemoteFastSeekSource() const {
    return m_sourceUrl.scheme().startsWith("http") && !isHlsSeekSource();
}

bool AudioSession::isRemoteFlacSource() const {
    return m_sourceUrl.scheme().startsWith("http") && isLikelyFlacSource(m_sourceUrl);
}

void AudioSession::publishDuration(qint64 durationMs, const char* source) {
    if (durationMs <= 0 || m_duration == durationMs) {
        return;
    }

    m_duration = durationMs;
    emit durationChanged(m_duration);
    infoLog("SESSION_EVT") << "Duration updated source=" << source << " duration=" << m_duration
                            << "ms";
}

qint64 AudioSession::fallbackDurationFromCache() const {
    const qint64 cachedDurationMs = AudioCacheManager::instance().knownTrackDuration(m_sourceUrl);
    return cachedDurationMs > 0 ? cachedDurationMs : -1;
}

void AudioSession::normalizeDecodedTimestamp(qint64 timestampMs, qint64& normalizedTimestampMs) {
    if (m_clock.timestampMode == DecoderTimestampMode::ProbePending) {
        const qint64 delta = m_clock.baseTimestampMs - timestampMs;
        if (qAbs(delta) > 3000) {
            warnLog("SEEK_EVT")
                << "Seek target differs from decoded timeline, keep requested timeline."
                << " expected=" << m_clock.baseTimestampMs << " decoded=" << timestampMs
                << " delta=" << delta;
        }
        if (isRemoteFlacSource()) {
            m_clock.timestampMode = DecoderTimestampMode::Passthrough;
            m_clock.offsetTimestampMs = 0;
            if (qAbs(delta) > 3000) {
                warnLog("SEEK_EVT") << "[FLAC_SEEK_VERIFY] reject timestamp rebase"
                                    << " target=" << m_clock.baseTimestampMs
                                    << " decoded=" << timestampMs << " drift=" << -delta;
            }
            return;
        }
        const bool proxyTakeover =
            AudioCacheManager::instance().hasRecentProxySeekTakeoverForPlaybackUrl(
                m_sourceUrl, m_clock.baseTimestampMs);
        if (proxyTakeover && qAbs(delta) > 3000) {
            m_clock.offsetTimestampMs = delta;
            m_clock.timestampMode = DecoderTimestampMode::OffsetApplied;
            normalizedTimestampMs = timestampMs + m_clock.offsetTimestampMs;
            infoLog("SEEK_EVT") << "Apply proxy seek timestamp rebase."
                                << " target=" << m_clock.baseTimestampMs
                                << " decoded=" << timestampMs << " offset=" << delta
                                << " normalized=" << normalizedTimestampMs;
        } else {
            m_clock.timestampMode = DecoderTimestampMode::Passthrough;
            m_clock.offsetTimestampMs = 0;
        }
    }

    if (m_clock.timestampMode == DecoderTimestampMode::OffsetApplied) {
        normalizedTimestampMs = timestampMs + m_clock.offsetTimestampMs;
    }
}

void AudioSession::extendDurationFromDecodedTimestamp(qint64 normalizedTimestampMs) {
    if (m_hasStableDuration) {
        return;
    }

    if (!isDecodeCompletedNow() && normalizedTimestampMs > 0 &&
        (m_duration <= 0 || normalizedTimestampMs > m_duration + 500)) {
        constexpr qint64 kDurationHeadroomMs = 2500;
        const qint64 desiredDuration = normalizedTimestampMs + kDurationHeadroomMs;
        if (desiredDuration > m_duration) {
            publishDuration(desiredDuration, "decoded_timestamp");
            infoLog("SESSION_EVT") << "Extend dynamic duration=" << m_duration
                                   << "ms from decoded timestamp=" << normalizedTimestampMs;
        }
    }
}

bool AudioSession::handleSeekLatencyOnDecodedTimestamp(qint64 rawTimestampMs,
                                                       qint64& normalizedTimestampMs,
                                                       int seekGeneration) {
    if (!isSeekLatencyActiveNow() || isSeekFirstPacketSeenNow()) {
        return false;
    }

    m_seekLatency.rawFirstPacketTimestampMs = rawTimestampMs;
    qint64 driftMs = normalizedTimestampMs - m_seekLatency.desiredTargetMs;
    qint64 absDriftMs = qAbs(driftMs);
    const qint64 originalLandingDriftMs = driftMs;
    const qint64 acceptToleranceMs = m_seekLatency.accuracyToleranceMs;
    const bool remoteFlac = isRemoteFlacSource();
    const bool remoteMp3 =
        m_sourceUrl.scheme().startsWith("http") && !remoteFlac && isLikelyMp3Source(m_sourceUrl);
    const char* verifyTag = remoteFlac ? "[FLAC_SEEK_VERIFY]" : "[MP3_SEEK_VERIFY]";
    if (!remoteFlac && !m_seekLatency.proxySeekTakeover) {
        m_seekLatency.proxySeekTakeover =
            AudioCacheManager::instance().hasRecentProxySeekTakeoverForPlaybackUrl(
                m_sourceUrl, m_seekLatency.desiredTargetMs);
    }

    infoLog("SEEK_EVT") << verifyTag
                        << " remote_flac=" << remoteFlac
                        << " seek_id=" << m_seekLatency.seekId
                        << " decoded_generation=" << seekGeneration
                        << " expected_generation=" << m_clock.minAcceptedDecodeGeneration
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " raw_decoded=" << rawTimestampMs
                        << " decoded=" << normalizedTimestampMs << " drift=" << driftMs
                        << " seek_landing_drift_ms=" << driftMs
                        << " tolerance=" << acceptToleranceMs
                        << " hinted_byte=" << m_seekLatency.hintedByte
                        << " seek_hint_exact=" << m_seekLatency.seekHintExact
                        << " seek_hint_confidence=" << m_seekLatency.seekHintConfidence
                        << " retry=" << m_seekLatency.accuracyRetryCount << "/"
                        << m_seekLatency.accuracyMaxRetries;

    bool seekObservationRecorded = false;
    auto noteSeekObservation = [this, rawTimestampMs, &seekObservationRecorded](bool accepted) {
        if (m_seekLatency.hintedByte < 0) {
            return;
        }
        if (seekObservationRecorded) {
            return;
        }
        seekObservationRecorded = true;
        AudioCacheManager::instance().noteSeekIndexObservation(
            seekObservationSourceUrl(m_sourceUrl), m_seekLatency.desiredTargetMs,
            m_seekLatency.hintedByte, rawTimestampMs, accepted,
            m_seekLatency.seekHintGeneration);
    };

    auto rebaseToDesiredTarget = [this, rawTimestampMs, originalLandingDriftMs,
                                  &normalizedTimestampMs, &driftMs, &absDriftMs,
                                  noteSeekObservation]() {
        const qint64 previousNormalizedTimestampMs = normalizedTimestampMs;
        const qint64 offset = m_seekLatency.desiredTargetMs - previousNormalizedTimestampMs;
        m_clock.offsetTimestampMs += offset;
        m_clock.timestampMode = DecoderTimestampMode::OffsetApplied;
        normalizedTimestampMs = m_seekLatency.desiredTargetMs;
        m_clock.lastDecodedMs = normalizedTimestampMs;
        m_player->setCurrentTimestamp(normalizedTimestampMs);
        driftMs = normalizedTimestampMs - m_seekLatency.desiredTargetMs;
        absDriftMs = qAbs(driftMs);
        infoLog("SEEK_EVT") << "seek_clock_rebase"
                            << " seek_id=" << m_seekLatency.seekId
                            << " seek_accept_reason=clock_rebase_early_frame"
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " raw=" << rawTimestampMs
                            << " decoded=" << previousNormalizedTimestampMs
                            << " normalized=" << normalizedTimestampMs
                            << " seek_landing_drift_ms=" << originalLandingDriftMs
                            << " offset_delta=" << offset
                            << " hinted_byte=" << m_seekLatency.hintedByte
                            << " seek_hint_exact=" << m_seekLatency.seekHintExact
                            << " seek_hint_confidence=" << m_seekLatency.seekHintConfidence;
        noteSeekObservation(true);
    };

    if (remoteMp3 && absDriftMs > acceptToleranceMs) {
        const bool confidentHint =
            m_seekLatency.seekHintExact || m_seekLatency.seekHintConfidence >= 0.70 ||
            m_seekLatency.seekIndexHit || m_seekLatency.proxySeekTakeover;
        const bool earlyLanding = normalizedTimestampMs < m_seekLatency.desiredTargetMs;
        const qint64 rebaseLimitMs = confidentHint ? 90000 : 30000;
        if (earlyLanding && absDriftMs <= rebaseLimitMs) {
            rebaseToDesiredTarget();
            infoLog("SEEK_EVT") << "[MP3_SEEK_ACCEPT]"
                                << " early_frame_accepted_by_clock_rebase=true"
                                << " seek_id=" << m_seekLatency.seekId
                                << " seek_accept_reason=mp3_clock_rebase_early_frame"
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " raw_decoded=" << rawTimestampMs
                                << " normalized=" << normalizedTimestampMs
                                << " seek_landing_drift_ms=" << originalLandingDriftMs
                                << " seek_index_hit=" << m_seekLatency.seekIndexHit
                                << " seek_hint_exact=" << m_seekLatency.seekHintExact
                                << " seek_hint_confidence=" << m_seekLatency.seekHintConfidence;
        } else if (m_seekLatency.accuracyRetryCount == 0 &&
                   m_seekLatency.seekHintExact && absDriftMs > 120000) {
            noteSeekObservation(false);
            ++m_seekLatency.accuracyRetryCount;
            m_seekLatency.seekHintExact = false;
            m_seekLatency.seekHintConfidence = 0.0;
            m_seekLatency.seekIndexHit = false;
            m_seekLatency.hintedByte = -1;
            m_seekLatency.firstPacketTimestampMs = -1;
            m_seekLatency.rawFirstPacketTimestampMs = -1;
            setSeekLatencyPhase(SeekLatencyPhase::WaitingFirstPacket,
                                "mp3_seek_hintless_reseek_after_large_drift");
            warnLog("SEEK_EVT") << "[MP3_SEEK_RESEEK_HINTLESS]"
                                << " decoded timestamp far before target,"
                                << " dispatch precise demux reseek without exact hint."
                                << " seek_id=" << m_seekLatency.seekId
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " raw_decoded=" << rawTimestampMs
                                << " decoded=" << normalizedTimestampMs
                                << " seek_landing_drift_ms=" << originalLandingDriftMs
                                << " accept_limit=" << rebaseLimitMs;
            dispatchDecoderSeekForLocalSeek(m_seekLatency.desiredTargetMs,
                                            AudioDecoder::SeekMode::Precise);
            if (m_seekRetryTimer) {
                m_seekRetryTimer->start(900);
            }
            return true;
        } else {
            noteSeekObservation(false);
            setSeekLatencyPhase(SeekLatencyPhase::FirstPacketSeen,
                                "mp3_seek_accept_first_packet_non_exact");
            m_seekLatency.firstPacketTimestampMs = normalizedTimestampMs;
            m_seekLatency.workingTargetMs = normalizedTimestampMs;
            if (m_seekRetryTimer) {
                m_seekRetryTimer->stop();
            }
            m_seekLatency.seekHintExact = false;
            m_seekLatency.seekHintConfidence = 0.0;
            warnLog("SEEK_EVT") << "[MP3_SEEK_ACCEPT_NON_EXACT]"
                                << " accept first packet without timestamp wait"
                                << " seek_id=" << m_seekLatency.seekId
                                << " seek_accept_reason=mp3_non_exact_first_packet"
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " raw_decoded=" << rawTimestampMs
                                << " decoded=" << normalizedTimestampMs
                                << " seek_landing_drift_ms=" << originalLandingDriftMs
                                << " accept_limit=" << rebaseLimitMs
                                << " early_landing=" << earlyLanding;
            return false;
        }
    }

    if (absDriftMs > acceptToleranceMs) {
        if (m_seekLatency.accuracyRetryCount < m_seekLatency.accuracyMaxRetries) {
            ++m_seekLatency.accuracyRetryCount;
            warnLog("SEEK_EVT") << (remoteFlac ? "[FLAC_SEEK_REJECT]" : "[SEEK_REJECT_DRIFT]")
                                << " decoded timestamp too far from target,"
                                << " seek_id=" << m_seekLatency.seekId
                                << " retry=" << m_seekLatency.accuracyRetryCount << "/"
                                << m_seekLatency.accuracyMaxRetries
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " decoded=" << normalizedTimestampMs << " drift=" << driftMs
                                << " acceptTolerance=" << acceptToleranceMs
                                << " seek_index_hit=" << m_seekLatency.seekIndexHit;
            m_seekLatency.firstPacketTimestampMs = -1;
            setSeekLatencyPhase(SeekLatencyPhase::WaitingFirstPacket, "seek_reject_large_drift");
            const int seekGeneration = m_decoder->seekTo(m_seekLatency.desiredTargetMs,
                                                         AudioDecoder::SeekMode::Fast,
                                                         m_seekLatency.seekId);
            m_clock.minAcceptedDecodeGeneration = seekGeneration;
            m_seek.dispatchedGeneration = seekGeneration;
            m_seek.dispatchedTargetMs = m_seekLatency.desiredTargetMs;
            m_decoder->startDecode();
            if (m_seekRetryTimer) {
                m_seekRetryTimer->start(900);
            }
            return true;
        }

        if (!m_seekLatency.largeDriftLogged) {
            warnLog("SEEK_EVT") << (remoteFlac ? "[FLAC_SEEK_ABORT]" : "[SEEK_REJECT_DRIFT]")
                                << " decoded timestamp still too far after retries,"
                                << " keep buffering."
                                << " seek_id=" << m_seekLatency.seekId
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " decoded=" << normalizedTimestampMs << " drift=" << driftMs
                                << " acceptTolerance=" << acceptToleranceMs
                                << " seek_index_hit=" << m_seekLatency.seekIndexHit;
            m_seekLatency.largeDriftLogged = true;
        }
        if (remoteFlac) {
            failRemoteFlacSeek(
                "flac_seek_verified_drift_reject",
                QStringLiteral("FLAC seek failed: decoded timestamp drift too large"));
            return true;
        }
        if (remoteMp3) {
            noteSeekObservation(false);
            setSeekLatencyPhase(SeekLatencyPhase::FirstPacketSeen,
                                "mp3_seek_accept_non_exact_large_drift");
            m_seekLatency.firstPacketTimestampMs = normalizedTimestampMs;
            m_seekLatency.workingTargetMs = normalizedTimestampMs;
            if (m_seekRetryTimer) {
                m_seekRetryTimer->stop();
            }
            infoLog("SEEK_EVT") << "[MP3_SEEK_ACCEPT_NON_EXACT]"
                                << " resume without keep-buffering loop"
                                << " seek_id=" << m_seekLatency.seekId
                                << " seek_accept_reason=mp3_non_exact_large_drift"
                                << " desired=" << m_seekLatency.desiredTargetMs
                                << " raw_decoded=" << rawTimestampMs
                                << " decoded_ts=" << normalizedTimestampMs
                                << " seek_landing_drift_ms=" << driftMs
                                << " acceptTolerance=" << acceptToleranceMs;
            return false;
        }
        m_seekLatency.firstPacketTimestampMs = -1;
        setSeekLatencyPhase(SeekLatencyPhase::WaitingFirstPacket, "seek_large_drift_keep_waiting");
        if (m_seekRetryTimer && !m_seekRetryTimer->isActive()) {
            m_seekRetryTimer->start(1500);
        }
        return true;
    }

    if (!isSeekFirstPacketSeenNow()) {
        setSeekLatencyPhase(SeekLatencyPhase::FirstPacketSeen, "seek_accept_decoded_in_tolerance");
        noteSeekObservation(true);
        m_seekLatency.firstPacketTimestampMs = normalizedTimestampMs;
        m_seekLatency.workingTargetMs = normalizedTimestampMs;
        const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
        infoLog("SEEK_EVT") << "seek_to_first_decoded_ms=" << elapsedMs
                            << " seek_id=" << m_seekLatency.seekId
                            << " seek_accept_reason=decoded_in_tolerance"
                            << " seek_target_ms=" << m_seekLatency.desiredTargetMs
                            << " seek_index_hit=" << m_seekLatency.seekIndexHit
                            << " hinted_byte=" << m_seekLatency.hintedByte
                            << " seek_hint_exact=" << m_seekLatency.seekHintExact
                            << " seek_hint_confidence=" << m_seekLatency.seekHintConfidence
                            << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs
                            << " raw_decoded_ts=" << rawTimestampMs
                            << " decoded_ts=" << normalizedTimestampMs << " drift=" << driftMs
                            << " abs_drift=" << absDriftMs
                            << " seek_landing_drift_ms=" << driftMs
                            << " acceptTolerance=" << acceptToleranceMs;
        if (remoteFlac) {
            infoLog("SEEK_EVT") << "[FLAC_SEEK_ACCEPT]"
                                << " seek_id=" << m_seekLatency.seekId
                                << " seek_accept_reason=flac_decoded_in_tolerance"
                                << " target=" << m_seekLatency.desiredTargetMs
                                << " decoded=" << normalizedTimestampMs << " drift=" << driftMs;
        }
        if (m_seekRetryTimer) {
            m_seekRetryTimer->stop();
        }
    }

    return false;
}

void AudioSession::failRemoteFlacSeek(const char* reason, const QString& error) {
    if (!isRemoteFlacSource()) {
        emit sessionError(error);
        return;
    }

    const qint64 targetMs =
        isSeekLatencyActiveNow() ? m_seekLatency.desiredTargetMs : qMax<qint64>(0, position());
    const int currentGeneration = m_decoder ? m_decoder->currentSeekGeneration() : 0;
    m_flacSeekRecovery.failed = true;
    m_flacSeekRecovery.recoveryInProgress = false;
    m_flacSeekRecovery.targetMs = targetMs;
    m_flacSeekRecovery.seekId = m_seekLatency.seekId;
    m_flacSeekRecovery.rejectedGeneration = currentGeneration;
    logRemoteFlacSeekRecoveryState("failed", reason);

    if (isSeekLatencyActiveNow()) {
        finalizeSeekLatency(reason);
    }

    m_seek.pendingTakeoverSeekMs.reset();
    m_seek.fastStartMode = false;
    m_seek.retryCount = 0;
    m_seek.dispatchedGeneration = 0;
    m_seek.dispatchedTargetMs = -1;
    setSeekPhase(SeekPhase::Idle, reason);
    m_clock.minAcceptedDecodeGeneration = currentGeneration + 1;
    m_clock.lastPausedPlaybackMs = targetMs;
    m_clock.lastDecodedMs = targetMs;
    m_buffering.active = false;
    m_buffering.decodedWriteSinceEnter = false;
    m_buffering.decoderPausedByFlowControl = false;
    m_playback.internalPausePending = false;
    m_playback.resumeSignalPending = false;
    stopBufferingWatchdog();
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
    if (m_seekGraceTimer) {
        m_seekGraceTimer->stop();
    }

    if (m_decoder) {
        m_decoder->pauseDecode();
    }
    if (isWriteOwnerActive()) {
        if (m_player->isPlaying() && !m_player->isPaused()) {
            m_playback.internalPausePending = true;
            m_player->pause();
            m_playback.internalPausePending = false;
        }
        m_player->resetBuffer();
        m_player->setCurrentTimestamp(targetMs);
    }

    emit bufferingFinished();
    emit sessionPaused();
    emit sessionError(error);
    validateState("failRemoteFlacSeek");
}

bool AudioSession::isRemoteFlacSeekFirstPacketError(const QString& error) const {
    if (!isRemoteFlacSource() || !isSeekLatencyActiveNow()) {
        return false;
    }

    return error.contains(QStringLiteral("Seek timed out before first audio packet")) ||
           error.contains(QStringLiteral("Seek reached EOF before first audio packet")) ||
           error.contains(QStringLiteral("Seek failed before first audio packet")) ||
           error.contains(QStringLiteral("decoded timestamp drift too large"));
}

void AudioSession::clearRemoteFlacSeekRecovery(const char* reason) {
    if (!m_flacSeekRecovery.failed && !m_flacSeekRecovery.recoveryInProgress) {
        return;
    }

    logRemoteFlacSeekRecoveryState("cleared", reason);
    m_flacSeekRecovery = FlacSeekRecoveryState{};
}

void AudioSession::logRemoteFlacSeekRecoveryState(const char* state, const char* reason) const {
    infoLog("SEEK_EVT") << "flac_seek_recovery_state=" << state
                        << " reason=" << reason
                        << " seek_id=" << m_flacSeekRecovery.seekId
                        << " failed=" << m_flacSeekRecovery.failed
                        << " recovery_in_progress=" << m_flacSeekRecovery.recoveryInProgress
                        << " target_ms=" << m_flacSeekRecovery.targetMs
                        << " rejected_generation=" << m_flacSeekRecovery.rejectedGeneration
                        << " min_generation=" << m_clock.minAcceptedDecodeGeneration
                        << " seek_active=" << isSeekLatencyActiveNow()
                        << " buffering_active=" << m_buffering.active
                        << " decoded_write_since_enter="
                        << m_buffering.decodedWriteSinceEnter
                        << " player_playing=" << (m_player ? m_player->isPlaying() : false)
                        << " player_paused=" << (m_player ? m_player->isPaused() : true);
}

void AudioSession::applyDecoderFlowControlBeforeWrite(bool isRemoteFlac, int fillLevelBeforeWrite) {
    const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    const int bufferedMs = pcmBufferedMsFromBytes(bufferedBytes);
    const bool shouldPause =
        isRemoteFlac ? bufferedMs >= 9000 : fillLevelBeforeWrite > 85;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool flowHoldElapsed =
        m_buffering.lastFlowControlChangeMs <= 0 ||
        nowMs - m_buffering.lastFlowControlChangeMs >= (isRemoteFlac ? 180 : 80);
    if (shouldPause && flowHoldElapsed && !m_buffering.decoderPausedByFlowControl &&
        m_decoder->isDecoding()) {
        m_decoder->pauseDecode();
        m_buffering.decoderPausedByFlowControl = true;
        m_buffering.lastFlowControlChangeMs = nowMs;
        if (!isRemoteFlac ||
            qAbs(fillLevelBeforeWrite - m_buffering.lastFlowControlLogFill) >= 5) {
            m_buffering.lastFlowControlLogFill = fillLevelBeforeWrite;
            infoLog("BUFFER_EVT") << "Buffer full, pause decoder fill=" << fillLevelBeforeWrite
                                  << "% pcmBufferedMs=" << bufferedMs;
        }
    }
}

void AudioSession::enterBufferingWaitForDecodedWrite(const char* reason, bool emitStarted,
                                                     bool resetDecodedWrite) {
    if (resetDecodedWrite) {
        m_buffering.decodedWriteSinceEnter = false;
        m_buffering.decodedWriteGeneration = 0;
        m_buffering.decodedWriteTimestampMs = -1;
    }
    if (m_buffering.active) {
        return;
    }

    m_buffering.active = true;
    if (emitStarted) {
        emit bufferingStarted();
    }
    infoLog("BUFFER_EVT") << "buffering_enter reason=" << reason
                          << " wait_decoded_write=" << !m_buffering.decodedWriteSinceEnter;
}

void AudioSession::markBufferingDecodedWrite(qint64 normalizedTimestampMs, int bufferedBytes,
                                             int seekGeneration) {
    if (!m_buffering.active || m_buffering.decodedWriteSinceEnter) {
        return;
    }

    m_buffering.decodedWriteSinceEnter = true;
    m_buffering.decodedWriteGeneration = seekGeneration;
    m_buffering.decodedWriteTimestampMs = normalizedTimestampMs;
    if (m_flacSeekRecovery.recoveryInProgress) {
        clearRemoteFlacSeekRecovery("decoded_write_ready");
    }
    infoLog("BUFFER_EVT") << "buffering_decoded_write_ready ts=" << normalizedTimestampMs
                          << " seek_id=" << m_seek.activeSeekId
                          << " decoded_generation=" << seekGeneration
                          << " expected_generation=" << m_clock.minAcceptedDecodeGeneration
                          << " bufferedBytes=" << bufferedBytes
                          << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                          << " seek_first_packet_seen=" << isSeekFirstPacketSeenNow();
}

bool AudioSession::shouldResumeFromDecodedWrite(int fillLevel, int bufferedBytes, bool isRemoteFlac,
                                                int& requiredLevel, int& requiredBytes) const {
    if (!hasRequiredDecodedGenerationForResume()) {
        return false;
    }

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

    const bool remoteSource = m_sourceUrl.scheme().startsWith("http");
    if (remoteSource && isRemoteFlac) {
        const int requiredMs =
            remoteFlacResumeBufferMs(startupPath, m_seek.fastStartMode, isSeekFirstPacketSeenNow(),
                                     m_seekLatency.seekIndexHit || m_seekLatency.hintedByte >= 0);
        requiredLevel = 0;
        requiredBytes = pcmBytesForMs(requiredMs);
        return bufferedBytes >= requiredBytes;
    }

    if (m_seek.fastStartMode) {
        if (remoteSource) {
            requiredLevel = 4;
            requiredBytes = 48 * 1024;
        } else {
            requiredLevel = 4;
            requiredBytes = 24 * 1024;
        }
    } else if (remoteSource) {
        const int remoteMinLevel = isRemoteFlac ? 8 : 12;
        const int remoteMinBytes = isRemoteFlac ? 512 * 1024 : 128 * 1024;
        requiredLevel = qMax(requiredLevel, remoteMinLevel);
        requiredBytes = qMax(requiredBytes, remoteMinBytes);
    }

    const bool readyByBytes = bufferedBytes >= requiredBytes;
    const bool readyByLevel = fillLevel >= requiredLevel;
    if (!remoteSource) {
        return readyByBytes || readyByLevel;
    }
    if (m_seek.fastStartMode) {
        return readyByBytes || readyByLevel;
    }

    if (isRemoteFlac) {
        return readyByBytes || readyByLevel;
    }

    return readyByBytes && readyByLevel;
}

bool AudioSession::hasRequiredDecodedGenerationForResume() const {
    if (!m_buffering.active) {
        return true;
    }
    if (!m_buffering.decodedWriteSinceEnter) {
        return false;
    }
    if (m_clock.minAcceptedDecodeGeneration <= 0) {
        return true;
    }
    return m_buffering.decodedWriteGeneration >= m_clock.minAcceptedDecodeGeneration;
}

void AudioSession::startPlayerFromDecodedReady(int fillLevel) {
    // Do not anchor to latest decoded timestamp here, otherwise
    // pre-buffered data can push UI progress forward abruptly.
    qint64 startTimestamp = m_clock.baseTimestampMs;
    if (startTimestamp < 0) {
        startTimestamp = 0;
    }
    m_player->setCurrentTimestamp(startTimestamp);
    m_player->start();
    infoLog("BUFFER_EVT") << "Initial playback started (onDataDecoded) fill=" << fillLevel << "%"
                          << " seek_id=" << m_seek.activeSeekId
                          << " decoded_resume_generation=" << m_buffering.decodedWriteGeneration
                          << " decoded_resume_ts=" << m_buffering.decodedWriteTimestampMs;
}

void AudioSession::resumePlayerFromDecodedReady(int fillLevel, int bufferedBytes, int requiredLevel,
                                                 int requiredBytes) {
    m_player->resume();
    infoLog("BUFFER_EVT") << "Buffer filled, resuming playback bytes=" << bufferedBytes << "/"
                          << requiredBytes << " pcmBufferedMs="
                          << pcmBufferedMsFromBytes(bufferedBytes) << " requiredPcmMs="
                          << pcmBufferedMsFromBytes(requiredBytes) << " fill=" << fillLevel << "/"
                          << requiredLevel << " seek_id=" << m_seek.activeSeekId
                          << " decoded_resume_generation=" << m_buffering.decodedWriteGeneration
                          << " decoded_resume_ts=" << m_buffering.decodedWriteTimestampMs;
}

void AudioSession::completeDecodedReadyResume() {
    if (isSeekLatencyActiveNow()) {
        const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
        infoLog("SEEK_EVT") << "buffer_resume_ms=" << elapsedMs
                            << " seek_id=" << m_seekLatency.seekId
                            << " seek_target_ms=" << m_seekLatency.desiredTargetMs
                            << " seek_index_hit=" << m_seekLatency.seekIndexHit
                            << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                            << " decoded_resume_generation="
                            << m_buffering.decodedWriteGeneration;
        finalizeSeekLatency("decoded_write_buffer_resume");
    }
    m_lastResumeTimer.restart();
    m_buffering.hasRecentResume = true;
    m_buffering.decodedWriteSinceEnter = false;
    m_seek.fastStartMode = false;
    stopBufferingWatchdog();
    emit bufferingFinished();
}

void AudioSession::kickBufferingRecovery(const char* reason, int fillLevel, int bufferedBytes) {
    if (!m_active || m_playback.manualPaused || !isWriteOwnerActive() || !m_decoder) {
        return;
    }

    if (m_buffering.decoderPausedByFlowControl) {
        m_buffering.decoderPausedByFlowControl = false;
    }

    requestDecoderStart();
    if (m_buffering.active && m_bufferingWatchdogTimer &&
        !m_bufferingWatchdogTimer->isActive()) {
        m_bufferingWatchdogTimer->start();
    }

    if (qstrcmp(reason, "buffering_watchdog") != 0 || isSeekLatencyActiveNow()) {
        infoLog("BUFFER_EVT") << "buffering_recovery_kick reason=" << reason
                              << " fill=" << fillLevel << "% bufferedBytes=" << bufferedBytes
                              << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                              << " isDecoding=" << m_decoder->isDecoding();
    }
}

void AudioSession::stopBufferingWatchdog() {
    if (m_bufferingWatchdogTimer && m_bufferingWatchdogTimer->isActive()) {
        m_bufferingWatchdogTimer->stop();
    }
}

bool AudioSession::shouldDebounceBufferingEnter() const {
    return m_buffering.hasRecentResume && m_lastResumeTimer.isValid() &&
           m_lastResumeTimer.elapsed() < (m_sourceUrl.scheme().startsWith("http") ? 1200 : 220);
}

bool AudioSession::canExitBufferingAfterDecodedWrite(const char* reason, int fillLevel,
                                                     int bufferedBytes) {
    if (!m_buffering.active) {
        return true;
    }
    if (hasRequiredDecodedGenerationForResume()) {
        return true;
    }

    kickBufferingRecovery(reason, fillLevel, bufferedBytes);
    if (qstrcmp(reason, "buffering_watchdog") != 0 || isSeekLatencyActiveNow()) {
        infoLog("BUFFER_EVT") << "buffering_exit_wait_decoded_write reason=" << reason
                              << " seek_id=" << m_seek.activeSeekId
                              << " fill=" << fillLevel << "% bufferedBytes=" << bufferedBytes
                              << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                              << " decoded_write_generation="
                              << m_buffering.decodedWriteGeneration
                              << " expected_generation=" << m_clock.minAcceptedDecodeGeneration
                              << " seek_active=" << isSeekLatencyActiveNow()
                              << " first_decoded_seen=" << isSeekFirstPacketSeenNow();
    }
    return false;
}

bool AudioSession::handleTailLowBufferFallback(qint64 currentPos, int fillLevel,
                                               int bufferedBytes) {
    constexpr qint64 kTailFinishRemainingMs = 180;
    constexpr qint64 kTailFallbackWindowMs = 8000;
    constexpr qint64 kTailDecodeLeadMs = 800;
    constexpr int kTailFinishBufferedBytes = 8 * 1024;
    constexpr int kTailFinishFillLevel = 1;

    qint64 tailRef = m_duration;
    if (m_clock.lastDecodedMs > tailRef) {
        tailRef = m_clock.lastDecodedMs;
    }
    const qint64 remainingToTail = (tailRef > 0) ? (tailRef - currentPos) : -1;
    const bool tailDrained =
        fillLevel <= kTailFinishFillLevel && bufferedBytes <= kTailFinishBufferedBytes;

    auto finishTailPlayback = [&](const char* reason, qint64 remainingMs) {
        qint64 finalDuration = m_duration;
        if (m_clock.lastDecodedMs > finalDuration) {
            finalDuration = m_clock.lastDecodedMs;
        }
        if (currentPos > finalDuration) {
            finalDuration = currentPos;
        }
        if (!m_hasStableDuration && finalDuration > 0 && finalDuration != m_duration) {
            m_duration = finalDuration;
            emit durationChanged(m_duration);
        }
        infoLog("BUFFER_EVT") << "Tail finish reason=" << reason << " position=" << currentPos
                              << " duration=" << m_duration
                              << " lastDecoded=" << m_clock.lastDecodedMs
                              << " remaining=" << remainingMs << " fill=" << fillLevel
                              << "% bufferedBytes=" << bufferedBytes;
        onPlaybackFinished();
        return true;
    };

    if (!isDecodeCompletedNow() && m_duration > 0 &&
        currentPos >= m_duration - kTailFallbackWindowMs && m_clock.lastDecodedMs > 0 &&
        currentPos >= m_clock.lastDecodedMs + kTailDecodeLeadMs && tailDrained) {
        warnLog("BUFFER_EVT") << "Tail fallback finish without decodeCompleted."
                              << " position=" << currentPos << " duration=" << m_duration
                              << " lastDecoded=" << m_clock.lastDecodedMs << " fill=" << fillLevel
                              << "% bufferedBytes=" << bufferedBytes;
        onPlaybackFinished();
        return true;
    }

    if (isDecodeCompletedNow()) {
        if (tailDrained && (remainingToTail < 0 || remainingToTail <= kTailFinishRemainingMs)) {
            return finishTailPlayback("decode_completed_drained", remainingToTail);
        }

        infoLog("BUFFER_EVT") << "Decode complete, keep draining tail"
                              << " fill=" << fillLevel << "% bufferedBytes=" << bufferedBytes
                              << " remaining=" << remainingToTail << "ms";
        return true;
    }

    return false;
}

bool AudioSession::canExitBufferingByThreshold(int fillLevel, int bufferedBytes, bool isRemoteFlac,
                                               int bufferingResumeMinBytes) const {
    int resumeLevelThreshold = m_seek.fastStartMode ? 6 : 25;
    if (isRemoteFlac) {
        const int flacLevel = m_seek.fastStartMode ? 6 : 10;
        resumeLevelThreshold = qMax(resumeLevelThreshold, flacLevel);
    }

    if (m_seek.fastStartMode && !isRemoteFlac) {
        return fillLevel >= resumeLevelThreshold || bufferedBytes >= qMin(bufferingResumeMinBytes, 48 * 1024);
    }
    if (isRemoteFlac) {
        return bufferedBytes >= bufferingResumeMinBytes;
    }
    return fillLevel >= resumeLevelThreshold || bufferedBytes >= bufferingResumeMinBytes;
}

void AudioSession::exitBufferingAndResume(int fillLevel, int bufferedBytes) {
    if (isSeekLatencyActiveNow()) {
        const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
        infoLog("SEEK_EVT") << "buffer_resume_ms=" << elapsedMs
                            << " seek_id=" << m_seekLatency.seekId
                            << " seek_target_ms=" << m_seekLatency.desiredTargetMs
                            << " seek_index_hit=" << m_seekLatency.seekIndexHit
                            << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                            << " decoded_resume_generation="
                            << m_buffering.decodedWriteGeneration;
        finalizeSeekLatency("decoded_write_threshold_resume");
    }
    m_buffering.active = false;
    stopBufferingWatchdog();
    if (!m_player->isPlaying() && m_active) {
        qint64 startTimestamp = m_clock.baseTimestampMs;
        if (startTimestamp < 0) {
            startTimestamp = 0;
        }
        m_player->setCurrentTimestamp(startTimestamp);
        m_player->start();
        infoLog("BUFFER_EVT") << "Initial playback started fill=" << fillLevel << "%"
                              << " seek_id=" << m_seek.activeSeekId
                              << " decoded_resume_generation="
                              << m_buffering.decodedWriteGeneration
                              << " bufferedBytes=" << bufferedBytes
                              << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes);
    } else {
        m_player->resume();
    }
    m_lastResumeTimer.restart();
    m_buffering.hasRecentResume = true;
    m_buffering.decodedWriteSinceEnter = false;

    m_decoder->startDecode();
    emit bufferingFinished();
    m_seek.fastStartMode = false;
    infoLog("BUFFER_EVT") << "Buffering finished fill=" << fillLevel
                          << "% bufferedBytes=" << bufferedBytes
                          << " pcmBufferedMs=" << pcmBufferedMsFromBytes(bufferedBytes)
                          << " seek_id=" << m_seek.activeSeekId
                          << " decoded_resume_generation="
                          << m_buffering.decodedWriteGeneration;
}

// ===== Seek 重试与退化策略 =====
bool AudioSession::isHlsSeekSource() const {
    const QString sourcePath = m_sourceUrl.path().toLower();
    return sourcePath.endsWith(".m3u8") || m_sourceUrl.toString().contains("/hls/");
}

qint64 AudioSession::calculateSeekHardTimeoutMs(bool isHlsSeek, bool isRemoteFlacSeek) const {
    if (m_seekLatency.seekIndexHit) {
        return 4500;
    }
    if (isHlsSeek) {
        return qMin<qint64>(m_seekLatency.hardTimeoutMs, 8000);
    }
    if (isRemoteFlacSeek) {
        return 4500;
    }
    return qMin<qint64>(m_seekLatency.hardTimeoutMs, 9000);
}

qint64 AudioSession::calculateSeekGiveUpTimeoutMs(bool isHlsSeek, bool isRemoteFlacSeek,
                                                  qint64 hardTimeoutMs) const {
    if (m_seekLatency.seekIndexHit) {
        return qMax<qint64>(hardTimeoutMs, 6000);
    }
    if (isHlsSeek) {
        return qMax<qint64>(hardTimeoutMs, 10000);
    }
    if (isRemoteFlacSeek) {
        return qMax<qint64>(hardTimeoutMs, 6500);
    }
    return qMax<qint64>(hardTimeoutMs, 9000);
}

AudioSession::SeekRetryAction AudioSession::decideSeekRetryAction(qint64 elapsedMs,
                                                                  qint64 hardTimeoutMs,
                                                                  qint64 giveUpTimeoutMs) const {
    if (elapsedMs >= giveUpTimeoutMs) {
        return SeekRetryAction::GiveUp;
    }
    if (isRemoteFlacSource() && elapsedMs >= hardTimeoutMs) {
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

void AudioSession::handleSeekRetryGiveUp(qint64 elapsedMs) {
    warnLog("SEEK_EVT") << "[SEEK_ABORT] elapsed=" << elapsedMs
                        << " seek_id=" << m_seekLatency.seekId
                        << " seek_index_hit=" << m_seekLatency.seekIndexHit
                        << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;
    finalizeSeekLatency("seek_timeout_no_packet");
    m_buffering.active = false;
    m_buffering.decodedWriteSinceEnter = false;
    stopBufferingWatchdog();
    emit bufferingFinished();
    emit sessionError(QStringLiteral("音频缓冲超时，请重试"));
}

void AudioSession::handleSeekRetryDegrade(qint64 elapsedMs, int nextRetryMs, const char* logTag) {
    warnLog("SEEK_EVT") << logTag << " elapsed=" << elapsedMs
                        << " seek_id=" << m_seekLatency.seekId
                        << " seek_index_hit=" << m_seekLatency.seekIndexHit
                        << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;
    enterBufferingWaitForDecodedWrite("seek_retry_degrade", !m_buffering.active, false);
    emit bufferingProgress(qMax(1, m_buffering.percent));
    const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    kickBufferingRecovery("seek_retry_degrade", m_player ? m_player->bufferFillLevel() : 0,
                          bufferedBytes);
    if (m_seekRetryTimer) {
        m_seekRetryTimer->start(nextRetryMs);
    }
}

void AudioSession::handleSeekRetryAttempt(qint64 elapsedMs, bool avoidRepeatReseek) {
    ++m_seek.retryCount;
    warnLog("SEEK_EVT") << "[SEEK_RETRY] retry=" << m_seek.retryCount << "/" << m_seek.maxRetries
                        << " elapsed=" << elapsedMs
                        << " seek_id=" << m_seekLatency.seekId
                        << " seek_index_hit=" << m_seekLatency.seekIndexHit
                        << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs;

    const bool sameActiveTarget =
        m_seek.dispatchedGeneration > 0 &&
        qAbs(m_seek.dispatchedTargetMs - m_seekLatency.workingTargetMs) <= 500;
    if (sameActiveTarget) {
        warnLog("SEEK_EVT") << "[SEEK_WAIT] seek_singleflight_suppressed"
                            << " elapsed=" << elapsedMs
                            << " seek_id=" << m_seekLatency.seekId
                            << " dispatched_generation=" << m_seek.dispatchedGeneration
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs;
    } else if (!avoidRepeatReseek) {
        dispatchDecoderSeekForLocalSeek(m_seekLatency.workingTargetMs,
                                        AudioDecoder::SeekMode::Fast);
    } else {
        warnLog("SEEK_EVT") << "[SEEK_WAIT] skip repeat reseek"
                            << " elapsed=" << elapsedMs
                            << " seek_id=" << m_seekLatency.seekId
                            << " desired=" << m_seekLatency.desiredTargetMs
                            << " working=" << m_seekLatency.workingTargetMs;
    }

    requestDecoderStart();
    enterBufferingWaitForDecodedWrite("seek_retry_attempt", !m_buffering.active, false);
    const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    kickBufferingRecovery("seek_retry_attempt", m_player ? m_player->bufferFillLevel() : 0,
                          bufferedBytes);
    if (m_seekRetryTimer) {
        const int nextMs = 650 + m_seek.retryCount * 350;
        m_seekRetryTimer->start(nextMs);
    }
}

// ===== 状态迁移与断言校验 =====
void AudioSession::setSeekPhase(SeekPhase phase, const char* reason) {
    if (m_seek.phase == phase) {
        return;
    }
    infoLog("STATE_EVT") << "SeekPhase " << static_cast<int>(m_seek.phase) << " -> "
                         << static_cast<int>(phase) << " reason=" << reason;
    m_seek.phase = phase;
}

void AudioSession::setSeekLatencyPhase(SeekLatencyPhase phase, const char* reason) {
    if (m_seekLatency.phase == phase) {
        return;
    }
    infoLog("STATE_EVT") << "SeekLatencyPhase " << static_cast<int>(m_seekLatency.phase) << " -> "
                         << static_cast<int>(phase) << " reason=" << reason;
    m_seekLatency.phase = phase;
}

void AudioSession::setDecodePhase(DecodePhase phase, const char* reason) {
    if (m_decode.phase == phase) {
        return;
    }
    infoLog("STATE_EVT") << "DecodePhase " << static_cast<int>(m_decode.phase) << " -> "
                         << static_cast<int>(phase) << " reason=" << reason;
    m_decode.phase = phase;
}

void AudioSession::validateState(const char* where) const {
#ifndef QT_NO_DEBUG
    const char* checkPoint = where ? where : "unknown";
    Q_ASSERT_X(!(m_playback.manualPaused && m_buffering.active), checkPoint,
               "manualPaused and buffering.active cannot both be true");
    Q_ASSERT_X(
        !(m_seek.phase == SeekPhase::Idle && m_seekGraceTimer && m_seekGraceTimer->isActive()),
        checkPoint, "seek grace timer active while phase is idle");
    Q_ASSERT_X(!(m_seekLatency.phase == SeekLatencyPhase::Inactive && m_seekRetryTimer &&
                 m_seekRetryTimer->isActive()),
               checkPoint, "seek retry timer active while seek latency is inactive");
    if (m_seekLatency.phase == SeekLatencyPhase::FirstPacketSeen) {
        Q_ASSERT_X(m_seekLatency.firstPacketTimestampMs >= 0, checkPoint,
                   "firstPacketTimestampMs must be initialized in FirstPacketSeen phase");
    }
#else
    Q_UNUSED(where);
#endif
}

void AudioSession::cleanup() {
    stop();
}

void AudioSession::requestDecoderStart() {
    if (!m_decoder) {
        return;
    }
    m_decoder->startDecode();
}

void AudioSession::finalizeControlPath(const char* where) {
    validateState(where);
}

QDebug AudioSession::infoLog(const char* domain) const {
    QDebug dbg = qDebug().noquote().nospace();
    dbg << '[' << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << ']' << '[' << domain
        << ']' << "[session=" << m_sessionId << "] ";
    return dbg;
}

QDebug AudioSession::warnLog(const char* domain) const {
    QDebug dbg = qWarning().noquote().nospace();
    dbg << '[' << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << ']' << '[' << domain
        << ']' << "[session=" << m_sessionId << "] ";
    return dbg;
}

void AudioSession::finalizeSeekLatency(const char* stage) {
    if (!isSeekLatencyActiveNow()) {
        return;
    }

    const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
    const QString metricName =
        QString("[SEEK_METRIC] seek_to_%1_ms:").arg(QString::fromUtf8(stage));
    infoLog("SEEK_EVT") << metricName << elapsedMs
                        << " seek_id=" << m_seekLatency.seekId
                        << " seek_accept_reason=" << stage
                        << " seek_target_ms=" << m_seekLatency.desiredTargetMs
                        << " seek_index_hit=" << m_seekLatency.seekIndexHit
                        << " corrective_reseek_count=" << m_seekLatency.accuracyRetryCount
                        << " desired=" << m_seekLatency.desiredTargetMs
                        << " working=" << m_seekLatency.workingTargetMs
                        << " first_decoded_seen=" << isSeekFirstPacketSeenNow()
                        << " first_decoded_ts=" << m_seekLatency.firstPacketTimestampMs
                        << " raw_first_decoded_ts=" << m_seekLatency.rawFirstPacketTimestampMs
                        << " decoded_resume_generation="
                        << m_buffering.decodedWriteGeneration;
    setSeekLatencyPhase(SeekLatencyPhase::Inactive, "finalizeSeekLatency");
    m_seekLatency.desiredTargetMs = 0;
    m_seekLatency.seekId = 0;
    m_seekLatency.seekIndexHit = false;
    m_seek.fastStartMode = false;
    m_seek.retryCount = 0;
    m_seekLatency.accuracyRetryCount = 0;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
}
