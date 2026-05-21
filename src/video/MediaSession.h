#ifndef MEDIASESSION_H
#define MEDIASESSION_H

#include "AudioSession.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class VideoSession;
struct AVFilterGraph;
struct AVFilterContext;

class MediaSession : public QObject {
    Q_OBJECT

  public:
    enum class DemuxState { Stopped, Running, Paused };
    enum class SourceType { LocalFile, HttpFile, HlsVod, HlsLive };

    enum PlaybackState { Stopped, Playing, Paused, Error };
    Q_ENUM(PlaybackState)

    struct MediaOpenOptions {
        int networkTimeoutMs = 5000;
        int readTimeoutMs = 5000;
        int reconnectDelayMaxSeconds = 5;
        int probeSizeBytes = 1024 * 1024;
        int analyzeDurationUs = 1500000;
        bool reconnectEnabled = true;
        bool preferSeekableHttp = true;
        bool enableHardwareVideoDecode = true;
        QString userAgent = QStringLiteral("ffmpeg_music_player/1.0");
        QMap<QString, QString> headers;
    };

    struct VideoSourceInfo {
        SourceType sourceType = SourceType::LocalFile;
        bool networkSource = false;
        bool hlsSource = false;
        bool liveSource = false;
        bool canSeek = true;
        qint64 contentLength = -1;
        qint64 preferredCacheMs = 450;
        QString url;
    };

    struct NetworkStats {
        int readErrorCount = 0;
        int eofRetryCount = 0;
        int openTimeoutMs = 0;
        int readTimeoutMs = 0;
        bool reconnectEnabled = false;
        QString lastReadError;
    };

    explicit MediaSession(QObject* parent = nullptr);
    ~MediaSession() override;

    bool loadSource(const QUrl& url);
    bool loadSource(const QUrl& url, const MediaOpenOptions& options);
    void unloadSource();

    void play();
    void pause();
    void stop();
    bool seekTo(qint64 positionMs);

    void setPlaybackRate(double rate);
    double playbackRate() const;

    AudioSession* audioSession() const {
        return m_audioSession;
    }
    VideoSession* videoSession() const {
        return m_videoSession;
    }

    void enableSync(bool enable);
    bool isSyncEnabled() const {
        return m_syncEnabled;
    }

    qint64 getCurrentPosition() const;
    qint64 getDuration() const {
        return m_duration;
    }
    PlaybackState state() const {
        return m_state;
    }
    bool hasVideo() const {
        return m_videoSession != nullptr;
    }
    bool hasAudio() const {
        return m_audioCodecCtx != nullptr || m_audioStreamIndex >= 0;
    }
    VideoSourceInfo sourceInfo() const {
        return m_sourceInfo;
    }
    NetworkStats networkStats() const {
        return m_networkStats;
    }

  signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void stateChanged(PlaybackState state);
    void playbackRateChanged(double rate);

    void metadataReady(QString title, QString artist, qint64 duration);
    void syncError(int offsetMs);
    void playbackFinished();
    void playbackError(const QString& message);
    void demuxFinished();

  private slots:
    void onAudioPositionChanged(qint64 audioPos);
    void onVideoFrameRendered(qint64 videoPts);
    void syncAudioVideo();
    void updatePosition();

  private:
    bool isDemuxRunning() const {
        return m_demuxState.load() != DemuxState::Stopped;
    }
    bool isDemuxPaused() const {
        return m_demuxState.load() == DemuxState::Paused;
    }
    void setDemuxState(DemuxState state) {
        m_demuxState.store(state);
    }

    bool initDemuxer(const QString& filePath);
    void cleanupDemuxer();
    int seekDemuxer(qint64 positionMs);
    qint64 normalizedSeekPosition(qint64 positionMs) const;
    bool initAudioDecoder(AVStream* stream);
    bool recreateAudioResampler();
    void cleanupAudioFilterGraph();
    void cleanupAudioDecoder();

    void startDemux();
    void stopDemux();
    bool waitForDemuxThreadStopped(int timeoutMs);
    void demuxLoop();

    void setState(PlaybackState state);
    VideoSourceInfo classifySource(const QString& filePath) const;
    void applyInputOptions(AVDictionary** options) const;
    void setIoDeadlineMs(int timeoutMs);
    void clearIoDeadline();
    bool shouldAbortIo() const;
    static int interruptCallback(void* opaque);

  private:
    AudioSession* m_audioSession;
    VideoSession* m_videoSession;

    AVFormatContext* m_formatContext;
    int m_audioStreamIndex;
    int m_videoStreamIndex;

    AVCodecContext* m_audioCodecCtx;
    AVFrame* m_audioFrame;
    AVFilterGraph* m_audioFilterGraph;
    AVFilterContext* m_audioFilterSrcCtx;
    AVFilterContext* m_audioFilterSinkCtx;
    int m_audioOutputSampleRate;
    std::mutex m_audioResampleMutex;

    QString m_currentUrl;
    MediaOpenOptions m_openOptions;
    VideoSourceInfo m_sourceInfo;
    NetworkStats m_networkStats;
    std::atomic<bool> m_abortIo;
    std::atomic<qint64> m_ioDeadlineMs;
    bool m_hlsSource;
    qint64 m_duration;
    PlaybackState m_state;
    std::atomic<double> m_playbackRate;

    QTimer* m_syncTimer;
    bool m_syncEnabled;
    qint64 m_masterClock;

    QThread* m_demuxThread;
    std::atomic<DemuxState> m_demuxState;
    std::mutex m_demuxPauseMutex;
    std::condition_variable m_demuxPauseCv;
    std::optional<qint64> m_pendingAudioClockSyncMs;
    qint64 m_lastSeekTargetMs;
    qint64 m_hlsSeekStepBackMs;
    bool m_waitingFirstPacketAfterSeek;
    int m_seekEofRetryCount;

    QString m_audioWriteOwnerId;

    QTimer* m_positionTimer;
};

#endif // MEDIASESSION_H
