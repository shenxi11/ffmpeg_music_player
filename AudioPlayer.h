#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QAudioOutput>
#include <QIODevice>
#include <QTimer>
#include <QElapsedTimer>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "AudioBuffer.h"

/**
 * @brief 音频播放控制器模块
 * 职责：管理音频硬件输出、播放控制和音频时钟同步
 * 注：使用单例模式，全局共享一个音频设备实例以消除启动延迟
 */
class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    // 单例接口
    static AudioPlayer& instance();
    
    // 禁止复制和赋值
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    
    // 播放控制
    bool start();
    void pause();
    void resume();
    void stop();
    
    // 清空缓冲区（切换歌曲时使用）
    void resetBuffer();
    
    // 音量控制 (0-100)
    void setVolume(int volume);
    int volume() const { return m_volume; }
    
    // 写入音频数据（从解码器）
    void writeAudioData(const QByteArray& data, qint64 timestampMs);
    
    // 时钟同步
    qint64 getCurrentTimestamp() const;  // 获取当前播放时间戳
    void setCurrentTimestamp(qint64 timestampMs);  // 设置当前时间戳（seek时使用）
    
    // 状态查询
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    
    // 缓冲区访问
    AudioBuffer* getBuffer() const { return m_buffer; }
    
    // 缓冲区状态
    int bufferFillLevel() const;  // 0-100

signals:
    // 播放状态变化
    void playbackStarted();
    void playbackPaused();
    void playbackResumed();
    void playbackStopped();
    
    // 播放进度（用于UI更新）
    void positionChanged(qint64 positionMs);
    
    // 缓冲状态
    void bufferStatusChanged(int fillLevel);
    void bufferUnderrun();  // 缓冲区饥饿
    
    // 错误信号
    void playbackError(const QString& error);

private slots:
    void onPositionUpdateTimer();

private:
    // 私有构造/析构（单例模式）
    AudioPlayer();
    ~AudioPlayer();
    
    void playbackThread();  // 播放线程
    bool initAudioOutput();
    void cleanupAudioOutput();
    void updateTimestamp(qint64 bytesConsumed);  // 更新时间戳
    
    // Qt 音频组件
    QAudioOutput* m_audioOutput;
    QIODevice* m_audioDevice;
    QTimer* m_positionTimer;
    
    // 缓冲区
    AudioBuffer* m_buffer;
    
    // 时间戳队列（用于同步）
    struct TimestampedData {
        qint64 timestamp;
        int dataSize;
    };
    std::queue<TimestampedData> m_timestampQueue;
    mutable std::mutex m_timestampMutex;
    
    // 播放线程
    std::thread m_playThread;
    std::mutex m_playMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_threadRunning;
    std::atomic<bool> m_stopRequested;
    
    // 播放状态
    std::atomic<bool> m_isPlaying;
    std::atomic<bool> m_isPaused;
    std::atomic<int> m_volume;
    
    // 音频时钟
    qint64 m_currentTimestamp;
    qint64 m_baseTimestamp;
    QElapsedTimer m_playbackTimer;
    qint64 m_playbackStartTimestamp;  // 播放开始时的时间戳（用于时钟同步）
    
    // 音频格式
    int m_sampleRate;
    int m_channels;
};

#endif // AUDIOPLAYER_H
