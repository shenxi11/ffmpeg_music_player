#include "AgentProcessManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QUrl>
#include <QDebug>

namespace {

QString normalizePath(const QString& path)
{
    return QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));
}

bool looksLikeAgentDir(const QString& path)
{
    if (path.isEmpty()) {
        return false;
    }

    const QDir dir(path);
    return dir.exists(QStringLiteral("pyproject.toml"))
        && dir.exists(QStringLiteral("src/music_agent/server.py"));
}

} // namespace

AgentProcessManager::AgentProcessManager(QObject* parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::started,
            this, &AgentProcessManager::onProcessStarted);
    connect(m_process, &QProcess::errorOccurred,
            this, &AgentProcessManager::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AgentProcessManager::onProcessFinished);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &AgentProcessManager::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &AgentProcessManager::onReadyReadStandardError);

    m_healthTimer.setInterval(300);
    m_healthTimer.setSingleShot(false);
    connect(&m_healthTimer, &QTimer::timeout,
            this, &AgentProcessManager::onHealthCheckTimeout);
}

qint64 AgentProcessManager::processId() const
{
    return m_process ? static_cast<qint64>(m_process->processId()) : -1;
}

bool AgentProcessManager::startAgent()
{
    if (!m_process) {
        emit startFailed(QStringLiteral("Agent 进程对象不可用。"));
        return false;
    }

    if (m_running) {
        emit started();
        return true;
    }

    if (m_starting) {
        return true;
    }

    QString resolvedAgentDir;
    if (!resolveAgentDirectory(&resolvedAgentDir)) {
        emit startFailed(QStringLiteral("未找到 agent 目录，请检查工程下的 agent 模块。"));
        return false;
    }

    QString program;
    QStringList arguments;
    if (!resolveStartCommand(resolvedAgentDir, &program, &arguments)) {
        emit startFailed(QStringLiteral("未找到可执行的 Agent 启动命令。"));
        return false;
    }

    if (probeHealthReadySync(600)) {
        m_attachedToExternalService = true;
        m_starting = false;
        m_startFailureNotified = false;
        m_startDeadlineMs = 0;
        setRunning(true);
        qDebug() << "[AgentProcess] attach to existing agent at 127.0.0.1:8765";
        emit started();
        return true;
    }

    qDebug() << "[AgentProcess] no healthy existing agent found, cleaning stale backend before launch";
    terminateExistingAgentBackend();

    m_starting = true;
    m_startFailureNotified = false;
    m_attachedToExternalService = false;
    m_startDeadlineMs = QDateTime::currentMSecsSinceEpoch() + 5000;

    m_agentDirectory = resolvedAgentDir;
    m_programPath = program;
    m_programArguments = arguments;
    m_workingDirectory = resolvedAgentDir;

    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->setProgram(m_programPath);
    m_process->setArguments(m_programArguments);
    m_process->start();

    qDebug() << "[AgentProcess] starting program:" << m_programPath
             << "args:" << m_programArguments
             << "cwd:" << m_workingDirectory;
    return true;
}

void AgentProcessManager::stopAgent()
{
    m_starting = false;
    m_startFailureNotified = false;
    m_startDeadlineMs = 0;
    m_healthTimer.stop();
    cleanupHealthReply();

    if (m_attachedToExternalService) {
        m_attachedToExternalService = false;
        setRunning(false);
        return;
    }

    if (!m_process) {
        setRunning(false);
        return;
    }

    if (m_process->state() == QProcess::NotRunning) {
        setRunning(false);
        return;
    }

    m_process->terminate();
    if (!m_process->waitForFinished(1200)) {
        m_process->kill();
        m_process->waitForFinished(800);
    }
    setRunning(false);
}

void AgentProcessManager::onProcessStarted()
{
    qDebug() << "[AgentProcess] started, pid=" << processId();
    beginHealthCheck();
}

void AgentProcessManager::onProcessError(QProcess::ProcessError error)
{
    const QString reason = QStringLiteral("启动 Agent 失败：%1")
                               .arg(m_process ? m_process->errorString() : QString());
    qWarning() << "[AgentProcess]" << reason << "error code:" << static_cast<int>(error);
    if (m_starting) {
        failStartup(reason);
    } else {
        emit stdErrReceived(reason);
    }
}

void AgentProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool wasStarting = m_starting;
    m_starting = false;
    m_healthTimer.stop();
    cleanupHealthReply();

    // attach 模式下，本进程可能是一次失败启动尝试，不应污染当前“已复用”的连接状态。
    if (!m_attachedToExternalService) {
        setRunning(false);
        emit exited(exitCode, exitStatus);
    }

    qDebug() << "[AgentProcess] exited, code=" << exitCode << "status=" << static_cast<int>(exitStatus);

    if (!m_attachedToExternalService && wasStarting && !m_startFailureNotified) {
        failStartup(QStringLiteral("Agent 启动后异常退出（exit=%1）。").arg(exitCode));
    }
}

void AgentProcessManager::onReadyReadStandardOutput()
{
    if (!m_process) {
        return;
    }

    const QString text = QString::fromUtf8(m_process->readAllStandardOutput());
    if (!text.trimmed().isEmpty()) {
        emit stdOutReceived(text);
    }
}

void AgentProcessManager::onReadyReadStandardError()
{
    if (!m_process) {
        return;
    }

    const QString text = QString::fromUtf8(m_process->readAllStandardError());
    if (!text.trimmed().isEmpty()) {
        emit stdErrReceived(text);
    }
}

void AgentProcessManager::onHealthCheckTimeout()
{
    if (!m_starting) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_startDeadlineMs > 0 && nowMs >= m_startDeadlineMs) {
        failStartup(QStringLiteral("Agent 健康检查超时（5 秒内未通过 /healthz）。"));
        return;
    }

    if (m_healthReply && !m_healthReply->isFinished()) {
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:8765/healthz")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(800);
#endif
    m_healthReply = m_networkAccessManager.get(request);
    connect(m_healthReply, &QNetworkReply::finished,
            this, &AgentProcessManager::onHealthReplyFinished);
}

void AgentProcessManager::onHealthReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const QByteArray body = reply->readAll();
    const bool success = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    if (m_healthReply == reply) {
        m_healthReply = nullptr;
    }

    if (!m_starting || !success) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    emit healthInfoUpdated(obj.toVariantMap());
    const QString status = obj.value(QStringLiteral("status")).toString().trimmed().toLower();
    if (status.isEmpty()) {
        return;
    }

    if (status == QStringLiteral("degraded")) {
        emit healthWarning(QStringLiteral("Agent 健康状态为 degraded：模型配置可能不完整。"));
    }

    m_starting = false;
    m_healthTimer.stop();
    setRunning(true);
    emit started();
}

bool AgentProcessManager::resolveAgentDirectory(QString* outDir) const
{
    QStringList candidates;
    const QString envDir = normalizePath(qEnvironmentVariable("AGENT_DIR"));
    if (!envDir.isEmpty()) {
        candidates << envDir;
    }

    const QString cwd = QDir::currentPath();
    const QString appDir = QCoreApplication::applicationDirPath();

    candidates << normalizePath(cwd + "/agent");
    candidates << normalizePath(cwd + "/../agent");
    candidates << normalizePath(cwd + "/../ffmpeg_music_player/agent");

    candidates << normalizePath(appDir + "/agent");
    candidates << normalizePath(appDir + "/../agent");
    candidates << normalizePath(appDir + "/../../agent");
    candidates << normalizePath(appDir + "/../../ffmpeg_music_player/agent");

    for (const QString& path : candidates) {
        if (looksLikeAgentDir(path)) {
            if (outDir) {
                *outDir = path;
            }
            return true;
        }
    }
    return false;
}

bool AgentProcessManager::resolveStartCommand(const QString& agentDir,
                                              QString* outProgram,
                                              QStringList* outArguments) const
{
    if (agentDir.trimmed().isEmpty()) {
        return false;
    }

#ifdef Q_OS_WIN
    const QString scriptDir = normalizePath(agentDir + "/.venv/Scripts");
    const QString serverExe = normalizePath(scriptDir + "/music-agent-server.exe");
    const QString pythonExe = normalizePath(scriptDir + "/python.exe");
#else
    const QString scriptDir = normalizePath(agentDir + "/.venv/bin");
    const QString serverExe = normalizePath(scriptDir + "/music-agent-server");
    const QString pythonExe = normalizePath(scriptDir + "/python");
#endif

    QString program;
    QStringList args;

    if (QFileInfo::exists(serverExe)) {
        program = serverExe;
    } else if (QFileInfo::exists(pythonExe)) {
        program = pythonExe;
        args << QStringLiteral("-m") << QStringLiteral("music_agent.server");
    } else {
        program = QStringLiteral("music-agent-server");
    }

    if (outProgram) {
        *outProgram = program;
    }
    if (outArguments) {
        *outArguments = args;
    }
    return !program.trimmed().isEmpty();
}

bool AgentProcessManager::terminateExistingAgentBackend()
{
    QStringList commands;
#ifdef Q_OS_WIN
    commands
        << QStringLiteral("$portPids = @(Get-NetTCPConnection -LocalPort 8765 -State Listen -ErrorAction SilentlyContinue | "
                          "Select-Object -ExpandProperty OwningProcess -Unique); "
                          "$agentPids = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | "
                          "Where-Object { $_.CommandLine -like '*music_agent.server*' -or $_.CommandLine -like '*music-agent-server*' } | "
                          "Select-Object -ExpandProperty ProcessId -Unique); "
                          "$all = @($portPids + $agentPids | Select-Object -Unique); "
                          "foreach ($pid in $all) { Stop-Process -Id $pid -Force -ErrorAction SilentlyContinue }; "
                          "exit 0");
#else
    commands
        << QStringLiteral("pkill -f 'music_agent.server'")
        << QStringLiteral("pkill -f 'music-agent-server'");
#endif

    bool executed = false;
    for (const QString& script : commands) {
#ifdef Q_OS_WIN
        QProcess process;
        process.start(QStringLiteral("powershell"),
                      QStringList{
                          QStringLiteral("-NoProfile"),
                          QStringLiteral("-ExecutionPolicy"),
                          QStringLiteral("Bypass"),
                          QStringLiteral("-Command"),
                          script,
                      });
#else
        QProcess process;
        process.start(QStringLiteral("sh"), QStringList{QStringLiteral("-lc"), script});
#endif
        if (!process.waitForStarted(1500)) {
            continue;
        }
        process.waitForFinished(5000);
        executed = true;
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1500);
        executed = true;
    }

    qDebug() << "[AgentProcess] forced backend cleanup finished, executed =" << executed;
    return executed;
}

void AgentProcessManager::beginHealthCheck()
{
    if (!m_starting) {
        return;
    }
    m_healthTimer.start();
    onHealthCheckTimeout();
}

void AgentProcessManager::failStartup(const QString& reason)
{
    if (m_startFailureNotified) {
        return;
    }

    m_startFailureNotified = true;
    m_starting = false;
    m_healthTimer.stop();
    cleanupHealthReply();

    emit startFailed(reason);

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(600)) {
            m_process->kill();
        }
    }
    setRunning(false);
}

void AgentProcessManager::setRunning(bool running)
{
    if (m_running == running) {
        return;
    }
    m_running = running;
    emit runningChanged();
}

void AgentProcessManager::cleanupHealthReply()
{
    if (!m_healthReply) {
        return;
    }

    disconnect(m_healthReply, nullptr, this, nullptr);
    if (!m_healthReply->isFinished()) {
        m_healthReply->abort();
    }
    m_healthReply->deleteLater();
    m_healthReply = nullptr;
}

bool AgentProcessManager::probeHealthReadySync(int timeoutMs) const
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:8765/healthz")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(qMax(100, timeoutMs));
#endif

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(qMax(100, timeoutMs));
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return false;
    }

    const bool ok = (reply->error() == QNetworkReply::NoError);
    const QByteArray body = reply->readAll();
    reply->deleteLater();
    if (!ok) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QString status = doc.object().value(QStringLiteral("status")).toString().trimmed().toLower();
    return (status == QStringLiteral("ok") || status == QStringLiteral("degraded"));
}
