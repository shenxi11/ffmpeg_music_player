#include "ai_assistant_plugin_page.h"

#include <QLabel>
#include <QMessageBox>
#include <QQmlError>
#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>

#include "AgentChatViewModel.h"

namespace {
const QStringList kRequiredPermissions = {QStringLiteral("ui.widget"),
                                          QStringLiteral("agent.host.read"),
                                          QStringLiteral("agent.host.control")};
}

AiAssistantPluginPage::AiAssistantPluginPage(QObject* hostContext,
                                             const QStringList& grantedPermissions,
                                             QWidget* parent)
    : QWidget(parent)
    , m_hostContext(hostContext)
    , m_grantedPermissions(grantedPermissions)
    , m_viewModel(new AgentChatViewModel(this))
{
    buildUi();
}

AiAssistantPluginPage::~AiAssistantPluginPage()
{
    if (m_viewModel) {
        m_viewModel->handleWindowClosed();
    }
}

void AiAssistantPluginPage::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    if (!hasRequiredPermissions() || !m_hostContext) {
        auto* label = new QLabel(QStringLiteral("AI 助手插件缺少宿主权限或宿主上下文未注入。"), this);
        label->setWordWrap(true);
        label->setMargin(24);
        rootLayout->addWidget(label);
        return;
    }

    m_viewModel->setHostContext(m_hostContext);
    m_viewModel->initialize();

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setClearColor(QColor("#F4F6FA"));
    m_quickWidget->rootContext()->setContextProperty(QStringLiteral("agentChatVM"), m_viewModel);
    m_quickWidget->setSource(QUrl(QStringLiteral("qrc:/qml/components/agent/AgentChatWindow.qml")));

    if (m_quickWidget->status() == QQuickWidget::Error) {
        QStringList lines;
        const auto errors = m_quickWidget->errors();
        for (const auto& error : errors) {
            lines << error.toString();
        }
        QMessageBox::warning(this,
                             QStringLiteral("AI 助手"),
                             QStringLiteral("聊天页面加载失败：\n%1").arg(lines.join(QStringLiteral("\n"))));
    }

    rootLayout->addWidget(m_quickWidget);
}

bool AiAssistantPluginPage::hasRequiredPermissions() const
{
    for (const QString& permission : kRequiredPermissions) {
        if (!m_grantedPermissions.contains(permission)) {
            return false;
        }
    }
    return true;
}
