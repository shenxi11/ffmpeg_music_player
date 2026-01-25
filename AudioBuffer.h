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
    
    // 重置缓冲区到初始容量（用于seek后优化）
    void reset();
    
    // 获取缓冲区容量
    int capacity() const { return m_capacity; }

private:
    bool expandBuffer(int requiredBytes);
    
    mutable QMutex m_mutex;
    int m_capacity;
    const int m_maxCapacity;
    QByteArray m_buffer;
    int m_readPos;
    int m_writePos;
    int m_dataSize;
};

#endif // AUDIOBUFFER_H
