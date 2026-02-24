#ifndef MEDIASESSION_H
#define MEDIASESSION_H

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QThread>
#include <QString>
#include <mutex>
#include <condition_variable>
#include "AudioSession.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

// 前向声明
class VideoSession;

/**
 * @brief 媒体会话 - 协调音视频播放
 * 
 * 设计思路：
 * - 管理 AudioSession（复用现有）和 VideoSession（新增）
 * - FFmpeg 解复用器在此层，分发音视频包
 * - 音视频同步协调
 * - 统一的播放控制接口
 */
class MediaSession : public QObject
{
    Q_OBJECT
    
public:
    enum PlaybackState {
        Stopped,
        Playing,
        Paused,
        Error
    };
    Q_ENUM(PlaybackState)
    
    explicit MediaSession(QObject* parent = nullptr);
    ~MediaSession() override;
    
    // ===== 媒体源控制 =====
    bool loadSource(const QUrl& url);
    void unloadSource();
    
    // ===== 播放控制 =====
    void play();
    void pause();
    void stop();
    void seekTo(qint64 positionMs);
    
    // ===== 获取子会话 =====
    AudioSession* audioSession() const { return m_audioSession; }
    VideoSession* videoSession() const { return m_videoSession; }
    
    // ===== 同步控制 =====
    void enableSync(bool enable);
    bool isSyncEnabled() const { return m_syncEnabled; }
    
    // ===== 状态查询 =====
    qint64 getCurrentPosition() const;
    qint64 getDuration() const { return m_duration; }
    PlaybackState state() const { return m_state; }
    bool hasVideo() const { return m_videoSession != nullptr; }
    bool hasAudio() const { return m_audioCodecCtx != nullptr || m_audioStreamIndex >= 0; }
    
signals:
    // 位置和状态信号
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void stateChanged(PlaybackState state);
    
    // 元数据信号
    void metadataReady(QString title, QString artist, qint64 duration);
    
    // 同步信号
    void syncError(int offsetMs);  // 音视频不同步（偏差值）
    
    // 播放完成
    void playbackFinished();
    void demuxFinished();
    
private slots:
    void onAudioPositionChanged(qint64 audioPos);
    void onVideoFrameRendered(qint64 videoPts);
    void syncAudioVideo();
    void updatePosition();
    
private:
    // ===== 初始化和清理 =====
    bool initDemuxer(const QString& filePath);
    void cleanupDemuxer();
    bool initAudioDecoder(AVStream* stream);
    void cleanupAudioDecoder();
    
    // ===== 解复用控制 =====
    void startDemux();
    void stopDemux();
    void demuxLoop();
    
    // ===== 状态管理 =====
    void setState(PlaybackState state);
    
private:
    // 子会话
    AudioSession* m_audioSession;
    VideoSession* m_videoSession;
    
    // FFmpeg 解复用器
    AVFormatContext* m_formatContext;
    int m_audioStreamIndex;
    int m_videoStreamIndex;
    
    // 音频解码器
    AVCodecContext* m_audioCodecCtx;
    SwrContext* m_audioSwrCtx;
    AVFrame* m_audioFrame;
    
    // 媒体信息
    QString m_currentUrl;
    qint64 m_duration;  // 总时长（ms）
    PlaybackState m_state;
    
    // 同步控制
    QTimer* m_syncTimer;
    bool m_syncEnabled;
    qint64 m_masterClock;  // 主时钟（音频）
    
    // 解复用线程
    QThread* m_demuxThread;
    bool m_demuxRunning;
    bool m_demuxPaused;
    std::mutex m_demuxPauseMutex;
    std::condition_variable m_demuxPauseCv;
    bool m_seekPending;  // Seek后等待首帧同步标志
    
    // MediaSession写入AudioPlayer时使用的所有权ID
    QString m_audioWriteOwnerId;
    
    // 位置更新定时器
    QTimer* m_positionTimer;
};

#endif // MEDIASESSION_H
