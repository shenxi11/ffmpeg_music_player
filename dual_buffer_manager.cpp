#include "dual_buffer_manager.h"
#include <QDebug>

DualBufferManager::DualBufferManager(QObject *parent)
    : QObject(parent)
    , m_isPlaying(false)
    , m_isSeeking(false)
    , m_currentPosition(0)
    , m_seekTargetPosition(0)
{
}

DualBufferManager::~DualBufferManager()
{
    stopPlayback();
    clearBuffers();
}

void DualBufferManager::addPCMData(const PCMData& data)
{
    if (m_isSeeking.load()) {
        // 跳转中，数据添加到跳转缓冲区
        QMutexLocker locker(&m_seekMutex);
        if (m_seekBuffer.size() < DUAL_BUFFER_MAX_SIZE) {
            m_seekBuffer.enqueue(data);
            
            // 检查是否到达目标位置附近
            qint64 targetPos = m_seekTargetPosition.load();
            if (abs(data.position - targetPos) < 1000) { // 1秒误差范围内
                emit seekBufferReady();
            }
        }
    } else {
        // 正常播放，数据添加到活动缓冲区
        QMutexLocker locker(&m_activeMutex);
        
        // 简单的缓冲区满处理：丢弃最旧的数据
        while (m_activeBuffer.size() >= DUAL_BUFFER_MAX_SIZE) {
            m_activeBuffer.dequeue();
        }
        
        m_activeBuffer.enqueue(data);
        m_currentPosition.store(data.position);
        
        // 通知消费者数据已可用
        m_dataAvailable.wakeOne();
    }
}

void DualBufferManager::addPCMData(const PCM& pcmData)
{
    // 兼容性重载：将PCM转换为PCMData
    PCMData data;
    data.data = pcmData.data_;
    data.timestamp = pcmData.timeMp;
    data.position = pcmData.timeMp; // 使用时间作为位置
    
    addPCMData(data);
}

PCMData DualBufferManager::getNextPCMData()
{
    QMutexLocker locker(&m_activeMutex);
    
    // 等待数据可用
    while (m_activeBuffer.isEmpty() && m_isPlaying.load()) {
        if (!m_dataAvailable.wait(&m_activeMutex, 100)) {
            // 超时返回空数据，让播放线程继续循环
            return PCMData();
        }
    }
    
    if (!m_activeBuffer.isEmpty()) {
        PCMData data = m_activeBuffer.dequeue();
        m_currentPosition.store(data.position);
        return data;
    }
    
    return PCMData();
}

void DualBufferManager::putBackPCMData(const PCMData& data)
{
    QMutexLocker locker(&m_activeMutex);
    
    // 将数据重新放回队列头部
    m_activeBuffer.prepend(data);
    
    // 通知有数据可用
    m_dataAvailable.wakeOne();
}

bool DualBufferManager::hasData() const
{
    QMutexLocker locker(&m_activeMutex);
    return !m_activeBuffer.isEmpty();
}

int DualBufferManager::getBufferSize() const
{
    QMutexLocker locker(&m_activeMutex);
    return m_activeBuffer.size();
}

bool DualBufferManager::shouldPauseDecoding() const
{
    QMutexLocker locker(&m_activeMutex);
    return m_activeBuffer.size() >= DUAL_BUFFER_HIGH_WATER;
}

void DualBufferManager::waitForBufferSpace()
{
    QMutexLocker locker(&m_activeMutex);
    
    // 如果缓冲区超过高水位，等待直到降到低水位以下
    while (m_activeBuffer.size() >= DUAL_BUFFER_HIGH_WATER) {
        qDebug() << "缓冲区已满 (" << m_activeBuffer.size() << "/" << DUAL_BUFFER_HIGH_WATER 
                 << ")，等待播放线程消费数据...";
        m_spaceAvailable.wait(&m_activeMutex, 1000); // 最多等待1秒
    }
}

void DualBufferManager::prepareSeek(qint64 newPosition)
{
    qDebug() << "准备跳转到位置:" << newPosition;
    
    m_isSeeking.store(true);
    m_seekTargetPosition.store(newPosition);
    
    // 清空跳转缓冲区，准备接收新位置的数据
    clearBuffer(m_seekBuffer, m_seekMutex);
    
    qDebug() << "跳转缓冲区已清空，等待新数据";
}

void DualBufferManager::commitSeek()
{
    if (!m_isSeeking.load()) {
        return;
    }
    
    qDebug() << "提交跳转，交换缓冲区";
    
    // 交换缓冲区：跳转缓冲区变成活动缓冲区
    swapBuffers();
    
    m_isSeeking.store(false);
    m_currentPosition.store(m_seekTargetPosition.load());
    
    qDebug() << "跳转完成，当前位置:" << m_currentPosition.load();
}

void DualBufferManager::cancelSeek()
{
    if (m_isSeeking.load()) {
        qDebug() << "取消跳转";
        
        // 清空跳转缓冲区
        clearBuffer(m_seekBuffer, m_seekMutex);
        m_isSeeking.store(false);
    }
}

void DualBufferManager::swapBuffers()
{
    // 使用两个锁确保原子操作
    QMutexLocker activeLocker(&m_activeMutex);
    QMutexLocker seekLocker(&m_seekMutex);
    
    // 清空当前活动缓冲区
    m_activeBuffer.clear();
    
    // 将跳转缓冲区的数据移动到活动缓冲区
    while (!m_seekBuffer.isEmpty()) {
        m_activeBuffer.enqueue(m_seekBuffer.dequeue());
    }
    
    // 通知等待的线程有新数据
    m_dataAvailable.wakeAll();
    
    qDebug() << "缓冲区交换完成，活动缓冲区大小:" << m_activeBuffer.size();
}

void DualBufferManager::startPlayback()
{
    qDebug() << "开始播放";
    m_isPlaying.store(true);
    m_dataAvailable.wakeAll();
}

void DualBufferManager::pausePlayback()
{
    qDebug() << "暂停播放";
    m_isPlaying.store(false);
}

void DualBufferManager::stopPlayback()
{
    qDebug() << "停止播放";
    m_isPlaying.store(false);
    m_isSeeking.store(false);
    clearBuffers();
}

void DualBufferManager::clearBuffers()
{
    clearBuffer(m_activeBuffer, m_activeMutex);
    clearBuffer(m_seekBuffer, m_seekMutex);
    m_currentPosition.store(0);
    qDebug() << "所有缓冲区已清空";
}

void DualBufferManager::clearBuffer(QQueue<PCMData>& buffer, QMutex& mutex)
{
    QMutexLocker locker(&mutex);
    buffer.clear();
}

int DualBufferManager::getActiveBufferSize() const
{
    QMutexLocker locker(&m_activeMutex);
    return m_activeBuffer.size();
}

int DualBufferManager::getSeekBufferSize() const
{
    QMutexLocker locker(&m_seekMutex);
    return m_seekBuffer.size();
}