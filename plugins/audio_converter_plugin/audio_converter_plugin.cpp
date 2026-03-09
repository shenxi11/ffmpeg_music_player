#include "audio_converter_plugin.h"

#include <QDebug>

AudioConverterPlugin::AudioConverterPlugin()
{
    qDebug() << "AudioConverterPlugin constructor";
}

AudioConverterPlugin::~AudioConverterPlugin()
{
    cleanup();
    qDebug() << "AudioConverterPlugin destructor";
}

QString AudioConverterPlugin::pluginName() const
{
    return QStringLiteral("音频转换器");
}

QString AudioConverterPlugin::pluginDescription() const
{
    return QStringLiteral("支持 MP3/WAV/FLAC/AAC/OGG 等音频格式互转");
}

QString AudioConverterPlugin::pluginVersion() const
{
    return QStringLiteral("1.0.0");
}

QIcon AudioConverterPlugin::pluginIcon() const
{
    return QIcon(QStringLiteral(":/icon/Music.png"));
}

QWidget* AudioConverterPlugin::createWidget(QWidget* parent)
{
    qDebug() << "Creating AudioConverter widget";
    AudioConverter* widget = new AudioConverter(parent);
    widget->setPluginHostContext(m_hostContext, m_grantedPermissions);
    return widget;
}

bool AudioConverterPlugin::initialize()
{
    if (m_initialized) {
        return true;
    }

    qDebug() << "Initializing AudioConverterPlugin...";
    m_initialized = true;
    qDebug() << "AudioConverterPlugin initialized successfully";
    return true;
}

void AudioConverterPlugin::cleanup()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "Cleaning up AudioConverterPlugin...";
    m_initialized = false;
    qDebug() << "AudioConverterPlugin cleaned up";
}

QString AudioConverterPlugin::pluginId() const
{
    return QStringLiteral("cloudmusic.audio_converter");
}

int AudioConverterPlugin::pluginApiVersion() const
{
    return 1;
}

QStringList AudioConverterPlugin::pluginCapabilities() const
{
    return { QStringLiteral("widget"), QStringLiteral("audio.convert") };
}

QString AudioConverterPlugin::pluginAuthor() const
{
    return QStringLiteral("CloudMusic Team");
}

QStringList AudioConverterPlugin::pluginDependencies() const
{
    return {};
}

QStringList AudioConverterPlugin::pluginPermissions() const
{
    return { QStringLiteral("ui.widget"), QStringLiteral("audio.convert"), QStringLiteral("storage.read") };
}

void AudioConverterPlugin::setHostContext(QObject* hostContext)
{
    m_hostContext = hostContext;
}

void AudioConverterPlugin::setGrantedPermissions(const QStringList& permissions)
{
    m_grantedPermissions = permissions;
    qDebug() << "AudioConverterPlugin granted permissions:" << m_grantedPermissions;
}
