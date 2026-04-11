#ifndef AGENT_PROCESS_MANAGER_H
#define AGENT_PROCESS_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>

/**
 * @brief 管理 AI sidecar 进程生命周期与健康检查。
 */
class AgentProcessManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

public:
    explicit AgentProcessManager(QObject* parent = nullptr);

    bool startAgent();
    void stopAgent();
    bool isRunning() const { return m_running; }
    qint64 processId() const;

    QString agentDirectory() const { return m_agentDirectory; }
    QString programPath() const { return m_programPath; }
    QString workingDirectory() const { return m_workingDirectory; }

signals:
    void runningChanged();
    void started();
    void startFailed(const QString& reason);
    void exited(int exitCode, QProcess::ExitStatus exitStatus);
    void stdOutReceived(const QString& text);
    void stdErrReceived(const QString& text);
    void healthWarning(const QString& message);
    void healthInfoUpdated(const QVariantMap& healthInfo);

private slots:
    void onProcessStarted();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

    void onHealthCheckTimeout();
    void onHealthReplyFinished();

private:
    bool resolveAgentDirectory(QString* outDir) const;
    bool resolveStartCommand(const QString& agentDir, QString* outProgram, QStringList* outArguments) const;
    QStringList queryListeningPids(quint16 port) const;
    QStringList queryAgentBackendPids() const;
    bool waitForPortRelease(quint16 port, int timeoutMs) const;
    bool probeHealthReadySync(int timeoutMs) const;
    bool terminateExistingAgentBackend();
    void beginHealthCheck();
    void failStartup(const QString& reason);
    void setRunning(bool running);
    void cleanupHealthReply();

private:
    QProcess* m_process = nullptr;
    QNetworkAccessManager m_networkAccessManager;
    QTimer m_healthTimer;
    QPointer<QNetworkReply> m_healthReply;

    bool m_running = false;
    bool m_starting = false;
    bool m_startFailureNotified = false;
    bool m_attachedToExternalService = false;
    qint64 m_startDeadlineMs = 0;

    QString m_agentDirectory;
    QString m_programPath;
    QStringList m_programArguments;
    QString m_workingDirectory;
};

#endif // AGENT_PROCESS_MANAGER_H
