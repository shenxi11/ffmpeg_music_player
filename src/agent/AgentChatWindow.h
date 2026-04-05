#ifndef AGENT_CHAT_WINDOW_H
#define AGENT_CHAT_WINDOW_H

#include <QWidget>

class AgentChatViewModel;
class QQuickWidget;

/**
 * @brief AI 助手独立聊天窗口（QML 页面承载容器）。
 */
class AgentChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AgentChatWindow(AgentChatViewModel* viewModel, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();

private:
    AgentChatViewModel* m_viewModel = nullptr;
    QQuickWidget* m_quickWidget = nullptr;
};

#endif // AGENT_CHAT_WINDOW_H
