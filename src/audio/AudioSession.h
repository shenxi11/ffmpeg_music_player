#ifndef AUDIOSESSION_H
#define AUDIOSESSION_H

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QElapsedTimer>
#include <optional>
#include "AudioDecoder.h"
#include "AudioPlayer.h"
#include "AudioBuffer.h"

class QDebug;

/**
 * AudioSession 表示一次独立的音频播放会话。
 * 负责解码器、播放器与缓冲状态的协同，并对外暴露会话级信号。
 */
class AudioSession : public QObject
{
    Q_OBJECT
public:
    explicit AudioSession(const QString& sessionId, QObject* parent = nullptr);
    ~AudioSession();

    // 会话唯一标识，用于区分并发/切换时的会话归属
    QString id() const { return m_sessionId; }
    
    // 加载音频源（本地文件或网络 URL）
    bool loadSource(const QUrl& url);
    
    // 会话播放控制
    void play();
    void pause();
    void resume();
    void stop();
    void seekTo(qint64 positionMs);
    
    // 设置会话音量（最终写入共享 AudioPlayer）
    void setVolume(int volume);
    
    // 会话状态查询
    bool isActive() const { return m_active; }
    bool isPlaying() const;
    bool isPaused() const;
    qint64 duration() const;
    qint64 position() const;
    
    // 当前曲目信息
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString albumArt() const { return m_albumArt; }

signals:
    // 会话生命周期信号
    void sessionStarted();
    void sessionPaused();
    void sessionResumed();
    void sessionStopped();
    void sessionFinished();  // 当前会话自然播放结束
    void sessionError(const QString& error);
    
    // 元数据与封面信号
    void metadataReady(const QString& title, const QString& artist, qint64 duration);
    void albumArtReady(const QString& imagePath);
    
    // 播放进度与总时长信号
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    
    // 缓冲状态信号
    void bufferingStarted();
    void bufferingProgress(int percent);
    void bufferingFinished();
    
    // 解码阶段完成信号
    void decodeFinished();

private slots:
    // 解码器回调
    void onDecodedData(const QByteArray& data, qint64 timestampMs, int seekGeneration);
    void onMetadataReady(qint64 durationMs, int sampleRate, int channels);
    void onAudioTagsReady(const QString& title, const QString& artist);
    void onAlbumArtReady(const QString& imagePath);
    void onDecodeError(const QString& error);
    void onDecodeCompleted();
    void onDecodeStarted();
    void onDecodePaused();
    void onDecodeStopped();
    
    // 播放器回调
    void onPlaybackError(const QString& error);
    void onPositionChanged(qint64 positionMs);
    void onBufferStatusChanged(int fillLevel);
    void onBufferUnderrun();
    void onPlaybackStarted();
    void onPlaybackPaused();
    void onPlaybackResumed();
    void onPlaybackStopped();
    void onPlaybackFinished();
    void onSeekGraceTimeout();
    void onSeekRetryTimeout();

private:
    enum class SeekPhase {
        Idle,
        Seeking,
        Grace
    };

    enum class DecoderTimestampMode {
        Passthrough,
        ProbePending,
        OffsetApplied
    };

    enum class DecodePhase {
        Running,
        Completed
    };

    enum class SeekLatencyPhase {
        Inactive,
        WaitingFirstPacket,
        FirstPacketSeen
    };

    enum class SeekRetryAction {
        GiveUp,
        DegradeByHardTimeout,
        DegradeByRetryBudget,
        RetryNow
    };

    enum class ResumeEmitPolicy {
        PendingOnly,
        PendingOrUnsuppressed
    };

    struct PlaybackState {
        bool manualPaused = false;
        bool hasStarted = false;
        bool resumeSignalPending = false;
        bool internalPausePending = false;
    };

    struct BufferingState {
        bool active = false;
        int percent = 0;
        bool decoderPausedByFlowControl = false;
        bool hasRecentResume = false;
        bool tailWaitLogActive = false;
    };

    struct ClockState {
        qint64 lastDecodedMs = 0;
        qint64 lastPausedPlaybackMs = 0;
        qint64 baseTimestampMs = 0;
        qint64 offsetTimestampMs = 0;
        DecoderTimestampMode timestampMode = DecoderTimestampMode::Passthrough;
        int minAcceptedDecodeGeneration = 0;
    };

    struct SeekState {
        SeekPhase phase = SeekPhase::Idle;
        bool fastStartMode = false;
        int retryCount = 0;
        int maxRetries = 4;
        std::optional<qint64> pendingTakeoverSeekMs;
    };

    struct SeekLatencyState {
        SeekLatencyPhase phase = SeekLatencyPhase::Inactive;
        qint64 desiredTargetMs = 0;
        qint64 workingTargetMs = 0;
        qint64 firstPacketTimestampMs = -1;
        int accuracyRetryCount = 0;
        int accuracyMaxRetries = 5;
        qint64 accuracyToleranceMs = 2500;
        qint64 hardTimeoutMs = 20000;
    };

    struct DecodeState {
        DecodePhase phase = DecodePhase::Running;
    };

    // 基础生命周期与通用收口
    void connectSignals();
    void cleanup();
    void requestDecoderStart();
    void finalizeControlPath(const char* where);
    QDebug infoLog(const char* domain) const;
    QDebug warnLog(const char* domain) const;

    // 状态机/阶段迁移
    bool isWriteOwnerActive() const;
    void finalizeSeekLatency(const char* stage);
    void setSeekPhase(SeekPhase phase, const char* reason);
    void setSeekLatencyPhase(SeekLatencyPhase phase, const char* reason);
    void setDecodePhase(DecodePhase phase, const char* reason);
    void validateState(const char* where) const;

    // Seek 核心控制链
    void resetRuntimeState();
    void beginSeekLatencyTracking(qint64 targetMs);
    void applySeekClockBase(qint64 targetMs);
    void enterSeekGraceWindow();
    qint64 resolveResumeTimestamp() const;

    // 播放事件分发与会话信号策略
    bool shouldSuppressExternalPauseResumeSignals() const;
    bool consumeResumeSignalPendingAndEmit();
    bool emitSessionResumedByPolicy(ResumeEmitPolicy policy);
    bool clearInternalPauseIfOwnershipLost();
    bool consumeInternalPausePending();
    void markPlaybackResumedActivity();
    void finalizeSeekLatencyOnPlaybackResumed();

    // Owner 接管与切换
    bool shouldTakeOverFromOwner(const QString& currentOwner, bool treatEmptyOwnerAsTakeover) const;
    void pauseSharedPlayerForTakeover(bool takingOverFromOtherOwner);
    void assignSessionAsWriteOwner();
    void resetSharedOutputAfterTakeover(qint64 timestampMs);
    void deferSeekUntilOwnerTakeover(qint64 positionMs);
    bool handleResumeOwnerTakeover(const QString& currentOwner);
    void applyResumeTakeoverAtTimestamp(const QString& currentOwner, qint64 resumeTimestamp);
    void startOrResumePlayerOutput();

    // 本地 seek 执行链
    bool isPlayerActivelyPlaying() const;
    void pausePipelineForLocalSeek(bool wasPlaying);
    void prepareOutputForLocalSeek(qint64 positionMs, bool wasPlaying);
    void dispatchDecoderSeekForLocalSeek(qint64 positionMs);

    // 解码时间轴/缓冲阈值/恢复判定
    bool isRemoteFlacSource() const;
    void normalizeDecodedTimestamp(qint64 timestampMs, qint64& normalizedTimestampMs);
    void extendDurationFromDecodedTimestamp(qint64 normalizedTimestampMs);
    bool handleSeekLatencyOnDecodedTimestamp(qint64 normalizedTimestampMs);
    void applyDecoderFlowControlBeforeWrite(bool isRemoteFlac, int fillLevelBeforeWrite);
    bool shouldResumeFromDecodedWrite(int fillLevel, int bufferedBytes, bool isRemoteFlac,
                                      int& requiredLevel, int& requiredBytes) const;
    void startPlayerFromDecodedReady(int fillLevel);
    void resumePlayerFromDecodedReady(int fillLevel, int bufferedBytes, int requiredLevel, int requiredBytes);
    void completeDecodedReadyResume();
    bool shouldDebounceBufferingEnter() const;
    bool handleTailLowBufferFallback(qint64 currentPos, int fillLevel, int bufferedBytes);
    bool canExitBufferingByThreshold(int fillLevel, int bufferedBytes, bool isRemoteFlac,
                                     int bufferingResumeMinBytes) const;
    void exitBufferingAndResume(int fillLevel, int bufferedBytes);

    // Seek 重试与退化策略
    bool isHlsSeekSource() const;
    qint64 calculateSeekHardTimeoutMs(bool isHlsSeek, bool isRemoteFlacSeek) const;
    qint64 calculateSeekGiveUpTimeoutMs(bool isHlsSeek, bool isRemoteFlacSeek, qint64 hardTimeoutMs) const;
    SeekRetryAction decideSeekRetryAction(qint64 elapsedMs, qint64 hardTimeoutMs, qint64 giveUpTimeoutMs) const;
    void handleSeekRetryGiveUp(qint64 elapsedMs);
    void handleSeekRetryDegrade(qint64 elapsedMs, int nextRetryMs, const char* logTag);
    void handleSeekRetryAttempt(qint64 elapsedMs, bool avoidRepeatReseek);
    bool isSeekingNow() const { return m_seek.phase != SeekPhase::Idle; }
    bool isSeekGraceActive() const { return m_seek.phase == SeekPhase::Grace; }
    bool isDecodeCompletedNow() const { return m_decode.phase == DecodePhase::Completed; }
    bool isSeekLatencyActiveNow() const { return m_seekLatency.phase != SeekLatencyPhase::Inactive; }
    bool isSeekFirstPacketSeenNow() const { return m_seekLatency.phase == SeekLatencyPhase::FirstPacketSeen; }
    
    // 会话基础状态
    QString m_sessionId;
    QUrl m_sourceUrl;
    bool m_active;
    
    // 核心组件
    AudioDecoder* m_decoder;
    AudioPlayer* m_player;
    
    // 当前媒体元数据
    QString m_title;
    QString m_artist;
    QString m_albumArt;
    qint64 m_duration;

    // 会话运行状态（替代分散标志位）
    PlaybackState m_playback;
    BufferingState m_buffering;
    ClockState m_clock;
    SeekState m_seek;
    SeekLatencyState m_seekLatency;
    DecodeState m_decode;

    // seek 宽限窗口，避免 seek 后瞬时状态抖动影响 UI 与自动控制
    QTimer* m_seekGraceTimer;
    QTimer* m_seekRetryTimer;
    QElapsedTimer m_lastResumeTimer;
    QElapsedTimer m_tailWaitLogTimer;
    QElapsedTimer m_seekLatencyTimer;
};

#endif // AUDIOSESSION_H

