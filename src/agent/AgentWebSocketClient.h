#ifndef AGENT_WEBSOCKET_CLIENT_H
#define AGENT_WEBSOCKET_CLIENT_H

#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include <QVariantMap>
#include <QWebSocket>

class AgentProtocolRouter;

/**
 * @brief 封装 Qt 侧与 Agent 的 WebSocket 通信协议。
 */
class AgentWebSocketClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)

public:
    explicit AgentWebSocketClient(QObject* parent = nullptr);

    void connectToServer();
    void reconnect();
    void disconnectFromServer();

    bool isConnected() const { return m_connected; }
    QString sessionId() const { return m_sessionId; }

    void setSessionId(const QString& sessionId);
    void clearSession();

    void sendHostSnapshot(const QVariantMap& hostContext,
                          const QVariantList& capabilities,
                          const QString& catalogVersion = QString());
    void sendUserMessage(const QString& content, const QString& requestId);
    void sendToolResult(const QString& toolCallId,
                        bool ok,
                        const QVariantMap& result,
                        const QVariantMap& error = QVariantMap());
    void sendApprovalResponse(const QString& planId,
                              bool approved,
                              const QString& reason = QString());
    void sendScriptValidationResult(const QString& requestId,
                                    bool ok,
                                    const QVariantMap& result,
                                    const QVariantMap& error = QVariantMap());
    void sendScriptDryRunResult(const QString& requestId,
                                bool ok,
                                const QVariantMap& result,
                                const QVariantMap& error = QVariantMap());
    void sendScriptExecutionStarted(const QString& requestId,
                                    const QString& executionId,
                                    const QVariantMap& summary);
    void sendScriptStepEvent(const QString& requestId,
                             const QString& executionId,
                             int stepIndex,
                             const QString& status,
                             const QVariantMap& payload);
    void sendScriptExecutionResult(const QString& requestId,
                                   const QString& executionId,
                                   bool ok,
                                   const QVariantMap& report,
                                   const QVariantMap& error = QVariantMap());
    void sendScriptCancellationResult(const QString& requestId,
                                      const QString& executionId,
                                      bool ok,
                                      const QVariantMap& result,
                                      const QVariantMap& error = QVariantMap());

signals:
    void connectionChanged();
    void sessionIdChanged();

    void connected();
    void disconnected();
    void sessionReady(const QString& sessionId);
    void sessionReadyDetailed(const QString& sessionId,
                              const QString& title,
                              const QStringList& capabilities,
                              const QVariantMap& payload);
    void assistantStartReceived(const QString& requestId);
    void assistantChunkReceived(const QString& requestId, const QString& delta);
    void assistantFinalReceived(const QString& requestId, const QString& content);
    void assistantMessageReceived(const QString& requestId, const QString& content);
    void protocolError(const QString& code, const QString& message);
    void requestError(const QString& requestId, const QString& code, const QString& message);
    void planPreviewReceived(const QString& planId, const QVariantMap& payload);
    void approvalRequestReceived(const QString& planId, const QString& message, const QVariantMap& payload);
    void clarificationRequestReceived(const QString& requestId,
                                      const QString& question,
                                      const QStringList& options,
                                      const QVariantMap& payload);
    void progressReceived(const QString& message, const QVariantMap& payload);
    void finalResultReceived(const QVariantMap& payload);
    void toolCallReceived(const QString& toolCallId, const QString& tool, const QVariantMap& args);
    void scriptValidationRequestReceived(const QString& requestId,
                                         const QString& scriptText,
                                         const QVariantMap& payload);
    void scriptDryRunRequestReceived(const QString& requestId,
                                     const QString& scriptText,
                                     const QVariantMap& payload);
    void scriptExecutionRequestReceived(const QString& requestId,
                                        const QString& scriptText,
                                        const QVariantMap& payload);
    void scriptCancelRequestReceived(const QString& requestId,
                                     const QString& executionId,
                                     const QVariantMap& payload);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketTextMessageReceived(const QString& text);
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QUrl buildUrl() const;
    void setConnected(bool connected);
    bool sendJsonPayload(const QJsonObject& payload,
                         const QString& requestIdForError = QString(),
                         const QString& actionName = QString());

private:
    QWebSocket m_socket;
    AgentProtocolRouter* m_router = nullptr;
    QString m_sessionId;
    bool m_connected = false;
};

#endif // AGENT_WEBSOCKET_CLIENT_H
