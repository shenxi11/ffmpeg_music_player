#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>

/**
 * @brief Global settings manager (singleton)
 */
class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(bool downloadLyrics READ downloadLyrics WRITE setDownloadLyrics NOTIFY downloadLyricsChanged)
    Q_PROPERTY(bool downloadCover READ downloadCover WRITE setDownloadCover NOTIFY downloadCoverChanged)
    Q_PROPERTY(QString audioCachePath READ audioCachePath WRITE setAudioCachePath NOTIFY audioCachePathChanged)
    Q_PROPERTY(QString logPath READ logPath WRITE setLogPath NOTIFY logPathChanged)
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverEndpointChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverEndpointChanged)

public:
    static SettingsManager& instance()
    {
        static SettingsManager instance;
        return instance;
    }

    QString downloadPath() const { return m_downloadPath; }
    void setDownloadPath(const QString& path);

    bool downloadLyrics() const { return m_downloadLyrics; }
    void setDownloadLyrics(bool enable);

    bool downloadCover() const { return m_downloadCover; }
    void setDownloadCover(bool enable);

    QString audioCachePath() const { return m_audioCachePath; }
    void setAudioCachePath(const QString& path);

    QString logPath() const { return m_logPath; }
    void setLogPath(const QString& path);

    QString serverHost() const { return m_serverHost; }
    void setServerHost(const QString& host);

    int serverPort() const { return m_serverPort; }
    void setServerPort(int port);

    QString serverBaseUrl() const;
    void setServerEndpoint(const QString& host, int port);

    QString cachedAccount() const { return m_cachedAccount; }
    QString cachedPassword() const { return m_cachedPassword; }
    QString cachedUsername() const { return m_cachedUsername; }
    bool autoLoginEnabled() const { return m_autoLoginEnabled; }

    void saveAccountCache(const QString& account,
                          const QString& password,
                          const QString& username,
                          bool enableAutoLogin);
    void setAutoLoginEnabled(bool enabled);
    void clearAccountCache();

signals:
    void downloadPathChanged();
    void downloadLyricsChanged();
    void downloadCoverChanged();
    void audioCachePathChanged();
    void logPathChanged();
    void serverEndpointChanged();
    void accountCacheChanged();
    void autoLoginChanged();

private:
    SettingsManager();
    ~SettingsManager() = default;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSettings m_settings;
    QString m_downloadPath;
    bool m_downloadLyrics;
    bool m_downloadCover;
    QString m_audioCachePath;
    QString m_logPath;
    QString m_serverHost;
    int m_serverPort = 8080;
    QString m_cachedAccount;
    QString m_cachedPassword;
    QString m_cachedUsername;
    bool m_autoLoginEnabled = false;
};

#endif // SETTINGS_MANAGER_H
