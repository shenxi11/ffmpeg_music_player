#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QObject>
#include <QString>
#include <QWidget>
#include <QIcon>

/**
 * @brief 插件接口基类
 * 所有插件必须实现此接口
 */
class PluginInterface {
public:
    virtual ~PluginInterface() {}
    
    /**
     * @brief 获取插件名称
     * @return 插件显示名称
     */
    virtual QString pluginName() const = 0;
    
    /**
     * @brief 获取插件描述
     * @return 插件功能描述
     */
    virtual QString pluginDescription() const = 0;
    
    /**
     * @brief 获取插件版本
     * @return 插件版本号
     */
    virtual QString pluginVersion() const = 0;
    
    /**
     * @brief 获取插件图标
     * @return 插件图标
     */
    virtual QIcon pluginIcon() const = 0;
    
    /**
     * @brief 创建插件主界面
     * @param parent 父窗口
     * @return 插件的主窗口部件
     */
    virtual QWidget* createWidget(QWidget* parent = nullptr) = 0;
    
    /**
     * @brief 插件初始化
     * @return 成功返回true，失败返回false
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief 插件清理
     */
    virtual void cleanup() = 0;
};

#define PluginInterface_iid "com.ffmpeg_music_player.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGIN_INTERFACE_H
