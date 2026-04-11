#ifndef PLUGIN_HOST_CONTEXT_H
#define PLUGIN_HOST_CONTEXT_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

class PluginHostContext : public QObject
{
    Q_OBJECT
public:
    explicit PluginHostContext(QObject* parent = nullptr);

    void registerService(const QString& serviceName, QObject* service);
    void unregisterService(const QString& serviceName);
    QObject* service(const QString& serviceName) const;
    QStringList serviceKeys() const;

    void setEnvironmentValue(const QString& key, const QVariant& value);
    QVariant environmentValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVariantMap environmentSnapshot() const;

private:
    QHash<QString, QPointer<QObject>> m_services;
    QVariantMap m_environment;
};

#endif // PLUGIN_HOST_CONTEXT_H
