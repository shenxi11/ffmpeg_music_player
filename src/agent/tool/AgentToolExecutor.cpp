#include "AgentToolExecutor.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>
#include <type_traits>

#include "HostStateProvider.h"
#include "settings_manager.h"
#include "ToolRegistry.h"

namespace {
QString sid(const QString& s)
{
    return QString::fromLatin1(QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Md5).toHex());
}

class HostServiceProxy
{
public:
    explicit HostServiceProxy(QObject* object = nullptr)
        : m_object(object)
    {
    }

    HostServiceProxy* operator->() { return this; }
    const HostServiceProxy* operator->() const { return this; }
    explicit operator bool() const { return m_object != nullptr; }

    template <typename Return, typename... Args>
    Return call(const char* method, const Return& fallback, Args&&... args) const
    {
        if (!m_object) {
            return fallback;
        }

        Return result = fallback;
        const bool ok = QMetaObject::invokeMethod(m_object,
                                                  method,
                                                  Qt::DirectConnection,
                                                  Q_RETURN_ARG(Return, result),
                                                  Q_ARG(std::decay_t<Args>, std::forward<Args>(args))...);
        return ok ? result : fallback;
    }

    template <typename... Args>
    bool callVoid(const char* method, Args&&... args) const
    {
        if (!m_object) {
            return false;
        }

        return QMetaObject::invokeMethod(m_object,
                                         method,
                                         Qt::DirectConnection,
                                         Q_ARG(std::decay_t<Args>, std::forward<Args>(args))...);
    }

    QVariantMap uiOverviewSnapshot() const { return call<QVariantMap>("uiOverviewSnapshot", {}); }
    QVariantMap uiPageStateSnapshot(const QString& page) const { return call<QVariantMap>("uiPageStateSnapshot", {}, page); }
    QVariantList musicTabItemsSnapshot(const QString& tab, const QVariantMap& options) const { return call<QVariantList>("musicTabItemsSnapshot", {}, tab, options); }
    QVariantMap resolveMusicTabItem(const QString& tab, const QVariantMap& selector) const { return call<QVariantMap>("resolveMusicTabItem", {}, tab, selector); }
    QVariantMap invokeSongAction(const QString& action, const QVariantMap& item) const { return call<QVariantMap>("invokeSongAction", {}, action, item); }
    QVariantMap userProfileSnapshot() const { return call<QVariantMap>("userProfileSnapshot", {}); }
    bool refreshUserProfile() const { return call<bool>("refreshUserProfile", false); }
    bool updateUsername(const QString& username) const { return call<bool>("updateUsername", false, username); }
    bool uploadAvatar(const QString& filePath) const { return call<bool>("uploadAvatar", false, filePath); }
    bool logoutUser() const { return call<bool>("logoutUser", false); }
    bool returnToWelcome() const { return call<bool>("returnToWelcome", false); }
    bool pausePlayback() const { return call<bool>("pausePlayback", false); }
    bool resumePlayback() const { return call<bool>("resumePlayback", false); }
    bool stopPlayback() const { return call<bool>("stopPlayback", false); }
    bool seekPlayback(qint64 positionMs) const { return call<bool>("seekPlayback", false, positionMs); }
    bool playNext() const { return call<bool>("playNext", false); }
    bool playPrevious() const { return call<bool>("playPrevious", false); }
    bool playAtIndex(int index) const { return call<bool>("playAtIndex", false, index); }
    bool setVolume(int volume) const { return call<bool>("setVolume", false, volume); }
    bool setPlayMode(int mode) const { return call<bool>("setPlayMode", false, mode); }
    bool setPlaybackQueue(const QVariantList& items, int startIndex, bool playNow) const { return call<bool>("setPlaybackQueue", false, items, startIndex, playNow); }
    bool addToPlaybackQueue(const QString& musicPath) const { return call<bool>("addToPlaybackQueue", false, musicPath); }
    bool removeFromPlaybackQueue(int index) const { return call<bool>("removeFromPlaybackQueue", false, index); }
    bool clearPlaybackQueue() const { return call<bool>("clearPlaybackQueue", false); }
    bool playTrack(const QString& musicPath) const { return call<bool>("playTrack", false, musicPath); }
    bool playPlaylist(const QVariantList& items) const { return call<bool>("playPlaylist", false, items); }
    bool addLocalTrack(const QVariantMap& track) const { return call<bool>("addLocalTrack", false, track); }
    bool removeLocalTrack(const QString& filePath) const { return call<bool>("removeLocalTrack", false, filePath); }
    QVariantMap downloadTasksSnapshot(const QString& scope) const { return call<QVariantMap>("downloadTasksSnapshot", {}, scope); }
    QVariantMap downloadTaskSnapshot(const QString& taskId) const { return call<QVariantMap>("downloadTaskSnapshot", {}, taskId); }
    QVariantMap startDownloadTrack(const QString& relativePath, const QString& coverUrl) const { return call<QVariantMap>("startDownloadTrack", {}, relativePath, coverUrl); }
    bool pauseDownloadTask(const QString& taskId) const { return call<bool>("pauseDownloadTask", false, taskId); }
    bool resumeDownloadTask(const QString& taskId) const { return call<bool>("resumeDownloadTask", false, taskId); }
    bool cancelDownloadTask(const QString& taskId) const { return call<bool>("cancelDownloadTask", false, taskId); }
    QVariantMap removeDownloadTask(const QString& taskId) const { return call<QVariantMap>("removeDownloadTask", {}, taskId); }
    QVariantMap videoWindowState() const { return call<QVariantMap>("videoWindowState", {}); }
    bool playVideo(const QString& source) const { return call<bool>("playVideo", false, source); }
    bool pauseVideo() const { return call<bool>("pauseVideo", false); }
    bool resumeVideo() const { return call<bool>("resumeVideo", false); }
    bool closeVideoWindow() const { return call<bool>("closeVideoWindow", false); }
    bool seekVideo(qint64 positionMs) const { return call<bool>("seekVideo", false, positionMs); }
    bool setVideoFullScreen(bool enabled) const { return call<bool>("setVideoFullScreen", false, enabled); }
    bool setVideoPlaybackRate(double rate) const { return call<bool>("setVideoPlaybackRate", false, rate); }
    bool setVideoQualityPreset(const QString& preset) const { return call<bool>("setVideoQualityPreset", false, preset); }
    QVariantMap desktopLyricsState() const { return call<QVariantMap>("desktopLyricsState", {}); }
    bool setDesktopLyricsVisible(bool visible) const { return call<bool>("setDesktopLyricsVisible", false, visible); }
    bool setDesktopLyricsStyle(const QVariantMap& style) const { return call<bool>("setDesktopLyricsStyle", false, style); }
    QVariantMap pluginsSnapshot() const { return call<QVariantMap>("pluginsSnapshot", {}); }
    QVariantMap pluginDiagnosticsSnapshot() const { return call<QVariantMap>("pluginDiagnosticsSnapshot", {}); }
    QVariantMap reloadPlugins() const { return call<QVariantMap>("reloadPlugins", {}); }
    bool unloadPlugin(const QString& pluginKey) const { return call<bool>("unloadPlugin", false, pluginKey); }
    int unloadAllPlugins() const { return call<int>("unloadAllPlugins", 0); }
    QVariantMap playbackQueueSnapshot() const { return call<QVariantMap>("playbackQueueSnapshot", {}); }

    void requestLyrics(const QString& musicPath) const { callVoid("requestLyrics", musicPath); }
    void requestVideoList() const { callVoid("requestVideoList"); }
    void requestVideoStreamUrl(const QString& videoPath) const { callVoid("requestVideoStreamUrl", videoPath); }
    void searchArtist(const QString& artist) const { callVoid("searchArtist", artist); }
    void requestArtistTracks(const QString& artist) const { callVoid("requestArtistTracks", artist); }
    void searchMusic(const QString& keyword) const { callVoid("searchMusic", keyword); }
    void requestHistory(const QString& userAccount, int limit, bool useCache) const { callVoid("requestHistory", userAccount, limit, useCache); }
    void requestFavorites(const QString& userAccount, bool useCache) const { callVoid("requestFavorites", userAccount, useCache); }
    void requestPlaylists(const QString& userAccount, int page, int pageSize, bool useCache) const { callVoid("requestPlaylists", userAccount, page, pageSize, useCache); }
    void requestPlaylistDetail(const QString& userAccount, qint64 playlistId, bool useCache) const { callVoid("requestPlaylistDetail", userAccount, playlistId, useCache); }
    void requestRecommendations(const QString& userAccount, const QString& scene, int limit, bool excludePlayed) const { callVoid("requestRecommendations", userAccount, scene, limit, excludePlayed); }
    void requestSimilarRecommendations(const QString& songId, int limit) const { callVoid("requestSimilarRecommendations", songId, limit); }
    void submitRecommendationFeedback(const QString& userId,
                                      const QString& songId,
                                      const QString& eventType,
                                      const QString& scene,
                                      const QString& requestId,
                                      const QString& modelVersion,
                                      qint64 playMs,
                                      qint64 durationMs) const
    {
        callVoid("submitRecommendationFeedback", userId, songId, eventType, scene, requestId, modelVersion, playMs, durationMs);
    }
    void addPlayHistory(const QString& userAccount,
                        const QString& musicPath,
                        const QString& title,
                        const QString& artist,
                        const QString& album,
                        const QString& durationSec,
                        bool isLocal) const
    {
        callVoid("addPlayHistory", userAccount, musicPath, title, artist, album, durationSec, isLocal);
    }
    void removeHistory(const QString& userAccount, const QStringList& paths) const { callVoid("removeHistory", userAccount, paths); }
    void addFavorite(const QString& userAccount,
                     const QString& musicPath,
                     const QString& title,
                     const QString& artist,
                     const QString& durationSec,
                     bool isLocal) const
    {
        callVoid("addFavorite", userAccount, musicPath, title, artist, durationSec, isLocal);
    }
    void removeFavorite(const QString& userAccount, const QStringList& paths) const { callVoid("removeFavorite", userAccount, paths); }
    void createPlaylist(const QString& userAccount, const QString& name, const QString& description, const QString& coverPath) const { callVoid("createPlaylist", userAccount, name, description, coverPath); }
    void updatePlaylist(const QString& userAccount, qint64 playlistId, const QString& name, const QString& description, const QString& coverPath) const { callVoid("updatePlaylist", userAccount, playlistId, name, description, coverPath); }
    void deletePlaylist(const QString& userAccount, qint64 playlistId) const { callVoid("deletePlaylist", userAccount, playlistId); }
    void addPlaylistItems(const QString& userAccount, qint64 playlistId, const QVariantList& items) const { callVoid("addPlaylistItems", userAccount, playlistId, items); }
    void removePlaylistItems(const QString& userAccount, qint64 playlistId, const QStringList& paths) const { callVoid("removePlaylistItems", userAccount, playlistId, paths); }
    void reorderPlaylistItems(const QString& userAccount, qint64 playlistId, const QVariantList& ordered) const { callVoid("reorderPlaylistItems", userAccount, playlistId, ordered); }

private:
    QObject* m_object = nullptr;
};

QVariantMap settingsSnapshot()
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
        {QStringLiteral("playerPageStyle"), settings.playerPageStyle()},
        {QStringLiteral("agentMode"), settings.agentMode()},
        {QStringLiteral("agentLocalModelPath"), settings.agentLocalModelPath()},
        {QStringLiteral("agentLocalModelBaseUrl"), settings.agentLocalModelBaseUrl()},
        {QStringLiteral("agentLocalModelName"), settings.agentLocalModelName()},
        {QStringLiteral("agentLocalContextSize"), settings.agentLocalContextSize()},
        {QStringLiteral("agentLocalThreadCount"), settings.agentLocalThreadCount()},
        {QStringLiteral("agentRemoteFallbackEnabled"), settings.agentRemoteFallbackEnabled()},
        {QStringLiteral("agentRemoteBaseUrl"), settings.agentRemoteBaseUrl()},
        {QStringLiteral("agentRemoteModelName"), settings.agentRemoteModelName()}
    };
}
} // namespace

AgentToolExecutor::AgentToolExecutor(HostStateProvider* hostStateProvider, QObject* parent)
    : QObject(parent)
    , m_hostStateProvider(hostStateProvider)
    , m_toolRegistry(new ToolRegistry())
{
}

AgentToolExecutor::~AgentToolExecutor()
{
    delete m_toolRegistry;
    m_toolRegistry = nullptr;
}

QObject* AgentToolExecutor::serviceFromHostContext(QObject* hostContext, const QString& serviceName)
{
    if (!hostContext) {
        return nullptr;
    }

    QObject* service = nullptr;
    QMetaObject::invokeMethod(hostContext,
                              "service",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QObject*, service),
                              Q_ARG(QString, serviceName));
    return service;
}

void AgentToolExecutor::setHostContext(QObject* hostContext)
{
    if (m_hostContext == hostContext) {
        return;
    }

    if (m_hostService) {
        disconnect(m_hostService, nullptr, this, nullptr);
    }

    clearPendingTasks();
    m_hostContext = hostContext;
    m_hostService = serviceFromHostContext(hostContext, QStringLiteral("clientAutomationHost"));

    if (m_hostStateProvider) {
        m_hostStateProvider->setHostContext(hostContext);
    }

    if (!m_hostService) {
        return;
    }

    connect(m_hostService, SIGNAL(searchResultsReady(QVariantList)), this, SLOT(onSearchResultsReady(QVariantList)));
    connect(m_hostService, SIGNAL(historyListReady(QVariantList)), this, SLOT(onHistoryListReady(QVariantList)));
    connect(m_hostService, SIGNAL(favoritesListReady(QVariantList)), this, SLOT(onFavoritesListReady(QVariantList)));
    connect(m_hostService, SIGNAL(playlistsListReady(QVariantList,int,int,int)), this, SLOT(onPlaylistsListReady(QVariantList,int,int,int)));
    connect(m_hostService, SIGNAL(playlistDetailReady(QVariantMap)), this, SLOT(onPlaylistDetailReady(QVariantMap)));
    connect(m_hostService, SIGNAL(recommendationListReady(QVariantMap,QVariantList)), this, SLOT(onRecommendationListReady(QVariantMap,QVariantList)));
    connect(m_hostService, SIGNAL(similarRecommendationListReady(QVariantMap,QVariantList,QString)), this, SLOT(onSimilarRecommendationListReady(QVariantMap,QVariantList,QString)));
    connect(m_hostService, SIGNAL(recommendationFeedbackResultReady(bool,QString,QString)), this, SLOT(onRecommendationFeedbackResultReady(bool,QString,QString)));

    connect(m_hostService, SIGNAL(addFavoriteResultReady(bool)), this, SLOT(onAddFavoriteResultReady(bool)));
    connect(m_hostService, SIGNAL(removeFavoriteResultReady(bool)), this, SLOT(onRemoveFavoriteResultReady(bool)));
    connect(m_hostService, SIGNAL(addHistoryResultReady(bool)), this, SLOT(onAddHistoryResultReady(bool)));
    connect(m_hostService, SIGNAL(removeHistoryResultReady(bool)), this, SLOT(onRemoveHistoryResultReady(bool)));
    connect(m_hostService, SIGNAL(createPlaylistResultReady(bool,qint64,QString)), this, SLOT(onCreatePlaylistResultReady(bool,qint64,QString)));
    connect(m_hostService, SIGNAL(deletePlaylistResultReady(bool,qint64,QString)), this, SLOT(onDeletePlaylistResultReady(bool,qint64,QString)));
    connect(m_hostService, SIGNAL(updatePlaylistResultReady(bool,qint64,QString)), this, SLOT(onUpdatePlaylistResultReady(bool,qint64,QString)));
    connect(m_hostService, SIGNAL(addPlaylistItemsResultReady(bool,qint64,int,int,QString)), this, SLOT(onAddPlaylistItemsResultReady(bool,qint64,int,int,QString)));
    connect(m_hostService, SIGNAL(removePlaylistItemsResultReady(bool,qint64,int,QString)), this, SLOT(onRemovePlaylistItemsResultReady(bool,qint64,int,QString)));
    connect(m_hostService, SIGNAL(reorderPlaylistItemsResultReady(bool,qint64,QString)), this, SLOT(onReorderPlaylistItemsResultReady(bool,qint64,QString)));

    connect(m_hostService, SIGNAL(lyricsReady(QStringList)), this, SLOT(onLyricsReady(QStringList)));
    connect(m_hostService, SIGNAL(videoListReady(QVariantList)), this, SLOT(onVideoListReady(QVariantList)));
    connect(m_hostService, SIGNAL(videoStreamUrlReady(QString)), this, SLOT(onVideoStreamUrlReady(QString)));
    connect(m_hostService, SIGNAL(artistSearchReady(bool,QString)), this, SLOT(onArtistSearchReady(bool,QString)));
    connect(m_hostService, SIGNAL(artistTracksReady(QVariantList,QString)), this, SLOT(onArtistTracksReady(QVariantList,QString)));
}

bool AgentToolExecutor::executeToolCall(const QString& toolCallId,const QString& tool,const QVariantMap& args){
    const QString id=toolCallId.trimmed();
    const QString name=tool.trimmed();
    if(id.isEmpty()||name.isEmpty()) return false;

    if(!m_toolRegistry->contains(name)){failTool(id,QStringLiteral("unsupported_tool"),QStringLiteral("Qt 端暂未支持工具：%1").arg(name));return true;}
    const AgentToolDefinition def = m_toolRegistry->definition(name);
    QString ec,em;
    if(!m_toolRegistry->validateArgs(name,args,&ec,&em)){failTool(id,ec,em);return true;}
    if(!m_hostStateProvider){failTool(id,QStringLiteral("host_state_unavailable"),QStringLiteral("HostStateProvider 不可用"));return true;}

    if(SettingsManager::instance().agentMode().trimmed().compare(QStringLiteral("assistant"), Qt::CaseInsensitive)==0
       && !def.readOnly){
        failTool(id,QStringLiteral("assistant_read_only"),
                 QStringLiteral("当前为 assistant 模式，只允许解释，不执行写操作。"));
        return true;
    }

    HostServiceProxy hostService(m_hostService);
    const auto ensureHostService=[&]()->bool{
        if(hostService) return true;
        failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("clientAutomationHost 服务不可用"));
        return false;
    };

    if(name==QStringLiteral("getHostContext")){succeedTool(id,m_hostStateProvider->hostContextSnapshot());return true;}
    if(name==QStringLiteral("getVisiblePage")){succeedTool(id,{{QStringLiteral("currentPage"),m_hostStateProvider->hostContextSnapshot().value(QStringLiteral("currentPage")).toString()}});return true;}
    if(name==QStringLiteral("getSelectedPlaylist")){succeedTool(id,m_hostStateProvider->hostContextSnapshot().value(QStringLiteral("selectedPlaylist")).toMap());return true;}
    if(name==QStringLiteral("getSelectedTrackIds")){const QVariantList items=m_hostStateProvider->hostContextSnapshot().value(QStringLiteral("selectedTrackIds")).toList();succeedTool(id,{{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});return true;}
    if(name==QStringLiteral("getUiOverview")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->uiOverviewSnapshot());
        return true;
    }
    if(name==QStringLiteral("getUiPageState")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->uiPageStateSnapshot(args.value(QStringLiteral("page")).toString().trimmed()));
        return true;
    }
    if(name==QStringLiteral("getMusicTabItems")){
        if(!ensureHostService()) return true;
        const QString tab=args.value(QStringLiteral("tab")).toString().trimmed().toLower();
        QVariantMap options;
        if(args.contains(QStringLiteral("limit"))) options.insert(QStringLiteral("limit"),args.value(QStringLiteral("limit")));
        if(args.contains(QStringLiteral("playlistId"))) options.insert(QStringLiteral("playlistId"),args.value(QStringLiteral("playlistId")));
        if(args.contains(QStringLiteral("playlistName"))) options.insert(QStringLiteral("playlistName"),args.value(QStringLiteral("playlistName")));
        const QVariantList items=hostService->musicTabItemsSnapshot(tab,options);
        if(tab==QStringLiteral("playlist_detail")&&items.isEmpty()){
            failTool(id,QStringLiteral("playlist_context_required"),QStringLiteral("当前没有可用的歌单详情上下文"));
            return true;
        }
        rememberTracks(items);
        rememberPlaylistMeta(items);
        succeedTool(id,{{QStringLiteral("tab"),tab},{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});
        return true;
    }
    if(name==QStringLiteral("getMusicTabItem")){
        if(!ensureHostService()) return true;
        const QString tab=args.value(QStringLiteral("tab")).toString().trimmed().toLower();
        const QVariantMap item=hostService->resolveMusicTabItem(tab,buildMusicTabSelector(args));
        if(item.isEmpty()){
            failTool(id,tab==QStringLiteral("playlist_detail")?QStringLiteral("playlist_context_required"):QStringLiteral("tab_item_not_found"),
                     tab==QStringLiteral("playlist_detail")?QStringLiteral("当前没有可用的歌单详情上下文"):QStringLiteral("目标 tab 中没有匹配的对象"));
            return true;
        }
        rememberTracks(QVariantList{item});
        rememberPlaylistMeta(QVariantList{item});
        succeedTool(id,item);
        return true;
    }
    if(name==QStringLiteral("playMusicTabTrack")){
        if(!ensureHostService()) return true;
        const QString tab=args.value(QStringLiteral("tab")).toString().trimmed().toLower();
        QString errorCode;
        QString errorMessage;
        const QVariantMap item=resolveMusicTabItem(tab,args,&errorCode,&errorMessage);
        if(item.isEmpty()){failTool(id,errorCode,errorMessage);return true;}
        if(item.value(QStringLiteral("musicPath")).toString().trimmed().isEmpty()){
            failTool(id,QStringLiteral("unsupported_action_for_tab"),QStringLiteral("当前 tab 不是歌曲列表，无法直接播放"));
            return true;
        }
        const QVariantMap result=hostService->invokeSongAction(QStringLiteral("play"),item);
        succeedTool(id,{{QStringLiteral("tab"),tab},{QStringLiteral("played"),true},{QStringLiteral("item"),item},{QStringLiteral("result"),result}});
        return true;
    }
    if(name==QStringLiteral("invokeMusicTabAction")){
        if(!ensureHostService()) return true;
        const QString tab=args.value(QStringLiteral("tab")).toString().trimmed().toLower();
        const QString action=args.value(QStringLiteral("action")).toString().trimmed().toLower();
        if(!musicTabActionSupported(tab,action)){
            failTool(id,QStringLiteral("unsupported_action_for_tab"),QStringLiteral("当前 tab 不支持该动作"));
            return true;
        }
        QString errorCode;
        QString errorMessage;
        QVariantMap item=resolveMusicTabItem(tab,args,&errorCode,&errorMessage);
        if(item.isEmpty()){failTool(id,errorCode,errorMessage);return true;}
        if(action==QStringLiteral("toggle_current_playback")){
            const QString currentPath=m_hostStateProvider->currentTrackSnapshot().value(QStringLiteral("musicPath")).toString().trimmed();
            const QString itemPath=item.value(QStringLiteral("musicPath")).toString().trimmed();
            const QString actualAction=(currentPath==itemPath&&!currentPath.isEmpty())?QStringLiteral("toggle_current_playback"):QStringLiteral("play");
            const QVariantMap result=hostService->invokeSongAction(actualAction,item);
            succeedTool(id,{{QStringLiteral("tab"),tab},{QStringLiteral("action"),action},{QStringLiteral("item"),item},{QStringLiteral("result"),result}});
            return true;
        }
        if(action==QStringLiteral("download")){
            const QString relativePath=item.value(QStringLiteral("path")).toString().trimmed();
            if(relativePath.isEmpty()||item.value(QStringLiteral("isLocal")).toBool()){
                failTool(id,QStringLiteral("unsupported_action_for_tab"),QStringLiteral("当前条目不支持下载"));
                return true;
            }
            QVariantMap downloadArgs{
                {QStringLiteral("relativePath"), relativePath},
                {QStringLiteral("coverUrl"), item.value(QStringLiteral("cover")).toString()}
            };
            return executeToolCall(id,QStringLiteral("downloadTrack"),downloadArgs);
        }
        if(action==QStringLiteral("add_to_playlist")){
            if(args.contains(QStringLiteral("playlistId"))) item.insert(QStringLiteral("playlistId"),args.value(QStringLiteral("playlistId")));
            if(args.contains(QStringLiteral("playlistName"))) item.insert(QStringLiteral("playlistName"),args.value(QStringLiteral("playlistName")));
        }
        if(action==QStringLiteral("remove_or_delete")&&tab==QStringLiteral("favorites")){
            failTool(id,QStringLiteral("unsupported_action_for_tab"),QStringLiteral("喜欢音乐页请使用 remove_favorite"));
            return true;
        }
        const QVariantMap result=hostService->invokeSongAction(action,item);
        succeedTool(id,{{QStringLiteral("tab"),tab},{QStringLiteral("action"),action},{QStringLiteral("item"),item},{QStringLiteral("result"),result}});
        return true;
    }
    if(name==QStringLiteral("getUserProfile")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->userProfileSnapshot());
        return true;
    }
    if(name==QStringLiteral("refreshUserProfile")){
        if(!ensureHostService()) return true;
        if(!hostService->refreshUserProfile()){failTool(id,QStringLiteral("profile_refresh_failed"),QStringLiteral("当前无法刷新用户资料"));return true;}
        succeedTool(id,{{QStringLiteral("accepted"),true},{QStringLiteral("message"),QStringLiteral("已触发资料刷新请求")}});return true;
    }
    if(name==QStringLiteral("updateUsername")){
        if(!ensureHostService()) return true;
        if(!hostService->updateUsername(args.value(QStringLiteral("username")).toString())){failTool(id,QStringLiteral("username_update_failed"),QStringLiteral("当前无法修改用户名"));return true;}
        succeedTool(id,{{QStringLiteral("accepted"),true},{QStringLiteral("username"),args.value(QStringLiteral("username")).toString().trimmed()}});return true;
    }
    if(name==QStringLiteral("uploadAvatar")){
        if(!ensureHostService()) return true;
        if(!hostService->uploadAvatar(args.value(QStringLiteral("filePath")).toString())){failTool(id,QStringLiteral("avatar_upload_failed"),QStringLiteral("当前无法上传头像"));return true;}
        succeedTool(id,{{QStringLiteral("accepted"),true},{QStringLiteral("filePath"),args.value(QStringLiteral("filePath")).toString().trimmed()}});return true;
    }
    if(name==QStringLiteral("logoutUser")){
        if(!ensureHostService()) return true;
        if(!hostService->logoutUser()){failTool(id,QStringLiteral("logout_failed"),QStringLiteral("当前无法退出登录"));return true;}
        succeedTool(id,{{QStringLiteral("accepted"),true},{QStringLiteral("message"),QStringLiteral("已退出登录")}});return true;
    }
    if(name==QStringLiteral("returnToWelcome")){
        if(!ensureHostService()) return true;
        if(!hostService->returnToWelcome()){failTool(id,QStringLiteral("return_to_welcome_failed"),QStringLiteral("当前无法返回欢迎页"));return true;}
        succeedTool(id,{{QStringLiteral("accepted"),true},{QStringLiteral("message"),QStringLiteral("已返回欢迎页")}});return true;
    }

    if (name == QStringLiteral("getCurrentTrack")) {succeedTool(id, m_hostStateProvider->currentTrackSnapshot());return true;}
    if (name == QStringLiteral("pausePlayback")) {
        if (!ensureHostService()) return true;
        hostService->pausePlayback();
        succeedTool(id, {{QStringLiteral("ok"), true}});
        return true;
    }
    if (name == QStringLiteral("resumePlayback")) {
        if (!ensureHostService()) return true;
        hostService->resumePlayback();
        succeedTool(id, {{QStringLiteral("ok"), true}});
        return true;
    }
    if (name == QStringLiteral("stopPlayback")) {
        if (!ensureHostService()) return true;
        hostService->stopPlayback();
        succeedTool(id, {{QStringLiteral("ok"), true}});
        return true;
    }
    if (name == QStringLiteral("seekPlayback")) {
        if (!ensureHostService()) return true;
        const qint64 p = qMax<qint64>(0, args.value(QStringLiteral("positionMs")).toLongLong());
        hostService->seekPlayback(p);
        succeedTool(id, {{QStringLiteral("positionMs"), p}});
        return true;
    }
    if (name == QStringLiteral("playNext")) {
        if (!ensureHostService()) return true;
        hostService->playNext();
        succeedTool(id, {{QStringLiteral("ok"), true}});
        return true;
    }
    if (name == QStringLiteral("playPrevious")) {
        if (!ensureHostService()) return true;
        hostService->playPrevious();
        succeedTool(id, {{QStringLiteral("ok"), true}});
        return true;
    }
    if (name == QStringLiteral("playAtIndex")) {
        if (!ensureHostService()) return true;
        const int index = args.value(QStringLiteral("index")).toInt();
        hostService->playAtIndex(index);
        succeedTool(id, {{QStringLiteral("index"), index}});
        return true;
    }
    if (name == QStringLiteral("setVolume")) {
        if (!ensureHostService()) return true;
        const int volume = qBound(0, args.value(QStringLiteral("volume")).toInt(), 100);
        hostService->setVolume(volume);
        succeedTool(id, {{QStringLiteral("volume"), volume}});
        return true;
    }
    if (name == QStringLiteral("setPlayMode")) {
        if (!ensureHostService()) return true;
        const int mode = parsePlayModeValue(args.value(QStringLiteral("mode")));
        if (mode < 0 || !hostService->setPlayMode(mode)) {
            failTool(id, QStringLiteral("invalid_args"), QStringLiteral("mode ??? sequential/repeat_one/repeat_all/shuffle ?????"));
            return true;
        }
        succeedTool(id, {{QStringLiteral("playMode"), mode}, {QStringLiteral("playModeName"), playModeName(mode)}});
        return true;
    }
    if (name == QStringLiteral("getPlaybackQueue")) {succeedTool(id, buildQueueSnapshot());return true;}
    if (name == QStringLiteral("setPlaybackQueue")) {
        if (!ensureHostService()) return true;
        QVariantList trackItems = args.value(QStringLiteral("items")).toList();
        if (trackItems.isEmpty()) {
            QString queueError;
            trackItems = buildPlaylistItemsFromTrackIds(args.value(QStringLiteral("trackIds")).toList(), &queueError);
            if (trackItems.isEmpty()) {
                failTool(id, QStringLiteral("invalid_args"), queueError.isEmpty() ? QStringLiteral("?????? trackIds/items") : queueError);
                return true;
            }
        }
        const int startIndex = qMax(0, args.value(QStringLiteral("startIndex"), 0).toInt());
        const bool playNow = args.value(QStringLiteral("playNow"), true).toBool();
        if (!hostService->setPlaybackQueue(trackItems, startIndex, playNow)) {
            failTool(id, QStringLiteral("invalid_args"), QStringLiteral("trackIds/items ?????????"));
            return true;
        }
        QVariantMap snapshot = buildQueueSnapshot();
        snapshot.insert(QStringLiteral("queueReset"), true);
        snapshot.insert(QStringLiteral("startIndex"), startIndex);
        snapshot.insert(QStringLiteral("playNow"), playNow);
        succeedTool(id, snapshot);
        return true;
    }
    if (name == QStringLiteral("addToPlaybackQueue")) {
        if (!ensureHostService()) return true;
        QString path = args.value(QStringLiteral("musicPath")).toString().trimmed();
        if (path.isEmpty()) path = resolveTrackById(args.value(QStringLiteral("trackId")).toString()).value(QStringLiteral("musicPath")).toString().trimmed();
        if (path.isEmpty() || !hostService->addToPlaybackQueue(path)) {
            failTool(id, QStringLiteral("invalid_args"), QStringLiteral("??????????????"));
            return true;
        }
        QVariantMap snapshot = buildQueueSnapshot();
        snapshot.insert(QStringLiteral("added"), true);
        snapshot.insert(QStringLiteral("musicPath"), path);
        succeedTool(id, snapshot);
        return true;
    }
    if (name == QStringLiteral("removeFromPlaybackQueue")) {
        if (!ensureHostService()) return true;
        const QVariantMap before = buildQueueSnapshot();
        const QVariantList items = before.value(QStringLiteral("items")).toList();
        const int index = args.value(QStringLiteral("index")).toInt();
        if (index < 0 || index >= items.size()) {
            failTool(id, QStringLiteral("invalid_args"), QStringLiteral("index ??????????"));
            return true;
        }
        const QVariantMap removedItem = items.at(index).toMap();
        if (!hostService->removeFromPlaybackQueue(index)) {
            failTool(id, QStringLiteral("queue_remove_failed"), QStringLiteral("?????????????"));
            return true;
        }
        QVariantMap snapshot = buildQueueSnapshot();
        snapshot.insert(QStringLiteral("removed"), true);
        snapshot.insert(QStringLiteral("removedIndex"), index);
        snapshot.insert(QStringLiteral("removedItem"), removedItem);
        succeedTool(id, snapshot);
        return true;
    }
    if (name == QStringLiteral("clearPlaybackQueue")) {
        if (!ensureHostService()) return true;
        hostService->clearPlaybackQueue();
        QVariantMap snapshot = buildQueueSnapshot();
        snapshot.insert(QStringLiteral("cleared"), true);
        succeedTool(id, snapshot);
        return true;
    }
    if(name==QStringLiteral("getLocalTracks")){const int l=qBound(1,args.value(QStringLiteral("limit"),500).toInt(),5000);QVariantList it=m_hostStateProvider->convertLocalMusicList(l);rememberTracks(it);succeedTool(id,{{QStringLiteral("items"),it},{QStringLiteral("count"),it.size()}});return true;}
    if(name==QStringLiteral("addLocalTrack")){
        if(!ensureHostService()) return true;
        const QString filePath=args.value(QStringLiteral("filePath")).toString().trimmed();
        if(filePath.isEmpty()||!QFileInfo::exists(filePath)){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("filePath ????????"));return true;}
        QVariantMap track{{QStringLiteral("filePath"),filePath},{QStringLiteral("fileName"),args.value(QStringLiteral("fileName")).toString().trimmed()},{QStringLiteral("artist"),args.value(QStringLiteral("artist")).toString().trimmed()}};
        if(!hostService->addLocalTrack(track)){failTool(id,QStringLiteral("add_local_track_failed"),QStringLiteral("??????????"));return true;}
        QVariantList items=m_hostStateProvider->convertLocalMusicList(5000);
        rememberTracks(items);
        QVariantMap addedTrack;
        for(const QVariant& value:items){const QVariantMap item=value.toMap();if(item.value(QStringLiteral("musicPath")).toString().trimmed()==filePath){addedTrack=item;break;}}
        succeedTool(id,{{QStringLiteral("added"),true},{QStringLiteral("track"),addedTrack},{QStringLiteral("count"),items.size()}});
        return true;
    }
    if(name==QStringLiteral("removeLocalTrack")){
        if(!ensureHostService()) return true;
        const QString filePath=args.value(QStringLiteral("filePath")).toString().trimmed();
        if(filePath.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("filePath ????"));return true;}
        if(!hostService->removeLocalTrack(filePath)){failTool(id,QStringLiteral("remove_local_track_failed"),QStringLiteral("??????????"));return true;}
        QVariantList items=m_hostStateProvider->convertLocalMusicList(5000);
        rememberTracks(items);
        succeedTool(id,{{QStringLiteral("removed"),true},{QStringLiteral("filePath"),filePath},{QStringLiteral("count"),items.size()}});
        return true;
    }
    if(name==QStringLiteral("getDownloadTasks")){
        if(!ensureHostService()) return true;
        const QString scope=args.value(QStringLiteral("scope"),QStringLiteral("all")).toString().trimmed().toLower();
        succeedTool(id,hostService->downloadTasksSnapshot(scope));
        return true;
    }
    if(name==QStringLiteral("downloadTrack")){
        if(!ensureHostService()) return true;
        if(!requireLogin(id)) return true;
        const QVariantMap result=hostService->startDownloadTrack(args.value(QStringLiteral("relativePath")).toString().trimmed(),args.value(QStringLiteral("coverUrl")).toString().trimmed());
        if(!result.value(QStringLiteral("accepted")).toBool()){
            failTool(id,result.value(QStringLiteral("code")).toString().trimmed().isEmpty()?QStringLiteral("download_start_failed"):result.value(QStringLiteral("code")).toString(),result.value(QStringLiteral("message")).toString().trimmed().isEmpty()?QStringLiteral("????????????"):result.value(QStringLiteral("message")).toString());
            return true;
        }
        succeedTool(id,result);
        return true;
    }
    if(name==QStringLiteral("pauseDownloadTask")){
        if(!ensureHostService()) return true;
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        hostService->pauseDownloadTask(taskId);
        succeedTool(id,{{QStringLiteral("task"),hostService->downloadTaskSnapshot(taskId)}});
        return true;
    }
    if(name==QStringLiteral("resumeDownloadTask")){
        if(!ensureHostService()) return true;
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        hostService->resumeDownloadTask(taskId);
        succeedTool(id,{{QStringLiteral("task"),hostService->downloadTaskSnapshot(taskId)}});
        return true;
    }
    if(name==QStringLiteral("cancelDownloadTask")){
        if(!ensureHostService()) return true;
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        hostService->cancelDownloadTask(taskId);
        succeedTool(id,{{QStringLiteral("task"),hostService->downloadTaskSnapshot(taskId)}});
        return true;
    }
    if(name==QStringLiteral("removeDownloadTask")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->removeDownloadTask(args.value(QStringLiteral("taskId")).toString().trimmed()));
        return true;
    }
    if(name==QStringLiteral("getVideoWindowState")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("playVideo")){
        if(!ensureHostService()) return true;
        QString source=args.value(QStringLiteral("videoUrl")).toString().trimmed();
        if(source.isEmpty()){
            source=args.value(QStringLiteral("videoPath")).toString().trimmed();
            if(!source.isEmpty()&&!source.startsWith(QStringLiteral("http"),Qt::CaseInsensitive)&&!QFileInfo(source).isAbsolute()&&!source.startsWith(QStringLiteral("video/"),Qt::CaseInsensitive)){
                source.prepend(QStringLiteral("video/"));
            }
            const QUrl videoUrl=toPlayableUrl(source);
            source=videoUrl.isEmpty()?source:videoUrl.toString();
        }
        if(!hostService->playVideo(source)){failTool(id,QStringLiteral("play_video_failed"),QStringLiteral("??????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("pauseVideoPlayback")){
        if(!ensureHostService()) return true;
        if(!hostService->pauseVideo()){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("resumeVideoPlayback")){
        if(!ensureHostService()) return true;
        if(!hostService->resumeVideo()){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("closeVideoWindow")){
        if(!ensureHostService()) return true;
        if(!hostService->closeVideoWindow()){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("seekVideoPlayback")){
        if(!ensureHostService()) return true;
        if(!hostService->seekVideo(qMax<qint64>(0,args.value(QStringLiteral("positionMs")).toLongLong()))){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("setVideoFullScreen")){
        if(!ensureHostService()) return true;
        if(!hostService->setVideoFullScreen(args.value(QStringLiteral("enabled")).toBool())){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("setVideoPlaybackRate")){
        if(!ensureHostService()) return true;
        if(!hostService->setVideoPlaybackRate(args.value(QStringLiteral("rate")).toDouble())){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("setVideoQualityPreset")){
        if(!ensureHostService()) return true;
        if(!hostService->setVideoQualityPreset(args.value(QStringLiteral("preset")).toString())){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("?????????????"));return true;}
        succeedTool(id,hostService->videoWindowState());
        return true;
    }
    if(name==QStringLiteral("getDesktopLyricsState")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->desktopLyricsState());
        return true;
    }
    if(name==QStringLiteral("showDesktopLyrics")){
        if(!ensureHostService()) return true;
        if(!hostService->setDesktopLyricsVisible(true)){failTool(id,QStringLiteral("desktop_lyrics_failed"),QStringLiteral("?????????"));return true;}
        succeedTool(id,hostService->desktopLyricsState());
        return true;
    }
    if(name==QStringLiteral("hideDesktopLyrics")){
        if(!ensureHostService()) return true;
        if(!hostService->setDesktopLyricsVisible(false)){failTool(id,QStringLiteral("desktop_lyrics_failed"),QStringLiteral("?????????"));return true;}
        succeedTool(id,hostService->desktopLyricsState());
        return true;
    }
    if(name==QStringLiteral("setDesktopLyricsStyle")){
        if(!ensureHostService()) return true;
        QVariantMap style;
        if(args.contains(QStringLiteral("color"))) style.insert(QStringLiteral("color"),args.value(QStringLiteral("color")));
        if(args.contains(QStringLiteral("fontSize"))) style.insert(QStringLiteral("fontSize"),args.value(QStringLiteral("fontSize")));
        if(args.contains(QStringLiteral("fontFamily"))) style.insert(QStringLiteral("fontFamily"),args.value(QStringLiteral("fontFamily")));
        if(style.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("???? color/fontSize/fontFamily ????"));return true;}
        if(!hostService->setDesktopLyricsStyle(style)){failTool(id,QStringLiteral("desktop_lyrics_failed"),QStringLiteral("??????????"));return true;}
        succeedTool(id,hostService->desktopLyricsState());
        return true;
    }
    if(name==QStringLiteral("getPlugins")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->pluginsSnapshot());
        return true;
    }
    if(name==QStringLiteral("getPluginDiagnostics")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->pluginDiagnosticsSnapshot());
        return true;
    }
    if(name==QStringLiteral("reloadPlugins")){
        if(!ensureHostService()) return true;
        succeedTool(id,hostService->reloadPlugins());
        return true;
    }
    if(name==QStringLiteral("unloadPlugin")){
        if(!ensureHostService()) return true;
        const QString pluginKey=args.value(QStringLiteral("pluginKey")).toString().trimmed();
        if(!hostService->unloadPlugin(pluginKey)){failTool(id,QStringLiteral("plugin_unload_failed"),QStringLiteral("????????"));return true;}
        succeedTool(id,{{QStringLiteral("success"),true},{QStringLiteral("pluginKey"),pluginKey}});
        return true;
    }
    if(name==QStringLiteral("unloadAllPlugins")){
        if(!ensureHostService()) return true;
        const int count=hostService->unloadAllPlugins();
        succeedTool(id,{{QStringLiteral("success"),true},{QStringLiteral("count"),count}});
        return true;
    }
    if(name==QStringLiteral("getSettingsSnapshot")){
        succeedTool(id,settingsSnapshot());
        return true;
    }
    if(name==QStringLiteral("updateSetting")){
        SettingsManager& settings=SettingsManager::instance();
        const QString key=args.value(QStringLiteral("key")).toString().trimmed();
        const QVariant value=args.value(QStringLiteral("value"));
        if(key==QStringLiteral("downloadPath")) settings.setDownloadPath(value.toString());
        else if(key==QStringLiteral("downloadLyrics")) settings.setDownloadLyrics(value.toBool());
        else if(key==QStringLiteral("downloadCover")) settings.setDownloadCover(value.toBool());
        else if(key==QStringLiteral("audioCachePath")) settings.setAudioCachePath(value.toString());
        else if(key==QStringLiteral("logPath")) settings.setLogPath(value.toString());
        else if(key==QStringLiteral("serverHost")) settings.setServerHost(value.toString());
        else if(key==QStringLiteral("serverPort")) settings.setServerPort(value.toInt());
        else if(key==QStringLiteral("playerPageStyle")) settings.setPlayerPageStyle(value.toInt());
        else if(key==QStringLiteral("agentMode")) settings.setAgentMode(value.toString().trimmed());
        else if(key==QStringLiteral("agentLocalModelPath")) settings.setAgentLocalModelPath(value.toString().trimmed());
        else if(key==QStringLiteral("agentLocalModelBaseUrl")) settings.setAgentLocalModelBaseUrl(value.toString().trimmed());
        else if(key==QStringLiteral("agentLocalModelName")) settings.setAgentLocalModelName(value.toString().trimmed());
        else if(key==QStringLiteral("agentLocalContextSize")) settings.setAgentLocalContextSize(value.toInt());
        else if(key==QStringLiteral("agentLocalThreadCount")) settings.setAgentLocalThreadCount(value.toInt());
        else if(key==QStringLiteral("agentRemoteFallbackEnabled")) settings.setAgentRemoteFallbackEnabled(value.toBool());
        else if(key==QStringLiteral("agentRemoteBaseUrl")) settings.setAgentRemoteBaseUrl(value.toString().trimmed());
        else if(key==QStringLiteral("agentRemoteModelName")) settings.setAgentRemoteModelName(value.toString().trimmed());
        else {failTool(id,QStringLiteral("unsupported_setting"),QStringLiteral("????? Agent ???????%1").arg(key));return true;}
        QVariantMap snapshot=settingsSnapshot();
        snapshot.insert(QStringLiteral("updatedKey"),key);
        succeedTool(id,snapshot);
        return true;
    }

    if(!ensureHostService()) return true;
    if(name==QStringLiteral("getLyrics")){
        QString musicPath=args.value(QStringLiteral("musicPath")).toString().trimmed();
        if(musicPath.isEmpty()) musicPath=resolveTrackById(args.value(QStringLiteral("trackId")).toString()).value(QStringLiteral("musicPath")).toString().trimmed();
        if(musicPath.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("?????? musicPath/trackId"));return true;}
        m_pendingLyricsFetch.enqueue({id,name,{{QStringLiteral("musicPath"),musicPath}}});
        hostService->requestLyrics(musicPath);
        return true;
    }
    if(name==QStringLiteral("getVideoList")){
        m_pendingVideoListFetch.enqueue({id,name,args});
        hostService->requestVideoList();
        return true;
    }
    if(name==QStringLiteral("getVideoStream")){
        const QString videoPath=args.value(QStringLiteral("videoPath")).toString().trimmed();
        if(videoPath.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("videoPath ????"));return true;}
        m_pendingVideoStreamFetch.enqueue({id,name,args});
        hostService->requestVideoStreamUrl(videoPath);
        return true;
    }
    if(name==QStringLiteral("searchArtist")){
        const QString artist=args.value(QStringLiteral("artist")).toString().trimmed();
        if(artist.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("artist ????"));return true;}
        m_pendingArtistSearch.enqueue({id,name,args});
        hostService->searchArtist(artist);
        return true;
    }
    if(name==QStringLiteral("getTracksByArtist")){
        const QString artist=args.value(QStringLiteral("artist")).toString().trimmed();
        if(artist.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("artist ????"));return true;}
        m_pendingArtistTracks.enqueue({id,name,args});
        hostService->requestArtistTracks(artist);
        return true;
    }

    if(name==QStringLiteral("searchTracks")){
        QString q=args.value(QStringLiteral("keyword")).toString().trimmed();
        if(q.isEmpty()) q=args.value(QStringLiteral("artist")).toString().trimmed();
        if(q.isEmpty()) q=args.value(QStringLiteral("album")).toString().trimmed();
        if(q.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("searchTracks ???? keyword/artist/album"));return true;}
        m_pendingSearch.enqueue({id,name,args});
        hostService->searchMusic(q);
        return true;
    }

    if(name==QStringLiteral("getRecentTracks")){
        if(!requireLogin(id)) return true;
        m_pendingHistoryFetch.enqueue({id,name,args});
        hostService->requestHistory(m_hostStateProvider->currentUserAccount(),qBound(1,args.value(QStringLiteral("limit"),10).toInt(),500),false);
        return true;
    }

    if(name==QStringLiteral("getFavorites")){
        if(!requireLogin(id)) return true;
        m_pendingFavoritesFetch.enqueue({id,name,args});
        hostService->requestFavorites(m_hostStateProvider->currentUserAccount(),false);
        return true;
    }

    if(name==QStringLiteral("getPlaylists")){
        if(!requireLogin(id)) return true;
        m_pendingPlaylistsFetch.enqueue({id,name,args});
        hostService->requestPlaylists(m_hostStateProvider->currentUserAccount(),1,50,false);
        return true;
    }
    if(name==QStringLiteral("getPlaylistTracks")){
        if(!requireLogin(id)) return true;
        const qint64 pid=parsePlaylistId(args);
        if(pid<=0){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("playlistId ????"));return true;}
        if(m_playlistDetailById.contains(pid)){succeedTool(id,m_playlistDetailById.value(pid));return true;}
        m_pendingPlaylistDetail.enqueue({id,name,args});
        hostService->requestPlaylistDetail(m_hostStateProvider->currentUserAccount(),pid,false);
        return true;
    }

    if(name==QStringLiteral("playTrack")){
        QString path=args.value(QStringLiteral("musicPath")).toString().trimmed();
        if(path.isEmpty()) path=resolveTrackById(args.value(QStringLiteral("trackId")).toString()).value(QStringLiteral("musicPath")).toString().trimmed();
        if(path.isEmpty()||!hostService->playTrack(path)){failTool(id,QStringLiteral("play_failed"),QStringLiteral("????"));return true;}
        succeedTool(id,{{QStringLiteral("played"),true},{QStringLiteral("musicPath"),path}});
        return true;
    }

    if(name==QStringLiteral("playPlaylist")){
        if(!requireLogin(id)) return true;
        const qint64 pid=parsePlaylistId(args);
        if(pid<=0){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("playlistId ????"));return true;}
        if(m_playlistDetailById.contains(pid)){
            const QVariantMap detail=m_playlistDetailById.value(pid);
            const QVariantList items=detail.value(QStringLiteral("items")).toList();
            if(items.isEmpty()||!hostService->playPlaylist(items)){failTool(id,QStringLiteral("playlist_empty"),QStringLiteral("???????????"));return true;}
            succeedTool(id,{{QStringLiteral("played"),true},{QStringLiteral("playlist"),detail.value(QStringLiteral("playlist")).toMap()}});
            return true;
        }
        m_pendingPlaylistPlay.enqueue({id,name,args});
        hostService->requestPlaylistDetail(m_hostStateProvider->currentUserAccount(),pid,false);
        return true;
    }

    if(name==QStringLiteral("getRecommendations")){
        if(!requireLogin(id)) return true;
        m_pendingRecommendationList.enqueue({id,name,args});
        hostService->requestRecommendations(m_hostStateProvider->currentUserAccount(),args.value(QStringLiteral("scene"),QStringLiteral("home")).toString(),qBound(1,args.value(QStringLiteral("limit"),24).toInt(),200),args.value(QStringLiteral("excludePlayed"),true).toBool());
        return true;
    }

    if(name==QStringLiteral("getSimilarRecommendations")){
        const QString songId=args.value(QStringLiteral("songId")).toString().trimmed();
        if(songId.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("songId ????"));return true;}
        m_pendingSimilarRecommendationList.enqueue({id,name,args});
        hostService->requestSimilarRecommendations(songId,qBound(1,args.value(QStringLiteral("limit"),12).toInt(),100));
        return true;
    }

    if(!requireLogin(id)) return true;
    const QString user=m_hostStateProvider->currentUserAccount();

    if(name==QStringLiteral("addRecentTrack")){
        m_pendingHistoryAdd.enqueue({id,name,args});
        hostService->addPlayHistory(user,args.value(QStringLiteral("musicPath")).toString(),args.value(QStringLiteral("title")).toString(),args.value(QStringLiteral("artist")).toString(),args.value(QStringLiteral("album")).toString(),QString::number(qMax(0,args.value(QStringLiteral("durationSec"),0).toInt())),args.value(QStringLiteral("isLocal"),true).toBool());
        return true;
    }
    if(name==QStringLiteral("removeRecentTracks")){
        m_pendingHistoryRemove.enqueue({id,name,args});
        hostService->removeHistory(user,variantToStringList(args.value(QStringLiteral("musicPaths"))));
        return true;
    }
    if(name==QStringLiteral("addFavorite")){
        m_pendingFavoriteAdd.enqueue({id,name,args});
        hostService->addFavorite(user,args.value(QStringLiteral("musicPath")).toString(),args.value(QStringLiteral("title")).toString(),args.value(QStringLiteral("artist")).toString(),QString::number(qMax(0,args.value(QStringLiteral("durationSec"),0).toInt())),args.value(QStringLiteral("isLocal"),true).toBool());
        return true;
    }
    if(name==QStringLiteral("removeFavorites")){
        m_pendingFavoriteRemove.enqueue({id,name,args});
        hostService->removeFavorite(user,variantToStringList(args.value(QStringLiteral("musicPaths"))));
        return true;
    }
    if(name==QStringLiteral("createPlaylist")){
        m_pendingCreatePlaylist.enqueue({id,name,args});
        hostService->createPlaylist(user,args.value(QStringLiteral("name")).toString(),args.value(QStringLiteral("description")).toString(),args.value(QStringLiteral("coverPath")).toString());
        return true;
    }
    if(name==QStringLiteral("updatePlaylist")){
        m_pendingUpdatePlaylist.enqueue({id,name,args});
        hostService->updatePlaylist(user,parsePlaylistId(args),args.value(QStringLiteral("name")).toString(),args.value(QStringLiteral("description")).toString(),args.value(QStringLiteral("coverPath")).toString());
        return true;
    }
    if(name==QStringLiteral("deletePlaylist")){
        m_pendingDeletePlaylist.enqueue({id,name,args});
        hostService->deletePlaylist(user,parsePlaylistId(args));
        return true;
    }
    if(name==QStringLiteral("addPlaylistItems")||name==QStringLiteral("addTracksToPlaylist")){
        QVariantList items=args.value(QStringLiteral("items")).toList();
        if(items.isEmpty()){
            QString err;
            items=buildPlaylistItemsFromTrackIds(args.value(QStringLiteral("trackIds")).toList(),&err);
            if(items.isEmpty()){failTool(id,QStringLiteral("invalid_args"),err.isEmpty()?QStringLiteral("???? trackIds/items"):err);return true;}
        }
        QVariantMap callArgs=args;
        callArgs.insert(QStringLiteral("items"),items);
        m_pendingAddPlaylistItems.enqueue({id,name,callArgs});
        hostService->addPlaylistItems(user,parsePlaylistId(args),items);
        return true;
    }
    if(name==QStringLiteral("removePlaylistItems")){
        QStringList p=variantToStringList(args.value(QStringLiteral("musicPaths")));
        if(p.isEmpty()) p=buildMusicPathsFromTrackIds(args.value(QStringLiteral("trackIds")).toList());
        QVariantMap callArgs=args;
        callArgs.insert(QStringLiteral("musicPaths"),p);
        m_pendingRemovePlaylistItems.enqueue({id,name,callArgs});
        hostService->removePlaylistItems(user,parsePlaylistId(args),p);
        return true;
    }
    if(name==QStringLiteral("reorderPlaylistItems")){
        QVariantList ordered=args.value(QStringLiteral("orderedItems")).toList();
        if(ordered.isEmpty()){
            int pos=1;
            for(const QString& p:variantToStringList(args.value(QStringLiteral("orderedPaths")))){QVariantMap it;it.insert(QStringLiteral("music_path"),p);it.insert(QStringLiteral("position"),pos++);ordered.push_back(it);}
        }
        QVariantMap callArgs=args;
        callArgs.insert(QStringLiteral("orderedItems"),ordered);
        m_pendingReorderPlaylistItems.enqueue({id,name,callArgs});
        hostService->reorderPlaylistItems(user,parsePlaylistId(args),ordered);
        return true;
    }
    if(name==QStringLiteral("submitRecommendationFeedback")){
        m_pendingRecommendationFeedback.enqueue({id,name,args});
        hostService->submitRecommendationFeedback(user,args.value(QStringLiteral("songId")).toString(),args.value(QStringLiteral("eventType")).toString(),args.value(QStringLiteral("scene"),QStringLiteral("home")).toString(),args.value(QStringLiteral("requestId")).toString(),args.value(QStringLiteral("modelVersion")).toString(),qMax<qint64>(0,args.value(QStringLiteral("playMs"),0).toLongLong()),qMax<qint64>(0,args.value(QStringLiteral("durationMs"),0).toLongLong()));
        return true;
    }

    failTool(id,QStringLiteral("unsupported_tool"),QStringLiteral("???????%1").arg(name));
    return true;
}

void AgentToolExecutor::onSearchResultsReady(const QVariantList& list)
{
    if(m_pendingSearch.isEmpty()) return;
    auto task = m_pendingSearch.dequeue();
    QVariantList items = list.mid(0, qBound(1, task.args.value(QStringLiteral("limit"), 10).toInt(), 200));
    rememberTracks(items);
    succeedTool(task.toolCallId,
                {{QStringLiteral("items"), items},
                 {QStringLiteral("count"), items.size()}});
}

void AgentToolExecutor::onHistoryListReady(const QVariantList& history)
{
    if(m_pendingHistoryFetch.isEmpty()) return;
    auto task = m_pendingHistoryFetch.dequeue();
    QVariantList items = m_hostStateProvider->convertHistoryList(
        history,
        qBound(1, task.args.value(QStringLiteral("limit"), 10).toInt(), 500));
    rememberTracks(items);
    succeedTool(task.toolCallId,
                {{QStringLiteral("items"), items},
                 {QStringLiteral("count"), items.size()}});
}

void AgentToolExecutor::onFavoritesListReady(const QVariantList& favorites)
{
    if(m_pendingFavoritesFetch.isEmpty()) return;
    auto task = m_pendingFavoritesFetch.dequeue();
    QVariantList items = m_hostStateProvider->convertFavoriteList(
        favorites,
        qBound(1, task.args.value(QStringLiteral("limit"), 500).toInt(), 2000));
    rememberTracks(items);
    succeedTool(task.toolCallId,
                {{QStringLiteral("items"), items},
                 {QStringLiteral("count"), items.size()}});
}

void AgentToolExecutor::onPlaylistsListReady(const QVariantList& playlists, int page, int pageSize, int total)
{
    if(m_pendingPlaylistsFetch.isEmpty()) return;
    auto task = m_pendingPlaylistsFetch.dequeue();
    QVariantList items = m_hostStateProvider->convertPlaylistList(playlists);
    rememberPlaylistMeta(items);
    succeedTool(task.toolCallId,
                {{QStringLiteral("items"), items},
                 {QStringLiteral("page"), page},
                 {QStringLiteral("pageSize"), pageSize},
                 {QStringLiteral("total"), total}});
}
void AgentToolExecutor::onPlaylistDetailReady(const QVariantMap& d){
    const QVariantMap nd=m_hostStateProvider->convertPlaylistDetail(d);
    const qint64 pid=nd.value(QStringLiteral("playlist")).toMap().value(QStringLiteral("playlistId")).toLongLong();
    const QVariantList items=nd.value(QStringLiteral("items")).toList();
    if(pid<=0){
        if(!m_pendingPlaylistDetail.isEmpty()){auto t=m_pendingPlaylistDetail.dequeue();failTool(t.toolCallId,QStringLiteral("playlist_detail_invalid"),QStringLiteral("歌单详情缺少有效 playlistId"),true);}
        if(!m_pendingPlaylistPlay.isEmpty()){auto t=m_pendingPlaylistPlay.dequeue();failTool(t.toolCallId,QStringLiteral("playlist_detail_invalid"),QStringLiteral("歌单详情缺少有效 playlistId"),true);}
        return;
    }
    rememberPlaylistDetail(nd);
    rememberTracks(items);

    if(!m_pendingPlaylistDetail.isEmpty()){
        int idx=-1;
        for(int i=0;i<m_pendingPlaylistDetail.size();++i){if(parsePlaylistId(m_pendingPlaylistDetail.at(i).args)==pid){idx=i;break;}}
        if(idx>=0){auto t=m_pendingPlaylistDetail.takeAt(idx);succeedTool(t.toolCallId,nd);}
    }

    if(!m_pendingPlaylistPlay.isEmpty()){
        int idx=-1;
        for(int i=0;i<m_pendingPlaylistPlay.size();++i){if(parsePlaylistId(m_pendingPlaylistPlay.at(i).args)==pid){idx=i;break;}}
        if(idx>=0){
            auto t=m_pendingPlaylistPlay.takeAt(idx);
            HostServiceProxy hostService(m_hostService);
            if(!hostService){failTool(t.toolCallId,QStringLiteral("service_unavailable"),QStringLiteral("clientAutomationHost ?????"));return;}
            if(items.isEmpty()||!hostService->playPlaylist(items)){failTool(t.toolCallId,QStringLiteral("playlist_empty"),QStringLiteral("???????????"));return;}
            succeedTool(t.toolCallId,{{QStringLiteral("played"),true},{QStringLiteral("playlist"),nd.value(QStringLiteral("playlist")).toMap()}});
        }
    }
}

void AgentToolExecutor::onRecommendationListReady(const QVariantMap& meta, const QVariantList& itemsSource)
{
    if(m_pendingRecommendationList.isEmpty()) return;
    auto task = m_pendingRecommendationList.dequeue();
    QVariantList items = m_hostStateProvider->convertHistoryList(
        itemsSource,
        qBound(1, task.args.value(QStringLiteral("limit"), 24).toInt(), 200));
    const int count = qMin(items.size(), itemsSource.size());
    for(int i = 0; i < count; ++i){
        QVariantMap row = items.at(i).toMap();
        QString songId = itemsSource.at(i).toMap().value(QStringLiteral("song_id")).toString().trimmed();
        if(!songId.isEmpty()) row.insert(QStringLiteral("trackId"), songId);
        items[i] = row;
    }
    rememberTracks(items);
    succeedTool(task.toolCallId,
                {{QStringLiteral("meta"), meta},
                 {QStringLiteral("items"), items},
                 {QStringLiteral("count"), items.size()}});
}

void AgentToolExecutor::onSimilarRecommendationListReady(const QVariantMap& meta,
                                                         const QVariantList& itemsSource,
                                                         const QString& anchorSongId)
{
    if(m_pendingSimilarRecommendationList.isEmpty()) return;
    int taskIndex = -1;
    for(int i = 0; i < m_pendingSimilarRecommendationList.size(); ++i){
        if(m_pendingSimilarRecommendationList.at(i).args.value(QStringLiteral("songId")).toString().trimmed()
           == anchorSongId.trimmed()){
            taskIndex = i;
            break;
        }
    }
    if(taskIndex < 0) taskIndex = 0;
    auto task = m_pendingSimilarRecommendationList.takeAt(taskIndex);
    QVariantList items = m_hostStateProvider->convertHistoryList(
        itemsSource,
        qBound(1, task.args.value(QStringLiteral("limit"), 12).toInt(), 100));
    rememberTracks(items);
    succeedTool(task.toolCallId,
                {{QStringLiteral("meta"), meta},
                 {QStringLiteral("anchorSongId"), anchorSongId},
                 {QStringLiteral("items"), items},
                 {QStringLiteral("count"), items.size()}});
}
void AgentToolExecutor::onLyricsReady(const QStringList& lines){
    if(m_pendingLyricsFetch.isEmpty()) return;
    auto task=m_pendingLyricsFetch.dequeue();
    succeedTool(task.toolCallId,{{QStringLiteral("musicPath"),task.args.value(QStringLiteral("musicPath")).toString()},
                                 {QStringLiteral("lines"),lines},
                                 {QStringLiteral("lineCount"),lines.size()},
                                 {QStringLiteral("text"),lines.join(QStringLiteral("\n"))}});
}
void AgentToolExecutor::onVideoListReady(const QVariantList& videoList){
    if(m_pendingVideoListFetch.isEmpty()) return;
    auto task=m_pendingVideoListFetch.dequeue();
    succeedTool(task.toolCallId,{{QStringLiteral("items"),videoList},
                                 {QStringLiteral("count"),videoList.size()}});
}
void AgentToolExecutor::onVideoStreamUrlReady(const QString& videoUrl){
    if(m_pendingVideoStreamFetch.isEmpty()) return;
    auto task=m_pendingVideoStreamFetch.dequeue();
    succeedTool(task.toolCallId,{{QStringLiteral("videoPath"),task.args.value(QStringLiteral("videoPath")).toString()},
                                 {QStringLiteral("videoUrl"),videoUrl},
                                 {QStringLiteral("resolved"),!videoUrl.trimmed().isEmpty()}});
}
void AgentToolExecutor::onArtistSearchReady(bool exists, const QString& artist){
    if(m_pendingArtistSearch.isEmpty()) return;
    int idx=-1;
    for(int i=0;i<m_pendingArtistSearch.size();++i){if(m_pendingArtistSearch.at(i).args.value(QStringLiteral("artist")).toString().trimmed()==artist.trimmed()){idx=i;break;}}
    if(idx<0) idx=0;
    auto task=m_pendingArtistSearch.takeAt(idx);
    succeedTool(task.toolCallId,{{QStringLiteral("artist"),artist},
                                 {QStringLiteral("exists"),exists}});
}
void AgentToolExecutor::onArtistTracksReady(const QVariantList& musicList, const QString& artist){
    if(m_pendingArtistTracks.isEmpty()) return;
    int idx=-1;
    for(int i=0;i<m_pendingArtistTracks.size();++i){if(m_pendingArtistTracks.at(i).args.value(QStringLiteral("artist")).toString().trimmed()==artist.trimmed()){idx=i;break;}}
    if(idx<0) idx=0;
    auto task=m_pendingArtistTracks.takeAt(idx);
    QVariantList items=musicList.mid(0,qBound(1,task.args.value(QStringLiteral("limit"),200).toInt(),2000));
    rememberTracks(items);
    succeedTool(task.toolCallId,{{QStringLiteral("artist"),artist},
                                 {QStringLiteral("items"),items},
                                 {QStringLiteral("count"),items.size()}});
}

void AgentToolExecutor::onAddFavoriteResultReady(bool success){
    if(m_pendingFavoriteAdd.isEmpty()) return;
    auto task=m_pendingFavoriteAdd.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("musicPath"),task.args.value(QStringLiteral("musicPath")).toString()},
                                     {QStringLiteral("title"),task.args.value(QStringLiteral("title")).toString()}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("add_favorite_failed"),QStringLiteral("添加喜欢音乐失败"),true);
}
void AgentToolExecutor::onRemoveFavoriteResultReady(bool success){
    if(m_pendingFavoriteRemove.isEmpty()) return;
    auto task=m_pendingFavoriteRemove.dequeue();
    if(success){
        const QStringList paths=variantToStringList(task.args.value(QStringLiteral("musicPaths")));
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("musicPaths"),paths},
                                     {QStringLiteral("removedCount"),paths.size()}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("remove_favorite_failed"),QStringLiteral("移除喜欢音乐失败"),true);
}
void AgentToolExecutor::onAddHistoryResultReady(bool success){
    if(m_pendingHistoryAdd.isEmpty()) return;
    auto task=m_pendingHistoryAdd.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("musicPath"),task.args.value(QStringLiteral("musicPath")).toString()},
                                     {QStringLiteral("title"),task.args.value(QStringLiteral("title")).toString()}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("add_history_failed"),QStringLiteral("添加最近播放失败"),true);
}
void AgentToolExecutor::onRemoveHistoryResultReady(bool success){
    if(m_pendingHistoryRemove.isEmpty()) return;
    auto task=m_pendingHistoryRemove.dequeue();
    if(success){
        const QStringList paths=variantToStringList(task.args.value(QStringLiteral("musicPaths")));
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("musicPaths"),paths},
                                     {QStringLiteral("removedCount"),paths.size()}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("remove_history_failed"),QStringLiteral("删除最近播放失败"),true);
}
void AgentToolExecutor::onCreatePlaylistResultReady(bool success,qint64 playlistId,const QString& message){
    if(m_pendingCreatePlaylist.isEmpty()) return;
    auto task=m_pendingCreatePlaylist.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("playlistId"),playlistId},
                                     {QStringLiteral("name"),task.args.value(QStringLiteral("name")).toString()},
                                     {QStringLiteral("message"),message}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("create_playlist_failed"),message.trimmed().isEmpty()?QStringLiteral("创建歌单失败"):message,true);
}
void AgentToolExecutor::onDeletePlaylistResultReady(bool success,qint64 playlistId,const QString& message){
    if(m_pendingDeletePlaylist.isEmpty()) return;
    auto task=m_pendingDeletePlaylist.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("playlistId"),playlistId},
                                     {QStringLiteral("message"),message}});
        m_playlistMetaById.remove(playlistId);
        m_playlistDetailById.remove(playlistId);
        return;
    }
    failTool(task.toolCallId,QStringLiteral("delete_playlist_failed"),message.trimmed().isEmpty()?QStringLiteral("删除歌单失败"):message,true);
}
void AgentToolExecutor::onUpdatePlaylistResultReady(bool success,qint64 playlistId,const QString& message){
    if(m_pendingUpdatePlaylist.isEmpty()) return;
    auto task=m_pendingUpdatePlaylist.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("playlistId"),playlistId},
                                     {QStringLiteral("name"),task.args.value(QStringLiteral("name")).toString()},
                                     {QStringLiteral("message"),message}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("update_playlist_failed"),message.trimmed().isEmpty()?QStringLiteral("更新歌单失败"):message,true);
}
void AgentToolExecutor::onAddPlaylistItemsResultReady(bool success,qint64 playlistId,int addedCount,int skippedCount,const QString& message){
    if(m_pendingAddPlaylistItems.isEmpty()) return;
    auto task=m_pendingAddPlaylistItems.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("playlistId"),playlistId},
                                     {QStringLiteral("addedCount"),addedCount},
                                     {QStringLiteral("skippedCount"),skippedCount},
                                     {QStringLiteral("message"),message}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("add_playlist_items_failed"),message.trimmed().isEmpty()?QStringLiteral("歌单加歌失败"):message,true);
}
void AgentToolExecutor::onRemovePlaylistItemsResultReady(bool success,qint64 playlistId,int deletedCount,const QString& message){
    if(m_pendingRemovePlaylistItems.isEmpty()) return;
    auto task=m_pendingRemovePlaylistItems.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("playlistId"),playlistId},
                                     {QStringLiteral("deletedCount"),deletedCount},
                                     {QStringLiteral("message"),message}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("remove_playlist_items_failed"),message.trimmed().isEmpty()?QStringLiteral("歌单删歌失败"):message,true);
}
void AgentToolExecutor::onReorderPlaylistItemsResultReady(bool success,qint64 playlistId,const QString& message){
    if(m_pendingReorderPlaylistItems.isEmpty()) return;
    auto task=m_pendingReorderPlaylistItems.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("playlistId"),playlistId},
                                     {QStringLiteral("message"),message}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("reorder_playlist_items_failed"),message.trimmed().isEmpty()?QStringLiteral("歌单排序失败"):message,true);
}
void AgentToolExecutor::onRecommendationFeedbackResultReady(bool success,const QString& eventType,const QString& songId){
    if(m_pendingRecommendationFeedback.isEmpty()) return;
    auto task=m_pendingRecommendationFeedback.dequeue();
    if(success){
        succeedTool(task.toolCallId,{{QStringLiteral("success"),true},
                                     {QStringLiteral("eventType"),eventType},
                                     {QStringLiteral("songId"),songId}});
        return;
    }
    failTool(task.toolCallId,QStringLiteral("recommendation_feedback_failed"),QStringLiteral("提交推荐反馈失败"),true);
}

void AgentToolExecutor::clearPendingTasks(){m_pendingSearch.clear();m_pendingLyricsFetch.clear();m_pendingVideoListFetch.clear();m_pendingVideoStreamFetch.clear();m_pendingArtistSearch.clear();m_pendingArtistTracks.clear();m_pendingHistoryFetch.clear();m_pendingHistoryAdd.clear();m_pendingHistoryRemove.clear();m_pendingFavoritesFetch.clear();m_pendingFavoriteAdd.clear();m_pendingFavoriteRemove.clear();m_pendingPlaylistsFetch.clear();m_pendingPlaylistDetail.clear();m_pendingPlaylistPlay.clear();m_pendingCreatePlaylist.clear();m_pendingDeletePlaylist.clear();m_pendingUpdatePlaylist.clear();m_pendingAddPlaylistItems.clear();m_pendingRemovePlaylistItems.clear();m_pendingReorderPlaylistItems.clear();m_pendingRecommendationList.clear();m_pendingSimilarRecommendationList.clear();m_pendingRecommendationFeedback.clear();m_trackCacheById.clear();m_playlistMetaById.clear();m_playlistDetailById.clear();}
bool AgentToolExecutor::requireLogin(const QString& tcid){const QString a=m_hostStateProvider?m_hostStateProvider->currentUserAccount():QString();if(!a.trimmed().isEmpty()) return true;failTool(tcid,QStringLiteral("not_logged_in"),QStringLiteral("当前用户未登录，无法执行该操作"));return false;}
void AgentToolExecutor::failTool(const QString& id,const QString& code,const QString& msg,bool r){emit toolResultReady(id,false,QVariantMap(),makeError(code,msg,r));}
void AgentToolExecutor::succeedTool(const QString& id,const QVariantMap& result){emit toolResultReady(id,true,result,QVariantMap());}
QVariantMap AgentToolExecutor::makeError(const QString& code,const QString& msg,bool r){QVariantMap e;e.insert(QStringLiteral("code"),code);e.insert(QStringLiteral("message"),msg);e.insert(QStringLiteral("retryable"),r);return e;}
qint64 AgentToolExecutor::parsePlaylistId(const QVariantMap& a){bool ok=false;qint64 id=a.value(QStringLiteral("playlistId")).toLongLong(&ok);if(ok&&id>0) return id;id=a.value(QStringLiteral("playlist_id")).toLongLong(&ok);if(ok&&id>0) return id;id=a.value(QStringLiteral("id")).toLongLong(&ok);if(ok&&id>0) return id;return 0;}
QUrl AgentToolExecutor::toPlayableUrl(const QString& raw){
    const QString t=raw.trimmed();
    if(t.isEmpty()) return QUrl();

    QUrl u(t);
    if(u.isValid()&&!u.scheme().isEmpty()) return u;

    QFileInfo fi(t);
    if(fi.isAbsolute()){
        return QUrl::fromLocalFile(fi.absoluteFilePath());
    }

    QString base=SettingsManager::instance().serverBaseUrl().trimmed();
    if(base.isEmpty()){
        base=QStringLiteral("http://127.0.0.1:8080/");
    }
    if(!base.endsWith('/')){
        base.append('/');
    }

    QString rel=t;
    while(rel.startsWith('/')){
        rel.remove(0,1);
    }
    const QString lower=rel.toLower();
    if(lower.startsWith(QStringLiteral("uploads/"))||
       lower.startsWith(QStringLiteral("video/"))||
       lower.startsWith(QStringLiteral("hls/"))){
        return QUrl(base+rel);
    }

    if(rel.contains('/')){
        return QUrl(base+QStringLiteral("uploads/")+rel);
    }

    return QUrl(base+QStringLiteral("uploads/")+rel);
}
QStringList AgentToolExecutor::variantToStringList(const QVariant& v){QStringList out;if(!v.isValid()) return out;if(v.type()==QVariant::StringList){for(const QString& i:v.toStringList()){QString t=i.trimmed();if(!t.isEmpty()) out.push_back(t);}return out;}if(v.type()==QVariant::List){for(const QVariant& i:v.toList()){QString t=i.toString().trimmed();if(!t.isEmpty()) out.push_back(t);}return out;}QString t=v.toString().trimmed();if(!t.isEmpty()) out.push_back(t);return out;}
QString AgentToolExecutor::playModeName(int m){switch(m){case 0:return QStringLiteral("sequential");case 1:return QStringLiteral("repeat_one");case 2:return QStringLiteral("repeat_all");case 3:return QStringLiteral("shuffle");default:return QStringLiteral("unknown");}}
int AgentToolExecutor::parsePlayModeValue(const QVariant& value){
    bool ok=false;
    const int numeric=value.toInt(&ok);
    if(ok&&numeric>=0&&numeric<=3){
        return numeric;
    }

    const QString text=value.toString().trimmed().toLower();
    if(text==QStringLiteral("sequential")||text==QStringLiteral("sequence")||text==QStringLiteral("0")){
        return 0;
    }
    if(text==QStringLiteral("repeat_one")||text==QStringLiteral("repeatone")||text==QStringLiteral("single")||text==QStringLiteral("1")){
        return 1;
    }
    if(text==QStringLiteral("repeat_all")||text==QStringLiteral("repeatall")||text==QStringLiteral("loop")||text==QStringLiteral("2")){
        return 2;
    }
    if(text==QStringLiteral("shuffle")||text==QStringLiteral("random")||text==QStringLiteral("3")){
        return 3;
    }
    return -1;
}
void AgentToolExecutor::rememberTracks(const QVariantList& tracks){for(const QVariant& v:tracks){QVariantMap t=v.toMap();QString id=t.value(QStringLiteral("trackId")).toString().trimmed();if(!id.isEmpty()) m_trackCacheById.insert(id,t);}}
void AgentToolExecutor::rememberPlaylistMeta(const QVariantList& playlists){for(const QVariant& v:playlists){QVariantMap p=v.toMap();qint64 id=p.value(QStringLiteral("playlistId")).toLongLong();if(id>0) m_playlistMetaById.insert(id,p);}}
void AgentToolExecutor::rememberPlaylistDetail(const QVariantMap& d){QVariantMap p=d.value(QStringLiteral("playlist")).toMap();qint64 id=p.value(QStringLiteral("playlistId")).toLongLong();if(id<=0) return;m_playlistDetailById.insert(id,d);m_playlistMetaById.insert(id,p);} 
QVariantMap AgentToolExecutor::resolveTrackById(const QString& trackId) const{QString id=trackId.trimmed();if(id.isEmpty()) return QVariantMap();return m_trackCacheById.value(id);} 
QVariantMap AgentToolExecutor::resolveTrackByPath(const QString& path) const{QString p=path.trimmed();if(p.isEmpty()) return QVariantMap();for(auto it=m_trackCacheById.constBegin();it!=m_trackCacheById.constEnd();++it){if(it.value().value(QStringLiteral("musicPath")).toString().trimmed()==p) return it.value();}return QVariantMap();}
QVariantMap AgentToolExecutor::buildMusicTabSelector(const QVariantMap& args){QVariantMap selector;if(args.contains(QStringLiteral("trackId"))) selector.insert(QStringLiteral("trackId"),args.value(QStringLiteral("trackId")));if(args.contains(QStringLiteral("musicPath"))) selector.insert(QStringLiteral("musicPath"),args.value(QStringLiteral("musicPath")));if(args.contains(QStringLiteral("playlistId"))) selector.insert(QStringLiteral("playlistId"),args.value(QStringLiteral("playlistId")));if(args.contains(QStringLiteral("playlistName"))) selector.insert(QStringLiteral("playlistName"),args.value(QStringLiteral("playlistName")));return selector;}
bool AgentToolExecutor::musicTabActionSupported(const QString& tab,const QString& action){
    const QString t=tab.trimmed().toLower();
    const QString a=action.trimmed().toLower();
    if(t==QStringLiteral("recommend")||t==QStringLiteral("online_music")) return a==QStringLiteral("toggle_current_playback")||a==QStringLiteral("play_next")||a==QStringLiteral("add_to_playlist")||a==QStringLiteral("download")||a==QStringLiteral("add_favorite")||a==QStringLiteral("remove_favorite");
    if(t==QStringLiteral("local_music")||t==QStringLiteral("downloaded_music")) return a==QStringLiteral("toggle_current_playback")||a==QStringLiteral("play_next")||a==QStringLiteral("add_to_playlist")||a==QStringLiteral("add_favorite")||a==QStringLiteral("remove_favorite")||a==QStringLiteral("remove_or_delete");
    if(t==QStringLiteral("history")) return a==QStringLiteral("toggle_current_playback")||a==QStringLiteral("play_next")||a==QStringLiteral("add_favorite")||a==QStringLiteral("remove_favorite")||a==QStringLiteral("remove_or_delete");
    if(t==QStringLiteral("favorites")) return a==QStringLiteral("toggle_current_playback")||a==QStringLiteral("play_next")||a==QStringLiteral("remove_favorite");
    if(t==QStringLiteral("playlist_detail")) return a==QStringLiteral("toggle_current_playback")||a==QStringLiteral("play_next")||a==QStringLiteral("add_to_playlist")||a==QStringLiteral("download")||a==QStringLiteral("add_favorite")||a==QStringLiteral("remove_favorite")||a==QStringLiteral("remove_or_delete");
    return false;
}
QVariantMap AgentToolExecutor::resolveMusicTabItem(const QString& tab,const QVariantMap& args,QString* errorCode,QString* errorMessage) const{
    HostServiceProxy hostService(m_hostService);
    if(!hostService){
        if(errorCode) *errorCode=QStringLiteral("service_unavailable");
        if(errorMessage) *errorMessage=QStringLiteral("clientAutomationHost ?????");
        return {};
    }
    const QVariantMap selector=buildMusicTabSelector(args);
    if(selector.isEmpty()){
        if(errorCode) *errorCode=(tab.trimmed().toLower()==QStringLiteral("playlist_detail"))?QStringLiteral("playlist_context_required"):QStringLiteral("invalid_args");
        if(errorMessage) *errorMessage=(tab.trimmed().toLower()==QStringLiteral("playlist_detail"))?QStringLiteral("????????????????"):QStringLiteral("???? trackId/musicPath/playlistId/playlistName ??");
        return {};
    }
    const QVariantMap item=hostService->resolveMusicTabItem(tab.trimmed().toLower(),selector);
    if(item.isEmpty()){
        if(errorCode) *errorCode=(tab.trimmed().toLower()==QStringLiteral("playlist_detail"))?QStringLiteral("playlist_context_required"):QStringLiteral("tab_item_not_found");
        if(errorMessage) *errorMessage=(tab.trimmed().toLower()==QStringLiteral("playlist_detail"))?QStringLiteral("??????????????????????"):QStringLiteral("?? tab ?????????");
    }
    return item;
}
QVariantList AgentToolExecutor::convertQueueToItems(const QList<QUrl>& urls) const{QVariantList out;out.reserve(urls.size());for(const QUrl& u:urls){QString p=u.toString();QVariantMap t=resolveTrackByPath(p);if(t.isEmpty()){t.insert(QStringLiteral("trackId"),sid(p));t.insert(QStringLiteral("musicPath"),p);t.insert(QStringLiteral("title"),QFileInfo(u.path()).completeBaseName());t.insert(QStringLiteral("artist"),QStringLiteral("未知艺术家"));t.insert(QStringLiteral("album"),QString());t.insert(QStringLiteral("durationMs"),0);t.insert(QStringLiteral("coverUrl"),QString());}out.push_back(t);}return out;}
QList<QUrl> AgentToolExecutor::buildQueueUrls(const QVariantList& ids,QString* err) const{
    QList<QUrl> urls;
    for(const QVariant& value:ids){
        const QString trackId=value.toString().trimmed();
        if(trackId.isEmpty()) continue;
        const QVariantMap track=resolveTrackById(trackId);
        if(track.isEmpty()){
            if(err) *err=QStringLiteral("trackId 无法解析：%1").arg(trackId);
            return {};
        }
        const QUrl url=toPlayableUrl(track.value(QStringLiteral("musicPath")).toString());
        if(url.isEmpty()){
            if(err) *err=QStringLiteral("trackId 对应歌曲路径无效：%1").arg(trackId);
            return {};
        }
        urls.push_back(url);
    }
    if(urls.isEmpty()&&err){
        *err=QStringLiteral("trackIds 为空或无法解析");
    }
    return urls;
}
QVariantMap AgentToolExecutor::buildQueueSnapshot() const{
    HostServiceProxy hostService(m_hostService);
    if(!hostService){
        return {};
    }
    return hostService->playbackQueueSnapshot();
}
QVariantList AgentToolExecutor::buildPlaylistItemsFromTrackIds(const QVariantList& ids,QString* err) const{QVariantList out;for(const QVariant& v:ids){QString id=v.toString().trimmed();if(id.isEmpty()) continue;QVariantMap t=resolveTrackById(id);if(t.isEmpty()){if(err) *err=QStringLiteral("trackId 无法解析：%1").arg(id);return QVariantList();}QString path=t.value(QStringLiteral("musicPath")).toString().trimmed();if(path.isEmpty()) continue;QVariantMap item;item.insert(QStringLiteral("music_path"),path);item.insert(QStringLiteral("music_title"),t.value(QStringLiteral("title")).toString());item.insert(QStringLiteral("artist"),t.value(QStringLiteral("artist")).toString());item.insert(QStringLiteral("album"),t.value(QStringLiteral("album")).toString());item.insert(QStringLiteral("duration_sec"),t.value(QStringLiteral("durationMs")).toLongLong()/1000);item.insert(QStringLiteral("is_local"),!path.startsWith(QStringLiteral("http"),Qt::CaseInsensitive));item.insert(QStringLiteral("cover_art_path"),t.value(QStringLiteral("coverUrl")).toString());out.push_back(item);}if(out.isEmpty()&&err)*err=QStringLiteral("trackIds 为空或无法解析");return out;}
QStringList AgentToolExecutor::buildMusicPathsFromTrackIds(const QVariantList& ids) const{QStringList out;for(const QVariant& v:ids){QString id=v.toString().trimmed();if(id.isEmpty()) continue;QString p=resolveTrackById(id).value(QStringLiteral("musicPath")).toString().trimmed();if(!p.isEmpty()) out.push_back(p);}return out;}
