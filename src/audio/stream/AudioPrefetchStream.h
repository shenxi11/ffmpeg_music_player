#ifndef AUDIOPREFETCHSTREAM_H
#define AUDIOPREFETCHSTREAM_H

#include "AudioCurlStreamBackend.h"
#include "AudioStreamAccess.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QVector>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

/*
模块名: AudioPrefetchStream
功能概述: 参考 VLC prefetch stream_filter，将网络读取放入后台线程，并向 FFmpeg AVIO
          暴露可 seek 的阻塞字节流，避免 demux 回调同步执行 HTTP 请求。
对外接口: AudioPrefetchStream
依赖关系: AudioStreamAccess、C++ thread/condition_variable
输入输出: 输入远程 URL、read/seek 字节请求，输出连续媒体字节。
异常与错误: 网络错误、EOF 和主动中断通过 ReadResult 传回调用方。
维护说明: 所有状态受互斥锁保护；seek 通过 generation 丢弃旧请求结果。
*/
class AudioPrefetchStream {
  public:
    enum ReadResult {
        ReadError = -1,
        ReadInterrupted = -2
    };

    using InterruptCallback = AudioStreamAccess::InterruptCallback;

    AudioPrefetchStream();
    ~AudioPrefetchStream();

    void setInterruptCallback(InterruptCallback callback);
    bool open(const QUrl& url);
    void stop();

    int read(uint8_t* dst, int size);
    bool seek(qint64 offset);
    void prepareSeekHint(qint64 targetMs, qint64 targetByte, qint64 contentLength, bool exact);
    bool forceSeekToHint(qint64 targetMs, qint64 targetByte, qint64 contentLength, bool exact);

    qint64 position() const;
    qint64 size() const;
    bool canSeek() const;
    bool wasLastReadInterrupted() const { return m_lastInterrupted.load(); }
    QString lastError() const;

  private:
    struct Track {
        qint64 start = 0;
        QByteArray data;
        qint64 lastUse = 0;
    };

    void workerLoop();
    void qtWorkerLoop();
    void curlWorkerLoop();
    bool appendBackendData(const QByteArray& data, int generation, qint64 replyStartOffset);
    bool startContinuousRequest(qint64 offset, int generation);
    void closeContinuousRequest(bool abortReply);
    void pumpNetworkEvents(int maxWaitMs);
    void appendNetworkData(const QByteArray& data, int generation, qint64 replyStartOffset);
    bool validateContinuousHeaders();
    bool shouldRetryStatus(int statusCode) const;
    void scheduleReconnectLocked(const QString& reason);
    void resetTracksLocked(qint64 offset);
    int findTrackContainingLocked(qint64 offset) const;
    int findTrackAppendableLocked(qint64 offset) const;
    int chooseReusableTrackLocked() const;
    qint64 trackEnd(const Track& track) const;
    qint64 cachedAheadLocked() const;
    qint64 resolveSeekOffsetLocked(qint64 requestedOffset);
    void trimTrackLocked(Track& track);
    bool shouldAbort() const;
    void waitForWorkerStopped();

    mutable std::mutex m_mutex;
    std::condition_variable m_dataCv;
    std::condition_variable m_spaceCv;
    std::thread m_worker;
    std::unique_ptr<AudioStreamAccess> m_access;
    std::unique_ptr<AudioCurlStreamBackend> m_curlBackend;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    std::atomic<bool> m_stopSignal{false};
    QNetworkReply* m_reply = nullptr;
    InterruptCallback m_interruptCallback;
    QUrl m_url;

    QVector<Track> m_tracks;
    int m_currentTrack = 0;
    qint64 m_pos = 0;
    qint64 m_prefetchOffset = 0;
    qint64 m_contentLength = -1;
    qint64 m_tick = 0;
    int m_generation = 0;

    bool m_opened = false;
    bool m_stopRequested = false;
    bool m_eof = false;
    bool m_error = false;
    bool m_canSeek = true;
    bool m_workerRunning = false;
    bool m_replyHeadersValidated = false;
    bool m_replyFinished = false;
    bool m_reconnectPending = false;
    bool m_abortReplyRequested = false;
    bool m_upstreamPaused = false;
    int m_retryCount = 0;
    int m_statusCode = 0;
    QString m_lastError;
    std::atomic<bool> m_lastInterrupted{false};
    QElapsedTimer m_streamTimer;
    int m_replyGeneration = -1;
    qint64 m_replyStartOffset = 0;
    qint64 m_lastAppendOffset = 0;
    qint64 m_firstByteStartMs = -1;
    qint64 m_reconnectNotBeforeMs = 0;
    qint64 m_lastReadWaitLogMs = 0;
    qint64 m_lastAppendLogOffset = 0;

    bool m_hasPendingSeekHint = false;
    bool m_pendingSeekHintExact = false;
    qint64 m_pendingSeekHintTargetMs = -1;
    qint64 m_pendingSeekHintByte = -1;
    qint64 m_pendingSeekHintContentLength = -1;

    const qint64 m_bufferMaxBytes = 24LL * 1024 * 1024;
    const qint64 m_trackMaxBytes = 8LL * 1024 * 1024;
    const qint64 m_lowWaterBytes = 2LL * 1024 * 1024;
    const qint64 m_seekThresholdBytes = 256LL * 1024;
    const int m_workerWaitMs = 20;
    const int m_maxRetryCount = 6;
};

#endif // AUDIOPREFETCHSTREAM_H
