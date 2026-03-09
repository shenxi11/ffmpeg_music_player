#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>
#include <QHash>
#include <QSet>
#include <QDateTime>

#include "plugin_interface.h"
#include "plugin_host_context.h"

enum class PluginState {
    Loaded,
    Initialized,
    Failed,
    Unloaded
};

struct PluginInfo {
    QString id;
    QString name;
    QString description;
    QString version;
    int apiVersion = 1;
    QStringList capabilities;
    QString author;
    QStringList dependencies;
    QStringList requestedPermissions;
    QStringList grantedPermissions;
    QIcon icon;
    PluginInterface* instance = nullptr;
    QPluginLoader* loader = nullptr;
    QString filePath;
    PluginState state = PluginState::Unloaded;
    QString lastError;
    bool isLoaded = false;
};

struct PluginLoadFailure {
    QString pluginId;
    QString filePath;
    QString reason;
    QDateTime timestamp;
};

class PluginManager : public QObject {
    Q_OBJECT
public:
    static PluginManager& instance();

    int loadPlugins(const QString& pluginDirPath);
    bool loadPlugin(const QString& pluginFilePath);

    // Accepts plugin id first, then falls back to display name.
    void unloadPlugin(const QString& pluginKey);
    void unloadAllPlugins();

    QVector<PluginInfo> getPluginInfos() const;

    // Compatible lookup: id first, then display name.
    PluginInterface* getPlugin(const QString& pluginKey) const;
    PluginInterface* getPluginById(const QString& pluginId) const;

    int pluginCount() const { return m_pluginsById.size(); }

    PluginHostContext* hostContext() { return &m_hostContext; }
    void setAllowedPermissions(const QStringList& permissions);
    QStringList allowedPermissions() const;
    QVector<PluginLoadFailure> loadFailures() const;
    void clearLoadFailures();
    QString diagnosticsReport() const;

signals:
    void pluginLoaded(const QString& pluginId);
    void pluginUnloaded(const QString& pluginId);
    void pluginLoadFailed(const QString& pluginFilePath, const QString& reason);

private:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager();
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    static QString normalizeLookupKey(const QString& value);
    static QString normalizePluginId(const QString& value);

    QString resolvePluginId(PluginInterface* plugin,
                            QPluginLoader* loader,
                            const QString& pluginFilePath,
                            const QSet<QString>& occupiedIds = {}) const;
    QString resolvePluginDisplayName(PluginInterface* plugin,
                                     const QString& pluginId,
                                     const QString& pluginFilePath) const;
    QString ensureUniquePluginId(const QString& baseId,
                                 const QSet<QString>& occupiedIds = {}) const;
    QString resolveIdFromKey(const QString& pluginKey) const;

    QHash<QString, PluginInfo> m_pluginsById;
    QHash<QString, QString> m_nameToId;
    QHash<QString, QString> m_filePathToId;
    QStringList m_loadOrder;
    QVector<PluginLoadFailure> m_loadFailures;

    PluginHostContext m_hostContext;
    QSet<QString> m_allowedPermissions;
};

#endif // PLUGIN_MANAGER_H
