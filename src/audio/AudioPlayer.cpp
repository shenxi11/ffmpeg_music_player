#include "AudioPlayer.h"
#include <QAudioFormat>
#include <QtGlobal>

AudioPlayer& AudioPlayer::instance()
{
    static AudioPlayer instance;  
    return instance;
}

AudioPlayer::AudioPlayer()
    : QObject(nullptr),  // 单例由静态对象托管，不需要父对象
      m_audioOutput(nullptr),
      m_audioDevice(nullptr),
      m_positionTimer(new QTimer(this)),
      m_buffer(new AudioBuffer(512 * 1024, 64 * 1024 * 1024)),  
      m_threadRunning(false),
      m_isPlaying(false),
      m_isPaused(true),  // 初始进入暂停态，等待会话显式启动
      m_volume(50),
      m_playbackRate(1.0),
      m_currentTimestamp(0),
      m_baseTimestamp(0),
      m_timestampConsumedBytes(0),
      m_playbackStartTimestamp(0),
      m_pausedPosition(0),
      m_sampleRate(44100),
      m_channels(2),
      m_stopRequested(false)
{
    connect(m_positionTimer, &QTimer::timeout, this, &AudioPlayer::onPositionUpdateTimer);
    
    // 预热音频输出设备，减少首次播放延迟
    initAudioOutput();
    
    
    m_threadRunning = true;
    m_playThread = std::thread(&AudioPlayer::playbackThread, this);
    
    qDebug() << "AudioPlayer created in thread:" << QThread::currentThreadId();
    qDebug() << "AudioPlayer: Playback thread started and ready";
    qDebug() << "AudioPlayer: Audio device is HOT and ready (no startup delay)";
}

AudioPlayer::~AudioPlayer()
{
    
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
    m_isPaused = false;  
    
    
    m_cv.notify_one();
    
    
    m_playbackTimer.start();
    m_playbackStartTimestamp = m_currentTimestamp;  
    m_positionTimer->start(100); 
    
    emit playbackStarted();
    qDebug() << "AudioPlayer: Start playback (device already hot, ZERO startup delay)";
    return true;
}

void AudioPlayer::pause()
{
    if (!m_isPlaying || m_isPaused) return;
    
    // 暂停位置优先使用已实际消费的PCM时间戳，避免UI进度跑在声音前面
    const qint64 renderedPosition = getCurrentTimestamp();
    if (renderedPosition > 0) {
        m_pausedPosition = renderedPosition;
    } else {
        const double rate = m_playbackRate.load();
        const qint64 scaledElapsed = static_cast<qint64>(m_playbackTimer.elapsed() * rate);
        m_pausedPosition = m_playbackStartTimestamp + scaledElapsed;
    }
    
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
    m_isPaused = true;  
    m_cv.notify_all();
    
    
    
    
    m_positionTimer->stop();
    
    emit playbackStopped();
    qDebug() << "AudioPlayer: Playback stopped (device and thread still hot)";
}

void AudioPlayer::resetBuffer()
{
    if (m_buffer) {
        m_buffer->clear();
    }
    
    
    {
        std::lock_guard<std::mutex> lock(m_timestampMutex);
        while (!m_timestampQueue.empty()) {
            m_timestampQueue.pop();
        }
        m_currentTimestamp = 0;
        m_baseTimestamp = 0;
        m_timestampConsumedBytes = 0;
        m_playbackStartTimestamp = 0;
        m_pausedPosition = 0;
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
        const QString previousOwner = m_writeOwner;
        const bool isHandoff = !previousOwner.isEmpty() && previousOwner != ownerId;
        if (isHandoff) {
            if (m_buffer) {
                m_buffer->clear();
            }
            std::lock_guard<std::mutex> tsLock(m_timestampMutex);
            while (!m_timestampQueue.empty()) {
                m_timestampQueue.pop();
            }
            m_currentTimestamp = 0;
            m_baseTimestamp = 0;
            m_timestampConsumedBytes = 0;
            m_playbackStartTimestamp = 0;
            m_pausedPosition = 0;
            m_playbackRate = 1.0;
            qDebug() << "AudioPlayer: Owner handoff" << previousOwner << "->" << ownerId
                     << ", cleared shared audio buffer/state and reset playback rate";
        }
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

void AudioPlayer::setPlaybackRate(double rate)
{
    const double clampedRate = qBound(0.5, rate, 2.0);
    const double currentRate = m_playbackRate.load();
    if (qFuzzyCompare(currentRate, clampedRate)) {
        return;
    }

    const qint64 positionBeforeChange = getPlaybackPosition();
    m_playbackRate = clampedRate;

    if (m_isPlaying && !m_isPaused) {
        m_playbackStartTimestamp = positionBeforeChange;
        m_playbackTimer.restart();
    } else if (m_isPlaying && m_isPaused) {
        m_pausedPosition = positionBeforeChange;
    }

    qDebug() << "AudioPlayer: Playback rate changed to" << clampedRate << "x";
}

void AudioPlayer::writeAudioData(const QByteArray& data, qint64 timestampMs, const QString& ownerId)
{
    if (!m_buffer || data.isEmpty()) return;
    if (ownerId.isEmpty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_ownerMutex);
        if (m_writeOwner.isEmpty() || ownerId != m_writeOwner) {
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
    
    // 写入共享环形缓冲区
    m_buffer->write(data.data(), data.size());
    
    
    m_cv.notify_one();
    
    
    {
        std::lock_guard<std::mutex> lock(m_timestampMutex);
        m_timestampQueue.push({timestampMs, data.size()});
    }
    
    
    m_cv.notify_one();
    
    
    int fillLevel = bufferFillLevel();
    emit bufferStatusChanged(fillLevel);
    
    // 预留：低水位告警（当前默认关闭，避免日志噪声）
    // if (fillLevel < 10 && m_isPlaying) {
    //     emit bufferUnderrun();
    //     qDebug() << "AudioPlayer: Buffer underrun, fill level:" << fillLevel << "%";
    // }
}

void AudioPlayer::playbackThread()
{
    qDebug() << "AudioPlayer: Playback thread started:" << QThread::currentThreadId();
    
    bool firstWrite = true;
    const int readSize = 4096; // 每次读取 4KB PCM
    char* readBuffer = new char[readSize];
    int loopCount = 0;
    while (m_threadRunning && !m_stopRequested) {
        {
            std::unique_lock<std::mutex> lock(m_playMutex);
            
            
            m_cv.wait(lock, [this]() {
                return (!m_isPaused && m_buffer->availableBytes() > 0) || m_stopRequested;
            });
            
            if (m_stopRequested) break;
            
            
            if (!m_audioOutput || !m_audioDevice) {
                qDebug() << "AudioPlayer: Audio device not available";
                break;
            }
            
            qint64 bytesFree = m_audioOutput->bytesFree();
            if (bytesFree < readSize * 2) {
                
                continue;
            }
            
            // 从共享缓冲区取出待播放数据
            int bytesRead = m_buffer->read(readBuffer, readSize);
            if (bytesRead <= 0) {
                continue;
            }
            
            if (firstWrite) {
                qDebug() << "AudioPlayer: First audio data write";
                firstWrite = false;
            }
            
            
            qint64 bytesWritten = m_audioDevice->write(readBuffer, bytesRead);
            if (bytesWritten < 0) {
                qDebug() << "AudioPlayer: Error writing audio data:" << m_audioDevice->errorString();
                emit playbackError("Audio write error");
                continue;
            }
            
            // 基于本次真实消费字节推进时间戳，避免 seek/reset 后累计量失真
            updateTimestamp(bytesRead);
        }
        
        
        if (++loopCount % 10 == 0) {
            int fillLevel = bufferFillLevel();
            emit bufferStatusChanged(fillLevel);
        }
        
        
        QThread::msleep(10);
    }
    
    delete[] readBuffer;
    qDebug() << "AudioPlayer: Playback thread finished";
}

void AudioPlayer::updateTimestamp(qint64 consumedBytesDelta)
{
    std::lock_guard<std::mutex> lock(m_timestampMutex);

    if (consumedBytesDelta <= 0) {
        return;
    }

    // 记录“在当前时间戳队列上的已消费字节”，队列清空后会同步归零
    m_timestampConsumedBytes += consumedBytesDelta;

    while (!m_timestampQueue.empty()) {
        auto& front = m_timestampQueue.front();
        if (m_timestampConsumedBytes >= front.dataSize) {
            m_timestampConsumedBytes -= front.dataSize;
            m_currentTimestamp = front.timestamp;
            m_timestampQueue.pop();
            continue;
        }

        qint64 nextTimestamp = front.timestamp;
        if (m_timestampQueue.size() > 1) {
            auto temp = m_timestampQueue;
            temp.pop();
            nextTimestamp = temp.front().timestamp;
        }

        const double progress = static_cast<double>(m_timestampConsumedBytes) /
                                static_cast<double>(qMax(front.dataSize, 1));
        m_currentTimestamp = front.timestamp +
            static_cast<qint64>((nextTimestamp - front.timestamp) * progress);
        break;
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
    
    
    m_audioOutput->setBufferSize(64 * 1024);
    
    
    m_audioDevice = m_audioOutput->start();
    if (!m_audioDevice) {
        qDebug() << "AudioPlayer: Failed to start audio device";
        delete m_audioOutput;
        m_audioOutput = nullptr;
        return false;
    }
    
    
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
    std::lock_guard<std::mutex> lock(m_timestampMutex);
    return m_currentTimestamp;
}
void AudioPlayer::setCurrentTimestamp(qint64 timestampMs)
{
    std::lock_guard<std::mutex> lock(m_timestampMutex);
    m_currentTimestamp = timestampMs;
    
    
    if (m_isPlaying && !m_isPaused) {
        m_playbackTimer.restart();
        m_playbackStartTimestamp = timestampMs;
        qDebug() << "AudioPlayer: Timestamp set to" << timestampMs << "ms, clock resynced";
    }
    
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

void AudioPlayer::ensureBufferCapacity(int targetCapacityBytes)
{
    if (!m_buffer || targetCapacityBytes <= 0) {
        return;
    }
    if (!m_buffer->ensureCapacity(targetCapacityBytes)) {
        qWarning() << "AudioPlayer: ensureBufferCapacity failed, target:" << targetCapacityBytes;
    }
}

qint64 AudioPlayer::getPlaybackPosition() const
{
    if (!m_isPlaying) {
        return 0;
    }

    if (m_isPaused) {
        return m_pausedPosition;
    }

    // Prefer consumed PCM timestamp to avoid progress running ahead during seek/buffering.
    qint64 renderedPosition = getCurrentTimestamp();
    if (renderedPosition > 0) {
        return renderedPosition;
    }

    const double rate = m_playbackRate.load();
    qint64 scaledElapsed = static_cast<qint64>(m_playbackTimer.elapsed() * rate);
    return m_playbackStartTimestamp + scaledElapsed;
}

qint64 AudioPlayer::getSyncPlaybackPosition() const
{
    if (!m_isPlaying) {
        return 0;
    }

    if (m_isPaused) {
        return m_pausedPosition;
    }

    const qint64 renderedPosition = getCurrentTimestamp();
    const double rate = m_playbackRate.load();
    const qint64 estimatedPosition =
        m_playbackStartTimestamp + static_cast<qint64>(m_playbackTimer.elapsed() * rate);

    if (renderedPosition <= 0) {
        return estimatedPosition;
    }

    // 在倍速播放时，渲染时钟（基于设备消费）可能明显慢于估算时钟，
    // 若继续以渲染时钟为主会导致视频长期等待“未来帧”而卡住。
    if (rate > 1.01) {
        const qint64 leadGuardMs = 120;
        if (estimatedPosition > renderedPosition + leadGuardMs) {
            return estimatedPosition - leadGuardMs;
        }
        return qMax(renderedPosition, estimatedPosition);
    }

    // 同步时钟采用融合策略：
    // 1) 渲染时钟优先，避免UI“跑在声音前面”
    // 2) 当渲染时钟明显落后时，适度向估算时钟靠拢，避免视频长期等待而卡住
    if (estimatedPosition <= renderedPosition) {
        return renderedPosition;
    }

    const qint64 drift = estimatedPosition - renderedPosition;
    if (drift <= 120) {
        return renderedPosition;
    }

    const qint64 maxCatchup = (rate > 1.0) ? 1200 : 400;
    return renderedPosition + qMin(drift, maxCatchup);
}

void AudioPlayer::onPositionUpdateTimer()
{
    if (m_isPlaying && !m_isPaused) {
        const double rate = m_playbackRate.load();
        const qint64 elapsedMs = m_playbackTimer.elapsed();
        const qint64 estimatedPosition = m_playbackStartTimestamp + static_cast<qint64>(elapsedMs * rate);
        const qint64 renderedPosition = getCurrentTimestamp();
        const qint64 reportedPosition = renderedPosition > 0 ? renderedPosition : estimatedPosition;

        emit positionChanged(reportedPosition);

        static int debugCount = 0;
        if (++debugCount % 10 == 0) {
            qDebug() << "AudioPlayer: Position update:" << reportedPosition
                     << "ms (rendered:" << renderedPosition
                     << "estimated:" << estimatedPosition
                     << "elapsed:" << elapsedMs
                     << "ms, rate:" << rate << "x)";
        }
    }
}
