#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QObject>
#include <QPoint>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

/**
 * @brief Global settings manager (singleton)
 */
class SettingsManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(
        QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(bool downloadLyrics READ downloadLyrics WRITE setDownloadLyrics NOTIFY
                   downloadLyricsChanged)
    Q_PROPERTY(
        bool downloadCover READ downloadCover WRITE setDownloadCover NOTIFY downloadCoverChanged)
    Q_PROPERTY(QString audioCachePath READ audioCachePath WRITE setAudioCachePath NOTIFY
                   audioCachePathChanged)
    Q_PROPERTY(QString logPath READ logPath WRITE setLogPath NOTIFY logPathChanged)
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverEndpointChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverEndpointChanged)
    Q_PROPERTY(int playerPageStyle READ playerPageStyle WRITE setPlayerPageStyle NOTIFY
                   playerPageStyleChanged)
    Q_PROPERTY(QString agentMode READ agentMode WRITE setAgentMode NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentLocalModelPath READ agentLocalModelPath WRITE
                   setAgentLocalModelPath NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentLocalModelBaseUrl READ agentLocalModelBaseUrl WRITE
                   setAgentLocalModelBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentLocalModelName READ agentLocalModelName WRITE setAgentLocalModelName
                   NOTIFY agentSettingsChanged)
    Q_PROPERTY(int agentLocalContextSize READ agentLocalContextSize WRITE setAgentLocalContextSize
                   NOTIFY agentSettingsChanged)
    Q_PROPERTY(int agentLocalThreadCount READ agentLocalThreadCount WRITE setAgentLocalThreadCount
                   NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool agentRemoteFallbackEnabled READ agentRemoteFallbackEnabled WRITE
                   setAgentRemoteFallbackEnabled NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentRemoteBaseUrl READ agentRemoteBaseUrl WRITE setAgentRemoteBaseUrl
                   NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentRemoteModelName READ agentRemoteModelName WRITE
                   setAgentRemoteModelName NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool launchAtStartup READ launchAtStartup WRITE setLaunchAtStartup NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool autoPlayOnStartup READ autoPlayOnStartup WRITE setAutoPlayOnStartup NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool autoOpenLyrics READ autoOpenLyrics WRITE setAutoOpenLyrics NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool disableDpiScale READ disableDpiScale WRITE setDisableDpiScale NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool disableHardwareAcceleration READ disableHardwareAcceleration WRITE
                   setDisableHardwareAcceleration NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool preventSystemSleep READ preventSystemSleep WRITE setPreventSystemSleep NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool listAutoSwitch READ listAutoSwitch WRITE setListAutoSwitch NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool enableFadeInOut READ enableFadeInOut WRITE setEnableFadeInOut NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool enableMultimediaKeys READ enableMultimediaKeys WRITE
                   setEnableMultimediaKeys NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool autoBestVolume READ autoBestVolume WRITE setAutoBestVolume NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool playbackAccelerationService READ playbackAccelerationService WRITE
                   setPlaybackAccelerationService NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool preferLocalDownloadedQuality READ preferLocalDownloadedQuality WRITE
                   setPreferLocalDownloadedQuality NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsShowOnStartup READ desktopLyricsShowOnStartup WRITE
                   setDesktopLyricsShowOnStartup NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsAlwaysOnTop READ desktopLyricsAlwaysOnTop WRITE
                   setDesktopLyricsAlwaysOnTop NOTIFY settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricColor READ desktopLyricColor WRITE setDesktopLyricColor NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(int desktopLyricFontSize READ desktopLyricFontSize WRITE
                   setDesktopLyricFontSize NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricFontFamily READ desktopLyricFontFamily WRITE
                   setDesktopLyricFontFamily NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutPlayPause READ shortcutPlayPause WRITE setShortcutPlayPause NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutPreviousTrack READ shortcutPreviousTrack WRITE
                   setShortcutPreviousTrack NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutNextTrack READ shortcutNextTrack WRITE setShortcutNextTrack NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutToggleDesktopLyrics READ shortcutToggleDesktopLyrics WRITE
                   setShortcutToggleDesktopLyrics NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutOpenSearch READ shortcutOpenSearch WRITE setShortcutOpenSearch
                   NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool audioEffectsEnabled READ audioEffectsEnabled WRITE setAudioEffectsEnabled
                   NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool replayGainEnabled READ replayGainEnabled WRITE setReplayGainEnabled NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool volumeNormalizationEnabled READ volumeNormalizationEnabled WRITE
                   setVolumeNormalizationEnabled NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool equalizerEnabled READ equalizerEnabled WRITE setEqualizerEnabled NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString effectPreset READ effectPreset WRITE setEffectPreset NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool proxyEnabled READ proxyEnabled WRITE setProxyEnabled NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString proxyHost READ proxyHost WRITE setProxyHost NOTIFY settingsCenterChanged)
    Q_PROPERTY(int proxyPort READ proxyPort WRITE setProxyPort NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool followSystemAudioDevice READ followSystemAudioDevice WRITE
                   setFollowSystemAudioDevice NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool exclusiveAudioDeviceMode READ exclusiveAudioDeviceMode WRITE
                   setExclusiveAudioDeviceMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool lowLatencyAudioMode READ lowLatencyAudioMode WRITE setLowLatencyAudioMode
                   NOTIFY settingsCenterChanged)
    Q_PROPERTY(int outputSampleRate READ outputSampleRate WRITE setOutputSampleRate NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool remoteControlComputerPlayback READ remoteControlComputerPlayback WRITE
                   setRemoteControlComputerPlayback NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool autoPlayMvWhenAvailable READ autoPlayMvWhenAvailable WRITE
                   setAutoPlayMvWhenAvailable NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool skipTrialTracks READ skipTrialTracks WRITE setSkipTrialTracks NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool syncRecentPlaylistToCloud READ syncRecentPlaylistToCloud WRITE
                   setSyncRecentPlaylistToCloud NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString preferredPlaybackQuality READ preferredPlaybackQuality WRITE
                   setPreferredPlaybackQuality NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString singleTrackQueueMode READ singleTrackQueueMode WRITE
                   setSingleTrackQueueMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool showMessageCenterBadge READ showMessageCenterBadge WRITE
                   setShowMessageCenterBadge NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool showSmtc READ showSmtc WRITE setShowSmtc NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool preferMp3Download READ preferMp3Download WRITE setPreferMp3Download NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString downloadFolderClassification READ downloadFolderClassification WRITE
                   setDownloadFolderClassification NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString downloadSongNamingFormat READ downloadSongNamingFormat WRITE
                   setDownloadSongNamingFormat NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString cacheSpaceMode READ cacheSpaceMode WRITE setCacheSpaceMode NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(int cacheSpaceLimitMb READ cacheSpaceLimitMb WRITE setCacheSpaceLimitMb NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool mp3TagApeV2 READ mp3TagApeV2 WRITE setMp3TagApeV2 NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool mp3TagId3v1 READ mp3TagId3v1 WRITE setMp3TagId3v1 NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool mp3TagId3v2 READ mp3TagId3v2 WRITE setMp3TagId3v2 NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricsLineMode READ desktopLyricsLineMode WRITE
                   setDesktopLyricsLineMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricsAlignment READ desktopLyricsAlignment WRITE
                   setDesktopLyricsAlignment NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricsColorPreset READ desktopLyricsColorPreset WRITE
                   setDesktopLyricsColorPreset NOTIFY settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricsPlayedColor READ desktopLyricsPlayedColor WRITE
                   setDesktopLyricsPlayedColor NOTIFY settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricsUnplayedColor READ desktopLyricsUnplayedColor WRITE
                   setDesktopLyricsUnplayedColor NOTIFY settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricsStrokeColor READ desktopLyricsStrokeColor WRITE
                   setDesktopLyricsStrokeColor NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsCoverTaskbar READ desktopLyricsCoverTaskbar WRITE
                   setDesktopLyricsCoverTaskbar NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsBold READ desktopLyricsBold WRITE setDesktopLyricsBold NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(int desktopLyricsOpacity READ desktopLyricsOpacity WRITE setDesktopLyricsOpacity
                   NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool globalShortcutsEnabled READ globalShortcutsEnabled WRITE
                   setGlobalShortcutsEnabled NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutVolumeUp READ shortcutVolumeUp WRITE setShortcutVolumeUp NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutVolumeDown READ shortcutVolumeDown WRITE setShortcutVolumeDown NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutLikeSong READ shortcutLikeSong WRITE setShortcutLikeSong NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutMusicRecognition READ shortcutMusicRecognition WRITE
                   setShortcutMusicRecognition NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutToggleMainWindow READ shortcutToggleMainWindow WRITE
                   setShortcutToggleMainWindow NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutRewind READ shortcutRewind WRITE setShortcutRewind NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutFastForward READ shortcutFastForward WRITE
                   setShortcutFastForward NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString effectPluginType READ effectPluginType WRITE setEffectPluginType NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString pluginDirectory READ pluginDirectory WRITE setPluginDirectory NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString proxyType READ proxyType WRITE setProxyType NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString proxyUsername READ proxyUsername WRITE setProxyUsername NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString proxyPassword READ proxyPassword WRITE setProxyPassword NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString audioOutputDevice READ audioOutputDevice WRITE setAudioOutputDevice NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString audioOutputFormat READ audioOutputFormat WRITE setAudioOutputFormat NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString audioFrequencyConversion READ audioFrequencyConversion WRITE
                   setAudioFrequencyConversion NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString audioOutputChannels READ audioOutputChannels WRITE
                   setAudioOutputChannels NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool audioLocalFileMemoryMode READ audioLocalFileMemoryMode WRITE
                   setAudioLocalFileMemoryMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool audioGaplessPlayback READ audioGaplessPlayback WRITE setAudioGaplessPlayback
                   NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString audioDsdMode READ audioDsdMode WRITE setAudioDsdMode NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(int audioCacheSizeMs READ audioCacheSizeMs WRITE setAudioCacheSizeMs NOTIFY
                   settingsCenterChanged)

  public:
    static SettingsManager& instance() {
        static SettingsManager instance;
        return instance;
    }

    QString downloadPath() const {
        return m_downloadPath;
    }
    void setDownloadPath(const QString& path);

    bool downloadLyrics() const {
        return m_downloadLyrics;
    }
    void setDownloadLyrics(bool enable);

    bool downloadCover() const {
        return m_downloadCover;
    }
    void setDownloadCover(bool enable);

    QString audioCachePath() const {
        return m_audioCachePath;
    }
    void setAudioCachePath(const QString& path);

    QString logPath() const {
        return m_logPath;
    }
    void setLogPath(const QString& path);

    QString serverHost() const {
        return m_serverHost;
    }
    void setServerHost(const QString& host);

    int serverPort() const {
        return m_serverPort;
    }
    void setServerPort(int port);
    int playerPageStyle() const {
        return m_playerPageStyle;
    }
    void setPlayerPageStyle(int styleId);
    QString agentMode() const {
        return m_agentMode;
    }
    void setAgentMode(const QString& mode);
    QString agentLocalModelPath() const {
        return m_agentLocalModelPath;
    }
    void setAgentLocalModelPath(const QString& modelPath);
    QString agentLocalModelBaseUrl() const {
        return m_agentLocalModelBaseUrl;
    }
    void setAgentLocalModelBaseUrl(const QString& baseUrl);
    QString agentLocalModelName() const {
        return m_agentLocalModelName;
    }
    void setAgentLocalModelName(const QString& modelName);
    int agentLocalContextSize() const {
        return m_agentLocalContextSize;
    }
    void setAgentLocalContextSize(int contextSize);
    int agentLocalThreadCount() const {
        return m_agentLocalThreadCount;
    }
    void setAgentLocalThreadCount(int threadCount);
    bool agentRemoteFallbackEnabled() const {
        return m_agentRemoteFallbackEnabled;
    }
    void setAgentRemoteFallbackEnabled(bool enabled);
    QString agentRemoteBaseUrl() const {
        return m_agentRemoteBaseUrl;
    }
    void setAgentRemoteBaseUrl(const QString& baseUrl);
    QString agentRemoteModelName() const {
        return m_agentRemoteModelName;
    }
    void setAgentRemoteModelName(const QString& modelName);
    bool launchAtStartup() const {
        return m_launchAtStartup;
    }
    void setLaunchAtStartup(bool enabled);
    bool autoPlayOnStartup() const {
        return m_autoPlayOnStartup;
    }
    void setAutoPlayOnStartup(bool enabled);
    bool autoOpenLyrics() const {
        return m_autoOpenLyrics;
    }
    void setAutoOpenLyrics(bool enabled);
    bool disableDpiScale() const {
        return m_disableDpiScale;
    }
    void setDisableDpiScale(bool enabled);
    bool disableHardwareAcceleration() const {
        return m_disableHardwareAcceleration;
    }
    void setDisableHardwareAcceleration(bool enabled);
    bool preventSystemSleep() const {
        return m_preventSystemSleep;
    }
    void setPreventSystemSleep(bool enabled);
    bool listAutoSwitch() const {
        return m_listAutoSwitch;
    }
    void setListAutoSwitch(bool enabled);
    bool enableFadeInOut() const {
        return m_enableFadeInOut;
    }
    void setEnableFadeInOut(bool enabled);
    bool enableMultimediaKeys() const {
        return m_enableMultimediaKeys;
    }
    void setEnableMultimediaKeys(bool enabled);
    bool autoBestVolume() const {
        return m_autoBestVolume;
    }
    void setAutoBestVolume(bool enabled);
    bool playbackAccelerationService() const {
        return m_playbackAccelerationService;
    }
    void setPlaybackAccelerationService(bool enabled);
    bool preferLocalDownloadedQuality() const {
        return m_preferLocalDownloadedQuality;
    }
    void setPreferLocalDownloadedQuality(bool enabled);
    bool desktopLyricsShowOnStartup() const {
        return m_desktopLyricsShowOnStartup;
    }
    void setDesktopLyricsShowOnStartup(bool enabled);
    bool desktopLyricsAlwaysOnTop() const {
        return m_desktopLyricsAlwaysOnTop;
    }
    void setDesktopLyricsAlwaysOnTop(bool enabled);
    QColor desktopLyricColor() const {
        return m_desktopLyricColor;
    }
    void setDesktopLyricColor(const QColor& color);
    int desktopLyricFontSize() const {
        return m_desktopLyricFontSize;
    }
    void setDesktopLyricFontSize(int size);
    QString desktopLyricFontFamily() const {
        return m_desktopLyricFontFamily;
    }
    void setDesktopLyricFontFamily(const QString& family);
    QString shortcutPlayPause() const {
        return m_shortcutPlayPause;
    }
    void setShortcutPlayPause(const QString& shortcut);
    QString shortcutPreviousTrack() const {
        return m_shortcutPreviousTrack;
    }
    void setShortcutPreviousTrack(const QString& shortcut);
    QString shortcutNextTrack() const {
        return m_shortcutNextTrack;
    }
    void setShortcutNextTrack(const QString& shortcut);
    QString shortcutToggleDesktopLyrics() const {
        return m_shortcutToggleDesktopLyrics;
    }
    void setShortcutToggleDesktopLyrics(const QString& shortcut);
    QString shortcutOpenSearch() const {
        return m_shortcutOpenSearch;
    }
    void setShortcutOpenSearch(const QString& shortcut);
    bool audioEffectsEnabled() const {
        return m_audioEffectsEnabled;
    }
    void setAudioEffectsEnabled(bool enabled);
    bool replayGainEnabled() const {
        return m_replayGainEnabled;
    }
    void setReplayGainEnabled(bool enabled);
    bool volumeNormalizationEnabled() const {
        return m_volumeNormalizationEnabled;
    }
    void setVolumeNormalizationEnabled(bool enabled);
    bool equalizerEnabled() const {
        return m_equalizerEnabled;
    }
    void setEqualizerEnabled(bool enabled);
    QString effectPreset() const {
        return m_effectPreset;
    }
    void setEffectPreset(const QString& preset);
    bool proxyEnabled() const {
        return m_proxyEnabled;
    }
    void setProxyEnabled(bool enabled);
    QString proxyHost() const {
        return m_proxyHost;
    }
    void setProxyHost(const QString& host);
    int proxyPort() const {
        return m_proxyPort;
    }
    void setProxyPort(int port);
    bool followSystemAudioDevice() const {
        return m_followSystemAudioDevice;
    }
    void setFollowSystemAudioDevice(bool enabled);
    bool exclusiveAudioDeviceMode() const {
        return m_exclusiveAudioDeviceMode;
    }
    void setExclusiveAudioDeviceMode(bool enabled);
    bool lowLatencyAudioMode() const {
        return m_lowLatencyAudioMode;
    }
    void setLowLatencyAudioMode(bool enabled);
    int outputSampleRate() const {
        return m_outputSampleRate;
    }
    void setOutputSampleRate(int sampleRate);
    bool remoteControlComputerPlayback() const {
        return m_remoteControlComputerPlayback;
    }
    void setRemoteControlComputerPlayback(bool enabled);
    bool autoPlayMvWhenAvailable() const {
        return m_autoPlayMvWhenAvailable;
    }
    void setAutoPlayMvWhenAvailable(bool enabled);
    bool skipTrialTracks() const {
        return m_skipTrialTracks;
    }
    void setSkipTrialTracks(bool enabled);
    bool syncRecentPlaylistToCloud() const {
        return m_syncRecentPlaylistToCloud;
    }
    void setSyncRecentPlaylistToCloud(bool enabled);
    QString preferredPlaybackQuality() const {
        return m_preferredPlaybackQuality;
    }
    void setPreferredPlaybackQuality(const QString& quality);
    QString singleTrackQueueMode() const {
        return m_singleTrackQueueMode;
    }
    void setSingleTrackQueueMode(const QString& mode);
    bool showMessageCenterBadge() const {
        return m_showMessageCenterBadge;
    }
    void setShowMessageCenterBadge(bool enabled);
    bool showSmtc() const {
        return m_showSmtc;
    }
    void setShowSmtc(bool enabled);
    bool preferMp3Download() const {
        return m_preferMp3Download;
    }
    void setPreferMp3Download(bool enabled);
    QString downloadFolderClassification() const {
        return m_downloadFolderClassification;
    }
    void setDownloadFolderClassification(const QString& mode);
    QString downloadSongNamingFormat() const {
        return m_downloadSongNamingFormat;
    }
    void setDownloadSongNamingFormat(const QString& format);
    QString cacheSpaceMode() const {
        return m_cacheSpaceMode;
    }
    void setCacheSpaceMode(const QString& mode);
    int cacheSpaceLimitMb() const {
        return m_cacheSpaceLimitMb;
    }
    void setCacheSpaceLimitMb(int limitMb);
    bool mp3TagApeV2() const {
        return m_mp3TagApeV2;
    }
    void setMp3TagApeV2(bool enabled);
    bool mp3TagId3v1() const {
        return m_mp3TagId3v1;
    }
    void setMp3TagId3v1(bool enabled);
    bool mp3TagId3v2() const {
        return m_mp3TagId3v2;
    }
    void setMp3TagId3v2(bool enabled);
    QString desktopLyricsLineMode() const {
        return m_desktopLyricsLineMode;
    }
    void setDesktopLyricsLineMode(const QString& mode);
    QString desktopLyricsAlignment() const {
        return m_desktopLyricsAlignment;
    }
    void setDesktopLyricsAlignment(const QString& alignment);
    QString desktopLyricsColorPreset() const {
        return m_desktopLyricsColorPreset;
    }
    void setDesktopLyricsColorPreset(const QString& preset);
    QColor desktopLyricsPlayedColor() const {
        return m_desktopLyricsPlayedColor;
    }
    void setDesktopLyricsPlayedColor(const QColor& color);
    QColor desktopLyricsUnplayedColor() const {
        return m_desktopLyricsUnplayedColor;
    }
    void setDesktopLyricsUnplayedColor(const QColor& color);
    QColor desktopLyricsStrokeColor() const {
        return m_desktopLyricsStrokeColor;
    }
    void setDesktopLyricsStrokeColor(const QColor& color);
    bool desktopLyricsCoverTaskbar() const {
        return m_desktopLyricsCoverTaskbar;
    }
    void setDesktopLyricsCoverTaskbar(bool enabled);
    bool desktopLyricsBold() const {
        return m_desktopLyricsBold;
    }
    void setDesktopLyricsBold(bool enabled);
    int desktopLyricsOpacity() const {
        return m_desktopLyricsOpacity;
    }
    void setDesktopLyricsOpacity(int opacity);
    bool globalShortcutsEnabled() const {
        return m_globalShortcutsEnabled;
    }
    void setGlobalShortcutsEnabled(bool enabled);
    QString shortcutVolumeUp() const {
        return m_shortcutVolumeUp;
    }
    void setShortcutVolumeUp(const QString& shortcut);
    QString shortcutVolumeDown() const {
        return m_shortcutVolumeDown;
    }
    void setShortcutVolumeDown(const QString& shortcut);
    QString shortcutLikeSong() const {
        return m_shortcutLikeSong;
    }
    void setShortcutLikeSong(const QString& shortcut);
    QString shortcutMusicRecognition() const {
        return m_shortcutMusicRecognition;
    }
    void setShortcutMusicRecognition(const QString& shortcut);
    QString shortcutToggleMainWindow() const {
        return m_shortcutToggleMainWindow;
    }
    void setShortcutToggleMainWindow(const QString& shortcut);
    QString shortcutRewind() const {
        return m_shortcutRewind;
    }
    void setShortcutRewind(const QString& shortcut);
    QString shortcutFastForward() const {
        return m_shortcutFastForward;
    }
    void setShortcutFastForward(const QString& shortcut);
    QString effectPluginType() const {
        return m_effectPluginType;
    }
    void setEffectPluginType(const QString& pluginType);
    QString pluginDirectory() const {
        return m_pluginDirectory;
    }
    void setPluginDirectory(const QString& path);
    QString proxyType() const {
        return m_proxyType;
    }
    void setProxyType(const QString& proxyType);
    QString proxyUsername() const {
        return m_proxyUsername;
    }
    void setProxyUsername(const QString& username);
    QString proxyPassword() const {
        return m_proxyPassword;
    }
    void setProxyPassword(const QString& password);
    QString audioOutputDevice() const {
        return m_audioOutputDevice;
    }
    void setAudioOutputDevice(const QString& device);
    QString audioOutputFormat() const {
        return m_audioOutputFormat;
    }
    void setAudioOutputFormat(const QString& format);
    QString audioFrequencyConversion() const {
        return m_audioFrequencyConversion;
    }
    void setAudioFrequencyConversion(const QString& mode);
    QString audioOutputChannels() const {
        return m_audioOutputChannels;
    }
    void setAudioOutputChannels(const QString& channels);
    bool audioLocalFileMemoryMode() const {
        return m_audioLocalFileMemoryMode;
    }
    void setAudioLocalFileMemoryMode(bool enabled);
    bool audioGaplessPlayback() const {
        return m_audioGaplessPlayback;
    }
    void setAudioGaplessPlayback(bool enabled);
    QString audioDsdMode() const {
        return m_audioDsdMode;
    }
    void setAudioDsdMode(const QString& mode);
    int audioCacheSizeMs() const {
        return m_audioCacheSizeMs;
    }
    void setAudioCacheSizeMs(int cacheSizeMs);
    void resetDesktopLyricsSettings();
    void resetShortcutSettings();
    void resetAudioDeviceSettings();

    QString serverBaseUrl() const;
    void setServerEndpoint(const QString& host, int port);

    QString cachedAccount() const {
        return m_cachedAccount;
    }
    QString cachedPassword() const {
        return m_cachedPassword;
    }
    QString cachedUsername() const {
        return m_cachedUsername;
    }
    QString cachedAvatarUrl() const {
        return m_cachedAvatarUrl;
    }
    QString cachedOnlineSessionToken() const {
        return m_cachedOnlineSessionToken;
    }
    QString cachedProfileCreatedAt() const {
        return m_cachedProfileCreatedAt;
    }
    QString cachedProfileUpdatedAt() const {
        return m_cachedProfileUpdatedAt;
    }
    bool autoLoginEnabled() const {
        return m_autoLoginEnabled;
    }
    bool manualLogoutMarked() const {
        return m_manualLogoutMarked;
    }
    bool shouldAutoLogin() const;

    void saveAccountCache(const QString& account, const QString& password, const QString& username,
                          bool enableAutoLogin);
    void saveProfileCache(const QString& username, const QString& avatarUrl,
                          const QString& onlineSessionToken, const QString& createdAt = QString(),
                          const QString& updatedAt = QString());
    void setAutoLoginEnabled(bool enabled);
    void setManualLogoutMarked(bool marked);
    void clearAccountCache();

    bool hasServerWelcomeWindowPos() const {
        return m_hasServerWelcomeWindowPos;
    }
    QPoint serverWelcomeWindowPos() const {
        return m_serverWelcomeWindowPos;
    }
    void setServerWelcomeWindowPos(const QPoint& pos);

    QByteArray pluginWindowGeometry(const QString& pluginId) const;
    void setPluginWindowGeometry(const QString& pluginId, const QByteArray& geometry);
    void clearPluginWindowGeometry(const QString& pluginId);
    QStringList searchHistoryKeywords() const;
    void addSearchHistoryKeyword(const QString& keyword);
    void removeSearchHistoryKeyword(const QString& keyword);
    void clearSearchHistoryKeywords();

  signals:
    void downloadPathChanged();
    void downloadLyricsChanged();
    void downloadCoverChanged();
    void audioCachePathChanged();
    void logPathChanged();
    void serverEndpointChanged();
    void playerPageStyleChanged();
    void agentSettingsChanged();
    void settingsCenterChanged();
    void accountCacheChanged();
    void autoLoginChanged();
    void searchHistoryChanged();

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
    int m_playerPageStyle = 0;
    QString m_agentMode;
    QString m_agentLocalModelPath;
    QString m_agentLocalModelBaseUrl;
    QString m_agentLocalModelName;
    int m_agentLocalContextSize = 16384;
    int m_agentLocalThreadCount = 4;
    bool m_agentRemoteFallbackEnabled = false;
    QString m_agentRemoteBaseUrl;
    QString m_agentRemoteModelName;
    bool m_launchAtStartup = false;
    bool m_autoPlayOnStartup = false;
    bool m_autoOpenLyrics = false;
    bool m_disableDpiScale = false;
    bool m_disableHardwareAcceleration = false;
    bool m_preventSystemSleep = false;
    bool m_listAutoSwitch = true;
    bool m_enableFadeInOut = true;
    bool m_enableMultimediaKeys = true;
    bool m_autoBestVolume = false;
    bool m_playbackAccelerationService = true;
    bool m_preferLocalDownloadedQuality = true;
    bool m_desktopLyricsShowOnStartup = false;
    bool m_desktopLyricsAlwaysOnTop = false;
    QColor m_desktopLyricColor;
    int m_desktopLyricFontSize = 18;
    QString m_desktopLyricFontFamily;
    QString m_shortcutPlayPause;
    QString m_shortcutPreviousTrack;
    QString m_shortcutNextTrack;
    QString m_shortcutToggleDesktopLyrics;
    QString m_shortcutOpenSearch;
    bool m_audioEffectsEnabled = false;
    bool m_replayGainEnabled = false;
    bool m_volumeNormalizationEnabled = false;
    bool m_equalizerEnabled = false;
    QString m_effectPreset;
    bool m_proxyEnabled = false;
    QString m_proxyHost;
    int m_proxyPort = 7890;
    bool m_followSystemAudioDevice = true;
    bool m_exclusiveAudioDeviceMode = false;
    bool m_lowLatencyAudioMode = false;
    int m_outputSampleRate = 44100;
    bool m_remoteControlComputerPlayback = false;
    bool m_autoPlayMvWhenAvailable = true;
    bool m_skipTrialTracks = false;
    bool m_syncRecentPlaylistToCloud = true;
    QString m_preferredPlaybackQuality;
    QString m_singleTrackQueueMode;
    bool m_showMessageCenterBadge = true;
    bool m_showSmtc = true;
    bool m_preferMp3Download = false;
    QString m_downloadFolderClassification;
    QString m_downloadSongNamingFormat;
    QString m_cacheSpaceMode;
    int m_cacheSpaceLimitMb = 5120;
    bool m_mp3TagApeV2 = false;
    bool m_mp3TagId3v1 = false;
    bool m_mp3TagId3v2 = false;
    QString m_desktopLyricsLineMode;
    QString m_desktopLyricsAlignment;
    QString m_desktopLyricsColorPreset;
    QColor m_desktopLyricsPlayedColor;
    QColor m_desktopLyricsUnplayedColor;
    QColor m_desktopLyricsStrokeColor;
    bool m_desktopLyricsCoverTaskbar = false;
    bool m_desktopLyricsBold = false;
    int m_desktopLyricsOpacity = 100;
    bool m_globalShortcutsEnabled = true;
    QString m_shortcutVolumeUp;
    QString m_shortcutVolumeDown;
    QString m_shortcutLikeSong;
    QString m_shortcutMusicRecognition;
    QString m_shortcutToggleMainWindow;
    QString m_shortcutRewind;
    QString m_shortcutFastForward;
    QString m_effectPluginType;
    QString m_pluginDirectory;
    QString m_proxyType;
    QString m_proxyUsername;
    QString m_proxyPassword;
    QString m_audioOutputDevice;
    QString m_audioOutputFormat;
    QString m_audioFrequencyConversion;
    QString m_audioOutputChannels;
    bool m_audioLocalFileMemoryMode = false;
    bool m_audioGaplessPlayback = true;
    QString m_audioDsdMode;
    int m_audioCacheSizeMs = 1000;
    QString m_cachedAccount;
    QString m_cachedPassword;
    QString m_cachedUsername;
    QString m_cachedAvatarUrl;
    QString m_cachedOnlineSessionToken;
    QString m_cachedProfileCreatedAt;
    QString m_cachedProfileUpdatedAt;
    bool m_autoLoginEnabled = false;
    bool m_manualLogoutMarked = false;
    QStringList m_searchHistoryKeywords;
    bool m_hasServerWelcomeWindowPos = false;
    QPoint m_serverWelcomeWindowPos;
};

#endif // SETTINGS_MANAGER_H
