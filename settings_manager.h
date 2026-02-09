#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStandardPaths>
#include <QDebug>

/**
 * @brief 全局设置管理器（单例模式）
 * 负责保存和读取应用程序设置
 */
class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(bool downloadLyrics READ downloadLyrics WRITE setDownloadLyrics NOTIFY downloadLyricsChanged)
    Q_PROPERTY(bool downloadCover READ downloadCover WRITE setDownloadCover NOTIFY downloadCoverChanged)

public:
    static SettingsManager& instance()
    {
        static SettingsManager instance;
        return instance;
    }

    // 下载路径
    QString downloadPath() const { return m_downloadPath; }
    void setDownloadPath(const QString& path);

    // 是否下载歌词
    bool downloadLyrics() const { return m_downloadLyrics; }
    void setDownloadLyrics(bool enable);

    // 是否下载专辑图片
    bool downloadCover() const { return m_downloadCover; }
    void setDownloadCover(bool enable);

signals:
    void downloadPathChanged();
    void downloadLyricsChanged();
    void downloadCoverChanged();

private:
    SettingsManager();
    ~SettingsManager() = default;

    // 禁用拷贝和赋值
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSettings m_settings;
    QString m_downloadPath;
    bool m_downloadLyrics;
    bool m_downloadCover;
};

#endif // SETTINGS_MANAGER_H
