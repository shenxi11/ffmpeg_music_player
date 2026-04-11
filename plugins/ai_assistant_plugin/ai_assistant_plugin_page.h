#ifndef AI_ASSISTANT_PLUGIN_PAGE_H
#define AI_ASSISTANT_PLUGIN_PAGE_H

#include <QWidget>
#include <QStringList>

class QQuickWidget;
class AgentChatViewModel;

// AI 助手插件页，承载 QML 聊天界面并管理插件内运行时生命周期。
class AiAssistantPluginPage : public QWidget
{
    Q_OBJECT

public:
    explicit AiAssistantPluginPage(QObject* hostContext,
                                   const QStringList& grantedPermissions,
                                   QWidget* parent = nullptr);
    ~AiAssistantPluginPage() override;

private:
    void buildUi();
    bool hasRequiredPermissions() const;

private:
    QObject* m_hostContext = nullptr;
    QStringList m_grantedPermissions;
    AgentChatViewModel* m_viewModel = nullptr;
    QQuickWidget* m_quickWidget = nullptr;
};

#endif // AI_ASSISTANT_PLUGIN_PAGE_H
