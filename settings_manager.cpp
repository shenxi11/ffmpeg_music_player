#include "settings_manager.h"
#include <QCoreApplication>
#include <QFileInfo>

namespace {

bool isProjectRoot(const QString& dirPath)
{
    const QDir dir(dirPath);
    return QFileInfo::exists(dir.filePath("qml/components/Settings.qml"));
}

QString inferPreferredLogPath()
{
    const QString cwd = QDir::currentPath();
    if (isProjectRoot(cwd)) {
        return QDir(cwd).absoluteFilePath(QStringLiteral("打印日志.txt"));
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString sourceSibling = QDir::cleanPath(appDir + "/../../ffmpeg_music_player");
    if (isProjectRoot(sourceSibling)) {
        return QDir(sourceSibling).absoluteFilePath(QStringLiteral("打印日志.txt"));
    }

    const QString cwdSibling = QDir::cleanPath(cwd + "/../ffmpeg_music_player");
    if (isProjectRoot(cwdSibling)) {
        return QDir(cwdSibling).absoluteFilePath(QStringLiteral("打印日志.txt"));
    }

    return QDir(cwd).absoluteFilePath(QStringLiteral("打印日志.txt"));
}

bool looksLikeBuildDefaultLogPath(const QString& path)
{
    return path.contains("ffmpeg_music_player_build", Qt::CaseInsensitive)
        && QFileInfo(path).fileName() == QStringLiteral("打印日志.txt");
}

} // namespace

SettingsManager::SettingsManager()
    : m_settings("FFmpegMusicPlayer", "Settings")
{
    const QString preferredPath = inferPreferredLogPath();

    m_downloadPath = m_settings.value("download/path", "D:/Music").toString();
    m_downloadLyrics = m_settings.value("download/lyrics", true).toBool();
    m_downloadCover = m_settings.value("download/cover", false).toBool();

    const QString storedLogPath = QDir::cleanPath(
        QDir::fromNativeSeparators(m_settings.value("logging/path").toString().trimmed()));

    if (storedLogPath.isEmpty()) {
        m_logPath = preferredPath;
        m_settings.setValue("logging/path", m_logPath);
    } else if (looksLikeBuildDefaultLogPath(storedLogPath) && storedLogPath != preferredPath) {
        // Migrate old default build-dir log path to project-root log path.
        m_logPath = preferredPath;
        m_settings.setValue("logging/path", m_logPath);
    } else {
        m_logPath = storedLogPath;
    }
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

void SettingsManager::setLogPath(const QString& path)
{
    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));
    if (normalized.isEmpty()) {
        return;
    }

    if (m_logPath != normalized) {
        m_logPath = normalized;
        m_settings.setValue("logging/path", m_logPath);
        emit logPathChanged();
    }
}
