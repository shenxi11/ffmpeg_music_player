#include "audio_ringbuffer.h"

int AudioRingBuffer::write(const char* data, int bytes) {
    QMutexLocker locker(&m_mutex);

    // 检查是否需要扩容
    if (bytes > availableSpace()) {
        if (!expandBuffer(bytes)) {
            bytes = availableSpace(); // 扩容失败，写入部分数据
            if (bytes <= 0) return 0;
        }
    }

    // 处理回绕写入
    int tailSpace = m_capacity - m_writePos;
    if (bytes <= tailSpace) {
        memcpy(m_buffer.data() + m_writePos, data, bytes);
        m_writePos += bytes;
    } else {
        memcpy(m_buffer.data() + m_writePos, data, tailSpace);
        memcpy(m_buffer.data(), data + tailSpace, bytes - tailSpace);
        m_writePos = bytes - tailSpace;
    }

    m_dataSize += bytes;
    return bytes;
}
bool AudioRingBuffer::expandBuffer(int requiredBytes) {
    int requiredCapacity = m_dataSize + requiredBytes;
    if (requiredCapacity > m_maxCapacity) return false;

    int newCapacity = m_capacity;
    while (newCapacity < requiredCapacity) {
        newCapacity *= 2;
        if (newCapacity > m_maxCapacity) {
            newCapacity = m_maxCapacity;
            break;
        }
    }

    QByteArray newBuffer(newCapacity, 0);

    // 复制现有数据（处理回绕）
    if (m_dataSize > 0) {
        if (m_readPos < m_writePos) {
            memcpy(newBuffer.data(), m_buffer.data() + m_readPos, m_dataSize);
        } else {
            int firstPart = m_capacity - m_readPos;
            memcpy(newBuffer.data(), m_buffer.data() + m_readPos, firstPart);
            memcpy(newBuffer.data() + firstPart, m_buffer.data(), m_writePos);
        }
    }

    m_buffer = std::move(newBuffer);
    m_capacity = newCapacity;
    m_readPos = 0;
    m_writePos = m_dataSize;
    return true;
}
int AudioRingBuffer::read(char* dest, int bytes) {
    QMutexLocker locker(&m_mutex);

    if (bytes > m_dataSize) bytes = m_dataSize;
    if (bytes <= 0) return 0;

    // 处理回绕读取
    int tailData = m_capacity - m_readPos;
    if (bytes <= tailData) {
        memcpy(dest, m_buffer.data() + m_readPos, bytes);
        m_readPos += bytes;
    } else {
        memcpy(dest, m_buffer.data() + m_readPos, tailData);
        memcpy(dest + tailData, m_buffer.data(), bytes - tailData);
        m_readPos = bytes - tailData;
    }

    m_dataSize -= bytes;
    return bytes;
}
int AudioRingBuffer::availableBytes() const {
    QMutexLocker locker(&m_mutex);
    return m_dataSize;
}
int AudioRingBuffer::availableSpace() const {
    QMutexLocker locker(&m_mutex);
    return m_capacity - m_dataSize;
}
void  AudioRingBuffer::clear() {
    QMutexLocker locker(&m_mutex);
    m_readPos = 0;
    m_writePos = 0;
    m_dataSize = 0;
}
