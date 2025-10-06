#ifndef AUDIO_CONVERTER_PLUGIN_H
#define AUDIO_CONVERTER_PLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "plugin_interface.h"
#include "audio_converter.h"

/**
 * @brief 音频转换插件
 * 实现音频格式转换功能
 */
class AudioConverterPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "audio_converter_plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    AudioConverterPlugin();
    ~AudioConverterPlugin() override;

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

#endif // AUDIO_CONVERTER_PLUGIN_H
