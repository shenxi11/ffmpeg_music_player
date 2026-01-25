#ifndef AUDIO_RINGBUFFER_H
#define AUDIO_RINGBUFFER_H

#include "headers.h"

class AudioRingBuffer
{
public:
    explicit AudioRingBuffer(int initialCapacity = 4096, int maxCapacity = 10 * 1024 * 1024)
        : m_capacity(initialCapacity),
        m_maxCapacity(maxCapacity),
        m_buffer(initialCapacity, 0),
        m_readPos(0),
        m_writePos(0),
        m_dataSize(0) {}
    int write(const char* data, int bytes);
    int read(char* dest, int bytes);
    int availableBytes() const;
    int availableSpace() const;
    void clear();
private:
    bool expandBuffer(int requiredBytes);
    mutable QMutex m_mutex;
    int m_capacity;
    const int m_maxCapacity;
    QByteArray m_buffer;
    int m_readPos = 0;
    int m_writePos = 0;
    int m_dataSize = 0;
};

#endif // AUDIO_RINGBUFFER_H
