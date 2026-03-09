#ifndef PLUGIN_HOST_WINDOW_H
#define PLUGIN_HOST_WINDOW_H

#include <QDialog>
#include <QIcon>
#include <QString>

class QVBoxLayout;
class QWidget;

class PluginHostWindow : public QDialog
{
    Q_OBJECT
public:
    struct Meta {
        QString pluginId;
        QString name;
        QString description;
        QString version;
        QIcon icon;
    };

    explicit PluginHostWindow(const Meta& meta, QWidget* parent = nullptr);
    void setPluginContent(QWidget* content);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void showHelpDialog();

private:
    void restoreWindowGeometry();
    void persistWindowGeometry() const;

private:
    Meta m_meta;
    QWidget* m_contentCard = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
};

#endif // PLUGIN_HOST_WINDOW_H
