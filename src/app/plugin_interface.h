#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QIcon>

class PluginInterface {
public:
    virtual ~PluginInterface() {}

    // Legacy required metadata (kept for compatibility).
    virtual QString pluginName() const = 0;
    virtual QString pluginDescription() const = 0;
    virtual QString pluginVersion() const = 0;
    virtual QIcon pluginIcon() const = 0;

    // Widget contribution entry.
    virtual QWidget* createWidget(QWidget* parent = nullptr) = 0;

    // Lifecycle.
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;

    // Extended metadata.
    virtual QString pluginId() const { return QString(); }
    virtual int pluginApiVersion() const { return 1; }
    virtual QStringList pluginCapabilities() const { return { QStringLiteral("widget") }; }
    virtual QString pluginAuthor() const { return QString(); }

    // Dependencies and permissions.
    virtual QStringList pluginDependencies() const { return {}; }
    virtual QStringList pluginPermissions() const { return {}; }

    // Host injection hooks.
    virtual void setHostContext(QObject* hostContext) { Q_UNUSED(hostContext); }
    virtual void setGrantedPermissions(const QStringList& permissions) { Q_UNUSED(permissions); }
};

#define PluginInterface_iid "com.ffmpeg_music_player.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGIN_INTERFACE_H
