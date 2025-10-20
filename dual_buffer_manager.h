#ifndef DUAL_BUFFER_MANAGER_H
#define DUAL_BUFFER_MANAGER_H

#include <QObject>
#include <QQueue>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include "headers.h" // 包含PCM结构体定义

// 缓冲区大小配置
#define DUAL_BUFFER_MAX_SIZE 2048  // 最大缓冲区大小（约30秒数据）
#define DUAL_BUFFER_MIN_SIZE 256   // 最小缓冲区大小（约4秒数据）
#define DUAL_BUFFER_HIGH_WATER 1536 // 高水位（停止解码）
#define DUAL_BUFFER_LOW_WATER 512   // 低水位（恢复解码）

// PCM数据结构
struct PCMData {
    QByteArray data;
    qint64 timestamp;
    qint64 position;
    
    PCMData() : timestamp(0), position(0) {}
    PCMData(const QByteArray& d, qint64 ts, qint64 pos) 
        : data(d), timestamp(ts), position(pos) {}
};

/**
 * 双缓冲音频管理器
 * 使用双缓冲策略，实现无缝的音频跳转和播放
 */
class DualBufferManager : public QObject
{
    Q_OBJECT

public:
    explicit DualBufferManager(QObject *parent = nullptr);
    ~DualBufferManager();
    
    // 缓冲区管理
    void addPCMData(const PCMData& data);
    void addPCMData(const PCM& pcmData); // 兼容性重载
    PCMData getNextPCMData();
    void putBackPCMData(const PCMData& data); // 将数据重新放回缓冲区头部
    bool hasData() const;
    int getBufferSize() const; // 获取当前缓冲区大小，用于调试
    
    // 跳转控制
    void prepareSeek(qint64 newPosition);
    void commitSeek();
    void cancelSeek();
    
    // 播放控制
    void startPlayback();
    void pausePlayback();
    void stopPlayback();
    void clearBuffers();
    
    // 状态查询
    bool isPlaying() const { return m_isPlaying.load(); }
    bool isSeeking() const { return m_isSeeking.load(); }
    qint64 getCurrentPosition() const { return m_currentPosition.load(); }
    
    // 背压控制
    bool shouldPauseDecoding() const; // 检查是否应该暂停解码
    void waitForBufferSpace(); // 等待缓冲区有空间
    
    // 缓冲区统计
    int getActiveBufferSize() const;
    int getSeekBufferSize() const;

signals:
    void bufferUnderrun();
    void seekBufferReady();
    void positionUpdated(qint64 position);
    void decodingResumeRequested(); // 新信号：请求恢复解码

private:
    // 双缓冲区
    QQueue<PCMData> m_activeBuffer;    // 当前播放缓冲区
    QQueue<PCMData> m_seekBuffer;      // 跳转预备缓冲区
    
    // 线程同步
    mutable QMutex m_activeMutex;
    mutable QMutex m_seekMutex;
    QWaitCondition m_dataAvailable;
    QWaitCondition m_spaceAvailable; // 新增：等待缓冲区有空间
    
    // 状态标志
    std::atomic<bool> m_isPlaying;
    std::atomic<bool> m_isSeeking;
    std::atomic<qint64> m_currentPosition;
    std::atomic<qint64> m_seekTargetPosition;
    
    // 缓冲区配置使用宏定义
    
    void swapBuffers();
    void clearBuffer(QQueue<PCMData>& buffer, QMutex& mutex);
};

#endif // DUAL_BUFFER_MANAGER_H