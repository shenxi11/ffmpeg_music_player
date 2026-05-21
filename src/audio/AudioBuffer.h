#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include "headers.h"

/**
 * @brief 音频环形缓冲区模块
 * 职责：管理音频数据的读写缓冲，支持动态扩容
 */
class AudioBuffer
{
public:
    struct BufferStats {
        int availableBytes = 0;
        int capacityBytes = 0;
        int freeBytes = 0;
        int initialCapacityBytes = 0;
        int maxCapacityBytes = 0;
    };

    explicit AudioBuffer(int initialCapacity = 64 * 1024, int maxCapacity = 10 * 1024 * 1024);
    ~AudioBuffer() = default;

    // 写入数据到缓冲区
    int write(const char* data, int bytes);
    
    // 从缓冲区读取数据
    int read(char* dest, int bytes);
    
    // 获取可读字节数
    int availableBytes() const;
    
    // 获取可写空间
    int availableSpace() const;
    
    // 清空缓冲区
    void clear();
    
    // 清空数据并显式恢复到构造时的初始容量；普通 seek 清空应使用 clear() 保持扩容容量。
    void reset();
    
    // 获取缓冲区容量
    int capacity() const;
    int initialCapacity() const;
    int maxCapacity() const;
    BufferStats stats() const;

    // 确保总容量至少达到目标值；该操作只扩容，不收缩。
    bool ensureCapacity(int targetCapacityBytes);

private:
    int clampedDataSizeLocked() const;
    int availableSpaceLocked() const;
    BufferStats statsLocked() const;
    bool expandBuffer(int requiredBytes);
    
    mutable QMutex m_mutex;
    const int m_initialCapacity;
    int m_capacity;
    const int m_maxCapacity;
    QByteArray m_buffer;
    int m_readPos;
    int m_writePos;
    int m_dataSize;
};

#endif // AUDIOBUFFER_H
