#include "audio_converter_plugin.h"
#include <QDebug>

AudioConverterPlugin::AudioConverterPlugin()
    : m_initialized(false)
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
    return "音频转换器";
}

QString AudioConverterPlugin::pluginDescription() const
{
    return "支持多种音频格式之间的转换，包括 MP3、WAV、FLAC、AAC、OGG 等格式";
}

QString AudioConverterPlugin::pluginVersion() const
{
    return "1.0.0";
}

QIcon AudioConverterPlugin::pluginIcon() const
{
    // 可以返回插件图标，如果没有则返回空图标
    return QIcon(":/icon/Music.png");
}

QWidget* AudioConverterPlugin::createWidget(QWidget* parent)
{
    qDebug() << "Creating AudioConverter widget";
    
    // 创建音频转换器窗口
    AudioConverter* converter = new AudioConverter(parent);
    
    return converter;
}

bool AudioConverterPlugin::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "Initializing AudioConverterPlugin...";
    
    // 这里可以进行插件初始化工作
    // 例如：检查 FFmpeg 是否可用，加载配置等
    
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
    
    // 这里可以进行插件清理工作
    
    m_initialized = false;
    qDebug() << "AudioConverterPlugin cleaned up";
}
