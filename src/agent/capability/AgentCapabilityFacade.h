#ifndef AGENT_CAPABILITY_FACADE_H
#define AGENT_CAPABILITY_FACADE_H

#include <QObject>
#include <QVariantMap>

class AgentToolExecutor;
class HostStateProvider;
class MainShellViewModel;
class ToolRegistry;

/*
模块名称: AgentCapabilityFacade
功能概述: 为 Agent 工具调用、脚本执行和后续 GUI/自动化入口提供统一能力入口。
对外接口: setMainShellViewModel()、executeCapability()、toolRegistry()
依赖关系: HostStateProvider、AgentToolExecutor、ToolRegistry、MainShellViewModel
输入输出: 输入能力名与参数，输出结构化结果信号和进度信号
异常与错误: 底层执行失败时由 capabilityResultReady 的 error 负载统一回传
维护说明: 当前阶段仅作为统一入口外观层，底层仍复用 AgentToolExecutor；后续可继续把业务逻辑逐步下沉到该层
*/
class AgentCapabilityFacade : public QObject
{
    Q_OBJECT

public:
    explicit AgentCapabilityFacade(HostStateProvider* hostStateProvider, QObject* parent = nullptr);
    ~AgentCapabilityFacade() override;

    void setMainShellViewModel(MainShellViewModel* shellViewModel);
    bool executeCapability(const QString& requestId,
                           const QString& capabilityName,
                           const QVariantMap& args);
    ToolRegistry* toolRegistry() const;

signals:
    void capabilityResultReady(const QString& requestId,
                               bool ok,
                               const QVariantMap& result,
                               const QVariantMap& error);
    void capabilityProgress(const QString& message);

private:
    AgentToolExecutor* m_toolExecutor = nullptr;
};

#endif // AGENT_CAPABILITY_FACADE_H
