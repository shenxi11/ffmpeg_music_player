#ifndef AGENT_CHAT_VIEW_MODEL_H
#define AGENT_CHAT_VIEW_MODEL_H

#include <QObject>
#include <QProcess>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "AgentConnectionState.h"
#include "ChatMessageListModel.h"
#include "ChatSessionListModel.h"

class AgentProcessManager;
class AgentSessionService;
class AgentWebSocketClient;
class AgentCapabilityFacade;
class AgentScriptExecutor;
class HostStateProvider;
class MainShellViewModel;

/**
 * @brief AI 聊天业务编排层，聚合进程管理、连接管理与消息模型。
 */
class AgentChatViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ChatMessageListModel* messageModel READ messageModel CONSTANT)
    Q_PROPERTY(ChatSessionListModel* sessionModel READ sessionModel CONSTANT)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QString currentSessionId READ currentSessionId NOTIFY currentSessionChanged)
    Q_PROPERTY(QString currentSessionTitle READ currentSessionTitle NOTIFY currentSessionChanged)
    Q_PROPERTY(bool loadingSessions READ loadingSessions NOTIFY sessionLoadingChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY connectionStateChanged)

public:
    explicit AgentChatViewModel(QObject* parent = nullptr);
    ~AgentChatViewModel() override;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void retryConnection();
    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void loadSessions(const QString& query = QString());
    Q_INVOKABLE void searchSessions(const QString& query);
    Q_INVOKABLE void createSession(const QString& title = QString());
    Q_INVOKABLE void selectSession(const QString& sessionId);
    Q_INVOKABLE void renameSession(const QString& sessionId, const QString& title);
    Q_INVOKABLE void deleteSession(const QString& sessionId);
    Q_INVOKABLE void refreshCurrentSession();
    Q_INVOKABLE void sendApprovalResponse(const QString& planId,
                                          bool approved,
                                          const QString& reason = QString());

    Q_INVOKABLE void startNewConversation();
    Q_INVOKABLE QVariantList exportMessages() const;
    Q_INVOKABLE void importMessages(const QVariantList& messages);
    Q_INVOKABLE void switchToSession(const QString& sessionId);
    Q_INVOKABLE void handleWindowClosed();
    Q_INVOKABLE void shutdownForAppExit();
    Q_INVOKABLE QVariantMap validateClientScript(const QString& scriptText) const;
    Q_INVOKABLE QVariantMap dryRunClientScript(const QString& scriptText) const;
    Q_INVOKABLE QString executeClientScript(const QString& scriptText);
    void setMainShellViewModel(MainShellViewModel* shellViewModel);

    ChatMessageListModel* messageModel() { return &m_messageModel; }
    ChatSessionListModel* sessionModel() { return &m_sessionModel; }
    QString connectionStateText() const;
    QString sessionId() const;
    QString currentSessionId() const { return m_currentSessionId; }
    QString currentSessionTitle() const { return m_currentSessionTitle; }
    bool loadingSessions() const { return m_loadingSessions; }
    QString lastError() const { return m_lastError; }
    bool isReady() const { return m_state == AgentConnectionState::Ready; }

signals:
    void connectionStateChanged();
    void sessionIdChanged();
    void currentSessionChanged();
    void sessionLoadingChanged();
    void lastErrorChanged();
    void toastRequested(const QString& message);

private slots:
    void onAgentStarted();
    void onAgentStartFailed(const QString& reason);
    void onAgentExited(int exitCode, QProcess::ExitStatus exitStatus);
    void onAgentStdOut(const QString& text);
    void onAgentStdErr(const QString& text);
    void onHealthWarning(const QString& message);
    void onHealthInfoUpdated(const QVariantMap& healthInfo);
    void onSessionsLoaded(const QVector<ChatSessionItem>& sessions);
    void onSessionCreated(const ChatSessionItem& session);
    void onSessionLoaded(const ChatSessionItem& session);
    void onSessionUpdated(const ChatSessionItem& session);
    void onSessionDeleted(const QString& sessionId);
    void onSessionMessagesLoaded(const ChatSessionItem& session,
                                 const QVector<ChatMessageItem>& messages);
    void onSessionRequestFailed(const QString& operation, const QString& errorMessage);

    void onSocketConnected();
    void onSocketDisconnected();
    void onSessionReady(const QString& sessionId);
    void onSessionReadyDetailed(const QString& sessionId,
                                const QString& title,
                                const QStringList& capabilities,
                                const QVariantMap& payload);
    void onAssistantStartReceived(const QString& requestId);
    void onAssistantChunkReceived(const QString& requestId, const QString& delta);
    void onAssistantFinalReceived(const QString& requestId, const QString& content);
    void onAssistantMessageReceived(const QString& requestId, const QString& content);
    void onPlanPreviewReceived(const QString& planId, const QVariantMap& payload);
    void onApprovalRequestReceived(const QString& planId,
                                   const QString& message,
                                   const QVariantMap& payload);
    void onClarificationRequestReceived(const QString& requestId,
                                        const QString& question,
                                        const QStringList& options,
                                        const QVariantMap& payload);
    void onProgressReceived(const QString& message, const QVariantMap& payload);
    void onFinalResultReceived(const QVariantMap& payload);
    void onProtocolError(const QString& code, const QString& message);
    void onRequestError(const QString& requestId, const QString& code, const QString& message);
    void onToolCallReceived(const QString& toolCallId, const QString& tool, const QVariantMap& args);
    void onToolResultReady(const QString& toolCallId,
                           bool ok,
                           const QVariantMap& result,
                           const QVariantMap& error);
    void onScriptValidationRequestReceived(const QString& requestId,
                                           const QString& scriptText,
                                           const QVariantMap& payload);
    void onScriptDryRunRequestReceived(const QString& requestId,
                                       const QString& scriptText,
                                       const QVariantMap& payload);
    void onScriptExecutionRequestReceived(const QString& requestId,
                                          const QString& scriptText,
                                          const QVariantMap& payload);
    void onScriptCancelRequestReceived(const QString& requestId,
                                       const QString& executionId,
                                       const QVariantMap& payload);
    void onScriptExecutionStarted(const QString& executionId, const QVariantMap& summary);
    void onScriptStepStarted(const QString& executionId,
                             int stepIndex,
                             const QVariantMap& stepSummary);
    void onScriptStepFinished(const QString& executionId,
                              int stepIndex,
                              bool ok,
                              const QVariantMap& payload);
    void onScriptExecutionFinished(const QString& executionId,
                                   bool ok,
                                   const QVariantMap& report);
    void onChunkFlushTimeout();

private:
    void setupConnections();
    void setState(AgentConnectionState state);
    void setLastError(const QString& errorText, bool toast = false);
    void setLoadingSessions(bool loading);
    void setCurrentSession(const ChatSessionItem& session);
    void clearCurrentSession();
    void rebuildMessages(const QVector<ChatMessageItem>& messages);
    void connectSocketForCurrentSession();
    void appendSystemMessage(const QString& text, const QString& status = QStringLiteral("done"));
    void appendUserMessage(const QString& requestId, const QString& text);
    void appendAssistantMessage(const QString& requestId, const QString& text);
    void appendStructuredAssistantMessage(const QString& requestId,
                                          const QString& messageType,
                                          const QString& text,
                                          const QVariantMap& meta,
                                          const QString& status = QStringLiteral("done"));
    void appendErrorMessage(const QString& requestId, const QString& text);
    void rememberDebugScript(const QString& requestId,
                             const QString& phase,
                             const QString& scriptText);
    QString claimDebugRequestIdForDirectToolCall() const;
    void rememberDirectToolCall(const QString& toolCallId,
                                const QString& tool,
                                const QVariantMap& args);
    void logAgentConversationDebug(const QString& requestId);
    void clearDebugTrace(const QString& requestId);
    QString nextRequestId();
    void flushPendingChunksForRequest(const QString& requestId);

private:
    AgentProcessManager* m_processManager = nullptr;
    AgentSessionService* m_sessionService = nullptr;
    AgentWebSocketClient* m_socketClient = nullptr;
    HostStateProvider* m_hostStateProvider = nullptr;
    AgentCapabilityFacade* m_capabilityFacade = nullptr;
    AgentScriptExecutor* m_scriptExecutor = nullptr;
    ChatMessageListModel m_messageModel;
    ChatSessionListModel m_sessionModel;

    AgentConnectionState m_state = AgentConnectionState::Idle;
    int m_requestCounter = 0;
    bool m_manualDisconnect = false;
    bool m_appExiting = false;

    QString m_lastError;
    QString m_currentSessionId;
    QString m_currentSessionTitle;
    QString m_lastSessionQuery;
    QString m_pendingMessageSessionId;
    bool m_loadingSessions = false;
    bool m_initialSessionsLoaded = false;
    QString m_protocolVersion;
    QSet<QString> m_capabilities;
    bool m_toolsEnabledByHealth = true;
    bool m_toolsEnabledBySession = true;
    bool m_toolsEnabled = true;
    QHash<QString, QString> m_pendingChunkByRequestId;
    QHash<QString, QString> m_scriptRequestIdByExecutionId;
    QHash<QString, QString> m_debugUserPromptByRequestId;
    QHash<QString, QString> m_debugAssistantReplyByRequestId;
    QHash<QString, QString> m_debugRawScriptByRequestId;
    QHash<QString, QStringList> m_debugScriptsByRequestId;
    QHash<QString, QStringList> m_debugDirectToolChainByRequestId;
    QStringList m_debugPendingRequestIds;
    QTimer m_chunkFlushTimer;
};

#endif // AGENT_CHAT_VIEW_MODEL_H
