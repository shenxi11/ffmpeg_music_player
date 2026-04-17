#include "MainShellViewModel.h"

#include "AudioPlayer.h"
#include "AudioService.h"
#include "cover_lookup.h"
#include "local_music_cache.h"
#include "online_presence_manager.h"
#include "plugin_manager.h"
#include "settings_manager.h"
#include "user.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString normalizeArtist(const QString& artist) {
    const QString trimmed = artist.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QString lower = trimmed.toLower();
    if (trimmed == QStringLiteral("未知艺术家") || trimmed == QStringLiteral("未知歌手") ||
        lower == QStringLiteral("unknown artist") || lower == QStringLiteral("unknown") ||
        lower == QStringLiteral("<unknown>")) {
        return QString();
    }
    return trimmed;
}

int parseDurationToSeconds(const QString& durationText) {
    const QString trimmed = durationText.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    if (trimmed.contains(':')) {
        const QStringList parts = trimmed.split(':');
        if (parts.size() >= 2) {
            bool okMin = false;
            bool okSec = false;
            const int minutes = parts[0].toInt(&okMin);
            const int seconds = parts[1].toInt(&okSec);
            if (okMin && okSec) {
                return qMax(0, minutes * 60 + seconds);
            }
        }
    }

    bool ok = false;
    const int seconds = trimmed.toInt(&ok);
    return ok ? qMax(0, seconds) : 0;
}

QString normalizeAudioPath(const QUrl& url) {
    if (url.isLocalFile()) {
        return QDir::fromNativeSeparators(url.toLocalFile());
    }
    return url.toString(QUrl::FullyDecoded).trimmed();
}

QString fallbackTrackTitle(const QString& path, const QUrl& url) {
    if (url.isLocalFile()) {
        const QString title = QFileInfo(url.toLocalFile()).completeBaseName().trimmed();
        return title.isEmpty() ? QStringLiteral("未知歌曲") : title;
    }

    QString decodedPath = QUrl::fromPercentEncoding(url.path().toUtf8()).trimmed();
    const int slashIndex = decodedPath.lastIndexOf(QLatin1Char('/'));
    if (slashIndex >= 0) {
        decodedPath = decodedPath.mid(slashIndex + 1);
    }
    const int dotIndex = decodedPath.lastIndexOf(QLatin1Char('.'));
    if (dotIndex > 0) {
        decodedPath = decodedPath.left(dotIndex);
    }

    const QString title = decodedPath.trimmed();
    if (!title.isEmpty()) {
        return title;
    }

    const QString fallback = QFileInfo(path).completeBaseName().trimmed();
    return fallback.isEmpty() ? QStringLiteral("未知歌曲") : fallback;
}

bool isLocalAudioPath(const QString& rawPath) {
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const QUrl url = QUrl::fromUserInput(trimmed);
    if (url.isLocalFile()) {
        return true;
    }

    const QFileInfo fileInfo(trimmed);
    return fileInfo.isAbsolute();
}

QString playModeName(int playMode) {
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

QString pluginStateName(PluginState state) {
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

QVariantMap convertPluginInfo(const PluginInfo& info) {
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

std::optional<LocalMusicInfo> findLocalMusicInfo(const QString& filePath) {
    const QString normalizedTarget = normalizeMusicPathForLookup(filePath);
    if (normalizedTarget.isEmpty()) {
        return std::nullopt;
    }

    for (const LocalMusicInfo& info : LocalMusicCache::instance().getMusicList()) {
        if (normalizeMusicPathForLookup(info.filePath) == normalizedTarget) {
            return info;
        }
    }
    return std::nullopt;
}

} // namespace

MainShellViewModel::MainShellViewModel(QObject* parent) : BaseViewModel(parent), m_request(this) {
    setupConnections();
}

QObject* MainShellViewModel::audioServiceObject() const {
    return &AudioService::instance();
}

QString MainShellViewModel::cachedAccount() const {
    return SettingsManager::instance().cachedAccount();
}

QString MainShellViewModel::cachedPassword() const {
    return SettingsManager::instance().cachedPassword();
}

QString MainShellViewModel::cachedUsername() const {
    return SettingsManager::instance().cachedUsername();
}

QString MainShellViewModel::cachedAvatarUrl() const {
    return SettingsManager::instance().cachedAvatarUrl();
}

QString MainShellViewModel::cachedOnlineSessionToken() const {
    return SettingsManager::instance().cachedOnlineSessionToken();
}

QString MainShellViewModel::cachedProfileCreatedAt() const {
    return SettingsManager::instance().cachedProfileCreatedAt();
}

QString MainShellViewModel::cachedProfileUpdatedAt() const {
    return SettingsManager::instance().cachedProfileUpdatedAt();
}

bool MainShellViewModel::autoLoginEnabled() const {
    return SettingsManager::instance().autoLoginEnabled();
}

bool MainShellViewModel::shouldAutoLogin() const {
    return SettingsManager::instance().shouldAutoLogin();
}

QString MainShellViewModel::currentUserAccount() const {
    return User::getInstance()->getAccount();
}

QString MainShellViewModel::currentUserPassword() const {
    return User::getInstance()->getPassword();
}

QString MainShellViewModel::currentOnlineSessionToken() const {
    const QString onlineToken = OnlinePresenceManager::instance().currentToken().trimmed();
    if (!onlineToken.isEmpty()) {
        return onlineToken;
    }
    return SettingsManager::instance().cachedOnlineSessionToken();
}

bool MainShellViewModel::hasLoggedInUser() const {
    return !currentUserAccount().trimmed().isEmpty();
}

void MainShellViewModel::clearCurrentUserProfile() {
    User::getInstance()->setAccount(QString());
    User::getInstance()->setPassword(QString());
    User::getInstance()->setUsername(QString());
}

void MainShellViewModel::searchMusic(const QString& keyword) {
    m_request.getMusic(keyword);
}

void MainShellViewModel::requestRecommendations(const QString& userAccount, const QString& scene,
                                                int limit, bool excludePlayed) {
    m_request.getAudioRecommendations(userAccount, scene, limit, excludePlayed);
}

void MainShellViewModel::requestSimilarRecommendations(const QString& songId, int limit) {
    m_request.getSimilarRecommendations(songId, limit);
}

void MainShellViewModel::submitRecommendationFeedback(
    const QString& userId, const QString& songId, const QString& eventType, const QString& scene,
    const QString& requestId, const QString& modelVersion, qint64 playMs, qint64 durationMs) {
    m_request.postRecommendationFeedback(userId, songId, eventType, scene, requestId, modelVersion,
                                         playMs, durationMs);
}

void MainShellViewModel::requestHistory(const QString& userAccount, int limit, bool useCache) {
    m_request.getPlayHistory(userAccount, limit, useCache);
}

void MainShellViewModel::addPlayHistory(const QString& userAccount, const QString& path,
                                        const QString& title, const QString& artist,
                                        const QString& album, const QString& duration,
                                        bool isLocal) {
    m_request.addPlayHistory(userAccount, path, title, artist, album, duration, isLocal);
}

void MainShellViewModel::removeHistory(const QString& userAccount, const QStringList& paths) {
    m_request.removePlayHistory(userAccount, paths);
}

void MainShellViewModel::requestFavorites(const QString& userAccount, bool useCache) {
    m_request.getFavorites(userAccount, useCache);
}

void MainShellViewModel::addFavorite(const QString& userAccount, const QString& path,
                                     const QString& title, const QString& artist,
                                     const QString& duration, bool isLocal) {
    m_request.addFavorite(userAccount, path, title, artist, duration, isLocal);
}

void MainShellViewModel::removeFavorite(const QString& userAccount, const QStringList& paths) {
    m_request.removeFavorite(userAccount, paths);
}

void MainShellViewModel::requestPlaylists(const QString& userAccount, int page, int pageSize,
                                          bool useCache) {
    m_request.getPlaylists(userAccount, page, pageSize, useCache);
}

void MainShellViewModel::createPlaylist(const QString& userAccount, const QString& name,
                                        const QString& description, const QString& coverPath) {
    m_request.createPlaylist(userAccount, name, description, coverPath);
}

void MainShellViewModel::requestPlaylistDetail(const QString& userAccount, qint64 playlistId,
                                               bool useCache) {
    m_request.getPlaylistDetail(userAccount, playlistId, useCache);
}

void MainShellViewModel::prefetchPlaylistCoverDetail(const QString& userAccount, qint64 playlistId,
                                                     bool useCache) {
    m_request.getPlaylistDetailForCover(userAccount, playlistId, useCache);
}

void MainShellViewModel::deletePlaylist(const QString& userAccount, qint64 playlistId) {
    m_request.deletePlaylist(userAccount, playlistId);
}

void MainShellViewModel::updatePlaylist(const QString& userAccount, qint64 playlistId,
                                        const QString& name, const QString& description,
                                        const QString& coverPath) {
    m_request.updatePlaylist(userAccount, playlistId, name, description, coverPath);
}

void MainShellViewModel::addPlaylistItems(const QString& userAccount, qint64 playlistId,
                                          const QVariantList& items) {
    m_request.addPlaylistItems(userAccount, playlistId, items);
}

void MainShellViewModel::removePlaylistItems(const QString& userAccount, qint64 playlistId,
                                             const QStringList& musicPaths) {
    m_request.removePlaylistItems(userAccount, playlistId, musicPaths);
}

void MainShellViewModel::reorderPlaylistItems(const QString& userAccount, qint64 playlistId,
                                              const QVariantList& orderedItems) {
    m_request.reorderPlaylistItems(userAccount, playlistId, orderedItems);
}

void MainShellViewModel::requestMusicComments(const QString& musicPath, int page, int pageSize) {
    m_request.getMusicComments(musicPath, page, pageSize);
}

void MainShellViewModel::requestMusicCommentReplies(qint64 rootCommentId, int page, int pageSize) {
    m_request.getMusicCommentReplies(rootCommentId, page, pageSize);
}

void MainShellViewModel::createMusicComment(const QString& musicPath, const QString& musicTitle,
                                            const QString& artist, const QString& content) {
    const QString account = currentUserAccount().trimmed();
    const QString onlineSessionToken = currentOnlineSessionToken().trimmed();
    if (account.isEmpty() || onlineSessionToken.isEmpty()) {
        emit createMusicCommentResultReady(false, QVariantMap(), QStringLiteral("请先登录后再评论"),
                                           401, musicPath);
        return;
    }

    m_request.createMusicComment(account, onlineSessionToken, musicPath, musicTitle, artist,
                                 content);
}

void MainShellViewModel::createMusicCommentReply(qint64 rootCommentId, const QString& content,
                                                 qint64 targetCommentId) {
    const QString account = currentUserAccount().trimmed();
    const QString onlineSessionToken = currentOnlineSessionToken().trimmed();
    if (account.isEmpty() || onlineSessionToken.isEmpty()) {
        emit createMusicCommentReplyResultReady(false, rootCommentId, QVariantMap(),
                                                QStringLiteral("请先登录后再回复"), 401,
                                                targetCommentId);
        return;
    }

    m_request.createMusicCommentReply(rootCommentId, account, onlineSessionToken, content,
                                      targetCommentId);
}

void MainShellViewModel::deleteMusicComment(qint64 commentId) {
    const QString account = currentUserAccount().trimmed();
    const QString onlineSessionToken = currentOnlineSessionToken().trimmed();
    if (account.isEmpty() || onlineSessionToken.isEmpty()) {
        emit deleteMusicCommentResultReady(false, commentId, QStringLiteral("请先登录后再删除评论"),
                                           401);
        return;
    }

    m_request.deleteMusicComment(commentId, account, onlineSessionToken);
}

void MainShellViewModel::handleLoginSuccess(const QString& account, const QString& password,
                                            const QString& username, const QString& avatarUrl,
                                            const QString& onlineSessionToken) {
    OnlinePresenceManager::instance().ensureSessionForUser(account, username);
    SettingsManager::instance().saveAccountCache(account, password, username, true);
    SettingsManager::instance().saveProfileCache(
        username, avatarUrl,
        onlineSessionToken.trimmed().isEmpty() ? OnlinePresenceManager::instance().currentToken()
                                               : onlineSessionToken,
        SettingsManager::instance().cachedProfileCreatedAt(),
        SettingsManager::instance().cachedProfileUpdatedAt());
    emit accountCacheChanged();
}

void MainShellViewModel::requestUserProfile() {
    const QString account =
        currentUserAccount().trimmed().isEmpty() ? cachedAccount() : currentUserAccount();
    const QString onlineSessionToken = currentOnlineSessionToken();
    m_request.getUserProfile(account, onlineSessionToken);
}

void MainShellViewModel::updateCurrentUsername(const QString& username) {
    const QString account =
        currentUserAccount().trimmed().isEmpty() ? cachedAccount() : currentUserAccount();
    const QString onlineSessionToken = currentOnlineSessionToken();
    m_request.updateUsername(account, onlineSessionToken, username);
}

void MainShellViewModel::uploadCurrentAvatar(const QString& filePath) {
    const QString account =
        currentUserAccount().trimmed().isEmpty() ? cachedAccount() : currentUserAccount();
    const QString onlineSessionToken = currentOnlineSessionToken();
    m_request.uploadAvatar(account, onlineSessionToken, filePath);
}

void MainShellViewModel::logoutCurrentUser(bool graceful, int gracefulTimeoutMs) {
    SettingsManager::instance().setManualLogoutMarked(true);
    SettingsManager::instance().setAutoLoginEnabled(false);
    OnlinePresenceManager::instance().logoutAndClear(graceful, gracefulTimeoutMs);
    clearCurrentUserProfile();
    emit accountCacheChanged();
}

void MainShellViewModel::returnToWelcomeAndKeepAccountCache(bool graceful, int gracefulTimeoutMs) {
    // 返回欢迎页时保留账号缓存与自动登录策略，仅收敛当前在线会话。
    OnlinePresenceManager::instance().logoutAndClear(graceful, gracefulTimeoutMs);
    clearCurrentUserProfile();
    emit accountCacheChanged();
}

void MainShellViewModel::shutdownUserSessionOnAppExit(bool graceful, int gracefulTimeoutMs) {
    // 应用正常关闭时仅释放在线会话，不改自动登录缓存状态。
    OnlinePresenceManager::instance().logoutAndClear(graceful, gracefulTimeoutMs);
    clearCurrentUserProfile();
}

bool MainShellViewModel::pauseAudioIfPlaying() {
    AudioService& service = AudioService::instance();
    if (service.isPlaying()) {
        service.pause();
    }
    return true;
}

bool MainShellViewModel::resumeAudioIfPaused() {
    AudioService::instance().resume();
    return true;
}

bool MainShellViewModel::stopAudio() {
    AudioService::instance().stop();
    return true;
}

bool MainShellViewModel::seekAudio(qint64 positionMs) {
    AudioService::instance().seekTo(qMax<qint64>(0, positionMs));
    return true;
}

bool MainShellViewModel::playNextAudio() {
    AudioService::instance().playNext();
    return true;
}

bool MainShellViewModel::playPreviousAudio() {
    AudioService::instance().playPrevious();
    return true;
}

bool MainShellViewModel::playAudioAtIndex(int index) {
    AudioService::instance().playAtIndex(index);
    return true;
}

bool MainShellViewModel::setAudioVolume(int volume) {
    AudioService::instance().setVolume(qBound(0, volume, 100));
    return true;
}

bool MainShellViewModel::setAudioPlayMode(int mode) {
    if (mode < AudioService::Sequential || mode > AudioService::Shuffle) {
        return false;
    }

    AudioService::instance().setPlayMode(static_cast<AudioService::PlayMode>(mode));
    return true;
}

bool MainShellViewModel::setAudioPlaylist(const QList<QUrl>& urls, bool playNow, int startIndex) {
    if (urls.isEmpty()) {
        return false;
    }

    AudioService& service = AudioService::instance();
    service.setPlaylist(urls);
    if (playNow) {
        service.playAtIndex(qBound(0, startIndex, urls.size() - 1));
    }
    return true;
}

bool MainShellViewModel::appendAudioToQueue(const QUrl& url) {
    if (!url.isValid() || url.isEmpty()) {
        return false;
    }

    AudioService::instance().addToPlaylist(url);
    return true;
}

bool MainShellViewModel::removeAudioFromQueue(int index) {
    AudioService& service = AudioService::instance();
    if (index < 0 || index >= service.playlistSize()) {
        return false;
    }

    service.removeFromPlaylist(index);
    return true;
}

bool MainShellViewModel::clearAudioQueue() {
    AudioService::instance().clearPlaylist();
    return true;
}

bool MainShellViewModel::playAudioUrl(const QUrl& url) {
    return url.isValid() && !url.isEmpty() && AudioService::instance().play(url);
}

bool MainShellViewModel::playAudioPlaylist(const QList<QUrl>& urls) {
    if (urls.isEmpty()) {
        return false;
    }

    AudioService& service = AudioService::instance();
    service.setPlaylist(urls);
    service.playPlaylist();
    return true;
}

QVariantMap MainShellViewModel::audioCurrentTrackSnapshot() const {
    QVariantMap track;
    AudioService& service = AudioService::instance();
    const QUrl currentUrl = service.currentUrl();
    const QString filePath = normalizeAudioPath(currentUrl);
    if (filePath.isEmpty()) {
        track.insert(QStringLiteral("playing"), false);
        return track;
    }

    AudioSession* session = service.currentSession();
    std::optional<LocalMusicInfo> localInfo = findLocalMusicInfo(filePath);

    QString title = session ? session->title().trimmed() : QString();
    if (title.isEmpty()) {
        title = localInfo && !localInfo->fileName.trimmed().isEmpty()
            ? localInfo->fileName.trimmed()
            : fallbackTrackTitle(filePath, currentUrl);
    }

    QString artist = session ? normalizeArtist(session->artist()) : QString();
    if (artist.isEmpty() && localInfo && !localInfo->artist.trimmed().isEmpty()) {
        artist = localInfo->artist.trimmed();
    }
    if (artist.isEmpty()) {
        artist = QStringLiteral("未知艺术家");
    }

    QString coverUrl = session ? session->albumArt().trimmed() : QString();
    if (coverUrl.isEmpty() && localInfo && !localInfo->coverUrl.trimmed().isEmpty()) {
        coverUrl = localInfo->coverUrl.trimmed();
    }
    if (coverUrl.isEmpty()) {
        coverUrl = queryBestCoverForTrack(filePath, title, artist).trimmed();
    }

    qint64 durationMs = session ? session->duration() : 0;
    if (durationMs <= 0 && localInfo) {
        durationMs = static_cast<qint64>(parseDurationToSeconds(localInfo->duration)) * 1000;
    }

    track.insert(QStringLiteral("url"), currentUrl.toString());
    track.insert(QStringLiteral("filePath"), filePath);
    track.insert(QStringLiteral("musicPath"), filePath);
    track.insert(QStringLiteral("title"), title);
    track.insert(QStringLiteral("artist"), artist);
    track.insert(QStringLiteral("album"), QString());
    track.insert(QStringLiteral("coverUrl"), coverUrl);
    track.insert(QStringLiteral("albumArt"), coverUrl);
    track.insert(QStringLiteral("durationMs"), qMax<qint64>(0, durationMs));
    track.insert(QStringLiteral("positionMs"),
                 qMax<qint64>(0, session ? session->position() : 0));
    track.insert(QStringLiteral("playing"), service.isPlaying());
    track.insert(QStringLiteral("paused"), service.isPaused());
    track.insert(QStringLiteral("isLocal"), isLocalAudioPath(filePath));
    return track;
}

QVariantMap MainShellViewModel::audioQueueSnapshot() const {
    AudioService& service = AudioService::instance();
    const QList<QUrl> playlist = service.playlist();
    const int currentIndex = service.currentIndex();
    const QVariantMap currentTrack = audioCurrentTrackSnapshot();
    const QString currentPath = currentTrack.value(QStringLiteral("musicPath")).toString();

    QVariantList items;
    items.reserve(playlist.size());
    for (int index = 0; index < playlist.size(); ++index) {
        const QUrl& url = playlist.at(index);
        const QString filePath = normalizeAudioPath(url);
        const bool isCurrent = index == currentIndex;
        const bool isLocal = isLocalAudioPath(filePath);
        const std::optional<LocalMusicInfo> localInfo = findLocalMusicInfo(filePath);

        QString title = isCurrent ? currentTrack.value(QStringLiteral("title")).toString().trimmed()
                                  : QString();
        if (title.isEmpty()) {
            title = localInfo && !localInfo->fileName.trimmed().isEmpty()
                ? localInfo->fileName.trimmed()
                : fallbackTrackTitle(filePath, url);
        }

        QString artist =
            isCurrent ? currentTrack.value(QStringLiteral("artist")).toString().trimmed() : QString();
        if (artist.isEmpty() && localInfo && !localInfo->artist.trimmed().isEmpty()) {
            artist = localInfo->artist.trimmed();
        }
        if (artist.isEmpty()) {
            artist = QStringLiteral("未知艺术家");
        }

        QString coverUrl =
            isCurrent ? currentTrack.value(QStringLiteral("coverUrl")).toString().trimmed() : QString();
        if (coverUrl.isEmpty() && localInfo && !localInfo->coverUrl.trimmed().isEmpty()) {
            coverUrl = localInfo->coverUrl.trimmed();
        }
        if (coverUrl.isEmpty()) {
            coverUrl = queryBestCoverForTrack(filePath, title, artist).trimmed();
        }

        qint64 durationMs = 0;
        if (isCurrent && filePath == currentPath) {
            durationMs = currentTrack.value(QStringLiteral("durationMs")).toLongLong();
        }
        if (durationMs <= 0 && localInfo) {
            durationMs = static_cast<qint64>(parseDurationToSeconds(localInfo->duration)) * 1000;
        }

        QVariantMap item;
        item.insert(QStringLiteral("musicPath"), filePath);
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("artist"), artist);
        item.insert(QStringLiteral("coverUrl"), coverUrl);
        item.insert(QStringLiteral("isCurrent"), isCurrent);
        item.insert(QStringLiteral("isLocal"), isLocal);
        item.insert(QStringLiteral("index"), index);
        item.insert(QStringLiteral("durationMs"), qMax<qint64>(0, durationMs));
        items.push_back(item);
    }

    QVariantMap snapshot = audioStateSnapshot();
    snapshot.insert(QStringLiteral("items"), items);
    snapshot.insert(QStringLiteral("count"), items.size());
    return snapshot;
}

QVariantMap MainShellViewModel::audioStateSnapshot() const {
    AudioService& service = AudioService::instance();
    const int playMode = static_cast<int>(service.playMode());
    return {{QStringLiteral("currentIndex"), service.currentIndex()},
            {QStringLiteral("playing"), service.isPlaying()},
            {QStringLiteral("paused"), service.isPaused()},
            {QStringLiteral("volume"), service.volume()},
            {QStringLiteral("playMode"), playMode},
            {QStringLiteral("playModeName"), playModeName(playMode)}};
}

void MainShellViewModel::shutdownAudioPipeline() {
    AudioService::instance().stop();
    AudioPlayer::instance().stop();
    AudioPlayer::instance().clearWriteOwner();
    AudioPlayer::instance().resetBuffer();
}

int MainShellViewModel::loadPlugins(const QString& pluginPath) {
    return PluginManager::instance().loadPlugins(pluginPath);
}

QVector<PluginLoadFailure> MainShellViewModel::pluginLoadFailures() const {
    return PluginManager::instance().loadFailures();
}

QVector<PluginInfo> MainShellViewModel::pluginInfos() const {
    return PluginManager::instance().getPluginInfos();
}

PluginInterface* MainShellViewModel::pluginById(const QString& pluginId) const {
    return PluginManager::instance().getPlugin(pluginId);
}

QString MainShellViewModel::pluginDiagnosticsReport() const {
    return PluginManager::instance().diagnosticsReport();
}

QVariantMap MainShellViewModel::pluginsSnapshot() const {
    QVariantList items;
    const QVector<PluginInfo> plugins = pluginInfos();
    items.reserve(plugins.size());
    for (const PluginInfo& info : plugins) {
        items.push_back(convertPluginInfo(info));
    }

    return {{QStringLiteral("items"), items},
            {QStringLiteral("count"), items.size()},
            {QStringLiteral("serviceKeys"), PluginManager::instance().hostContext()->serviceKeys()}};
}

QVariantMap MainShellViewModel::pluginDiagnosticsSnapshot() const {
    QVariantList failures;
    const QVector<PluginLoadFailure> loadFailureList = pluginLoadFailures();
    failures.reserve(loadFailureList.size());
    for (const PluginLoadFailure& failure : loadFailureList) {
        failures.push_back(QVariantMap{
            {QStringLiteral("pluginId"), failure.pluginId},
            {QStringLiteral("filePath"), failure.filePath},
            {QStringLiteral("reason"), failure.reason},
            {QStringLiteral("timestamp"), failure.timestamp.toString(Qt::ISODate)}});
    }

    return {{QStringLiteral("report"), pluginDiagnosticsReport()},
            {QStringLiteral("loadFailures"), failures},
            {QStringLiteral("environment"),
             PluginManager::instance().hostContext()->environmentSnapshot()}};
}

QVariantMap MainShellViewModel::reloadPluginsSnapshot() {
    PluginManager& manager = PluginManager::instance();
    const QString pluginDir =
        manager.hostContext()->environmentValue(QStringLiteral("pluginDir")).toString().trimmed();
    if (pluginDir.isEmpty()) {
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("code"), QStringLiteral("plugin_dir_missing")}};
    }

    manager.unloadAllPlugins();
    manager.clearLoadFailures();
    const int loadedCount = manager.loadPlugins(pluginDir);

    QVariantMap snapshot = pluginsSnapshot();
    snapshot.insert(QStringLiteral("ok"), true);
    snapshot.insert(QStringLiteral("pluginDir"), pluginDir);
    snapshot.insert(QStringLiteral("loadedCount"), loadedCount);
    return snapshot;
}

bool MainShellViewModel::unloadPluginByKey(const QString& pluginKey) {
    PluginManager::instance().unloadPlugin(pluginKey);
    return true;
}

int MainShellViewModel::unloadAllPluginsAndReturnCount() {
    PluginManager& manager = PluginManager::instance();
    const int count = manager.pluginCount();
    manager.unloadAllPlugins();
    return count;
}

void MainShellViewModel::registerPluginHostService(const QString& serviceName, QObject* service) {
    PluginManager::instance().hostContext()->registerService(serviceName, service);
}

void MainShellViewModel::unloadAllPlugins() {
    PluginManager::instance().unloadAllPlugins();
}

QVariantMap MainShellViewModel::settingsSnapshot() const {
    SettingsManager& settings = SettingsManager::instance();
    return {{QStringLiteral("downloadPath"), settings.downloadPath()},
            {QStringLiteral("downloadLyrics"), settings.downloadLyrics()},
            {QStringLiteral("downloadCover"), settings.downloadCover()},
            {QStringLiteral("audioCachePath"), settings.audioCachePath()},
            {QStringLiteral("logPath"), settings.logPath()},
            {QStringLiteral("serverHost"), settings.serverHost()},
            {QStringLiteral("serverPort"), settings.serverPort()},
            {QStringLiteral("serverBaseUrl"), settings.serverBaseUrl()},
            {QStringLiteral("playerPageStyle"), settings.playerPageStyle()}};
}

bool MainShellViewModel::updateSettingValue(const QString& key, const QVariant& value) {
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

void MainShellViewModel::addLocalMusicCacheEntry(const QString& filePath, const QString& fileName) {
    LocalMusicInfo info;
    info.filePath = filePath;
    info.fileName = fileName;
    LocalMusicCache::instance().addMusic(info);
}

void MainShellViewModel::updateLocalMusicCacheMetadata(const QString& filePath,
                                                       const QString& coverUrl,
                                                       const QString& duration,
                                                       const QString& artist) {
    LocalMusicCache::instance().updateMetadata(filePath, coverUrl, duration, artist);
}

void MainShellViewModel::removeLocalMusicCacheEntry(const QString& filePath) {
    LocalMusicCache::instance().removeMusic(filePath);
}

int MainShellViewModel::localMusicCacheDurationSeconds(const QString& filePath) const {
    for (const LocalMusicInfo& info : LocalMusicCache::instance().getMusicList()) {
        if (info.filePath == filePath) {
            return parseDurationToSeconds(info.duration);
        }
    }
    return 0;
}

bool MainShellViewModel::resolveHistorySnapshot(const QString& sessionId, const QString& filePath,
                                                const QString& fallbackArtist, QString* title,
                                                QString* artist, QString* album, qint64* durationMs,
                                                bool* isLocal) const {
    AudioSession* session = AudioService::instance().getSession(sessionId);
    if (!session) {
        session = AudioService::instance().currentSession();
    }
    if (!session) {
        return false;
    }

    QString resolvedTitle = session->title();
    if (resolvedTitle.trimmed().isEmpty()) {
        resolvedTitle = QFileInfo(filePath).completeBaseName();
    }

    QString resolvedArtist = normalizeArtist(session->artist());
    if (resolvedArtist.isEmpty()) {
        resolvedArtist = normalizeArtist(fallbackArtist);
    }
    if (resolvedArtist.isEmpty()) {
        resolvedArtist = QStringLiteral("未知艺术家");
    }

    if (title) {
        *title = resolvedTitle;
    }
    if (artist) {
        *artist = resolvedArtist;
    }
    if (album) {
        *album = QString();
    }
    if (durationMs) {
        *durationMs = session->duration();
    }
    if (isLocal) {
        *isLocal = !filePath.startsWith(QStringLiteral("http"), Qt::CaseInsensitive);
    }

    return true;
}

void MainShellViewModel::onSessionExpired() {
    emit sessionExpired();
}
