#include "AgentToolExecutor.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "AudioService.h"
#include "HostStateProvider.h"
#include "download_manager.h"
#include "httprequest_v2.h"
#include "local_music_cache.h"
#include "MainShellViewModel.h"
#include "plugin_manager.h"
#include "settings_manager.h"
#include "ToolRegistry.h"
#include "music.h"

namespace {
QString sid(const QString& s){
    return QString::fromLatin1(QCryptographicHash::hash(s.toUtf8(),QCryptographicHash::Md5).toHex());
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

QObject* mainWidgetService()
{
    PluginHostContext* hostContext = PluginManager::instance().hostContext();
    return hostContext ? hostContext->service(QStringLiteral("mainWidget")) : nullptr;
}

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
        {QStringLiteral("playerPageStyle"), settings.playerPageStyle()}
    };
}
}

AgentToolExecutor::AgentToolExecutor(HostStateProvider* h,QObject* p)
    :QObject(p),m_shellViewModel(nullptr),m_hostStateProvider(h),m_toolRegistry(new ToolRegistry()){}
AgentToolExecutor::~AgentToolExecutor(){delete m_toolRegistry; m_toolRegistry=nullptr;}

void AgentToolExecutor::setMainShellViewModel(MainShellViewModel* s){
    if(m_shellViewModel==s) return;
    if(m_shellViewModel){disconnect(m_shellViewModel,nullptr,this,nullptr);}
    if(m_requestGateway){disconnect(m_requestGateway,nullptr,this,nullptr); m_requestGateway=nullptr;}
    if(m_shellViewModel){ clearPendingTasks(); }
    m_shellViewModel=s;
    if(!m_shellViewModel) return;
    m_requestGateway=m_shellViewModel->requestGateway();

    connect(s,&MainShellViewModel::searchResultsReady,this,&AgentToolExecutor::onSearchResultsReady);
    connect(s,&MainShellViewModel::historyListReady,this,&AgentToolExecutor::onHistoryListReady);
    connect(s,&MainShellViewModel::favoritesListReady,this,&AgentToolExecutor::onFavoritesListReady);
    connect(s,&MainShellViewModel::playlistsListReady,this,&AgentToolExecutor::onPlaylistsListReady);
    connect(s,&MainShellViewModel::playlistDetailReady,this,&AgentToolExecutor::onPlaylistDetailReady);
    connect(s,&MainShellViewModel::recommendationListReady,this,&AgentToolExecutor::onRecommendationListReady);
    connect(s,&MainShellViewModel::similarRecommendationListReady,this,&AgentToolExecutor::onSimilarRecommendationListReady);

    connect(s,&MainShellViewModel::addFavoriteResultReady,this,&AgentToolExecutor::onAddFavoriteResultReady);
    connect(s,&MainShellViewModel::removeFavoriteResultReady,this,&AgentToolExecutor::onRemoveFavoriteResultReady);
    connect(s,&MainShellViewModel::addHistoryResultReady,this,&AgentToolExecutor::onAddHistoryResultReady);
    connect(s,&MainShellViewModel::removeHistoryResultReady,this,&AgentToolExecutor::onRemoveHistoryResultReady);
    connect(s,&MainShellViewModel::createPlaylistResultReady,this,&AgentToolExecutor::onCreatePlaylistResultReady);
    connect(s,&MainShellViewModel::deletePlaylistResultReady,this,&AgentToolExecutor::onDeletePlaylistResultReady);
    connect(s,&MainShellViewModel::updatePlaylistResultReady,this,&AgentToolExecutor::onUpdatePlaylistResultReady);
    connect(s,&MainShellViewModel::addPlaylistItemsResultReady,this,&AgentToolExecutor::onAddPlaylistItemsResultReady);
    connect(s,&MainShellViewModel::removePlaylistItemsResultReady,this,&AgentToolExecutor::onRemovePlaylistItemsResultReady);
    connect(s,&MainShellViewModel::reorderPlaylistItemsResultReady,this,&AgentToolExecutor::onReorderPlaylistItemsResultReady);
    connect(s,&MainShellViewModel::recommendationFeedbackResultReady,this,&AgentToolExecutor::onRecommendationFeedbackResultReady);

    if(m_requestGateway){
        connect(m_requestGateway,&HttpRequestV2::signalLrc,this,&AgentToolExecutor::onLyricsReady);
        connect(m_requestGateway,&HttpRequestV2::signalVideoList,this,&AgentToolExecutor::onVideoListReady);
        connect(m_requestGateway,&HttpRequestV2::signalVideoStreamUrl,this,&AgentToolExecutor::onVideoStreamUrlReady);
        connect(m_requestGateway,&HttpRequestV2::signalArtistExists,this,&AgentToolExecutor::onArtistSearchReady);
        connect(m_requestGateway,&HttpRequestV2::signalArtistMusicList,this,&AgentToolExecutor::onArtistTracksReady);
    }
}

bool AgentToolExecutor::executeToolCall(const QString& toolCallId,const QString& tool,const QVariantMap& args){
    const QString id=toolCallId.trimmed();
    const QString name=tool.trimmed();
    if(id.isEmpty()||name.isEmpty()) return false;

    if(!m_toolRegistry->contains(name)){failTool(id,QStringLiteral("unsupported_tool"),QStringLiteral("Qt 端暂未支持工具：%1").arg(name));return true;}
    QString ec,em;
    if(!m_toolRegistry->validateArgs(name,args,&ec,&em)){failTool(id,ec,em);return true;}
    if(!m_hostStateProvider){failTool(id,QStringLiteral("host_state_unavailable"),QStringLiteral("HostStateProvider 不可用"));return true;}

    if(name==QStringLiteral("getCurrentTrack")){succeedTool(id,m_hostStateProvider->currentTrackSnapshot());return true;}
    if(name==QStringLiteral("pausePlayback")){AudioService::instance().pause();succeedTool(id,{{QStringLiteral("ok"),true}});return true;}
    if(name==QStringLiteral("resumePlayback")){AudioService::instance().resume();succeedTool(id,{{QStringLiteral("ok"),true}});return true;}
    if(name==QStringLiteral("stopPlayback")){AudioService::instance().stop();succeedTool(id,{{QStringLiteral("ok"),true}});return true;}
    if(name==QStringLiteral("seekPlayback")){const qint64 p=qMax<qint64>(0,args.value(QStringLiteral("positionMs")).toLongLong());AudioService::instance().seekTo(p);succeedTool(id,{{QStringLiteral("positionMs"),p}});return true;}
    if(name==QStringLiteral("playNext")){AudioService::instance().playNext();succeedTool(id,{{QStringLiteral("ok"),true}});return true;}
    if(name==QStringLiteral("playPrevious")){AudioService::instance().playPrevious();succeedTool(id,{{QStringLiteral("ok"),true}});return true;}
    if(name==QStringLiteral("playAtIndex")){const int i=args.value(QStringLiteral("index")).toInt();AudioService::instance().playAtIndex(i);succeedTool(id,{{QStringLiteral("index"),i}});return true;}
    if(name==QStringLiteral("setVolume")){const int v=qBound(0,args.value(QStringLiteral("volume")).toInt(),100);AudioService::instance().setVolume(v);succeedTool(id,{{QStringLiteral("volume"),v}});return true;}
    if(name==QStringLiteral("setPlayMode")){
        const int mode=parsePlayModeValue(args.value(QStringLiteral("mode")));
        if(mode<0){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("mode 仅支持 sequential/repeat_one/repeat_all/shuffle 或对应整数"));return true;}
        AudioService::instance().setPlayMode(static_cast<AudioService::PlayMode>(mode));
        succeedTool(id,{{QStringLiteral("playMode"),mode},{QStringLiteral("playModeName"),playModeName(mode)}});
        return true;
    }
    if(name==QStringLiteral("getPlaybackQueue")){succeedTool(id,buildQueueSnapshot());return true;}
    if(name==QStringLiteral("setPlaybackQueue")){
        QString errorMessage;
        const QList<QUrl> urls=buildQueueUrls(args.value(QStringLiteral("trackIds")).toList(),&errorMessage);
        if(urls.isEmpty()){failTool(id,QStringLiteral("invalid_args"),errorMessage.isEmpty()?QStringLiteral("trackIds 无法解析为播放队列"):errorMessage);return true;}
        AudioService& service=AudioService::instance();
        service.clearPlaylist();
        for(const QUrl& url:urls){service.addToPlaylist(url);}
        const int startIndex=qBound(0,args.value(QStringLiteral("startIndex"),0).toInt(),urls.size()-1);
        if(args.value(QStringLiteral("playNow"),true).toBool()){
            service.playAtIndex(startIndex);
        }
        QVariantMap snapshot=buildQueueSnapshot();
        snapshot.insert(QStringLiteral("queueReset"),true);
        snapshot.insert(QStringLiteral("startIndex"),startIndex);
        snapshot.insert(QStringLiteral("playNow"),args.value(QStringLiteral("playNow"),true).toBool());
        succeedTool(id,snapshot);
        return true;
    }
    if(name==QStringLiteral("addToPlaybackQueue")){
        QString path=args.value(QStringLiteral("musicPath")).toString().trimmed();
        if(path.isEmpty()){
            path=resolveTrackById(args.value(QStringLiteral("trackId")).toString()).value(QStringLiteral("musicPath")).toString().trimmed();
        }
        const QUrl url=toPlayableUrl(path);
        if(url.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("无法解析待加入队列的歌曲路径"));return true;}
        AudioService::instance().addToPlaylist(url);
        QVariantMap snapshot=buildQueueSnapshot();
        snapshot.insert(QStringLiteral("added"),true);
        snapshot.insert(QStringLiteral("musicPath"),path);
        succeedTool(id,snapshot);
        return true;
    }
    if(name==QStringLiteral("removeFromPlaybackQueue")){
        const int index=args.value(QStringLiteral("index")).toInt();
        const QList<QUrl> queueBefore=AudioService::instance().playlist();
        if(index<0||index>=queueBefore.size()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("index 超出当前播放队列范围"));return true;}
        QVariantMap removedItem=convertQueueToItems(queueBefore).value(index).toMap();
        AudioService::instance().removeFromPlaylist(index);
        QVariantMap snapshot=buildQueueSnapshot();
        snapshot.insert(QStringLiteral("removed"),true);
        snapshot.insert(QStringLiteral("removedIndex"),index);
        snapshot.insert(QStringLiteral("removedItem"),removedItem);
        succeedTool(id,snapshot);
        return true;
    }
    if(name==QStringLiteral("clearPlaybackQueue")){
        AudioService::instance().clearPlaylist();
        succeedTool(id,{{QStringLiteral("cleared"),true},{QStringLiteral("items"),QVariantList()},{QStringLiteral("count"),0},{QStringLiteral("currentIndex"),-1},{QStringLiteral("playing"),AudioService::instance().isPlaying()},{QStringLiteral("volume"),AudioService::instance().volume()},{QStringLiteral("playMode"),static_cast<int>(AudioService::instance().playMode())},{QStringLiteral("playModeName"),playModeName(static_cast<int>(AudioService::instance().playMode()))}});
        return true;
    }
    if(name==QStringLiteral("getLocalTracks")){const int l=qBound(1,args.value(QStringLiteral("limit"),500).toInt(),5000);QVariantList it=m_hostStateProvider->convertLocalMusicList(l);rememberTracks(it);succeedTool(id,{{QStringLiteral("items"),it},{QStringLiteral("count"),it.size()}});return true;}
    if(name==QStringLiteral("addLocalTrack")){
        const QString filePath=args.value(QStringLiteral("filePath")).toString().trimmed();
        if(filePath.isEmpty()||!QFileInfo::exists(filePath)){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("filePath 不存在或不可访问"));return true;}
        LocalMusicInfo info;
        info.filePath=filePath;
        info.fileName=args.value(QStringLiteral("fileName")).toString().trimmed();
        if(info.fileName.isEmpty()) info.fileName=QFileInfo(filePath).fileName();
        info.artist=args.value(QStringLiteral("artist")).toString().trimmed();
        if(info.artist.isEmpty()) info.artist=QStringLiteral("未知艺术家");
        LocalMusicCache::instance().addMusic(info);
        QVariantList items=m_hostStateProvider->convertLocalMusicList(5000);
        QVariantMap track;
        for(const QVariant& value:items){
            const QVariantMap item=value.toMap();
            if(item.value(QStringLiteral("musicPath")).toString().trimmed()==filePath){
                track=item;
                break;
            }
        }
        rememberTracks(items);
        succeedTool(id,{{QStringLiteral("added"),true},{QStringLiteral("track"),track},{QStringLiteral("count"),items.size()}});
        return true;
    }
    if(name==QStringLiteral("removeLocalTrack")){
        const QString filePath=args.value(QStringLiteral("filePath")).toString().trimmed();
        if(filePath.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("filePath 不能为空"));return true;}
        LocalMusicCache::instance().removeMusic(filePath);
        QVariantList items=m_hostStateProvider->convertLocalMusicList(5000);
        rememberTracks(items);
        succeedTool(id,{{QStringLiteral("removed"),true},{QStringLiteral("filePath"),filePath},{QStringLiteral("count"),items.size()}});
        return true;
    }
    if(name==QStringLiteral("getDownloadTasks")){
        const QString scope=args.value(QStringLiteral("scope"),QStringLiteral("all")).toString().trimmed().toLower();
        QList<DownloadTask> tasks;
        if(scope==QStringLiteral("active")) tasks=DownloadManager::instance().getActiveTasks();
        else if(scope==QStringLiteral("completed")) tasks=DownloadManager::instance().getCompletedTasks();
        else tasks=DownloadManager::instance().getAllTasks();
        succeedTool(id,{{QStringLiteral("scope"),scope},{QStringLiteral("items"),convertDownloadTasks(tasks)},{QStringLiteral("count"),tasks.size()},{QStringLiteral("activeDownloads"),DownloadManager::instance().activeDownloads()},{QStringLiteral("queueSize"),DownloadManager::instance().queueSize()}});
        return true;
    }
    if(name==QStringLiteral("pauseDownloadTask")){
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        DownloadManager::instance().pauseDownload(taskId);
        succeedTool(id,{{QStringLiteral("task"),convertDownloadTask(DownloadManager::instance().getTask(taskId))}});
        return true;
    }
    if(name==QStringLiteral("resumeDownloadTask")){
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        DownloadManager::instance().resumeDownload(taskId);
        succeedTool(id,{{QStringLiteral("task"),convertDownloadTask(DownloadManager::instance().getTask(taskId))}});
        return true;
    }
    if(name==QStringLiteral("cancelDownloadTask")){
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        DownloadManager::instance().cancelDownload(taskId);
        succeedTool(id,{{QStringLiteral("task"),convertDownloadTask(DownloadManager::instance().getTask(taskId))}});
        return true;
    }
    if(name==QStringLiteral("removeDownloadTask")){
        const QString taskId=args.value(QStringLiteral("taskId")).toString().trimmed();
        const DownloadTask task=DownloadManager::instance().getTask(taskId);
        DownloadManager::instance().removeTask(taskId);
        succeedTool(id,{{QStringLiteral("removed"),true},{QStringLiteral("taskId"),taskId},{QStringLiteral("filename"),task.filename}});
        return true;
    }
    if(name==QStringLiteral("getVideoWindowState")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentVideoWindowState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("playVideo")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        QString source=args.value(QStringLiteral("videoUrl")).toString().trimmed();
        if(source.isEmpty()){
            source=args.value(QStringLiteral("videoPath")).toString().trimmed();
            if(!source.isEmpty()&&!source.startsWith(QStringLiteral("http"),Qt::CaseInsensitive)&&!QFileInfo(source).isAbsolute()&&!source.startsWith(QStringLiteral("video/"),Qt::CaseInsensitive)){
                source.prepend(QStringLiteral("video/"));
            }
            QUrl videoUrl=toPlayableUrl(source);
            source=videoUrl.isEmpty()?source:videoUrl.toString();
        }
        bool ok=false;
        QMetaObject::invokeMethod(widget,"agentPlayVideo",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(QString,source));
        if(!ok){failTool(id,QStringLiteral("play_video_failed"),QStringLiteral("视频窗口未就绪或视频地址无效"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentVideoWindowState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("pauseVideoPlayback")||name==QStringLiteral("resumeVideoPlayback")||name==QStringLiteral("closeVideoWindow")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        bool ok=false;
        const char* method=(name==QStringLiteral("pauseVideoPlayback"))?"agentPauseVideo":(name==QStringLiteral("resumeVideoPlayback"))?"agentResumeVideo":"agentCloseVideoWindow";
        QMetaObject::invokeMethod(widget,method,Qt::DirectConnection,Q_RETURN_ARG(bool,ok));
        if(!ok){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("视频窗口当前不可执行该操作"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentVideoWindowState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("seekVideoPlayback")||name==QStringLiteral("setVideoFullScreen")||name==QStringLiteral("setVideoPlaybackRate")||name==QStringLiteral("setVideoQualityPreset")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        bool ok=false;
        if(name==QStringLiteral("seekVideoPlayback")){
            QMetaObject::invokeMethod(widget,"agentSeekVideo",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(qint64,qMax<qint64>(0,args.value(QStringLiteral("positionMs")).toLongLong())));
        }else if(name==QStringLiteral("setVideoFullScreen")){
            QMetaObject::invokeMethod(widget,"agentSetVideoFullScreen",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(bool,args.value(QStringLiteral("enabled")).toBool()));
        }else if(name==QStringLiteral("setVideoPlaybackRate")){
            QMetaObject::invokeMethod(widget,"agentSetVideoPlaybackRate",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(double,args.value(QStringLiteral("rate")).toDouble()));
        }else{
            QMetaObject::invokeMethod(widget,"agentSetVideoQualityPreset",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(QString,args.value(QStringLiteral("preset")).toString()));
        }
        if(!ok){failTool(id,QStringLiteral("video_control_failed"),QStringLiteral("视频窗口未就绪或参数不合法"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentVideoWindowState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("getDesktopLyricsState")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentDesktopLyricsState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("showDesktopLyrics")||name==QStringLiteral("hideDesktopLyrics")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        bool ok=false;
        QMetaObject::invokeMethod(widget,"agentSetDesktopLyricsVisible",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(bool,name==QStringLiteral("showDesktopLyrics")));
        if(!ok){failTool(id,QStringLiteral("desktop_lyrics_failed"),QStringLiteral("桌面歌词窗口不可用"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentDesktopLyricsState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("setDesktopLyricsStyle")){
        QObject* widget=mainWidgetService();
        if(!widget){failTool(id,QStringLiteral("service_unavailable"),QStringLiteral("mainWidget 服务不可用"));return true;}
        QVariantMap style;
        if(args.contains(QStringLiteral("color"))) style.insert(QStringLiteral("color"),args.value(QStringLiteral("color")));
        if(args.contains(QStringLiteral("fontSize"))) style.insert(QStringLiteral("fontSize"),args.value(QStringLiteral("fontSize")));
        if(args.contains(QStringLiteral("fontFamily"))) style.insert(QStringLiteral("fontFamily"),args.value(QStringLiteral("fontFamily")));
        if(style.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("至少需要 color/fontSize/fontFamily 中的一项"));return true;}
        bool ok=false;
        QMetaObject::invokeMethod(widget,"agentSetDesktopLyricsStyle",Qt::DirectConnection,Q_RETURN_ARG(bool,ok),Q_ARG(QVariantMap,style));
        if(!ok){failTool(id,QStringLiteral("desktop_lyrics_failed"),QStringLiteral("桌面歌词样式更新失败"));return true;}
        QVariantMap state;
        QMetaObject::invokeMethod(widget,"agentDesktopLyricsState",Qt::DirectConnection,Q_RETURN_ARG(QVariantMap,state));
        succeedTool(id,state);
        return true;
    }
    if(name==QStringLiteral("getPlugins")){
        QVariantList items;
        const QVector<PluginInfo> plugins=PluginManager::instance().getPluginInfos();
        items.reserve(plugins.size());
        for(const PluginInfo& info:plugins){items.push_back(convertPluginInfo(info));}
        succeedTool(id,{{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()},{QStringLiteral("serviceKeys"),PluginManager::instance().hostContext()->serviceKeys()}});
        return true;
    }
    if(name==QStringLiteral("getPluginDiagnostics")){
        QVariantList failures;
        const QVector<PluginLoadFailure> loadFailures=PluginManager::instance().loadFailures();
        for(const PluginLoadFailure& failure:loadFailures){
            failures.push_back(QVariantMap{{QStringLiteral("pluginId"),failure.pluginId},{QStringLiteral("filePath"),failure.filePath},{QStringLiteral("reason"),failure.reason},{QStringLiteral("timestamp"),failure.timestamp.toString(Qt::ISODate)}});
        }
        succeedTool(id,{{QStringLiteral("report"),PluginManager::instance().diagnosticsReport()},{QStringLiteral("loadFailures"),failures},{QStringLiteral("environment"),PluginManager::instance().hostContext()->environmentSnapshot()}});
        return true;
    }
    if(name==QStringLiteral("reloadPlugins")){
        PluginManager& manager=PluginManager::instance();
        const QString pluginDir=manager.hostContext()->environmentValue(QStringLiteral("pluginDir")).toString().trimmed();
        if(pluginDir.isEmpty()){failTool(id,QStringLiteral("plugin_dir_missing"),QStringLiteral("插件目录未配置"));return true;}
        manager.unloadAllPlugins();
        manager.clearLoadFailures();
        const int loaded=manager.loadPlugins(pluginDir);
        QVariantList items;
        const QVector<PluginInfo> plugins=manager.getPluginInfos();
        for(const PluginInfo& info:plugins){items.push_back(convertPluginInfo(info));}
        succeedTool(id,{{QStringLiteral("pluginDir"),pluginDir},{QStringLiteral("loadedCount"),loaded},{QStringLiteral("items"),items}});
        return true;
    }
    if(name==QStringLiteral("unloadPlugin")){
        PluginManager::instance().unloadPlugin(args.value(QStringLiteral("pluginKey")).toString());
        succeedTool(id,{{QStringLiteral("success"),true},{QStringLiteral("pluginKey"),args.value(QStringLiteral("pluginKey")).toString()}});
        return true;
    }
    if(name==QStringLiteral("unloadAllPlugins")){
        PluginManager::instance().unloadAllPlugins();
        succeedTool(id,{{QStringLiteral("success"),true},{QStringLiteral("count"),PluginManager::instance().pluginCount()}});
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
        else {failTool(id,QStringLiteral("unsupported_setting"),QStringLiteral("不允许通过 Agent 修改该设置项：%1").arg(key));return true;}
        QVariantMap snapshot=settingsSnapshot();
        snapshot.insert(QStringLiteral("updatedKey"),key);
        succeedTool(id,snapshot);
        return true;
    }

    if(!m_shellViewModel){failTool(id,QStringLiteral("shell_not_ready"),QStringLiteral("MainShellViewModel 未注入"));return true;}
    if(!m_requestGateway){failTool(id,QStringLiteral("request_gateway_unavailable"),QStringLiteral("HttpRequestV2 未注入"));return true;}

    if(name==QStringLiteral("getLyrics")){
        QString musicPath=args.value(QStringLiteral("musicPath")).toString().trimmed();
        if(musicPath.isEmpty()){
            musicPath=resolveTrackById(args.value(QStringLiteral("trackId")).toString()).value(QStringLiteral("musicPath")).toString().trimmed();
        }
        if(musicPath.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("缺少可解析的 musicPath/trackId"));return true;}
        m_pendingLyricsFetch.enqueue({id,name,{{QStringLiteral("musicPath"),musicPath}}});
        m_requestGateway->getLyrics(musicPath);
        return true;
    }
    if(name==QStringLiteral("getVideoList")){
        m_pendingVideoListFetch.enqueue({id,name,args});
        m_requestGateway->getVideoList();
        return true;
    }
    if(name==QStringLiteral("getVideoStream")){
        const QString videoPath=args.value(QStringLiteral("videoPath")).toString().trimmed();
        if(videoPath.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("videoPath 不能为空"));return true;}
        m_pendingVideoStreamFetch.enqueue({id,name,args});
        m_requestGateway->getVideoStreamUrl(videoPath);
        return true;
    }
    if(name==QStringLiteral("searchArtist")){
        const QString artist=args.value(QStringLiteral("artist")).toString().trimmed();
        if(artist.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("artist 不能为空"));return true;}
        m_pendingArtistSearch.enqueue({id,name,args});
        m_requestGateway->searchArtist(artist);
        return true;
    }
    if(name==QStringLiteral("getTracksByArtist")){
        const QString artist=args.value(QStringLiteral("artist")).toString().trimmed();
        if(artist.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("artist 不能为空"));return true;}
        m_pendingArtistTracks.enqueue({id,name,args});
        m_requestGateway->getMusicByArtist(artist);
        return true;
    }

    if(name==QStringLiteral("searchTracks")){
        QString q=args.value(QStringLiteral("keyword")).toString().trimmed();
        if(q.isEmpty()) q=args.value(QStringLiteral("artist")).toString().trimmed();
        if(q.isEmpty()) q=args.value(QStringLiteral("album")).toString().trimmed();
        if(q.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("searchTracks 至少需要 keyword/artist/album"));return true;}
        m_pendingSearch.enqueue({id,name,args});
        m_shellViewModel->searchMusic(q);
        return true;
    }

    if(name==QStringLiteral("getRecentTracks")){
        if(!requireLogin(id)) return true;
        m_pendingHistoryFetch.enqueue({id,name,args});
        m_shellViewModel->requestHistory(m_hostStateProvider->currentUserAccount(),qBound(1,args.value(QStringLiteral("limit"),10).toInt(),500),false);
        return true;
    }

    if(name==QStringLiteral("getFavorites")){
        if(!requireLogin(id)) return true;
        m_pendingFavoritesFetch.enqueue({id,name,args});
        m_shellViewModel->requestFavorites(m_hostStateProvider->currentUserAccount(),false);
        return true;
    }

    if(name==QStringLiteral("getPlaylists")){
        if(!requireLogin(id)) return true;
        m_pendingPlaylistsFetch.enqueue({id,name,args});
        m_shellViewModel->requestPlaylists(m_hostStateProvider->currentUserAccount(),1,50,false);
        return true;
    }
    if(name==QStringLiteral("getPlaylistTracks")){
        if(!requireLogin(id)) return true;
        const qint64 pid=parsePlaylistId(args);
        if(pid<=0){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("playlistId 参数非法"));return true;}
        if(m_playlistDetailById.contains(pid)){succeedTool(id,m_playlistDetailById.value(pid));return true;}
        m_pendingPlaylistDetail.enqueue({id,name,args});
        m_shellViewModel->requestPlaylistDetail(m_hostStateProvider->currentUserAccount(),pid,false);
        return true;
    }

    if(name==QStringLiteral("playTrack")){
        QString path=args.value(QStringLiteral("musicPath")).toString().trimmed();
        if(path.isEmpty()) path=resolveTrackById(args.value(QStringLiteral("trackId")).toString()).value(QStringLiteral("musicPath")).toString().trimmed();
        QUrl u=toPlayableUrl(path);
        qDebug() << "[AgentToolExecutor] playTrack requested path =" << path << ", resolved url =" << u;
        if(u.isEmpty()||!AudioService::instance().play(u)){failTool(id,QStringLiteral("play_failed"),QStringLiteral("播放失败"));return true;}
        succeedTool(id,{{QStringLiteral("played"),true},{QStringLiteral("musicPath"),path}});
        return true;
    }

    if(name==QStringLiteral("playPlaylist")){
        if(!requireLogin(id)) return true;
        const qint64 pid=parsePlaylistId(args);
        if(pid<=0){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("playlistId 参数非法"));return true;}
        if(m_playlistDetailById.contains(pid)){
            QList<QUrl> urls;
            const QVariantMap detail=m_playlistDetailById.value(pid);
            for(const QVariant& v:detail.value(QStringLiteral("items")).toList()){
                QUrl u=toPlayableUrl(v.toMap().value(QStringLiteral("musicPath")).toString());
                if(!u.isEmpty()) urls.push_back(u);
            }
            if(urls.isEmpty()){failTool(id,QStringLiteral("playlist_empty"),QStringLiteral("目标歌单没有可播放歌曲"));return true;}
            AudioService::instance().setPlaylist(urls);
            AudioService::instance().playPlaylist();
            succeedTool(id,{{QStringLiteral("played"),true},{QStringLiteral("playlist"),detail.value(QStringLiteral("playlist")).toMap()}});
            return true;
        }
        m_pendingPlaylistPlay.enqueue({id,name,args});
        m_shellViewModel->requestPlaylistDetail(m_hostStateProvider->currentUserAccount(),pid,false);
        return true;
    }

    if(name==QStringLiteral("getRecommendations")){
        if(!requireLogin(id)) return true;
        m_pendingRecommendationList.enqueue({id,name,args});
        m_shellViewModel->requestRecommendations(m_hostStateProvider->currentUserAccount(),args.value(QStringLiteral("scene"),QStringLiteral("home")).toString(),qBound(1,args.value(QStringLiteral("limit"),24).toInt(),200),args.value(QStringLiteral("excludePlayed"),true).toBool());
        return true;
    }

    if(name==QStringLiteral("getSimilarRecommendations")){
        const QString songId=args.value(QStringLiteral("songId")).toString().trimmed();
        if(songId.isEmpty()){failTool(id,QStringLiteral("invalid_args"),QStringLiteral("songId 不能为空"));return true;}
        m_pendingSimilarRecommendationList.enqueue({id,name,args});
        m_shellViewModel->requestSimilarRecommendations(songId,qBound(1,args.value(QStringLiteral("limit"),12).toInt(),100));
        return true;
    }

    if(!requireLogin(id)) return true;
    const QString user=m_hostStateProvider->currentUserAccount();

    if(name==QStringLiteral("addRecentTrack")){
        m_pendingHistoryAdd.enqueue({id,name,args});
        m_shellViewModel->addPlayHistory(user,args.value(QStringLiteral("musicPath")).toString(),args.value(QStringLiteral("title")).toString(),args.value(QStringLiteral("artist")).toString(),args.value(QStringLiteral("album")).toString(),QString::number(qMax(0,args.value(QStringLiteral("durationSec"),0).toInt())),args.value(QStringLiteral("isLocal"),true).toBool());
        return true;
    }
    if(name==QStringLiteral("removeRecentTracks")){
        m_pendingHistoryRemove.enqueue({id,name,args});
        m_shellViewModel->removeHistory(user,variantToStringList(args.value(QStringLiteral("musicPaths"))));
        return true;
    }
    if(name==QStringLiteral("addFavorite")){
        m_pendingFavoriteAdd.enqueue({id,name,args});
        m_shellViewModel->addFavorite(user,args.value(QStringLiteral("musicPath")).toString(),args.value(QStringLiteral("title")).toString(),args.value(QStringLiteral("artist")).toString(),QString::number(qMax(0,args.value(QStringLiteral("durationSec"),0).toInt())),args.value(QStringLiteral("isLocal"),true).toBool());
        return true;
    }
    if(name==QStringLiteral("removeFavorites")){
        m_pendingFavoriteRemove.enqueue({id,name,args});
        m_shellViewModel->removeFavorite(user,variantToStringList(args.value(QStringLiteral("musicPaths"))));
        return true;
    }
    if(name==QStringLiteral("createPlaylist")){
        m_pendingCreatePlaylist.enqueue({id,name,args});
        m_shellViewModel->createPlaylist(user,args.value(QStringLiteral("name")).toString(),args.value(QStringLiteral("description")).toString(),args.value(QStringLiteral("coverPath")).toString());
        return true;
    }
    if(name==QStringLiteral("updatePlaylist")){
        m_pendingUpdatePlaylist.enqueue({id,name,args});
        m_shellViewModel->updatePlaylist(user,parsePlaylistId(args),args.value(QStringLiteral("name")).toString(),args.value(QStringLiteral("description")).toString(),args.value(QStringLiteral("coverPath")).toString());
        return true;
    }
    if(name==QStringLiteral("deletePlaylist")){
        m_pendingDeletePlaylist.enqueue({id,name,args});
        m_shellViewModel->deletePlaylist(user,parsePlaylistId(args));
        return true;
    }
    if(name==QStringLiteral("addPlaylistItems")){
        QVariantList items=args.value(QStringLiteral("items")).toList();
        if(items.isEmpty()){
            QString err;
            items=buildPlaylistItemsFromTrackIds(args.value(QStringLiteral("trackIds")).toList(),&err);
            if(items.isEmpty()){failTool(id,QStringLiteral("invalid_args"),err.isEmpty()?QStringLiteral("缺少可用 trackIds/items"):err);return true;}
        }
        QVariantMap callArgs=args;
        callArgs.insert(QStringLiteral("items"),items);
        m_pendingAddPlaylistItems.enqueue({id,name,callArgs});
        m_shellViewModel->addPlaylistItems(user,parsePlaylistId(args),items);
        return true;
    }
    if(name==QStringLiteral("removePlaylistItems")){
        QStringList p=variantToStringList(args.value(QStringLiteral("musicPaths")));
        if(p.isEmpty()) p=buildMusicPathsFromTrackIds(args.value(QStringLiteral("trackIds")).toList());
        QVariantMap callArgs=args;
        callArgs.insert(QStringLiteral("musicPaths"),p);
        m_pendingRemovePlaylistItems.enqueue({id,name,callArgs});
        m_shellViewModel->removePlaylistItems(user,parsePlaylistId(args),p);
        return true;
    }
    if(name==QStringLiteral("reorderPlaylistItems")){
        QVariantList ordered=args.value(QStringLiteral("orderedItems")).toList();
        if(ordered.isEmpty()){
            int pos=1;
            for(const QString& p:variantToStringList(args.value(QStringLiteral("orderedPaths")))){
                QVariantMap it;it.insert(QStringLiteral("music_path"),p);it.insert(QStringLiteral("position"),pos++);ordered.push_back(it);
            }
        }
        QVariantMap callArgs=args;
        callArgs.insert(QStringLiteral("orderedItems"),ordered);
        m_pendingReorderPlaylistItems.enqueue({id,name,callArgs});
        m_shellViewModel->reorderPlaylistItems(user,parsePlaylistId(args),ordered);
        return true;
    }
    if(name==QStringLiteral("submitRecommendationFeedback")){
        m_pendingRecommendationFeedback.enqueue({id,name,args});
        m_shellViewModel->submitRecommendationFeedback(user,args.value(QStringLiteral("songId")).toString(),args.value(QStringLiteral("eventType")).toString(),args.value(QStringLiteral("scene"),QStringLiteral("home")).toString(),args.value(QStringLiteral("requestId")).toString(),args.value(QStringLiteral("modelVersion")).toString(),qMax<qint64>(0,args.value(QStringLiteral("playMs"),0).toLongLong()),qMax<qint64>(0,args.value(QStringLiteral("durationMs"),0).toLongLong()));
        return true;
    }

    failTool(id,QStringLiteral("unsupported_tool"),QStringLiteral("工具暂未实现：%1").arg(name));
    return true;
}

void AgentToolExecutor::onSearchResultsReady(const QList<Music>& list){if(m_pendingSearch.isEmpty()) return;auto t=m_pendingSearch.dequeue();QVariantList items=m_hostStateProvider->convertMusicList(list,qBound(1,t.args.value(QStringLiteral("limit"),10).toInt(),200));rememberTracks(items);succeedTool(t.toolCallId,{{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});} 
void AgentToolExecutor::onHistoryListReady(const QVariantList& h){if(m_pendingHistoryFetch.isEmpty()) return;auto t=m_pendingHistoryFetch.dequeue();QVariantList items=m_hostStateProvider->convertHistoryList(h,qBound(1,t.args.value(QStringLiteral("limit"),10).toInt(),500));rememberTracks(items);succeedTool(t.toolCallId,{{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});} 
void AgentToolExecutor::onFavoritesListReady(const QVariantList& f){if(m_pendingFavoritesFetch.isEmpty()) return;auto t=m_pendingFavoritesFetch.dequeue();QVariantList items=m_hostStateProvider->convertFavoriteList(f,qBound(1,t.args.value(QStringLiteral("limit"),500).toInt(),2000));rememberTracks(items);succeedTool(t.toolCallId,{{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});} 
void AgentToolExecutor::onPlaylistsListReady(const QVariantList& p,int page,int pageSize,int total){if(m_pendingPlaylistsFetch.isEmpty()) return;auto t=m_pendingPlaylistsFetch.dequeue();QVariantList items=m_hostStateProvider->convertPlaylistList(p);rememberPlaylistMeta(items);succeedTool(t.toolCallId,{{QStringLiteral("items"),items},{QStringLiteral("page"),page},{QStringLiteral("pageSize"),pageSize},{QStringLiteral("total"),total}});} 
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
            QList<QUrl> urls;
            for(const QVariant& v:items){QUrl u=toPlayableUrl(v.toMap().value(QStringLiteral("musicPath")).toString());if(!u.isEmpty()) urls.push_back(u);} 
            if(urls.isEmpty()){failTool(t.toolCallId,QStringLiteral("playlist_empty"),QStringLiteral("目标歌单没有可播放歌曲"));return;} 
            AudioService::instance().setPlaylist(urls);
            AudioService::instance().playPlaylist();
            succeedTool(t.toolCallId,{{QStringLiteral("played"),true},{QStringLiteral("playlist"),nd.value(QStringLiteral("playlist")).toMap()}});
        }
    }
}

void AgentToolExecutor::onRecommendationListReady(const QVariantMap& m,const QVariantList& it){if(m_pendingRecommendationList.isEmpty()) return;auto t=m_pendingRecommendationList.dequeue();QVariantList items=m_hostStateProvider->convertHistoryList(it,qBound(1,t.args.value(QStringLiteral("limit"),24).toInt(),200));const int n=qMin(items.size(),it.size());for(int i=0;i<n;++i){QVariantMap row=items.at(i).toMap();QString sg=it.at(i).toMap().value(QStringLiteral("song_id")).toString().trimmed();if(!sg.isEmpty()) row.insert(QStringLiteral("trackId"),sg);items[i]=row;}rememberTracks(items);succeedTool(t.toolCallId,{{QStringLiteral("meta"),m},{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});} 
void AgentToolExecutor::onSimilarRecommendationListReady(const QVariantMap& m,const QVariantList& it,const QString& anchor){if(m_pendingSimilarRecommendationList.isEmpty()) return;int idx=-1;for(int i=0;i<m_pendingSimilarRecommendationList.size();++i){if(m_pendingSimilarRecommendationList.at(i).args.value(QStringLiteral("songId")).toString().trimmed()==anchor.trimmed()){idx=i;break;}}if(idx<0) idx=0;auto t=m_pendingSimilarRecommendationList.takeAt(idx);QVariantList items=m_hostStateProvider->convertHistoryList(it,qBound(1,t.args.value(QStringLiteral("limit"),12).toInt(),100));rememberTracks(items);succeedTool(t.toolCallId,{{QStringLiteral("meta"),m},{QStringLiteral("anchorSongId"),anchor},{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}});} 
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
void AgentToolExecutor::onArtistTracksReady(const QList<Music>& musicList, const QString& artist){
    if(m_pendingArtistTracks.isEmpty()) return;
    int idx=-1;
    for(int i=0;i<m_pendingArtistTracks.size();++i){if(m_pendingArtistTracks.at(i).args.value(QStringLiteral("artist")).toString().trimmed()==artist.trimmed()){idx=i;break;}}
    if(idx<0) idx=0;
    auto task=m_pendingArtistTracks.takeAt(idx);
    QVariantList items=m_hostStateProvider->convertMusicList(musicList,qBound(1,task.args.value(QStringLiteral("limit"),200).toInt(),2000));
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
QString AgentToolExecutor::playModeName(int m){switch(m){case AudioService::Sequential:return QStringLiteral("sequential");case AudioService::RepeatOne:return QStringLiteral("repeat_one");case AudioService::RepeatAll:return QStringLiteral("repeat_all");case AudioService::Shuffle:return QStringLiteral("shuffle");default:return QStringLiteral("unknown");}}
int AgentToolExecutor::parsePlayModeValue(const QVariant& value){
    bool ok=false;
    const int numeric=value.toInt(&ok);
    if(ok&&numeric>=AudioService::Sequential&&numeric<=AudioService::Shuffle){
        return numeric;
    }

    const QString text=value.toString().trimmed().toLower();
    if(text==QStringLiteral("sequential")||text==QStringLiteral("sequence")||text==QStringLiteral("0")){
        return AudioService::Sequential;
    }
    if(text==QStringLiteral("repeat_one")||text==QStringLiteral("repeatone")||text==QStringLiteral("single")||text==QStringLiteral("1")){
        return AudioService::RepeatOne;
    }
    if(text==QStringLiteral("repeat_all")||text==QStringLiteral("repeatall")||text==QStringLiteral("loop")||text==QStringLiteral("2")){
        return AudioService::RepeatAll;
    }
    if(text==QStringLiteral("shuffle")||text==QStringLiteral("random")||text==QStringLiteral("3")){
        return AudioService::Shuffle;
    }
    return -1;
}
void AgentToolExecutor::rememberTracks(const QVariantList& tracks){for(const QVariant& v:tracks){QVariantMap t=v.toMap();QString id=t.value(QStringLiteral("trackId")).toString().trimmed();if(!id.isEmpty()) m_trackCacheById.insert(id,t);}}
void AgentToolExecutor::rememberPlaylistMeta(const QVariantList& playlists){for(const QVariant& v:playlists){QVariantMap p=v.toMap();qint64 id=p.value(QStringLiteral("playlistId")).toLongLong();if(id>0) m_playlistMetaById.insert(id,p);}}
void AgentToolExecutor::rememberPlaylistDetail(const QVariantMap& d){QVariantMap p=d.value(QStringLiteral("playlist")).toMap();qint64 id=p.value(QStringLiteral("playlistId")).toLongLong();if(id<=0) return;m_playlistDetailById.insert(id,d);m_playlistMetaById.insert(id,p);} 
QVariantMap AgentToolExecutor::resolveTrackById(const QString& trackId) const{QString id=trackId.trimmed();if(id.isEmpty()) return QVariantMap();return m_trackCacheById.value(id);} 
QVariantMap AgentToolExecutor::resolveTrackByPath(const QString& path) const{QString p=path.trimmed();if(p.isEmpty()) return QVariantMap();for(auto it=m_trackCacheById.constBegin();it!=m_trackCacheById.constEnd();++it){if(it.value().value(QStringLiteral("musicPath")).toString().trimmed()==p) return it.value();}return QVariantMap();}
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
    const QList<QUrl> queue=AudioService::instance().playlist();
    return {{QStringLiteral("items"),convertQueueToItems(queue)},
            {QStringLiteral("count"),queue.size()},
            {QStringLiteral("currentIndex"),AudioService::instance().currentIndex()},
            {QStringLiteral("playing"),AudioService::instance().isPlaying()},
            {QStringLiteral("volume"),AudioService::instance().volume()},
            {QStringLiteral("playMode"),static_cast<int>(AudioService::instance().playMode())},
            {QStringLiteral("playModeName"),playModeName(static_cast<int>(AudioService::instance().playMode()))}};
}
QVariantList AgentToolExecutor::buildPlaylistItemsFromTrackIds(const QVariantList& ids,QString* err) const{QVariantList out;for(const QVariant& v:ids){QString id=v.toString().trimmed();if(id.isEmpty()) continue;QVariantMap t=resolveTrackById(id);if(t.isEmpty()){if(err) *err=QStringLiteral("trackId 无法解析：%1").arg(id);return QVariantList();}QString path=t.value(QStringLiteral("musicPath")).toString().trimmed();if(path.isEmpty()) continue;QVariantMap item;item.insert(QStringLiteral("music_path"),path);item.insert(QStringLiteral("music_title"),t.value(QStringLiteral("title")).toString());item.insert(QStringLiteral("artist"),t.value(QStringLiteral("artist")).toString());item.insert(QStringLiteral("album"),t.value(QStringLiteral("album")).toString());item.insert(QStringLiteral("duration_sec"),t.value(QStringLiteral("durationMs")).toLongLong()/1000);item.insert(QStringLiteral("is_local"),!path.startsWith(QStringLiteral("http"),Qt::CaseInsensitive));item.insert(QStringLiteral("cover_art_path"),t.value(QStringLiteral("coverUrl")).toString());out.push_back(item);}if(out.isEmpty()&&err)*err=QStringLiteral("trackIds 为空或无法解析");return out;}
QStringList AgentToolExecutor::buildMusicPathsFromTrackIds(const QVariantList& ids) const{QStringList out;for(const QVariant& v:ids){QString id=v.toString().trimmed();if(id.isEmpty()) continue;QString p=resolveTrackById(id).value(QStringLiteral("musicPath")).toString().trimmed();if(!p.isEmpty()) out.push_back(p);}return out;}
