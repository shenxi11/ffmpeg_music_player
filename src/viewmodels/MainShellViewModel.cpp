#include "MainShellViewModel.h"

#include "AudioPlayer.h"
#include "AudioService.h"
#include "local_music_cache.h"
#include "online_presence_manager.h"
#include "plugin_manager.h"
#include "settings_manager.h"
#include "user.h"

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

void MainShellViewModel::pauseAudioIfPlaying() {
    if (AudioService::instance().isPlaying()) {
        AudioService::instance().pause();
    }
}

void MainShellViewModel::stopAudio() {
    AudioService::instance().stop();
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

void MainShellViewModel::registerPluginHostService(const QString& serviceName, QObject* service) {
    PluginManager::instance().hostContext()->registerService(serviceName, service);
}

void MainShellViewModel::unloadAllPlugins() {
    PluginManager::instance().unloadAllPlugins();
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
