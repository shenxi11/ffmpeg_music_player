#ifndef AGENT_TOOL_REGISTRY_H
#define AGENT_TOOL_REGISTRY_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

/**
 * @brief Agent 工具定义。
 */
struct AgentToolDefinition
{
    QString name;
    QString description;
    QStringList requiredArgs;
    QStringList optionalArgs;
    bool readOnly = true;
    bool requireApproval = false;
};

/**
 * @brief 工具注册表，统一管理可调用工具和参数校验。
 */
class ToolRegistry
{
public:
    ToolRegistry();

    bool contains(const QString& toolName) const;
    AgentToolDefinition definition(const QString& toolName) const;
    QVector<AgentToolDefinition> allTools() const;

    bool validateArgs(const QString& toolName,
                      const QVariantMap& args,
                      QString* errorCode,
                      QString* errorMessage) const;

private:
    void registerDefaultTools();
    void registerTool(const AgentToolDefinition& def);

private:
    QVector<AgentToolDefinition> m_tools;
};

#endif // AGENT_TOOL_REGISTRY_H
