#include "frame_buffer_queue.h"
#include <QDebug>

// ========== VideoFrameQueue 实现 ==========

VideoFrameQueue::VideoFrameQueue(int maxSize)
    : maxSize(maxSize)
    , abortFlag(false)
{
}

VideoFrameQueue::~VideoFrameQueue()
{
    clear();
}

bool VideoFrameQueue::enqueue(const VideoFrame& frame, int timeoutMs)
{
    QMutexLocker locker(&mutex);
    
    // 等待队列有空间
    while (queue.size() >= maxSize && !abortFlag) {
        if (timeoutMs < 0) {
            notFull.wait(&mutex);
        } else {
            if (!notFull.wait(&mutex, timeoutMs)) {
                return false; // 超时
            }
        }
    }
    
    if (abortFlag) {
        return false;
    }
    
    queue.enqueue(frame);
    notEmpty.wakeOne();
    
    return true;
}

bool VideoFrameQueue::dequeue(VideoFrame& frame, int timeoutMs)
{
    QMutexLocker locker(&mutex);
    
    // 等待队列有数据
    while (queue.isEmpty() && !abortFlag) {
        if (timeoutMs < 0) {
            notEmpty.wait(&mutex);
        } else {
            if (!notEmpty.wait(&mutex, timeoutMs)) {
                return false; // 超时
            }
        }
    }
    
    if (abortFlag || queue.isEmpty()) {
        return false;
    }
    
    frame = queue.dequeue();
    notFull.wakeOne();
    
    return true;
}

bool VideoFrameQueue::peek(VideoFrame& frame)
{
    QMutexLocker locker(&mutex);
    
    if (queue.isEmpty()) {
        return false;
    }
    
    frame = queue.head();
    return true;
}

void VideoFrameQueue::clear()
{
    QMutexLocker locker(&mutex);
    queue.clear();
    notFull.wakeAll();
}

int VideoFrameQueue::size() const
{
    QMutexLocker locker(&mutex);
    return queue.size();
}

bool VideoFrameQueue::isEmpty() const
{
    QMutexLocker locker(&mutex);
    return queue.isEmpty();
}

bool VideoFrameQueue::isFull() const
{
    QMutexLocker locker(&mutex);
    return queue.size() >= maxSize;
}

void VideoFrameQueue::setAbort(bool abort)
{
    QMutexLocker locker(&mutex);
    abortFlag = abort;
    notEmpty.wakeAll();
    notFull.wakeAll();
}

// ========== AudioFrameQueue 实现 ==========

AudioFrameQueue::AudioFrameQueue(int maxSize)
    : maxSize(maxSize)
    , abortFlag(false)
{
}

AudioFrameQueue::~AudioFrameQueue()
{
    clear();
}

bool AudioFrameQueue::enqueue(const AudioFrame& frame, int timeoutMs)
{
    QMutexLocker locker(&mutex);
    
    while (queue.size() >= maxSize && !abortFlag) {
        if (timeoutMs < 0) {
            notFull.wait(&mutex);
        } else {
            if (!notFull.wait(&mutex, timeoutMs)) {
                return false;
            }
        }
    }
    
    if (abortFlag) {
        return false;
    }
    
    queue.enqueue(frame);
    notEmpty.wakeOne();
    
    return true;
}

bool AudioFrameQueue::dequeue(AudioFrame& frame, int timeoutMs)
{
    QMutexLocker locker(&mutex);
    
    while (queue.isEmpty() && !abortFlag) {
        if (timeoutMs < 0) {
            notEmpty.wait(&mutex);
        } else {
            if (!notEmpty.wait(&mutex, timeoutMs)) {
                return false;
            }
        }
    }
    
    if (abortFlag || queue.isEmpty()) {
        return false;
    }
    
    frame = queue.dequeue();
    notFull.wakeOne();
    
    return true;
}

bool AudioFrameQueue::peek(AudioFrame& frame)
{
    QMutexLocker locker(&mutex);
    
    if (queue.isEmpty()) {
        return false;
    }
    
    frame = queue.head();
    return true;
}

void AudioFrameQueue::clear()
{
    QMutexLocker locker(&mutex);
    queue.clear();
    notFull.wakeAll();
}

int AudioFrameQueue::size() const
{
    QMutexLocker locker(&mutex);
    return queue.size();
}

bool AudioFrameQueue::isEmpty() const
{
    QMutexLocker locker(&mutex);
    return queue.isEmpty();
}

bool AudioFrameQueue::isFull() const
{
    QMutexLocker locker(&mutex);
    return queue.size() >= maxSize;
}

void AudioFrameQueue::setAbort(bool abort)
{
    QMutexLocker locker(&mutex);
    abortFlag = abort;
    notEmpty.wakeAll();
    notFull.wakeAll();
}
