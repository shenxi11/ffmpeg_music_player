#ifndef AGENT_PROTOCOL_ROUTER_H
#define AGENT_PROTOCOL_ROUTER_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>

/**
 * @brief Agent 协议路由器，负责将 WebSocket 文本消息解析为结构化信号。
 */
class AgentProtocolRouter : public QObject
{
    Q_OBJECT

public:
    explicit AgentProtocolRouter(QObject* parent = nullptr);

    void parseMessage(const QString& text);

signals:
    void sessionReady(const QString& sessionId);
    void sessionReadyDetailed(const QString& sessionId,
                              const QString& title,
                              const QStringList& capabilities,
                              const QVariantMap& payload);
    void assistantStartReceived(const QString& requestId);
    void assistantChunkReceived(const QString& requestId, const QString& delta);
    void assistantFinalReceived(const QString& requestId, const QString& content);
    void assistantMessageReceived(const QString& requestId, const QString& content);
    void requestError(const QString& requestId, const QString& code, const QString& message);
    void protocolError(const QString& code, const QString& message);

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
};

#endif // AGENT_PROTOCOL_ROUTER_H
