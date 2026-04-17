#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include "BaseViewModel.h"

#include <QColor>
#include <QDateTime>
#include <QString>

class QWidget;

/**
 * @brief 设置窗口视图模型。
 *
 * 负责设置页展示状态、设置持久化桥接、目录选择和在线状态读取，
 * 让 QML 视图只负责渲染和转发交互。
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
    Q_PROPERTY(QString agentMode READ agentMode NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentLocalModelPath READ agentLocalModelPath NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentLocalModelBaseUrl READ agentLocalModelBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentLocalModelName READ agentLocalModelName NOTIFY agentSettingsChanged)
    Q_PROPERTY(int agentLocalContextSize READ agentLocalContextSize NOTIFY agentSettingsChanged)
    Q_PROPERTY(int agentLocalThreadCount READ agentLocalThreadCount NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool agentRemoteFallbackEnabled READ agentRemoteFallbackEnabled NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentRemoteBaseUrl READ agentRemoteBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentRemoteModelName READ agentRemoteModelName NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString serverHost READ serverHost NOTIFY settingsCenterChanged)
    Q_PROPERTY(int serverPort READ serverPort NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool launchAtStartup READ launchAtStartup NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool autoPlayOnStartup READ autoPlayOnStartup NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool autoOpenLyrics READ autoOpenLyrics NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool disableDpiScale READ disableDpiScale NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool disableHardwareAcceleration READ disableHardwareAcceleration NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool preventSystemSleep READ preventSystemSleep NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool listAutoSwitch READ listAutoSwitch NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool enableFadeInOut READ enableFadeInOut NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool enableMultimediaKeys READ enableMultimediaKeys NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool autoBestVolume READ autoBestVolume NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool playbackAccelerationService READ playbackAccelerationService NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool preferLocalDownloadedQuality READ preferLocalDownloadedQuality NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsShowOnStartup READ desktopLyricsShowOnStartup NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsAlwaysOnTop READ desktopLyricsAlwaysOnTop NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricColor READ desktopLyricColor NOTIFY settingsCenterChanged)
    Q_PROPERTY(int desktopLyricFontSize READ desktopLyricFontSize NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricFontFamily READ desktopLyricFontFamily NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutPlayPause READ shortcutPlayPause NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutPreviousTrack READ shortcutPreviousTrack NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutNextTrack READ shortcutNextTrack NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutToggleDesktopLyrics READ shortcutToggleDesktopLyrics NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutOpenSearch READ shortcutOpenSearch NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool audioEffectsEnabled READ audioEffectsEnabled NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool replayGainEnabled READ replayGainEnabled NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool volumeNormalizationEnabled READ volumeNormalizationEnabled NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool equalizerEnabled READ equalizerEnabled NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString effectPreset READ effectPreset NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool proxyEnabled READ proxyEnabled NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString proxyHost READ proxyHost NOTIFY settingsCenterChanged)
    Q_PROPERTY(int proxyPort READ proxyPort NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool followSystemAudioDevice READ followSystemAudioDevice NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool exclusiveAudioDeviceMode READ exclusiveAudioDeviceMode NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool lowLatencyAudioMode READ lowLatencyAudioMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(int outputSampleRate READ outputSampleRate NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool remoteControlComputerPlayback READ remoteControlComputerPlayback NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool autoPlayMvWhenAvailable READ autoPlayMvWhenAvailable NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool skipTrialTracks READ skipTrialTracks NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool syncRecentPlaylistToCloud READ syncRecentPlaylistToCloud NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString preferredPlaybackQuality READ preferredPlaybackQuality NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString singleTrackQueueMode READ singleTrackQueueMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool showMessageCenterBadge READ showMessageCenterBadge NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool showSmtc READ showSmtc NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool preferMp3Download READ preferMp3Download NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString downloadFolderClassification READ downloadFolderClassification NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString downloadSongNamingFormat READ downloadSongNamingFormat NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString cacheSpaceMode READ cacheSpaceMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(int cacheSpaceLimitMb READ cacheSpaceLimitMb NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool mp3TagApeV2 READ mp3TagApeV2 NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool mp3TagId3v1 READ mp3TagId3v1 NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool mp3TagId3v2 READ mp3TagId3v2 NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricsLineMode READ desktopLyricsLineMode NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricsAlignment READ desktopLyricsAlignment NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString desktopLyricsColorPreset READ desktopLyricsColorPreset NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricsPlayedColor READ desktopLyricsPlayedColor NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricsUnplayedColor READ desktopLyricsUnplayedColor NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QColor desktopLyricsStrokeColor READ desktopLyricsStrokeColor NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsCoverTaskbar READ desktopLyricsCoverTaskbar NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool desktopLyricsBold READ desktopLyricsBold NOTIFY settingsCenterChanged)
    Q_PROPERTY(int desktopLyricsOpacity READ desktopLyricsOpacity NOTIFY settingsCenterChanged)
    Q_PROPERTY(bool globalShortcutsEnabled READ globalShortcutsEnabled NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutVolumeUp READ shortcutVolumeUp NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutVolumeDown READ shortcutVolumeDown NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutLikeSong READ shortcutLikeSong NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutMusicRecognition READ shortcutMusicRecognition NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutToggleMainWindow READ shortcutToggleMainWindow NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString shortcutRewind READ shortcutRewind NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString shortcutFastForward READ shortcutFastForward NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString effectPluginType READ effectPluginType NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString pluginDirectory READ pluginDirectory NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString proxyType READ proxyType NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString proxyUsername READ proxyUsername NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString proxyPassword READ proxyPassword NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString audioOutputDevice READ audioOutputDevice NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString audioOutputFormat READ audioOutputFormat NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString audioFrequencyConversion READ audioFrequencyConversion NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(QString audioOutputChannels READ audioOutputChannels NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool audioLocalFileMemoryMode READ audioLocalFileMemoryMode NOTIFY
                   settingsCenterChanged)
    Q_PROPERTY(bool audioGaplessPlayback READ audioGaplessPlayback NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString audioDsdMode READ audioDsdMode NOTIFY settingsCenterChanged)
    Q_PROPERTY(int audioCacheSizeMs READ audioCacheSizeMs NOTIFY settingsCenterChanged)
    Q_PROPERTY(QString presenceAccount READ presenceAccount NOTIFY presenceChanged)
    Q_PROPERTY(QString presenceSessionToken READ presenceSessionToken NOTIFY presenceChanged)
    Q_PROPERTY(bool presenceOnline READ presenceOnline NOTIFY presenceChanged)
    Q_PROPERTY(int presenceHeartbeatIntervalSec READ presenceHeartbeatIntervalSec NOTIFY
                   presenceChanged)
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
    QString agentMode() const;
    QString agentLocalModelPath() const;
    QString agentLocalModelBaseUrl() const;
    QString agentLocalModelName() const;
    int agentLocalContextSize() const;
    int agentLocalThreadCount() const;
    bool agentRemoteFallbackEnabled() const;
    QString agentRemoteBaseUrl() const;
    QString agentRemoteModelName() const;
    QString serverHost() const;
    int serverPort() const;
    bool launchAtStartup() const;
    bool autoPlayOnStartup() const;
    bool autoOpenLyrics() const;
    bool disableDpiScale() const;
    bool disableHardwareAcceleration() const;
    bool preventSystemSleep() const;
    bool listAutoSwitch() const;
    bool enableFadeInOut() const;
    bool enableMultimediaKeys() const;
    bool autoBestVolume() const;
    bool playbackAccelerationService() const;
    bool preferLocalDownloadedQuality() const;
    bool desktopLyricsShowOnStartup() const;
    bool desktopLyricsAlwaysOnTop() const;
    QColor desktopLyricColor() const;
    int desktopLyricFontSize() const;
    QString desktopLyricFontFamily() const;
    QString shortcutPlayPause() const;
    QString shortcutPreviousTrack() const;
    QString shortcutNextTrack() const;
    QString shortcutToggleDesktopLyrics() const;
    QString shortcutOpenSearch() const;
    bool audioEffectsEnabled() const;
    bool replayGainEnabled() const;
    bool volumeNormalizationEnabled() const;
    bool equalizerEnabled() const;
    QString effectPreset() const;
    bool proxyEnabled() const;
    QString proxyHost() const;
    int proxyPort() const;
    bool followSystemAudioDevice() const;
    bool exclusiveAudioDeviceMode() const;
    bool lowLatencyAudioMode() const;
    int outputSampleRate() const;
    bool remoteControlComputerPlayback() const;
    bool autoPlayMvWhenAvailable() const;
    bool skipTrialTracks() const;
    bool syncRecentPlaylistToCloud() const;
    QString preferredPlaybackQuality() const;
    QString singleTrackQueueMode() const;
    bool showMessageCenterBadge() const;
    bool showSmtc() const;
    bool preferMp3Download() const;
    QString downloadFolderClassification() const;
    QString downloadSongNamingFormat() const;
    QString cacheSpaceMode() const;
    int cacheSpaceLimitMb() const;
    bool mp3TagApeV2() const;
    bool mp3TagId3v1() const;
    bool mp3TagId3v2() const;
    QString desktopLyricsLineMode() const;
    QString desktopLyricsAlignment() const;
    QString desktopLyricsColorPreset() const;
    QColor desktopLyricsPlayedColor() const;
    QColor desktopLyricsUnplayedColor() const;
    QColor desktopLyricsStrokeColor() const;
    bool desktopLyricsCoverTaskbar() const;
    bool desktopLyricsBold() const;
    int desktopLyricsOpacity() const;
    bool globalShortcutsEnabled() const;
    QString shortcutVolumeUp() const;
    QString shortcutVolumeDown() const;
    QString shortcutLikeSong() const;
    QString shortcutMusicRecognition() const;
    QString shortcutToggleMainWindow() const;
    QString shortcutRewind() const;
    QString shortcutFastForward() const;
    QString effectPluginType() const;
    QString pluginDirectory() const;
    QString proxyType() const;
    QString proxyUsername() const;
    QString proxyPassword() const;
    QString audioOutputDevice() const;
    QString audioOutputFormat() const;
    QString audioFrequencyConversion() const;
    QString audioOutputChannels() const;
    bool audioLocalFileMemoryMode() const;
    bool audioGaplessPlayback() const;
    QString audioDsdMode() const;
    int audioCacheSizeMs() const;

    QString presenceAccount() const { return m_presenceAccount; }
    QString presenceSessionToken() const { return m_presenceSessionToken; }
    bool presenceOnline() const { return m_presenceOnline; }
    int presenceHeartbeatIntervalSec() const { return m_presenceHeartbeatIntervalSec; }
    int presenceOnlineTtlSec() const { return m_presenceOnlineTtlSec; }
    int presenceTtlRemainingSec() const { return m_presenceTtlRemainingSec; }
    QString presenceStatusMessage() const { return m_presenceStatusMessage; }
    QString presenceLastSeenText() const { return m_presenceLastSeenText; }

public slots:
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
    void setAgentMode(const QString& mode);
    void setAgentLocalModelPath(const QString& modelPath);
    void setAgentLocalModelBaseUrl(const QString& baseUrl);
    void setAgentLocalModelName(const QString& modelName);
    void setAgentLocalContextSize(int contextSize);
    void setAgentLocalThreadCount(int threadCount);
    void setAgentRemoteFallbackEnabled(bool enabled);
    void setAgentRemoteBaseUrl(const QString& baseUrl);
    void setAgentRemoteModelName(const QString& modelName);
    void setServerHost(const QString& host);
    void setServerPort(int port);
    void setLaunchAtStartup(bool enabled);
    void setAutoPlayOnStartup(bool enabled);
    void setAutoOpenLyrics(bool enabled);
    void setDisableDpiScale(bool enabled);
    void setDisableHardwareAcceleration(bool enabled);
    void setPreventSystemSleep(bool enabled);
    void setListAutoSwitch(bool enabled);
    void setEnableFadeInOut(bool enabled);
    void setEnableMultimediaKeys(bool enabled);
    void setAutoBestVolume(bool enabled);
    void setPlaybackAccelerationService(bool enabled);
    void setPreferLocalDownloadedQuality(bool enabled);
    void setDesktopLyricsShowOnStartup(bool enabled);
    void setDesktopLyricsAlwaysOnTop(bool enabled);
    void setDesktopLyricColor(const QColor& color);
    void setDesktopLyricFontSize(int size);
    void setDesktopLyricFontFamily(const QString& family);
    void setShortcutPlayPause(const QString& shortcut);
    void setShortcutPreviousTrack(const QString& shortcut);
    void setShortcutNextTrack(const QString& shortcut);
    void setShortcutToggleDesktopLyrics(const QString& shortcut);
    void setShortcutOpenSearch(const QString& shortcut);
    void setAudioEffectsEnabled(bool enabled);
    void setReplayGainEnabled(bool enabled);
    void setVolumeNormalizationEnabled(bool enabled);
    void setEqualizerEnabled(bool enabled);
    void setEffectPreset(const QString& preset);
    void setProxyEnabled(bool enabled);
    void setProxyHost(const QString& host);
    void setProxyPort(int port);
    void setFollowSystemAudioDevice(bool enabled);
    void setExclusiveAudioDeviceMode(bool enabled);
    void setLowLatencyAudioMode(bool enabled);
    void setOutputSampleRate(int sampleRate);
    void setRemoteControlComputerPlayback(bool enabled);
    void setAutoPlayMvWhenAvailable(bool enabled);
    void setSkipTrialTracks(bool enabled);
    void setSyncRecentPlaylistToCloud(bool enabled);
    void setPreferredPlaybackQuality(const QString& quality);
    void setSingleTrackQueueMode(const QString& mode);
    void setShowMessageCenterBadge(bool enabled);
    void setShowSmtc(bool enabled);
    void setPreferMp3Download(bool enabled);
    void setDownloadFolderClassification(const QString& mode);
    void setDownloadSongNamingFormat(const QString& format);
    void setCacheSpaceMode(const QString& mode);
    void setCacheSpaceLimitMb(int limitMb);
    void setMp3TagApeV2(bool enabled);
    void setMp3TagId3v1(bool enabled);
    void setMp3TagId3v2(bool enabled);
    void setDesktopLyricsLineMode(const QString& mode);
    void setDesktopLyricsAlignment(const QString& alignment);
    void setDesktopLyricsColorPreset(const QString& preset);
    void setDesktopLyricsPlayedColor(const QColor& color);
    void setDesktopLyricsUnplayedColor(const QColor& color);
    void setDesktopLyricsStrokeColor(const QColor& color);
    void setDesktopLyricsCoverTaskbar(bool enabled);
    void setDesktopLyricsBold(bool enabled);
    void setDesktopLyricsOpacity(int opacity);
    void setGlobalShortcutsEnabled(bool enabled);
    void setShortcutVolumeUp(const QString& shortcut);
    void setShortcutVolumeDown(const QString& shortcut);
    void setShortcutLikeSong(const QString& shortcut);
    void setShortcutMusicRecognition(const QString& shortcut);
    void setShortcutToggleMainWindow(const QString& shortcut);
    void setShortcutRewind(const QString& shortcut);
    void setShortcutFastForward(const QString& shortcut);
    void setEffectPluginType(const QString& pluginType);
    void setPluginDirectory(const QString& path);
    void setProxyType(const QString& proxyType);
    void setProxyUsername(const QString& username);
    void setProxyPassword(const QString& password);
    void setAudioOutputDevice(const QString& device);
    void setAudioOutputFormat(const QString& format);
    void setAudioFrequencyConversion(const QString& mode);
    void setAudioOutputChannels(const QString& channels);
    void setAudioLocalFileMemoryMode(bool enabled);
    void setAudioGaplessPlayback(bool enabled);
    void setAudioDsdMode(const QString& mode);
    void setAudioCacheSizeMs(int cacheSizeMs);
    void refreshPresence();
    void syncFromSettings();
    Q_INVOKABLE void resetDesktopLyricsSettings();
    Q_INVOKABLE void resetShortcutSettings();
    Q_INVOKABLE void resetAudioDeviceSettings();

signals:
    void downloadPathChanged();
    void audioCachePathChanged();
    void logPathChanged();
    void downloadLyricsChanged();
    void downloadCoverChanged();
    void playerPageStyleChanged();
    void agentSettingsChanged();
    void settingsCenterChanged();
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
