#ifndef AGENT_CONNECTION_STATE_H
#define AGENT_CONNECTION_STATE_H

#include <QString>

/**
 * @brief AI 助手运行状态。
 */
enum class AgentConnectionState
{
    Idle,
    StartingProcess,
    ConnectingSocket,
    Ready,
    Error
};

inline QString agentConnectionStateText(AgentConnectionState state)
{
    switch (state) {
    case AgentConnectionState::Idle:
        return QStringLiteral("未就绪");
    case AgentConnectionState::StartingProcess:
        return QStringLiteral("正在准备 AI 助手...");
    case AgentConnectionState::ConnectingSocket:
        return QStringLiteral("正在初始化本地会话...");
    case AgentConnectionState::Ready:
        return QStringLiteral("AI 助手已就绪");
    case AgentConnectionState::Error:
        return QStringLiteral("AI 助手异常");
    }
    return QStringLiteral("未知状态");
}

#endif // AGENT_CONNECTION_STATE_H
