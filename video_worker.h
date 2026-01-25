#ifndef VIDEO_WORKER_H
#define VIDEO_WORKER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include "videode_coder.h"
#include "video_audio_player.h"

class VideoWorker : public QObject
{
    Q_OBJECT
public:
    explicit VideoWorker(QObject *parent = nullptr);
    ~VideoWorker();

public slots:
    // 播放控制
    void openVideo(const QString& filePath);
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);
    void setVolume(int volume); // 0-100

signals:
    // 视频信息
    void videoOpened(int width, int height, double fps, qint64 durationMs);
    void videoFrameReady(QImage frame, qint64 ptsMs);
    void playbackProgress(qint64 currentMs, qint64 totalMs);
    
    // 状态信号
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
    void errorOccurred(QString error);

private slots:
    // 内部处理
    void onVideoOpened(int width, int height, double fps, qint64 durationMs);
    void onDecodingFinished();
    void onErrorOccurred(QString error);

private:
    void videoPlaybackThread();  // 视频播放线程函数
    
    VideoDecoder* videoDecoder;
    QThread* decoderThread;
    VideoAudioPlayer* audioPlayer;
    
    // 视频播放线程
    std::thread* videoThread;
    std::mutex videoMutex;
    std::condition_variable videoCondition;
    std::atomic<bool> videoThreadRunning;
    
    // 视频帧缓冲
    struct VideoFrame {
        QImage image;
        qint64 pts;
    };
    std::queue<VideoFrame> videoBuffer;
    
    // 同步控制
    qint64 startPlayTime;  // 播放开始时间
    qint64 firstVideoPts;  // 第一帧PTS基准
    
    bool isPlaying;
    QString currentVideoPath;
    qint64 videoDuration;
    double videoFPS;
};

#endif // VIDEO_WORKER_H
