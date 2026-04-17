#include "AgentCapabilityFacade.h"

#include "../tool/AgentToolExecutor.h"
#include "../tool/ToolRegistry.h"

namespace {
QString normalizeCapabilityName(const QString& capabilityName)
{
    const QString normalized = capabilityName.trimmed();
    if (normalized == QStringLiteral("addTracksToPlaylist")) {
        return QStringLiteral("addPlaylistItems");
    }
    return normalized;
}
}

AgentCapabilityFacade::AgentCapabilityFacade(HostStateProvider* hostStateProvider, QObject* parent)
    : QObject(parent)
    , m_toolExecutor(new AgentToolExecutor(hostStateProvider, this))
{
    connect(m_toolExecutor,
            &AgentToolExecutor::toolResultReady,
            this,
            &AgentCapabilityFacade::capabilityResultReady);
    connect(m_toolExecutor,
            &AgentToolExecutor::toolProgress,
            this,
            &AgentCapabilityFacade::capabilityProgress);
}

AgentCapabilityFacade::~AgentCapabilityFacade() = default;

void AgentCapabilityFacade::setHostContext(QObject* hostContext)
{
    if (m_toolExecutor) {
        m_toolExecutor->setHostContext(hostContext);
    }
}

bool AgentCapabilityFacade::executeCapability(const QString& requestId,
                                              const QString& capabilityName,
                                              const QVariantMap& args)
{
    if (!m_toolExecutor) {
        return false;
    }
    return m_toolExecutor->executeToolCall(requestId, normalizeCapabilityName(capabilityName), args);
}

ToolRegistry* AgentCapabilityFacade::toolRegistry() const
{
    return m_toolExecutor ? m_toolExecutor->toolRegistry() : nullptr;
}
