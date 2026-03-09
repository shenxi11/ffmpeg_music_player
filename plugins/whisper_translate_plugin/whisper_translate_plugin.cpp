#include "whisper_translate_plugin.h"

#include <QDebug>

WhisperTranslatePlugin::WhisperTranslatePlugin()
{
    qDebug() << "WhisperTranslatePlugin constructor";
}

WhisperTranslatePlugin::~WhisperTranslatePlugin()
{
    cleanup();
    qDebug() << "WhisperTranslatePlugin destructor";
}

QString WhisperTranslatePlugin::pluginName() const
{
    return QStringLiteral("Whisper 转写");
}

QString WhisperTranslatePlugin::pluginDescription() const
{
    return QStringLiteral("基于 Whisper.cpp 的音频转文本插件");
}

QString WhisperTranslatePlugin::pluginVersion() const
{
    return QStringLiteral("1.0.0");
}

QIcon WhisperTranslatePlugin::pluginIcon() const
{
    return QIcon();
}

QWidget* WhisperTranslatePlugin::createWidget(QWidget* parent)
{
    qDebug() << "WhisperTranslatePlugin creating TranslateWidget";
    TranslateWidget* widget = new TranslateWidget(parent);
    widget->setPluginHostContext(m_hostContext, m_grantedPermissions);
    return widget;
}

bool WhisperTranslatePlugin::initialize()
{
    if (m_initialized) {
        return true;
    }

    qDebug() << "Initializing WhisperTranslatePlugin...";
    m_initialized = true;
    qDebug() << "WhisperTranslatePlugin initialized successfully";
    return true;
}

void WhisperTranslatePlugin::cleanup()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "Cleaning up WhisperTranslatePlugin...";
    m_initialized = false;
    qDebug() << "WhisperTranslatePlugin cleaned up";
}

QString WhisperTranslatePlugin::pluginId() const
{
    return QStringLiteral("cloudmusic.whisper_translate");
}

int WhisperTranslatePlugin::pluginApiVersion() const
{
    return 1;
}

QStringList WhisperTranslatePlugin::pluginCapabilities() const
{
    return { QStringLiteral("widget"), QStringLiteral("speech.transcribe") };
}

QString WhisperTranslatePlugin::pluginAuthor() const
{
    return QStringLiteral("CloudMusic Team");
}

QStringList WhisperTranslatePlugin::pluginDependencies() const
{
    return {};
}

QStringList WhisperTranslatePlugin::pluginPermissions() const
{
    return { QStringLiteral("ui.widget"), QStringLiteral("speech.transcribe"), QStringLiteral("storage.read") };
}

void WhisperTranslatePlugin::setHostContext(QObject* hostContext)
{
    m_hostContext = hostContext;
}

void WhisperTranslatePlugin::setGrantedPermissions(const QStringList& permissions)
{
    m_grantedPermissions = permissions;
    qDebug() << "WhisperTranslatePlugin granted permissions:" << m_grantedPermissions;
}
