#include "settings_manager.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace {

bool isProjectRoot(const QString& dirPath) {
    const QDir dir(dirPath);
    return QFileInfo::exists(dir.filePath("qml/components/settings/Settings.qml"));
}

QString defaultAudioCachePath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.trimmed().isEmpty()) {
        base = QDir::currentPath();
    }
    return QDir(base).absoluteFilePath(QStringLiteral("audio_cache"));
}

QString inferPreferredLogPath() {
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

bool looksLikeBuildDefaultLogPath(const QString& path) {
    return path.contains("ffmpeg_music_player_build", Qt::CaseInsensitive) &&
           QFileInfo(path).fileName() == QStringLiteral("打印日志.txt");
}

QString normalizedPluginKey(const QString& pluginId) {
    QString key = pluginId.trimmed();
    key.replace('/', '_');
    key.replace('\\', '_');
    if (key.isEmpty()) {
        key = QStringLiteral("unknown_plugin");
    }
    return key;
}

QString normalizeSearchKeyword(const QString& keyword) {
    return keyword.trimmed();
}

QString normalizeShortcut(const QString& shortcut) {
    return shortcut.trimmed().toUpper();
}

QString normalizeFontFamily(const QString& fontFamily) {
    return fontFamily.trimmed();
}

QString normalizeChoice(const QString& value, const QString& fallback) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

QString defaultPluginDirectory() {
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("Plugins"));
}

} // namespace

SettingsManager::SettingsManager() : m_settings("FFmpegMusicPlayer", "Settings") {
    static constexpr int kDefaultPlayerPageStyle = 0;
    static constexpr int kMaxPlayerPageStyle = 4;
    static constexpr int kMaxSearchHistoryItems = 10;
    static constexpr int kDefaultProxyPort = 0;
    static constexpr int kDefaultSampleRate = 44100;
    const QString preferredPath = inferPreferredLogPath();
    const QString defaultCachePath = defaultAudioCachePath();

    m_downloadPath = m_settings.value("download/path", "D:/Music").toString();
    m_downloadLyrics = m_settings.value("download/lyrics", true).toBool();
    m_downloadCover = m_settings.value("download/cover", false).toBool();
    m_audioCachePath = QDir::cleanPath(QDir::fromNativeSeparators(
        m_settings.value("cache/audio_path", defaultCachePath).toString().trimmed()));
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
    m_cachedAvatarUrl = m_settings.value("account/cache/avatar_url").toString().trimmed();
    m_cachedOnlineSessionToken =
        m_settings.value("account/cache/online_session_token").toString().trimmed();
    m_cachedProfileCreatedAt =
        m_settings.value("account/cache/profile_created_at").toString().trimmed();
    m_cachedProfileUpdatedAt =
        m_settings.value("account/cache/profile_updated_at").toString().trimmed();
    m_autoLoginEnabled = m_settings.value("account/cache/auto_login", false).toBool();
    m_manualLogoutMarked = m_settings.value("account/cache/manual_logout", false).toBool();
    m_serverHost =
        m_settings.value("server/host", QStringLiteral("192.168.1.208")).toString().trimmed();
    m_serverPort = m_settings.value("server/port", 8080).toInt();
    m_playerPageStyle = m_settings.value("player/page_style", kDefaultPlayerPageStyle).toInt();
    m_launchAtStartup = m_settings.value("settings/general/launch_at_startup", false).toBool();
    m_autoPlayOnStartup = m_settings.value("settings/general/auto_play_on_startup", false).toBool();
    m_autoOpenLyrics = m_settings.value("settings/general/auto_open_lyrics", false).toBool();
    m_disableDpiScale = m_settings.value("settings/general/disable_dpi_scale", false).toBool();
    m_disableHardwareAcceleration =
        m_settings.value("settings/general/disable_hardware_acceleration", false).toBool();
    m_preventSystemSleep =
        m_settings.value("settings/general/prevent_system_sleep", false).toBool();
    m_listAutoSwitch = m_settings.value("settings/general/list_auto_switch", true).toBool();
    m_enableFadeInOut = m_settings.value("settings/general/enable_fade_in_out", true).toBool();
    m_enableMultimediaKeys =
        m_settings.value("settings/general/enable_multimedia_keys", true).toBool();
    m_autoBestVolume = m_settings.value("settings/general/auto_best_volume", false).toBool();
    m_playbackAccelerationService =
        m_settings.value("settings/general/playback_acceleration_service", true).toBool();
    m_preferLocalDownloadedQuality =
        m_settings.value("settings/general/prefer_local_downloaded_quality", true).toBool();
    m_desktopLyricsShowOnStartup =
        m_settings.value("settings/desktop_lyrics/show_on_startup", false).toBool();
    m_desktopLyricsAlwaysOnTop =
        m_settings.value("settings/desktop_lyrics/always_on_top", false).toBool();
    m_desktopLyricColor =
        m_settings.value("DeskLrc/color", QColor(QStringLiteral("#ffffff"))).value<QColor>();
    if (!m_desktopLyricColor.isValid()) {
        m_desktopLyricColor = QColor(QStringLiteral("#ffffff"));
    }
    m_desktopLyricFontSize =
        qBound(12, m_settings.value("DeskLrc/fontSize", 57).toInt(), 64);
    m_desktopLyricFontFamily = normalizeFontFamily(
        m_settings.value("DeskLrc/fontFamily", QStringLiteral("Microsoft YaHei")).toString());
    if (m_desktopLyricFontFamily.isEmpty()) {
        m_desktopLyricFontFamily = QStringLiteral("Microsoft YaHei");
    }
    m_shortcutPlayPause = normalizeShortcut(
        m_settings.value("settings/shortcuts/play_pause", QStringLiteral("CTRL+ALT+F5")).toString());
    m_shortcutPreviousTrack = normalizeShortcut(
        m_settings.value("settings/shortcuts/previous_track", QStringLiteral("CTRL+ALT+LEFT"))
            .toString());
    m_shortcutNextTrack = normalizeShortcut(
        m_settings.value("settings/shortcuts/next_track", QStringLiteral("CTRL+ALT+RIGHT"))
            .toString());
    m_shortcutToggleDesktopLyrics = normalizeShortcut(
        m_settings
            .value("settings/shortcuts/toggle_desktop_lyrics", QStringLiteral("CTRL+ALT+W"))
            .toString());
    m_shortcutOpenSearch = normalizeShortcut(
        m_settings.value("settings/shortcuts/open_search", QStringLiteral("CTRL+K")).toString());
    m_audioEffectsEnabled =
        m_settings.value("settings/effects/audio_effects_enabled", false).toBool();
    m_replayGainEnabled = m_settings.value("settings/effects/replay_gain_enabled", false).toBool();
    m_volumeNormalizationEnabled =
        m_settings.value("settings/effects/volume_normalization_enabled", false).toBool();
    m_equalizerEnabled = m_settings.value("settings/effects/equalizer_enabled", false).toBool();
    m_effectPreset = m_settings
                         .value("settings/effects/effect_preset", QStringLiteral("默认"))
                         .toString()
                         .trimmed();
    if (m_effectPreset.isEmpty()) {
        m_effectPreset = QStringLiteral("默认");
    }
    m_proxyEnabled = m_settings.value("settings/network/proxy_enabled", false).toBool();
    m_proxyHost = m_settings.value("settings/network/proxy_host").toString().trimmed();
    m_proxyPort = m_settings.value("settings/network/proxy_port", kDefaultProxyPort).toInt();
    if (m_proxyPort <= 0 || m_proxyPort > 65535) {
        m_proxyPort = kDefaultProxyPort;
    }
    m_followSystemAudioDevice =
        m_settings.value("settings/audio_device/follow_system_device", true).toBool();
    m_exclusiveAudioDeviceMode =
        m_settings.value("settings/audio_device/exclusive_mode", false).toBool();
    m_lowLatencyAudioMode =
        m_settings.value("settings/audio_device/low_latency_mode", false).toBool();
    m_outputSampleRate = m_settings
                             .value("settings/audio_device/output_sample_rate", kDefaultSampleRate)
                             .toInt();
    if (m_outputSampleRate <= 0) {
        m_outputSampleRate = kDefaultSampleRate;
    }
    m_remoteControlComputerPlayback =
        m_settings.value("settings/general/remote_control_computer_playback", false).toBool();
    m_autoPlayMvWhenAvailable =
        m_settings.value("settings/general/auto_play_mv_when_available", true).toBool();
    m_skipTrialTracks = m_settings.value("settings/general/skip_trial_tracks", false).toBool();
    m_syncRecentPlaylistToCloud =
        m_settings.value("settings/general/sync_recent_playlist_to_cloud", true).toBool();
    m_preferredPlaybackQuality = normalizeChoice(
        m_settings.value("settings/general/preferred_playback_quality", QStringLiteral("hq"))
            .toString(),
        QStringLiteral("hq"));
    m_singleTrackQueueMode = normalizeChoice(
        m_settings.value("settings/general/single_track_queue_mode", QStringLiteral("full_list"))
            .toString(),
        QStringLiteral("full_list"));
    m_showMessageCenterBadge =
        m_settings.value("settings/general/show_message_center_badge", true).toBool();
    m_showSmtc = m_settings.value("settings/general/show_smtc", true).toBool();
    m_preferMp3Download = m_settings.value("download/prefer_mp3_download", false).toBool();
    m_downloadFolderClassification = normalizeChoice(
        m_settings.value("download/folder_classification", QStringLiteral("flat")).toString(),
        QStringLiteral("flat"));
    m_downloadSongNamingFormat = normalizeChoice(
        m_settings.value("download/song_naming_format", QStringLiteral("artist_title")).toString(),
        QStringLiteral("artist_title"));
    m_cacheSpaceMode = normalizeChoice(
        m_settings.value("cache/space_mode", QStringLiteral("auto")).toString(),
        QStringLiteral("auto"));
    m_cacheSpaceLimitMb = m_settings.value("cache/space_limit_mb", 5120).toInt();
    if (m_cacheSpaceLimitMb < 2048) {
        m_cacheSpaceLimitMb = 5120;
    }
    m_mp3TagApeV2 = m_settings.value("download/mp3_tags/apev2", false).toBool();
    m_mp3TagId3v1 = m_settings.value("download/mp3_tags/id3v1", false).toBool();
    m_mp3TagId3v2 = m_settings.value("download/mp3_tags/id3v2", false).toBool();
    m_desktopLyricsLineMode = normalizeChoice(
        m_settings.value("settings/desktop_lyrics/line_mode", QStringLiteral("double")).toString(),
        QStringLiteral("double"));
    m_desktopLyricsAlignment = normalizeChoice(
        m_settings.value("settings/desktop_lyrics/alignment", QStringLiteral("split")).toString(),
        QStringLiteral("split"));
    m_desktopLyricsColorPreset = normalizeChoice(
        m_settings.value("settings/desktop_lyrics/color_preset", QStringLiteral("custom"))
            .toString(),
        QStringLiteral("custom"));
    m_desktopLyricsPlayedColor = m_settings
                                     .value("settings/desktop_lyrics/played_color",
                                            QColor(QStringLiteral("#ded93e")))
                                     .value<QColor>();
    m_desktopLyricsUnplayedColor = m_settings
                                       .value("settings/desktop_lyrics/unplayed_color",
                                              QColor(QStringLiteral("#4ca6e8")))
                                       .value<QColor>();
    m_desktopLyricsStrokeColor = m_settings
                                     .value("settings/desktop_lyrics/stroke_color",
                                            QColor(QStringLiteral("#000000")))
                                     .value<QColor>();
    if (!m_desktopLyricsPlayedColor.isValid()) {
        m_desktopLyricsPlayedColor = QColor(QStringLiteral("#ded93e"));
    }
    if (!m_desktopLyricsUnplayedColor.isValid()) {
        m_desktopLyricsUnplayedColor = QColor(QStringLiteral("#4ca6e8"));
    }
    if (!m_desktopLyricsStrokeColor.isValid()) {
        m_desktopLyricsStrokeColor = QColor(QStringLiteral("#000000"));
    }
    m_desktopLyricsCoverTaskbar =
        m_settings.value("settings/desktop_lyrics/cover_taskbar", false).toBool();
    m_desktopLyricsBold = m_settings.value("settings/desktop_lyrics/bold", false).toBool();
    m_desktopLyricsOpacity = qBound(
        0, m_settings.value("settings/desktop_lyrics/opacity", 100).toInt(), 100);
    m_globalShortcutsEnabled =
        m_settings.value("settings/shortcuts/global_enabled", true).toBool();
    m_shortcutVolumeUp = normalizeShortcut(
        m_settings.value("settings/shortcuts/volume_up", QStringLiteral("CTRL+ALT+UP"))
            .toString());
    m_shortcutVolumeDown = normalizeShortcut(
        m_settings.value("settings/shortcuts/volume_down", QStringLiteral("CTRL+ALT+DOWN"))
            .toString());
    m_shortcutLikeSong = normalizeShortcut(
        m_settings.value("settings/shortcuts/like_song", QStringLiteral("CTRL+ALT+V")).toString());
    m_shortcutMusicRecognition = normalizeShortcut(
        m_settings.value("settings/shortcuts/music_recognition", QStringLiteral("SHIFT+ALT+S"))
            .toString());
    m_shortcutToggleMainWindow = normalizeShortcut(
        m_settings.value("settings/shortcuts/toggle_main_window", QStringLiteral("CTRL+ALT+Q"))
            .toString());
    m_shortcutRewind = normalizeShortcut(
        m_settings.value("settings/shortcuts/rewind", QStringLiteral("CTRL+LEFT")).toString());
    m_shortcutFastForward = normalizeShortcut(
        m_settings.value("settings/shortcuts/fast_forward", QStringLiteral("CTRL+RIGHT"))
            .toString());
    m_effectPluginType = normalizeChoice(
        m_settings.value("settings/effects/plugin_type", QStringLiteral("vst3")).toString(),
        QStringLiteral("vst3"));
    m_pluginDirectory = QDir::cleanPath(QDir::fromNativeSeparators(
        m_settings.value("settings/effects/plugin_directory", defaultPluginDirectory())
            .toString()
            .trimmed()));
    if (m_pluginDirectory.isEmpty()) {
        m_pluginDirectory = defaultPluginDirectory();
    }
    m_proxyType = normalizeChoice(
        m_settings.value("settings/network/proxy_type", QStringLiteral("none")).toString(),
        QStringLiteral("none"));
    m_proxyUsername = m_settings.value("settings/network/proxy_username").toString().trimmed();
    m_proxyPassword = m_settings.value("settings/network/proxy_password").toString();
    m_audioOutputDevice = normalizeChoice(
        m_settings
            .value("settings/audio_device/output_device", QStringLiteral("DS: 主声音驱动程序"))
            .toString(),
        QStringLiteral("DS: 主声音驱动程序"));
    m_audioOutputFormat = normalizeChoice(
        m_settings.value("settings/audio_device/output_format", QStringLiteral("原始比特"))
            .toString(),
        QStringLiteral("原始比特"));
    m_audioFrequencyConversion = normalizeChoice(
        m_settings.value("settings/audio_device/frequency_conversion", QStringLiteral("原始频率"))
            .toString(),
        QStringLiteral("原始频率"));
    m_audioOutputChannels = normalizeChoice(
        m_settings.value("settings/audio_device/output_channels", QStringLiteral("原始声道数"))
            .toString(),
        QStringLiteral("原始声道数"));
    m_audioLocalFileMemoryMode =
        m_settings.value("settings/audio_device/local_file_memory_mode", false).toBool();
    m_audioGaplessPlayback =
        m_settings.value("settings/audio_device/gapless_playback", true).toBool();
    m_audioDsdMode = normalizeChoice(
        m_settings.value("settings/audio_device/dsd_mode", QStringLiteral("pcm_only")).toString(),
        QStringLiteral("pcm_only"));
    m_audioCacheSizeMs =
        qBound(200, m_settings.value("settings/audio_device/cache_size_ms", 1000).toInt(), 5000);
    QStringList storedSearchHistory = m_settings.value("search/history/keywords").toStringList();
    for (const QString& keyword : storedSearchHistory) {
        const QString normalized = normalizeSearchKeyword(keyword);
        if (normalized.isEmpty() || m_searchHistoryKeywords.contains(normalized)) {
            continue;
        }
        m_searchHistoryKeywords.append(normalized);
    }
    if (m_searchHistoryKeywords.size() > kMaxSearchHistoryItems) {
        m_searchHistoryKeywords = m_searchHistoryKeywords.mid(0, kMaxSearchHistoryItems);
    }
    m_settings.setValue("search/history/keywords", m_searchHistoryKeywords);
    if (m_serverHost.isEmpty()) {
        m_serverHost = QStringLiteral("192.168.1.208");
        m_settings.setValue("server/host", m_serverHost);
    }
    if (m_serverPort <= 0 || m_serverPort > 65535) {
        m_serverPort = 8080;
        m_settings.setValue("server/port", m_serverPort);
    }
    if (m_playerPageStyle < 0 || m_playerPageStyle > kMaxPlayerPageStyle) {
        m_playerPageStyle = kDefaultPlayerPageStyle;
        m_settings.setValue("player/page_style", m_playerPageStyle);
    }

    if (m_cachedAccount.isEmpty() || m_cachedPassword.isEmpty()) {
        m_autoLoginEnabled = false;
        m_manualLogoutMarked = false;
        m_settings.setValue("account/cache/auto_login", false);
        m_settings.setValue("account/cache/manual_logout", false);
    } else if (!m_autoLoginEnabled && !m_manualLogoutMarked &&
               !m_settings.contains("account/cache/manual_logout")) {
        // 兼容迁移：历史版本可能因会话过期误关自动登录。
        m_autoLoginEnabled = true;
        m_settings.setValue("account/cache/auto_login", true);
    }

    m_hasServerWelcomeWindowPos = m_settings.value("ui/server_welcome/pos_valid", false).toBool();
    m_serverWelcomeWindowPos = m_settings.value("ui/server_welcome/pos", QPoint(0, 0)).toPoint();
}

void SettingsManager::setDownloadPath(const QString& path) {
    if (m_downloadPath != path) {
        m_downloadPath = path;
        m_settings.setValue("download/path", path);
        emit downloadPathChanged();
    }
}

void SettingsManager::setDownloadLyrics(bool enable) {
    if (m_downloadLyrics != enable) {
        m_downloadLyrics = enable;
        m_settings.setValue("download/lyrics", enable);
        emit downloadLyricsChanged();
    }
}

void SettingsManager::setDownloadCover(bool enable) {
    if (m_downloadCover != enable) {
        m_downloadCover = enable;
        m_settings.setValue("download/cover", enable);
        emit downloadCoverChanged();
    }
}

void SettingsManager::setAudioCachePath(const QString& path) {
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

void SettingsManager::setLogPath(const QString& path) {
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

void SettingsManager::setServerHost(const QString& host) {
    const QString normalized = host.trimmed();
    if (normalized.isEmpty() || m_serverHost == normalized) {
        return;
    }

    m_serverHost = normalized;
    m_settings.setValue("server/host", m_serverHost);
    emit serverEndpointChanged();
}

void SettingsManager::setServerPort(int port) {
    if (port <= 0 || port > 65535 || m_serverPort == port) {
        return;
    }

    m_serverPort = port;
    m_settings.setValue("server/port", m_serverPort);
    emit serverEndpointChanged();
}

void SettingsManager::setPlayerPageStyle(int styleId) {
    static constexpr int kMinPlayerPageStyle = 0;
    static constexpr int kMaxPlayerPageStyle = 4;
    const int normalized = qBound(kMinPlayerPageStyle, styleId, kMaxPlayerPageStyle);
    if (m_playerPageStyle == normalized) {
        return;
    }

    m_playerPageStyle = normalized;
    m_settings.setValue("player/page_style", m_playerPageStyle);
    emit playerPageStyleChanged();
}

void SettingsManager::setLaunchAtStartup(bool enabled) {
    if (m_launchAtStartup == enabled) {
        return;
    }
    m_launchAtStartup = enabled;
    m_settings.setValue("settings/general/launch_at_startup", m_launchAtStartup);
    emit settingsCenterChanged();
}

void SettingsManager::setAutoPlayOnStartup(bool enabled) {
    if (m_autoPlayOnStartup == enabled) {
        return;
    }
    m_autoPlayOnStartup = enabled;
    m_settings.setValue("settings/general/auto_play_on_startup", m_autoPlayOnStartup);
    emit settingsCenterChanged();
}

void SettingsManager::setAutoOpenLyrics(bool enabled) {
    if (m_autoOpenLyrics == enabled) {
        return;
    }
    m_autoOpenLyrics = enabled;
    m_settings.setValue("settings/general/auto_open_lyrics", m_autoOpenLyrics);
    emit settingsCenterChanged();
}

void SettingsManager::setDisableDpiScale(bool enabled) {
    if (m_disableDpiScale == enabled) {
        return;
    }
    m_disableDpiScale = enabled;
    m_settings.setValue("settings/general/disable_dpi_scale", m_disableDpiScale);
    emit settingsCenterChanged();
}

void SettingsManager::setDisableHardwareAcceleration(bool enabled) {
    if (m_disableHardwareAcceleration == enabled) {
        return;
    }
    m_disableHardwareAcceleration = enabled;
    m_settings.setValue("settings/general/disable_hardware_acceleration",
                        m_disableHardwareAcceleration);
    emit settingsCenterChanged();
}

void SettingsManager::setPreventSystemSleep(bool enabled) {
    if (m_preventSystemSleep == enabled) {
        return;
    }
    m_preventSystemSleep = enabled;
    m_settings.setValue("settings/general/prevent_system_sleep", m_preventSystemSleep);
    emit settingsCenterChanged();
}

void SettingsManager::setListAutoSwitch(bool enabled) {
    if (m_listAutoSwitch == enabled) {
        return;
    }
    m_listAutoSwitch = enabled;
    m_settings.setValue("settings/general/list_auto_switch", m_listAutoSwitch);
    emit settingsCenterChanged();
}

void SettingsManager::setEnableFadeInOut(bool enabled) {
    if (m_enableFadeInOut == enabled) {
        return;
    }
    m_enableFadeInOut = enabled;
    m_settings.setValue("settings/general/enable_fade_in_out", m_enableFadeInOut);
    emit settingsCenterChanged();
}

void SettingsManager::setEnableMultimediaKeys(bool enabled) {
    if (m_enableMultimediaKeys == enabled) {
        return;
    }
    m_enableMultimediaKeys = enabled;
    m_settings.setValue("settings/general/enable_multimedia_keys", m_enableMultimediaKeys);
    emit settingsCenterChanged();
}

void SettingsManager::setAutoBestVolume(bool enabled) {
    if (m_autoBestVolume == enabled) {
        return;
    }
    m_autoBestVolume = enabled;
    m_settings.setValue("settings/general/auto_best_volume", m_autoBestVolume);
    emit settingsCenterChanged();
}

void SettingsManager::setPlaybackAccelerationService(bool enabled) {
    if (m_playbackAccelerationService == enabled) {
        return;
    }
    m_playbackAccelerationService = enabled;
    m_settings.setValue("settings/general/playback_acceleration_service",
                        m_playbackAccelerationService);
    emit settingsCenterChanged();
}

void SettingsManager::setPreferLocalDownloadedQuality(bool enabled) {
    if (m_preferLocalDownloadedQuality == enabled) {
        return;
    }
    m_preferLocalDownloadedQuality = enabled;
    m_settings.setValue("settings/general/prefer_local_downloaded_quality",
                        m_preferLocalDownloadedQuality);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsShowOnStartup(bool enabled) {
    if (m_desktopLyricsShowOnStartup == enabled) {
        return;
    }
    m_desktopLyricsShowOnStartup = enabled;
    m_settings.setValue("settings/desktop_lyrics/show_on_startup", m_desktopLyricsShowOnStartup);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsAlwaysOnTop(bool enabled) {
    if (m_desktopLyricsAlwaysOnTop == enabled) {
        return;
    }
    m_desktopLyricsAlwaysOnTop = enabled;
    m_settings.setValue("settings/desktop_lyrics/always_on_top", m_desktopLyricsAlwaysOnTop);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricColor(const QColor& color) {
    if (!color.isValid() || m_desktopLyricColor == color) {
        return;
    }
    m_desktopLyricColor = color;
    m_settings.setValue("DeskLrc/color", m_desktopLyricColor);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricFontSize(int size) {
    const int normalized = qBound(12, size, 64);
    if (m_desktopLyricFontSize == normalized) {
        return;
    }
    m_desktopLyricFontSize = normalized;
    m_settings.setValue("DeskLrc/fontSize", m_desktopLyricFontSize);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricFontFamily(const QString& family) {
    const QString normalized = normalizeFontFamily(family);
    if (normalized.isEmpty() || m_desktopLyricFontFamily == normalized) {
        return;
    }
    m_desktopLyricFontFamily = normalized;
    m_settings.setValue("DeskLrc/fontFamily", m_desktopLyricFontFamily);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutPlayPause(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutPlayPause == normalized) {
        return;
    }
    m_shortcutPlayPause = normalized;
    m_settings.setValue("settings/shortcuts/play_pause", m_shortcutPlayPause);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutPreviousTrack(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutPreviousTrack == normalized) {
        return;
    }
    m_shortcutPreviousTrack = normalized;
    m_settings.setValue("settings/shortcuts/previous_track", m_shortcutPreviousTrack);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutNextTrack(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutNextTrack == normalized) {
        return;
    }
    m_shortcutNextTrack = normalized;
    m_settings.setValue("settings/shortcuts/next_track", m_shortcutNextTrack);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutToggleDesktopLyrics(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutToggleDesktopLyrics == normalized) {
        return;
    }
    m_shortcutToggleDesktopLyrics = normalized;
    m_settings.setValue("settings/shortcuts/toggle_desktop_lyrics",
                        m_shortcutToggleDesktopLyrics);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutOpenSearch(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutOpenSearch == normalized) {
        return;
    }
    m_shortcutOpenSearch = normalized;
    m_settings.setValue("settings/shortcuts/open_search", m_shortcutOpenSearch);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioEffectsEnabled(bool enabled) {
    if (m_audioEffectsEnabled == enabled) {
        return;
    }
    m_audioEffectsEnabled = enabled;
    m_settings.setValue("settings/effects/audio_effects_enabled", m_audioEffectsEnabled);
    emit settingsCenterChanged();
}

void SettingsManager::setReplayGainEnabled(bool enabled) {
    if (m_replayGainEnabled == enabled) {
        return;
    }
    m_replayGainEnabled = enabled;
    m_settings.setValue("settings/effects/replay_gain_enabled", m_replayGainEnabled);
    emit settingsCenterChanged();
}

void SettingsManager::setVolumeNormalizationEnabled(bool enabled) {
    if (m_volumeNormalizationEnabled == enabled) {
        return;
    }
    m_volumeNormalizationEnabled = enabled;
    m_settings.setValue("settings/effects/volume_normalization_enabled",
                        m_volumeNormalizationEnabled);
    emit settingsCenterChanged();
}

void SettingsManager::setEqualizerEnabled(bool enabled) {
    if (m_equalizerEnabled == enabled) {
        return;
    }
    m_equalizerEnabled = enabled;
    m_settings.setValue("settings/effects/equalizer_enabled", m_equalizerEnabled);
    emit settingsCenterChanged();
}

void SettingsManager::setEffectPreset(const QString& preset) {
    const QString normalized = preset.trimmed();
    if (normalized.isEmpty() || m_effectPreset == normalized) {
        return;
    }
    m_effectPreset = normalized;
    m_settings.setValue("settings/effects/effect_preset", m_effectPreset);
    emit settingsCenterChanged();
}

void SettingsManager::setProxyEnabled(bool enabled) {
    if (m_proxyEnabled == enabled) {
        return;
    }
    m_proxyEnabled = enabled;
    m_settings.setValue("settings/network/proxy_enabled", m_proxyEnabled);
    emit settingsCenterChanged();
}

void SettingsManager::setProxyHost(const QString& host) {
    const QString normalized = host.trimmed();
    if (m_proxyHost == normalized) {
        return;
    }
    m_proxyHost = normalized;
    m_settings.setValue("settings/network/proxy_host", m_proxyHost);
    emit settingsCenterChanged();
}

void SettingsManager::setProxyPort(int port) {
    if (port < 0 || port > 65535 || m_proxyPort == port) {
        return;
    }
    m_proxyPort = port;
    m_settings.setValue("settings/network/proxy_port", m_proxyPort);
    emit settingsCenterChanged();
}

void SettingsManager::setFollowSystemAudioDevice(bool enabled) {
    if (m_followSystemAudioDevice == enabled) {
        return;
    }
    m_followSystemAudioDevice = enabled;
    m_settings.setValue("settings/audio_device/follow_system_device",
                        m_followSystemAudioDevice);
    emit settingsCenterChanged();
}

void SettingsManager::setExclusiveAudioDeviceMode(bool enabled) {
    if (m_exclusiveAudioDeviceMode == enabled) {
        return;
    }
    m_exclusiveAudioDeviceMode = enabled;
    m_settings.setValue("settings/audio_device/exclusive_mode", m_exclusiveAudioDeviceMode);
    emit settingsCenterChanged();
}

void SettingsManager::setLowLatencyAudioMode(bool enabled) {
    if (m_lowLatencyAudioMode == enabled) {
        return;
    }
    m_lowLatencyAudioMode = enabled;
    m_settings.setValue("settings/audio_device/low_latency_mode", m_lowLatencyAudioMode);
    emit settingsCenterChanged();
}

void SettingsManager::setOutputSampleRate(int sampleRate) {
    if (sampleRate <= 0 || m_outputSampleRate == sampleRate) {
        return;
    }
    m_outputSampleRate = sampleRate;
    m_settings.setValue("settings/audio_device/output_sample_rate", m_outputSampleRate);
    emit settingsCenterChanged();
}

void SettingsManager::setRemoteControlComputerPlayback(bool enabled) {
    if (m_remoteControlComputerPlayback == enabled) {
        return;
    }
    m_remoteControlComputerPlayback = enabled;
    m_settings.setValue("settings/general/remote_control_computer_playback",
                        m_remoteControlComputerPlayback);
    emit settingsCenterChanged();
}

void SettingsManager::setAutoPlayMvWhenAvailable(bool enabled) {
    if (m_autoPlayMvWhenAvailable == enabled) {
        return;
    }
    m_autoPlayMvWhenAvailable = enabled;
    m_settings.setValue("settings/general/auto_play_mv_when_available",
                        m_autoPlayMvWhenAvailable);
    emit settingsCenterChanged();
}

void SettingsManager::setSkipTrialTracks(bool enabled) {
    if (m_skipTrialTracks == enabled) {
        return;
    }
    m_skipTrialTracks = enabled;
    m_settings.setValue("settings/general/skip_trial_tracks", m_skipTrialTracks);
    emit settingsCenterChanged();
}

void SettingsManager::setSyncRecentPlaylistToCloud(bool enabled) {
    if (m_syncRecentPlaylistToCloud == enabled) {
        return;
    }
    m_syncRecentPlaylistToCloud = enabled;
    m_settings.setValue("settings/general/sync_recent_playlist_to_cloud",
                        m_syncRecentPlaylistToCloud);
    emit settingsCenterChanged();
}

void SettingsManager::setPreferredPlaybackQuality(const QString& quality) {
    const QString normalized = normalizeChoice(quality, QStringLiteral("hq"));
    if (m_preferredPlaybackQuality == normalized) {
        return;
    }
    m_preferredPlaybackQuality = normalized;
    m_settings.setValue("settings/general/preferred_playback_quality", m_preferredPlaybackQuality);
    emit settingsCenterChanged();
}

void SettingsManager::setSingleTrackQueueMode(const QString& mode) {
    const QString normalized = normalizeChoice(mode, QStringLiteral("full_list"));
    if (m_singleTrackQueueMode == normalized) {
        return;
    }
    m_singleTrackQueueMode = normalized;
    m_settings.setValue("settings/general/single_track_queue_mode", m_singleTrackQueueMode);
    emit settingsCenterChanged();
}

void SettingsManager::setShowMessageCenterBadge(bool enabled) {
    if (m_showMessageCenterBadge == enabled) {
        return;
    }
    m_showMessageCenterBadge = enabled;
    m_settings.setValue("settings/general/show_message_center_badge", m_showMessageCenterBadge);
    emit settingsCenterChanged();
}

void SettingsManager::setShowSmtc(bool enabled) {
    if (m_showSmtc == enabled) {
        return;
    }
    m_showSmtc = enabled;
    m_settings.setValue("settings/general/show_smtc", m_showSmtc);
    emit settingsCenterChanged();
}

void SettingsManager::setPreferMp3Download(bool enabled) {
    if (m_preferMp3Download == enabled) {
        return;
    }
    m_preferMp3Download = enabled;
    m_settings.setValue("download/prefer_mp3_download", m_preferMp3Download);
    emit settingsCenterChanged();
}

void SettingsManager::setDownloadFolderClassification(const QString& mode) {
    const QString normalized = normalizeChoice(mode, QStringLiteral("flat"));
    if (m_downloadFolderClassification == normalized) {
        return;
    }
    m_downloadFolderClassification = normalized;
    m_settings.setValue("download/folder_classification", m_downloadFolderClassification);
    emit settingsCenterChanged();
}

void SettingsManager::setDownloadSongNamingFormat(const QString& format) {
    const QString normalized = normalizeChoice(format, QStringLiteral("artist_title"));
    if (m_downloadSongNamingFormat == normalized) {
        return;
    }
    m_downloadSongNamingFormat = normalized;
    m_settings.setValue("download/song_naming_format", m_downloadSongNamingFormat);
    emit settingsCenterChanged();
}

void SettingsManager::setCacheSpaceMode(const QString& mode) {
    const QString normalized = normalizeChoice(mode, QStringLiteral("auto"));
    if (m_cacheSpaceMode == normalized) {
        return;
    }
    m_cacheSpaceMode = normalized;
    m_settings.setValue("cache/space_mode", m_cacheSpaceMode);
    emit settingsCenterChanged();
}

void SettingsManager::setCacheSpaceLimitMb(int limitMb) {
    const int normalized = qMax(2048, limitMb);
    if (m_cacheSpaceLimitMb == normalized) {
        return;
    }
    m_cacheSpaceLimitMb = normalized;
    m_settings.setValue("cache/space_limit_mb", m_cacheSpaceLimitMb);
    emit settingsCenterChanged();
}

void SettingsManager::setMp3TagApeV2(bool enabled) {
    if (m_mp3TagApeV2 == enabled) {
        return;
    }
    m_mp3TagApeV2 = enabled;
    m_settings.setValue("download/mp3_tags/apev2", m_mp3TagApeV2);
    emit settingsCenterChanged();
}

void SettingsManager::setMp3TagId3v1(bool enabled) {
    if (m_mp3TagId3v1 == enabled) {
        return;
    }
    m_mp3TagId3v1 = enabled;
    m_settings.setValue("download/mp3_tags/id3v1", m_mp3TagId3v1);
    emit settingsCenterChanged();
}

void SettingsManager::setMp3TagId3v2(bool enabled) {
    if (m_mp3TagId3v2 == enabled) {
        return;
    }
    m_mp3TagId3v2 = enabled;
    m_settings.setValue("download/mp3_tags/id3v2", m_mp3TagId3v2);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsLineMode(const QString& mode) {
    const QString normalized = normalizeChoice(mode, QStringLiteral("double"));
    if (m_desktopLyricsLineMode == normalized) {
        return;
    }
    m_desktopLyricsLineMode = normalized;
    m_settings.setValue("settings/desktop_lyrics/line_mode", m_desktopLyricsLineMode);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsAlignment(const QString& alignment) {
    const QString normalized = normalizeChoice(alignment, QStringLiteral("split"));
    if (m_desktopLyricsAlignment == normalized) {
        return;
    }
    m_desktopLyricsAlignment = normalized;
    m_settings.setValue("settings/desktop_lyrics/alignment", m_desktopLyricsAlignment);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsColorPreset(const QString& preset) {
    const QString normalized = normalizeChoice(preset, QStringLiteral("custom"));
    if (m_desktopLyricsColorPreset == normalized) {
        return;
    }
    m_desktopLyricsColorPreset = normalized;
    m_settings.setValue("settings/desktop_lyrics/color_preset", m_desktopLyricsColorPreset);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsPlayedColor(const QColor& color) {
    if (!color.isValid() || m_desktopLyricsPlayedColor == color) {
        return;
    }
    m_desktopLyricsPlayedColor = color;
    m_settings.setValue("settings/desktop_lyrics/played_color", m_desktopLyricsPlayedColor);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsUnplayedColor(const QColor& color) {
    if (!color.isValid() || m_desktopLyricsUnplayedColor == color) {
        return;
    }
    m_desktopLyricsUnplayedColor = color;
    m_settings.setValue("settings/desktop_lyrics/unplayed_color", m_desktopLyricsUnplayedColor);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsStrokeColor(const QColor& color) {
    if (!color.isValid() || m_desktopLyricsStrokeColor == color) {
        return;
    }
    m_desktopLyricsStrokeColor = color;
    m_settings.setValue("settings/desktop_lyrics/stroke_color", m_desktopLyricsStrokeColor);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsCoverTaskbar(bool enabled) {
    if (m_desktopLyricsCoverTaskbar == enabled) {
        return;
    }
    m_desktopLyricsCoverTaskbar = enabled;
    m_settings.setValue("settings/desktop_lyrics/cover_taskbar", m_desktopLyricsCoverTaskbar);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsBold(bool enabled) {
    if (m_desktopLyricsBold == enabled) {
        return;
    }
    m_desktopLyricsBold = enabled;
    m_settings.setValue("settings/desktop_lyrics/bold", m_desktopLyricsBold);
    emit settingsCenterChanged();
}

void SettingsManager::setDesktopLyricsOpacity(int opacity) {
    const int normalized = qBound(0, opacity, 100);
    if (m_desktopLyricsOpacity == normalized) {
        return;
    }
    m_desktopLyricsOpacity = normalized;
    m_settings.setValue("settings/desktop_lyrics/opacity", m_desktopLyricsOpacity);
    emit settingsCenterChanged();
}

void SettingsManager::setGlobalShortcutsEnabled(bool enabled) {
    if (m_globalShortcutsEnabled == enabled) {
        return;
    }
    m_globalShortcutsEnabled = enabled;
    m_settings.setValue("settings/shortcuts/global_enabled", m_globalShortcutsEnabled);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutVolumeUp(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutVolumeUp == normalized) {
        return;
    }
    m_shortcutVolumeUp = normalized;
    m_settings.setValue("settings/shortcuts/volume_up", m_shortcutVolumeUp);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutVolumeDown(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutVolumeDown == normalized) {
        return;
    }
    m_shortcutVolumeDown = normalized;
    m_settings.setValue("settings/shortcuts/volume_down", m_shortcutVolumeDown);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutLikeSong(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutLikeSong == normalized) {
        return;
    }
    m_shortcutLikeSong = normalized;
    m_settings.setValue("settings/shortcuts/like_song", m_shortcutLikeSong);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutMusicRecognition(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutMusicRecognition == normalized) {
        return;
    }
    m_shortcutMusicRecognition = normalized;
    m_settings.setValue("settings/shortcuts/music_recognition", m_shortcutMusicRecognition);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutToggleMainWindow(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutToggleMainWindow == normalized) {
        return;
    }
    m_shortcutToggleMainWindow = normalized;
    m_settings.setValue("settings/shortcuts/toggle_main_window", m_shortcutToggleMainWindow);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutRewind(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutRewind == normalized) {
        return;
    }
    m_shortcutRewind = normalized;
    m_settings.setValue("settings/shortcuts/rewind", m_shortcutRewind);
    emit settingsCenterChanged();
}

void SettingsManager::setShortcutFastForward(const QString& shortcut) {
    const QString normalized = normalizeShortcut(shortcut);
    if (normalized.isEmpty() || m_shortcutFastForward == normalized) {
        return;
    }
    m_shortcutFastForward = normalized;
    m_settings.setValue("settings/shortcuts/fast_forward", m_shortcutFastForward);
    emit settingsCenterChanged();
}

void SettingsManager::setEffectPluginType(const QString& pluginType) {
    const QString normalized = normalizeChoice(pluginType, QStringLiteral("vst3"));
    if (m_effectPluginType == normalized) {
        return;
    }
    m_effectPluginType = normalized;
    m_settings.setValue("settings/effects/plugin_type", m_effectPluginType);
    emit settingsCenterChanged();
}

void SettingsManager::setPluginDirectory(const QString& path) {
    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));
    if (normalized.isEmpty() || m_pluginDirectory == normalized) {
        return;
    }
    m_pluginDirectory = normalized;
    m_settings.setValue("settings/effects/plugin_directory", m_pluginDirectory);
    emit settingsCenterChanged();
}

void SettingsManager::setProxyType(const QString& proxyType) {
    const QString normalized = normalizeChoice(proxyType, QStringLiteral("none"));
    if (m_proxyType == normalized) {
        return;
    }
    m_proxyType = normalized;
    m_settings.setValue("settings/network/proxy_type", m_proxyType);
    emit settingsCenterChanged();
}

void SettingsManager::setProxyUsername(const QString& username) {
    const QString normalized = username.trimmed();
    if (m_proxyUsername == normalized) {
        return;
    }
    m_proxyUsername = normalized;
    m_settings.setValue("settings/network/proxy_username", m_proxyUsername);
    emit settingsCenterChanged();
}

void SettingsManager::setProxyPassword(const QString& password) {
    if (m_proxyPassword == password) {
        return;
    }
    m_proxyPassword = password;
    m_settings.setValue("settings/network/proxy_password", m_proxyPassword);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioOutputDevice(const QString& device) {
    const QString normalized = normalizeChoice(device, QStringLiteral("DS: 主声音驱动程序"));
    if (m_audioOutputDevice == normalized) {
        return;
    }
    m_audioOutputDevice = normalized;
    m_settings.setValue("settings/audio_device/output_device", m_audioOutputDevice);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioOutputFormat(const QString& format) {
    const QString normalized = normalizeChoice(format, QStringLiteral("原始比特"));
    if (m_audioOutputFormat == normalized) {
        return;
    }
    m_audioOutputFormat = normalized;
    m_settings.setValue("settings/audio_device/output_format", m_audioOutputFormat);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioFrequencyConversion(const QString& mode) {
    const QString normalized = normalizeChoice(mode, QStringLiteral("原始频率"));
    if (m_audioFrequencyConversion == normalized) {
        return;
    }
    m_audioFrequencyConversion = normalized;
    m_settings.setValue("settings/audio_device/frequency_conversion",
                        m_audioFrequencyConversion);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioOutputChannels(const QString& channels) {
    const QString normalized = normalizeChoice(channels, QStringLiteral("原始声道数"));
    if (m_audioOutputChannels == normalized) {
        return;
    }
    m_audioOutputChannels = normalized;
    m_settings.setValue("settings/audio_device/output_channels", m_audioOutputChannels);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioLocalFileMemoryMode(bool enabled) {
    if (m_audioLocalFileMemoryMode == enabled) {
        return;
    }
    m_audioLocalFileMemoryMode = enabled;
    m_settings.setValue("settings/audio_device/local_file_memory_mode",
                        m_audioLocalFileMemoryMode);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioGaplessPlayback(bool enabled) {
    if (m_audioGaplessPlayback == enabled) {
        return;
    }
    m_audioGaplessPlayback = enabled;
    m_settings.setValue("settings/audio_device/gapless_playback", m_audioGaplessPlayback);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioDsdMode(const QString& mode) {
    const QString normalized = normalizeChoice(mode, QStringLiteral("pcm_only"));
    if (m_audioDsdMode == normalized) {
        return;
    }
    m_audioDsdMode = normalized;
    m_settings.setValue("settings/audio_device/dsd_mode", m_audioDsdMode);
    emit settingsCenterChanged();
}

void SettingsManager::setAudioCacheSizeMs(int cacheSizeMs) {
    const int normalized = qBound(200, cacheSizeMs, 5000);
    if (m_audioCacheSizeMs == normalized) {
        return;
    }
    m_audioCacheSizeMs = normalized;
    m_settings.setValue("settings/audio_device/cache_size_ms", m_audioCacheSizeMs);
    emit settingsCenterChanged();
}

void SettingsManager::resetDesktopLyricsSettings() {
    setDesktopLyricsShowOnStartup(false);
    setDesktopLyricsAlwaysOnTop(false);
    setDesktopLyricsLineMode(QStringLiteral("double"));
    setDesktopLyricsAlignment(QStringLiteral("split"));
    setDesktopLyricFontFamily(QStringLiteral("Microsoft YaHei"));
    setDesktopLyricFontSize(57);
    setDesktopLyricsColorPreset(QStringLiteral("custom"));
    setDesktopLyricsPlayedColor(QColor(QStringLiteral("#ded93e")));
    setDesktopLyricsUnplayedColor(QColor(QStringLiteral("#4ca6e8")));
    setDesktopLyricsStrokeColor(QColor(QStringLiteral("#000000")));
    setDesktopLyricsCoverTaskbar(false);
    setDesktopLyricsBold(false);
    setDesktopLyricsOpacity(100);
}

void SettingsManager::resetShortcutSettings() {
    setGlobalShortcutsEnabled(true);
    setShortcutPlayPause(QStringLiteral("CTRL+ALT+F5"));
    setShortcutPreviousTrack(QStringLiteral("CTRL+ALT+LEFT"));
    setShortcutNextTrack(QStringLiteral("CTRL+ALT+RIGHT"));
    setShortcutVolumeUp(QStringLiteral("CTRL+ALT+UP"));
    setShortcutVolumeDown(QStringLiteral("CTRL+ALT+DOWN"));
    setShortcutLikeSong(QStringLiteral("CTRL+ALT+V"));
    setShortcutMusicRecognition(QStringLiteral("SHIFT+ALT+S"));
    setShortcutToggleMainWindow(QStringLiteral("CTRL+ALT+Q"));
    setShortcutToggleDesktopLyrics(QStringLiteral("CTRL+ALT+W"));
    setShortcutRewind(QStringLiteral("CTRL+LEFT"));
    setShortcutFastForward(QStringLiteral("CTRL+RIGHT"));
}

void SettingsManager::resetAudioDeviceSettings() {
    setAudioOutputDevice(QStringLiteral("DS: 主声音驱动程序"));
    setAudioOutputFormat(QStringLiteral("原始比特"));
    setAudioFrequencyConversion(QStringLiteral("原始频率"));
    setAudioOutputChannels(QStringLiteral("原始声道数"));
    setAudioLocalFileMemoryMode(false);
    setAudioGaplessPlayback(true);
    setAudioDsdMode(QStringLiteral("pcm_only"));
    setAudioCacheSizeMs(1000);
    setFollowSystemAudioDevice(true);
    setExclusiveAudioDeviceMode(false);
    setLowLatencyAudioMode(false);
    setOutputSampleRate(44100);
}

QString SettingsManager::serverBaseUrl() const {
    return QStringLiteral("http://%1:%2/").arg(m_serverHost, QString::number(m_serverPort));
}

void SettingsManager::setServerEndpoint(const QString& host, int port) {
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

void SettingsManager::saveAccountCache(const QString& account, const QString& password,
                                       const QString& username, bool enableAutoLogin) {
    const QString trimmedAccount = account.trimmed();
    if (trimmedAccount.isEmpty()) {
        return;
    }

    const bool cacheChanged = m_cachedAccount != trimmedAccount || m_cachedPassword != password ||
                              m_cachedUsername != username;
    const bool autoChanged = m_autoLoginEnabled != enableAutoLogin;
    const bool manualLogoutChanged = m_manualLogoutMarked;

    m_cachedAccount = trimmedAccount;
    m_cachedPassword = password;
    m_cachedUsername = username.trimmed();
    m_autoLoginEnabled = enableAutoLogin;
    m_manualLogoutMarked = false;

    m_settings.setValue("account/cache/account", m_cachedAccount);
    m_settings.setValue("account/cache/password", m_cachedPassword);
    m_settings.setValue("account/cache/username", m_cachedUsername);
    m_settings.setValue("account/cache/auto_login", m_autoLoginEnabled);
    m_settings.setValue("account/cache/manual_logout", m_manualLogoutMarked);

    if (cacheChanged) {
        emit accountCacheChanged();
    }
    if (autoChanged) {
        emit autoLoginChanged();
    }
    if (manualLogoutChanged && !autoChanged) {
        emit autoLoginChanged();
    }
}

void SettingsManager::setAutoLoginEnabled(bool enabled) {
    if (enabled && (m_cachedAccount.isEmpty() || m_cachedPassword.isEmpty())) {
        enabled = false;
    }

    if (m_autoLoginEnabled == enabled) {
        return;
    }

    m_autoLoginEnabled = enabled;
    if (enabled) {
        m_manualLogoutMarked = false;
        m_settings.setValue("account/cache/manual_logout", false);
    }
    m_settings.setValue("account/cache/auto_login", m_autoLoginEnabled);
    emit autoLoginChanged();
}

void SettingsManager::setManualLogoutMarked(bool marked) {
    if (m_manualLogoutMarked == marked) {
        return;
    }

    m_manualLogoutMarked = marked;
    m_settings.setValue("account/cache/manual_logout", m_manualLogoutMarked);
}

void SettingsManager::saveProfileCache(const QString& username, const QString& avatarUrl,
                                       const QString& onlineSessionToken, const QString& createdAt,
                                       const QString& updatedAt) {
    const QString normalizedUsername = username.trimmed();
    const QString normalizedAvatarUrl = avatarUrl.trimmed();
    const QString normalizedToken = onlineSessionToken.trimmed();
    const QString normalizedCreatedAt = createdAt.trimmed();
    const QString normalizedUpdatedAt = updatedAt.trimmed();

    const bool changed = m_cachedUsername != normalizedUsername ||
                         m_cachedAvatarUrl != normalizedAvatarUrl ||
                         m_cachedOnlineSessionToken != normalizedToken ||
                         m_cachedProfileCreatedAt != normalizedCreatedAt ||
                         m_cachedProfileUpdatedAt != normalizedUpdatedAt;

    m_cachedUsername = normalizedUsername;
    m_cachedAvatarUrl = normalizedAvatarUrl;
    m_cachedOnlineSessionToken = normalizedToken;
    m_cachedProfileCreatedAt = normalizedCreatedAt;
    m_cachedProfileUpdatedAt = normalizedUpdatedAt;

    m_settings.setValue("account/cache/username", m_cachedUsername);
    m_settings.setValue("account/cache/avatar_url", m_cachedAvatarUrl);
    m_settings.setValue("account/cache/online_session_token", m_cachedOnlineSessionToken);
    m_settings.setValue("account/cache/profile_created_at", m_cachedProfileCreatedAt);
    m_settings.setValue("account/cache/profile_updated_at", m_cachedProfileUpdatedAt);

    if (changed) {
        emit accountCacheChanged();
    }
}

void SettingsManager::clearAccountCache() {
    const bool hadCache =
        !m_cachedAccount.isEmpty() || !m_cachedPassword.isEmpty() || !m_cachedUsername.isEmpty() ||
        !m_cachedAvatarUrl.isEmpty() || !m_cachedOnlineSessionToken.isEmpty() ||
        !m_cachedProfileCreatedAt.isEmpty() || !m_cachedProfileUpdatedAt.isEmpty();
    const bool autoChanged = m_autoLoginEnabled;

    m_cachedAccount.clear();
    m_cachedPassword.clear();
    m_cachedUsername.clear();
    m_cachedAvatarUrl.clear();
    m_cachedOnlineSessionToken.clear();
    m_cachedProfileCreatedAt.clear();
    m_cachedProfileUpdatedAt.clear();
    m_autoLoginEnabled = false;
    m_manualLogoutMarked = false;

    m_settings.remove("account/cache/account");
    m_settings.remove("account/cache/password");
    m_settings.remove("account/cache/username");
    m_settings.remove("account/cache/avatar_url");
    m_settings.remove("account/cache/online_session_token");
    m_settings.remove("account/cache/profile_created_at");
    m_settings.remove("account/cache/profile_updated_at");
    m_settings.setValue("account/cache/auto_login", false);
    m_settings.setValue("account/cache/manual_logout", false);

    if (hadCache) {
        emit accountCacheChanged();
    }
    if (autoChanged) {
        emit autoLoginChanged();
    }
}

bool SettingsManager::shouldAutoLogin() const {
    if (m_cachedAccount.isEmpty() || m_cachedPassword.isEmpty()) {
        return false;
    }

    return m_autoLoginEnabled || !m_manualLogoutMarked;
}

void SettingsManager::setServerWelcomeWindowPos(const QPoint& pos) {
    if (m_hasServerWelcomeWindowPos && m_serverWelcomeWindowPos == pos) {
        return;
    }

    m_hasServerWelcomeWindowPos = true;
    m_serverWelcomeWindowPos = pos;
    m_settings.setValue("ui/server_welcome/pos_valid", true);
    m_settings.setValue("ui/server_welcome/pos", m_serverWelcomeWindowPos);
}

QByteArray SettingsManager::pluginWindowGeometry(const QString& pluginId) const {
    const QString key = QStringLiteral("ui/plugins/%1/geometry").arg(normalizedPluginKey(pluginId));
    return m_settings.value(key).toByteArray();
}

void SettingsManager::setPluginWindowGeometry(const QString& pluginId, const QByteArray& geometry) {
    if (geometry.isEmpty()) {
        return;
    }

    const QString key = QStringLiteral("ui/plugins/%1/geometry").arg(normalizedPluginKey(pluginId));
    m_settings.setValue(key, geometry);
}

void SettingsManager::clearPluginWindowGeometry(const QString& pluginId) {
    const QString key = QStringLiteral("ui/plugins/%1/geometry").arg(normalizedPluginKey(pluginId));
    m_settings.remove(key);
}

QStringList SettingsManager::searchHistoryKeywords() const {
    return m_searchHistoryKeywords;
}

void SettingsManager::addSearchHistoryKeyword(const QString& keyword) {
    static constexpr int kMaxSearchHistoryItems = 10;
    const QString normalized = normalizeSearchKeyword(keyword);
    if (normalized.isEmpty()) {
        return;
    }

    QStringList updated = m_searchHistoryKeywords;
    updated.removeAll(normalized);
    updated.prepend(normalized);
    while (updated.size() > kMaxSearchHistoryItems) {
        updated.removeLast();
    }

    if (updated == m_searchHistoryKeywords) {
        return;
    }

    m_searchHistoryKeywords = updated;
    m_settings.setValue("search/history/keywords", m_searchHistoryKeywords);
    emit searchHistoryChanged();
}

void SettingsManager::removeSearchHistoryKeyword(const QString& keyword) {
    const QString normalized = normalizeSearchKeyword(keyword);
    if (normalized.isEmpty()) {
        return;
    }

    QStringList updated = m_searchHistoryKeywords;
    if (!updated.removeAll(normalized)) {
        return;
    }

    m_searchHistoryKeywords = updated;
    m_settings.setValue("search/history/keywords", m_searchHistoryKeywords);
    emit searchHistoryChanged();
}

void SettingsManager::clearSearchHistoryKeywords() {
    if (m_searchHistoryKeywords.isEmpty()) {
        return;
    }

    m_searchHistoryKeywords.clear();
    m_settings.setValue("search/history/keywords", m_searchHistoryKeywords);
    emit searchHistoryChanged();
}
