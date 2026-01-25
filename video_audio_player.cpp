#include "video_audio_player.h"
#include <QDebug>
#include <QDateTime>

VideoAudioPlayer::VideoAudioPlayer(QObject *parent)
    : QObject(parent)
    , audioOutput(nullptr)
    , audioDevice(nullptr)
    , playThread(nullptr)
    , threadRunning(false)
    , isPlaying(false)
    , currentAudioPts(0)
    , lastAudioPts(0)
    , lastAudioTime(0)
    , firstAudioPts(-1)
{
    qDebug() << "[VIDEO_AUDIO] Initializing (thread mode)...";
    
    // 设置音频格式：44.1kHz, 16-bit, 立体声
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // 创建音频输出设备
    audioOutput = new QAudioOutput(format, this);
    audioOutput->setBufferSize(192 * 1024); // 192KB缓冲
    audioOutput->setVolume(0.75);
    
    qDebug() << "[VIDEO_AUDIO] QAudioOutput created";
    
    audioDevice = nullptr;
    
    // 启动播放线程
    threadRunning = true;
    playThread = new std::thread(&VideoAudioPlayer::playbackThread, this);
    
    qDebug() << "[VIDEO_AUDIO] Playback thread started";
}

VideoAudioPlayer::~VideoAudioPlayer()
{
    qDebug() << "[VIDEO_AUDIO] Destructor";
    
    // 停止播放
    stop();
    
    // 停止线程
    threadRunning = false;
    bufferCondition.notify_all();
    
    if (playThread && playThread->joinable()) {
        playThread->join();
        delete playThread;
    }
    
    if (audioOutput) {
        audioOutput->stop();
    }
    
    qDebug() << "[VIDEO_AUDIO] Destroyed";
}

void VideoAudioPlayer::receiveAudioData(const QByteArray &data, qint64 pts)
{
    const int MAX_BUFFER_SIZE = 200;
    
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    // 缓冲区满，丢弃最旧的帧
    if (audioBuffer.size() >= MAX_BUFFER_SIZE) {
        audioBuffer.pop();
    }
    
    AudioData frame;
    frame.data = data;
    frame.pts = pts;
    audioBuffer.push(frame);
    
    // 唤醒播放线程
    bufferCondition.notify_one();
}

void VideoAudioPlayer::start()
{
    qDebug() << "[VIDEO_AUDIO] Starting playback...";
    
    if (isPlaying) {
        qDebug() << "[VIDEO_AUDIO] Already playing";
        return;
    }
    
    // 清空旧的音频缓冲区（防止暂停时积累的数据）
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        int cleared = 0;
        while (!audioBuffer.empty()) {
            audioBuffer.pop();
            cleared++;
        }
        if (cleared > 0) {
            qDebug() << "[VIDEO_AUDIO] Cleared" << cleared << "old audio frames on start";
        }
    }
    
    // 重置音频时钟（重要！）
    {
        std::lock_guard<std::mutex> lock(clockMutex);
        currentAudioPts = 0;
        lastAudioPts = 0;
        lastAudioTime = 0;
        firstAudioPts = -1;  // 重置基准
    }
    
    // 启动音频设备
    audioDevice = audioOutput->start();
    if (!audioDevice) {
        qDebug() << "[VIDEO_AUDIO] Failed to start audio device!";
        return;
    }
    
    isPlaying = true;
    bufferCondition.notify_all();  // 唤醒播放线程
    
    qDebug() << "[VIDEO_AUDIO] Started";
}

void VideoAudioPlayer::pause()
{
    qDebug() << "[VIDEO_AUDIO] Pausing...";
    
    if (!isPlaying) {
        qDebug() << "[VIDEO_AUDIO] Not playing";
        return;
    }
    
    isPlaying = false;
    
    // 清空音频缓冲区（关键！）
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        int cleared = 0;
        while (!audioBuffer.empty()) {
            audioBuffer.pop();
            cleared++;
        }
        qDebug() << "[VIDEO_AUDIO] Cleared" << cleared << "audio frames";
    }
    
    // 重置音频时钟（暂停时清零）
    {
        std::lock_guard<std::mutex> lock(clockMutex);
        currentAudioPts = 0;
        lastAudioPts = 0;
        lastAudioTime = 0;
        firstAudioPts = -1;  // 重置基准
    }
    
    qDebug() << "[VIDEO_AUDIO] Paused";
}

void VideoAudioPlayer::stop()
{
    qDebug() << "[VIDEO_AUDIO] Stopping...";
    
    isPlaying = false;
    
    // 重置音频时钟
    {
        std::lock_guard<std::mutex> lock(clockMutex);
        currentAudioPts = 0;
        lastAudioPts = 0;
        lastAudioTime = 0;
        firstAudioPts = -1;  // 重置基准
    }
    
    // 清空缓冲
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        while (!audioBuffer.empty()) {
            audioBuffer.pop();
        }
    }
    bufferCondition.notify_all();
    
    // 停止音频输出
    if (audioOutput) {
        audioOutput->stop();
    }
    
    audioDevice = nullptr;
    
    qDebug() << "[VIDEO_AUDIO] Stopped";
}

void VideoAudioPlayer::setVolume(int volume)
{
    if (audioOutput) {
        audioOutput->setVolume(volume / 100.0);
    }
}

void VideoAudioPlayer::playbackThread()
{
    qDebug() << "[VIDEO_AUDIO] Playback thread started";
    
    while (threadRunning) {
        std::unique_lock<std::mutex> lock(bufferMutex);
        
        // 等待条件：有数据 且 正在播放
        bufferCondition.wait(lock, [this] {
            return !threadRunning || (!audioBuffer.empty() && isPlaying);
        });
        
        if (!threadRunning) {
            break;
        }
        
        // 处理音频数据
        if (isPlaying && audioDevice && !audioBuffer.empty()) {
            AudioData frame = audioBuffer.front();
            audioBuffer.pop();
            
            // 更新音频时钟（使用相对PTS）
            {
                std::lock_guard<std::mutex> clockLock(clockMutex);
                
                // 记录首帧音频PTS作为基准
                if (firstAudioPts < 0) {
                    firstAudioPts = frame.pts;
                    qDebug() << "[AUDIO_CLOCK] First audio PTS:" << firstAudioPts;
                }
                
                // 计算相对PTS（从0开始）
                qint64 relativePts = frame.pts - firstAudioPts;
                
                currentAudioPts = relativePts;
                lastAudioPts = relativePts;
                lastAudioTime = QDateTime::currentMSecsSinceEpoch();
            }
            
            // 解锁后写入，避免阻塞接收线程
            lock.unlock();
            
            // 写入音频设备
            qint64 bytesWritten = audioDevice->write(frame.data);
            
            if (bytesWritten <= 0) {
                // 写入失败，稍微等待
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } else {
            lock.unlock();
            // 没有数据或未播放，稍微等待
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    qDebug() << "[VIDEO_AUDIO] Playback thread stopped";
}

qint64 VideoAudioPlayer::getAudioClock() const
{
    std::lock_guard<std::mutex> lock(clockMutex);
    
    if (!isPlaying || lastAudioPts == 0) {
        return 0;
    }
    
    // 计算当前音频时钟 = 最后相对PTS + 经过的时间
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - lastAudioTime;
    qint64 audioClock = lastAudioPts + elapsed;
    
    static int logCount = 0;
    if (++logCount % 500 == 0) {
        qDebug() << "[AUDIO_CLOCK]" << audioClock << "ms (lastPts:" << lastAudioPts << "elapsed:" << elapsed << ")";
    }
    
    return audioClock;
}
