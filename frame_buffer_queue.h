#ifndef FRAME_BUFFER_QUEUE_H
#define FRAME_BUFFER_QUEUE_H

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>

// 视频帧结构
struct VideoFrame {
    QImage image;
    qint64 pts;  // 显示时间戳（毫秒）
    
    VideoFrame() : pts(0) {}
    VideoFrame(const QImage& img, qint64 p) : image(img), pts(p) {}
};

// 音频帧结构
struct AudioFrame {
    QByteArray data;
    qint64 pts;  // 显示时间戳（毫秒）
    
    AudioFrame() : pts(0) {}
    AudioFrame(const QByteArray& d, qint64 p) : data(d), pts(p) {}
};

// 视频帧缓冲队列
class VideoFrameQueue
{
public:
    VideoFrameQueue(int maxSize = 100);
    ~VideoFrameQueue();
    
    // 入队（解码线程调用）
    bool enqueue(const VideoFrame& frame, int timeoutMs = -1);
    
    // 出队（播放线程调用）
    bool dequeue(VideoFrame& frame, int timeoutMs = -1);
    
    // 查看队首元素但不移除
    bool peek(VideoFrame& frame);
    
    // 清空队列
    void clear();
    
    // 获取当前队列大小
    int size() const;
    
    // 检查队列是否为空
    bool isEmpty() const;
    
    // 检查队列是否已满
    bool isFull() const;
    
    // 设置中止标志（用于停止等待）
    void setAbort(bool abort);
    
private:
    QQueue<VideoFrame> queue;
    mutable QMutex mutex;
    QWaitCondition notEmpty;  // 队列非空条件
    QWaitCondition notFull;   // 队列非满条件
    int maxSize;
    bool abortFlag;
};

// 音频帧缓冲队列
class AudioFrameQueue
{
public:
    AudioFrameQueue(int maxSize = 200);
    ~AudioFrameQueue();
    
    bool enqueue(const AudioFrame& frame, int timeoutMs = -1);
    bool dequeue(AudioFrame& frame, int timeoutMs = -1);
    bool peek(AudioFrame& frame);
    void clear();
    int size() const;
    bool isEmpty() const;
    bool isFull() const;
    void setAbort(bool abort);
    
private:
    QQueue<AudioFrame> queue;
    mutable QMutex mutex;
    QWaitCondition notEmpty;
    QWaitCondition notFull;
    int maxSize;
    bool abortFlag;
};

#endif // FRAME_BUFFER_QUEUE_H
