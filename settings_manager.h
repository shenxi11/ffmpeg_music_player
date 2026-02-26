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
    Q_PROPERTY(QString logPath READ logPath WRITE setLogPath NOTIFY logPathChanged)

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

    QString logPath() const { return m_logPath; }
    void setLogPath(const QString& path);

signals:
    void downloadPathChanged();
    void downloadLyricsChanged();
    void downloadCoverChanged();
    void logPathChanged();

private:
    SettingsManager();
    ~SettingsManager() = default;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSettings m_settings;
    QString m_downloadPath;
    bool m_downloadLyrics;
    bool m_downloadCover;
    QString m_logPath;
};

#endif // SETTINGS_MANAGER_H
