#include "AudioPlayer.h"
#include <QAudioFormat>
#include <QtGlobal>

AudioPlayer& AudioPlayer::instance()
{
    static AudioPlayer instance;  // 闈欐€佸眬閮ㄥ彉閲忥紝绾跨▼瀹夊叏
    return instance;
}

AudioPlayer::AudioPlayer()
    : QObject(nullptr),  // 鍗曚緥涓嶉渶瑕佺埗瀵硅薄
      m_audioOutput(nullptr),
      m_audioDevice(nullptr),
      m_positionTimer(new QTimer(this)),
      m_buffer(new AudioBuffer(512 * 1024, 64 * 1024 * 1024)),  // 鍒濆512KB锛屽簲瀵圭綉缁滄尝鍔?
      m_threadRunning(false),
      m_isPlaying(false),
      m_isPaused(true),  // 鍒濆涓烘殏鍋滅姸鎬?
      m_volume(75),
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
    
    // 鍒濆鍖栭煶棰戣緭鍑?
    initAudioOutput();
    
    // 绔嬪嵆鍚姩鎾斁绾跨▼锛堜繚鎸佽繍琛岋紝閫氳繃鏆傚仠鐘舵€佹帶鍒讹級
    m_threadRunning = true;
    m_playThread = std::thread(&AudioPlayer::playbackThread, this);
    
    qDebug() << "AudioPlayer created in thread:" << QThread::currentThreadId();
    qDebug() << "AudioPlayer: Playback thread started and ready";
    qDebug() << "AudioPlayer: Audio device is HOT and ready (no startup delay)";
}

AudioPlayer::~AudioPlayer()
{
    // 鍋滄鎾斁绾跨▼
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
    m_isPaused = false;  // 鍙栨秷鏆傚仠锛屾挱鏀剧嚎绋嬩細绔嬪嵆寮€濮嬪伐浣?
    
    // 绔嬪嵆鍞ら啋鎾斁绾跨▼锛堢嚎绋嬪凡缁忓湪杩愯锛岄煶棰戣澶囦篃宸茬粡鎵撳紑锛?
    m_cv.notify_one();
    
    // 鍚姩杩涘害鏇存柊瀹氭椂鍣?
    m_playbackTimer.start();
    m_playbackStartTimestamp = m_currentTimestamp;  // 璁板綍鎾斁寮€濮嬫椂鐨勬椂闂存埑
    m_positionTimer->start(100); // 姣?00ms鏇存柊涓€娆¤繘搴︼紙鎻愰珮鍝嶅簲鎬э級
    
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
    
    // 鎭㈠鎾斁鏃堕噸鏂板悓姝ユ椂閽燂紝浣跨敤鏆傚仠鏃朵繚瀛樼殑浣嶇疆
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
    m_isPaused = true;  // 璁剧疆涓烘殏鍋滅姸鎬侊紝涓嶅仠姝㈢嚎绋?
    m_cv.notify_all();
    
    // 涓嶈皟鐢?m_audioOutput->stop()锛屼繚鎸侀煶棰戣澶囩儹鐘舵€?
    // 鎾斁绾跨▼浼氬仠姝㈠線璁惧鍐欐暟鎹紝IODevice浼氳嚜鐒跺仠姝㈣緭鍑?
    
    m_positionTimer->stop();
    
    emit playbackStopped();
    qDebug() << "AudioPlayer: Playback stopped (device and thread still hot)";
}

void AudioPlayer::resetBuffer()
{
    if (m_buffer) {
        m_buffer->clear();
    }
    
    // 娓呯┖鏃堕棿鎴抽槦鍒?
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
    
    // 鍐欏叆鏁版嵁鍒扮紦鍐插尯
    m_buffer->write(data.data(), data.size());
    
    // 鍞ら啋鎾斁绾跨▼
    m_cv.notify_one();
    
    // 璁板綍鏃堕棿鎴?
    {
        std::lock_guard<std::mutex> lock(m_timestampMutex);
        m_timestampQueue.push({timestampMs, data.size()});
    }
    
    // 閫氱煡鎾斁绾跨▼
    m_cv.notify_one();
    
    // 妫€鏌ョ紦鍐插尯鐘舵€?
    int fillLevel = bufferFillLevel();
    emit bufferStatusChanged(fillLevel);
    
    // 缂撳啿鍖洪ゥ楗胯鍛?
    // if (fillLevel < 10 && m_isPlaying) {
    //     emit bufferUnderrun();
    //     qDebug() << "AudioPlayer: Buffer underrun, fill level:" << fillLevel << "%";
    // }
}

void AudioPlayer::playbackThread()
{
    qDebug() << "AudioPlayer: Playback thread started:" << QThread::currentThreadId();
    
    bool firstWrite = true;
    const int readSize = 4096; // 姣忔璇诲彇4KB
    char* readBuffer = new char[readSize];
    int loopCount = 0;
    while (m_threadRunning && !m_stopRequested) {
        {
            std::unique_lock<std::mutex> lock(m_playMutex);
            
            // 绛夊緟鏉′欢锛氭湁鏁版嵁涓旀湭鍋滄涓旀湭鏆傚仠
            m_cv.wait(lock, [this]() {
                return (!m_isPaused && m_buffer->availableBytes() > 0) || m_stopRequested;
            });
            
            if (m_stopRequested) break;
            
            // 妫€鏌ラ煶棰戣澶囨槸鍚︽湁瓒冲绌洪棿
            if (!m_audioOutput || !m_audioDevice) {
                qDebug() << "AudioPlayer: Audio device not available";
                break;
            }
            
            qint64 bytesFree = m_audioOutput->bytesFree();
            if (bytesFree < readSize * 2) {
                // 闊抽缂撳啿鍖哄凡婊★紝绛夊緟
                continue;
            }
            
            // 浠庣紦鍐插尯璇诲彇鏁版嵁
            int bytesRead = m_buffer->read(readBuffer, readSize);
            if (bytesRead <= 0) {
                continue;
            }
            
            if (firstWrite) {
                qDebug() << "AudioPlayer: First audio data write";
                firstWrite = false;
            }
            
            // 鍐欏叆闊抽璁惧
            qint64 bytesWritten = m_audioDevice->write(readBuffer, bytesRead);
            if (bytesWritten < 0) {
                qDebug() << "AudioPlayer: Error writing audio data:" << m_audioDevice->errorString();
                emit playbackError("Audio write error");
                continue;
            }
            
            // 基于本次真实消费字节推进时间戳，避免 seek/reset 后累计量失真
            updateTimestamp(bytesRead);
        }
        
        // 瀹氭湡鍙戦€乥uffer鐘舵€侊紙姣?0娆″惊鐜害100ms锛?
        if (++loopCount % 10 == 0) {
            int fillLevel = bufferFillLevel();
            emit bufferStatusChanged(fillLevel);
        }
        
        // 鐭殏鐫＄湢锛岄伩鍏岰PU鍗犵敤杩囬珮
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
    
    // 璁剧疆缂撳啿鍖哄ぇ灏忥紙64KB锛?
    m_audioOutput->setBufferSize(64 * 1024);
    
    // 鍚姩闊抽杈撳嚭锛堜竴娆℃€у惎鍔紝淇濇寔鐑姸鎬侊級
    m_audioDevice = m_audioOutput->start();
    if (!m_audioDevice) {
        qDebug() << "AudioPlayer: Failed to start audio device";
        delete m_audioOutput;
        m_audioOutput = nullptr;
        return false;
    }
    
    // 璁剧疆鍒濆闊抽噺
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
    
    // 濡傛灉姝ｅ湪鎾斁涓旀湭鏆傚仠锛岄噸鏂板悓姝ユ椂閽?
    if (m_isPlaying && !m_isPaused) {
        m_playbackTimer.restart();
        m_playbackStartTimestamp = timestampMs;
        qDebug() << "AudioPlayer: Timestamp set to" << timestampMs << "ms, clock resynced";
    }
    // 濡傛灉宸叉殏鍋滐紝鏇存柊鏆傚仠浣嶇疆锛堢敤浜巗eek鍦烘櫙锛?
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
