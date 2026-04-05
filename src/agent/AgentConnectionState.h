#ifndef AGENT_CONNECTION_STATE_H
#define AGENT_CONNECTION_STATE_H

#include <QString>

/**
 * @brief AI 助手连接状态。
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
        return QStringLiteral("未连接");
    case AgentConnectionState::StartingProcess:
        return QStringLiteral("正在启动 Agent...");
    case AgentConnectionState::ConnectingSocket:
        return QStringLiteral("正在连接 Agent...");
    case AgentConnectionState::Ready:
        return QStringLiteral("已连接");
    case AgentConnectionState::Error:
        return QStringLiteral("连接异常");
    }
    return QStringLiteral("未知状态");
}

#endif // AGENT_CONNECTION_STATE_H
