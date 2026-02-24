#include "AudioPlayer.h"
#include <QAudioFormat>

AudioPlayer& AudioPlayer::instance()
{
    static AudioPlayer instance;  // 静态局部变量，线程安全
    return instance;
}

AudioPlayer::AudioPlayer()
    : QObject(nullptr),  // 单例不需要父对象
      m_audioOutput(nullptr),
      m_audioDevice(nullptr),
      m_positionTimer(new QTimer(this)),
      m_buffer(new AudioBuffer(512 * 1024, 64 * 1024 * 1024)),  // 初始512KB，应对网络波动
      m_threadRunning(false),
      m_isPlaying(false),
      m_isPaused(true),  // 初始为暂停状态
      m_volume(75),
      m_currentTimestamp(0),
      m_baseTimestamp(0),
      m_playbackStartTimestamp(0),
      m_pausedPosition(0),
      m_sampleRate(44100),
      m_channels(2),
      m_stopRequested(false)
{
    connect(m_positionTimer, &QTimer::timeout, this, &AudioPlayer::onPositionUpdateTimer);
    
    // 初始化音频输出
    initAudioOutput();
    
    // 立即启动播放线程（保持运行，通过暂停状态控制）
    m_threadRunning = true;
    m_playThread = std::thread(&AudioPlayer::playbackThread, this);
    
    qDebug() << "AudioPlayer created in thread:" << QThread::currentThreadId();
    qDebug() << "AudioPlayer: Playback thread started and ready";
    qDebug() << "AudioPlayer: Audio device is HOT and ready (no startup delay)";
}

AudioPlayer::~AudioPlayer()
{
    // 停止播放线程
    m_stopRequested = true;
    m_threadRunning = false;
    m_cv.notify_all();
    
    if (m_playThread.joinable()) {
        m_playThread.join();
    }
    
    cleanupAudioOutput();
    
    if (m_buffer) {
        delete m_buffer;
        m_buffer = nullptr;
    }
    
    qDebug() << "AudioPlayer destroyed";
}

bool AudioPlayer::start()
{
    if (m_isPlaying) return true;
    
    if (!m_audioOutput || !m_audioDevice) {
        qDebug() << "AudioPlayer: Audio output not initialized";
        return false;
    }
    
    m_isPlaying = true;
    m_isPaused = false;  // 取消暂停，播放线程会立即开始工作
    
    // 立即唤醒播放线程（线程已经在运行，音频设备也已经打开）
    m_cv.notify_one();
    
    // 启动进度更新定时器
    m_playbackTimer.start();
    m_playbackStartTimestamp = m_currentTimestamp;  // 记录播放开始时的时间戳
    m_positionTimer->start(100); // 每100ms更新一次进度（提高响应性）
    
    emit playbackStarted();
    qDebug() << "AudioPlayer: Start playback (device already hot, ZERO startup delay)";
    return true;
}

void AudioPlayer::pause()
{
    if (!m_isPlaying || m_isPaused) return;
    
    // 保存当前播放位置
    m_pausedPosition = m_playbackStartTimestamp + m_playbackTimer.elapsed();
    
    m_isPaused = true;
    m_cv.notify_all();
    
    m_positionTimer->stop();
    emit playbackPaused();
    qDebug() << "AudioPlayer: Playback paused at position:" << m_pausedPosition << "ms";
}

void AudioPlayer::resume()
{
    if (!m_isPlaying || !m_isPaused) return;
    
    m_isPaused = false;
    m_cv.notify_all();
    
    // 恢复播放时重新同步时钟，使用暂停时保存的位置
    m_playbackTimer.restart();
    m_playbackStartTimestamp = m_pausedPosition;
    m_positionTimer->start(100);
    
    emit playbackResumed();
    qDebug() << "AudioPlayer: Playback resumed from position:" << m_pausedPosition << "ms";
}

void AudioPlayer::stop()
{
    if (!m_isPlaying) return;
    
    m_isPlaying = false;
    m_isPaused = true;  // 设置为暂停状态，不停止线程
    m_cv.notify_all();
    
    // 不调用 m_audioOutput->stop()，保持音频设备热状态
    // 播放线程会停止往设备写数据，IODevice会自然停止输出
    
    m_positionTimer->stop();
    
    emit playbackStopped();
    qDebug() << "AudioPlayer: Playback stopped (device and thread still hot)";
}

void AudioPlayer::resetBuffer()
{
    if (m_buffer) {
        m_buffer->clear();
    }
    
    // 清空时间戳队列
    {
        std::lock_guard<std::mutex> lock(m_timestampMutex);
        while (!m_timestampQueue.empty()) {
            m_timestampQueue.pop();
        }
        m_currentTimestamp = 0;
        m_baseTimestamp = 0;
        m_playbackStartTimestamp = 0;
    }
    
    qDebug() << "AudioPlayer: Buffer reset (ready for new song)";
}

void AudioPlayer::setWriteOwner(const QString& ownerId)
{
    if (ownerId.isEmpty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_ownerMutex);
    if (m_writeOwner != ownerId) {
        m_writeOwner = ownerId;
        qDebug() << "AudioPlayer: Write owner set to" << m_writeOwner;
    }
}

void AudioPlayer::clearWriteOwner(const QString& ownerId)
{
    std::lock_guard<std::mutex> lock(m_ownerMutex);
    if (ownerId.isEmpty() || m_writeOwner == ownerId) {
        if (!m_writeOwner.isEmpty()) {
            qDebug() << "AudioPlayer: Write owner cleared from" << m_writeOwner;
        }
        m_writeOwner.clear();
    }
}

QString AudioPlayer::writeOwner() const
{
    std::lock_guard<std::mutex> lock(m_ownerMutex);
    return m_writeOwner;
}

void AudioPlayer::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    
    if (m_audioOutput) {
        m_audioOutput->setVolume(m_volume / 100.0);
    }
    
    qDebug() << "AudioPlayer: Volume set to" << m_volume;
}

void AudioPlayer::writeAudioData(const QByteArray& data, qint64 timestampMs, const QString& ownerId)
{
    if (!m_buffer || data.isEmpty()) return;

    {
        std::lock_guard<std::mutex> lock(m_ownerMutex);
        if (!ownerId.isEmpty() && m_writeOwner.isEmpty()) {
            m_writeOwner = ownerId;
            qDebug() << "AudioPlayer: Write owner auto-set to" << m_writeOwner;
        }

        if (!ownerId.isEmpty() && !m_writeOwner.isEmpty() && ownerId != m_writeOwner) {
            static int droppedPacketCount = 0;
            droppedPacketCount++;
            if (droppedPacketCount % 200 == 1) {
                qDebug() << "AudioPlayer: Dropped audio packet from non-owner source"
                         << ownerId << "current owner:" << m_writeOwner;
            }
            return;
        }
    }
    
    static int writeCount = 0;
    if (++writeCount % 100 == 0) {
        qDebug() << "AudioPlayer: Received" << writeCount << "data packets, size:" << data.size() 
                 << "buffer level:" << m_buffer->availableBytes();
    }
    
    // 写入数据到缓冲区
    m_buffer->write(data.data(), data.size());
    
    // 唤醒播放线程
    m_cv.notify_one();
    
    // 记录时间戳
    {
        std::lock_guard<std::mutex> lock(m_timestampMutex);
        m_timestampQueue.push({timestampMs, data.size()});
    }
    
    // 通知播放线程
    m_cv.notify_one();
    
    // 检查缓冲区状态
    int fillLevel = bufferFillLevel();
    emit bufferStatusChanged(fillLevel);
    
    // 缓冲区饥饿警告
    // if (fillLevel < 10 && m_isPlaying) {
    //     emit bufferUnderrun();
    //     qDebug() << "AudioPlayer: Buffer underrun, fill level:" << fillLevel << "%";
    // }
}

void AudioPlayer::playbackThread()
{
    qDebug() << "AudioPlayer: Playback thread started:" << QThread::currentThreadId();
    
    bool firstWrite = true;
    const int readSize = 4096; // 每次读取4KB
    char* readBuffer = new char[readSize];
    int loopCount = 0;
    qint64 totalBytesConsumed = 0;  // 累积消耗的字节数
    
    while (m_threadRunning && !m_stopRequested) {
        {
            std::unique_lock<std::mutex> lock(m_playMutex);
            
            // 等待条件：有数据且未停止且未暂停
            m_cv.wait(lock, [this]() {
                return (!m_isPaused && m_buffer->availableBytes() > 0) || m_stopRequested;
            });
            
            if (m_stopRequested) break;
            
            // 检查音频设备是否有足够空间
            if (!m_audioOutput || !m_audioDevice) {
                qDebug() << "AudioPlayer: Audio device not available";
                break;
            }
            
            qint64 bytesFree = m_audioOutput->bytesFree();
            if (bytesFree < readSize * 2) {
                // 音频缓冲区已满，等待
                continue;
            }
            
            // 从缓冲区读取数据
            int bytesRead = m_buffer->read(readBuffer, readSize);
            if (bytesRead <= 0) {
                continue;
            }
            
            if (firstWrite) {
                qDebug() << "AudioPlayer: First audio data write";
                firstWrite = false;
            }
            
            // 写入音频设备
            qint64 bytesWritten = m_audioDevice->write(readBuffer, bytesRead);
            if (bytesWritten < 0) {
                qDebug() << "AudioPlayer: Error writing audio data:" << m_audioDevice->errorString();
                emit playbackError("Audio write error");
                continue;
            }
            
            // 累积消耗字节数并更新时间戳
            totalBytesConsumed += bytesRead;  // 使用从buffer读取的字节数，不是写入设备的
            updateTimestamp(totalBytesConsumed);
        }
        
        // 定期发送buffer状态（每10次循环约100ms）
        if (++loopCount % 10 == 0) {
            int fillLevel = bufferFillLevel();
            emit bufferStatusChanged(fillLevel);
        }
        
        // 短暂睡眠，避免CPU占用过高
        QThread::msleep(10);
    }
    
    delete[] readBuffer;
    qDebug() << "AudioPlayer: Playback thread finished";
}

void AudioPlayer::updateTimestamp(qint64 totalBytesConsumed)
{
    std::lock_guard<std::mutex> lock(m_timestampMutex);
    
    qint64 bytesProcessed = 0;
    
    // 从时间戳队列中计算当前位置
    while (!m_timestampQueue.empty()) {
        auto& front = m_timestampQueue.front();
        
        if (totalBytesConsumed >= bytesProcessed + front.dataSize) {
            // 完整消费了这个数据块
            m_currentTimestamp = front.timestamp;
            bytesProcessed += front.dataSize;
            m_timestampQueue.pop();
        } else {
            // 部分消费，使用线性插值计算精确时间戳
            qint64 bytesIntoPacket = totalBytesConsumed - bytesProcessed;
            
            // 获取下一个数据包的时间戳用于插值
            qint64 nextTimestamp = front.timestamp;
            if (m_timestampQueue.size() > 1) {
                // 有下一个包，使用两个包之间插值
                auto temp = m_timestampQueue;
                temp.pop();
                nextTimestamp = temp.front().timestamp;
                
                // 线性插值: currentTime = timestamp + (nextTimestamp - timestamp) * progress
                double progress = static_cast<double>(bytesIntoPacket) / front.dataSize;
                m_currentTimestamp = front.timestamp + 
                    static_cast<qint64>((nextTimestamp - front.timestamp) * progress);
            } else {
                // 只有一个包，使用当前包的时间戳
                m_currentTimestamp = front.timestamp;
            }
            break;
        }
    }
}

bool AudioPlayer::initAudioOutput()
{
    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
    format.setSampleSize(16); // 16-bit
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    
    m_audioOutput = new QAudioOutput(format, this);
    if (!m_audioOutput) {
        qDebug() << "AudioPlayer: Failed to create QAudioOutput";
        return false;
    }
    
    // 设置缓冲区大小（64KB）
    m_audioOutput->setBufferSize(64 * 1024);
    
    // 启动音频输出（一次性启动，保持热状态）
    m_audioDevice = m_audioOutput->start();
    if (!m_audioDevice) {
        qDebug() << "AudioPlayer: Failed to start audio device";
        delete m_audioOutput;
        m_audioOutput = nullptr;
        return false;
    }
    
    // 设置初始音量
    m_audioOutput->setVolume(m_volume / 100.0);
    
    qDebug() << "AudioPlayer: Audio device OPENED and ready (hot state)";
    qDebug() << "  Sample Rate:" << m_sampleRate;
    qDebug() << "  Channels:" << m_channels;
    qDebug() << "  Buffer Size:" << m_audioOutput->bufferSize();
    qDebug() << "  Device will remain open until destruction";
    
    return true;
}

void AudioPlayer::cleanupAudioOutput()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        m_audioOutput->reset();
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
    
    m_audioDevice = nullptr;
    
    qDebug() << "AudioPlayer: Audio output cleaned up";
}

qint64 AudioPlayer::getCurrentTimestamp() const
{
    return m_currentTimestamp;
}

void AudioPlayer::setCurrentTimestamp(qint64 timestampMs)
{
    std::lock_guard<std::mutex> lock(m_timestampMutex);
    m_currentTimestamp = timestampMs;
    
    // 如果正在播放且未暂停，重新同步时钟
    if (m_isPlaying && !m_isPaused) {
        m_playbackTimer.restart();
        m_playbackStartTimestamp = timestampMs;
        qDebug() << "AudioPlayer: Timestamp set to" << timestampMs << "ms, clock resynced";
    }
    // 如果已暂停，更新暂停位置（用于seek场景）
    else if (m_isPlaying && m_isPaused) {
        m_pausedPosition = timestampMs;
        qDebug() << "AudioPlayer: Paused position updated to" << timestampMs << "ms (for seek)";
    }
}

int AudioPlayer::bufferFillLevel() const
{
    if (!m_buffer) return 0;
    
    int available = m_buffer->availableBytes();
    int capacity = m_buffer->capacity();
    
    return capacity > 0 ? (available * 100 / capacity) : 0;
}

qint64 AudioPlayer::getPlaybackPosition() const
{
    if (!m_isPlaying) {
        return 0;
    }
    
    // 如果暂停，返回暂停时保存的位置
    if (m_isPaused) {
        return m_pausedPosition;
    }
    
    qint64 elapsedMs = m_playbackTimer.elapsed();
    return m_playbackStartTimestamp + elapsedMs;
}

void AudioPlayer::onPositionUpdateTimer()
{
    if (m_isPlaying && !m_isPaused) {
        // 使用系统时钟计算播放位置（保证均匀更新）
        qint64 elapsedMs = m_playbackTimer.elapsed();
        qint64 estimatedPosition = m_playbackStartTimestamp + elapsedMs;
        
        emit positionChanged(estimatedPosition);
        
        // 调试：每10次（约1秒）打印一次位置
        static int debugCount = 0;
        if (++debugCount % 10 == 0) {
            qDebug() << "AudioPlayer: Position update:" << estimatedPosition << "ms (elapsed:" << elapsedMs << "ms)";
        }
    }
}
