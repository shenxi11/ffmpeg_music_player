#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include "BaseViewModel.h"

#include <QDateTime>
#include <QString>

class QWidget;

/**
 * @brief 设置窗口视图模型。
 *
 * 负责下载目录、缓存目录、日志路径、在线状态与缓存清理等设置逻辑，
 * 让设置界面仅负责展示属性和转发交互。
 */
class SettingsViewModel : public BaseViewModel
{
    Q_OBJECT

    Q_PROPERTY(QString downloadPath READ downloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(QString audioCachePath READ audioCachePath NOTIFY audioCachePathChanged)
    Q_PROPERTY(QString logPath READ logPath NOTIFY logPathChanged)
    Q_PROPERTY(bool downloadLyrics READ downloadLyrics NOTIFY downloadLyricsChanged)
    Q_PROPERTY(bool downloadCover READ downloadCover NOTIFY downloadCoverChanged)
    Q_PROPERTY(int playerPageStyle READ playerPageStyle NOTIFY playerPageStyleChanged)
    Q_PROPERTY(QString presenceAccount READ presenceAccount NOTIFY presenceChanged)
    Q_PROPERTY(QString presenceSessionToken READ presenceSessionToken NOTIFY presenceChanged)
    Q_PROPERTY(bool presenceOnline READ presenceOnline NOTIFY presenceChanged)
    Q_PROPERTY(int presenceHeartbeatIntervalSec READ presenceHeartbeatIntervalSec NOTIFY presenceChanged)
    Q_PROPERTY(int presenceOnlineTtlSec READ presenceOnlineTtlSec NOTIFY presenceChanged)
    Q_PROPERTY(int presenceTtlRemainingSec READ presenceTtlRemainingSec NOTIFY presenceChanged)
    Q_PROPERTY(QString presenceStatusMessage READ presenceStatusMessage NOTIFY presenceChanged)
    Q_PROPERTY(QString presenceLastSeenText READ presenceLastSeenText NOTIFY presenceChanged)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);

    QString downloadPath() const;
    QString audioCachePath() const;
    QString logPath() const;
    bool downloadLyrics() const;
    bool downloadCover() const;
    int playerPageStyle() const;
    QString serverHost() const;
    int serverPort() const;

    QString presenceAccount() const { return m_presenceAccount; }
    QString presenceSessionToken() const { return m_presenceSessionToken; }
    bool presenceOnline() const { return m_presenceOnline; }
    int presenceHeartbeatIntervalSec() const { return m_presenceHeartbeatIntervalSec; }
    int presenceOnlineTtlSec() const { return m_presenceOnlineTtlSec; }
    int presenceTtlRemainingSec() const { return m_presenceTtlRemainingSec; }
    QString presenceStatusMessage() const { return m_presenceStatusMessage; }
    QString presenceLastSeenText() const { return m_presenceLastSeenText; }

    void chooseDownloadPath(QWidget* parent);
    void chooseAudioCachePath(QWidget* parent);
    void chooseLogPath(QWidget* parent);
    void clearLocalCache(QWidget* parent);
    void setDownloadPath(const QString& path);
    void setAudioCachePath(const QString& path);
    void setLogPath(const QString& path);
    void setDownloadLyrics(bool enabled);
    void setDownloadCover(bool enabled);
    void setPlayerPageStyle(int styleId);
    void refreshPresence();
    void syncFromSettings();

signals:
    void downloadPathChanged();
    void audioCachePathChanged();
    void logPathChanged();
    void downloadLyricsChanged();
    void downloadCoverChanged();
    void playerPageStyleChanged();
    void presenceChanged();
    void messageRequested(const QString& title, const QString& message);
    void warningRequested(const QString& title, const QString& message);
    void questionRequested(const QString& title,
                           const QString& message,
                           const QString& context);

private slots:
    void onPresenceSnapshotChanged(const QString& account,
                                   const QString& sessionToken,
                                   bool online,
                                   int heartbeatIntervalSec,
                                   int onlineTtlSec,
                                   int ttlRemainingSec,
                                   const QString& statusMessage,
                                   qint64 lastSeenAt);

private:
    void updatePresenceSnapshot(const QString& account,
                                const QString& sessionToken,
                                bool online,
                                int heartbeatIntervalSec,
                                int onlineTtlSec,
                                int ttlRemainingSec,
                                const QString& statusMessage,
                                qint64 lastSeenAt);

    QString m_presenceAccount;
    QString m_presenceSessionToken;
    bool m_presenceOnline = false;
    int m_presenceHeartbeatIntervalSec = 0;
    int m_presenceOnlineTtlSec = 0;
    int m_presenceTtlRemainingSec = 0;
    QString m_presenceStatusMessage;
    QString m_presenceLastSeenText;
};

#endif // SETTINGSVIEWMODEL_H
