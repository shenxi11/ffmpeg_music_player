#ifndef AGENT_LOCAL_MODEL_GATEWAY_H
#define AGENT_LOCAL_MODEL_GATEWAY_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class AgentLocalModelGateway : public QObject
{
    Q_OBJECT

public:
    explicit AgentLocalModelGateway(QObject* parent = nullptr);

    void compileIntent(const QString& requestId,
                       const QString& userMessage,
                       const QVariantMap& hostContext,
                       const QVariantList& capabilityItems,
                       const QVariantMap& memorySnapshot);
    void generateAssistantReply(const QString& requestId,
                                const QString& userMessage,
                                const QVariantMap& hostContext);

signals:
    void controlIntentReady(const QString& requestId, const QVariantMap& intentPayload);
    void assistantReplyReady(const QString& requestId, const QString& text);
    void requestFailed(const QString& requestId, const QString& code, const QString& message);

private:
    QVariantMap buildControlCapabilityProjection(const QVariantList& capabilityItems) const;
    QVariantMap extractJsonObject(const QString& text, QString* errorMessage) const;
};

#endif // AGENT_LOCAL_MODEL_GATEWAY_H
