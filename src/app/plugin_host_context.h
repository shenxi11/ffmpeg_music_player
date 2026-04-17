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

    Q_INVOKABLE void registerService(const QString& serviceName, QObject* service);
    Q_INVOKABLE void unregisterService(const QString& serviceName);
    Q_INVOKABLE QObject* service(const QString& serviceName) const;
    Q_INVOKABLE QStringList serviceKeys() const;

    Q_INVOKABLE void setEnvironmentValue(const QString& key, const QVariant& value);
    Q_INVOKABLE QVariant environmentValue(const QString& key,
                                          const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE QVariantMap environmentSnapshot() const;

private:
    QHash<QString, QPointer<QObject>> m_services;
    QVariantMap m_environment;
};

#endif // PLUGIN_HOST_CONTEXT_H
