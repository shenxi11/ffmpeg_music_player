#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTimer>

#include "viewmodels/SettingsViewModel.h"

class SettingsWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);

private slots:
    void onChooseDownloadPath();
    void onChooseAudioCachePath();
    void onChooseLogPath();
    void onClearLocalCacheRequested();
    void onDownloadPathChanged();
    void onDownloadLyricsChanged();
    void onDownloadCoverChanged();
    void onAudioCachePathChanged();
    void onLogPathChanged();
    void onPlayerPageStyleChanged();
    void onRefreshPresenceRequested();
    void syncViewModelToRoot();
    void syncPresenceToRoot();

private:
    // 连接拆分：保持构造函数聚焦于 UI 构建流程。
    void setupRootConnections(QQuickItem* root);
    void setupViewModelConnections();
    void setupRefreshTimer();

    QTimer m_presenceRefreshTimer;
    SettingsViewModel* m_viewModel = nullptr;
};

#endif // SETTINGS_WIDGET_H
