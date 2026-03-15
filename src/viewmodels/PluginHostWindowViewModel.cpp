#include "PluginHostWindowViewModel.h"

#include "settings_manager.h"

PluginHostWindowViewModel::PluginHostWindowViewModel(QObject* parent)
    : BaseViewModel(parent)
{
}

QByteArray PluginHostWindowViewModel::loadWindowGeometry(const QString& pluginId) const
{
    if (pluginId.trimmed().isEmpty()) {
        return QByteArray();
    }
    return SettingsManager::instance().pluginWindowGeometry(pluginId);
}

void PluginHostWindowViewModel::saveWindowGeometry(const QString& pluginId, const QByteArray& geometry)
{
    if (pluginId.trimmed().isEmpty() || geometry.isEmpty()) {
        return;
    }
    SettingsManager::instance().setPluginWindowGeometry(pluginId, geometry);
}
