#include "settings_manager.h"

SettingsManager::SettingsManager()
    : m_settings("FFmpegMusicPlayer", "Settings")
{
    // 读取设置，如果不存在则使用默认值
    m_downloadPath = m_settings.value("download/path", "D:/Music").toString();
    m_downloadLyrics = m_settings.value("download/lyrics", true).toBool();
    m_downloadCover = m_settings.value("download/cover", false).toBool();

    qDebug() << "[SettingsManager] Initialized with:";
    qDebug() << "  Download Path:" << m_downloadPath;
    qDebug() << "  Download Lyrics:" << m_downloadLyrics;
    qDebug() << "  Download Cover:" << m_downloadCover;
}

void SettingsManager::setDownloadPath(const QString& path)
{
    if (m_downloadPath != path) {
        m_downloadPath = path;
        m_settings.setValue("download/path", path);
        emit downloadPathChanged();
    }
}

void SettingsManager::setDownloadLyrics(bool enable)
{
    if (m_downloadLyrics != enable) {
        m_downloadLyrics = enable;
        m_settings.setValue("download/lyrics", enable);
        emit downloadLyricsChanged();
    }
}

void SettingsManager::setDownloadCover(bool enable)
{
    if (m_downloadCover != enable) {
        m_downloadCover = enable;
        m_settings.setValue("download/cover", enable);
        emit downloadCoverChanged();
    }
}
