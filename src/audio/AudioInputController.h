#ifndef AUDIOINPUTCONTROLLER_H
#define AUDIOINPUTCONTROLLER_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QUrl>

class AudioService;

class AudioInputController : public QObject {
    Q_OBJECT
  public:
    explicit AudioInputController(AudioService* service, QObject* parent = nullptr);

    bool postPlayUrl(const QUrl& url);
    void postPause();
    void postResume();
    void postStop();
    void postSeek(const QString& sessionId, qint64 positionMs, qint64 currentPositionMs);
    void postPlayNext();
    void postPlayPrevious();
    void postPlayAtIndex(int index);
    void postSetVolume(int volume);

    void clearPendingForSession(const QString& sessionId);
    void clearAll();
    bool pendingSeekForSession(const QString& sessionId, qint64* positionMs = nullptr) const;

  private slots:
    void processNextCommand();
    void dispatchPendingSeek();

  private:
    enum class CommandType {
        PlayUrl,
        Pause,
        Resume,
        Stop,
        Seek,
        PlayNext,
        PlayPrevious,
        PlayAtIndex,
        SetVolume
    };

    struct Command {
        CommandType type = CommandType::Pause;
        int generation = 0;
        QString sessionId;
        QUrl url;
        qint64 positionMs = 0;
        int index = -1;
        int volume = 0;
    };

    void enqueue(const Command& command);
    void scheduleProcess(int delayMs = 0);
    void removeQueuedCommands(CommandType type);
    void removeQueuedCommandsForSession(const QString& sessionId);
    Command makeCommand(CommandType type);

    AudioService* m_service = nullptr;
    QQueue<Command> m_queue;
    QTimer m_processTimer;
    QTimer m_seekTimer;
    int m_nextGeneration = 0;

    bool m_hasPendingSeek = false;
    Command m_pendingSeek;
    QString m_lastSeekSessionId;
    qint64 m_lastSeekPositionMs = -1;
};

#endif // AUDIOINPUTCONTROLLER_H
