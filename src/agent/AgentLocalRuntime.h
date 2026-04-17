#ifndef AGENT_LOCAL_RUNTIME_H
#define AGENT_LOCAL_RUNTIME_H

#include <QObject>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>

class AgentCapabilityFacade;
class AgentLocalModelGateway;

class AgentLocalRuntime : public QObject
{
    Q_OBJECT

public:
    explicit AgentLocalRuntime(AgentCapabilityFacade* capabilityFacade, QObject* parent = nullptr);

    void sendUserMessage(const QString& sessionId,
                         const QString& requestId,
                         const QString& userMessage,
                         const QVariantMap& hostContext,
                         const QVariantList& capabilityItems);
    void sendApprovalResponse(const QString& sessionId,
                              const QString& planId,
                              bool approved,
                              const QString& reason = QString());

signals:
    void assistantStartReceived(const QString& requestId);
    void assistantFinalReceived(const QString& requestId, const QString& content);
    void planPreviewReceived(const QString& planId, const QVariantMap& payload);
    void approvalRequestReceived(const QString& planId,
                                 const QString& message,
                                 const QVariantMap& payload);
    void progressReceived(const QString& message, const QVariantMap& payload);
    void finalResultReceived(const QVariantMap& payload);
    void requestError(const QString& requestId, const QString& code, const QString& message);

private slots:
    void onControlIntentReady(const QString& requestId, const QVariantMap& intentPayload);
    void onAssistantReplyReady(const QString& requestId, const QString& text);
    void onModelRequestFailed(const QString& requestId, const QString& code, const QString& message);
    void onCapabilityResultReady(const QString& toolCallId,
                                 bool ok,
                                 const QVariantMap& result,
                                 const QVariantMap& error);

private:
    struct PendingPlan
    {
        QString sessionId;
        QString requestId;
        QString userMessage;
        QString planId;
        QString intent;
        QVariantMap arguments;
        QVariantMap hostContext;
        QString toolCallId;
        QString currentTool;
        QVariantMap currentArgs;
        QVariantMap cachedSearchResult;
        QVariantMap cachedPlaylist;
        bool awaitingApproval = false;
    };

    QVariantMap matchFastIntent(const QString& userMessage) const;
    bool isReadOnlyIntent(const QVariantMap& intentPayload) const;
    bool dispatchIntent(const QString& sessionId,
                        const QString& requestId,
                        const QString& userMessage,
                        const QVariantMap& hostContext,
                        const QVariantMap& intentPayload);
    void dispatchToolPlan(PendingPlan plan,
                          const QString& tool,
                          const QVariantMap& args,
                          const QString& progressMessage = QString());
    void completePlanSuccess(const PendingPlan& plan, const QString& summary);
    void completePlanFailure(const PendingPlan& plan, const QString& summary);
    QString summarizeUiOverview(const QVariantMap& result) const;
    QString summarizeUiPageState(const QVariantMap& result) const;
    QString summarizeMusicTabItems(const PendingPlan& plan, const QVariantMap& result) const;
    QString summarizeMusicTabItem(const QVariantMap& result) const;
    QString summarizePlayedMusicTabTrack(const QVariantMap& result) const;
    QString summarizeMusicTabAction(const QVariantMap& result) const;
    QString summarizeCurrentPlayback(const QVariantMap& result) const;
    QString summarizeLocalTracks(const QVariantMap& result) const;
    QString summarizePlaylistTracks(const QVariantMap& result) const;
    QVariantMap resolvePlaylistSelection(const QVariantList& playlists,
                                         const QVariantMap& arguments,
                                         const QVariantMap& hostContext,
                                         QString* errorMessage) const;

private:
    AgentCapabilityFacade* m_capabilityFacade = nullptr;
    AgentLocalModelGateway* m_modelGateway = nullptr;
    QHash<QString, QVariantMap> m_pendingCompileContext;
    QHash<QString, PendingPlan> m_pendingPlanByToolCallId;
    QHash<QString, PendingPlan> m_pendingApprovalByPlanId;
};

#endif // AGENT_LOCAL_RUNTIME_H
