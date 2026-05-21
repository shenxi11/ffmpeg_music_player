#include "AudioBuffer.h"

AudioBuffer::AudioBuffer(int initialCapacity, int maxCapacity)
    : m_initialCapacity(qMax(1, initialCapacity)),
      m_capacity(m_initialCapacity),
      m_maxCapacity(qMax(m_initialCapacity, maxCapacity)),
      m_buffer(m_initialCapacity, 0),
      m_readPos(0),
      m_writePos(0),
      m_dataSize(0)
{
}

int AudioBuffer::write(const char* data, int bytes)
{
    QMutexLocker locker(&m_mutex);
    
    if (bytes <= 0) {
        return 0;
    }
    
    // 检查是否需要扩容
    if (availableSpaceLocked() < bytes) {
        if (!expandBuffer(bytes)) {
            qWarning() << "AudioBuffer: Failed to expand buffer"
                       << "requestedBytes=" << bytes << "availableBytes=" << m_dataSize
                       << "capacityBytes=" << m_capacity << "freeBytes=" << availableSpaceLocked()
                       << "maxCapacityBytes=" << m_maxCapacity;
            return 0;
        }
    }
    
    int bytesWritten = 0;
    
    // 分段写入（处理环形边界）
    if (m_writePos + bytes <= m_capacity) {
        // 一次性写入
        memcpy(m_buffer.data() + m_writePos, data, bytes);
        m_writePos = (m_writePos + bytes) % m_capacity;
        bytesWritten = bytes;
    } else {
        // 分两段写入
        int firstPart = m_capacity - m_writePos;
        memcpy(m_buffer.data() + m_writePos, data, firstPart);
        memcpy(m_buffer.data(), data + firstPart, bytes - firstPart);
        m_writePos = bytes - firstPart;
        bytesWritten = bytes;
    }
    
    m_dataSize += bytesWritten;
    return bytesWritten;
}

int AudioBuffer::read(char* dest, int bytes)
{
    QMutexLocker locker(&m_mutex);
    
    if (bytes <= 0 || m_dataSize == 0) {
        return 0;
    }
    
    int bytesToRead = qMin(bytes, clampedDataSizeLocked());
    int bytesRead = 0;
    
    // 分段读取（处理环形边界）
    if (m_readPos + bytesToRead <= m_capacity) {
        // 一次性读取
        memcpy(dest, m_buffer.data() + m_readPos, bytesToRead);
        m_readPos = (m_readPos + bytesToRead) % m_capacity;
        bytesRead = bytesToRead;
    } else {
        // 分两段读取
        int firstPart = m_capacity - m_readPos;
        memcpy(dest, m_buffer.data() + m_readPos, firstPart);
        memcpy(dest + firstPart, m_buffer.data(), bytesToRead - firstPart);
        m_readPos = bytesToRead - firstPart;
        bytesRead = bytesToRead;
    }
    
    m_dataSize -= bytesRead;
    return bytesRead;
}

int AudioBuffer::availableBytes() const
{
    QMutexLocker locker(&m_mutex);
    return clampedDataSizeLocked();
}

int AudioBuffer::availableSpace() const
{
    QMutexLocker locker(&m_mutex);
    return availableSpaceLocked();
}

void AudioBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    m_readPos = 0;
    m_writePos = 0;
    m_dataSize = 0;
}

void AudioBuffer::reset()
{
    QMutexLocker locker(&m_mutex);
    m_readPos = 0;
    m_writePos = 0;
    m_dataSize = 0;
    m_capacity = m_initialCapacity;
    m_buffer.resize(m_capacity);
    qDebug() << "AudioBuffer: Reset to initial capacity"
             << "capacityBytes=" << m_capacity << "maxCapacityBytes=" << m_maxCapacity;
}

bool AudioBuffer::ensureCapacity(int targetCapacityBytes)
{
    if (targetCapacityBytes <= 0) {
        return true;
    }

    QMutexLocker locker(&m_mutex);
    if (targetCapacityBytes <= m_capacity) {
        return true;
    }

    const int requiredBytes = qMax(0, targetCapacityBytes - m_dataSize);
    const bool ok = expandBuffer(requiredBytes);
    if (ok) {
        qDebug() << "AudioBuffer: Ensured capacity"
                 << "targetCapacityBytes=" << targetCapacityBytes
                 << "capacityBytes=" << m_capacity
                 << "availableBytes=" << clampedDataSizeLocked()
                 << "freeBytes=" << availableSpaceLocked()
                 << "maxCapacityBytes=" << m_maxCapacity;
    }
    return ok;
}

int AudioBuffer::capacity() const
{
    QMutexLocker locker(&m_mutex);
    return m_capacity;
}

int AudioBuffer::initialCapacity() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialCapacity;
}

int AudioBuffer::maxCapacity() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxCapacity;
}

AudioBuffer::BufferStats AudioBuffer::stats() const
{
    QMutexLocker locker(&m_mutex);
    return statsLocked();
}

int AudioBuffer::clampedDataSizeLocked() const
{
    return qBound(0, m_dataSize, m_capacity);
}

int AudioBuffer::availableSpaceLocked() const
{
    return qMax(0, m_capacity - clampedDataSizeLocked());
}

AudioBuffer::BufferStats AudioBuffer::statsLocked() const
{
    const int available = clampedDataSizeLocked();
    return {available, m_capacity, qMax(0, m_capacity - available), m_initialCapacity,
            m_maxCapacity};
}

bool AudioBuffer::expandBuffer(int requiredBytes)
{
    if (requiredBytes <= 0) {
        return true;
    }

    int newCapacity = m_capacity;
    while (newCapacity - m_dataSize < requiredBytes) {
        if (newCapacity > m_maxCapacity / 2) {
            newCapacity = m_maxCapacity;
        } else {
            newCapacity *= 2;
        }
        if (newCapacity > m_maxCapacity) {
            qWarning() << "AudioBuffer: Cannot expand beyond max capacity" << m_maxCapacity;
            return false;
        }
        if (newCapacity - m_dataSize < requiredBytes && newCapacity == m_maxCapacity) {
            qWarning() << "AudioBuffer: Cannot expand enough"
                       << "requiredBytes=" << requiredBytes << "availableBytes=" << m_dataSize
                       << "capacityBytes=" << m_capacity << "maxCapacityBytes=" << m_maxCapacity;
            return false;
        }
    }
    
    if (newCapacity == m_capacity) {
        return true;
    }
    
    // 重新分配缓冲区
    QByteArray newBuffer(newCapacity, 0);
    
    // 复制现有数据
    if (m_dataSize > 0) {
        if (m_readPos < m_writePos) {
            // 数据连续
            memcpy(newBuffer.data(), m_buffer.data() + m_readPos, m_dataSize);
        } else {
            // 数据分段
            int firstPart = m_capacity - m_readPos;
            memcpy(newBuffer.data(), m_buffer.data() + m_readPos, firstPart);
            memcpy(newBuffer.data() + firstPart, m_buffer.data(), m_dataSize - firstPart);
        }
    }
    
    m_buffer = newBuffer;
    m_capacity = newCapacity;
    m_readPos = 0;
    m_writePos = m_dataSize;
    
    qDebug() << "AudioBuffer: Expanded"
             << "capacityBytes=" << newCapacity << "availableBytes=" << clampedDataSizeLocked()
             << "freeBytes=" << availableSpaceLocked()
             << "maxCapacityBytes=" << m_maxCapacity;
    return true;
}
