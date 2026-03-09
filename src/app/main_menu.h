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
    void refreshPlugins();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void pluginRequested(const QString& pluginId);
    void pluginDiagnosticsRequested();
    void settingsRequested();
    void aboutRequested();

private slots:
    void onPluginButtonClicked();
    void onPluginDiagnosticsClicked();
    void onSettingsClicked();
    void onAboutClicked();

private:
    void setupUI();
    void hideMenu();
    void createPluginButtons();
    QString createButtonStyle();

    QVBoxLayout* menuLayout;
    QVector<QPushButton*> pluginButtons;
    QPushButton* diagnosticsBtn;
    QPushButton* settingsBtn;
    QPushButton* aboutBtn;
    QTimer* hideTimer;
};

#endif // MAINMENU_H
