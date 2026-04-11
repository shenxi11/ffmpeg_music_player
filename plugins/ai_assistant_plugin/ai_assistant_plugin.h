#ifndef AI_ASSISTANT_PLUGIN_H
#define AI_ASSISTANT_PLUGIN_H

#include <QObject>
#include <QtPlugin>

#include "plugin_interface.h"

class AiAssistantPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "ai_assistant_plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    AiAssistantPlugin();
    ~AiAssistantPlugin() override;

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

private:
    bool m_initialized = false;
};

#endif // AI_ASSISTANT_PLUGIN_H
