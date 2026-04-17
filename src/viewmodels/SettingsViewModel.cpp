#include "SettingsViewModel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QWidget>

#include "AudioCacheManager.h"
#include "AudioService.h"
#include "network_service.h"
#include "online_presence_manager.h"
#include "settings_manager.h"

namespace {

QString formatBytes(qint64 bytes)
{
    static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(qMax<qint64>(0, bytes));
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        ++unitIndex;
    }

    if (unitIndex == 0) {
        return QString::number(static_cast<qint64>(value)) + QStringLiteral(" ") + kUnits[unitIndex];
    }
    return QString::number(value, 'f', 2) + QStringLiteral(" ") + kUnits[unitIndex];
}

SettingsManager& settings()
{
    return SettingsManager::instance();
}

} // namespace

SettingsViewModel::SettingsViewModel(QObject* parent)
    : BaseViewModel(parent)
{
    connect(&OnlinePresenceManager::instance(),
            &OnlinePresenceManager::presenceSnapshotChanged,
            this,
            &SettingsViewModel::onPresenceSnapshotChanged);

    connect(&settings(), &SettingsManager::downloadPathChanged, this, &SettingsViewModel::downloadPathChanged);
    connect(&settings(), &SettingsManager::audioCachePathChanged, this, &SettingsViewModel::audioCachePathChanged);
    connect(&settings(), &SettingsManager::logPathChanged, this, &SettingsViewModel::logPathChanged);
    connect(&settings(), &SettingsManager::downloadLyricsChanged, this, &SettingsViewModel::downloadLyricsChanged);
    connect(&settings(), &SettingsManager::downloadCoverChanged, this, &SettingsViewModel::downloadCoverChanged);
    connect(&settings(), &SettingsManager::playerPageStyleChanged, this, &SettingsViewModel::playerPageStyleChanged);
    connect(&settings(), &SettingsManager::serverEndpointChanged, this, &SettingsViewModel::settingsCenterChanged);
    connect(&settings(), &SettingsManager::settingsCenterChanged, this, &SettingsViewModel::settingsCenterChanged);

    syncFromSettings();
    updatePresenceSnapshot(OnlinePresenceManager::instance().currentAccount(),
                           OnlinePresenceManager::instance().currentToken(),
                           OnlinePresenceManager::instance().currentOnline(),
                           OnlinePresenceManager::instance().currentHeartbeatIntervalSec(),
                           OnlinePresenceManager::instance().currentOnlineTtlSec(),
                           OnlinePresenceManager::instance().currentTtlRemainingSec(),
                           OnlinePresenceManager::instance().currentStatusMessage(),
                           OnlinePresenceManager::instance().currentLastSeenAt());
}

QString SettingsViewModel::downloadPath() const
{
    return settings().downloadPath();
}

QString SettingsViewModel::audioCachePath() const
{
    return settings().audioCachePath();
}

QString SettingsViewModel::logPath() const
{
    return settings().logPath();
}

bool SettingsViewModel::downloadLyrics() const
{
    return settings().downloadLyrics();
}

bool SettingsViewModel::downloadCover() const
{
    return settings().downloadCover();
}

int SettingsViewModel::playerPageStyle() const
{
    return settings().playerPageStyle();
}

QString SettingsViewModel::serverHost() const
{
    return settings().serverHost();
}

int SettingsViewModel::serverPort() const
{
    return settings().serverPort();
}

bool SettingsViewModel::launchAtStartup() const
{
    return settings().launchAtStartup();
}

bool SettingsViewModel::autoPlayOnStartup() const
{
    return settings().autoPlayOnStartup();
}

bool SettingsViewModel::autoOpenLyrics() const
{
    return settings().autoOpenLyrics();
}

bool SettingsViewModel::disableDpiScale() const
{
    return settings().disableDpiScale();
}

bool SettingsViewModel::disableHardwareAcceleration() const
{
    return settings().disableHardwareAcceleration();
}

bool SettingsViewModel::preventSystemSleep() const
{
    return settings().preventSystemSleep();
}

bool SettingsViewModel::listAutoSwitch() const
{
    return settings().listAutoSwitch();
}

bool SettingsViewModel::enableFadeInOut() const
{
    return settings().enableFadeInOut();
}

bool SettingsViewModel::enableMultimediaKeys() const
{
    return settings().enableMultimediaKeys();
}

bool SettingsViewModel::autoBestVolume() const
{
    return settings().autoBestVolume();
}

bool SettingsViewModel::playbackAccelerationService() const
{
    return settings().playbackAccelerationService();
}

bool SettingsViewModel::preferLocalDownloadedQuality() const
{
    return settings().preferLocalDownloadedQuality();
}

bool SettingsViewModel::desktopLyricsShowOnStartup() const
{
    return settings().desktopLyricsShowOnStartup();
}

bool SettingsViewModel::desktopLyricsAlwaysOnTop() const
{
    return settings().desktopLyricsAlwaysOnTop();
}

QColor SettingsViewModel::desktopLyricColor() const
{
    return settings().desktopLyricColor();
}

int SettingsViewModel::desktopLyricFontSize() const
{
    return settings().desktopLyricFontSize();
}

QString SettingsViewModel::desktopLyricFontFamily() const
{
    return settings().desktopLyricFontFamily();
}

QString SettingsViewModel::shortcutPlayPause() const
{
    return settings().shortcutPlayPause();
}

QString SettingsViewModel::shortcutPreviousTrack() const
{
    return settings().shortcutPreviousTrack();
}

QString SettingsViewModel::shortcutNextTrack() const
{
    return settings().shortcutNextTrack();
}

QString SettingsViewModel::shortcutToggleDesktopLyrics() const
{
    return settings().shortcutToggleDesktopLyrics();
}

QString SettingsViewModel::shortcutOpenSearch() const
{
    return settings().shortcutOpenSearch();
}

bool SettingsViewModel::audioEffectsEnabled() const
{
    return settings().audioEffectsEnabled();
}

bool SettingsViewModel::replayGainEnabled() const
{
    return settings().replayGainEnabled();
}

bool SettingsViewModel::volumeNormalizationEnabled() const
{
    return settings().volumeNormalizationEnabled();
}

bool SettingsViewModel::equalizerEnabled() const
{
    return settings().equalizerEnabled();
}

QString SettingsViewModel::effectPreset() const
{
    return settings().effectPreset();
}

bool SettingsViewModel::proxyEnabled() const
{
    return settings().proxyEnabled();
}

QString SettingsViewModel::proxyHost() const
{
    return settings().proxyHost();
}

int SettingsViewModel::proxyPort() const
{
    return settings().proxyPort();
}

bool SettingsViewModel::followSystemAudioDevice() const
{
    return settings().followSystemAudioDevice();
}

bool SettingsViewModel::exclusiveAudioDeviceMode() const
{
    return settings().exclusiveAudioDeviceMode();
}

bool SettingsViewModel::lowLatencyAudioMode() const
{
    return settings().lowLatencyAudioMode();
}

int SettingsViewModel::outputSampleRate() const
{
    return settings().outputSampleRate();
}

bool SettingsViewModel::remoteControlComputerPlayback() const
{
    return settings().remoteControlComputerPlayback();
}

bool SettingsViewModel::autoPlayMvWhenAvailable() const
{
    return settings().autoPlayMvWhenAvailable();
}

bool SettingsViewModel::skipTrialTracks() const
{
    return settings().skipTrialTracks();
}

bool SettingsViewModel::syncRecentPlaylistToCloud() const
{
    return settings().syncRecentPlaylistToCloud();
}

QString SettingsViewModel::preferredPlaybackQuality() const
{
    return settings().preferredPlaybackQuality();
}

QString SettingsViewModel::singleTrackQueueMode() const
{
    return settings().singleTrackQueueMode();
}

bool SettingsViewModel::showMessageCenterBadge() const
{
    return settings().showMessageCenterBadge();
}

bool SettingsViewModel::showSmtc() const
{
    return settings().showSmtc();
}

bool SettingsViewModel::preferMp3Download() const
{
    return settings().preferMp3Download();
}

QString SettingsViewModel::downloadFolderClassification() const
{
    return settings().downloadFolderClassification();
}

QString SettingsViewModel::downloadSongNamingFormat() const
{
    return settings().downloadSongNamingFormat();
}

QString SettingsViewModel::cacheSpaceMode() const
{
    return settings().cacheSpaceMode();
}

int SettingsViewModel::cacheSpaceLimitMb() const
{
    return settings().cacheSpaceLimitMb();
}

bool SettingsViewModel::mp3TagApeV2() const
{
    return settings().mp3TagApeV2();
}

bool SettingsViewModel::mp3TagId3v1() const
{
    return settings().mp3TagId3v1();
}

bool SettingsViewModel::mp3TagId3v2() const
{
    return settings().mp3TagId3v2();
}

QString SettingsViewModel::desktopLyricsLineMode() const
{
    return settings().desktopLyricsLineMode();
}

QString SettingsViewModel::desktopLyricsAlignment() const
{
    return settings().desktopLyricsAlignment();
}

QString SettingsViewModel::desktopLyricsColorPreset() const
{
    return settings().desktopLyricsColorPreset();
}

QColor SettingsViewModel::desktopLyricsPlayedColor() const
{
    return settings().desktopLyricsPlayedColor();
}

QColor SettingsViewModel::desktopLyricsUnplayedColor() const
{
    return settings().desktopLyricsUnplayedColor();
}

QColor SettingsViewModel::desktopLyricsStrokeColor() const
{
    return settings().desktopLyricsStrokeColor();
}

bool SettingsViewModel::desktopLyricsCoverTaskbar() const
{
    return settings().desktopLyricsCoverTaskbar();
}

bool SettingsViewModel::desktopLyricsBold() const
{
    return settings().desktopLyricsBold();
}

int SettingsViewModel::desktopLyricsOpacity() const
{
    return settings().desktopLyricsOpacity();
}

bool SettingsViewModel::globalShortcutsEnabled() const
{
    return settings().globalShortcutsEnabled();
}

QString SettingsViewModel::shortcutVolumeUp() const
{
    return settings().shortcutVolumeUp();
}

QString SettingsViewModel::shortcutVolumeDown() const
{
    return settings().shortcutVolumeDown();
}

QString SettingsViewModel::shortcutLikeSong() const
{
    return settings().shortcutLikeSong();
}

QString SettingsViewModel::shortcutMusicRecognition() const
{
    return settings().shortcutMusicRecognition();
}

QString SettingsViewModel::shortcutToggleMainWindow() const
{
    return settings().shortcutToggleMainWindow();
}

QString SettingsViewModel::shortcutRewind() const
{
    return settings().shortcutRewind();
}

QString SettingsViewModel::shortcutFastForward() const
{
    return settings().shortcutFastForward();
}

QString SettingsViewModel::effectPluginType() const
{
    return settings().effectPluginType();
}

QString SettingsViewModel::pluginDirectory() const
{
    return settings().pluginDirectory();
}

QString SettingsViewModel::proxyType() const
{
    return settings().proxyType();
}

QString SettingsViewModel::proxyUsername() const
{
    return settings().proxyUsername();
}

QString SettingsViewModel::proxyPassword() const
{
    return settings().proxyPassword();
}

QString SettingsViewModel::audioOutputDevice() const
{
    return settings().audioOutputDevice();
}

QString SettingsViewModel::audioOutputFormat() const
{
    return settings().audioOutputFormat();
}

QString SettingsViewModel::audioFrequencyConversion() const
{
    return settings().audioFrequencyConversion();
}

QString SettingsViewModel::audioOutputChannels() const
{
    return settings().audioOutputChannels();
}

bool SettingsViewModel::audioLocalFileMemoryMode() const
{
    return settings().audioLocalFileMemoryMode();
}

bool SettingsViewModel::audioGaplessPlayback() const
{
    return settings().audioGaplessPlayback();
}

QString SettingsViewModel::audioDsdMode() const
{
    return settings().audioDsdMode();
}

int SettingsViewModel::audioCacheSizeMs() const
{
    return settings().audioCacheSizeMs();
}

void SettingsViewModel::chooseDownloadPath(QWidget* parent)
{
    const QString dir = QFileDialog::getExistingDirectory(
        parent,
        QStringLiteral("选择下载保存路径"),
        downloadPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        setDownloadPath(dir);
    }
}

void SettingsViewModel::chooseAudioCachePath(QWidget* parent)
{
    const QString dir = QFileDialog::getExistingDirectory(
        parent,
        QStringLiteral("选择音频缓存目录"),
        audioCachePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        setAudioCachePath(dir);
    }
}

void SettingsViewModel::chooseLogPath(QWidget* parent)
{
    QString filePath = QFileDialog::getSaveFileName(
        parent,
        QStringLiteral("选择日志文件路径"),
        logPath(),
        QStringLiteral("Text File (*.txt);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo info(filePath);
    if (info.suffix().isEmpty()) {
        filePath += QStringLiteral(".txt");
    }

    setLogPath(filePath);
}

void SettingsViewModel::clearLocalCache(QWidget* parent)
{
    const QString cachePath = audioCachePath().trimmed();
    if (cachePath.isEmpty()) {
        QMessageBox::warning(parent,
                             QStringLiteral("清理失败"),
                             QStringLiteral("当前缓存目录无效，请先设置缓存目录。"));
        return;
    }

    const bool isAudioActive = AudioService::instance().isPlaying() || AudioService::instance().isPaused();
    if (isAudioActive) {
        const auto stopReply = QMessageBox::question(
            parent,
            QStringLiteral("清理本地缓存"),
            QStringLiteral("检测到正在播放音频，清理缓存前需要先停止播放。是否继续？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (stopReply != QMessageBox::Yes) {
            return;
        }
        AudioService::instance().stop();
    }

    const auto confirmReply = QMessageBox::question(
        parent,
        QStringLiteral("确认清理"),
        QStringLiteral("将删除缓存目录中的本地缓存文件：\n%1\n\n该操作不可恢复，是否继续？").arg(cachePath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (confirmReply != QMessageBox::Yes) {
        return;
    }

    qint64 removedBytes = 0;
    qint64 removedFiles = 0;
    QString errorMessage;
    const bool cleared = AudioCacheManager::instance().clearCache(&errorMessage, &removedBytes, &removedFiles);
    Network::NetworkService::instance().clearCache();

    if (!cleared) {
        QMessageBox::warning(
            parent,
            QStringLiteral("清理完成（部分失败）"),
            QStringLiteral("缓存已部分清理，部分文件可能正在占用。\n已清理文件：%1\n已释放空间：%2\n详情：%3")
                .arg(QString::number(removedFiles),
                     formatBytes(removedBytes),
                     errorMessage.isEmpty() ? QStringLiteral("未知错误") : errorMessage));
        return;
    }

    QMessageBox::information(
        parent,
        QStringLiteral("清理完成"),
        QStringLiteral("本地缓存已清理完成。\n已清理文件：%1\n已释放空间：%2")
            .arg(QString::number(removedFiles), formatBytes(removedBytes)));
}

void SettingsViewModel::setDownloadPath(const QString& path)
{
    settings().setDownloadPath(path);
}

void SettingsViewModel::setAudioCachePath(const QString& path)
{
    settings().setAudioCachePath(path);
}

void SettingsViewModel::setLogPath(const QString& path)
{
    settings().setLogPath(path);
}

void SettingsViewModel::setDownloadLyrics(bool enabled)
{
    settings().setDownloadLyrics(enabled);
}

void SettingsViewModel::setDownloadCover(bool enabled)
{
    settings().setDownloadCover(enabled);
}

void SettingsViewModel::setPlayerPageStyle(int styleId)
{
    settings().setPlayerPageStyle(styleId);
}

void SettingsViewModel::setServerHost(const QString& host)
{
    settings().setServerHost(host);
}

void SettingsViewModel::setServerPort(int port)
{
    settings().setServerPort(port);
}

void SettingsViewModel::setLaunchAtStartup(bool enabled)
{
    settings().setLaunchAtStartup(enabled);
}

void SettingsViewModel::setAutoPlayOnStartup(bool enabled)
{
    settings().setAutoPlayOnStartup(enabled);
}

void SettingsViewModel::setAutoOpenLyrics(bool enabled)
{
    settings().setAutoOpenLyrics(enabled);
}

void SettingsViewModel::setDisableDpiScale(bool enabled)
{
    settings().setDisableDpiScale(enabled);
}

void SettingsViewModel::setDisableHardwareAcceleration(bool enabled)
{
    settings().setDisableHardwareAcceleration(enabled);
}

void SettingsViewModel::setPreventSystemSleep(bool enabled)
{
    settings().setPreventSystemSleep(enabled);
}

void SettingsViewModel::setListAutoSwitch(bool enabled)
{
    settings().setListAutoSwitch(enabled);
}

void SettingsViewModel::setEnableFadeInOut(bool enabled)
{
    settings().setEnableFadeInOut(enabled);
}

void SettingsViewModel::setEnableMultimediaKeys(bool enabled)
{
    settings().setEnableMultimediaKeys(enabled);
}

void SettingsViewModel::setAutoBestVolume(bool enabled)
{
    settings().setAutoBestVolume(enabled);
}

void SettingsViewModel::setPlaybackAccelerationService(bool enabled)
{
    settings().setPlaybackAccelerationService(enabled);
}

void SettingsViewModel::setPreferLocalDownloadedQuality(bool enabled)
{
    settings().setPreferLocalDownloadedQuality(enabled);
}

void SettingsViewModel::setDesktopLyricsShowOnStartup(bool enabled)
{
    settings().setDesktopLyricsShowOnStartup(enabled);
}

void SettingsViewModel::setDesktopLyricsAlwaysOnTop(bool enabled)
{
    settings().setDesktopLyricsAlwaysOnTop(enabled);
}

void SettingsViewModel::setDesktopLyricColor(const QColor& color)
{
    settings().setDesktopLyricColor(color);
}

void SettingsViewModel::setDesktopLyricFontSize(int size)
{
    settings().setDesktopLyricFontSize(size);
}

void SettingsViewModel::setDesktopLyricFontFamily(const QString& family)
{
    settings().setDesktopLyricFontFamily(family);
}

void SettingsViewModel::setShortcutPlayPause(const QString& shortcut)
{
    settings().setShortcutPlayPause(shortcut);
}

void SettingsViewModel::setShortcutPreviousTrack(const QString& shortcut)
{
    settings().setShortcutPreviousTrack(shortcut);
}

void SettingsViewModel::setShortcutNextTrack(const QString& shortcut)
{
    settings().setShortcutNextTrack(shortcut);
}

void SettingsViewModel::setShortcutToggleDesktopLyrics(const QString& shortcut)
{
    settings().setShortcutToggleDesktopLyrics(shortcut);
}

void SettingsViewModel::setShortcutOpenSearch(const QString& shortcut)
{
    settings().setShortcutOpenSearch(shortcut);
}

void SettingsViewModel::setAudioEffectsEnabled(bool enabled)
{
    settings().setAudioEffectsEnabled(enabled);
}

void SettingsViewModel::setReplayGainEnabled(bool enabled)
{
    settings().setReplayGainEnabled(enabled);
}

void SettingsViewModel::setVolumeNormalizationEnabled(bool enabled)
{
    settings().setVolumeNormalizationEnabled(enabled);
}

void SettingsViewModel::setEqualizerEnabled(bool enabled)
{
    settings().setEqualizerEnabled(enabled);
}

void SettingsViewModel::setEffectPreset(const QString& preset)
{
    settings().setEffectPreset(preset);
}

void SettingsViewModel::setProxyEnabled(bool enabled)
{
    settings().setProxyEnabled(enabled);
}

void SettingsViewModel::setProxyHost(const QString& host)
{
    settings().setProxyHost(host);
}

void SettingsViewModel::setProxyPort(int port)
{
    settings().setProxyPort(port);
}

void SettingsViewModel::setFollowSystemAudioDevice(bool enabled)
{
    settings().setFollowSystemAudioDevice(enabled);
}

void SettingsViewModel::setExclusiveAudioDeviceMode(bool enabled)
{
    settings().setExclusiveAudioDeviceMode(enabled);
}

void SettingsViewModel::setLowLatencyAudioMode(bool enabled)
{
    settings().setLowLatencyAudioMode(enabled);
}

void SettingsViewModel::setOutputSampleRate(int sampleRate)
{
    settings().setOutputSampleRate(sampleRate);
}

void SettingsViewModel::setRemoteControlComputerPlayback(bool enabled)
{
    settings().setRemoteControlComputerPlayback(enabled);
}

void SettingsViewModel::setAutoPlayMvWhenAvailable(bool enabled)
{
    settings().setAutoPlayMvWhenAvailable(enabled);
}

void SettingsViewModel::setSkipTrialTracks(bool enabled)
{
    settings().setSkipTrialTracks(enabled);
}

void SettingsViewModel::setSyncRecentPlaylistToCloud(bool enabled)
{
    settings().setSyncRecentPlaylistToCloud(enabled);
}

void SettingsViewModel::setPreferredPlaybackQuality(const QString& quality)
{
    settings().setPreferredPlaybackQuality(quality);
}

void SettingsViewModel::setSingleTrackQueueMode(const QString& mode)
{
    settings().setSingleTrackQueueMode(mode);
}

void SettingsViewModel::setShowMessageCenterBadge(bool enabled)
{
    settings().setShowMessageCenterBadge(enabled);
}

void SettingsViewModel::setShowSmtc(bool enabled)
{
    settings().setShowSmtc(enabled);
}

void SettingsViewModel::setPreferMp3Download(bool enabled)
{
    settings().setPreferMp3Download(enabled);
}

void SettingsViewModel::setDownloadFolderClassification(const QString& mode)
{
    settings().setDownloadFolderClassification(mode);
}

void SettingsViewModel::setDownloadSongNamingFormat(const QString& format)
{
    settings().setDownloadSongNamingFormat(format);
}

void SettingsViewModel::setCacheSpaceMode(const QString& mode)
{
    settings().setCacheSpaceMode(mode);
}

void SettingsViewModel::setCacheSpaceLimitMb(int limitMb)
{
    settings().setCacheSpaceLimitMb(limitMb);
}

void SettingsViewModel::setMp3TagApeV2(bool enabled)
{
    settings().setMp3TagApeV2(enabled);
}

void SettingsViewModel::setMp3TagId3v1(bool enabled)
{
    settings().setMp3TagId3v1(enabled);
}

void SettingsViewModel::setMp3TagId3v2(bool enabled)
{
    settings().setMp3TagId3v2(enabled);
}

void SettingsViewModel::setDesktopLyricsLineMode(const QString& mode)
{
    settings().setDesktopLyricsLineMode(mode);
}

void SettingsViewModel::setDesktopLyricsAlignment(const QString& alignment)
{
    settings().setDesktopLyricsAlignment(alignment);
}

void SettingsViewModel::setDesktopLyricsColorPreset(const QString& preset)
{
    settings().setDesktopLyricsColorPreset(preset);
}

void SettingsViewModel::setDesktopLyricsPlayedColor(const QColor& color)
{
    settings().setDesktopLyricsPlayedColor(color);
}

void SettingsViewModel::setDesktopLyricsUnplayedColor(const QColor& color)
{
    settings().setDesktopLyricsUnplayedColor(color);
}

void SettingsViewModel::setDesktopLyricsStrokeColor(const QColor& color)
{
    settings().setDesktopLyricsStrokeColor(color);
}

void SettingsViewModel::setDesktopLyricsCoverTaskbar(bool enabled)
{
    settings().setDesktopLyricsCoverTaskbar(enabled);
}

void SettingsViewModel::setDesktopLyricsBold(bool enabled)
{
    settings().setDesktopLyricsBold(enabled);
}

void SettingsViewModel::setDesktopLyricsOpacity(int opacity)
{
    settings().setDesktopLyricsOpacity(opacity);
}

void SettingsViewModel::setGlobalShortcutsEnabled(bool enabled)
{
    settings().setGlobalShortcutsEnabled(enabled);
}

void SettingsViewModel::setShortcutVolumeUp(const QString& shortcut)
{
    settings().setShortcutVolumeUp(shortcut);
}

void SettingsViewModel::setShortcutVolumeDown(const QString& shortcut)
{
    settings().setShortcutVolumeDown(shortcut);
}

void SettingsViewModel::setShortcutLikeSong(const QString& shortcut)
{
    settings().setShortcutLikeSong(shortcut);
}

void SettingsViewModel::setShortcutMusicRecognition(const QString& shortcut)
{
    settings().setShortcutMusicRecognition(shortcut);
}

void SettingsViewModel::setShortcutToggleMainWindow(const QString& shortcut)
{
    settings().setShortcutToggleMainWindow(shortcut);
}

void SettingsViewModel::setShortcutRewind(const QString& shortcut)
{
    settings().setShortcutRewind(shortcut);
}

void SettingsViewModel::setShortcutFastForward(const QString& shortcut)
{
    settings().setShortcutFastForward(shortcut);
}

void SettingsViewModel::setEffectPluginType(const QString& pluginType)
{
    settings().setEffectPluginType(pluginType);
}

void SettingsViewModel::setPluginDirectory(const QString& path)
{
    settings().setPluginDirectory(path);
}

void SettingsViewModel::setProxyType(const QString& proxyType)
{
    settings().setProxyType(proxyType);
}

void SettingsViewModel::setProxyUsername(const QString& username)
{
    settings().setProxyUsername(username);
}

void SettingsViewModel::setProxyPassword(const QString& password)
{
    settings().setProxyPassword(password);
}

void SettingsViewModel::setAudioOutputDevice(const QString& device)
{
    settings().setAudioOutputDevice(device);
}

void SettingsViewModel::setAudioOutputFormat(const QString& format)
{
    settings().setAudioOutputFormat(format);
}

void SettingsViewModel::setAudioFrequencyConversion(const QString& mode)
{
    settings().setAudioFrequencyConversion(mode);
}

void SettingsViewModel::setAudioOutputChannels(const QString& channels)
{
    settings().setAudioOutputChannels(channels);
}

void SettingsViewModel::setAudioLocalFileMemoryMode(bool enabled)
{
    settings().setAudioLocalFileMemoryMode(enabled);
}

void SettingsViewModel::setAudioGaplessPlayback(bool enabled)
{
    settings().setAudioGaplessPlayback(enabled);
}

void SettingsViewModel::setAudioDsdMode(const QString& mode)
{
    settings().setAudioDsdMode(mode);
}

void SettingsViewModel::setAudioCacheSizeMs(int cacheSizeMs)
{
    settings().setAudioCacheSizeMs(cacheSizeMs);
}

void SettingsViewModel::refreshPresence()
{
    OnlinePresenceManager::instance().requestStatusRefresh();
}

void SettingsViewModel::syncFromSettings()
{
    emit downloadPathChanged();
    emit audioCachePathChanged();
    emit logPathChanged();
    emit downloadLyricsChanged();
    emit downloadCoverChanged();
    emit playerPageStyleChanged();
    emit settingsCenterChanged();
}

void SettingsViewModel::resetDesktopLyricsSettings()
{
    settings().resetDesktopLyricsSettings();
}

void SettingsViewModel::resetShortcutSettings()
{
    settings().resetShortcutSettings();
}

void SettingsViewModel::resetAudioDeviceSettings()
{
    settings().resetAudioDeviceSettings();
}

void SettingsViewModel::onPresenceSnapshotChanged(const QString& account,
                                                  const QString& sessionToken,
                                                  bool online,
                                                  int heartbeatIntervalSec,
                                                  int onlineTtlSec,
                                                  int ttlRemainingSec,
                                                  const QString& statusMessage,
                                                  qint64 lastSeenAt)
{
    updatePresenceSnapshot(account,
                           sessionToken,
                           online,
                           heartbeatIntervalSec,
                           onlineTtlSec,
                           ttlRemainingSec,
                           statusMessage,
                           lastSeenAt);
}

void SettingsViewModel::updatePresenceSnapshot(const QString& account,
                                               const QString& sessionToken,
                                               bool online,
                                               int heartbeatIntervalSec,
                                               int onlineTtlSec,
                                               int ttlRemainingSec,
                                               const QString& statusMessage,
                                               qint64 lastSeenAt)
{
    m_presenceAccount = account;
    m_presenceSessionToken = sessionToken;
    m_presenceOnline = online;
    m_presenceHeartbeatIntervalSec = heartbeatIntervalSec;
    m_presenceOnlineTtlSec = onlineTtlSec;
    m_presenceTtlRemainingSec = ttlRemainingSec;
    m_presenceStatusMessage = statusMessage;

    m_presenceLastSeenText = QStringLiteral("未上报");
    if (lastSeenAt > 0) {
        const QDateTime dt = QDateTime::fromSecsSinceEpoch(lastSeenAt);
        m_presenceLastSeenText = dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    emit presenceChanged();
}
