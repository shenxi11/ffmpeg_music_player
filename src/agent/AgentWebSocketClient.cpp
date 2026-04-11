#include "AgentWebSocketClient.h"

#include "protocol/AgentProtocolRouter.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QUrlQuery>

AgentWebSocketClient::AgentWebSocketClient(QObject* parent)
    : QObject(parent)
    , m_router(new AgentProtocolRouter(this))
{
    connect(&m_socket, &QWebSocket::connected,
            this, &AgentWebSocketClient::onSocketConnected);
    connect(&m_socket, &QWebSocket::disconnected,
            this, &AgentWebSocketClient::onSocketDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &AgentWebSocketClient::onSocketTextMessageReceived);
    connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &AgentWebSocketClient::onSocketError);

    connect(m_router, &AgentProtocolRouter::sessionReady,
            this, [this](const QString& sid) {
                setSessionId(sid);
                emit sessionReady(sid);
            });
    connect(m_router, &AgentProtocolRouter::sessionReadyDetailed,
            this, &AgentWebSocketClient::sessionReadyDetailed);
    connect(m_router, &AgentProtocolRouter::assistantStartReceived,
            this, &AgentWebSocketClient::assistantStartReceived);
    connect(m_router, &AgentProtocolRouter::assistantChunkReceived,
            this, &AgentWebSocketClient::assistantChunkReceived);
    connect(m_router, &AgentProtocolRouter::assistantFinalReceived,
            this, &AgentWebSocketClient::assistantFinalReceived);
    connect(m_router, &AgentProtocolRouter::assistantMessageReceived,
            this, &AgentWebSocketClient::assistantMessageReceived);
    connect(m_router, &AgentProtocolRouter::requestError,
            this, &AgentWebSocketClient::requestError);
    connect(m_router, &AgentProtocolRouter::protocolError,
            this, &AgentWebSocketClient::protocolError);
    connect(m_router, &AgentProtocolRouter::planPreviewReceived,
            this, &AgentWebSocketClient::planPreviewReceived);
    connect(m_router, &AgentProtocolRouter::approvalRequestReceived,
            this, &AgentWebSocketClient::approvalRequestReceived);
    connect(m_router, &AgentProtocolRouter::clarificationRequestReceived,
            this, &AgentWebSocketClient::clarificationRequestReceived);
    connect(m_router, &AgentProtocolRouter::progressReceived,
            this, &AgentWebSocketClient::progressReceived);
    connect(m_router, &AgentProtocolRouter::finalResultReceived,
            this, &AgentWebSocketClient::finalResultReceived);
    connect(m_router, &AgentProtocolRouter::toolCallReceived,
            this, &AgentWebSocketClient::toolCallReceived);
    connect(m_router, &AgentProtocolRouter::scriptValidationRequestReceived,
            this, &AgentWebSocketClient::scriptValidationRequestReceived);
    connect(m_router, &AgentProtocolRouter::scriptDryRunRequestReceived,
            this, &AgentWebSocketClient::scriptDryRunRequestReceived);
    connect(m_router, &AgentProtocolRouter::scriptExecutionRequestReceived,
            this, &AgentWebSocketClient::scriptExecutionRequestReceived);
    connect(m_router, &AgentProtocolRouter::scriptCancelRequestReceived,
            this, &AgentWebSocketClient::scriptCancelRequestReceived);
}

void AgentWebSocketClient::connectToServer()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        if (!m_connected) {
            setConnected(true);
            emit connected();
        }
        return;
    }

    if (m_socket.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    const QUrl url = buildUrl();
    qDebug() << "[AgentWs] connect to" << url;
    m_socket.open(url);
}

void AgentWebSocketClient::reconnect()
{
    disconnectFromServer();
    QMetaObject::invokeMethod(this, [this]() {
        connectToServer();
    }, Qt::QueuedConnection);
}

void AgentWebSocketClient::disconnectFromServer()
{
    if (m_socket.state() == QAbstractSocket::UnconnectedState) {
        setConnected(false);
        return;
    }
    m_socket.close();
}

void AgentWebSocketClient::setSessionId(const QString& sessionId)
{
    const QString trimmed = sessionId.trimmed();
    if (m_sessionId == trimmed) {
        return;
    }
    m_sessionId = trimmed;
    emit sessionIdChanged();
}

void AgentWebSocketClient::clearSession()
{
    setSessionId(QString());
}

void AgentWebSocketClient::sendUserMessage(const QString& content, const QString& requestId)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        emit requestError(requestId,
                          QStringLiteral("invalid_message"),
                          QStringLiteral("消息内容不能为空。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("user_message"));
    obj.insert(QStringLiteral("content"), trimmed);
    if (!requestId.trimmed().isEmpty()) {
        obj.insert(QStringLiteral("requestId"), requestId.trimmed());
    }
    sendJsonPayload(obj, requestId, QStringLiteral("user_message"));
}

void AgentWebSocketClient::sendToolResult(const QString& toolCallId,
                                          bool ok,
                                          const QVariantMap& result,
                                          const QVariantMap& error)
{
    const QString trimmedId = toolCallId.trimmed();
    if (trimmedId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_tool_result"),
                           QStringLiteral("tool_result 缺少 toolCallId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("tool_result"));
    obj.insert(QStringLiteral("toolCallId"), trimmedId);
    obj.insert(QStringLiteral("ok"), ok);
    if (ok) {
        obj.insert(QStringLiteral("result"), QJsonObject::fromVariantMap(result));
    } else {
        obj.insert(QStringLiteral("error"), QJsonObject::fromVariantMap(error));
    }
    sendJsonPayload(obj, trimmedId, QStringLiteral("tool_result"));
}

void AgentWebSocketClient::sendApprovalResponse(const QString& planId,
                                                bool approved,
                                                const QString& reason)
{
    const QString trimmedPlanId = planId.trimmed();
    if (trimmedPlanId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_approval_response"),
                           QStringLiteral("approval_response 缺少 planId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("approval_response"));
    obj.insert(QStringLiteral("planId"), trimmedPlanId);
    obj.insert(QStringLiteral("approved"), approved);
    if (!reason.trimmed().isEmpty()) {
        obj.insert(QStringLiteral("reason"), reason.trimmed());
    }
    sendJsonPayload(obj, trimmedPlanId, QStringLiteral("approval_response"));
}

void AgentWebSocketClient::sendScriptValidationResult(const QString& requestId,
                                                      bool ok,
                                                      const QVariantMap& result,
                                                      const QVariantMap& error)
{
    const QString trimmedRequestId = requestId.trimmed();
    if (trimmedRequestId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_script_validation_result"),
                           QStringLiteral("script_validation_result 缺少 requestId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("script_validation_result"));
    obj.insert(QStringLiteral("requestId"), trimmedRequestId);
    obj.insert(QStringLiteral("ok"), ok);
    if (ok) {
        obj.insert(QStringLiteral("result"), QJsonObject::fromVariantMap(result));
    } else {
        obj.insert(QStringLiteral("error"), QJsonObject::fromVariantMap(error));
    }
    sendJsonPayload(obj, trimmedRequestId, QStringLiteral("script_validation_result"));
}

void AgentWebSocketClient::sendScriptDryRunResult(const QString& requestId,
                                                  bool ok,
                                                  const QVariantMap& result,
                                                  const QVariantMap& error)
{
    const QString trimmedRequestId = requestId.trimmed();
    if (trimmedRequestId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_script_dry_run_result"),
                           QStringLiteral("script_dry_run_result 缺少 requestId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("script_dry_run_result"));
    obj.insert(QStringLiteral("requestId"), trimmedRequestId);
    obj.insert(QStringLiteral("ok"), ok);
    if (ok) {
        obj.insert(QStringLiteral("result"), QJsonObject::fromVariantMap(result));
    } else {
        obj.insert(QStringLiteral("error"), QJsonObject::fromVariantMap(error));
    }
    sendJsonPayload(obj, trimmedRequestId, QStringLiteral("script_dry_run_result"));
}

void AgentWebSocketClient::sendScriptExecutionStarted(const QString& requestId,
                                                      const QString& executionId,
                                                      const QVariantMap& summary)
{
    const QString trimmedRequestId = requestId.trimmed();
    const QString trimmedExecutionId = executionId.trimmed();
    if (trimmedRequestId.isEmpty() || trimmedExecutionId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_script_execution_started"),
                           QStringLiteral("script_execution_started 缺少 requestId 或 executionId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("script_execution_started"));
    obj.insert(QStringLiteral("requestId"), trimmedRequestId);
    obj.insert(QStringLiteral("executionId"), trimmedExecutionId);
    obj.insert(QStringLiteral("summary"), QJsonObject::fromVariantMap(summary));
    sendJsonPayload(obj, trimmedRequestId, QStringLiteral("script_execution_started"));
}

void AgentWebSocketClient::sendScriptStepEvent(const QString& requestId,
                                               const QString& executionId,
                                               int stepIndex,
                                               const QString& status,
                                               const QVariantMap& payload)
{
    const QString trimmedRequestId = requestId.trimmed();
    const QString trimmedExecutionId = executionId.trimmed();
    const QString trimmedStatus = status.trimmed();
    if (trimmedRequestId.isEmpty() || trimmedExecutionId.isEmpty() || trimmedStatus.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_script_step_event"),
                           QStringLiteral("script_step_event 缺少 requestId、executionId 或 status。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("script_step_event"));
    obj.insert(QStringLiteral("requestId"), trimmedRequestId);
    obj.insert(QStringLiteral("executionId"), trimmedExecutionId);
    obj.insert(QStringLiteral("stepIndex"), stepIndex);
    obj.insert(QStringLiteral("status"), trimmedStatus);
    obj.insert(QStringLiteral("payload"), QJsonObject::fromVariantMap(payload));
    sendJsonPayload(obj, trimmedRequestId, QStringLiteral("script_step_event"));
}

void AgentWebSocketClient::sendScriptExecutionResult(const QString& requestId,
                                                     const QString& executionId,
                                                     bool ok,
                                                     const QVariantMap& report,
                                                     const QVariantMap& error)
{
    const QString trimmedRequestId = requestId.trimmed();
    const QString trimmedExecutionId = executionId.trimmed();
    if (trimmedRequestId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_script_execution_result"),
                           QStringLiteral("script_execution_result 缺少 requestId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("script_execution_result"));
    obj.insert(QStringLiteral("requestId"), trimmedRequestId);
    if (!trimmedExecutionId.isEmpty()) {
        obj.insert(QStringLiteral("executionId"), trimmedExecutionId);
    }
    obj.insert(QStringLiteral("ok"), ok);
    if (ok) {
        obj.insert(QStringLiteral("report"), QJsonObject::fromVariantMap(report));
    } else {
        obj.insert(QStringLiteral("error"), QJsonObject::fromVariantMap(error));
    }
    sendJsonPayload(obj, trimmedRequestId, QStringLiteral("script_execution_result"));
}

void AgentWebSocketClient::sendScriptCancellationResult(const QString& requestId,
                                                        const QString& executionId,
                                                        bool ok,
                                                        const QVariantMap& result,
                                                        const QVariantMap& error)
{
    const QString trimmedRequestId = requestId.trimmed();
    const QString trimmedExecutionId = executionId.trimmed();
    if (trimmedRequestId.isEmpty() || trimmedExecutionId.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_script_cancellation_result"),
                           QStringLiteral("script_cancellation_result 缺少 requestId 或 executionId。"));
        return;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("script_cancellation_result"));
    obj.insert(QStringLiteral("requestId"), trimmedRequestId);
    obj.insert(QStringLiteral("executionId"), trimmedExecutionId);
    obj.insert(QStringLiteral("ok"), ok);
    if (ok) {
        obj.insert(QStringLiteral("result"), QJsonObject::fromVariantMap(result));
    } else {
        obj.insert(QStringLiteral("error"), QJsonObject::fromVariantMap(error));
    }
    sendJsonPayload(obj, trimmedRequestId, QStringLiteral("script_cancellation_result"));
}

void AgentWebSocketClient::onSocketConnected()
{
    qDebug() << "[AgentWs] connected, sessionId =" << m_sessionId;
    setConnected(true);
    emit connected();
}

void AgentWebSocketClient::onSocketDisconnected()
{
    qDebug() << "[AgentWs] disconnected";
    const bool wasConnected = m_connected;
    setConnected(false);
    if (wasConnected) {
        emit disconnected();
    }
}

void AgentWebSocketClient::onSocketTextMessageReceived(const QString& text)
{
    qDebug().noquote() << "[AgentWs] recv:" << text;
    if (!m_router) {
        emit protocolError(QStringLiteral("router_unavailable"),
                           QStringLiteral("协议路由器不可用。"));
        return;
    }
    m_router->parseMessage(text);
}

void AgentWebSocketClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qWarning() << "[AgentWs] socket error:" << m_socket.errorString();
    emit protocolError(QStringLiteral("socket_error"), m_socket.errorString());
}

QUrl AgentWebSocketClient::buildUrl() const
{
    QUrl url(QStringLiteral("ws://127.0.0.1:8765/ws/chat"));
    if (m_sessionId.trimmed().isEmpty()) {
        return url;
    }

    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("session_id"), m_sessionId.trimmed());
    url.setQuery(query);
    return url;
}

void AgentWebSocketClient::setConnected(bool connected)
{
    if (m_connected == connected) {
        return;
    }
    m_connected = connected;
    emit connectionChanged();
}

bool AgentWebSocketClient::sendJsonPayload(const QJsonObject& payload,
                                           const QString& requestIdForError,
                                           const QString& actionName)
{
    if (!m_connected || m_socket.state() != QAbstractSocket::ConnectedState) {
        emit requestError(requestIdForError,
                          QStringLiteral("socket_not_connected"),
                          QStringLiteral("尚未连接到 AI 助手，无法发送 %1。").arg(actionName));
        return false;
    }

    const QString text = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_socket.sendTextMessage(text);
    return true;
}
