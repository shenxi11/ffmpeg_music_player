#ifndef AUDIO_CONVERTER_PLUGIN_H
#define AUDIO_CONVERTER_PLUGIN_H

#include <QObject>
#include <QtPlugin>

#include "plugin_interface.h"
#include "audio_converter.h"

class AudioConverterPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "audio_converter_plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    AudioConverterPlugin();
    ~AudioConverterPlugin() override;

    QString pluginName() const override;
    QString pluginDescription() const override;
    QString pluginVersion() const override;
    QIcon pluginIcon() const override;
    QWidget* createWidget(QWidget* parent = nullptr) override;
    bool initialize() override;
    void cleanup() override;

    QString pluginId() const override;
    int pluginApiVersion() const override;
    QStringList pluginCapabilities() const override;
    QString pluginAuthor() const override;
    QStringList pluginDependencies() const override;
    QStringList pluginPermissions() const override;
    void setHostContext(QObject* hostContext) override;
    void setGrantedPermissions(const QStringList& permissions) override;

private:
    bool m_initialized = false;
    QObject* m_hostContext = nullptr;
    QStringList m_grantedPermissions;
};

#endif // AUDIO_CONVERTER_PLUGIN_H
