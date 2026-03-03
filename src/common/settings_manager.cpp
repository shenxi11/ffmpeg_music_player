#include "settings_manager.h"
#include <QCoreApplication>
#include <QFileInfo>

namespace {

bool isProjectRoot(const QString& dirPath)
{
    const QDir dir(dirPath);
    return QFileInfo::exists(dir.filePath("qml/components/settings/Settings.qml"));
}

QString defaultAudioCachePath()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.trimmed().isEmpty()) {
        base = QDir::currentPath();
    }
    return QDir(base).absoluteFilePath(QStringLiteral("audio_cache"));
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
    const QString defaultCachePath = defaultAudioCachePath();

    m_downloadPath = m_settings.value("download/path", "D:/Music").toString();
    m_downloadLyrics = m_settings.value("download/lyrics", true).toBool();
    m_downloadCover = m_settings.value("download/cover", false).toBool();
    m_audioCachePath = QDir::cleanPath(
        QDir::fromNativeSeparators(m_settings.value("cache/audio_path", defaultCachePath).toString().trimmed()));
    if (m_audioCachePath.isEmpty()) {
        m_audioCachePath = defaultCachePath;
    }
    QDir().mkpath(m_audioCachePath);
    m_settings.setValue("cache/audio_path", m_audioCachePath);

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

    m_cachedAccount = m_settings.value("account/cache/account").toString().trimmed();
    m_cachedPassword = m_settings.value("account/cache/password").toString();
    m_cachedUsername = m_settings.value("account/cache/username").toString().trimmed();
    m_autoLoginEnabled = m_settings.value("account/cache/auto_login", false).toBool();
    m_serverHost = m_settings.value("server/host", QStringLiteral("192.168.1.208")).toString().trimmed();
    m_serverPort = m_settings.value("server/port", 8080).toInt();
    if (m_serverHost.isEmpty()) {
        m_serverHost = QStringLiteral("192.168.1.208");
        m_settings.setValue("server/host", m_serverHost);
    }
    if (m_serverPort <= 0 || m_serverPort > 65535) {
        m_serverPort = 8080;
        m_settings.setValue("server/port", m_serverPort);
    }

    if (m_cachedAccount.isEmpty() || m_cachedPassword.isEmpty()) {
        m_autoLoginEnabled = false;
        m_settings.setValue("account/cache/auto_login", false);
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

void SettingsManager::setAudioCachePath(const QString& path)
{
    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));
    if (normalized.isEmpty()) {
        return;
    }

    QDir dir;
    if (!dir.mkpath(normalized)) {
        qWarning() << "[SettingsManager] Failed to create audio cache directory:" << normalized;
        return;
    }

    if (m_audioCachePath != normalized) {
        m_audioCachePath = normalized;
        m_settings.setValue("cache/audio_path", m_audioCachePath);
        emit audioCachePathChanged();
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

void SettingsManager::setServerHost(const QString& host)
{
    const QString normalized = host.trimmed();
    if (normalized.isEmpty() || m_serverHost == normalized) {
        return;
    }

    m_serverHost = normalized;
    m_settings.setValue("server/host", m_serverHost);
    emit serverEndpointChanged();
}

void SettingsManager::setServerPort(int port)
{
    if (port <= 0 || port > 65535 || m_serverPort == port) {
        return;
    }

    m_serverPort = port;
    m_settings.setValue("server/port", m_serverPort);
    emit serverEndpointChanged();
}

QString SettingsManager::serverBaseUrl() const
{
    return QStringLiteral("http://%1:%2/").arg(m_serverHost, QString::number(m_serverPort));
}

void SettingsManager::setServerEndpoint(const QString& host, int port)
{
    const QString normalizedHost = host.trimmed();
    if (normalizedHost.isEmpty() || port <= 0 || port > 65535) {
        return;
    }

    const bool changed = (m_serverHost != normalizedHost) || (m_serverPort != port);
    if (!changed) {
        return;
    }

    m_serverHost = normalizedHost;
    m_serverPort = port;
    m_settings.setValue("server/host", m_serverHost);
    m_settings.setValue("server/port", m_serverPort);
    emit serverEndpointChanged();
}

void SettingsManager::saveAccountCache(const QString& account,
                                       const QString& password,
                                       const QString& username,
                                       bool enableAutoLogin)
{
    const QString trimmedAccount = account.trimmed();
    if (trimmedAccount.isEmpty()) {
        return;
    }

    const bool cacheChanged = m_cachedAccount != trimmedAccount
            || m_cachedPassword != password
            || m_cachedUsername != username;
    const bool autoChanged = m_autoLoginEnabled != enableAutoLogin;

    m_cachedAccount = trimmedAccount;
    m_cachedPassword = password;
    m_cachedUsername = username.trimmed();
    m_autoLoginEnabled = enableAutoLogin;

    m_settings.setValue("account/cache/account", m_cachedAccount);
    m_settings.setValue("account/cache/password", m_cachedPassword);
    m_settings.setValue("account/cache/username", m_cachedUsername);
    m_settings.setValue("account/cache/auto_login", m_autoLoginEnabled);

    if (cacheChanged) {
        emit accountCacheChanged();
    }
    if (autoChanged) {
        emit autoLoginChanged();
    }
}

void SettingsManager::setAutoLoginEnabled(bool enabled)
{
    if (enabled && (m_cachedAccount.isEmpty() || m_cachedPassword.isEmpty())) {
        enabled = false;
    }

    if (m_autoLoginEnabled == enabled) {
        return;
    }

    m_autoLoginEnabled = enabled;
    m_settings.setValue("account/cache/auto_login", m_autoLoginEnabled);
    emit autoLoginChanged();
}

void SettingsManager::clearAccountCache()
{
    const bool hadCache = !m_cachedAccount.isEmpty()
            || !m_cachedPassword.isEmpty()
            || !m_cachedUsername.isEmpty();
    const bool autoChanged = m_autoLoginEnabled;

    m_cachedAccount.clear();
    m_cachedPassword.clear();
    m_cachedUsername.clear();
    m_autoLoginEnabled = false;

    m_settings.remove("account/cache/account");
    m_settings.remove("account/cache/password");
    m_settings.remove("account/cache/username");
    m_settings.setValue("account/cache/auto_login", false);

    if (hadCache) {
        emit accountCacheChanged();
    }
    if (autoChanged) {
        emit autoLoginChanged();
    }
}
