#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QFileDialog>
#include <QDebug>
#include "settings_manager.h"

/**
 * @brief 设置窗口的QML包装类
 */
class SettingsWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

private slots:
    void onChooseDownloadPath();
    void onDownloadPathChanged();
    void onDownloadLyricsChanged();
    void onDownloadCoverChanged();
};

#endif // SETTINGS_WIDGET_H
