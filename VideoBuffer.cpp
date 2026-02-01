#include "VideoBuffer.h"
#include <QDebug>
#include <QMutexLocker>

VideoBuffer::VideoBuffer(QObject* parent)
    : QObject(parent)
    , m_maxSize(30)  // 默认缓冲30帧（约1秒@30fps）
{
    qDebug() << "[VideoBuffer] Created with capacity:" << m_maxSize;
}

VideoBuffer::~VideoBuffer()
{
    qDebug() << "[VideoBuffer] Destroying...";
    clear();
    qDebug() << "[VideoBuffer] Destroyed";
}

void VideoBuffer::push(VideoFrame* frame)
{
    if (!frame) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 检查缓冲区是否已满
    if (m_queue.size() >= m_maxSize) {
        // 缓冲区满了，删除最旧的帧（PTS最小的帧）
        // 保持buffer为滑动窗口，始终包含最新的30帧
        VideoFrame* oldFrame = m_queue.dequeue();  // 队列头部是PTS最小的
        delete oldFrame;
        emit bufferFull();
    }
    
    // 按PTS排序插入
    insertSorted(frame);
    
    // 发送缓冲区级别变化信号
    emit bufferLevelChanged(m_queue.size());
}

VideoFrame* VideoBuffer::pop()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_queue.isEmpty()) {
        emit bufferEmpty();
        return nullptr;
    }
    
    VideoFrame* frame = m_queue.dequeue();
    
    // 发送缓冲区级别变化信号
    emit bufferLevelChanged(m_queue.size());
    
    //qDebug() << "[VideoBuffer] Popped frame - PTS:" << frame->pts 
    //         << "Buffer size:" << m_queue.size();
    
    return frame;
}

VideoFrame* VideoBuffer::peek() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_queue.isEmpty()) {
        return nullptr;
    }
    
    return m_queue.first();
}

void VideoBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    
    int count = m_queue.size();
    
    // 删除所有帧
    while (!m_queue.isEmpty()) {
        VideoFrame* frame = m_queue.dequeue();
        delete frame;
    }
    
    qDebug() << "[VideoBuffer] Cleared" << count << "frames";
    
    emit bufferLevelChanged(0);
}

void VideoBuffer::clearNonKeyFrames()
{
    QMutexLocker locker(&m_mutex);
    
    QQueue<VideoFrame*> newQueue;
    int removedCount = 0;
    
    // 保留关键帧，删除非关键帧
    while (!m_queue.isEmpty()) {
        VideoFrame* frame = m_queue.dequeue();
        
        if (frame->isKeyFrame) {
            newQueue.enqueue(frame);
        } else {
            delete frame;
            removedCount++;
        }
    }
    
    m_queue = newQueue;
    
    qDebug() << "[VideoBuffer] Cleared" << removedCount << "non-key frames"
             << "Remaining:" << m_queue.size();
    
    emit bufferLevelChanged(m_queue.size());
}

int VideoBuffer::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_queue.size();
}

bool VideoBuffer::isEmpty() const
{
    QMutexLocker locker(&m_mutex);
    return m_queue.isEmpty();
}

bool VideoBuffer::isFull() const
{
    QMutexLocker locker(&m_mutex);
    return m_queue.size() >= m_maxSize;
}

void VideoBuffer::insertSorted(VideoFrame* frame)
{
    // 按PTS从小到大排序插入
    // 注意：此函数在已加锁状态下调用
    
    if (m_queue.isEmpty()) {
        m_queue.enqueue(frame);
        return;
    }
    
    // 简化版本：大多数情况下帧是按顺序到达的
    // 如果新帧的PTS大于队列最后一帧，直接追加
    if (frame->pts >= m_queue.last()->pts) {
        m_queue.enqueue(frame);
        return;
    }
    
    // 否则需要查找插入位置
    QQueue<VideoFrame*> tempQueue;
    bool inserted = false;
    
    while (!m_queue.isEmpty()) {
        VideoFrame* currentFrame = m_queue.dequeue();
        
        if (!inserted && frame->pts < currentFrame->pts) {
            tempQueue.enqueue(frame);
            inserted = true;
        }
        
        tempQueue.enqueue(currentFrame);
    }
    
    // 如果还没插入（所有帧PTS都小于新帧），追加到末尾
    if (!inserted) {
        tempQueue.enqueue(frame);
    }
    
    m_queue = tempQueue;
}
