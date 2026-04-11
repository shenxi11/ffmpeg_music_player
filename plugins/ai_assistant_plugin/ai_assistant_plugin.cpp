#include "ai_assistant_plugin.h"

#include <QDebug>
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

AiAssistantPlugin::AiAssistantPlugin()
{
    qDebug() << "AiAssistantPlugin placeholder constructor";
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
    return QStringLiteral("AI 助手开发已暂停，当前版本仅保留插件占位入口。");
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
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("AI 助手已暂停开发"), page);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto* body = new QLabel(
        QStringLiteral("当前版本只保留空插件占位，不加载本地模型、不启动 Agent runtime，也不执行软件控制。"),
        page);
    body->setWordWrap(true);

    layout->addStretch();
    layout->addWidget(title, 0, Qt::AlignHCenter);
    layout->addWidget(body, 0, Qt::AlignHCenter);
    layout->addStretch();
    return page;
}

bool AiAssistantPlugin::initialize()
{
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
    return {QStringLiteral("widget")};
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
    return {QStringLiteral("ui.widget")};
}
