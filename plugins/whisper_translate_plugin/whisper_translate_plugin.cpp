#include "whisper_translate_plugin.h"
#include <QDebug>

WhisperTranslatePlugin::WhisperTranslatePlugin()
    : m_initialized(false)
{
    qDebug() << "WhisperTranslatePlugin: Constructor called";
}

WhisperTranslatePlugin::~WhisperTranslatePlugin()
{
    qDebug() << "WhisperTranslatePlugin: Destructor called";
    cleanup();
}

QString WhisperTranslatePlugin::pluginName() const
{
    return "Whisper 翻译";
}

QString WhisperTranslatePlugin::pluginDescription() const
{
    return "基于 Whisper.cpp 的音频转文字插件";
}

QString WhisperTranslatePlugin::pluginVersion() const
{
    return "1.0.0";
}

QIcon WhisperTranslatePlugin::pluginIcon() const
{
    // 可以返回一个自定义图标
    return QIcon();
}

QWidget* WhisperTranslatePlugin::createWidget(QWidget* parent)
{
    qDebug() << "WhisperTranslatePlugin: Creating TranslateWidget";
    return new TranslateWidget(parent);
}

bool WhisperTranslatePlugin::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "WhisperTranslatePlugin: Initializing...";
    
    // 这里可以进行插件初始化工作
    // 例如：检查 Whisper 模型是否存在等
    
    m_initialized = true;
    qDebug() << "WhisperTranslatePlugin: Initialized successfully";
    return true;
}

void WhisperTranslatePlugin::cleanup()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "WhisperTranslatePlugin: Cleaning up...";
    
    // 这里可以进行插件清理工作
    
    m_initialized = false;
    qDebug() << "WhisperTranslatePlugin: Cleaned up";
}
