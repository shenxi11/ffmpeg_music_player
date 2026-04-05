#include "AgentScriptExecutor.h"

#include "capability/AgentCapabilityFacade.h"
#include "tool/ToolRegistry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSet>

namespace {
QVariantMap makeValidationError(const QString& code,
                                const QString& message,
                                const QVariantMap& detail = QVariantMap())
{
    QVariantMap error{
        {QStringLiteral("ok"), false},
        {QStringLiteral("code"), code},
        {QStringLiteral("message"), message}
    };
    if (!detail.isEmpty()) {
        error.insert(QStringLiteral("detail"), detail);
    }
    return error;
}

QVariantMap makeValidationSuccess(const QVariantMap& payload)
{
    QVariantMap result = payload;
    result.insert(QStringLiteral("ok"), true);
    return result;
}

QVariantMap makeStepSummary(int index,
                            const QString& stepId,
                            const QString& action,
                            const QString& saveAs,
                            const QVariantMap& args)
{
    return {
        {QStringLiteral("index"), index},
        {QStringLiteral("stepId"), stepId},
        {QStringLiteral("action"), action},
        {QStringLiteral("saveAs"), saveAs},
        {QStringLiteral("args"), args}
    };
}
}

AgentScriptExecutor::AgentScriptExecutor(AgentCapabilityFacade* capabilityFacade, QObject* parent)
    : QObject(parent)
    , m_capabilityFacade(capabilityFacade)
{
    if (m_capabilityFacade) {
        connect(m_capabilityFacade,
                &AgentCapabilityFacade::capabilityResultReady,
                this,
                &AgentScriptExecutor::onToolResultReady);
        m_toolRegistry = m_capabilityFacade->toolRegistry();
    }
    if (!m_toolRegistry) {
        m_toolRegistry = new ToolRegistry();
    }
}

AgentScriptExecutor::~AgentScriptExecutor()
{
    qDeleteAll(m_timeoutTimerByExecutionId);
    m_timeoutTimerByExecutionId.clear();
    if (!m_capabilityFacade) {
        delete m_toolRegistry;
    }
    m_toolRegistry = nullptr;
}

QVariantMap AgentScriptExecutor::validateScript(const QString& scriptText) const
{
    ParsedScript parsedScript;
    const QVariantMap parseResult = parseScript(scriptText, &parsedScript);
    if (!parseResult.value(QStringLiteral("ok")).toBool()) {
        return parseResult;
    }
    return validateParsedScript(parsedScript);
}

QVariantMap AgentScriptExecutor::dryRunScript(const QString& scriptText) const
{
    const QVariantMap validation = validateScript(scriptText);
    if (!validation.value(QStringLiteral("ok")).toBool()) {
        return validation;
    }

    QVariantMap report = validation;
    report.insert(QStringLiteral("mode"), QStringLiteral("dry_run"));
    report.insert(QStringLiteral("wouldExecute"), true);
    report.insert(QStringLiteral("canAutoExecute"),
                  validation.value(QStringLiteral("autoExecutable")).toBool());
    return report;
}

QString AgentScriptExecutor::executeScript(const QString& scriptText)
{
    if (!m_capabilityFacade) {
        return QString();
    }

    ParsedScript parsedScript;
    const QVariantMap parseResult = parseScript(scriptText, &parsedScript);
    if (!parseResult.value(QStringLiteral("ok")).toBool()) {
        return QString();
    }

    const QVariantMap validationResult = validateParsedScript(parsedScript);
    if (!validationResult.value(QStringLiteral("ok")).toBool()) {
        return QString();
    }

    ExecutionState state;
    state.executionId = nextExecutionId();
    state.script = parsedScript;
    startExecution(state);
    return state.executionId;
}

bool AgentScriptExecutor::cancelExecution(const QString& executionId, const QString& reason)
{
    if (!m_executionById.contains(executionId)) {
        return false;
    }

    const ExecutionState state = m_executionById.value(executionId);
    if (state.finished) {
        return false;
    }

    if (!state.pendingToolCallId.isEmpty()) {
        m_executionIdByToolCallId.remove(state.pendingToolCallId);
    }
    finishExecution(executionId,
                    false,
                    QVariantMap(),
                    {
                        {QStringLiteral("code"), QStringLiteral("script_cancelled")},
                        {QStringLiteral("message"), reason.trimmed().isEmpty()
                                                      ? QStringLiteral("脚本执行已取消")
                                                      : reason.trimmed()},
                        {QStringLiteral("status"), QStringLiteral("cancelled")}
                    });
    return true;
}

void AgentScriptExecutor::onToolResultReady(const QString& toolCallId,
                                            bool ok,
                                            const QVariantMap& result,
                                            const QVariantMap& error)
{
    const QString executionId = m_executionIdByToolCallId.take(toolCallId);
    if (executionId.isEmpty() || !m_executionById.contains(executionId)) {
        return;
    }

    ExecutionState state = m_executionById.value(executionId);
    const int stepIndex = state.currentStepIndex;
    if (stepIndex < 0 || stepIndex >= state.script.steps.size()) {
        finishExecution(executionId,
                        false,
                        QVariantMap(),
                        {
                            {QStringLiteral("code"), QStringLiteral("script_state_invalid")},
                            {QStringLiteral("message"), QStringLiteral("脚本执行状态异常")}
                        });
        return;
    }

    const ParsedStep& step = state.script.steps.at(stepIndex);
    QVariantMap stepPayload{
        {QStringLiteral("stepId"), step.stepId},
        {QStringLiteral("action"), step.action},
        {QStringLiteral("ok"), ok}
    };
    if (ok) {
        stepPayload.insert(QStringLiteral("result"), result);
    } else {
        stepPayload.insert(QStringLiteral("error"), error);
    }
    state.stepReports.push_back(stepPayload);
    state.pendingToolCallId.clear();

    emit scriptStepFinished(executionId, stepIndex, ok, stepPayload);

    if (!ok) {
        m_executionById.insert(executionId, state);
        finishExecution(executionId, false, QVariantMap(), error);
        return;
    }

    state.lastResult = result;
    if (!step.saveAs.isEmpty()) {
        state.savedResults.insert(step.saveAs, result);
    }
    if (!step.stepId.isEmpty()) {
        state.savedResults.insert(step.stepId, result);
    }
    m_executionById.insert(executionId, state);
    startNextStep(executionId);
}

QVariantMap AgentScriptExecutor::parseScript(const QString& scriptText,
                                             ParsedScript* parsedScript) const
{
    if (!parsedScript) {
        return makeValidationError(QStringLiteral("invalid_state"),
                                   QStringLiteral("脚本解析器内部状态异常"));
    }

    const QString trimmed = scriptText.trimmed();
    if (trimmed.isEmpty()) {
        return makeValidationError(QStringLiteral("empty_script"),
                                   QStringLiteral("脚本内容不能为空"));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return makeValidationError(QStringLiteral("invalid_json"),
                                   QStringLiteral("脚本 JSON 解析失败"),
                                   {
                                       {QStringLiteral("offset"), parseError.offset},
                                       {QStringLiteral("detail"), parseError.errorString()}
                                   });
    }
    if (!document.isObject()) {
        return makeValidationError(QStringLiteral("invalid_script"),
                                   QStringLiteral("脚本根节点必须是 JSON 对象"));
    }

    const QJsonObject object = document.object();
    ParsedScript script;
    script.version = object.value(QStringLiteral("version")).toInt(1);
    script.title = object.value(QStringLiteral("title")).toString().trimmed();
    script.allowDangerous = object.value(QStringLiteral("allowDangerous")).toBool(false);
    script.timeoutMs = object.value(QStringLiteral("timeoutMs")).toInt(60000);

    const QJsonArray steps = object.value(QStringLiteral("steps")).toArray();
    script.steps.reserve(steps.size());
    for (int i = 0; i < steps.size(); ++i) {
        if (!steps.at(i).isObject()) {
            return makeValidationError(QStringLiteral("invalid_step"),
                                       QStringLiteral("steps[%1] 必须是对象").arg(i));
        }

        const QJsonObject stepObject = steps.at(i).toObject();
        ParsedStep step;
        step.stepId = stepObject.value(QStringLiteral("id")).toString().trimmed();
        if (step.stepId.isEmpty()) {
            step.stepId = QStringLiteral("step_%1").arg(i + 1);
        }
        step.action = stepObject.value(QStringLiteral("action")).toString().trimmed();
        step.args = stepObject.value(QStringLiteral("args")).toObject().toVariantMap();
        step.saveAs = stepObject.value(QStringLiteral("saveAs")).toString().trimmed();
        script.steps.push_back(step);
    }

    *parsedScript = script;
    return makeValidationSuccess({
        {QStringLiteral("version"), script.version},
        {QStringLiteral("stepCount"), script.steps.size()}
    });
}

QVariantMap AgentScriptExecutor::validateParsedScript(const ParsedScript& parsedScript) const
{
    if (parsedScript.version != 1) {
        return makeValidationError(QStringLiteral("unsupported_version"),
                                   QStringLiteral("当前仅支持 version=1 的客户端脚本"));
    }
    if (parsedScript.timeoutMs <= 0 || parsedScript.timeoutMs > 300000) {
        return makeValidationError(QStringLiteral("invalid_timeout"),
                                   QStringLiteral("timeoutMs 必须在 1 到 300000 毫秒之间"));
    }
    if (parsedScript.steps.isEmpty()) {
        return makeValidationError(QStringLiteral("empty_steps"),
                                   QStringLiteral("脚本至少需要一个步骤"));
    }
    if (!m_capabilityFacade) {
        return makeValidationError(QStringLiteral("tool_executor_unavailable"),
                                   QStringLiteral("脚本执行器尚未绑定统一能力入口"));
    }

    QVariantList stepSummaries;
    QSet<QString> aliases;
    const QRegularExpression identifierPattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    bool requiresApproval = false;
    bool containsWrite = false;
    QString overallRisk = QStringLiteral("low");
    QStringList domains;
    QStringList mutationKinds;
    QStringList targetKinds;
    auto appendUnique = [](QStringList* list, const QString& value) {
        if (!list || value.trimmed().isEmpty() || list->contains(value)) {
            return;
        }
        list->push_back(value);
    };
    for (int i = 0; i < parsedScript.steps.size(); ++i) {
        const ParsedStep& step = parsedScript.steps.at(i);
        if (step.action.isEmpty()) {
            return makeValidationError(QStringLiteral("invalid_step"),
                                       QStringLiteral("steps[%1].action 不能为空").arg(i));
        }
        if (!m_toolRegistry->contains(step.action)) {
            return makeValidationError(QStringLiteral("unsupported_tool"),
                                       QStringLiteral("steps[%1].action 暂不支持：%2").arg(i).arg(step.action));
        }
        if (!step.saveAs.isEmpty()) {
            if (!identifierPattern.match(step.saveAs).hasMatch()) {
                return makeValidationError(QStringLiteral("invalid_alias"),
                                           QStringLiteral("steps[%1].saveAs 仅支持字母、数字和下划线，且不能以数字开头").arg(i));
            }
            if (aliases.contains(step.saveAs)) {
                return makeValidationError(QStringLiteral("duplicate_alias"),
                                           QStringLiteral("steps[%1].saveAs 与前序步骤重复").arg(i));
            }
            aliases.insert(step.saveAs);
        }

        QString errorCode;
        QString errorMessage;
        if (!m_toolRegistry->validateArgs(step.action, step.args, &errorCode, &errorMessage)) {
            return makeValidationError(errorCode,
                                       QStringLiteral("steps[%1] 参数非法：%2").arg(i).arg(errorMessage));
        }

        const QVariantMap policy = buildStepPolicy(step, i);
        requiresApproval = requiresApproval || policy.value(QStringLiteral("requireApproval")).toBool();
        containsWrite = containsWrite || !policy.value(QStringLiteral("readOnly")).toBool();
        overallRisk = mergeRiskLevel(overallRisk, policy.value(QStringLiteral("riskLevel")).toString());
        appendUnique(&domains, policy.value(QStringLiteral("domain")).toString());
        appendUnique(&mutationKinds, policy.value(QStringLiteral("mutationKind")).toString());
        appendUnique(&targetKinds, policy.value(QStringLiteral("targetKind")).toString());

        QVariantMap summary = makeStepSummary(i,
                                             step.stepId,
                                             step.action,
                                             step.saveAs,
                                             step.args);
        summary.insert(QStringLiteral("policy"), policy);
        stepSummaries.push_back(summary);
    }

    return makeValidationSuccess({
        {QStringLiteral("version"), parsedScript.version},
        {QStringLiteral("title"), parsedScript.title},
        {QStringLiteral("allowDangerous"), parsedScript.allowDangerous},
        {QStringLiteral("timeoutMs"), parsedScript.timeoutMs},
        {QStringLiteral("stepCount"), parsedScript.steps.size()},
        {QStringLiteral("steps"), stepSummaries},
        {QStringLiteral("requiresApproval"), requiresApproval},
        {QStringLiteral("containsWrite"), containsWrite},
        {QStringLiteral("riskLevel"), overallRisk},
        {QStringLiteral("autoExecutable"), !requiresApproval},
        {QStringLiteral("domains"), domains},
        {QStringLiteral("mutationKinds"), mutationKinds},
        {QStringLiteral("targetKinds"), targetKinds}
    });
}

QVariantMap AgentScriptExecutor::buildStepPolicy(const ParsedStep& step, int index) const
{
    Q_UNUSED(index);
    const AgentToolDefinition def = m_toolRegistry ? m_toolRegistry->definition(step.action)
                                                   : AgentToolDefinition();
    const QString riskLevel = riskLevelForStep(def.readOnly, def.requireApproval);
    const QString domain = domainForTool(def.name);
    const QString mutationKind = mutationKindForTool(def.name, def.readOnly);
    const QString targetKind = targetKindForTool(def.name);
    return {
        {QStringLiteral("tool"), def.name},
        {QStringLiteral("readOnly"), def.readOnly},
        {QStringLiteral("requireApproval"), def.requireApproval},
        {QStringLiteral("riskLevel"), riskLevel},
        {QStringLiteral("domain"), domain},
        {QStringLiteral("mutationKind"), mutationKind},
        {QStringLiteral("targetKind"), targetKind}
    };
}

QString AgentScriptExecutor::riskLevelForStep(bool readOnly, bool requireApproval) const
{
    if (requireApproval) {
        return QStringLiteral("high");
    }
    if (!readOnly) {
        return QStringLiteral("medium");
    }
    return QStringLiteral("low");
}

QString AgentScriptExecutor::mergeRiskLevel(const QString& lhs, const QString& rhs) const
{
    auto weight = [](const QString& level) {
        if (level == QStringLiteral("high")) {
            return 3;
        }
        if (level == QStringLiteral("medium")) {
            return 2;
        }
        return 1;
    };
    return weight(lhs) >= weight(rhs) ? lhs : rhs;
}

QString AgentScriptExecutor::domainForTool(const QString& toolName) const
{
    if (toolName.contains(QStringLiteral("Playlist"), Qt::CaseInsensitive)) {
        return QStringLiteral("playlist");
    }
    if (toolName.contains(QStringLiteral("Favorite"), Qt::CaseInsensitive)) {
        return QStringLiteral("favorite");
    }
    if (toolName.contains(QStringLiteral("Recent"), Qt::CaseInsensitive) ||
        toolName.contains(QStringLiteral("History"), Qt::CaseInsensitive)) {
        return QStringLiteral("history");
    }
    if (toolName.contains(QStringLiteral("Download"), Qt::CaseInsensitive)) {
        return QStringLiteral("download");
    }
    if (toolName.contains(QStringLiteral("Video"), Qt::CaseInsensitive)) {
        return QStringLiteral("video");
    }
    if (toolName.contains(QStringLiteral("DesktopLyrics"), Qt::CaseInsensitive)) {
        return QStringLiteral("desktop_lyrics");
    }
    if (toolName.contains(QStringLiteral("Plugin"), Qt::CaseInsensitive)) {
        return QStringLiteral("plugin");
    }
    if (toolName.contains(QStringLiteral("Setting"), Qt::CaseInsensitive)) {
        return QStringLiteral("settings");
    }
    if (toolName.contains(QStringLiteral("Recommendation"), Qt::CaseInsensitive)) {
        return QStringLiteral("recommendation");
    }
    if (toolName.contains(QStringLiteral("Artist"), Qt::CaseInsensitive)) {
        return QStringLiteral("artist");
    }
    if (toolName.contains(QStringLiteral("Local"), Qt::CaseInsensitive)) {
        return QStringLiteral("local_library");
    }
    if (toolName.contains(QStringLiteral("Queue"), Qt::CaseInsensitive) ||
        toolName.contains(QStringLiteral("Playback"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("play"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("pause"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("resume"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("stop"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("seek"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("setVolume"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("setPlayMode"), Qt::CaseInsensitive)) {
        return QStringLiteral("playback");
    }
    if (toolName.contains(QStringLiteral("Track"), Qt::CaseInsensitive) ||
        toolName.contains(QStringLiteral("Lyrics"), Qt::CaseInsensitive)) {
        return QStringLiteral("track");
    }
    return QStringLiteral("general");
}

QString AgentScriptExecutor::mutationKindForTool(const QString& toolName, bool readOnly) const
{
    if (readOnly) {
        return QStringLiteral("read");
    }
    if (toolName.startsWith(QStringLiteral("create"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("add"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("show"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("play"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("resume"), Qt::CaseInsensitive)) {
        return QStringLiteral("create_or_start");
    }
    if (toolName.startsWith(QStringLiteral("update"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("set"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("rename"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("reorder"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("seek"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("pause"), Qt::CaseInsensitive)) {
        return QStringLiteral("update");
    }
    if (toolName.startsWith(QStringLiteral("delete"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("remove"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("clear"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("cancel"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("hide"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("stop"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("unload"), Qt::CaseInsensitive)) {
        return QStringLiteral("delete_or_stop");
    }
    return QStringLiteral("write");
}

QString AgentScriptExecutor::targetKindForTool(const QString& toolName) const
{
    if (toolName.contains(QStringLiteral("Playlist"), Qt::CaseInsensitive)) {
        return QStringLiteral("playlist");
    }
    if (toolName.contains(QStringLiteral("Queue"), Qt::CaseInsensitive)) {
        return QStringLiteral("playback_queue");
    }
    if (toolName.contains(QStringLiteral("Download"), Qt::CaseInsensitive)) {
        return QStringLiteral("download_task");
    }
    if (toolName.contains(QStringLiteral("Video"), Qt::CaseInsensitive)) {
        return QStringLiteral("video_session");
    }
    if (toolName.contains(QStringLiteral("DesktopLyrics"), Qt::CaseInsensitive)) {
        return QStringLiteral("desktop_lyrics");
    }
    if (toolName.contains(QStringLiteral("Plugin"), Qt::CaseInsensitive)) {
        return QStringLiteral("plugin_runtime");
    }
    if (toolName.contains(QStringLiteral("Setting"), Qt::CaseInsensitive)) {
        return QStringLiteral("client_setting");
    }
    if (toolName.contains(QStringLiteral("Artist"), Qt::CaseInsensitive)) {
        return QStringLiteral("artist");
    }
    if (toolName.contains(QStringLiteral("Lyrics"), Qt::CaseInsensitive)) {
        return QStringLiteral("lyrics");
    }
    if (toolName.contains(QStringLiteral("Track"), Qt::CaseInsensitive) ||
        toolName.contains(QStringLiteral("Favorite"), Qt::CaseInsensitive) ||
        toolName.contains(QStringLiteral("Recent"), Qt::CaseInsensitive) ||
        toolName.contains(QStringLiteral("History"), Qt::CaseInsensitive) ||
        toolName.startsWith(QStringLiteral("play"), Qt::CaseInsensitive)) {
        return QStringLiteral("track");
    }
    return QStringLiteral("general");
}

QVariant AgentScriptExecutor::resolveValue(const QVariant& value, const ExecutionState& state) const
{
    if (value.type() == QVariant::String) {
        const QString text = value.toString().trimmed();
        if (text.startsWith(QLatin1Char('$'))) {
            return resolveReference(text, state);
        }
        return value;
    }

    if (value.type() == QVariant::Map) {
        return resolveArgs(value.toMap(), state);
    }

    if (value.type() == QVariant::List) {
        QVariantList resolvedList;
        const QVariantList sourceList = value.toList();
        resolvedList.reserve(sourceList.size());
        for (const QVariant& item : sourceList) {
            resolvedList.push_back(resolveValue(item, state));
        }
        return resolvedList;
    }

    return value;
}

QVariant AgentScriptExecutor::resolveReference(const QString& expression,
                                               const ExecutionState& state) const
{
    const QStringList segments = expression.mid(1).split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (segments.isEmpty()) {
        return QVariant();
    }

    if (segments.first() == QStringLiteral("last")) {
        return followPath(state.lastResult, segments.mid(1));
    }

    if (segments.first() == QStringLiteral("steps")) {
        if (segments.size() < 2) {
            return QVariant();
        }
        const QVariant value = state.savedResults.value(segments.at(1));
        return followPath(value, segments.mid(2));
    }

    return QVariant();
}

QVariantMap AgentScriptExecutor::resolveArgs(const QVariantMap& args, const ExecutionState& state) const
{
    QVariantMap resolvedArgs;
    for (auto it = args.constBegin(); it != args.constEnd(); ++it) {
        resolvedArgs.insert(it.key(), resolveValue(it.value(), state));
    }
    return resolvedArgs;
}

QVariant AgentScriptExecutor::followPath(const QVariant& value,
                                         const QStringList& segments) const
{
    QVariant current = value;
    for (const QString& segment : segments) {
        if (!current.isValid()) {
            return QVariant();
        }

        if (current.type() == QVariant::Map) {
            current = current.toMap().value(segment);
            continue;
        }

        if (current.type() == QVariant::List) {
            bool ok = false;
            const int index = segment.toInt(&ok);
            const QVariantList list = current.toList();
            if (!ok || index < 0 || index >= list.size()) {
                return QVariant();
            }
            current = list.at(index);
            continue;
        }

        return QVariant();
    }
    return current;
}

void AgentScriptExecutor::startExecution(ExecutionState state)
{
    state.startedAt = QDateTime::currentDateTime();
    const QVariantMap summary{
        {QStringLiteral("executionId"), state.executionId},
        {QStringLiteral("title"), state.script.title},
        {QStringLiteral("stepCount"), state.script.steps.size()},
        {QStringLiteral("version"), state.script.version},
        {QStringLiteral("timeoutMs"), state.script.timeoutMs},
        {QStringLiteral("startedAt"), state.startedAt.toString(Qt::ISODate)}
    };
    m_executionById.insert(state.executionId, state);
    startTimeoutGuard(state.executionId, state.script.timeoutMs);
    emit scriptExecutionStarted(state.executionId, summary);
    startNextStep(state.executionId);
}

void AgentScriptExecutor::startTimeoutGuard(const QString& executionId, int timeoutMs)
{
    stopTimeoutGuard(executionId);

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [this, executionId]() {
        if (!m_executionById.contains(executionId)) {
            return;
        }
        finishExecution(executionId,
                        false,
                        QVariantMap(),
                        {
                            {QStringLiteral("code"), QStringLiteral("script_timeout")},
                            {QStringLiteral("message"), QStringLiteral("脚本执行超时")},
                            {QStringLiteral("status"), QStringLiteral("timeout")}
                        });
    });
    m_timeoutTimerByExecutionId.insert(executionId, timer);
    timer->start(timeoutMs);
}

void AgentScriptExecutor::stopTimeoutGuard(const QString& executionId)
{
    QTimer* timer = m_timeoutTimerByExecutionId.take(executionId);
    if (!timer) {
        return;
    }
    timer->stop();
    timer->deleteLater();
}

void AgentScriptExecutor::startNextStep(const QString& executionId)
{
    if (!m_executionById.contains(executionId)) {
        return;
    }

    ExecutionState state = m_executionById.value(executionId);
    if (state.finished) {
        return;
    }

    const int nextIndex = state.currentStepIndex + 1;
    if (nextIndex >= state.script.steps.size()) {
        finishExecution(executionId,
                        true,
                        {
                            {QStringLiteral("lastResult"), state.lastResult},
                            {QStringLiteral("savedResults"), state.savedResults},
                            {QStringLiteral("steps"), state.stepReports}
                        });
        return;
    }

    const ParsedStep& step = state.script.steps.at(nextIndex);
    const QVariantMap resolvedArgs = resolveArgs(step.args, state);
    const QString toolCallId = nextToolCallId(executionId, nextIndex);

    state.currentStepIndex = nextIndex;
    state.pendingToolCallId = toolCallId;
    m_executionById.insert(executionId, state);
    m_executionIdByToolCallId.insert(toolCallId, executionId);

    QVariantMap stepSummary = makeStepSummary(nextIndex,
                                             step.stepId,
                                             step.action,
                                             step.saveAs,
                                             step.args);
    stepSummary.insert(QStringLiteral("resolvedArgs"), resolvedArgs);
    emit scriptStepStarted(executionId, nextIndex, stepSummary);

    if (!m_capabilityFacade->executeCapability(toolCallId, step.action, resolvedArgs)) {
        m_executionIdByToolCallId.remove(toolCallId);
        finishExecution(executionId,
                        false,
                        QVariantMap(),
                        {
                            {QStringLiteral("code"), QStringLiteral("tool_dispatch_failed")},
                            {QStringLiteral("message"), QStringLiteral("脚本步骤无法分发到客户端工具执行器")},
                            {QStringLiteral("stepId"), step.stepId},
                            {QStringLiteral("action"), step.action}
                        });
    }
}

void AgentScriptExecutor::finishExecution(const QString& executionId,
                                          bool ok,
                                          const QVariantMap& finalResult,
                                          const QVariantMap& error)
{
    if (!m_executionById.contains(executionId)) {
        return;
    }

    ExecutionState state = m_executionById.take(executionId);
    state.finished = true;
    stopTimeoutGuard(executionId);
    if (!state.pendingToolCallId.isEmpty()) {
        m_executionIdByToolCallId.remove(state.pendingToolCallId);
    }

    const QDateTime finishedAt = QDateTime::currentDateTime();
    const qint64 durationMs = state.startedAt.isValid() ? state.startedAt.msecsTo(finishedAt) : 0;
    QString status = ok ? QStringLiteral("succeeded") : QStringLiteral("failed");
    if (!ok && error.value(QStringLiteral("status")).toString() == QStringLiteral("cancelled")) {
        status = QStringLiteral("cancelled");
    } else if (!ok && error.value(QStringLiteral("status")).toString() == QStringLiteral("timeout")) {
        status = QStringLiteral("timeout");
    }

    QVariantMap report{
        {QStringLiteral("executionId"), executionId},
        {QStringLiteral("ok"), ok},
        {QStringLiteral("title"), state.script.title},
        {QStringLiteral("status"), status},
        {QStringLiteral("stepCount"), state.script.steps.size()},
        {QStringLiteral("completedSteps"), state.stepReports.size()},
        {QStringLiteral("steps"), state.stepReports},
        {QStringLiteral("startedAt"), state.startedAt.toString(Qt::ISODate)},
        {QStringLiteral("finishedAt"), finishedAt.toString(Qt::ISODate)},
        {QStringLiteral("durationMs"), durationMs}
    };
    if (ok) {
        report.insert(QStringLiteral("result"), finalResult);
    } else {
        report.insert(QStringLiteral("error"), error);
    }
    emit scriptExecutionFinished(executionId, ok, report);
}

QString AgentScriptExecutor::nextExecutionId()
{
    ++m_executionCounter;
    return QStringLiteral("script_exec_%1").arg(m_executionCounter);
}

QString AgentScriptExecutor::nextToolCallId(const QString& executionId, int stepIndex) const
{
    return QStringLiteral("%1_step_%2").arg(executionId).arg(stepIndex + 1);
}

