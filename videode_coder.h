#ifndef VIDEODE_CODER_H
#define VIDEODE_CODER_H

#include <QObject>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "headers.h"

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

public slots:
    void openVideo(QString videoPath);
    void seekToPosition(qint64 timestampMs);
    void startDecoding();
    void pauseDecoding();
    void stopDecoding();
    
signals:
    // 发送解码后的视频帧（RGB格式）
    void videoFrameReady(QImage frame, qint64 ptsMs);
    
    // 发送解码后的音频数据
    void audioDataReady(const QByteArray& data, qint64 ptsMs);
    
    // 视频信息
    void videoOpened(int width, int height, double fps, qint64 durationMs);
    void videoSizeChanged(int width, int height);
    void durationChanged(qint64 durationMs);
    
    // 解码状态
    void decodingStarted();
    void decodingPaused();
    void decodingStopped();
    void decodingFinished();
    void errorOccurred(QString error);
    
    // 内部信号（线程通信）
    void signal_start_decode();
    void signal_open_video(QString path);

private:
    void decodeThread();  // 解码线程主循环
    void openVideoInternal(const QString& path);
    void cleanupResources();
    bool decodeOneFrame();
    QImage convertFrameToImage(AVFrame* frame);
    
    // FFmpeg 组件
    AVFormatContext* formatCtx;
    AVCodecContext* videoCodecCtx;
    AVCodecContext* audioCodecCtx;
    AVFrame* videoFrame;
    AVFrame* rgbFrame;
    AVFrame* audioFrame;
    AVPacket* packet;
    SwsContext* swsCtx;
    SwrContext* swrCtx;
    
    int videoStreamIndex;
    int audioStreamIndex;
    
    // 视频信息
    int videoWidth;
    int videoHeight;
    double videoFPS;
    qint64 videoDuration;  // 微秒
    AVRational timeBase;
    
    // 线程控制
    std::thread decodeThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> isRunning_;
    std::atomic<bool> shouldDecode_;
    std::atomic<bool> shouldStop_;
    
    // 帧缓冲
    uint8_t* rgbBuffer;
    int rgbBufferSize;
    
    // 播放时间控制
    qint64 startTime;
    qint64 pauseTime;
    bool isPaused;
    
    // 帧率控制
    double frameInterval;  // 每帧间隔(毫秒)
};

#endif // VIDEODE_CODER_H
