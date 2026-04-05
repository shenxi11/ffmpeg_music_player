#include "AgentProtocolRouter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {

QString safeString(const QJsonObject& obj, const QString& key)
{
    return obj.value(key).toString().trimmed();
}

QString extractScriptText(const QJsonObject& obj)
{
    const QString directText = obj.value(QStringLiteral("scriptText")).toString().trimmed();
    if (!directText.isEmpty()) {
        return directText;
    }

    const QJsonValue scriptValue = obj.value(QStringLiteral("script"));
    if (scriptValue.isObject()) {
        return QString::fromUtf8(QJsonDocument(scriptValue.toObject()).toJson(QJsonDocument::Compact));
    }
    if (scriptValue.isArray()) {
        return QString::fromUtf8(QJsonDocument(scriptValue.toArray()).toJson(QJsonDocument::Compact));
    }
    if (scriptValue.isString()) {
        return scriptValue.toString().trimmed();
    }
    return QString();
}

}

AgentProtocolRouter::AgentProtocolRouter(QObject* parent)
    : QObject(parent)
{
}

void AgentProtocolRouter::parseMessage(const QString& text)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit protocolError(QStringLiteral("invalid_json"),
                           QStringLiteral("收到无法解析的消息。"));
        return;
    }

    const QJsonObject obj = doc.object();
    const QString type = safeString(obj, QStringLiteral("type"));
    if (type.isEmpty()) {
        emit protocolError(QStringLiteral("invalid_message_type"),
                           QStringLiteral("消息缺少 type 字段。"));
        return;
    }

    if (type == QStringLiteral("session_ready")) {
        const QString sid = safeString(obj, QStringLiteral("sessionId"));
        if (sid.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_session"),
                               QStringLiteral("session_ready 缺少 sessionId。"));
            return;
        }
        emit sessionReady(sid);
        QStringList capabilities;
        const QJsonArray capsArray = obj.value(QStringLiteral("capabilities")).toArray();
        capabilities.reserve(capsArray.size());
        for (const QJsonValue& value : capsArray) {
            const QString cap = value.toString().trimmed();
            if (!cap.isEmpty()) {
                capabilities.push_back(cap);
            }
        }
        emit sessionReadyDetailed(sid,
                                  obj.value(QStringLiteral("title")).toString(),
                                  capabilities,
                                  obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("assistant_start")) {
        emit assistantStartReceived(safeString(obj, QStringLiteral("requestId")));
        return;
    }

    if (type == QStringLiteral("assistant_chunk")) {
        emit assistantChunkReceived(safeString(obj, QStringLiteral("requestId")),
                                    obj.value(QStringLiteral("delta")).toString());
        return;
    }

    if (type == QStringLiteral("assistant_final")) {
        const QString requestId = safeString(obj, QStringLiteral("requestId"));
        const QString content = obj.value(QStringLiteral("content")).toString();
        emit assistantFinalReceived(requestId, content);
        emit assistantMessageReceived(requestId, content);
        return;
    }

    if (type == QStringLiteral("error")) {
        const QString requestId = safeString(obj, QStringLiteral("requestId"));
        QString code = safeString(obj, QStringLiteral("code"));
        QString message = obj.value(QStringLiteral("message")).toString();
        if (code.isEmpty()) {
            code = QStringLiteral("unknown_error");
        }
        if (message.trimmed().isEmpty()) {
            message = QStringLiteral("Agent 返回了错误。");
        }
        emit requestError(requestId, code, message);
        return;
    }

    if (type == QStringLiteral("plan_preview")) {
        emit planPreviewReceived(safeString(obj, QStringLiteral("planId")), obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("approval_request")) {
        emit approvalRequestReceived(safeString(obj, QStringLiteral("planId")),
                                     obj.value(QStringLiteral("message")).toString(),
                                     obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("clarification_request")) {
        QStringList options;
        const QJsonArray optionsArray = obj.value(QStringLiteral("options")).toArray();
        options.reserve(optionsArray.size());
        for (const QJsonValue& value : optionsArray) {
            const QString option = value.toString().trimmed();
            if (!option.isEmpty()) {
                options.push_back(option);
            }
        }
        emit clarificationRequestReceived(safeString(obj, QStringLiteral("requestId")),
                                          obj.value(QStringLiteral("question")).toString(),
                                          options,
                                          obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("progress")) {
        emit progressReceived(obj.value(QStringLiteral("message")).toString(), obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("final_result")) {
        emit finalResultReceived(obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("tool_call")) {
        const QString toolCallId = safeString(obj, QStringLiteral("toolCallId"));
        const QString tool = safeString(obj, QStringLiteral("tool"));
        const QVariantMap args = obj.value(QStringLiteral("args")).toObject().toVariantMap();
        if (toolCallId.isEmpty() || tool.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_tool_call"),
                               QStringLiteral("tool_call 缺少 toolCallId 或 tool。"));
            return;
        }
        emit toolCallReceived(toolCallId, tool, args);
        return;
    }

    if (type == QStringLiteral("validate_script")) {
        const QString requestId = safeString(obj, QStringLiteral("requestId"));
        const QString scriptText = extractScriptText(obj);
        if (requestId.isEmpty() || scriptText.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_validate_script"),
                               QStringLiteral("validate_script 缺少 requestId 或脚本内容。"));
            return;
        }
        emit scriptValidationRequestReceived(requestId, scriptText, obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("dry_run_script")) {
        const QString requestId = safeString(obj, QStringLiteral("requestId"));
        const QString scriptText = extractScriptText(obj);
        if (requestId.isEmpty() || scriptText.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_dry_run_script"),
                               QStringLiteral("dry_run_script 缺少 requestId 或脚本内容。"));
            return;
        }
        emit scriptDryRunRequestReceived(requestId, scriptText, obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("execute_script")) {
        const QString requestId = safeString(obj, QStringLiteral("requestId"));
        const QString scriptText = extractScriptText(obj);
        if (requestId.isEmpty() || scriptText.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_execute_script"),
                               QStringLiteral("execute_script 缺少 requestId 或脚本内容。"));
            return;
        }
        emit scriptExecutionRequestReceived(requestId, scriptText, obj.toVariantMap());
        return;
    }

    if (type == QStringLiteral("cancel_script")) {
        const QString requestId = safeString(obj, QStringLiteral("requestId"));
        const QString executionId = safeString(obj, QStringLiteral("executionId"));
        if (requestId.isEmpty() || executionId.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_cancel_script"),
                               QStringLiteral("cancel_script 缺少 requestId 或 executionId。"));
            return;
        }
        emit scriptCancelRequestReceived(requestId, executionId, obj.toVariantMap());
        return;
    }

    emit protocolError(QStringLiteral("unsupported_message_type"),
                       QStringLiteral("收到不支持的消息类型：%1").arg(type));
}
