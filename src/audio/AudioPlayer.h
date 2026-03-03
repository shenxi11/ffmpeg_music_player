#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QAudioOutput>
#include <QIODevice>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "AudioBuffer.h"

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    static AudioPlayer& instance();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    bool start();
    void pause();
    void resume();
    void stop();

    void resetBuffer();

    void setVolume(int volume);
    int volume() const { return m_volume; }

    void writeAudioData(const QByteArray& data, qint64 timestampMs, const QString& ownerId = QString());
    void setWriteOwner(const QString& ownerId);
    void clearWriteOwner(const QString& ownerId = QString());
    QString writeOwner() const;

    qint64 getCurrentTimestamp() const;
    void setCurrentTimestamp(qint64 timestampMs);
    qint64 getPlaybackPosition() const;

    void setPlaybackRate(double rate);
    double playbackRate() const { return m_playbackRate.load(); }

    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }

    AudioBuffer* getBuffer() const { return m_buffer; }
    int bufferFillLevel() const;
    void ensureBufferCapacity(int targetCapacityBytes);

signals:
    void playbackStarted();
    void playbackPaused();
    void playbackResumed();
    void playbackStopped();

    void positionChanged(qint64 positionMs);

    void bufferStatusChanged(int fillLevel);
    void bufferUnderrun();

    void playbackError(const QString& error);

private slots:
    void onPositionUpdateTimer();

private:
    AudioPlayer();
    ~AudioPlayer();

    void playbackThread();
    bool initAudioOutput();
    void cleanupAudioOutput();
    void updateTimestamp(qint64 bytesConsumed);

private:
    QAudioOutput* m_audioOutput;
    QIODevice* m_audioDevice;
    QTimer* m_positionTimer;

    AudioBuffer* m_buffer;

    struct TimestampedData {
        qint64 timestamp;
        int dataSize;
    };
    std::queue<TimestampedData> m_timestampQueue;
    mutable std::mutex m_timestampMutex;

    QString m_writeOwner;
    mutable std::mutex m_ownerMutex;

    std::thread m_playThread;
    std::mutex m_playMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_threadRunning;
    std::atomic<bool> m_stopRequested;

    std::atomic<bool> m_isPlaying;
    std::atomic<bool> m_isPaused;
    std::atomic<int> m_volume;
    std::atomic<double> m_playbackRate;

    qint64 m_currentTimestamp;
    qint64 m_baseTimestamp;
    qint64 m_timestampConsumedBytes;
    QElapsedTimer m_playbackTimer;
    qint64 m_playbackStartTimestamp;
    qint64 m_pausedPosition;

    int m_sampleRate;
    int m_channels;
};

#endif // AUDIOPLAYER_H
