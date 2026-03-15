#include "MainShellViewModel.h"

#include "AudioService.h"
#include "online_presence_manager.h"
#include "plugin_manager.h"

/*
模块名称: MainShellViewModel 连接配置
功能概述: 统一管理请求网关、在线状态、音频服务与插件管理器的信号绑定。
对外接口: MainShellViewModel::setupConnections()
维护说明: 连接关系集中维护，避免构造函数过长。
*/

void MainShellViewModel::setupConnections()
{
    connect(&m_request, &HttpRequestV2::signalAddSongList,
            this, &MainShellViewModel::searchResultsReady);
    connect(&m_request, &HttpRequestV2::signalRecommendationList,
            this, &MainShellViewModel::recommendationListReady);
    connect(&m_request, &HttpRequestV2::signalSimilarRecommendationList,
            this, &MainShellViewModel::similarRecommendationListReady);
    connect(&m_request, &HttpRequestV2::signalHistoryList,
            this, &MainShellViewModel::historyListReady);
    connect(&m_request, &HttpRequestV2::signalFavoritesList,
            this, &MainShellViewModel::favoritesListReady);
    connect(&m_request, &HttpRequestV2::signalAddFavoriteResult,
            this, &MainShellViewModel::addFavoriteResultReady);
    connect(&m_request, &HttpRequestV2::signalRemoveFavoriteResult,
            this, &MainShellViewModel::removeFavoriteResultReady);
    connect(&m_request, &HttpRequestV2::signalRemoveHistoryResult,
            this, &MainShellViewModel::removeHistoryResultReady);

    connect(&OnlinePresenceManager::instance(),
            &OnlinePresenceManager::sessionExpired,
            this,
            &MainShellViewModel::onSessionExpired);

    connect(&AudioService::instance(),
            &AudioService::playbackStarted,
            this,
            &MainShellViewModel::audioPlaybackStarted);
    connect(&AudioService::instance(),
            &AudioService::playbackPaused,
            this,
            &MainShellViewModel::audioPlaybackPaused);
    connect(&AudioService::instance(),
            &AudioService::playbackResumed,
            this,
            &MainShellViewModel::audioPlaybackResumed);
    connect(&AudioService::instance(),
            &AudioService::playbackStopped,
            this,
            &MainShellViewModel::audioPlaybackStopped);

    connect(&PluginManager::instance(),
            &PluginManager::pluginLoadFailed,
            this,
            &MainShellViewModel::pluginLoadFailed);
}
