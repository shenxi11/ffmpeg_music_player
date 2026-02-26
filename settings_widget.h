#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>

#include "settings_manager.h"

class SettingsWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

private slots:
    void onChooseDownloadPath();
    void onChooseLogPath();
    void onDownloadPathChanged();
    void onDownloadLyricsChanged();
    void onDownloadCoverChanged();
    void onLogPathChanged();
};

#endif // SETTINGS_WIDGET_H
