#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QDebug>
#include <QVector>
#include <QPainterPath>
#include "headers.h"
#include "plugin_manager.h"

class MainMenu : public QWidget
{
    Q_OBJECT
public:
    explicit MainMenu(QWidget *parent = nullptr);
    void showMenu(const QPoint& position);
    void refreshPlugins(); // 刷新插件列表

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void pluginRequested(const QString& pluginName); // 通用插件请求信号
    void settingsRequested();
    void aboutRequested();

private slots:
    void onPluginButtonClicked(); // 通用插件按钮点击槽
    void onSettingsClicked();
    void onAboutClicked();

private:
    void setupUI();
    void hideMenu();
    void createPluginButtons(); // 创建插件按钮
    QString createButtonStyle();
    
    QVBoxLayout* menuLayout;
    QVector<QPushButton*> pluginButtons; // 存储插件按钮
    QPushButton* settingsBtn;
    QPushButton* aboutBtn;
    QTimer* hideTimer;
};

#endif // MAINMENU_H
