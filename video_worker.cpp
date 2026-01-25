#include "video_worker.h"
#include <QDebug>
#include <QDateTime>

// 音视频同步常量
const qint64 AV_SYNC_THRESHOLD_MIN = 10;   // 最小同步阈值 10ms
const qint64 AV_SYNC_THRESHOLD_MAX = 100;  // 最大同步阈值 100ms
const qint64 AV_SYNC_FRAMEDUP_THRESHOLD = 100;  // 重复帧阈值
const qint64 AV_NOSYNC_THRESHOLD = 10000;  // 失步阈值 10秒
#include <QDateTime>

VideoWorker::VideoWorker(QObject *parent)
    : QObject(parent)
    , videoDecoder(nullptr)
    , decoderThread(nullptr)
    , audioPlayer(nullptr)
    , videoThread(nullptr)
    , videoThreadRunning(false)
    , startPlayTime(0)
    , firstVideoPts(-1)
    , isPlaying(false)
    , videoDuration(0)
    , videoFPS(0.0)
{
    qDebug() << "[VIDEO_WORKER] Initializing (thread mode)...";
    
    // 创建视频解码器线程
    decoderThread = new QThread(this);
    videoDecoder = new VideoDecoder();
    videoDecoder->moveToThread(decoderThread);
    decoderThread->start();
    
    // 创建音频播放器（在主线程）
    audioPlayer = new VideoAudioPlayer(this);
    
    // 启动视频播放线程
    videoThreadRunning = true;
    videoThread = new std::thread(&VideoWorker::videoPlaybackThread, this);
    
    // 连接解码器信号
    connect(videoDecoder, &VideoDecoder::videoOpened,
            this, &VideoWorker::onVideoOpened);
    connect(videoDecoder, &VideoDecoder::videoFrameReady,
            this, [this](QImage frame, qint64 pts) {
                std::lock_guard<std::mutex> lock(videoMutex);
                
                const int MAX_VIDEO_BUFFER = 60;  // 最大60帧
                if (videoBuffer.size() >= MAX_VIDEO_BUFFER) {
                    videoBuffer.pop();  // 丢弃最旧的帧
                }
                
                VideoFrame vf;
                vf.image = frame;
                vf.pts = pts;
                videoBuffer.push(vf);

                videoCondition.notify_one();  // 唤醒播放线程
            });
    connect(videoDecoder, &VideoDecoder::audioDataReady,
            audioPlayer, &VideoAudioPlayer::receiveAudioData);
    connect(videoDecoder, &VideoDecoder::decodingFinished,
            this, &VideoWorker::onDecodingFinished);
    connect(videoDecoder, &VideoDecoder::errorOccurred,
            this, &VideoWorker::onErrorOccurred);
    
    qDebug() << "[VIDEO_WORKER] Initialized successfully";
}

VideoWorker::~VideoWorker()
{
    qDebug() << "[VIDEO_WORKER] Destructor";
    
    stop();
    
    // 停止视频播放线程
    videoThreadRunning = false;
    videoCondition.notify_all();
    
    if (videoThread && videoThread->joinable()) {
        videoThread->join();
        delete videoThread;
    }
    
    // 停止解码线程
    if (decoderThread) {
        decoderThread->quit();
        decoderThread->wait(3000);
    }
}

void VideoWorker::openVideo(const QString& filePath)
{
    qDebug() << "[VIDEO_WORKER] Opening video:" << filePath;
    
    // 停止当前播放
    if (isPlaying) {
        stop();
    }
    
    currentVideoPath = filePath;
    
    // 调用解码器打开视频
    QMetaObject::invokeMethod(videoDecoder, "openVideo",
                             Qt::QueuedConnection,
                             Q_ARG(QString, filePath));
}

void VideoWorker::play()
{
    qDebug() << "[VIDEO_WORKER] Play (thread mode)";
    
    if (isPlaying) {
        qDebug() << "[VIDEO_WORKER] Already playing";
        return;
    }
    
    // 清空所有缓冲区（防止暂停时积累的数据）
    {
        std::lock_guard<std::mutex> lock(videoMutex);
        int cleared = 0;
        while (!videoBuffer.empty()) {
            videoBuffer.pop();
            cleared++;
        }
        if (cleared > 0) {
            qDebug() << "[VIDEO_WORKER] Cleared" << cleared << "old video frames on play";
        }
    }
    
    // 重置所有时钟（关键！）
    startPlayTime = QDateTime::currentMSecsSinceEpoch();
    firstVideoPts = -1;  // 重新建立基准
    
    qDebug() << "[VIDEO_WORKER] Reset clocks, startPlayTime:" << startPlayTime;
    
    // 启动音频播放（会重置音频时钟并清空音频缓冲）
    audioPlayer->start();
    
    // 唤醒视频播放线程
    isPlaying = true;
    videoCondition.notify_all();
    
    // 启动解码
    QMetaObject::invokeMethod(videoDecoder, "startDecoding", Qt::QueuedConnection);
    
    emit playbackStarted();
}

void VideoWorker::pause()
{
    qDebug() << "[VIDEO_WORKER] Pause";
    
    if (!isPlaying) {
        qDebug() << "[VIDEO_WORKER] Not playing";
        return;
    }
    
    isPlaying = false;
    
    // 停止解码器（关键！）
    QMetaObject::invokeMethod(videoDecoder, "pauseDecoding", Qt::QueuedConnection);
    
    // 清空视频缓冲区
    {
        std::lock_guard<std::mutex> lock(videoMutex);
        while (!videoBuffer.empty()) {
            videoBuffer.pop();
        }
        qDebug() << "[VIDEO_WORKER] Video buffer cleared";
    }
    
    // 暂停音频（会清空音频缓冲）
    audioPlayer->pause();
    
    emit playbackPaused();
}

void VideoWorker::stop()
{
    qDebug() << "[VIDEO_WORKER] Stop";
    
    isPlaying = false;
    
    // 清空视频缓冲
    {
        std::lock_guard<std::mutex> lock(videoMutex);
        while (!videoBuffer.empty()) {
            videoBuffer.pop();
        }
    }
    videoCondition.notify_all();
    
    // 停止解码
    QMetaObject::invokeMethod(videoDecoder, "stopDecoding", Qt::QueuedConnection);
    
    // 停止音频
    audioPlayer->stop();
    
    isPlaying = false;
    emit playbackStopped();
}

void VideoWorker::seek(qint64 positionMs)
{
    qDebug() << "[VIDEO_WORKER] Seek to" << positionMs << "ms";
    
    // 清空视频缓冲
    {
        std::lock_guard<std::mutex> lock(videoMutex);
        while (!videoBuffer.empty()) {
            videoBuffer.pop();
        }
    }
    
    // 重置时间基准
    startPlayTime = QDateTime::currentMSecsSinceEpoch();
    firstVideoPts = -1;
    
    // 清空音频
    audioPlayer->stop();
    
    // 执行seek
    QMetaObject::invokeMethod(videoDecoder, "seek",
                             Qt::QueuedConnection,
                             Q_ARG(qint64, positionMs));
    
    // 如果正在播放，重新启动
    if (isPlaying) {
        audioPlayer->start();
    }
}

void VideoWorker::setVolume(int volume)
{
    audioPlayer->setVolume(volume);
}

void VideoWorker::onVideoOpened(int width, int height, double fps, qint64 durationMs)
{
    qDebug() << "[VIDEO_WORKER] Video opened -"
             << "Size:" << width << "x" << height
             << "FPS:" << fps
             << "Duration:" << durationMs << "ms";
    
    videoDuration = durationMs;
    videoFPS = fps;
    
    // 重置基准
    firstVideoPts = -1;
    
    // 准备音频播放器
    audioPlayer->stop();
    
    // 转发信号
    emit videoOpened(width, height, fps, durationMs);
}

void VideoWorker::onDecodingFinished()
{
    qDebug() << "[VIDEO_WORKER] Decoding finished";
    
    isPlaying = false;
    audioPlayer->stop();
    
    emit playbackFinished();
}

void VideoWorker::onErrorOccurred(QString error)
{
    qDebug() << "[VIDEO_WORKER] Error:" << error;
    
    isPlaying = false;
    audioPlayer->stop();
    
    emit errorOccurred(error);
}

// 视频播放线程
void VideoWorker::videoPlaybackThread()
{
    qDebug() << "[VIDEO_WORKER] Video playback thread started";
    
    while (videoThreadRunning) {
        std::unique_lock<std::mutex> lock(videoMutex);
        
        // 等待条件：有数据 且 正在播放
        videoCondition.wait(lock, [this] {
            return !videoThreadRunning || (!videoBuffer.empty() && isPlaying);
        });
        
        if (!videoThreadRunning) {
            break;
        }
        
        if (!isPlaying || videoBuffer.empty()) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // 获取队首帧
        VideoFrame frame = videoBuffer.front();
        
        // 第一帧，记录基准PTS
        if (firstVideoPts < 0) {
            firstVideoPts = frame.pts;
            qDebug() << "[VIDEO_WORKER] First video frame PTS:" << firstVideoPts;
        }
        
        // 计算相对PTS（从0开始）
        qint64 videoPts = frame.pts - firstVideoPts;
        
        // ========== 以音频为基准的同步策略 ==========
        
        // 获取当前音频时钟
        qint64 audioClock = audioPlayer->getAudioClock();
        
        // 如果音频还没开始，等待
        if (audioClock == 0) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // 计算音视频时间差（视频PTS - 音频时钟）
        qint64 diff = videoPts - audioClock;
        
        // 同步阈值动态调整（根据差异大小）
        qint64 sync_threshold = qMax(AV_SYNC_THRESHOLD_MIN, qMin(AV_SYNC_THRESHOLD_MAX, qAbs(diff)));
        
        if (qAbs(diff) < AV_NOSYNC_THRESHOLD) {
            // 在同步范围内
            
            if (diff > sync_threshold) {
                // 视频超前音频，需要等待
                lock.unlock();
                qint64 delay = diff - sync_threshold;
                if (delay > 500) delay = 500;  // 最多等待500ms
                
                static int waitCount = 0;
                if (++waitCount % 100 == 0) {
                    qDebug() << "[VIDEO_SYNC] Video ahead, waiting" << delay << "ms, diff:" << diff;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                continue;  // 不显示，继续等待
                
            } else if (diff < -sync_threshold) {
                // 视频滞后音频，需要加速（丢帧）
                
                if (diff < -AV_SYNC_FRAMEDUP_THRESHOLD) {
                    // 滞后过多，直接丢弃这一帧
                    videoBuffer.pop();
                    lock.unlock();
                    
                    static int dropCount = 0;
                    if (++dropCount % 10 == 0) {
                        qDebug() << "[VIDEO_SYNC] Video lagging, dropped frame, diff:" << diff << "total drops:" << dropCount;
                    }
                    continue;  // 丢帧，取下一帧
                } else {
                    // 轻微滞后，正常显示但不等待
                    static int catchupCount = 0;
                    if (++catchupCount % 100 == 0) {
                        qDebug() << "[VIDEO_SYNC] Video slightly behind, catching up, diff:" << diff;
                    }
                }
            } else {
                // 差异在阈值内，正常显示
                static int syncCount = 0;
                if (++syncCount % 500 == 0) {
                    qDebug() << "[VIDEO_SYNC] In sync, diff:" << diff << "ms";
                }
            }
        } else {
            // 失步过多，但不频繁重置（防抖动）
            static qint64 lastResetTime = 0;
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            
            if (now - lastResetTime > 1000) {  // 最快1秒重置一次
                qDebug() << "[VIDEO_SYNC] Major desync (diff:" << diff << "ms), resetting clocks...";
                
                // 重置两个时钟
                firstVideoPts = frame.pts;  // 重新设置视频基准
                startPlayTime = QDateTime::currentMSecsSinceEpoch();
                lastResetTime = now;
            }
        }
        
        // 显示该帧
        videoBuffer.pop();
        lock.unlock();
        
        // 发送信号（跨线程，Qt会自动排队）
        emit videoFrameReady(frame.image, frame.pts);
        emit playbackProgress(videoPts, videoDuration);
    }
    
    qDebug() << "[VIDEO_WORKER] Video playback thread stopped";
}
