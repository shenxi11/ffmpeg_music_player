#ifndef AUDIOSESSION_H
#define AUDIOSESSION_H

#include <QObject>
#include <QUrl>
#include <QTimer>
#include "AudioDecoder.h"
#include "AudioPlayer.h"
#include "AudioBuffer.h"

/**
 * @brief 音频会话管理模块
 * 职责：管理单个音频播放会话，协调解码器和播放器
 */
class AudioSession : public QObject
{
    Q_OBJECT
public:
    explicit AudioSession(const QString& sessionId, QObject* parent = nullptr);
    ~AudioSession();

    // 会话标识
    QString id() const { return m_sessionId; }
    
    // 加载音频源
    bool loadSource(const QUrl& url);
    
    // 播放控制
    void play();
    void pause();
    void resume();
    void stop();
    void seekTo(qint64 positionMs);
    
    // 音量控制
    void setVolume(int volume);
    
    // 状态查询
    bool isActive() const { return m_active; }
    bool isPlaying() const;
    bool isPaused() const;
    qint64 duration() const;
    qint64 position() const;
    
    // 元数据
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString albumArt() const { return m_albumArt; }

signals:
    // 会话状态
    void sessionStarted();
    void sessionPaused();
    void sessionResumed();
    void sessionStopped();
    void sessionFinished();  // 播放完成
    void sessionError(const QString& error);
    
    // 元数据就绪
    void metadataReady(const QString& title, const QString& artist, qint64 duration);
    void albumArtReady(const QString& imagePath);
    
    // 播放进度
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    
    // 缓冲状态
    void bufferingStarted();
    void bufferingProgress(int percent);
    void bufferingFinished();
    
    // 解码状态
    void decodeFinished();

private slots:
    // 解码器槽函数
    void onDecodedData(const QByteArray& data, qint64 timestampMs);
    void onMetadataReady(qint64 durationMs, int sampleRate, int channels);
    void onAlbumArtReady(const QString& imagePath);
    void onDecodeError(const QString& error);
    void onDecodeCompleted();
    void onDecodeStarted();
    void onDecodePaused();
    void onDecodeStopped();
    
    // 播放器槽函数
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
    
    // 会话信息
    QString m_sessionId;
    QUrl m_sourceUrl;
    bool m_active;
    
    // 核心组件
    AudioDecoder* m_decoder;
    AudioPlayer* m_player;
    
    // 元数据
    QString m_title;
    QString m_artist;
    QString m_albumArt;
    qint64 m_duration;
    
    // 缓冲管理
    bool m_isBuffering;
    int m_bufferingPercent;
    bool m_decoderPausedByFlowControl;  // 解码器是否被流控暂停
    
    // Seek优化
    bool m_seekGracePeriod;  // seek后的宽限期，避免立即触发buffering
    QTimer* m_seekGraceTimer;  // 宽限期定时器
    bool m_isSeeking;  // 正在执行seek操作，避免误发射状态信号
};

#endif // AUDIOSESSION_H
