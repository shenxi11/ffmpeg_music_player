#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

#include "online_presence_manager.h"
#include "settings_manager.h"

class SettingsWidget : public QQuickWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

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
    void onRefreshPresenceRequested();
    void onPresenceSnapshotChanged(const QString& account,
                                   const QString& sessionToken,
                                   bool online,
                                   int heartbeatIntervalSec,
                                   int onlineTtlSec,
                                   int ttlRemainingSec,
                                   const QString& statusMessage,
                                   qint64 lastSeenAt);

private:
    QTimer m_presenceRefreshTimer;
};

#endif // SETTINGS_WIDGET_H
