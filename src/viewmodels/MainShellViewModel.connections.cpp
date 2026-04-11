#include "MainShellViewModel.h"

#include "AudioService.h"
#include "online_presence_manager.h"
#include "plugin_manager.h"
#include "settings_manager.h"
#include "user.h"

/*
模块名称: MainShellViewModel 连接配置
功能概述: 统一管理请求网关、在线状态、音频服务与插件管理器的信号绑定。
对外接口: MainShellViewModel::setupConnections()
维护说明: 连接关系集中维护，避免构造函数过长。
*/

void MainShellViewModel::setupConnections() {
    connect(&m_request, &HttpRequestV2::signalAddSongList, this,
            &MainShellViewModel::searchResultsReady);
    connect(&m_request, &HttpRequestV2::signalRecommendationList, this,
            &MainShellViewModel::recommendationListReady);
    connect(&m_request, &HttpRequestV2::signalSimilarRecommendationList, this,
            &MainShellViewModel::similarRecommendationListReady);
    connect(&m_request, &HttpRequestV2::signalRecommendationFeedbackResult, this,
            &MainShellViewModel::recommendationFeedbackResultReady);
    connect(&m_request, &HttpRequestV2::signalHistoryList, this,
            &MainShellViewModel::historyListReady);
    connect(&m_request, &HttpRequestV2::signalAddHistoryResult, this,
            &MainShellViewModel::addHistoryResultReady);
    connect(&m_request, &HttpRequestV2::signalFavoritesList, this,
            &MainShellViewModel::favoritesListReady);
    connect(&m_request, &HttpRequestV2::signalAddFavoriteResult, this,
            &MainShellViewModel::addFavoriteResultReady);
    connect(&m_request, &HttpRequestV2::signalRemoveFavoriteResult, this,
            &MainShellViewModel::removeFavoriteResultReady);
    connect(&m_request, &HttpRequestV2::signalRemoveHistoryResult, this,
            &MainShellViewModel::removeHistoryResultReady);
    connect(&m_request, &HttpRequestV2::signalPlaylistsList, this,
            &MainShellViewModel::playlistsListReady);
    connect(&m_request, &HttpRequestV2::signalPlaylistDetail, this,
            &MainShellViewModel::playlistDetailReady);
    connect(&m_request, &HttpRequestV2::signalPlaylistCoverDetail, this,
            &MainShellViewModel::playlistCoverDetailReady);
    connect(&m_request, &HttpRequestV2::signalCreatePlaylistResult, this,
            &MainShellViewModel::createPlaylistResultReady);
    connect(&m_request, &HttpRequestV2::signalDeletePlaylistResult, this,
            &MainShellViewModel::deletePlaylistResultReady);
    connect(&m_request, &HttpRequestV2::signalUpdatePlaylistResult, this,
            &MainShellViewModel::updatePlaylistResultReady);
    connect(&m_request, &HttpRequestV2::signalAddPlaylistItemsResult, this,
            &MainShellViewModel::addPlaylistItemsResultReady);
    connect(&m_request, &HttpRequestV2::signalRemovePlaylistItemsResult, this,
            &MainShellViewModel::removePlaylistItemsResultReady);
    connect(&m_request, &HttpRequestV2::signalReorderPlaylistItemsResult, this,
            &MainShellViewModel::reorderPlaylistItemsResultReady);
    connect(
        &m_request, &HttpRequestV2::signalUserProfileResult, this,
        [this](bool success, const QVariantMap& profile, const QString& message, int statusCode) {
            if (!success) {
                if (statusCode == 401) {
                    emit sessionExpired();
                }
                emit userProfileRequestFailed(message, statusCode);
                return;
            }

            const QString username = profile.value(QStringLiteral("username")).toString();
            const QString avatarUrl = profile.value(QStringLiteral("avatar_url")).toString();
            const QString createdAt = profile.value(QStringLiteral("created_at")).toString();
            const QString updatedAt = profile.value(QStringLiteral("updated_at")).toString();
            const QString onlineToken = currentOnlineSessionToken();
            const QString persistedUsername = username.trimmed().isEmpty()
                                                  ? SettingsManager::instance().cachedUsername()
                                                  : username;

            if (!username.trimmed().isEmpty()) {
                User::getInstance()->setUsername(username);
                OnlinePresenceManager::instance().updateCurrentUsername(username);
            }
            SettingsManager::instance().saveProfileCache(persistedUsername, avatarUrl, onlineToken,
                                                         createdAt, updatedAt);
            emit userProfileReady(profile);
        });
    connect(&m_request, &HttpRequestV2::signalUpdateUsernameResult, this,
            [this](bool success, const QString& username, const QString& message, int statusCode) {
                if (success && !username.trimmed().isEmpty()) {
                    User::getInstance()->setUsername(username);
                    OnlinePresenceManager::instance().updateCurrentUsername(username);
                    SettingsManager::instance().saveProfileCache(
                        username, SettingsManager::instance().cachedAvatarUrl(),
                        currentOnlineSessionToken(),
                        SettingsManager::instance().cachedProfileCreatedAt(),
                        SettingsManager::instance().cachedProfileUpdatedAt());
                } else if (statusCode == 401) {
                    emit sessionExpired();
                }
                emit updateUsernameResultReady(success, username, message, statusCode);
            });
    connect(&m_request, &HttpRequestV2::signalUploadAvatarResult, this,
            [this](bool success, const QString& avatarUrl, const QString& message, int statusCode) {
                if (success) {
                    SettingsManager::instance().saveProfileCache(
                        SettingsManager::instance().cachedUsername(), avatarUrl,
                        currentOnlineSessionToken(),
                        SettingsManager::instance().cachedProfileCreatedAt(),
                        SettingsManager::instance().cachedProfileUpdatedAt());
                } else if (statusCode == 401) {
                    emit sessionExpired();
                }
                emit uploadAvatarResultReady(success, avatarUrl, message, statusCode);
            });

    connect(&OnlinePresenceManager::instance(), &OnlinePresenceManager::sessionExpired, this,
            &MainShellViewModel::onSessionExpired);

    connect(&AudioService::instance(), &AudioService::playbackStarted, this,
            &MainShellViewModel::audioPlaybackStarted);
    connect(&AudioService::instance(), &AudioService::playbackPaused, this,
            &MainShellViewModel::audioPlaybackPaused);
    connect(&AudioService::instance(), &AudioService::playbackResumed, this,
            &MainShellViewModel::audioPlaybackResumed);
    connect(&AudioService::instance(), &AudioService::playbackStopped, this,
            &MainShellViewModel::audioPlaybackStopped);

    connect(&PluginManager::instance(), &PluginManager::pluginLoadFailed, this,
            &MainShellViewModel::pluginLoadFailed);
}
