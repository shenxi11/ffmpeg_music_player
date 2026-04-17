#include "ai_assistant_plugin.h"

#include <QDebug>
#include <QIcon>

#include "ai_assistant_plugin_page.h"

AiAssistantPlugin::AiAssistantPlugin()
{
    qDebug() << "AiAssistantPlugin constructed";
}

AiAssistantPlugin::~AiAssistantPlugin()
{
    cleanup();
}

QString AiAssistantPlugin::pluginName() const
{
    return QStringLiteral("AI 助手");
}

QString AiAssistantPlugin::pluginDescription() const
{
    return QStringLiteral("本地运行时 AI 助手插件，通过 clientAutomationHost 访问宿主能力。");
}

QString AiAssistantPlugin::pluginVersion() const
{
    return QStringLiteral("1.0.0");
}

QIcon AiAssistantPlugin::pluginIcon() const
{
    return QIcon(QStringLiteral(":/qml/assets/ai/icons/run.svg"));
}

QWidget* AiAssistantPlugin::createWidget(QWidget* parent)
{
    return new AiAssistantPluginPage(m_hostContext, m_grantedPermissions, parent);
}

bool AiAssistantPlugin::initialize()
{
    if (m_initialized) {
        return true;
    }
    m_initialized = true;
    return true;
}

void AiAssistantPlugin::cleanup()
{
    m_initialized = false;
}

QString AiAssistantPlugin::pluginId() const
{
    return QStringLiteral("cloudmusic.ai_assistant");
}

int AiAssistantPlugin::pluginApiVersion() const
{
    return 1;
}

QStringList AiAssistantPlugin::pluginCapabilities() const
{
    return {QStringLiteral("widget"), QStringLiteral("agent.chat")};
}

QString AiAssistantPlugin::pluginAuthor() const
{
    return QStringLiteral("CloudMusic Team");
}

QStringList AiAssistantPlugin::pluginDependencies() const
{
    return {};
}

QStringList AiAssistantPlugin::pluginPermissions() const
{
    return {QStringLiteral("ui.widget"),
            QStringLiteral("agent.host.read"),
            QStringLiteral("agent.host.control"),
            QStringLiteral("network.read"),
            QStringLiteral("storage.read"),
            QStringLiteral("playback.control")};
}

void AiAssistantPlugin::setHostContext(QObject* hostContext)
{
    m_hostContext = hostContext;
}

void AiAssistantPlugin::setGrantedPermissions(const QStringList& permissions)
{
    m_grantedPermissions = permissions;
}
