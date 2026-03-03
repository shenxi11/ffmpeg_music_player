#ifndef MEDIASESSION_H
#define MEDIASESSION_H

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QThread>
#include <QString>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "AudioSession.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class VideoSession;
struct AVFilterGraph;
struct AVFilterContext;

class MediaSession : public QObject
{
    Q_OBJECT

public:
    enum PlaybackState {
        Stopped,
        Playing,
        Paused,
        Error
    };
    Q_ENUM(PlaybackState)

    explicit MediaSession(QObject* parent = nullptr);
    ~MediaSession() override;

    bool loadSource(const QUrl& url);
    void unloadSource();

    void play();
    void pause();
    void stop();
    void seekTo(qint64 positionMs);

    void setPlaybackRate(double rate);
    double playbackRate() const;

    AudioSession* audioSession() const { return m_audioSession; }
    VideoSession* videoSession() const { return m_videoSession; }

    void enableSync(bool enable);
    bool isSyncEnabled() const { return m_syncEnabled; }

    qint64 getCurrentPosition() const;
    qint64 getDuration() const { return m_duration; }
    PlaybackState state() const { return m_state; }
    bool hasVideo() const { return m_videoSession != nullptr; }
    bool hasAudio() const { return m_audioCodecCtx != nullptr || m_audioStreamIndex >= 0; }

signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void stateChanged(PlaybackState state);
    void playbackRateChanged(double rate);

    void metadataReady(QString title, QString artist, qint64 duration);
    void syncError(int offsetMs);
    void playbackFinished();
    void demuxFinished();

private slots:
    void onAudioPositionChanged(qint64 audioPos);
    void onVideoFrameRendered(qint64 videoPts);
    void syncAudioVideo();
    void updatePosition();

private:
    bool initDemuxer(const QString& filePath);
    void cleanupDemuxer();
    bool initAudioDecoder(AVStream* stream);
    bool recreateAudioResampler();
    void cleanupAudioFilterGraph();
    void cleanupAudioDecoder();

    void startDemux();
    void stopDemux();
    void demuxLoop();

    void setState(PlaybackState state);

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
    qint64 m_duration;
    PlaybackState m_state;
    std::atomic<double> m_playbackRate;

    QTimer* m_syncTimer;
    bool m_syncEnabled;
    qint64 m_masterClock;

    QThread* m_demuxThread;
    bool m_demuxRunning;
    bool m_demuxPaused;
    std::mutex m_demuxPauseMutex;
    std::condition_variable m_demuxPauseCv;
    bool m_seekPending;

    QString m_audioWriteOwnerId;

    QTimer* m_positionTimer;
};

#endif // MEDIASESSION_H
