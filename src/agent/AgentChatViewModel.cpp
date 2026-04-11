#include "AgentChatViewModel.h"

#include "AgentProcessManager.h"
#include "AgentSessionService.h"
#include "AgentWebSocketClient.h"
#include "capability/AgentCapabilityFacade.h"
#include "host/HostStateProvider.h"
#include "script/AgentScriptExecutor.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

AgentChatViewModel::AgentChatViewModel(QObject* parent)
    : QObject(parent)
    , m_processManager(new AgentProcessManager(this))
    , m_sessionService(new AgentSessionService(this))
    , m_socketClient(new AgentWebSocketClient(this))
    , m_hostStateProvider(new HostStateProvider(this))
    , m_capabilityFacade(new AgentCapabilityFacade(m_hostStateProvider, this))
    , m_scriptExecutor(new AgentScriptExecutor(m_capabilityFacade, this))
    , m_messageModel(this)
    , m_sessionModel(this)
{
    m_chunkFlushTimer.setSingleShot(true);
    m_chunkFlushTimer.setInterval(40);
    connect(&m_chunkFlushTimer, &QTimer::timeout, this, &AgentChatViewModel::onChunkFlushTimeout);
    setupConnections();
}

AgentChatViewModel::~AgentChatViewModel() = default;

void AgentChatViewModel::initialize()
{
    if (m_appExiting) {
        return;
    }

    setLastError(QString());

    if (m_processManager->isRunning()) {
        if (!m_initialSessionsLoaded) {
            loadSessions(m_lastSessionQuery);
            return;
        }

        if (!m_currentSessionId.isEmpty() && !m_socketClient->isConnected()) {
            connectSocketForCurrentSession();
            return;
        }

        if (m_currentSessionId.isEmpty()) {
            setState(AgentConnectionState::Idle);
        }
        return;
    }

    setState(AgentConnectionState::StartingProcess);
    if (!m_processManager->startAgent()) {
        setState(AgentConnectionState::Error);
    }
}

void AgentChatViewModel::retryConnection()
{
    if (m_appExiting) {
        return;
    }

    if (m_socketClient->isConnected()) {
        m_manualDisconnect = true;
    }
    setLastError(QString());

    if (m_processManager->isRunning()) {
        if (m_currentSessionId.isEmpty()) {
            loadSessions(m_lastSessionQuery);
            return;
        }

        setState(AgentConnectionState::ConnectingSocket);
        m_socketClient->reconnect();
        return;
    }

    initialize();
}

void AgentChatViewModel::sendMessage(const QString& text)
{
    const QString content = text.trimmed();
    if (content.isEmpty()) {
        return;
    }

    if (m_currentSessionId.isEmpty()) {
        appendErrorMessage(QString(), QStringLiteral("请先在左侧新建或选择会话。"));
        emit toastRequested(QStringLiteral("请先选择会话"));
        return;
    }

    if (!isReady()) {
        appendErrorMessage(QString(), QStringLiteral("当前未连接到 AI 助手，请先重连。"));
        emit toastRequested(QStringLiteral("AI 助手未连接"));
        return;
    }

    const QString requestId = nextRequestId();
    m_debugUserPromptByRequestId.insert(requestId, content);
    m_debugPendingRequestIds.append(requestId);
    appendUserMessage(requestId, content);
    m_socketClient->sendUserMessage(content, requestId);
}

void AgentChatViewModel::loadSessions(const QString& query)
{
    m_lastSessionQuery = query.trimmed();
    if (!m_processManager->isRunning()) {
        initialize();
        return;
    }

    setLoadingSessions(true);
    m_sessionService->fetchSessions(m_lastSessionQuery);
}

void AgentChatViewModel::searchSessions(const QString& query)
{
    loadSessions(query);
}

void AgentChatViewModel::createSession(const QString& title)
{
    if (!m_processManager->isRunning()) {
        initialize();
        appendErrorMessage(QString(), QStringLiteral("AI 服务尚未就绪，请稍后再试。"));
        return;
    }

    setLastError(QString());
    m_sessionService->createSession(title);
}

void AgentChatViewModel::selectSession(const QString& sessionIdValue)
{
    const QString sid = sessionIdValue.trimmed();
    if (sid.isEmpty()) {
        return;
    }

    if (!m_processManager->isRunning()) {
        initialize();
        return;
    }

    m_pendingMessageSessionId = sid;
    m_sessionModel.setSelectedSession(sid);
    m_pendingChunkByRequestId.clear();

    if (m_socketClient->isConnected()) {
        m_manualDisconnect = true;
        m_socketClient->disconnectFromServer();
    }
    m_socketClient->setSessionId(sid);

    setState(AgentConnectionState::ConnectingSocket);
    m_sessionService->fetchSessionMessages(sid);
}

void AgentChatViewModel::renameSession(const QString& sessionIdValue, const QString& title)
{
    const QString sid = sessionIdValue.trimmed();
    const QString trimmedTitle = title.trimmed();
    if (sid.isEmpty() || trimmedTitle.isEmpty()) {
        return;
    }
    m_sessionService->renameSession(sid, trimmedTitle);
}

void AgentChatViewModel::deleteSession(const QString& sessionIdValue)
{
    const QString sid = sessionIdValue.trimmed();
    if (sid.isEmpty()) {
        return;
    }
    m_sessionService->deleteSession(sid);
}

void AgentChatViewModel::refreshCurrentSession()
{
    if (m_currentSessionId.isEmpty()) {
        return;
    }
    m_sessionService->fetchSession(m_currentSessionId);
}

void AgentChatViewModel::sendApprovalResponse(const QString& planId,
                                              bool approved,
                                              const QString& reason)
{
    if (!m_socketClient) {
        appendErrorMessage(QString(), QStringLiteral("AI 连接未就绪，无法提交审批。"));
        return;
    }
    if (!m_socketClient->isConnected()) {
        appendErrorMessage(QString(), QStringLiteral("当前未连接到 AI 助手，请先重连。"));
        return;
    }

    m_socketClient->sendApprovalResponse(planId, approved, reason);
    const QString actionText = approved ? QStringLiteral("已同意执行计划。")
                                        : QStringLiteral("已拒绝执行计划。");
    appendSystemMessage(actionText, QStringLiteral("done"));
}

void AgentChatViewModel::startNewConversation()
{
    createSession(QStringLiteral("新建会话"));
}

QVariantList AgentChatViewModel::exportMessages() const
{
    return m_messageModel.dumpMessages();
}

void AgentChatViewModel::importMessages(const QVariantList& messages)
{
    m_messageModel.loadMessages(messages);
}

void AgentChatViewModel::switchToSession(const QString& sessionIdValue)
{
    selectSession(sessionIdValue);
}

void AgentChatViewModel::handleWindowClosed()
{
    if (m_socketClient->isConnected()) {
        m_manualDisconnect = true;
    }
    m_socketClient->disconnectFromServer();
    if (!m_appExiting) {
        setState(AgentConnectionState::Idle);
    }
}

void AgentChatViewModel::shutdownForAppExit()
{
    m_appExiting = true;
    if (m_socketClient && m_socketClient->isConnected()) {
        m_manualDisconnect = true;
    }

    if (m_socketClient) {
        m_socketClient->disconnectFromServer();
    }
    if (m_processManager) {
        m_processManager->stopAgent();
    }

    setState(AgentConnectionState::Idle);
}

void AgentChatViewModel::setMainShellViewModel(MainShellViewModel* shellViewModel)
{
    if (m_hostStateProvider) {
        m_hostStateProvider->setMainShellViewModel(shellViewModel);
    }
    if (m_capabilityFacade) {
        m_capabilityFacade->setMainShellViewModel(shellViewModel);
    }
}

QVariantMap AgentChatViewModel::validateClientScript(const QString& scriptText) const
{
    if (!m_scriptExecutor) {
        return {
            {QStringLiteral("ok"), false},
            {QStringLiteral("code"), QStringLiteral("script_executor_unavailable")},
            {QStringLiteral("message"), QStringLiteral("客户端脚本执行器尚未初始化")}
        };
    }
    return m_scriptExecutor->validateScript(scriptText);
}

QVariantMap AgentChatViewModel::dryRunClientScript(const QString& scriptText) const
{
    if (!m_scriptExecutor) {
        return {
            {QStringLiteral("ok"), false},
            {QStringLiteral("code"), QStringLiteral("script_executor_unavailable")},
            {QStringLiteral("message"), QStringLiteral("客户端脚本执行器尚未初始化")}
        };
    }
    return m_scriptExecutor->dryRunScript(scriptText);
}

QString AgentChatViewModel::executeClientScript(const QString& scriptText)
{
    if (!m_scriptExecutor) {
        appendErrorMessage(QString(), QStringLiteral("客户端脚本执行器尚未初始化。"));
        return QString();
    }

    const QVariantMap validation = m_scriptExecutor->validateScript(scriptText);
    if (!validation.value(QStringLiteral("ok")).toBool()) {
        appendErrorMessage(QString(),
                           validation.value(QStringLiteral("message")).toString().trimmed().isEmpty()
                               ? QStringLiteral("客户端脚本校验失败。")
                               : validation.value(QStringLiteral("message")).toString());
        return QString();
    }

    const QString executionId = m_scriptExecutor->executeScript(scriptText);
    if (executionId.isEmpty()) {
        appendErrorMessage(QString(), QStringLiteral("客户端脚本执行启动失败。"));
        return QString();
    }
    return executionId;
}

void AgentChatViewModel::onScriptValidationRequestReceived(const QString& requestId,
                                                           const QString& scriptText,
                                                           const QVariantMap& payload)
{
    Q_UNUSED(payload);
    if (!m_socketClient) {
        return;
    }

    rememberDebugScript(requestId, QStringLiteral("validate_script"), scriptText);

    const QVariantMap validation = validateClientScript(scriptText);
    const bool ok = validation.value(QStringLiteral("ok")).toBool();
    if (ok) {
        m_socketClient->sendScriptValidationResult(requestId, true, validation);
        return;
    }

    QVariantMap error{
        {QStringLiteral("code"), validation.value(QStringLiteral("code")).toString().trimmed().isEmpty()
                                      ? QStringLiteral("script_validation_failed")
                                      : validation.value(QStringLiteral("code")).toString()},
        {QStringLiteral("message"), validation.value(QStringLiteral("message")).toString().trimmed().isEmpty()
                                         ? QStringLiteral("客户端脚本校验失败")
                                         : validation.value(QStringLiteral("message")).toString()}
    };
    if (validation.contains(QStringLiteral("detail"))) {
        error.insert(QStringLiteral("detail"), validation.value(QStringLiteral("detail")));
    }
    m_socketClient->sendScriptValidationResult(requestId, false, QVariantMap(), error);
}

void AgentChatViewModel::onScriptDryRunRequestReceived(const QString& requestId,
                                                       const QString& scriptText,
                                                       const QVariantMap& payload)
{
    Q_UNUSED(payload);
    if (!m_socketClient) {
        return;
    }

    rememberDebugScript(requestId, QStringLiteral("dry_run_script"), scriptText);

    const QVariantMap report = dryRunClientScript(scriptText);
    const bool ok = report.value(QStringLiteral("ok")).toBool();
    if (ok) {
        m_socketClient->sendScriptDryRunResult(requestId, true, report);
        return;
    }

    QVariantMap error{
        {QStringLiteral("code"), report.value(QStringLiteral("code")).toString().trimmed().isEmpty()
                                      ? QStringLiteral("script_dry_run_failed")
                                      : report.value(QStringLiteral("code")).toString()},
        {QStringLiteral("message"), report.value(QStringLiteral("message")).toString().trimmed().isEmpty()
                                         ? QStringLiteral("客户端脚本 dry-run 失败")
                                         : report.value(QStringLiteral("message")).toString()}
    };
    if (report.contains(QStringLiteral("detail"))) {
        error.insert(QStringLiteral("detail"), report.value(QStringLiteral("detail")));
    }
    m_socketClient->sendScriptDryRunResult(requestId, false, QVariantMap(), error);
}

void AgentChatViewModel::onScriptExecutionRequestReceived(const QString& requestId,
                                                          const QString& scriptText,
                                                          const QVariantMap& payload)
{
    if (!m_socketClient) {
        return;
    }

    rememberDebugScript(requestId, QStringLiteral("execute_script"), scriptText);

    const QVariantMap validation = validateClientScript(scriptText);
    if (!validation.value(QStringLiteral("ok")).toBool()) {
        QVariantMap error{
            {QStringLiteral("code"), validation.value(QStringLiteral("code")).toString().trimmed().isEmpty()
                                          ? QStringLiteral("script_validation_failed")
                                          : validation.value(QStringLiteral("code")).toString()},
            {QStringLiteral("message"), validation.value(QStringLiteral("message")).toString().trimmed().isEmpty()
                                             ? QStringLiteral("客户端脚本校验失败")
                                             : validation.value(QStringLiteral("message")).toString()}
        };
        if (validation.contains(QStringLiteral("detail"))) {
            error.insert(QStringLiteral("detail"), validation.value(QStringLiteral("detail")));
        }
        m_socketClient->sendScriptExecutionResult(requestId, QString(), false, QVariantMap(), error);
        return;
    }

    const bool requiresApproval = validation.value(QStringLiteral("requiresApproval")).toBool();
    const QVariant approvedValue = payload.value(QStringLiteral("approved"));
    const bool approved = approvedValue.isValid() ? approvedValue.toBool() : false;
    if (requiresApproval && !approved) {
        m_socketClient->sendScriptExecutionResult(
            requestId,
            QString(),
            false,
            QVariantMap(),
            {
                {QStringLiteral("code"), QStringLiteral("approval_required")},
                {QStringLiteral("message"), QStringLiteral("该脚本包含高风险步骤，客户端要求先审批后再执行")},
                {QStringLiteral("riskLevel"), validation.value(QStringLiteral("riskLevel")).toString()},
                {QStringLiteral("validation"), validation}
            });
        return;
    }

    const QString executionId = executeClientScript(scriptText);
    if (executionId.isEmpty()) {
        m_socketClient->sendScriptExecutionResult(
            requestId,
            QString(),
            false,
            QVariantMap(),
            {
                {QStringLiteral("code"), QStringLiteral("script_execution_start_failed")},
                {QStringLiteral("message"), QStringLiteral("客户端脚本执行启动失败")}
            });
        return;
    }

    m_scriptRequestIdByExecutionId.insert(executionId, requestId);
}

void AgentChatViewModel::onScriptCancelRequestReceived(const QString& requestId,
                                                       const QString& executionId,
                                                       const QVariantMap& payload)
{
    if (!m_socketClient || !m_scriptExecutor) {
        return;
    }

    const QString reason = payload.value(QStringLiteral("reason")).toString().trimmed();
    const bool cancelled = m_scriptExecutor->cancelExecution(
        executionId,
        reason.isEmpty() ? QStringLiteral("服务端请求取消脚本执行") : reason);

    if (cancelled) {
        m_socketClient->sendScriptCancellationResult(
            requestId,
            executionId,
            true,
            {
                {QStringLiteral("accepted"), true},
                {QStringLiteral("executionId"), executionId}
            });
        return;
    }

    m_socketClient->sendScriptCancellationResult(
        requestId,
        executionId,
        false,
        QVariantMap(),
        {
            {QStringLiteral("code"), QStringLiteral("execution_not_found")},
            {QStringLiteral("message"), QStringLiteral("未找到可取消的脚本执行实例")}
        });
}

void AgentChatViewModel::onScriptExecutionStarted(const QString& executionId, const QVariantMap& summary)
{
    if (!m_socketClient) {
        return;
    }

    const QString requestId = m_scriptRequestIdByExecutionId.value(executionId);
    if (requestId.isEmpty()) {
        return;
    }
    m_socketClient->sendScriptExecutionStarted(requestId, executionId, summary);
}

void AgentChatViewModel::onScriptStepStarted(const QString& executionId,
                                             int stepIndex,
                                             const QVariantMap& stepSummary)
{
    if (!m_socketClient) {
        return;
    }

    const QString requestId = m_scriptRequestIdByExecutionId.value(executionId);
    if (requestId.isEmpty()) {
        return;
    }
    m_socketClient->sendScriptStepEvent(requestId,
                                        executionId,
                                        stepIndex,
                                        QStringLiteral("started"),
                                        stepSummary);
}

void AgentChatViewModel::onScriptStepFinished(const QString& executionId,
                                              int stepIndex,
                                              bool ok,
                                              const QVariantMap& payload)
{
    if (!m_socketClient) {
        return;
    }

    const QString requestId = m_scriptRequestIdByExecutionId.value(executionId);
    if (requestId.isEmpty()) {
        return;
    }

    QVariantMap stepPayload = payload;
    stepPayload.insert(QStringLiteral("ok"), ok);
    m_socketClient->sendScriptStepEvent(requestId,
                                        executionId,
                                        stepIndex,
                                        QStringLiteral("finished"),
                                        stepPayload);
}

void AgentChatViewModel::onScriptExecutionFinished(const QString& executionId,
                                                   bool ok,
                                                   const QVariantMap& report)
{
    if (!m_socketClient) {
        return;
    }

    const QString requestId = m_scriptRequestIdByExecutionId.take(executionId);
    if (requestId.isEmpty()) {
        return;
    }

    if (ok) {
        m_socketClient->sendScriptExecutionResult(requestId, executionId, true, report);
        return;
    }

    const QVariantMap error = report.value(QStringLiteral("error")).toMap();
    m_socketClient->sendScriptExecutionResult(requestId, executionId, false, QVariantMap(), error);
}

QString AgentChatViewModel::connectionStateText() const
{
    return agentConnectionStateText(m_state);
}

QString AgentChatViewModel::sessionId() const
{
    return m_currentSessionId;
}

void AgentChatViewModel::onAgentStarted()
{
    if (m_appExiting) {
        return;
    }

    appendSystemMessage(QStringLiteral("AI 服务已启动。"));
    loadSessions(m_lastSessionQuery);
}

void AgentChatViewModel::onAgentStartFailed(const QString& reason)
{
    setLastError(reason, true);
    setState(AgentConnectionState::Error);
    appendErrorMessage(QString(), QStringLiteral("AI 服务启动失败：%1").arg(reason));
}

void AgentChatViewModel::onAgentExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_appExiting) {
        return;
    }

    const QString reason = QStringLiteral("AI 服务已退出（code=%1，status=%2）")
                               .arg(exitCode)
                               .arg(static_cast<int>(exitStatus));
    appendErrorMessage(QString(), reason);
    setLastError(reason, true);
    setState(AgentConnectionState::Error);
}

void AgentChatViewModel::onAgentStdOut(const QString& text)
{
    qDebug().noquote() << "[AgentStdOut]" << text.trimmed();
}

void AgentChatViewModel::onAgentStdErr(const QString& text)
{
    qWarning().noquote() << "[AgentStdErr]" << text.trimmed();
}

void AgentChatViewModel::onHealthWarning(const QString& message)
{
    appendSystemMessage(QStringLiteral("健康检查告警：%1").arg(message));
    emit toastRequested(message);
}

void AgentChatViewModel::onHealthInfoUpdated(const QVariantMap& healthInfo)
{
    const QString protocolVersion = healthInfo.value(QStringLiteral("protocolVersion")).toString().trimmed();
    if (!protocolVersion.isEmpty()) {
        m_protocolVersion = protocolVersion;
    }

    QSet<QString> healthCaps;
    const QVariantList caps = healthInfo.value(QStringLiteral("capabilities")).toList();
    for (const QVariant& capValue : caps) {
        const QString cap = capValue.toString().trimmed().toLower();
        if (!cap.isEmpty()) {
            healthCaps.insert(cap);
        }
    }

    const bool toolModeEnabled = healthInfo.value(QStringLiteral("toolModeEnabled"), true).toBool();
    const bool toolsCapEnabled = healthCaps.isEmpty() || healthCaps.contains(QStringLiteral("tools"));
    m_toolsEnabledByHealth = toolModeEnabled && toolsCapEnabled;
    m_toolsEnabled = m_toolsEnabledByHealth && m_toolsEnabledBySession;

    if (!m_toolsEnabled) {
        appendSystemMessage(QStringLiteral("healthz 显示 tools 能力未开启，当前仅支持聊天。"));
    }
}

void AgentChatViewModel::onSessionsLoaded(const QVector<ChatSessionItem>& sessions)
{
    setLoadingSessions(false);
    setLastError(QString());
    m_initialSessionsLoaded = true;

    QVector<ChatSessionItem> normalized = sessions;
    for (ChatSessionItem& item : normalized) {
        item.selected = (!m_currentSessionId.isEmpty() && item.sessionId == m_currentSessionId);
    }
    m_sessionModel.setSessions(normalized);

    if (normalized.isEmpty()) {
        m_messageModel.clear();
        m_socketClient->clearSession();
        clearCurrentSession();
        setState(AgentConnectionState::Idle);
        return;
    }

    if (!m_currentSessionId.isEmpty() && m_sessionModel.containsSession(m_currentSessionId)) {
        m_sessionModel.setSelectedSession(m_currentSessionId);
        if (!m_socketClient->isConnected() && m_messageModel.rowCount() == 0) {
            selectSession(m_currentSessionId);
        }
        return;
    }

    if (m_lastSessionQuery.isEmpty()) {
        selectSession(normalized.first().sessionId);
    }
}

void AgentChatViewModel::onSessionCreated(const ChatSessionItem& session)
{
    if (session.sessionId.trimmed().isEmpty()) {
        return;
    }

    m_sessionModel.upsertSession(session, true);
    selectSession(session.sessionId);
    emit toastRequested(QStringLiteral("会话已创建"));
}

void AgentChatViewModel::onSessionLoaded(const ChatSessionItem& session)
{
    if (session.sessionId.trimmed().isEmpty()) {
        return;
    }

    m_sessionModel.upsertSession(session, false);
    if (session.sessionId == m_currentSessionId) {
        setCurrentSession(session);
    }
}

void AgentChatViewModel::onSessionUpdated(const ChatSessionItem& session)
{
    if (session.sessionId.trimmed().isEmpty()) {
        return;
    }

    m_sessionModel.upsertSession(session, false);
    if (session.sessionId == m_currentSessionId) {
        setCurrentSession(session);
    }
}

void AgentChatViewModel::onSessionDeleted(const QString& sessionIdValue)
{
    const QString sid = sessionIdValue.trimmed();
    if (sid.isEmpty()) {
        return;
    }

    m_sessionModel.removeSession(sid);
    emit toastRequested(QStringLiteral("会话已删除"));

    if (sid != m_currentSessionId) {
        return;
    }

    m_pendingChunkByRequestId.clear();
    m_messageModel.clear();
    clearCurrentSession();
    m_socketClient->clearSession();
    if (m_socketClient->isConnected()) {
        m_manualDisconnect = true;
    }
    m_socketClient->disconnectFromServer();

    if (m_sessionModel.rowCount() > 0) {
        selectSession(m_sessionModel.sessionAt(0).sessionId);
    } else {
        setState(AgentConnectionState::Idle);
    }
}

void AgentChatViewModel::onSessionMessagesLoaded(const ChatSessionItem& session,
                                                 const QVector<ChatMessageItem>& messages)
{
    if (!m_pendingMessageSessionId.isEmpty() && session.sessionId != m_pendingMessageSessionId) {
        return;
    }
    m_pendingMessageSessionId.clear();

    setCurrentSession(session);
    m_sessionModel.setSelectedSession(session.sessionId);
    rebuildMessages(messages);
    connectSocketForCurrentSession();
}

void AgentChatViewModel::onSessionRequestFailed(const QString& operation, const QString& errorMessage)
{
    setLoadingSessions(false);

    const QString op = operation.trimmed().isEmpty() ? QStringLiteral("session_request") : operation.trimmed();
    const QString text = QStringLiteral("%1：%2").arg(op, errorMessage);
    appendErrorMessage(QString(), text);
    setLastError(errorMessage, true);

    if (op == QStringLiteral("fetch_messages")) {
        setState(AgentConnectionState::Error);
    }
}

void AgentChatViewModel::onSocketConnected()
{
    if (m_appExiting) {
        return;
    }

    setState(AgentConnectionState::ConnectingSocket);
}

void AgentChatViewModel::onSocketDisconnected()
{
    if (m_appExiting) {
        return;
    }

    if (m_manualDisconnect) {
        m_manualDisconnect = false;
        return;
    }

    const QString reason = QStringLiteral("AI 会话连接已断开，可点击“重连”恢复。");
    appendErrorMessage(QString(), reason);
    setLastError(reason, true);
    setState(AgentConnectionState::Error);
}

void AgentChatViewModel::onSessionReady(const QString& sessionIdValue)
{
    const QString sid = sessionIdValue.trimmed();
    if (sid.isEmpty()) {
        return;
    }

    if (sid != m_currentSessionId) {
        ChatSessionItem session = m_sessionModel.sessionById(sid);
        if (session.sessionId.isEmpty()) {
            session.sessionId = sid;
            session.title = QStringLiteral("新建会话");
            session.selected = true;
            m_sessionModel.upsertSession(session, true);
        }
        setCurrentSession(session);
        m_sessionModel.setSelectedSession(sid);
    }

    setState(AgentConnectionState::Ready);
    setLastError(QString());
    m_capabilities.clear();
    m_toolsEnabledBySession = true;
    m_toolsEnabled = m_toolsEnabledByHealth;
    emit sessionIdChanged();

    if (!m_currentSessionId.isEmpty()) {
        m_sessionService->fetchSession(m_currentSessionId);
    }
}

void AgentChatViewModel::onSessionReadyDetailed(const QString& sessionIdValue,
                                                const QString& title,
                                                const QStringList& capabilities,
                                                const QVariantMap& payload)
{
    Q_UNUSED(payload);

    onSessionReady(sessionIdValue);

    const QString sid = sessionIdValue.trimmed();
    if (sid.isEmpty()) {
        return;
    }

    ChatSessionItem session = m_sessionModel.sessionById(sid);
    if (!title.trimmed().isEmpty()) {
        session.title = title.trimmed();
        if (session.sessionId.isEmpty()) {
            session.sessionId = sid;
        }
        m_sessionModel.upsertSession(session, sid == m_currentSessionId);
        if (sid == m_currentSessionId) {
            setCurrentSession(session);
        }
    }

    m_capabilities.clear();
    for (const QString& cap : capabilities) {
        const QString normalized = cap.trimmed().toLower();
        if (!normalized.isEmpty()) {
            m_capabilities.insert(normalized);
        }
    }
    const bool toolsCapEnabled = m_capabilities.isEmpty() || m_capabilities.contains(QStringLiteral("tools"));
    m_toolsEnabledBySession = toolsCapEnabled;
    m_toolsEnabled = m_toolsEnabledByHealth && m_toolsEnabledBySession;

    if (!m_toolsEnabled) {
        appendSystemMessage(QStringLiteral("当前 Agent 会话未开启工具调用能力，将仅使用聊天模式。"));
    }
}

void AgentChatViewModel::onAssistantStartReceived(const QString& requestId)
{
    if (requestId.trimmed().isEmpty()) {
        return;
    }
    m_messageModel.beginAssistantMessage(requestId);
}

void AgentChatViewModel::onAssistantChunkReceived(const QString& requestId, const QString& delta)
{
    if (requestId.trimmed().isEmpty()) {
        return;
    }
    m_pendingChunkByRequestId[requestId] += delta;
    if (!m_chunkFlushTimer.isActive()) {
        m_chunkFlushTimer.start();
    }
}

void AgentChatViewModel::onAssistantFinalReceived(const QString& requestId, const QString& content)
{
    flushPendingChunksForRequest(requestId);

    if (requestId.trimmed().isEmpty()) {
        appendAssistantMessage(requestId, content);
    } else {
        m_messageModel.setRawTextAndReparse(requestId, content, QStringLiteral("done"));
        m_debugAssistantReplyByRequestId.insert(requestId, content);
        logAgentConversationDebug(requestId);
        clearDebugTrace(requestId);
        m_debugPendingRequestIds.removeAll(requestId);
    }

    if (!m_currentSessionId.isEmpty()) {
        m_sessionService->fetchSession(m_currentSessionId);
        if (m_lastSessionQuery.isEmpty()) {
            m_sessionService->fetchSessions();
        }
    }
}

void AgentChatViewModel::onAssistantMessageReceived(const QString& requestId, const QString& content)
{
    onAssistantFinalReceived(requestId, content);
}

void AgentChatViewModel::onPlanPreviewReceived(const QString& planId, const QVariantMap& payload)
{
    QVariantMap meta = payload;
    if (!planId.trimmed().isEmpty()) {
        meta.insert(QStringLiteral("planId"), planId.trimmed());
    }

    QString summary = payload.value(QStringLiteral("summary")).toString().trimmed();
    if (summary.isEmpty()) {
        summary = QStringLiteral("Agent 给出了一个执行计划。");
    }

    appendStructuredAssistantMessage(QString(),
                                     QStringLiteral("plan_preview"),
                                     summary,
                                     meta,
                                     QStringLiteral("done"));
}

void AgentChatViewModel::onApprovalRequestReceived(const QString& planId,
                                                   const QString& message,
                                                   const QVariantMap& payload)
{
    QVariantMap meta = payload;
    if (!planId.trimmed().isEmpty()) {
        meta.insert(QStringLiteral("planId"), planId.trimmed());
    }

    QString prompt = message.trimmed();
    if (prompt.isEmpty()) {
        prompt = QStringLiteral("该计划需要你的确认后继续执行。");
    }

    appendStructuredAssistantMessage(QString(),
                                     QStringLiteral("approval_request"),
                                     prompt,
                                     meta,
                                     QStringLiteral("pending"));
}

void AgentChatViewModel::onClarificationRequestReceived(const QString& requestId,
                                                        const QString& question,
                                                        const QStringList& options,
                                                        const QVariantMap& payload)
{
    Q_UNUSED(payload);
    QString text = question.trimmed();
    if (text.isEmpty()) {
        text = QStringLiteral("请补充更具体的信息。");
    }
    if (!options.isEmpty()) {
        text += QStringLiteral("\n");
        for (int i = 0; i < options.size(); ++i) {
            text += QStringLiteral("\n%1. %2").arg(i + 1).arg(options.at(i));
        }
    }
    appendAssistantMessage(requestId, text);
}

void AgentChatViewModel::onProgressReceived(const QString& message, const QVariantMap& payload)
{
    const QString planId = payload.value(QStringLiteral("planId")).toString().trimmed();
    const QString stepId = payload.value(QStringLiteral("stepId")).toString().trimmed();
    QString text = message.trimmed();
    if (text.isEmpty()) {
        text = QStringLiteral("任务执行中…");
    }
    if (!planId.isEmpty() || !stepId.isEmpty()) {
        text = QStringLiteral("[进度 %1 %2] %3")
                   .arg(planId.isEmpty() ? QStringLiteral("-") : planId,
                        stepId.isEmpty() ? QStringLiteral("-") : stepId,
                        text);
    }
    appendSystemMessage(text, QStringLiteral("pending"));
}

void AgentChatViewModel::onFinalResultReceived(const QVariantMap& payload)
{
    const bool ok = payload.value(QStringLiteral("ok"), true).toBool();
    const QString summary = payload.value(QStringLiteral("summary")).toString().trimmed();
    QString text = summary.isEmpty()
        ? (ok ? QStringLiteral("任务执行完成。") : QStringLiteral("任务执行失败。"))
        : summary;
    if (!ok) {
        text = QStringLiteral("执行失败：%1").arg(text);
    }
    appendSystemMessage(text, ok ? QStringLiteral("done") : QStringLiteral("error"));
}

void AgentChatViewModel::onProtocolError(const QString& code, const QString& message)
{
    const QString text = QStringLiteral("协议错误[%1]：%2").arg(code, message);
    appendErrorMessage(QString(), text);
    setLastError(text, true);
}

void AgentChatViewModel::onRequestError(const QString& requestId, const QString& code, const QString& message)
{
    flushPendingChunksForRequest(requestId);

    const QString text = QStringLiteral("请求失败[%1]：%2").arg(code, message);
    if (!m_messageModel.markAssistantMessageError(requestId, text)) {
        appendErrorMessage(requestId, text);
    }
    setLastError(text, true);
    if (!requestId.trimmed().isEmpty()) {
        m_debugAssistantReplyByRequestId.insert(requestId, text);
        logAgentConversationDebug(requestId);
        clearDebugTrace(requestId);
        m_debugPendingRequestIds.removeAll(requestId);
    }
}

void AgentChatViewModel::onChunkFlushTimeout()
{
    if (m_pendingChunkByRequestId.isEmpty()) {
        return;
    }

    const auto keys = m_pendingChunkByRequestId.keys();
    for (const QString& requestId : keys) {
        const QString delta = m_pendingChunkByRequestId.take(requestId);
        if (requestId.trimmed().isEmpty() || delta.isEmpty()) {
            continue;
        }
        m_messageModel.appendRawDeltaAndReparse(requestId, delta);
    }
}

void AgentChatViewModel::onToolCallReceived(const QString& toolCallId,
                                            const QString& tool,
                                            const QVariantMap& args)
{
    rememberDirectToolCall(toolCallId, tool, args);

    if (!m_toolsEnabled) {
        m_socketClient->sendToolResult(toolCallId,
                                       false,
                                       QVariantMap(),
                                       QVariantMap{
                                           {QStringLiteral("code"), QStringLiteral("tool_mode_disabled")},
                                           {QStringLiteral("message"), QStringLiteral("服务端未开启 tools 能力")},
                                           {QStringLiteral("retryable"), false}
                                       });
        return;
    }

    if (!m_capabilityFacade) {
        m_socketClient->sendToolResult(toolCallId,
                                       false,
                                       QVariantMap(),
                                       QVariantMap{
                                           {QStringLiteral("code"), QStringLiteral("executor_unavailable")},
                                           {QStringLiteral("message"), QStringLiteral("Qt 工具执行器不可用")},
                                           {QStringLiteral("retryable"), false}
                                       });
        return;
    }

    if (!m_capabilityFacade->executeCapability(toolCallId, tool, args)) {
        m_socketClient->sendToolResult(toolCallId,
                                       false,
                                       QVariantMap(),
                                       QVariantMap{
                                           {QStringLiteral("code"), QStringLiteral("invalid_tool_call")},
                                           {QStringLiteral("message"), QStringLiteral("tool_call 参数非法")},
                                           {QStringLiteral("retryable"), false}
                                       });
    }
}

void AgentChatViewModel::onToolResultReady(const QString& toolCallId,
                                           bool ok,
                                           const QVariantMap& result,
                                           const QVariantMap& error)
{
    if (!m_socketClient) {
        return;
    }
    m_socketClient->sendToolResult(toolCallId, ok, result, error);
}

void AgentChatViewModel::setupConnections()
{
    connect(m_processManager, &AgentProcessManager::started,
            this, &AgentChatViewModel::onAgentStarted);
    connect(m_processManager, &AgentProcessManager::startFailed,
            this, &AgentChatViewModel::onAgentStartFailed);
    connect(m_processManager, &AgentProcessManager::exited,
            this, &AgentChatViewModel::onAgentExited);
    connect(m_processManager, &AgentProcessManager::stdOutReceived,
            this, &AgentChatViewModel::onAgentStdOut);
    connect(m_processManager, &AgentProcessManager::stdErrReceived,
            this, &AgentChatViewModel::onAgentStdErr);
    connect(m_processManager, &AgentProcessManager::healthWarning,
            this, &AgentChatViewModel::onHealthWarning);
    connect(m_processManager, &AgentProcessManager::healthInfoUpdated,
            this, &AgentChatViewModel::onHealthInfoUpdated);

    connect(m_sessionService, &AgentSessionService::sessionsLoaded,
            this, &AgentChatViewModel::onSessionsLoaded);
    connect(m_sessionService, &AgentSessionService::sessionCreated,
            this, &AgentChatViewModel::onSessionCreated);
    connect(m_sessionService, &AgentSessionService::sessionLoaded,
            this, &AgentChatViewModel::onSessionLoaded);
    connect(m_sessionService, &AgentSessionService::sessionUpdated,
            this, &AgentChatViewModel::onSessionUpdated);
    connect(m_sessionService, &AgentSessionService::sessionDeleted,
            this, &AgentChatViewModel::onSessionDeleted);
    connect(m_sessionService, &AgentSessionService::sessionMessagesLoaded,
            this, &AgentChatViewModel::onSessionMessagesLoaded);
    connect(m_sessionService, &AgentSessionService::requestFailed,
            this, &AgentChatViewModel::onSessionRequestFailed);

    connect(m_socketClient, &AgentWebSocketClient::connected,
            this, &AgentChatViewModel::onSocketConnected);
    connect(m_socketClient, &AgentWebSocketClient::disconnected,
            this, &AgentChatViewModel::onSocketDisconnected);
    connect(m_socketClient, &AgentWebSocketClient::sessionReady,
            this, &AgentChatViewModel::onSessionReady);
    connect(m_socketClient, &AgentWebSocketClient::sessionReadyDetailed,
            this, &AgentChatViewModel::onSessionReadyDetailed);
    connect(m_socketClient, &AgentWebSocketClient::assistantStartReceived,
            this, &AgentChatViewModel::onAssistantStartReceived);
    connect(m_socketClient, &AgentWebSocketClient::assistantChunkReceived,
            this, &AgentChatViewModel::onAssistantChunkReceived);
    connect(m_socketClient, &AgentWebSocketClient::assistantFinalReceived,
            this, &AgentChatViewModel::onAssistantFinalReceived);
    connect(m_socketClient, &AgentWebSocketClient::protocolError,
            this, &AgentChatViewModel::onProtocolError);
    connect(m_socketClient, &AgentWebSocketClient::requestError,
            this, &AgentChatViewModel::onRequestError);
    connect(m_socketClient, &AgentWebSocketClient::sessionIdChanged,
            this, &AgentChatViewModel::sessionIdChanged);
    connect(m_socketClient, &AgentWebSocketClient::toolCallReceived,
            this, &AgentChatViewModel::onToolCallReceived);
    connect(m_socketClient, &AgentWebSocketClient::scriptValidationRequestReceived,
            this, &AgentChatViewModel::onScriptValidationRequestReceived);
    connect(m_socketClient, &AgentWebSocketClient::scriptDryRunRequestReceived,
            this, &AgentChatViewModel::onScriptDryRunRequestReceived);
    connect(m_socketClient, &AgentWebSocketClient::scriptExecutionRequestReceived,
            this, &AgentChatViewModel::onScriptExecutionRequestReceived);
    connect(m_socketClient, &AgentWebSocketClient::scriptCancelRequestReceived,
            this, &AgentChatViewModel::onScriptCancelRequestReceived);
    connect(m_socketClient, &AgentWebSocketClient::planPreviewReceived,
            this, &AgentChatViewModel::onPlanPreviewReceived);
    connect(m_socketClient, &AgentWebSocketClient::approvalRequestReceived,
            this, &AgentChatViewModel::onApprovalRequestReceived);
    connect(m_socketClient, &AgentWebSocketClient::clarificationRequestReceived,
            this, &AgentChatViewModel::onClarificationRequestReceived);
    connect(m_socketClient, &AgentWebSocketClient::progressReceived,
            this, &AgentChatViewModel::onProgressReceived);
    connect(m_socketClient, &AgentWebSocketClient::finalResultReceived,
            this, &AgentChatViewModel::onFinalResultReceived);

    connect(m_capabilityFacade, &AgentCapabilityFacade::capabilityResultReady,
            this, &AgentChatViewModel::onToolResultReady);
    connect(m_capabilityFacade, &AgentCapabilityFacade::capabilityProgress,
            this, [this](const QString& message) {
                appendSystemMessage(message, QStringLiteral("pending"));
            });
    connect(m_scriptExecutor, &AgentScriptExecutor::scriptExecutionStarted,
            this, &AgentChatViewModel::onScriptExecutionStarted);
    connect(m_scriptExecutor, &AgentScriptExecutor::scriptStepStarted,
            this, &AgentChatViewModel::onScriptStepStarted);
    connect(m_scriptExecutor, &AgentScriptExecutor::scriptStepFinished,
            this, &AgentChatViewModel::onScriptStepFinished);
    connect(m_scriptExecutor, &AgentScriptExecutor::scriptExecutionFinished,
            this, &AgentChatViewModel::onScriptExecutionFinished);
}

void AgentChatViewModel::setState(AgentConnectionState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit connectionStateChanged();
}

void AgentChatViewModel::setLastError(const QString& errorText, bool toast)
{
    if (m_lastError == errorText) {
        if (toast && !errorText.isEmpty()) {
            emit toastRequested(errorText);
        }
        return;
    }

    m_lastError = errorText;
    emit lastErrorChanged();
    if (toast && !errorText.isEmpty()) {
        emit toastRequested(errorText);
    }
}

void AgentChatViewModel::setLoadingSessions(bool loading)
{
    if (m_loadingSessions == loading) {
        return;
    }
    m_loadingSessions = loading;
    emit sessionLoadingChanged();
}

void AgentChatViewModel::setCurrentSession(const ChatSessionItem& session)
{
    const QString sid = session.sessionId.trimmed();
    const QString title = session.title.trimmed().isEmpty()
        ? QStringLiteral("新建会话")
        : session.title.trimmed();
    const bool idChanged = (m_currentSessionId != sid);
    const bool titleChanged = (m_currentSessionTitle != title);

    m_currentSessionId = sid;
    m_currentSessionTitle = title;

    if (idChanged) {
        emit sessionIdChanged();
    }
    if (idChanged || titleChanged) {
        emit currentSessionChanged();
    }
}

void AgentChatViewModel::clearCurrentSession()
{
    const bool hadSession = !m_currentSessionId.isEmpty() || !m_currentSessionTitle.isEmpty();
    m_currentSessionId.clear();
    m_currentSessionTitle.clear();
    if (hadSession) {
        emit sessionIdChanged();
        emit currentSessionChanged();
    }
}

void AgentChatViewModel::rebuildMessages(const QVector<ChatMessageItem>& messages)
{
    m_messageModel.clear();
    for (const ChatMessageItem& item : messages) {
        m_messageModel.appendMessage(item);
    }
}

void AgentChatViewModel::connectSocketForCurrentSession()
{
    if (m_currentSessionId.isEmpty()) {
        setState(AgentConnectionState::Idle);
        return;
    }

    if (!m_processManager->isRunning()) {
        initialize();
        return;
    }

    if (m_socketClient->isConnected() && m_socketClient->sessionId() == m_currentSessionId) {
        setState(AgentConnectionState::Ready);
        return;
    }

    if (m_socketClient->isConnected() && m_socketClient->sessionId() != m_currentSessionId) {
        m_manualDisconnect = true;
        m_socketClient->disconnectFromServer();
    }

    m_socketClient->setSessionId(m_currentSessionId);
    setState(AgentConnectionState::ConnectingSocket);
    m_socketClient->connectToServer();
}

void AgentChatViewModel::appendSystemMessage(const QString& text, const QString& status)
{
    ChatMessageItem item;
    item.id = QStringLiteral("sys-%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.role = QStringLiteral("system");
    item.messageType = QStringLiteral("text");
    item.text = text;
    item.rawText = text;
    item.timestamp = QDateTime::currentDateTime();
    item.status = status;
    m_messageModel.appendMessage(item);
}

void AgentChatViewModel::appendUserMessage(const QString& requestId, const QString& text)
{
    ChatMessageItem item;
    item.id = requestId;
    item.role = QStringLiteral("user");
    item.messageType = QStringLiteral("text");
    item.text = text;
    item.rawText = text;
    item.requestId = requestId;
    item.timestamp = QDateTime::currentDateTime();
    item.status = QStringLiteral("sent");
    m_messageModel.appendMessage(item);
}

void AgentChatViewModel::appendAssistantMessage(const QString& requestId, const QString& text)
{
    ChatMessageItem item;
    item.id = QStringLiteral("assistant-%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.role = QStringLiteral("assistant");
    item.messageType = QStringLiteral("text");
    item.text = text;
    item.rawText = text;
    item.requestId = requestId;
    item.timestamp = QDateTime::currentDateTime();
    item.status = QStringLiteral("done");
    m_messageModel.appendMessage(item);
}

void AgentChatViewModel::appendStructuredAssistantMessage(const QString& requestId,
                                                          const QString& messageType,
                                                          const QString& text,
                                                          const QVariantMap& meta,
                                                          const QString& status)
{
    ChatMessageItem item;
    item.id = QStringLiteral("assistant-%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.role = QStringLiteral("assistant");
    item.messageType = messageType.trimmed().isEmpty()
        ? QStringLiteral("text")
        : messageType.trimmed();
    item.text = text;
    item.rawText = text;
    item.meta = meta;
    item.requestId = requestId;
    item.timestamp = QDateTime::currentDateTime();
    item.status = status;
    m_messageModel.appendMessage(item);
}

void AgentChatViewModel::appendErrorMessage(const QString& requestId, const QString& text)
{
    ChatMessageItem item;
    item.id = QStringLiteral("error-%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.role = QStringLiteral("error");
    item.messageType = QStringLiteral("text");
    item.text = text;
    item.rawText = text;
    item.requestId = requestId;
    item.timestamp = QDateTime::currentDateTime();
    item.status = QStringLiteral("error");
    m_messageModel.appendMessage(item);
}

void AgentChatViewModel::rememberDebugScript(const QString& requestId,
                                             const QString& phase,
                                             const QString& scriptText)
{
    const QString trimmedRequestId = requestId.trimmed();
    const QString rawScript = scriptText;
    const QString trimmedScript = rawScript.trimmed();
    if (trimmedRequestId.isEmpty() || trimmedScript.isEmpty()) {
        return;
    }

    if (!m_debugRawScriptByRequestId.contains(trimmedRequestId)) {
        m_debugRawScriptByRequestId.insert(trimmedRequestId, rawScript);
    }

    QStringList& scripts = m_debugScriptsByRequestId[trimmedRequestId];
    scripts.append(QStringLiteral("[%1]\n%2").arg(phase, trimmedScript));
}

QString AgentChatViewModel::claimDebugRequestIdForDirectToolCall() const
{
    for (const QString& requestId : m_debugPendingRequestIds) {
        if (!requestId.trimmed().isEmpty()) {
            return requestId;
        }
    }
    return QString();
}

void AgentChatViewModel::rememberDirectToolCall(const QString& toolCallId,
                                                const QString& tool,
                                                const QVariantMap& args)
{
    const QString requestId = claimDebugRequestIdForDirectToolCall();
    if (requestId.isEmpty()) {
        return;
    }

    QString serializedArgs = QStringLiteral("{}");
    if (!args.isEmpty()) {
        serializedArgs = QString::fromUtf8(
            QJsonDocument(QJsonObject::fromVariantMap(args)).toJson(QJsonDocument::Compact));
    }

    QStringList& toolChain = m_debugDirectToolChainByRequestId[requestId];
    toolChain.append(QStringLiteral("toolCallId=%1 | tool=%2 | args=%3")
                         .arg(toolCallId.trimmed().isEmpty() ? QStringLiteral("<empty>") : toolCallId.trimmed(),
                              tool.trimmed().isEmpty() ? QStringLiteral("<empty>") : tool.trimmed(),
                              serializedArgs));
}

void AgentChatViewModel::logAgentConversationDebug(const QString& requestId)
{
    const QString trimmedRequestId = requestId.trimmed();
    if (trimmedRequestId.isEmpty()) {
        return;
    }

    const QString userPrompt = m_debugUserPromptByRequestId.value(trimmedRequestId);
    const QString assistantReply = m_debugAssistantReplyByRequestId.value(trimmedRequestId);
    const QString rawScript = m_debugRawScriptByRequestId.value(trimmedRequestId);
    const QStringList scripts = m_debugScriptsByRequestId.value(trimmedRequestId);
    const QStringList directToolChain = m_debugDirectToolChainByRequestId.value(trimmedRequestId);

    QStringList lines;
    lines << QStringLiteral("[AgentDebug] ===== AI 对话调试开始 =====")
          << QStringLiteral("[AgentDebug] requestId: %1").arg(trimmedRequestId)
          << QStringLiteral("[AgentDebug] sessionId: %1").arg(m_currentSessionId)
          << QStringLiteral("[AgentDebug] 用户问题:")
          << (userPrompt.isEmpty() ? QStringLiteral("<empty>") : userPrompt)
          << QStringLiteral("[AgentDebug] 原始脚本:")
          << (rawScript.trimmed().isEmpty() ? QStringLiteral("<none>") : rawScript)
          << QStringLiteral("[AgentDebug] Agent 回复:")
          << (assistantReply.isEmpty() ? QStringLiteral("<empty>") : assistantReply);

    if (scripts.isEmpty()) {
        lines << QStringLiteral("[AgentDebug] Agent 脚本: <none>");
    } else {
        lines << QStringLiteral("[AgentDebug] Agent 脚本数量: %1").arg(scripts.size());
        for (int index = 0; index < scripts.size(); ++index) {
            lines << QStringLiteral("[AgentDebug] 脚本 #%1").arg(index + 1)
                  << scripts.at(index);
        }
    }

    if (directToolChain.isEmpty()) {
        lines << QStringLiteral("[AgentDebug] 直接工具链: <none>");
    } else {
        lines << QStringLiteral("[AgentDebug] 直接工具链数量: %1").arg(directToolChain.size());
        for (int index = 0; index < directToolChain.size(); ++index) {
            lines << QStringLiteral("[AgentDebug] 工具链 #%1").arg(index + 1)
                  << directToolChain.at(index);
        }
    }

    if (scripts.isEmpty() && directToolChain.isEmpty()) {
        lines << QStringLiteral("[AgentDebug] 本轮执行路径: 无脚本、无工具链，可能为服务端阻断 / 澄清 / 纯聊天回复。");
    } else if (scripts.isEmpty() && !directToolChain.isEmpty()) {
        lines << QStringLiteral("[AgentDebug] 本轮执行路径: 未走脚本，直接走工具链 fallback。");
    } else if (!scripts.isEmpty() && directToolChain.isEmpty()) {
        lines << QStringLiteral("[AgentDebug] 本轮执行路径: 走脚本链路。");
    } else {
        lines << QStringLiteral("[AgentDebug] 本轮执行路径: 脚本与直接工具链均出现，请重点核对服务端执行载体路由。");
    }

    lines << QStringLiteral("[AgentDebug] ===== AI 对话调试结束 =====");
    qDebug().noquote() << lines.join(QLatin1Char('\n'));
}

void AgentChatViewModel::clearDebugTrace(const QString& requestId)
{
    const QString trimmedRequestId = requestId.trimmed();
    if (trimmedRequestId.isEmpty()) {
        return;
    }

    m_debugUserPromptByRequestId.remove(trimmedRequestId);
    m_debugAssistantReplyByRequestId.remove(trimmedRequestId);
    m_debugRawScriptByRequestId.remove(trimmedRequestId);
    m_debugScriptsByRequestId.remove(trimmedRequestId);
    m_debugDirectToolChainByRequestId.remove(trimmedRequestId);
    m_debugPendingRequestIds.removeAll(trimmedRequestId);
}

QString AgentChatViewModel::nextRequestId()
{
    ++m_requestCounter;
    return QStringLiteral("req-%1").arg(m_requestCounter);
}

void AgentChatViewModel::flushPendingChunksForRequest(const QString& requestId)
{
    if (requestId.trimmed().isEmpty()) {
        return;
    }
    const QString delta = m_pendingChunkByRequestId.take(requestId);
    if (delta.isEmpty()) {
        return;
    }
    m_messageModel.appendRawDeltaAndReparse(requestId, delta);
}
