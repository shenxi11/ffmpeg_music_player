#ifndef VIDEO_AUDIO_PLAYER_H
#define VIDEO_AUDIO_PLAYER_H

#include <QObject>
#include <QAudioOutput>
#include <QIODevice>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

class VideoAudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit VideoAudioPlayer(QObject *parent = nullptr);
    ~VideoAudioPlayer();

public slots:
    void receiveAudioData(const QByteArray &data, qint64 pts);
    void start();
    void pause();
    void stop();
    void setVolume(int volume); // 0-100
    
    // 获取当前音频时钟（用于同步）
    qint64 getAudioClock() const;

private:
    void playbackThread();  // 播放线程函数
    
    QAudioOutput* audioOutput;
    QIODevice* audioDevice;
    
    // 线程同步
    std::thread* playThread;
    std::mutex bufferMutex;
    std::condition_variable bufferCondition;
    std::atomic<bool> threadRunning;
    std::atomic<bool> isPlaying;
    
    // 音频缓冲
    struct AudioData {
        QByteArray data;
        qint64 pts;
    };
    std::queue<AudioData> audioBuffer;
    
    // 音频时钟（用于同步）
    mutable std::mutex clockMutex;
    qint64 currentAudioPts;  // 当前播放的音频PTS
    qint64 lastAudioPts;     // 上次更新的PTS
    qint64 lastAudioTime;    // 上次更新的系统时间
    qint64 firstAudioPts;    // 首帧音频PTS（作为基准）
};

#endif // VIDEO_AUDIO_PLAYER_H
