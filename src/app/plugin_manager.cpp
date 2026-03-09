#include "plugin_manager.h"

#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

namespace {
constexpr int kHostPluginApiVersion = 1;

struct PluginCandidate {
    QString id;
    QString filePath;
    QStringList dependencies;
};

QStringList parseStringList(const QJsonObject& obj, const QString& key)
{
    QStringList values;
    const QJsonArray array = obj.value(key).toArray();
    for (const QJsonValue& value : array) {
        const QString text = value.toString().trimmed();
        if (!text.isEmpty()) {
            values.append(text);
        }
    }
    return values;
}

QString normalizePermission(const QString& value)
{
    return value.trimmed().toLower();
}

void appendUnique(QStringList& out, const QString& value)
{
    if (!value.isEmpty() && !out.contains(value)) {
        out.append(value);
    }
}
} // namespace

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
    , m_hostContext(this)
{
    m_allowedPermissions = {
        QStringLiteral("ui.widget"),
        QStringLiteral("audio.convert"),
        QStringLiteral("speech.transcribe"),
        QStringLiteral("network.read"),
        QStringLiteral("storage.read"),
        QStringLiteral("playback.control")
    };

    m_hostContext.registerService(QStringLiteral("pluginManager"), this);
    m_hostContext.setEnvironmentValue(QStringLiteral("pluginHostApiVersion"), kHostPluginApiVersion);

    qDebug() << "PluginManager initialized, hostApiVersion=" << kHostPluginApiVersion;
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

PluginManager& PluginManager::instance()
{
    static PluginManager inst;
    return inst;
}

void PluginManager::setAllowedPermissions(const QStringList& permissions)
{
    m_allowedPermissions.clear();
    for (const QString& permission : permissions) {
        const QString normalized = normalizePermission(permission);
        if (!normalized.isEmpty()) {
            m_allowedPermissions.insert(normalized);
        }
    }
}

QStringList PluginManager::allowedPermissions() const
{
    QStringList values = m_allowedPermissions.values();
    std::sort(values.begin(), values.end());
    return values;
}

QVector<PluginLoadFailure> PluginManager::loadFailures() const
{
    return m_loadFailures;
}

void PluginManager::clearLoadFailures()
{
    m_loadFailures.clear();
}

QString PluginManager::diagnosticsReport() const
{
    QString report;
    QTextStream stream(&report);

    stream << QStringLiteral("插件诊断报告\n");
    stream << QStringLiteral("====================================\n");
    stream << QStringLiteral("主机 API 版本: ") << kHostPluginApiVersion << "\n";
    stream << QStringLiteral("已加载插件数: ") << m_pluginsById.size() << "\n\n";

    stream << QStringLiteral("[已加载插件]\n");
    for (const QString& id : m_loadOrder) {
        const auto it = m_pluginsById.constFind(id);
        if (it == m_pluginsById.constEnd()) {
            continue;
        }
        const PluginInfo& info = it.value();
        stream << QStringLiteral("- ID: ") << info.id
               << QStringLiteral(" | 名称: ") << info.name
               << QStringLiteral(" | 版本: ") << info.version << "\n";
        stream << QStringLiteral("  能力: ") << info.capabilities.join(QStringLiteral(", ")) << "\n";
        stream << QStringLiteral("  依赖: ")
               << (info.dependencies.isEmpty() ? QStringLiteral("无") : info.dependencies.join(QStringLiteral(", ")))
               << "\n";
        stream << QStringLiteral("  权限: ")
               << (info.grantedPermissions.isEmpty() ? QStringLiteral("无") : info.grantedPermissions.join(QStringLiteral(", ")))
               << "\n";
    }

    stream << QStringLiteral("\n[权限白名单]\n");
    stream << allowedPermissions().join(QStringLiteral(", ")) << "\n";

    stream << QStringLiteral("\n[Host 服务]\n");
    const QStringList hostServices = m_hostContext.serviceKeys();
    stream << (hostServices.isEmpty() ? QStringLiteral("无") : hostServices.join(QStringLiteral(", "))) << "\n";

    stream << QStringLiteral("\n[加载失败记录]\n");
    if (m_loadFailures.isEmpty()) {
        stream << QStringLiteral("无\n");
    } else {
        for (const PluginLoadFailure& failure : m_loadFailures) {
            stream << QStringLiteral("- 时间: ")
                   << failure.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                   << QStringLiteral(" | 文件: ") << failure.filePath << "\n";
            if (!failure.pluginId.isEmpty()) {
                stream << QStringLiteral("  插件ID: ") << failure.pluginId << "\n";
            }
            stream << QStringLiteral("  原因: ") << failure.reason << "\n";
        }
    }

    return report;
}

QString PluginManager::normalizeLookupKey(const QString& value)
{
    return value.trimmed().toLower();
}

QString PluginManager::normalizePluginId(const QString& value)
{
    QString candidate = value.trimmed().toLower();
    candidate.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral("_"));
    candidate.replace(QRegularExpression(QStringLiteral("[^a-z0-9._-]")), QStringLiteral("_"));
    candidate.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
    candidate.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    return candidate;
}

QString PluginManager::ensureUniquePluginId(const QString& baseId,
                                            const QSet<QString>& occupiedIds) const
{
    QString candidate = normalizePluginId(baseId);
    if (candidate.isEmpty()) {
        candidate = QStringLiteral("plugin");
    }

    auto isTaken = [this, &occupiedIds](const QString& id) {
        return m_pluginsById.contains(id) || occupiedIds.contains(id);
    };

    if (!isTaken(candidate)) {
        return candidate;
    }

    int suffix = 2;
    while (true) {
        const QString next = QStringLiteral("%1_%2").arg(candidate).arg(suffix);
        if (!isTaken(next)) {
            return next;
        }
        ++suffix;
    }
}

QString PluginManager::resolvePluginId(PluginInterface* plugin,
                                       QPluginLoader* loader,
                                       const QString& pluginFilePath,
                                       const QSet<QString>& occupiedIds) const
{
    QString candidate = plugin ? plugin->pluginId().trimmed() : QString();

    if (candidate.isEmpty() && loader) {
        const QJsonObject root = loader->metaData();
        const QJsonObject meta = root.value(QStringLiteral("MetaData")).toObject();
        candidate = meta.value(QStringLiteral("id")).toString().trimmed();
        if (candidate.isEmpty()) {
            candidate = meta.value(QStringLiteral("name")).toString().trimmed();
        }
    }

    if (candidate.isEmpty() && plugin) {
        candidate = plugin->pluginName().trimmed();
    }

    if (candidate.isEmpty()) {
        candidate = QFileInfo(pluginFilePath).completeBaseName();
    }

    return ensureUniquePluginId(candidate, occupiedIds);
}

QString PluginManager::resolvePluginDisplayName(PluginInterface* plugin,
                                                const QString& pluginId,
                                                const QString& pluginFilePath) const
{
    if (plugin) {
        const QString name = plugin->pluginName().trimmed();
        if (!name.isEmpty()) {
            return name;
        }
    }

    if (!pluginId.isEmpty()) {
        return pluginId;
    }

    return QFileInfo(pluginFilePath).completeBaseName();
}

QString PluginManager::resolveIdFromKey(const QString& pluginKey) const
{
    const QString trimmedKey = pluginKey.trimmed();
    if (trimmedKey.isEmpty()) {
        return QString();
    }

    if (m_pluginsById.contains(trimmedKey)) {
        return trimmedKey;
    }

    const QString normalizedNameKey = normalizeLookupKey(trimmedKey);
    return m_nameToId.value(normalizedNameKey);
}

int PluginManager::loadPlugins(const QString& pluginDirPath)
{
    QDir pluginsDir(pluginDirPath);

    if (!pluginsDir.exists()) {
        qWarning() << "Plugin directory does not exist:" << pluginDirPath;
        pluginsDir.mkpath(pluginDirPath);
        return 0;
    }

    qDebug() << "Loading plugins from:" << pluginsDir.absolutePath();

    QStringList filters;
#ifdef Q_OS_WIN
    filters << "*_plugin.dll";
#elif defined(Q_OS_LINUX)
    filters << "*_plugin.so";
#elif defined(Q_OS_MAC)
    filters << "*_plugin.dylib";
#endif

    pluginsDir.setNameFilters(filters);
    const QStringList pluginFiles = pluginsDir.entryList(QDir::Files);

    qDebug() << "Found" << pluginFiles.size() << "plugin files";

    QSet<QString> occupiedIds = QSet<QString>::fromList(m_pluginsById.keys());
    QHash<QString, PluginCandidate> candidates;

    for (const QString& fileName : pluginFiles) {
        const QString filePath = QDir::cleanPath(pluginsDir.absoluteFilePath(fileName));
        if (m_filePathToId.contains(filePath)) {
            continue;
        }

        QPluginLoader loader(filePath);
        const QString id = resolvePluginId(nullptr, &loader, filePath, occupiedIds);
        occupiedIds.insert(id);

        const QJsonObject meta = loader.metaData().value(QStringLiteral("MetaData")).toObject();
        QStringList dependencies;
        for (const QString& dep : parseStringList(meta, QStringLiteral("dependencies"))) {
            const QString normalizedDep = normalizePluginId(dep);
            appendUnique(dependencies, normalizedDep);
        }

        PluginCandidate candidate;
        candidate.id = id;
        candidate.filePath = filePath;
        candidate.dependencies = dependencies;
        candidates.insert(id, candidate);
    }

    QHash<QString, int> indegree;
    QHash<QString, QStringList> edges;

    for (auto it = candidates.constBegin(); it != candidates.constEnd(); ++it) {
        indegree.insert(it.key(), 0);
    }

    for (auto it = candidates.constBegin(); it != candidates.constEnd(); ++it) {
        const QString id = it.key();
        for (const QString& dep : it->dependencies) {
            if (dep == id) {
                continue;
            }
            if (candidates.contains(dep)) {
                indegree[id] += 1;
                edges[dep].append(id);
            }
        }
    }

    QStringList zeroIndegree;
    for (auto it = indegree.constBegin(); it != indegree.constEnd(); ++it) {
        if (it.value() == 0) {
            zeroIndegree.append(it.key());
        }
    }
    std::sort(zeroIndegree.begin(), zeroIndegree.end());

    QStringList sortedIds;
    while (!zeroIndegree.isEmpty()) {
        const QString id = zeroIndegree.takeFirst();
        sortedIds.append(id);

        const QStringList nextList = edges.value(id);
        for (const QString& next : nextList) {
            const int current = indegree.value(next) - 1;
            indegree[next] = current;
            if (current == 0) {
                zeroIndegree.append(next);
            }
        }
        std::sort(zeroIndegree.begin(), zeroIndegree.end());
    }

    if (sortedIds.size() != candidates.size()) {
        QStringList unresolved;
        for (auto it = candidates.constBegin(); it != candidates.constEnd(); ++it) {
            if (!sortedIds.contains(it.key())) {
                unresolved.append(it.key());
            }
        }
        std::sort(unresolved.begin(), unresolved.end());
        qWarning() << "Plugin dependency cycle detected:" << unresolved;
        sortedIds.append(unresolved);
    }

    int loadedCount = 0;
    for (const QString& id : sortedIds) {
        const QString filePath = candidates.value(id).filePath;
        if (loadPlugin(filePath)) {
            ++loadedCount;
        }
    }

    qDebug() << "Successfully loaded" << loadedCount << "plugins";
    return loadedCount;
}

bool PluginManager::loadPlugin(const QString& pluginFilePath)
{
    const QString canonicalPath = QDir::cleanPath(QDir::fromNativeSeparators(pluginFilePath));
    if (m_filePathToId.contains(canonicalPath)) {
        qDebug() << "Plugin already loaded, skipping:" << canonicalPath;
        return true;
    }

    const QFileInfo fileInfo(canonicalPath);
    const QString fileName = fileInfo.fileName();

    qDebug() << "Attempting to load plugin:" << fileName;

    QPluginLoader* loader = new QPluginLoader(canonicalPath, this);
    const QJsonObject meta = loader->metaData().value(QStringLiteral("MetaData")).toObject();

    QString resolvedPluginId = resolvePluginId(nullptr, loader, canonicalPath);

    auto failAndCleanup = [this, loader, canonicalPath, fileName, &resolvedPluginId](const QString& reason) {
        qWarning() << "Failed to load plugin:" << fileName << "reason:" << reason;

        const bool duplicated = std::any_of(m_loadFailures.cbegin(),
                                            m_loadFailures.cend(),
                                            [&canonicalPath, &reason](const PluginLoadFailure& failure) {
                                                return failure.filePath == canonicalPath && failure.reason == reason;
                                            });
        if (!duplicated) {
            PluginLoadFailure failure;
            failure.pluginId = resolvedPluginId;
            failure.filePath = canonicalPath;
            failure.reason = reason;
            failure.timestamp = QDateTime::currentDateTime();
            m_loadFailures.push_back(failure);
        }

        emit pluginLoadFailed(canonicalPath, reason);
        if (loader) {
            loader->unload();
            delete loader;
        }
        return false;
    };

    QObject* pluginObj = loader->instance();
    if (!pluginObj) {
        const QString reason = loader->errorString();
        return failAndCleanup(reason.isEmpty() ? QStringLiteral("plugin instance is null") : reason);
    }

    PluginInterface* plugin = qobject_cast<PluginInterface*>(pluginObj);
    if (!plugin) {
        return failAndCleanup(QStringLiteral("Plugin does not implement PluginInterface"));
    }

    const int declaredApi = meta.value(QStringLiteral("apiVersion")).toInt(plugin->pluginApiVersion());
    const int pluginApi = qMax(plugin->pluginApiVersion(), declaredApi);
    if (pluginApi > kHostPluginApiVersion) {
        return failAndCleanup(QStringLiteral("Plugin apiVersion=%1 is newer than host apiVersion=%2")
                                  .arg(pluginApi)
                                  .arg(kHostPluginApiVersion));
    }

    const QString pluginId = resolvePluginId(plugin, loader, canonicalPath);
    resolvedPluginId = pluginId;
    if (m_pluginsById.contains(pluginId)) {
        return failAndCleanup(QStringLiteral("Plugin id duplicated: %1").arg(pluginId));
    }

    QStringList dependencies;
    for (const QString& dep : parseStringList(meta, QStringLiteral("dependencies"))) {
        appendUnique(dependencies, normalizePluginId(dep));
    }
    for (const QString& dep : plugin->pluginDependencies()) {
        appendUnique(dependencies, normalizePluginId(dep));
    }

    for (const QString& depId : dependencies) {
        if (depId == pluginId) {
            return failAndCleanup(QStringLiteral("Plugin cannot depend on itself"));
        }
        if (!depId.isEmpty() && !m_pluginsById.contains(depId)) {
            return failAndCleanup(QStringLiteral("Missing dependency: %1").arg(depId));
        }
    }

    QStringList requestedPermissions;
    for (const QString& permission : parseStringList(meta, QStringLiteral("permissions"))) {
        appendUnique(requestedPermissions, normalizePermission(permission));
    }
    for (const QString& permission : plugin->pluginPermissions()) {
        appendUnique(requestedPermissions, normalizePermission(permission));
    }

    QStringList grantedPermissions;
    QStringList deniedPermissions;
    for (const QString& permission : requestedPermissions) {
        if (m_allowedPermissions.contains(permission)) {
            appendUnique(grantedPermissions, permission);
        } else {
            appendUnique(deniedPermissions, permission);
        }
    }

    if (!deniedPermissions.isEmpty()) {
        return failAndCleanup(QStringLiteral("Permission denied: %1").arg(deniedPermissions.join(',')));
    }

    plugin->setHostContext(&m_hostContext);
    plugin->setGrantedPermissions(grantedPermissions);

    PluginInfo info;
    info.id = pluginId;
    info.name = resolvePluginDisplayName(plugin, info.id, canonicalPath);
    info.description = plugin->pluginDescription();
    info.version = plugin->pluginVersion();
    info.apiVersion = pluginApi;
    info.capabilities = plugin->pluginCapabilities();
    info.author = plugin->pluginAuthor();
    info.dependencies = dependencies;
    info.requestedPermissions = requestedPermissions;
    info.grantedPermissions = grantedPermissions;
    info.icon = plugin->pluginIcon();
    info.instance = plugin;
    info.loader = loader;
    info.filePath = canonicalPath;
    info.state = PluginState::Loaded;

    if (!plugin->initialize()) {
        info.state = PluginState::Failed;
        info.lastError = QStringLiteral("plugin initialize() returned false");
        return failAndCleanup(info.lastError);
    }

    info.state = PluginState::Initialized;
    info.isLoaded = true;

    m_pluginsById.insert(info.id, info);
    m_filePathToId.insert(canonicalPath, info.id);
    m_loadOrder.append(info.id);

    const QString nameKey = normalizeLookupKey(info.name);
    if (!nameKey.isEmpty() && !m_nameToId.contains(nameKey)) {
        m_nameToId.insert(nameKey, info.id);
    }

    qDebug() << "Plugin loaded successfully, id=" << info.id
             << "name=" << info.name
             << "version=" << info.version
             << "apiVersion=" << info.apiVersion
             << "dependencies=" << info.dependencies
             << "grantedPermissions=" << info.grantedPermissions;

    emit pluginLoaded(info.id);
    return true;
}

void PluginManager::unloadPlugin(const QString& pluginKey)
{
    const QString pluginId = resolveIdFromKey(pluginKey);
    if (pluginId.isEmpty() || !m_pluginsById.contains(pluginId)) {
        qWarning() << "Plugin not found:" << pluginKey;
        return;
    }

    PluginInfo info = m_pluginsById.take(pluginId);

    const QString nameKey = normalizeLookupKey(info.name);
    if (!nameKey.isEmpty() && m_nameToId.value(nameKey) == pluginId) {
        m_nameToId.remove(nameKey);
    }

    const QString canonicalPath = QDir::cleanPath(QDir::fromNativeSeparators(info.filePath));
    if (m_filePathToId.value(canonicalPath) == pluginId) {
        m_filePathToId.remove(canonicalPath);
    }

    m_loadOrder.removeAll(pluginId);

    if (info.instance) {
        info.instance->cleanup();
    }

    if (info.loader) {
        info.loader->unload();
        delete info.loader;
    }

    qDebug() << "Plugin unloaded:" << pluginId << "(" << info.name << ")";
    emit pluginUnloaded(pluginId);
}

void PluginManager::unloadAllPlugins()
{
    qDebug() << "Unloading all plugins...";

    const QStringList loadedIds = m_loadOrder;
    for (const QString& id : loadedIds) {
        unloadPlugin(id);
    }

    m_pluginsById.clear();
    m_nameToId.clear();
    m_filePathToId.clear();
    m_loadOrder.clear();

    qDebug() << "All plugins unloaded";
}

QVector<PluginInfo> PluginManager::getPluginInfos() const
{
    QVector<PluginInfo> infos;
    infos.reserve(m_loadOrder.size());

    for (const QString& id : m_loadOrder) {
        const auto it = m_pluginsById.constFind(id);
        if (it != m_pluginsById.constEnd()) {
            infos.push_back(it.value());
        }
    }

    return infos;
}

PluginInterface* PluginManager::getPlugin(const QString& pluginKey) const
{
    const QString id = resolveIdFromKey(pluginKey);
    if (id.isEmpty()) {
        return nullptr;
    }

    const auto it = m_pluginsById.constFind(id);
    return (it != m_pluginsById.constEnd()) ? it.value().instance : nullptr;
}

PluginInterface* PluginManager::getPluginById(const QString& pluginId) const
{
    const auto it = m_pluginsById.constFind(pluginId.trimmed());
    return (it != m_pluginsById.constEnd()) ? it.value().instance : nullptr;
}
