#ifndef VIDEOBUFFER_H
#define VIDEOBUFFER_H

#include "VideoDecoder.h"

#include <QMutex>
#include <QObject>
#include <QQueue>

/**
 * @brief 视频帧缓冲区 - PTS排序队列
 *
 * 设计思路：
 * - 使用优先级队列按PTS排序
 * - 支持跳帧（清除非关键帧）
 * - 缓冲区大小限制
 */
class VideoBuffer : public QObject {
    Q_OBJECT

  public:
    struct FrameInfo {
        qint64 pts = 0;
        qint64 duration = 0;
        bool isKeyFrame = false;
        QSize size;
        int width = 0;
        int height = 0;
    };

    explicit VideoBuffer(QObject* parent = nullptr);
    ~VideoBuffer() override;

    // ===== 缓冲区操作 =====
    void push(VideoFrame* frame);
    VideoFrame* pop();
    VideoFrame* peek() const;
    bool peekInfo(FrameInfo* info) const;
    VideoFrame* popIfPts(qint64 pts);

    // ===== 缓冲区控制 =====
    void clear();
    void clearNonKeyFrames(); // 清除非关键帧（用于追帧）

    // ===== 状态查询 =====
    int size() const;
    bool isEmpty() const;
    bool isFull() const;
    qint64 bufferedDurationMs() const;
    int capacity() const {
        return m_maxSize;
    }
    void setCapacity(int maxSize) {
        m_maxSize = maxSize;
    }

  signals:
    void bufferFull();
    void bufferEmpty();
    void bufferLevelChanged(int level);

  private:
    void insertSorted(VideoFrame* frame);

  private:
    QQueue<VideoFrame*> m_queue;
    mutable QMutex m_mutex;
    int m_maxSize; // 最大缓冲帧数
};

#endif // VIDEOBUFFER_H
