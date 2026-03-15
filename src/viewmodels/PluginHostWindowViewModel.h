#ifndef PLUGINHOSTWINDOWVIEWMODEL_H
#define PLUGINHOSTWINDOWVIEWMODEL_H

#include "BaseViewModel.h"

#include <QByteArray>
#include <QString>

/**
 * @brief 插件窗口视图模型。
 *
 * 负责插件窗口几何信息的读取与持久化，
 * 避免插件窗口视图直接依赖设置单例。
 */
class PluginHostWindowViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit PluginHostWindowViewModel(QObject* parent = nullptr);

    QByteArray loadWindowGeometry(const QString& pluginId) const;
    void saveWindowGeometry(const QString& pluginId, const QByteArray& geometry);
};

#endif // PLUGINHOSTWINDOWVIEWMODEL_H
