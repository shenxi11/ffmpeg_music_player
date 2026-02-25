#ifndef AUDIOSESSION_H
#define AUDIOSESSION_H

#include <QObject>
#include <QUrl>
#include <QTimer>
#include "AudioDecoder.h"
#include "AudioPlayer.h"
#include "AudioBuffer.h"

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
    void onDecodedData(const QByteArray& data, qint64 timestampMs);
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

private:
    void connectSignals();
    void cleanup();
    bool isWriteOwnerActive() const;
    
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
    
    // 缓冲与时间戳状态（用于 seek/切换后恢复）
    bool m_isBuffering;
    int m_bufferingPercent;
    bool m_decoderPausedByFlowControl;
    bool m_hasStartedPlayback;
    bool m_manualPaused;
    qint64 m_lastDecodedTimestampMs;
    qint64 m_lastPausedPlaybackMs;
    qint64 m_decoderTimestampBaseMs;
    qint64 m_decoderTimestampOffsetMs;
    bool m_decoderTimestampNeedsProbe;
    bool m_decoderTimestampHasOffset;
    bool m_pendingResumeSignal;
    
    // seek 宽限窗口，避免 seek 后瞬时状态抖动影响 UI 与自动控制
    bool m_seekGracePeriod;
    QTimer* m_seekGraceTimer;
    bool m_isSeeking;
};

#endif // AUDIOSESSION_H

