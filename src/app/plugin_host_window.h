#ifndef PLUGIN_HOST_WINDOW_H
#define PLUGIN_HOST_WINDOW_H

#include <QDialog>
#include <QIcon>
#include <QString>

class QVBoxLayout;
class QWidget;
class PluginHostWindowViewModel;

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
    // 连接拆分：集中管理宿主窗口内部交互连接。
    void setupConnections(QPushButton* helpButton);
    void restoreWindowGeometry();
    void persistWindowGeometry() const;

private:
    Meta m_meta;
    QWidget* m_contentCard = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    PluginHostWindowViewModel* m_viewModel = nullptr;
};

#endif // PLUGIN_HOST_WINDOW_H
