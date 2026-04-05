#ifndef AGENT_SCRIPT_EXECUTOR_H
#define AGENT_SCRIPT_EXECUTOR_H

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class AgentCapabilityFacade;
class ToolRegistry;

/**
 * @brief Agent 脚本执行器，负责顺序执行受控脚本步骤并复用现有工具执行链路。
 *
 * 第一阶段目标不是替代所有工具协议，而是在客户端内部形成统一脚本入口，
 * 让后续 Agent 可以把多步任务收束成脚本，再由客户端顺序执行和返回结构化结果。
 */
class AgentScriptExecutor : public QObject
{
    Q_OBJECT

public:
    explicit AgentScriptExecutor(AgentCapabilityFacade* capabilityFacade, QObject* parent = nullptr);
    ~AgentScriptExecutor() override;

    QVariantMap validateScript(const QString& scriptText) const;
    QVariantMap dryRunScript(const QString& scriptText) const;
    QString executeScript(const QString& scriptText);
    bool cancelExecution(const QString& executionId,
                         const QString& reason = QStringLiteral("脚本执行已取消"));

signals:
    void scriptExecutionStarted(const QString& executionId, const QVariantMap& summary);
    void scriptStepStarted(const QString& executionId, int stepIndex, const QVariantMap& stepSummary);
    void scriptStepFinished(const QString& executionId,
                            int stepIndex,
                            bool ok,
                            const QVariantMap& payload);
    void scriptExecutionFinished(const QString& executionId,
                                 bool ok,
                                 const QVariantMap& report);

private slots:
    void onToolResultReady(const QString& toolCallId,
                           bool ok,
                           const QVariantMap& result,
                           const QVariantMap& error);

private:
    struct ParsedStep
    {
        QString stepId;
        QString action;
        QVariantMap args;
        QString saveAs;
    };

    struct ParsedScript
    {
        int version = 1;
        QString title;
        bool allowDangerous = false;
        int timeoutMs = 60000;
        QVector<ParsedStep> steps;
    };

    struct ExecutionState
    {
        QString executionId;
        ParsedScript script;
        int currentStepIndex = -1;
        QString pendingToolCallId;
        QVariantMap lastResult;
        QVariantMap savedResults;
        QVariantList stepReports;
        QDateTime startedAt;
        bool finished = false;
    };

    QVariantMap parseScript(const QString& scriptText, ParsedScript* parsedScript) const;
    QVariantMap validateParsedScript(const ParsedScript& parsedScript) const;
    QVariantMap buildStepPolicy(const ParsedStep& step, int index) const;
    QString riskLevelForStep(bool readOnly, bool requireApproval) const;
    QString mergeRiskLevel(const QString& lhs, const QString& rhs) const;
    QString domainForTool(const QString& toolName) const;
    QString mutationKindForTool(const QString& toolName, bool readOnly) const;
    QString targetKindForTool(const QString& toolName) const;

    QVariant resolveValue(const QVariant& value, const ExecutionState& state) const;
    QVariant resolveReference(const QString& expression, const ExecutionState& state) const;
    QVariantMap resolveArgs(const QVariantMap& args, const ExecutionState& state) const;
    QVariant followPath(const QVariant& value, const QStringList& segments) const;

    void startExecution(ExecutionState state);
    void startNextStep(const QString& executionId);
    void startTimeoutGuard(const QString& executionId, int timeoutMs);
    void stopTimeoutGuard(const QString& executionId);
    void finishExecution(const QString& executionId,
                         bool ok,
                         const QVariantMap& finalResult,
                         const QVariantMap& error = QVariantMap());
    QString nextExecutionId();
    QString nextToolCallId(const QString& executionId, int stepIndex) const;

private:
    AgentCapabilityFacade* m_capabilityFacade = nullptr;
    ToolRegistry* m_toolRegistry = nullptr;
    quint64 m_executionCounter = 0;
    QHash<QString, ExecutionState> m_executionById;
    QHash<QString, QString> m_executionIdByToolCallId;
    QHash<QString, QTimer*> m_timeoutTimerByExecutionId;
};

#endif // AGENT_SCRIPT_EXECUTOR_H
