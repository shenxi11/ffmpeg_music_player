#include "client_automation_host_service.h"

#include "AudioService.h"
#include "download_manager.h"
#include "local_music_cache.h"
#include "main_widget.h"
#include "music.h"
#include "plugin_manager.h"
#include "settings_manager.h"
#include "viewmodels/MainShellViewModel.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace {

QString fallbackTrackId(const QString& path, const QString& title, const QString& artist)
{
    const QString base = QStringLiteral("%1|%2|%3").arg(path, title, artist);
    const QByteArray digest = QCryptographicHash::hash(base.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(digest.constData(), digest.size());
}

QUrl toPlayableUrl(const QString& rawPath)
{
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (trimmed.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        return QUrl(trimmed);
    }
    return QUrl::fromLocalFile(trimmed);
}

QString playModeName(int playMode)
{
    switch (playMode) {
    case AudioService::Sequential:
        return QStringLiteral("sequential");
    case AudioService::RepeatOne:
        return QStringLiteral("repeat_one");
    case AudioService::RepeatAll:
        return QStringLiteral("repeat_all");
    case AudioService::Shuffle:
        return QStringLiteral("shuffle");
    default:
        return QStringLiteral("unknown");
    }
}

QString downloadStateName(DownloadState state)
{
    switch (state) {
    case DownloadState::Waiting:
        return QStringLiteral("waiting");
    case DownloadState::Downloading:
        return QStringLiteral("downloading");
    case DownloadState::Paused:
        return QStringLiteral("paused");
    case DownloadState::Completed:
        return QStringLiteral("completed");
    case DownloadState::Failed:
        return QStringLiteral("failed");
    case DownloadState::Cancelled:
        return QStringLiteral("cancelled");
    }
    return QStringLiteral("unknown");
}

QVariantMap convertDownloadTask(const DownloadTask& task)
{
    return {
        {QStringLiteral("taskId"), task.taskId},
        {QStringLiteral("url"), task.url},
        {QStringLiteral("filename"), task.filename},
        {QStringLiteral("savePath"), task.savePath},
        {QStringLiteral("coverUrl"), task.coverUrl},
        {QStringLiteral("state"), downloadStateName(task.state)},
        {QStringLiteral("progress"), task.progress()},
        {QStringLiteral("downloadedSize"), task.downloadedSize},
        {QStringLiteral("totalSize"), task.totalSize},
        {QStringLiteral("error"), task.errorMsg},
        {QStringLiteral("createTime"), task.createTime.toString(Qt::ISODate)}
    };
}

QVariantList convertDownloadTasks(const QList<DownloadTask>& tasks)
{
    QVariantList items;
    items.reserve(tasks.size());
    for (const DownloadTask& task : tasks) {
        items.push_back(convertDownloadTask(task));
    }
    return items;
}

QString findDownloadTaskIdBySavePath(const QList<DownloadTask>& tasks, const QString& savePath)
{
    const QString targetPath = QDir::cleanPath(savePath);
    for (const DownloadTask& task : tasks) {
        if (QDir::cleanPath(task.savePath) == targetPath) {
            return task.taskId;
        }
    }
    return {};
}

QString pluginStateName(PluginState state)
{
    switch (state) {
    case PluginState::Loaded:
        return QStringLiteral("loaded");
    case PluginState::Initialized:
        return QStringLiteral("initialized");
    case PluginState::Failed:
        return QStringLiteral("failed");
    case PluginState::Unloaded:
        return QStringLiteral("unloaded");
    }
    return QStringLiteral("unknown");
}

QVariantMap convertPluginInfo(const PluginInfo& info)
{
    return {
        {QStringLiteral("id"), info.id},
        {QStringLiteral("name"), info.name},
        {QStringLiteral("description"), info.description},
        {QStringLiteral("version"), info.version},
        {QStringLiteral("apiVersion"), info.apiVersion},
        {QStringLiteral("author"), info.author},
        {QStringLiteral("capabilities"), info.capabilities},
        {QStringLiteral("dependencies"), info.dependencies},
        {QStringLiteral("requestedPermissions"), info.requestedPermissions},
        {QStringLiteral("grantedPermissions"), info.grantedPermissions},
        {QStringLiteral("filePath"), info.filePath},
        {QStringLiteral("isLoaded"), info.isLoaded},
        {QStringLiteral("state"), pluginStateName(info.state)},
        {QStringLiteral("lastError"), info.lastError}
    };
}

qint64 parseDurationSeconds(const QString& durationText)
{
    const QString trimmed = durationText.trimmed();
    if (trimmed.contains(QLatin1Char(':'))) {
        const QStringList parts = trimmed.split(QLatin1Char(':'));
        if (parts.size() >= 2) {
            bool okMin = false;
            bool okSec = false;
            const qint64 minutes = parts.at(0).toLongLong(&okMin);
            const qint64 seconds = parts.at(1).toLongLong(&okSec);
            if (okMin && okSec) {
                return qMax<qint64>(0, minutes * 60 + seconds);
            }
        }
    }
    bool ok = false;
    const qint64 seconds = trimmed.toLongLong(&ok);
    return ok ? qMax<qint64>(0, seconds) : 0;
}

} // namespace

ClientAutomationHostService::ClientAutomationHostService(MainWidget* mainWidget,
                                                         MainShellViewModel* shellViewModel,
                                                         QObject* parent)
    : QObject(parent)
    , m_mainWidget(mainWidget)
    , m_shellViewModel(shellViewModel)
{
    if (!m_shellViewModel) {
        return;
    }

    connect(m_shellViewModel, &MainShellViewModel::searchResultsReady, this,
            [this](const QList<Music>& list) { emit searchResultsReady(convertMusicList(list)); });
    connect(m_shellViewModel, &MainShellViewModel::historyListReady, this,
            &ClientAutomationHostService::historyListReady);
    connect(m_shellViewModel, &MainShellViewModel::favoritesListReady, this,
            &ClientAutomationHostService::favoritesListReady);
    connect(m_shellViewModel, &MainShellViewModel::playlistsListReady, this,
            &ClientAutomationHostService::playlistsListReady);
    connect(m_shellViewModel, &MainShellViewModel::playlistDetailReady, this,
            &ClientAutomationHostService::playlistDetailReady);
    connect(m_shellViewModel, &MainShellViewModel::recommendationListReady, this,
            &ClientAutomationHostService::recommendationListReady);
    connect(m_shellViewModel, &MainShellViewModel::similarRecommendationListReady, this,
            &ClientAutomationHostService::similarRecommendationListReady);
    connect(m_shellViewModel, &MainShellViewModel::recommendationFeedbackResultReady, this,
            &ClientAutomationHostService::recommendationFeedbackResultReady);
    connect(m_shellViewModel, &MainShellViewModel::addFavoriteResultReady, this,
            &ClientAutomationHostService::addFavoriteResultReady);
    connect(m_shellViewModel, &MainShellViewModel::removeFavoriteResultReady, this,
            &ClientAutomationHostService::removeFavoriteResultReady);
    connect(m_shellViewModel, &MainShellViewModel::addHistoryResultReady, this,
            &ClientAutomationHostService::addHistoryResultReady);
    connect(m_shellViewModel, &MainShellViewModel::removeHistoryResultReady, this,
            &ClientAutomationHostService::removeHistoryResultReady);
    connect(m_shellViewModel, &MainShellViewModel::createPlaylistResultReady, this,
            &ClientAutomationHostService::createPlaylistResultReady);
    connect(m_shellViewModel, &MainShellViewModel::deletePlaylistResultReady, this,
            &ClientAutomationHostService::deletePlaylistResultReady);
    connect(m_shellViewModel, &MainShellViewModel::updatePlaylistResultReady, this,
            &ClientAutomationHostService::updatePlaylistResultReady);
    connect(m_shellViewModel, &MainShellViewModel::addPlaylistItemsResultReady, this,
            &ClientAutomationHostService::addPlaylistItemsResultReady);
    connect(m_shellViewModel, &MainShellViewModel::removePlaylistItemsResultReady, this,
            &ClientAutomationHostService::removePlaylistItemsResultReady);
    connect(m_shellViewModel, &MainShellViewModel::reorderPlaylistItemsResultReady, this,
            &ClientAutomationHostService::reorderPlaylistItemsResultReady);

    if (HttpRequestV2* gateway = m_shellViewModel->requestGateway()) {
        connect(gateway, &HttpRequestV2::signalLrc, this,
                &ClientAutomationHostService::lyricsReady);
        connect(gateway, &HttpRequestV2::signalVideoList, this,
                &ClientAutomationHostService::videoListReady);
        connect(gateway, &HttpRequestV2::signalVideoStreamUrl, this,
                &ClientAutomationHostService::videoStreamUrlReady);
        connect(gateway, &HttpRequestV2::signalArtistExists, this,
                &ClientAutomationHostService::artistSearchReady);
        connect(gateway, &HttpRequestV2::signalArtistMusicList, this,
                [this](const QList<Music>& list, const QString& artist) {
                    emit artistTracksReady(convertMusicList(list), artist);
                });
    }
}

QString ClientAutomationHostService::currentUserAccount() const
{
    return m_shellViewModel ? m_shellViewModel->currentUserAccount().trimmed() : QString();
}

QVariantMap ClientAutomationHostService::hostContextSnapshot() const
{
    QVariantMap payload = uiOverviewSnapshot();
    payload.insert(QStringLiteral("currentTrack"), currentTrackSnapshot());
    payload.insert(QStringLiteral("queueSummary"), playbackQueueSnapshot());
    return payload;
}

QVariantMap ClientAutomationHostService::currentTrackSnapshot() const
{
    return m_mainWidget ? m_mainWidget->agentCurrentTrackSnapshot() : QVariantMap();
}

QVariantList ClientAutomationHostService::localMusicItems(int limit) const
{
    const QList<LocalMusicInfo> locals = LocalMusicCache::instance().getMusicList();
    const int bounded = (limit <= 0) ? locals.size() : qMin(limit, locals.size());
    QVariantList items;
    items.reserve(bounded);
    for (int i = 0; i < bounded; ++i) {
        const LocalMusicInfo& local = locals.at(i);
        const QString path = local.filePath.trimmed();
        const QString title = local.fileName.trimmed().isEmpty()
            ? QFileInfo(path).completeBaseName()
            : local.fileName.trimmed();
        const QString artist = local.artist.trimmed().isEmpty()
            ? QStringLiteral("未知艺术家")
            : local.artist.trimmed();
        QVariantMap item;
        item.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
        item.insert(QStringLiteral("musicPath"), path);
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("artist"), artist);
        item.insert(QStringLiteral("album"), QString());
        item.insert(QStringLiteral("durationMs"), parseDurationSeconds(local.duration) * 1000);
        item.insert(QStringLiteral("coverUrl"), local.coverUrl);
        item.insert(QStringLiteral("isFavorite"), false);
        item.insert(QStringLiteral("isLocal"), true);
        items.push_back(item);
    }
    return items;
}

QVariantMap ClientAutomationHostService::playbackQueueSnapshot() const
{
    return m_mainWidget ? m_mainWidget->agentQueueSnapshot() : QVariantMap();
}

QVariantMap ClientAutomationHostService::settingsSnapshot() const
{
    SettingsManager& settings = SettingsManager::instance();
    return {
        {QStringLiteral("downloadPath"), settings.downloadPath()},
        {QStringLiteral("downloadLyrics"), settings.downloadLyrics()},
        {QStringLiteral("downloadCover"), settings.downloadCover()},
        {QStringLiteral("audioCachePath"), settings.audioCachePath()},
        {QStringLiteral("logPath"), settings.logPath()},
        {QStringLiteral("serverHost"), settings.serverHost()},
        {QStringLiteral("serverPort"), settings.serverPort()},
        {QStringLiteral("serverBaseUrl"), settings.serverBaseUrl()},
        {QStringLiteral("playerPageStyle"), settings.playerPageStyle()}
    };
}

bool ClientAutomationHostService::updateSetting(const QString& key, const QVariant& value)
{
    SettingsManager& settings = SettingsManager::instance();
    if (key == QStringLiteral("downloadPath")) {
        settings.setDownloadPath(value.toString());
    } else if (key == QStringLiteral("downloadLyrics")) {
        settings.setDownloadLyrics(value.toBool());
    } else if (key == QStringLiteral("downloadCover")) {
        settings.setDownloadCover(value.toBool());
    } else if (key == QStringLiteral("audioCachePath")) {
        settings.setAudioCachePath(value.toString());
    } else if (key == QStringLiteral("logPath")) {
        settings.setLogPath(value.toString());
    } else if (key == QStringLiteral("serverHost")) {
        settings.setServerHost(value.toString());
    } else if (key == QStringLiteral("serverPort")) {
        settings.setServerPort(value.toInt());
    } else if (key == QStringLiteral("playerPageStyle")) {
        settings.setPlayerPageStyle(value.toInt());
    } else {
        return false;
    }
    return true;
}

bool ClientAutomationHostService::pausePlayback()
{
    AudioService::instance().pause();
    return true;
}

bool ClientAutomationHostService::resumePlayback()
{
    AudioService::instance().resume();
    return true;
}

bool ClientAutomationHostService::stopPlayback()
{
    AudioService::instance().stop();
    return true;
}

bool ClientAutomationHostService::seekPlayback(qint64 positionMs)
{
    AudioService::instance().seekTo(qMax<qint64>(0, positionMs));
    return true;
}

bool ClientAutomationHostService::playNext()
{
    AudioService::instance().playNext();
    return true;
}

bool ClientAutomationHostService::playPrevious()
{
    AudioService::instance().playPrevious();
    return true;
}

bool ClientAutomationHostService::playAtIndex(int index)
{
    AudioService::instance().playAtIndex(index);
    return true;
}

bool ClientAutomationHostService::setVolume(int volume)
{
    AudioService::instance().setVolume(qBound(0, volume, 100));
    return true;
}

bool ClientAutomationHostService::setPlayMode(int mode)
{
    if (mode < AudioService::Sequential || mode > AudioService::Shuffle) {
        return false;
    }
    AudioService::instance().setPlayMode(static_cast<AudioService::PlayMode>(mode));
    return true;
}

bool ClientAutomationHostService::setPlaybackQueue(const QVariantList& trackItems,
                                                   int startIndex,
                                                   bool playNow)
{
    QList<QUrl> urls;
    for (const QVariant& value : trackItems) {
        const QString path = value.toMap().value(QStringLiteral("musicPath")).toString().trimmed();
        const QUrl url = toPlayableUrl(path);
        if (!url.isEmpty()) {
            urls.push_back(url);
        }
    }
    if (urls.isEmpty()) {
        return false;
    }
    AudioService& service = AudioService::instance();
    service.clearPlaylist();
    for (const QUrl& url : urls) {
        service.addToPlaylist(url);
    }
    if (playNow) {
        service.playAtIndex(qBound(0, startIndex, urls.size() - 1));
    }
    return true;
}

bool ClientAutomationHostService::addToPlaybackQueue(const QString& musicPath)
{
    const QUrl url = toPlayableUrl(musicPath);
    if (url.isEmpty()) {
        return false;
    }
    AudioService::instance().addToPlaylist(url);
    return true;
}

bool ClientAutomationHostService::removeFromPlaybackQueue(int index)
{
    const QList<QUrl> queue = AudioService::instance().playlist();
    if (index < 0 || index >= queue.size()) {
        return false;
    }
    AudioService::instance().removeFromPlaylist(index);
    return true;
}

bool ClientAutomationHostService::clearPlaybackQueue()
{
    AudioService::instance().clearPlaylist();
    return true;
}

bool ClientAutomationHostService::playTrack(const QString& musicPath)
{
    const QUrl url = toPlayableUrl(musicPath);
    return !url.isEmpty() && AudioService::instance().play(url);
}

bool ClientAutomationHostService::playPlaylist(const QVariantList& trackItems)
{
    QList<QUrl> urls;
    for (const QVariant& value : trackItems) {
        const QUrl url = toPlayableUrl(value.toMap().value(QStringLiteral("musicPath")).toString());
        if (!url.isEmpty()) {
            urls.push_back(url);
        }
    }
    if (urls.isEmpty()) {
        return false;
    }
    AudioService::instance().setPlaylist(urls);
    AudioService::instance().playPlaylist();
    return true;
}

bool ClientAutomationHostService::addLocalTrack(const QVariantMap& track)
{
    const QString filePath = track.value(QStringLiteral("filePath")).toString().trimmed();
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return false;
    }
    LocalMusicInfo info;
    info.filePath = filePath;
    info.fileName = track.value(QStringLiteral("fileName")).toString().trimmed();
    if (info.fileName.isEmpty()) {
        info.fileName = QFileInfo(filePath).fileName();
    }
    info.artist = track.value(QStringLiteral("artist")).toString().trimmed();
    if (info.artist.isEmpty()) {
        info.artist = QStringLiteral("未知艺术家");
    }
    LocalMusicCache::instance().addMusic(info);
    return true;
}

bool ClientAutomationHostService::removeLocalTrack(const QString& filePath)
{
    if (filePath.trimmed().isEmpty()) {
        return false;
    }
    LocalMusicCache::instance().removeMusic(filePath.trimmed());
    return true;
}

QVariantMap ClientAutomationHostService::downloadTasksSnapshot(const QString& scope) const
{
    const QString normalized = scope.trimmed().toLower();
    QList<DownloadTask> tasks;
    if (normalized == QStringLiteral("active")) {
        tasks = DownloadManager::instance().getActiveTasks();
    } else if (normalized == QStringLiteral("completed")) {
        tasks = DownloadManager::instance().getCompletedTasks();
    } else {
        tasks = DownloadManager::instance().getAllTasks();
    }
    return {
        {QStringLiteral("scope"), normalized.isEmpty() ? QStringLiteral("all") : normalized},
        {QStringLiteral("items"), convertDownloadTasks(tasks)},
        {QStringLiteral("count"), tasks.size()},
        {QStringLiteral("activeDownloads"), DownloadManager::instance().activeDownloads()},
        {QStringLiteral("queueSize"), DownloadManager::instance().queueSize()}
    };
}

QVariantMap ClientAutomationHostService::downloadTaskSnapshot(const QString& taskId) const
{
    return convertDownloadTask(DownloadManager::instance().getTask(taskId));
}

QVariantMap ClientAutomationHostService::startDownloadTrack(const QString& relativePath,
                                                            const QString& coverUrl)
{
    if (!m_shellViewModel || relativePath.trimmed().isEmpty()) {
        return {{QStringLiteral("accepted"), false}};
    }
    const QString downloadFolder = SettingsManager::instance().downloadPath();
    const QString savePath = QDir::cleanPath(downloadFolder + QLatin1Char('/') + relativePath);
    const QString existingTaskId =
        findDownloadTaskIdBySavePath(DownloadManager::instance().getAllTasks(), savePath);
    m_shellViewModel->requestGateway()->download(relativePath,
                                                 downloadFolder,
                                                 SettingsManager::instance().downloadLyrics(),
                                                 coverUrl);
    const QString taskId =
        findDownloadTaskIdBySavePath(DownloadManager::instance().getAllTasks(), savePath);
    if (taskId.isEmpty()) {
        return {{QStringLiteral("accepted"), false}};
    }
    return {
        {QStringLiteral("accepted"), true},
        {QStringLiteral("taskId"), taskId},
        {QStringLiteral("relativePath"), relativePath},
        {QStringLiteral("coverUrl"), coverUrl},
        {QStringLiteral("savePath"), savePath},
        {QStringLiteral("reusedExistingTask"), !existingTaskId.isEmpty() && existingTaskId == taskId},
        {QStringLiteral("task"), convertDownloadTask(DownloadManager::instance().getTask(taskId))}
    };
}

bool ClientAutomationHostService::pauseDownloadTask(const QString& taskId)
{
    DownloadManager::instance().pauseDownload(taskId);
    return true;
}

bool ClientAutomationHostService::resumeDownloadTask(const QString& taskId)
{
    DownloadManager::instance().resumeDownload(taskId);
    return true;
}

bool ClientAutomationHostService::cancelDownloadTask(const QString& taskId)
{
    DownloadManager::instance().cancelDownload(taskId);
    return true;
}

QVariantMap ClientAutomationHostService::removeDownloadTask(const QString& taskId)
{
    const DownloadTask task = DownloadManager::instance().getTask(taskId);
    DownloadManager::instance().removeTask(taskId);
    return {{QStringLiteral("removed"), true},
            {QStringLiteral("taskId"), taskId},
            {QStringLiteral("filename"), task.filename}};
}

QVariantMap ClientAutomationHostService::videoWindowState() const
{
    return m_mainWidget ? m_mainWidget->agentVideoWindowState() : QVariantMap();
}

bool ClientAutomationHostService::playVideo(const QString& source)
{
    return m_mainWidget && m_mainWidget->agentPlayVideo(source);
}

bool ClientAutomationHostService::pauseVideo()
{
    return m_mainWidget && m_mainWidget->agentPauseVideo();
}

bool ClientAutomationHostService::resumeVideo()
{
    return m_mainWidget && m_mainWidget->agentResumeVideo();
}

bool ClientAutomationHostService::seekVideo(qint64 positionMs)
{
    return m_mainWidget && m_mainWidget->agentSeekVideo(positionMs);
}

bool ClientAutomationHostService::setVideoFullScreen(bool enabled)
{
    return m_mainWidget && m_mainWidget->agentSetVideoFullScreen(enabled);
}

bool ClientAutomationHostService::setVideoPlaybackRate(double rate)
{
    return m_mainWidget && m_mainWidget->agentSetVideoPlaybackRate(rate);
}

bool ClientAutomationHostService::setVideoQualityPreset(const QString& preset)
{
    return m_mainWidget && m_mainWidget->agentSetVideoQualityPreset(preset);
}

bool ClientAutomationHostService::closeVideoWindow()
{
    return m_mainWidget && m_mainWidget->agentCloseVideoWindow();
}

QVariantMap ClientAutomationHostService::desktopLyricsState() const
{
    return m_mainWidget ? m_mainWidget->agentDesktopLyricsState() : QVariantMap();
}

bool ClientAutomationHostService::setDesktopLyricsVisible(bool visible)
{
    return m_mainWidget && m_mainWidget->agentSetDesktopLyricsVisible(visible);
}

bool ClientAutomationHostService::setDesktopLyricsStyle(const QVariantMap& style)
{
    return m_mainWidget && m_mainWidget->agentSetDesktopLyricsStyle(style);
}

QVariantMap ClientAutomationHostService::uiOverviewSnapshot() const
{
    return m_mainWidget ? m_mainWidget->agentUiOverviewSnapshot() : QVariantMap();
}

QVariantMap ClientAutomationHostService::uiPageStateSnapshot(const QString& pageKey) const
{
    return m_mainWidget ? m_mainWidget->agentUiPageStateSnapshot(pageKey) : QVariantMap();
}

QVariantList ClientAutomationHostService::musicTabItemsSnapshot(const QString& tabKey,
                                                                const QVariantMap& options) const
{
    return m_mainWidget ? m_mainWidget->agentMusicTabItemsSnapshot(tabKey, options) : QVariantList();
}

QVariantMap ClientAutomationHostService::resolveMusicTabItem(const QString& tabKey,
                                                             const QVariantMap& selector) const
{
    return m_mainWidget ? m_mainWidget->agentResolveMusicTabItem(tabKey, selector) : QVariantMap();
}

QVariantMap ClientAutomationHostService::invokeSongAction(const QString& action,
                                                          const QVariantMap& songData)
{
    return m_mainWidget ? m_mainWidget->agentInvokeSongAction(action, songData) : QVariantMap();
}

QVariantMap ClientAutomationHostService::userProfileSnapshot() const
{
    return m_mainWidget ? m_mainWidget->agentUserProfileSnapshot() : QVariantMap();
}

bool ClientAutomationHostService::refreshUserProfile()
{
    return m_mainWidget && m_mainWidget->agentRefreshUserProfile();
}

bool ClientAutomationHostService::updateUsername(const QString& username)
{
    return m_mainWidget && m_mainWidget->agentUpdateUsername(username);
}

bool ClientAutomationHostService::uploadAvatar(const QString& filePath)
{
    return m_mainWidget && m_mainWidget->agentUploadAvatar(filePath);
}

bool ClientAutomationHostService::logoutUser()
{
    return m_mainWidget && m_mainWidget->agentLogoutUser();
}

bool ClientAutomationHostService::returnToWelcome()
{
    return m_mainWidget && m_mainWidget->agentReturnToWelcome();
}

QVariantMap ClientAutomationHostService::pluginsSnapshot() const
{
    QVariantList items;
    const QVector<PluginInfo> plugins = PluginManager::instance().getPluginInfos();
    items.reserve(plugins.size());
    for (const PluginInfo& info : plugins) {
        items.push_back(convertPluginInfo(info));
    }
    return {{QStringLiteral("items"), items},
            {QStringLiteral("count"), items.size()},
            {QStringLiteral("serviceKeys"), PluginManager::instance().hostContext()->serviceKeys()}};
}

QVariantMap ClientAutomationHostService::pluginDiagnosticsSnapshot() const
{
    QVariantList failures;
    for (const PluginLoadFailure& failure : PluginManager::instance().loadFailures()) {
        failures.push_back(QVariantMap{
            {QStringLiteral("pluginId"), failure.pluginId},
            {QStringLiteral("filePath"), failure.filePath},
            {QStringLiteral("reason"), failure.reason},
            {QStringLiteral("timestamp"), failure.timestamp.toString(Qt::ISODate)}});
    }
    return {{QStringLiteral("report"), PluginManager::instance().diagnosticsReport()},
            {QStringLiteral("loadFailures"), failures},
            {QStringLiteral("environment"),
             PluginManager::instance().hostContext()->environmentSnapshot()}};
}

QVariantMap ClientAutomationHostService::reloadPlugins()
{
    PluginManager& manager = PluginManager::instance();
    const QString pluginDir =
        manager.hostContext()->environmentValue(QStringLiteral("pluginDir")).toString().trimmed();
    if (pluginDir.isEmpty()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("code"), QStringLiteral("plugin_dir_missing")}};
    }
    manager.unloadAllPlugins();
    manager.clearLoadFailures();
    const int loaded = manager.loadPlugins(pluginDir);
    QVariantMap snapshot = pluginsSnapshot();
    snapshot.insert(QStringLiteral("ok"), true);
    snapshot.insert(QStringLiteral("pluginDir"), pluginDir);
    snapshot.insert(QStringLiteral("loadedCount"), loaded);
    return snapshot;
}

bool ClientAutomationHostService::unloadPlugin(const QString& pluginKey)
{
    PluginManager::instance().unloadPlugin(pluginKey);
    return true;
}

int ClientAutomationHostService::unloadAllPlugins()
{
    const int count = PluginManager::instance().pluginCount();
    PluginManager::instance().unloadAllPlugins();
    return count;
}

void ClientAutomationHostService::searchMusic(const QString& keyword)
{
    if (m_shellViewModel) {
        m_shellViewModel->searchMusic(keyword);
    }
}

void ClientAutomationHostService::requestHistory(const QString& userAccount, int limit, bool useCache)
{
    if (m_shellViewModel) {
        m_shellViewModel->requestHistory(userAccount, limit, useCache);
    }
}

void ClientAutomationHostService::requestFavorites(const QString& userAccount, bool useCache)
{
    if (m_shellViewModel) {
        m_shellViewModel->requestFavorites(userAccount, useCache);
    }
}

void ClientAutomationHostService::requestPlaylists(const QString& userAccount,
                                                   int page,
                                                   int pageSize,
                                                   bool useCache)
{
    if (m_shellViewModel) {
        m_shellViewModel->requestPlaylists(userAccount, page, pageSize, useCache);
    }
}

void ClientAutomationHostService::requestPlaylistDetail(const QString& userAccount,
                                                        qint64 playlistId,
                                                        bool useCache)
{
    if (m_shellViewModel) {
        m_shellViewModel->requestPlaylistDetail(userAccount, playlistId, useCache);
    }
}

void ClientAutomationHostService::requestRecommendations(const QString& userAccount,
                                                         const QString& scene,
                                                         int limit,
                                                         bool excludePlayed)
{
    if (m_shellViewModel) {
        m_shellViewModel->requestRecommendations(userAccount, scene, limit, excludePlayed);
    }
}

void ClientAutomationHostService::requestSimilarRecommendations(const QString& songId, int limit)
{
    if (m_shellViewModel) {
        m_shellViewModel->requestSimilarRecommendations(songId, limit);
    }
}

void ClientAutomationHostService::submitRecommendationFeedback(const QString& userId,
                                                               const QString& songId,
                                                               const QString& eventType,
                                                               const QString& scene,
                                                               const QString& requestId,
                                                               const QString& modelVersion,
                                                               qint64 playMs,
                                                               qint64 durationMs)
{
    if (m_shellViewModel) {
        m_shellViewModel->submitRecommendationFeedback(
            userId, songId, eventType, scene, requestId, modelVersion, playMs, durationMs);
    }
}

void ClientAutomationHostService::addPlayHistory(const QString& userAccount,
                                                 const QString& path,
                                                 const QString& title,
                                                 const QString& artist,
                                                 const QString& album,
                                                 const QString& duration,
                                                 bool isLocal)
{
    if (m_shellViewModel) {
        m_shellViewModel->addPlayHistory(userAccount, path, title, artist, album, duration, isLocal);
    }
}

void ClientAutomationHostService::removeHistory(const QString& userAccount,
                                                const QStringList& paths)
{
    if (m_shellViewModel) {
        m_shellViewModel->removeHistory(userAccount, paths);
    }
}

void ClientAutomationHostService::addFavorite(const QString& userAccount,
                                              const QString& path,
                                              const QString& title,
                                              const QString& artist,
                                              const QString& duration,
                                              bool isLocal)
{
    if (m_shellViewModel) {
        m_shellViewModel->addFavorite(userAccount, path, title, artist, duration, isLocal);
    }
}

void ClientAutomationHostService::removeFavorite(const QString& userAccount,
                                                 const QStringList& paths)
{
    if (m_shellViewModel) {
        m_shellViewModel->removeFavorite(userAccount, paths);
    }
}

void ClientAutomationHostService::createPlaylist(const QString& userAccount,
                                                 const QString& name,
                                                 const QString& description,
                                                 const QString& coverPath)
{
    if (m_shellViewModel) {
        m_shellViewModel->createPlaylist(userAccount, name, description, coverPath);
    }
}

void ClientAutomationHostService::updatePlaylist(const QString& userAccount,
                                                 qint64 playlistId,
                                                 const QString& name,
                                                 const QString& description,
                                                 const QString& coverPath)
{
    if (m_shellViewModel) {
        m_shellViewModel->updatePlaylist(userAccount, playlistId, name, description, coverPath);
    }
}

void ClientAutomationHostService::deletePlaylist(const QString& userAccount, qint64 playlistId)
{
    if (m_shellViewModel) {
        m_shellViewModel->deletePlaylist(userAccount, playlistId);
    }
}

void ClientAutomationHostService::addPlaylistItems(const QString& userAccount,
                                                   qint64 playlistId,
                                                   const QVariantList& items)
{
    if (m_shellViewModel) {
        m_shellViewModel->addPlaylistItems(userAccount, playlistId, items);
    }
}

void ClientAutomationHostService::removePlaylistItems(const QString& userAccount,
                                                      qint64 playlistId,
                                                      const QStringList& musicPaths)
{
    if (m_shellViewModel) {
        m_shellViewModel->removePlaylistItems(userAccount, playlistId, musicPaths);
    }
}

void ClientAutomationHostService::reorderPlaylistItems(const QString& userAccount,
                                                       qint64 playlistId,
                                                       const QVariantList& orderedItems)
{
    if (m_shellViewModel) {
        m_shellViewModel->reorderPlaylistItems(userAccount, playlistId, orderedItems);
    }
}

void ClientAutomationHostService::requestLyrics(const QString& musicPath)
{
    if (m_shellViewModel && m_shellViewModel->requestGateway()) {
        m_shellViewModel->requestGateway()->getLyrics(musicPath);
    }
}

void ClientAutomationHostService::requestVideoList()
{
    if (m_shellViewModel && m_shellViewModel->requestGateway()) {
        m_shellViewModel->requestGateway()->getVideoList();
    }
}

void ClientAutomationHostService::requestVideoStreamUrl(const QString& videoPath)
{
    if (m_shellViewModel && m_shellViewModel->requestGateway()) {
        m_shellViewModel->requestGateway()->getVideoStreamUrl(videoPath);
    }
}

void ClientAutomationHostService::searchArtist(const QString& artist)
{
    if (m_shellViewModel && m_shellViewModel->requestGateway()) {
        m_shellViewModel->requestGateway()->searchArtist(artist);
    }
}

void ClientAutomationHostService::requestArtistTracks(const QString& artist)
{
    if (m_shellViewModel && m_shellViewModel->requestGateway()) {
        m_shellViewModel->requestGateway()->getMusicByArtist(artist);
    }
}

QVariantList ClientAutomationHostService::convertMusicList(const QList<Music>& musicList,
                                                           int limit) const
{
    QVariantList items;
    const int bounded = (limit <= 0) ? musicList.size() : qMin(limit, musicList.size());
    items.reserve(bounded);
    for (int i = 0; i < bounded; ++i) {
        const Music& music = musicList.at(i);
        const QString path = music.getSongPath();
        const QString title = music.getSongName().trimmed().isEmpty()
            ? QFileInfo(path).completeBaseName()
            : music.getSongName().trimmed();
        const QString artist = music.getSinger().trimmed().isEmpty()
            ? QStringLiteral("未知艺术家")
            : music.getSinger().trimmed();
        QVariantMap item;
        item.insert(QStringLiteral("trackId"), fallbackTrackId(path, title, artist));
        item.insert(QStringLiteral("musicPath"), path);
        item.insert(QStringLiteral("path"), path);
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("artist"), artist);
        item.insert(QStringLiteral("album"), QString());
        item.insert(QStringLiteral("durationMs"),
                    music.getDuration() < 24 * 60 * 60 ? music.getDuration() * 1000
                                                        : music.getDuration());
        item.insert(QStringLiteral("coverUrl"), music.getPicPath());
        item.insert(QStringLiteral("cover"), music.getPicPath());
        item.insert(QStringLiteral("isFavorite"), false);
        item.insert(QStringLiteral("isLocal"),
                    !path.startsWith(QStringLiteral("http"), Qt::CaseInsensitive));
        items.push_back(item);
    }
    return items;
}
