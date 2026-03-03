#include "plugin_manager.h"
#include <QDebug>
#include <QFileInfo>

PluginManager::PluginManager(QObject* parent) 
    : QObject(parent) 
{
    qDebug() << "PluginManager initialized";
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

int PluginManager::loadPlugins(const QString& pluginDirPath) 
{
    QDir pluginsDir(pluginDirPath);
    
    if (!pluginsDir.exists()) {
        qWarning() << "Plugin directory does not exist:" << pluginDirPath;
        pluginsDir.mkpath(pluginDirPath);
        return 0;
    }
    
    qDebug() << "Loading plugins from:" << pluginsDir.absolutePath();
    
    int loadedCount = 0;
    QStringList filters;
#ifdef Q_OS_WIN
    filters << "*_plugin.dll";  // 只加载以 _plugin.dll 结尾的文件
#elif defined(Q_OS_LINUX)
    filters << "*_plugin.so";
#elif defined(Q_OS_MAC)
    filters << "*_plugin.dylib";
#endif
    
    pluginsDir.setNameFilters(filters);
    QStringList pluginFiles = pluginsDir.entryList(QDir::Files);
    
    qDebug() << "Found" << pluginFiles.size() << "plugin files";
    
    for (const QString& fileName : pluginFiles) {
        QString filePath = pluginsDir.absoluteFilePath(fileName);
        if (loadPlugin(filePath)) {
            loadedCount++;
        }
    }
    
    qDebug() << "Successfully loaded" << loadedCount << "plugins";
    return loadedCount;
}

bool PluginManager::loadPlugin(const QString& pluginFilePath) 
{
    QFileInfo fileInfo(pluginFilePath);
    QString fileName = fileInfo.fileName();
    
    qDebug() << "Attempting to load plugin:" << fileName;
    
    QPluginLoader* loader = new QPluginLoader(pluginFilePath, this);
    QObject* pluginObj = loader->instance();
    
    if (!pluginObj) {
        qWarning() << "Failed to load plugin:" << fileName;
        qWarning() << "Error:" << loader->errorString();
        delete loader;
        return false;
    }
    
    PluginInterface* plugin = qobject_cast<PluginInterface*>(pluginObj);
    if (!plugin) {
        qWarning() << "Plugin does not implement PluginInterface:" << fileName;
        loader->unload();
        delete loader;
        return false;
    }
    
    // 初始化插件
    if (!plugin->initialize()) {
        qWarning() << "Plugin initialization failed:" << fileName;
        loader->unload();
        delete loader;
        return false;
    }
    
    // 保存插件信息
    PluginInfo info;
    info.name = plugin->pluginName();
    info.description = plugin->pluginDescription();
    info.version = plugin->pluginVersion();
    info.icon = plugin->pluginIcon();
    info.instance = plugin;
    info.loader = loader;
    info.filePath = pluginFilePath;
    info.isLoaded = true;
    
    m_plugins[info.name] = info;
    
    qDebug() << "Plugin loaded successfully:" << info.name 
             << "Version:" << info.version;
    
    emit pluginLoaded(info.name);
    return true;
}

void PluginManager::unloadPlugin(const QString& pluginName) 
{
    if (!m_plugins.contains(pluginName)) {
        qWarning() << "Plugin not found:" << pluginName;
        return;
    }
    
    PluginInfo info = m_plugins[pluginName];
    
    // 清理插件
    if (info.instance) {
        info.instance->cleanup();
    }
    
    // 卸载插件
    if (info.loader) {
        info.loader->unload();
        delete info.loader;
    }
    
    m_plugins.remove(pluginName);
    
    qDebug() << "Plugin unloaded:" << pluginName;
    emit pluginUnloaded(pluginName);
}

void PluginManager::unloadAllPlugins() 
{
    qDebug() << "Unloading all plugins...";
    
    QStringList pluginNames = m_plugins.keys();
    for (const QString& name : pluginNames) {
        unloadPlugin(name);
    }
    
    m_plugins.clear();
    qDebug() << "All plugins unloaded";
}

QVector<PluginInfo> PluginManager::getPluginInfos() const 
{
    QVector<PluginInfo> infos;
    for (const PluginInfo& info : m_plugins.values()) {
        infos.append(info);
    }
    return infos;
}

PluginInterface* PluginManager::getPlugin(const QString& pluginName) const 
{
    if (m_plugins.contains(pluginName)) {
        return m_plugins[pluginName].instance;
    }
    return nullptr;
}
