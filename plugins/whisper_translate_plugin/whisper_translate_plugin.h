#ifndef WHISPER_TRANSLATE_PLUGIN_H
#define WHISPER_TRANSLATE_PLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "plugin_interface.h"
#include "translate_widget.h"

/**
 * @brief Whisper 翻译插件
 * 实现音频转文字功能，基于 Whisper.cpp
 */
class WhisperTranslatePlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "whisper_translate_plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    WhisperTranslatePlugin();
    ~WhisperTranslatePlugin() override;

    // 实现 PluginInterface 接口
    QString pluginName() const override;
    QString pluginDescription() const override;
    QString pluginVersion() const override;
    QIcon pluginIcon() const override;
    QWidget* createWidget(QWidget* parent = nullptr) override;
    bool initialize() override;
    void cleanup() override;

private:
    bool m_initialized;
};

#endif // WHISPER_TRANSLATE_PLUGIN_H
