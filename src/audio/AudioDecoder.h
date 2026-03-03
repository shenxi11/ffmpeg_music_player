#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QObject>
#include <QThread>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "headers.h"

/**
 * @brief 音频解码引擎模块
 * 职责：负责音频文件的解封装、解码和重采样
 */
class AudioDecoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder();

    // 初始化解码器
    bool initDecoder(const QString& filePath);
    
    // 控制解码
    void startDecode();
    void pauseDecode();
    void stopDecode();
    void seekTo(qint64 positionMs);
    
    // 状态查询
    bool isDecoding() const { return m_decoding; }
    qint64 duration() const { return m_duration; }
    int currentSeekGeneration() const { return m_seekGeneration.load(); }

    // 元数据查询
    QString extractedTitle() const { return m_extractedTitle; }
    QString extractedArtist() const { return m_extractedArtist; }
    
signals:
    // 解码数据输出
    void decodedData(const QByteArray& data, qint64 timestampMs, int seekGeneration);
    
    // 元数据信息
    void metadataReady(qint64 durationMs, int sampleRate, int channels);

    // 音频标签信息（标题、艺术家）
    void audioTagsReady(const QString& title, const QString& artist);

    // 封面图片
    void albumArtReady(const QString& imagePath);
    
    // 解码状态
    void decodeStarted();
    void decodePaused();
    void decodeStopped();
    void decodeCompleted();
    void decodeError(const QString& error);
    
    // 进度信息
    void progressUpdated(qint64 currentMs);

public slots:
    void decode();  // 解码线程主函数

private:
    static int interruptCallback(void* opaque);
    void setIoDeadlineMs(int timeoutMs);
    void clearIoDeadline();
    void cleanup();
    void extractAlbumArt();
    void extractAudioTags();
    bool openFile(const QString& filePath);
    bool setupDecoder();
    bool setupResampler();
    
    // FFmpeg 组件
    AVFormatContext* m_formatCtx;
    AVCodecContext* m_codecCtx;
    SwrContext* m_swrCtx;
    AVFrame* m_frame;
    AVPacket* m_packet;
    
    // 流信息
    int m_audioStreamIndex;
    QString m_filePath;
    qint64 m_duration;  // 总时长(ms)
    
    // 输出格式
    int m_outSampleRate;
    int m_outChannels;
    AVSampleFormat m_outSampleFormat;
    
    // 线程控制
    std::thread m_decodeThread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_decoding;
    std::atomic<bool> m_paused;
    std::atomic<bool> m_stopRequested;
    
    // 定位控制
    std::atomic<bool> m_seekRequested;
    std::atomic<qint64> m_seekTarget;
    std::atomic<int> m_seekGeneration;
    std::atomic<int> m_appliedSeekGeneration;
    
    // 当前位置
    qint64 m_currentPts;
    
    // 基准时间戳（第一个数据包的PTS）
    qint64 m_firstPts;
    bool m_firstPtsSet;

    // 提取的元数据
    QString m_extractedTitle;
    QString m_extractedArtist;

    std::atomic<qint64> m_ioDeadlineUs;
    std::atomic<bool> m_ioAbortRequested;
    std::atomic<bool> m_networkSource;
    std::atomic<bool> m_waitingFirstPacketAfterSeek;
    std::atomic<bool> m_hlsSource;
    int m_seekEofRetryCount;
};

#endif // AUDIODECODER_H
