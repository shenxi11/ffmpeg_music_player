#include "AudioSession.h"
#include <chrono>
#include <QFileInfo>
#include <QCoreApplication>
#include <QEventLoop>
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

} // namespace

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
      m_pendingTakeoverSeek(false),
      m_pendingTakeoverSeekMs(0),
      m_internalPausePending(false),
      m_decodeCompleted(false),
      m_seekGracePeriod(false),
      m_seekGraceTimer(new QTimer(this)),
      m_seekRetryTimer(new QTimer(this)),
      m_isSeeking(false),
      m_seekFastStartMode(false),
      m_seekRetryCount(0),
      m_seekMaxRetries(4),
      m_minAcceptedDecodeGeneration(0),
      m_hasRecentResume(false),
      m_tailWaitLogActive(false),
      m_seekLatencyActive(false),
      m_seekFirstPacketSeen(false),
      m_seekDesiredTargetMs(0),
      m_seekLatencyTargetMs(0),
      m_seekFirstPacketTimestampMs(-1),
      m_seekAccuracyRetryCount(0),
      m_seekAccuracyMaxRetries(5),
      m_seekAccuracyToleranceMs(2500),
      m_seekHardTimeoutMs(20000)
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

    m_seekRetryTimer->setSingleShot(true);
    connect(m_seekRetryTimer, &QTimer::timeout, this, &AudioSession::onSeekRetryTimeout);
    
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
        // 网络初始化期间持续处理事件，避免本地代理请求被主线程阻塞。
        result = initDecoderWithEventPumping(m_decoder, url.toString());
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
        m_pendingTakeoverSeek = false;
        m_pendingTakeoverSeekMs = 0;
        m_internalPausePending = false;
        m_decodeCompleted = false;
        m_seekLatencyActive = false;
        m_seekFirstPacketSeen = false;
        m_seekDesiredTargetMs = 0;
        m_seekLatencyTargetMs = 0;
        m_seekFirstPacketTimestampMs = -1;
        m_seekAccuracyRetryCount = 0;
        m_seekFastStartMode = false;
        m_seekRetryCount = 0;
        m_minAcceptedDecodeGeneration = 0;
        m_hasRecentResume = false;
        m_tailWaitLogActive = false;
        if (m_seekRetryTimer) {
            m_seekRetryTimer->stop();
        }
        emit sessionStarted();
    }
    m_manualPaused = false;

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = !currentOwner.isEmpty() && currentOwner != m_sessionId;
    if (takingOverFromOtherOwner && m_player->isPlaying() && !m_player->isPaused()) {
        m_internalPausePending = true;
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
    m_internalPausePending = false;
    m_decodeCompleted = false;

    const QString currentOwner = m_player->writeOwner();
    // 当前 owner 不是本会话（包括 owner 为空）时，都视为需要重建时间轴。
    // 典型场景：视频关闭后清空了 owner，但共享 AudioPlayer 仍保留了视频时钟。
    const bool takingOverFromOtherOwner = currentOwner != m_sessionId;
    if (takingOverFromOtherOwner && m_player->isPlaying() && !m_player->isPaused()) {
        m_internalPausePending = true;
        m_player->pause();
    }

    m_player->setWriteOwner(m_sessionId);

    if (takingOverFromOtherOwner) {
        // Let decoder refill from this session's timeline after takeover.
        qint64 resumeTimestamp = m_pendingTakeoverSeek
                ? m_pendingTakeoverSeekMs
                : (m_lastPausedPlaybackMs > 0 ? m_lastPausedPlaybackMs : m_lastDecodedTimestampMs);
        if (resumeTimestamp < 0) {
            resumeTimestamp = 0;
        }

        qDebug() << "AudioSession: Resume takeover from owner" << currentOwner
                 << "resumeTimestamp:" << resumeTimestamp
                 << "lastPaused:" << m_lastPausedPlaybackMs
                 << "lastDecoded:" << m_lastDecodedTimestampMs;

        m_pendingTakeoverSeek = false;
        m_pendingTakeoverSeekMs = 0;
        m_lastDecodedTimestampMs = resumeTimestamp;
        m_decoderTimestampBaseMs = resumeTimestamp;
        m_decoderTimestampOffsetMs = 0;
        m_decoderTimestampNeedsProbe = true;
        m_decoderTimestampHasOffset = false;
        m_isSeeking = true;
        m_seekGracePeriod = true;
        if (m_seekGraceTimer) {
            m_seekGraceTimer->start(800);
        }
        m_player->stop();
        m_player->resetBuffer();
        m_player->setCurrentTimestamp(resumeTimestamp);
        m_decoder->seekTo(resumeTimestamp);
        m_minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
        m_seekFastStartMode = true;
        m_seekRetryCount = 0;
        if (m_seekRetryTimer) {
            m_seekRetryTimer->start(1200);
        }
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
    m_pendingTakeoverSeek = false;
    m_pendingTakeoverSeekMs = 0;
    m_internalPausePending = false;
    m_decodeCompleted = false;
    m_seekLatencyActive = false;
    m_seekFirstPacketSeen = false;
    m_seekDesiredTargetMs = 0;
    m_seekLatencyTargetMs = 0;
    m_seekFirstPacketTimestampMs = -1;
    m_seekAccuracyRetryCount = 0;
    m_seekFastStartMode = false;
    m_seekRetryCount = 0;
    m_minAcceptedDecodeGeneration = 0;
    m_hasRecentResume = false;
    m_tailWaitLogActive = false;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
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
    m_seekLatencyActive = true;
    m_seekFirstPacketSeen = false;
    m_seekDesiredTargetMs = positionMs;
    m_seekLatencyTargetMs = positionMs;
    m_seekFirstPacketTimestampMs = -1;
    m_seekAccuracyRetryCount = 0;
    m_seekLatencyTimer.restart();
    m_seekFastStartMode = true;
    m_seekRetryCount = 0;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->start(1200);
    }
    qDebug() << "[SEEK_METRIC] seek_start target:" << positionMs
             << "session:" << m_sessionId
             << "source:" << m_sourceUrl.toString();
    m_lastDecodedTimestampMs = positionMs;
    m_lastPausedPlaybackMs = positionMs;
    m_decoderTimestampBaseMs = positionMs;
    m_decoderTimestampOffsetMs = 0;
    m_decoderTimestampNeedsProbe = true;
    m_decoderTimestampHasOffset = false;
    m_decodeCompleted = false;

    const QString currentOwner = m_player->writeOwner();
    const bool takingOverFromOtherOwner = !currentOwner.isEmpty() && currentOwner != m_sessionId;
    if (takingOverFromOtherOwner) {
        // 在 owner handoff 场景中，seek 统一延迟到 resume() 执行，
        // 避免同一次用户操作触发两次 seek（seekTo + resume 内部 seek）。
        m_pendingTakeoverSeek = true;
        m_pendingTakeoverSeekMs = positionMs;
        m_minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration() + 1;
        qDebug() << "AudioSession: Deferring seek until owner handoff resume at" << positionMs << "ms";
        return;
    }
    
    // 进入 seek 状态，抑制部分瞬时回调
    m_isSeeking = true;
    
    // 记录 seek 前是否在播放，seek 完成后按原状态恢复
    bool wasPlaying = m_player->isPlaying() && !m_player->isPaused();
    
    // 播放中 seek：先暂停解码与播放，避免旧时间轴数据继续输出
    if (wasPlaying && m_decoder) {
        m_decoder->pauseDecode();
        m_internalPausePending = true;
        m_player->pause();
    }
    
    // 清空共享缓冲与时间戳队列，防止 seek 前旧帧和旧时间轴混入
    m_player->resetBuffer();
    
    // 先对齐播放器时钟到目标位置
    // 后续写入的解码数据将基于该时间轴继续
    m_player->setCurrentTimestamp(positionMs);
    
    // 播放中 seek 后应进入 buffering，等待新位置数据回填后再恢复输出
    m_isBuffering = wasPlaying;
    if (m_isBuffering) {
        emit bufferingStarted();
    }
    
    // 启动 seek 宽限窗口，过滤 seek 后短暂抖动
    m_seekGracePeriod = true;
    if (m_seekGraceTimer) {
        m_seekGraceTimer->start(800);
    }
    
    // 通知解码器跳转到目标时间
    if (m_decoder) {
        m_decoder->seekTo(positionMs);
        m_minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
    }
    
    // seek 后立即恢复解码线程
    if (m_decoder) {
        m_decoder->startDecode();
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

void AudioSession::onDecodedData(const QByteArray& data, qint64 timestampMs, int seekGeneration)
{
    if (!m_active || m_manualPaused || !isWriteOwnerActive()) {
        return;
    }

    if (seekGeneration < m_minAcceptedDecodeGeneration) {
        static int staleDropCount = 0;
        if (++staleDropCount % 50 == 1) {
            qDebug() << "AudioSession: Dropping stale decoded frame generation:"
                     << seekGeneration << "expected>=" << m_minAcceptedDecodeGeneration;
        }
        return;
    }

    qint64 normalizedTimestampMs = timestampMs;
    if (m_decoderTimestampNeedsProbe) {
        // seek/owner handoff 后优先信任解码时间轴，避免把 UI 强行钉到 seek 目标导致歌词错位。
        const qint64 delta = m_decoderTimestampBaseMs - timestampMs;
        if (qAbs(delta) > 3000) {
            qWarning() << "AudioSession: Seek target differs from decoded timeline, trust decoded timestamp."
                       << "expected:" << m_decoderTimestampBaseMs
                       << "decoded:" << timestampMs
                       << "delta:" << delta;
            // 更新基准给后续 start/resume 逻辑使用，避免继续沿用错误目标值。
            m_decoderTimestampBaseMs = timestampMs;
        } else {
            m_decoderTimestampBaseMs = timestampMs;
        }
        m_decoderTimestampHasOffset = false;
        m_decoderTimestampOffsetMs = 0;
        m_decoderTimestampNeedsProbe = false;
    }

    if (m_decoderTimestampHasOffset) {
        normalizedTimestampMs = timestampMs + m_decoderTimestampOffsetMs;
    }

    m_lastDecodedTimestampMs = normalizedTimestampMs;

    // 网络音频（尤其 VBR/流媒体）元数据时长可能偏小，导致进度条先跑完。
    // 仅在“解码时间轴已明显超出元时长”时扩容，避免尾段误扩容导致显示时长偏大。
    if (m_duration > 0 && !m_decodeCompleted && normalizedTimestampMs > m_duration + 500) {
        constexpr qint64 kDurationHeadroomMs = 2500;
        const qint64 desiredDuration = normalizedTimestampMs + kDurationHeadroomMs;
        if (desiredDuration > m_duration) {
            m_duration = desiredDuration;
            emit durationChanged(m_duration);
            qDebug() << "AudioSession: Extend dynamic duration to" << m_duration
                     << "ms based on decoded timestamp" << normalizedTimestampMs;
        }
    }

    if (m_seekLatencyActive && !m_seekFirstPacketSeen) {
        const qint64 driftMs = normalizedTimestampMs - m_seekDesiredTargetMs;
        const qint64 absDriftMs = qAbs(driftMs);
        qint64 acceptToleranceMs = m_seekAccuracyToleranceMs;
        if (m_seekAccuracyRetryCount >= 2) {
            acceptToleranceMs = qMax<qint64>(acceptToleranceMs, 8000);
        }
        if (m_seekAccuracyRetryCount >= 4) {
            acceptToleranceMs = qMax<qint64>(acceptToleranceMs, 15000);
        }

        if (absDriftMs > acceptToleranceMs) {
            if (m_seekAccuracyRetryCount < m_seekAccuracyMaxRetries) {
                ++m_seekAccuracyRetryCount;
                const int retryRound = m_seekAccuracyRetryCount;

                // 围绕“用户目标”小步逼近，避免一次跳到末尾导致长时间重试循环。
                qint64 correctedTargetMs = m_seekDesiredTargetMs;
                if (driftMs < 0) {
                    // 首包落点早于目标：向后推进 seek 目标，但限制单次推进幅度。
                    qint64 forwardCapMs = 20000 + static_cast<qint64>(retryRound - 1) * 8000;
                    forwardCapMs = qMin<qint64>(forwardCapMs, 60000);
                    const qint64 forwardStepMs = qBound<qint64>(4000, ((-driftMs) * 2) / 3, forwardCapMs);
                    correctedTargetMs = m_seekDesiredTargetMs + forwardStepMs;
                } else {
                    // 首包落点晚于目标：向前回拉 seek 目标，同样限制单次回拉幅度。
                    qint64 backwardCapMs = 12000 + static_cast<qint64>(retryRound - 1) * 4000;
                    backwardCapMs = qMin<qint64>(backwardCapMs, 30000);
                    const qint64 backwardStepMs = qBound<qint64>(3000, (driftMs * 2) / 3, backwardCapMs);
                    correctedTargetMs = m_seekDesiredTargetMs - backwardStepMs;
                }

                if (correctedTargetMs < 0) {
                    correctedTargetMs = 0;
                }
                if (m_duration > 0 && correctedTargetMs > m_duration - 500) {
                    correctedTargetMs = qMax<qint64>(0, m_duration - 500);
                }

                m_seekLatencyTargetMs = correctedTargetMs;

                qWarning() << "[SEEK_CORRECT] first packet drift too large, corrective seek"
                           << m_seekAccuracyRetryCount << "/" << m_seekAccuracyMaxRetries
                           << "desired:" << m_seekDesiredTargetMs
                           << "working:" << m_seekLatencyTargetMs
                           << "decoded:" << normalizedTimestampMs
                           << "drift:" << driftMs
                           << "acceptTolerance:" << acceptToleranceMs
                           << "session:" << m_sessionId;

                // 丢弃本次偏差首包，发起校正 seek，等待新落点。
                m_seekFirstPacketSeen = false;
                m_seekFirstPacketTimestampMs = -1;
                m_decoder->seekTo(m_seekLatencyTargetMs);
                m_minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
                m_decoder->startDecode();
                if (m_seekRetryTimer) {
                    m_seekRetryTimer->start(900);
                }
                return;
            }

            // 校正次数耗尽后转为“就近接受”策略，避免反复 reseek 导致长时间无声。
            qWarning() << "[SEEK_ACCEPT] drift remains after correction retries, accept nearest keyframe."
                       << "desired:" << m_seekDesiredTargetMs
                       << "working:" << m_seekLatencyTargetMs
                       << "decoded:" << normalizedTimestampMs
                       << "drift:" << driftMs
                       << "acceptTolerance:" << acceptToleranceMs
                       << "session:" << m_sessionId;
            m_seekFirstPacketSeen = true;
            m_seekFirstPacketTimestampMs = normalizedTimestampMs;
            m_seekLatencyTargetMs = normalizedTimestampMs;
            if (m_seekRetryTimer) {
                m_seekRetryTimer->stop();
            }
        }

        if (!m_seekFirstPacketSeen) {
            m_seekFirstPacketSeen = true;
            m_seekFirstPacketTimestampMs = normalizedTimestampMs;
            const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
            qDebug() << "[SEEK_METRIC] seek_to_first_decoded_ms:" << elapsedMs
                     << "desired:" << m_seekDesiredTargetMs
                     << "working:" << m_seekLatencyTargetMs
                     << "decoded_ts:" << normalizedTimestampMs
                     << "session:" << m_sessionId;
            if (m_seekRetryTimer) {
                m_seekRetryTimer->stop();
            }
        }
    }

    const bool isRemoteFlac = m_sourceUrl.scheme().startsWith("http")
            && m_sourceUrl.path().toLower().endsWith(".flac");

    // 缓冲过高时暂停解码，避免无意义堆积
    const int flowControlPauseThreshold = isRemoteFlac ? 96 : 85;
    int fillLevelBeforeWrite = m_player->bufferFillLevel();
    if (fillLevelBeforeWrite > flowControlPauseThreshold
            && !m_decoderPausedByFlowControl
            && m_decoder->isDecoding()) {
        m_decoder->pauseDecode();
        m_decoderPausedByFlowControl = true;
        qDebug() << "AudioSession: Buffer full (" << fillLevelBeforeWrite << "%), pausing decoder";
    }
    
    // 写入解码数据，携带会话 ID 以保证写入归属
    m_player->writeAudioData(data, normalizedTimestampMs, m_sessionId);
    
    // 基于写入后的最新水位与字节量判断是否可恢复
    const int fillLevel = m_player->bufferFillLevel();
    const AudioBuffer* buffer = m_player->getBuffer();
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    const bool startupPath = (!m_player->isPlaying() && m_active);
    const bool resumePath = (m_player->isPlaying() && m_player->isPaused());

    // 字节阈值优先（低延迟），百分比阈值兜底（防止低速网络抖动）
    int requiredLevel = 85;
    int requiredBytes = 512 * 1024;
    if (startupPath) {
        requiredLevel = 40;
        requiredBytes = 256 * 1024;
    } else if (resumePath) {
        requiredLevel = 25;
        requiredBytes = 192 * 1024;
    }
    if (m_seekFastStartMode) {
        if (isRemoteFlac) {
            // 远端 FLAC seek 后使用中等恢复门槛，兼顾稳定性与恢复时延。
            requiredLevel = 20;
            requiredBytes = 768 * 1024;
        } else {
            requiredLevel = 4;
            requiredBytes = 24 * 1024;
        }
    } else if (isRemoteFlac) {
        // 远端 FLAC 对吞吐抖动更敏感，恢复门槛提高，减少“恢复后立刻再卡”。
        if (startupPath) {
            requiredLevel = qMax(requiredLevel, 26);
            requiredBytes = qMax(requiredBytes, 1024 * 1024);
        } else if (resumePath) {
            requiredLevel = qMax(requiredLevel, 24);
            requiredBytes = qMax(requiredBytes, 896 * 1024);
        }
    }

    const bool readyByBytes = bufferedBytes >= requiredBytes;
    const bool readyByLevel = fillLevel >= requiredLevel;
    
    bool readyToResume = readyByBytes || readyByLevel;
    if (isRemoteFlac) {
        // 远端 FLAC 使用“字节 + 百分比”双条件，避免过早恢复触发周期性卡顿。
        readyToResume = readyByBytes && readyByLevel;
    }

    if (m_isBuffering && readyToResume) {
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
            qDebug() << "AudioSession: Buffer filled, resuming playback."
                     << "bytes:" << bufferedBytes << "/" << requiredBytes
                     << "fill:" << fillLevel << "/" << requiredLevel;
        }
        m_lastResumeTimer.restart();
        m_hasRecentResume = true;
        m_seekFastStartMode = false;
        
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
    if (m_seekLatencyActive) {
        qDebug() << "[SEEK_METRIC] seek_failed_on_decode_error desired:" << m_seekDesiredTargetMs
                 << "working:" << m_seekLatencyTargetMs
                 << "error:" << error
                 << "elapsed:" << (m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1) << "ms";
        m_seekLatencyActive = false;
    }
    m_seekFastStartMode = false;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
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
    
    // 仅在解码已完成后，再允许按时长阈值收敛到 finished。
    // 避免部分 VBR/容器时长偏差导致“还剩几秒就提前结束”。
    if (m_duration > 0 && positionMs >= m_duration - 100) {
        if (!m_decodeCompleted && positionMs > m_duration + 500) {
            constexpr qint64 kDurationHeadroomMs = 2500;
            const qint64 desiredDuration = positionMs + kDurationHeadroomMs;
            if (desiredDuration > m_duration) {
                m_duration = desiredDuration;
                emit durationChanged(m_duration);
                qDebug() << "AudioSession: Extend duration by playback position to" << m_duration
                         << "ms (position:" << positionMs << ")";
            }
        }

        if (m_decodeCompleted) {
            onPlaybackFinished();
        } else {
            // 尾段等待 EOS 时做日志节流，避免刷屏影响定位其他问题。
            if (!m_tailWaitLogActive) {
                m_tailWaitLogTimer.restart();
                m_tailWaitLogActive = true;
                qDebug() << "AudioSession: Position reached metadata duration but decode not completed, waiting for EOS";
            } else if (m_tailWaitLogTimer.elapsed() >= 1000) {
                m_tailWaitLogTimer.restart();
                qDebug() << "AudioSession: Still waiting EOS at tail."
                         << "position:" << positionMs
                         << "duration:" << m_duration;
            }

            // 安全兜底：若已超出元数据时长较多且缓冲极低，按尾段自然结束收敛。
            const AudioBuffer* buffer = m_player ? m_player->getBuffer() : nullptr;
            const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
            if (positionMs >= m_duration + 2500 && bufferedBytes < 32 * 1024) {
                qWarning() << "AudioSession: Tail force-finish fallback."
                           << "position:" << positionMs
                           << "duration:" << m_duration
                           << "bufferedBytes:" << bufferedBytes;
                onPlaybackFinished();
            }
        }
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
    
    const AudioBuffer* buffer = m_player->getBuffer();
    const int bufferedBytes = buffer ? buffer->availableBytes() : 0;
    const bool isRemoteFlac = m_sourceUrl.scheme().startsWith("http")
            && m_sourceUrl.path().toLower().endsWith(".flac");
    int kBufferingEnterMinBytes = m_seekFastStartMode ? (24 * 1024) : (96 * 1024);
    int kBufferingResumeMinBytes = m_seekFastStartMode ? (48 * 1024) : (192 * 1024);
    if (isRemoteFlac) {
        // 远端 FLAC 一律使用更高缓冲水位，避免周期性抖动。
        kBufferingEnterMinBytes = 224 * 1024;
        kBufferingResumeMinBytes = 768 * 1024;
    }

    // 水位回落后恢复此前因流控暂停的解码线程
    if (fillLevel < 60 && m_decoderPausedByFlowControl && m_decoder->isDecoding()) {
        m_decoder->startDecode();
        m_decoderPausedByFlowControl = false;
        qDebug() << "AudioSession: Buffer low (" << fillLevel << "%), resuming decoder";
    }
    
    // 低水位进入 buffering，并暂停输出等待回填
    // 仅当“百分比低 + 绝对字节也低”时才进入，避免大缓冲容量场景下反复抖动
    const bool resumeDebounceActive = m_hasRecentResume
            && m_lastResumeTimer.isValid()
            && m_lastResumeTimer.elapsed() < 220;
    if (resumeDebounceActive) {
        // 刚恢复播放后的短窗口内忽略一次低水位回落，避免图标/状态闪烁。
        return;
    }

    if (fillLevel < 10
            && bufferedBytes < kBufferingEnterMinBytes
            && m_player->isPlaying()
            && !m_isBuffering) {
        const qint64 currentPos = m_player->getPlaybackPosition();
        const qint64 remainingToDuration = (m_duration > 0) ? (m_duration - currentPos) : -1;

        // 尾段兜底：接近结尾时低水位通常是自然排空，直接收敛结束，避免“卡在最后几秒”。
        if (remainingToDuration >= 0 && remainingToDuration <= 1500) {
            qint64 finalDuration = m_duration;
            if (m_lastDecodedTimestampMs > finalDuration) {
                finalDuration = m_lastDecodedTimestampMs;
            }
            if (currentPos > finalDuration) {
                finalDuration = currentPos;
            }
            if (finalDuration > 0 && finalDuration != m_duration) {
                m_duration = finalDuration;
                emit durationChanged(m_duration);
            }
            qDebug() << "AudioSession: Tail low-buffer finish."
                     << "position:" << currentPos
                     << "duration:" << m_duration
                     << "remaining:" << remainingToDuration
                     << "bufferedBytes:" << bufferedBytes;
            onPlaybackFinished();
            return;
        }

        // 尾段兜底：接近结尾且解码时间轴已明显落后，说明已进入自然尾排空，直接收敛结束。
        if (!m_decodeCompleted
                && m_duration > 0
                && currentPos >= m_duration - 8000
                && m_lastDecodedTimestampMs > 0
                && currentPos >= m_lastDecodedTimestampMs + 800
                && bufferedBytes < 16 * 1024) {
            qWarning() << "AudioSession: Tail fallback finish without decodeCompleted."
                       << "position:" << currentPos
                       << "duration:" << m_duration
                       << "lastDecoded:" << m_lastDecodedTimestampMs
                       << "bufferedBytes:" << bufferedBytes;
            onPlaybackFinished();
            return;
        }

        // 解码已完成时，低水位属于尾段自然排空，不应再进入 buffering/pause。
        if (m_decodeCompleted) {
            qint64 tailRef = m_duration;
            if (m_lastDecodedTimestampMs > tailRef) {
                tailRef = m_lastDecodedTimestampMs;
            }
            const qint64 remainingMs = tailRef > 0 ? (tailRef - currentPos) : -1;

            if (fillLevel <= 2 || (remainingMs >= 0 && remainingMs <= 1500)) {
                qDebug() << "AudioSession: Tail drained after decode complete, finishing playback."
                         << "fill:" << fillLevel << "% remaining:" << remainingMs << "ms";
                onPlaybackFinished();
                return;
            }

            qDebug() << "AudioSession: Decode already complete, skip buffering pause at tail."
                     << "fill:" << fillLevel << "% remaining:" << remainingMs << "ms";
            return;
        }

        m_isBuffering = true;
        m_internalPausePending = true;
        m_player->pause();
        emit bufferingStarted();
        qDebug() << "AudioSession: Buffering started, fill level:" << fillLevel << "%";
    }
    // 水位/字节恢复后退出 buffering（字节门槛优先，百分比兜底）
    else if (m_isBuffering) {
        int resumeLevelThreshold = m_seekFastStartMode ? 6 : 25;
        if (isRemoteFlac) {
            const int flacLevel = m_seekFastStartMode ? 20 : 24;
            resumeLevelThreshold = qMax(resumeLevelThreshold, flacLevel);
        }

        if (isRemoteFlac) {
            // 远端 FLAC 避免“只满足 25% 就恢复”的抖动，改为双门槛都满足再恢复。
            if (fillLevel < resumeLevelThreshold || bufferedBytes < kBufferingResumeMinBytes) {
                return;
            }
        } else {
            if (fillLevel < resumeLevelThreshold && bufferedBytes < kBufferingResumeMinBytes) {
                return;
            }
        }

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
        m_lastResumeTimer.restart();
        m_hasRecentResume = true;
        
        // 退出 buffering 后确保解码继续推进
        m_decoder->startDecode();
        emit bufferingFinished();
        m_seekFastStartMode = false;
        qDebug() << "AudioSession: Buffering finished, fill level:" << fillLevel << "%,"
                 << "buffered bytes:" << bufferedBytes;
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
        m_internalPausePending = true;
        m_player->pause();
        emit bufferingStarted();
    }
}

void AudioSession::onDecodeCompleted()
{
    m_decodeCompleted = true;
    m_tailWaitLogActive = false;
    if (m_lastDecodedTimestampMs > m_duration) {
        m_duration = m_lastDecodedTimestampMs;
        emit durationChanged(m_duration);
        qDebug() << "AudioSession: Final duration corrected to decoded tail:" << m_duration << "ms";
    }
    qDebug() << "AudioSession: Decode completed";
    emit decodeFinished();
    
    // 解码完成仅表示数据生产结束
    // 会话结束仍以实际播放完成事件为准
}

void AudioSession::onDecodeStarted()
{
    m_decodeCompleted = false;
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
    finalizeSeekLatency("playback_started");

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
    if (!m_active) {
        return;
    }

    if (!isWriteOwnerActive()) {
        // Owner handoff 后延迟到达的 paused 回调，清掉内部标记并忽略。
        m_internalPausePending = false;
        return;
    }

    if (m_internalPausePending) {
        m_internalPausePending = false;
        qDebug() << "AudioSession: Ignore internal playbackPaused";
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
    m_lastResumeTimer.restart();
    m_hasRecentResume = true;
    if (!m_seekLatencyActive || m_seekFirstPacketSeen) {
        finalizeSeekLatency("playback_resumed");
    } else {
        qDebug() << "[SEEK_METRIC] skip finalize on playback_resumed before first decoded packet"
                 << "desired:" << m_seekDesiredTargetMs
                 << "working:" << m_seekLatencyTargetMs
                 << "session:" << m_sessionId;
    }
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
    m_tailWaitLogActive = false;
    emit sessionFinished();
}

void AudioSession::onSeekRetryTimeout()
{
    if (!m_active || !isWriteOwnerActive() || !m_seekLatencyActive || m_seekFirstPacketSeen || m_manualPaused) {
        return;
    }

    const QString sourcePath = m_sourceUrl.path().toLower();
    const bool isHlsSeek = sourcePath.endsWith(".m3u8") || m_sourceUrl.toString().contains("/hls/");
    const bool isRemoteFlacSeek = (m_sourceUrl.scheme().startsWith("http") && sourcePath.endsWith(".flac"));
    const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
    const qint64 hardTimeoutMs = isHlsSeek
            ? qMin<qint64>(m_seekHardTimeoutMs, 12000)
            : (isRemoteFlacSeek ? 12000 : qMax<qint64>(m_seekHardTimeoutMs, 30000));
    const qint64 giveUpTimeoutMs = isHlsSeek
            ? qMax<qint64>(hardTimeoutMs, 45000)
            : (isRemoteFlacSeek ? qMax<qint64>(hardTimeoutMs, 15000) : qMax<qint64>(hardTimeoutMs, 60000));
    if (elapsedMs >= giveUpTimeoutMs) {
        qWarning() << "[SEEK_ABORT] seek timeout without decoded packet (give up)."
                   << "elapsed:" << elapsedMs
                   << "desired:" << m_seekDesiredTargetMs
                   << "working:" << m_seekLatencyTargetMs
                   << "session:" << m_sessionId;
        finalizeSeekLatency("seek_timeout_no_packet");
        m_isBuffering = false;
        emit bufferingFinished();
        emit sessionError(QStringLiteral("音频缓冲超时，请重试"));
        return;
    }

    if (elapsedMs >= hardTimeoutMs) {
        qWarning() << "[SEEK_DEGRADE] seek waiting exceeded hard timeout, keep buffering."
                   << "elapsed:" << elapsedMs
                   << "desired:" << m_seekDesiredTargetMs
                   << "working:" << m_seekLatencyTargetMs
                   << "session:" << m_sessionId;
        if (!m_isBuffering) {
            m_isBuffering = true;
            emit bufferingStarted();
        }
        emit bufferingProgress(qMax(1, m_bufferingPercent));
        m_decoder->startDecode();
        if (m_seekRetryTimer) {
            m_seekRetryTimer->start(1500);
        }
        return;
    }

    if (m_seekRetryCount >= m_seekMaxRetries) {
        qWarning() << "[SEEK_DEGRADE] Slow network while seeking, still buffering."
                   << "elapsed:" << elapsedMs
                   << "desired:" << m_seekDesiredTargetMs
                   << "working:" << m_seekLatencyTargetMs
                   << "session:" << m_sessionId;
        emit bufferingProgress(qMax(1, m_bufferingPercent));
        m_decoder->startDecode();
        if (m_seekRetryTimer) {
            m_seekRetryTimer->start(1800);
        }
        return;
    }

    ++m_seekRetryCount;
    qWarning() << "[SEEK_RETRY] No decoded packet after seek, retry"
               << m_seekRetryCount << "/" << m_seekMaxRetries
               << "elapsed:" << elapsedMs
               << "desired:" << m_seekDesiredTargetMs
               << "working:" << m_seekLatencyTargetMs
               << "session:" << m_sessionId;

    // HLS/远端 FLAC 在首包阶段频繁重复 seek 会不断重置 demux 状态，导致长时间无首包。
    const bool avoidRepeatReseek = isHlsSeek || isRemoteFlacSeek;
    if (!avoidRepeatReseek || m_seekRetryCount <= 1) {
        m_decoder->seekTo(m_seekLatencyTargetMs);
        m_minAcceptedDecodeGeneration = m_decoder->currentSeekGeneration();
    } else {
        qWarning() << "[SEEK_WAIT] seek waiting for first packet, skip extra reseek."
                   << "elapsed:" << elapsedMs
                   << "desired:" << m_seekDesiredTargetMs
                   << "working:" << m_seekLatencyTargetMs
                   << "session:" << m_sessionId;
    }
    m_decoder->startDecode();
    if (!m_isBuffering) {
        m_isBuffering = true;
        emit bufferingStarted();
    }

    if (m_seekRetryTimer) {
        const int nextMs = 900 + m_seekRetryCount * 450;
        m_seekRetryTimer->start(nextMs);
    }
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

void AudioSession::finalizeSeekLatency(const char* stage)
{
    if (!m_seekLatencyActive) {
        return;
    }

    const qint64 elapsedMs = m_seekLatencyTimer.isValid() ? m_seekLatencyTimer.elapsed() : -1;
    const QString metricName = QString("[SEEK_METRIC] seek_to_%1_ms:").arg(QString::fromUtf8(stage));
    qDebug() << metricName << elapsedMs
             << "desired:" << m_seekDesiredTargetMs
             << "working:" << m_seekLatencyTargetMs
             << "first_decoded_seen:" << m_seekFirstPacketSeen
             << "first_decoded_ts:" << m_seekFirstPacketTimestampMs
             << "session:" << m_sessionId;
    m_seekLatencyActive = false;
    m_seekDesiredTargetMs = 0;
    m_seekFastStartMode = false;
    m_seekRetryCount = 0;
    m_seekAccuracyRetryCount = 0;
    if (m_seekRetryTimer) {
        m_seekRetryTimer->stop();
    }
}


