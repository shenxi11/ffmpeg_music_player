#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDir>
#include <QPluginLoader>
#include <QHash>
#include "plugin_interface.h"

/**
 * @brief 插件信息结构
 */
struct PluginInfo {
    QString name;
    QString description;
    QString version;
    QIcon icon;
    PluginInterface* instance;
    QPluginLoader* loader;
    QString filePath;
    bool isLoaded;
};

/**
 * @brief 插件管理器
 * 负责插件的加载、卸载和管理
 */
class PluginManager : public QObject {
    Q_OBJECT
public:
    static PluginManager& instance();
    
    /**
     * @brief 加载指定目录下的所有插件
     * @param pluginDirPath 插件目录路径
     * @return 成功加载的插件数量
     */
    int loadPlugins(const QString& pluginDirPath);
    
    /**
     * @brief 加载单个插件
     * @param pluginFilePath 插件文件路径
     * @return 成功返回true
     */
    bool loadPlugin(const QString& pluginFilePath);
    
    /**
     * @brief 卸载插件
     * @param pluginName 插件名称
     */
    void unloadPlugin(const QString& pluginName);
    
    /**
     * @brief 卸载所有插件
     */
    void unloadAllPlugins();
    
    /**
     * @brief 获取所有已加载的插件信息
     * @return 插件信息列表
     */
    QVector<PluginInfo> getPluginInfos() const;
    
    /**
     * @brief 根据名称获取插件实例
     * @param pluginName 插件名称
     * @return 插件实例指针，未找到返回nullptr
     */
    PluginInterface* getPlugin(const QString& pluginName) const;
    
    /**
     * @brief 获取插件数量
     * @return 已加载插件数量
     */
    int pluginCount() const { return m_plugins.size(); }

signals:
    /**
     * @brief 插件加载完成信号
     * @param pluginName 插件名称
     */
    void pluginLoaded(const QString& pluginName);
    
    /**
     * @brief 插件卸载信号
     * @param pluginName 插件名称
     */
    void pluginUnloaded(const QString& pluginName);

private:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager();
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    QHash<QString, PluginInfo> m_plugins;
};

#endif // PLUGIN_MANAGER_H
