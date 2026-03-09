#include "plugin_host_context.h"

PluginHostContext::PluginHostContext(QObject* parent)
    : QObject(parent)
{
}

void PluginHostContext::registerService(const QString& serviceName, QObject* service)
{
    const QString key = serviceName.trimmed();
    if (key.isEmpty()) {
        return;
    }
    m_services.insert(key, service);
}

void PluginHostContext::unregisterService(const QString& serviceName)
{
    m_services.remove(serviceName.trimmed());
}

QObject* PluginHostContext::service(const QString& serviceName) const
{
    const auto it = m_services.constFind(serviceName.trimmed());
    if (it == m_services.constEnd()) {
        return nullptr;
    }
    return it.value().data();
}

QStringList PluginHostContext::serviceKeys() const
{
    return m_services.keys();
}

void PluginHostContext::setEnvironmentValue(const QString& key, const QVariant& value)
{
    const QString normalized = key.trimmed();
    if (normalized.isEmpty()) {
        return;
    }
    m_environment.insert(normalized, value);
}

QVariant PluginHostContext::environmentValue(const QString& key, const QVariant& defaultValue) const
{
    return m_environment.value(key.trimmed(), defaultValue);
}

QVariantMap PluginHostContext::environmentSnapshot() const
{
    return m_environment;
}
