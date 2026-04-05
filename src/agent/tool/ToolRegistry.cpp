#include "ToolRegistry.h"

ToolRegistry::ToolRegistry()
{
    registerDefaultTools();
}

bool ToolRegistry::contains(const QString& toolName) const
{
    const QString target = toolName.trimmed();
    for (const AgentToolDefinition& def : m_tools) {
        if (def.name == target) {
            return true;
        }
    }
    return false;
}

AgentToolDefinition ToolRegistry::definition(const QString& toolName) const
{
    const QString target = toolName.trimmed();
    for (const AgentToolDefinition& def : m_tools) {
        if (def.name == target) {
            return def;
        }
    }
    return AgentToolDefinition();
}

QVector<AgentToolDefinition> ToolRegistry::allTools() const
{
    return m_tools;
}

bool ToolRegistry::validateArgs(const QString& toolName,
                                const QVariantMap& args,
                                QString* errorCode,
                                QString* errorMessage) const
{
    if (!contains(toolName)) {
        if (errorCode) {
            *errorCode = QStringLiteral("unsupported_tool");
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("不支持的工具：%1").arg(toolName);
        }
        return false;
    }

    const AgentToolDefinition def = definition(toolName);
    for (const QString& key : def.requiredArgs) {
        const QVariant value = args.value(key);
        if (!value.isValid()) {
            if (errorCode) {
                *errorCode = QStringLiteral("invalid_args");
            }
            if (errorMessage) {
                *errorMessage = QStringLiteral("缺少必填参数：%1").arg(key);
            }
            return false;
        }
        if (value.type() == QVariant::String && value.toString().trimmed().isEmpty()) {
            if (errorCode) {
                *errorCode = QStringLiteral("invalid_args");
            }
            if (errorMessage) {
                *errorMessage = QStringLiteral("参数不能为空：%1").arg(key);
            }
            return false;
        }
    }

    return true;
}

void ToolRegistry::registerDefaultTools()
{
    registerTool({QStringLiteral("searchTracks"),
                  QStringLiteral("按关键词搜索歌曲"),
                  {},
                  {QStringLiteral("keyword"), QStringLiteral("artist"), QStringLiteral("album"), QStringLiteral("limit")},
                  true,
                  false});

    registerTool({QStringLiteral("getLyrics"),
                  QStringLiteral("获取歌曲歌词"),
                  {},
                  {QStringLiteral("trackId"), QStringLiteral("musicPath")},
                  true,
                  false});

    registerTool({QStringLiteral("getVideoList"),
                  QStringLiteral("获取视频列表"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("getVideoStream"),
                  QStringLiteral("获取视频播放地址"),
                  {QStringLiteral("videoPath")},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("searchArtist"),
                  QStringLiteral("搜索歌手是否存在"),
                  {QStringLiteral("artist")},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("getTracksByArtist"),
                  QStringLiteral("按歌手获取歌曲列表"),
                  {QStringLiteral("artist")},
                  {QStringLiteral("limit")},
                  true,
                  false});

    registerTool({QStringLiteral("getCurrentTrack"),
                  QStringLiteral("获取当前播放歌曲"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("getRecentTracks"),
                  QStringLiteral("获取最近播放列表"),
                  {},
                  {QStringLiteral("limit")},
                  true,
                  false});

    registerTool({QStringLiteral("addRecentTrack"),
                  QStringLiteral("添加最近播放记录"),
                  {QStringLiteral("musicPath")},
                  {QStringLiteral("title"), QStringLiteral("artist"), QStringLiteral("album"),
                   QStringLiteral("durationSec"), QStringLiteral("isLocal")},
                  false,
                  false});

    registerTool({QStringLiteral("removeRecentTracks"),
                  QStringLiteral("删除最近播放记录"),
                  {QStringLiteral("musicPaths")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getFavorites"),
                  QStringLiteral("获取喜欢音乐"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("addFavorite"),
                  QStringLiteral("添加喜欢音乐"),
                  {QStringLiteral("musicPath")},
                  {QStringLiteral("title"), QStringLiteral("artist"),
                   QStringLiteral("durationSec"), QStringLiteral("isLocal")},
                  false,
                  false});

    registerTool({QStringLiteral("removeFavorites"),
                  QStringLiteral("移除喜欢音乐"),
                  {QStringLiteral("musicPaths")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getPlaylists"),
                  QStringLiteral("获取用户歌单列表"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("getPlaylistTracks"),
                  QStringLiteral("获取歌单歌曲列表"),
                  {QStringLiteral("playlistId")},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("createPlaylist"),
                  QStringLiteral("创建歌单"),
                  {QStringLiteral("name")},
                  {QStringLiteral("description"), QStringLiteral("coverPath")},
                  false,
                  true});

    registerTool({QStringLiteral("updatePlaylist"),
                  QStringLiteral("更新歌单信息"),
                  {QStringLiteral("playlistId"), QStringLiteral("name")},
                  {QStringLiteral("description"), QStringLiteral("coverPath")},
                  false,
                  true});

    registerTool({QStringLiteral("deletePlaylist"),
                  QStringLiteral("删除歌单"),
                  {QStringLiteral("playlistId")},
                  {},
                  false,
                  true});

    registerTool({QStringLiteral("addPlaylistItems"),
                  QStringLiteral("歌单加歌"),
                  {QStringLiteral("playlistId"), QStringLiteral("trackIds")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("addTracksToPlaylist"),
                  QStringLiteral("歌单加歌（历史协议别名）"),
                  {QStringLiteral("playlistId"), QStringLiteral("trackIds")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("removePlaylistItems"),
                  QStringLiteral("歌单删歌"),
                  {QStringLiteral("playlistId"), QStringLiteral("musicPaths")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("reorderPlaylistItems"),
                  QStringLiteral("歌单排序"),
                  {QStringLiteral("playlistId"), QStringLiteral("orderedPaths")},
                  {},
                  false,
                  true});

    registerTool({QStringLiteral("playTrack"),
                  QStringLiteral("播放指定歌曲"),
                  {},
                  {QStringLiteral("trackId"), QStringLiteral("musicPath")},
                  false,
                  false});

    registerTool({QStringLiteral("playPlaylist"),
                  QStringLiteral("播放指定歌单"),
                  {QStringLiteral("playlistId")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getRecommendations"),
                  QStringLiteral("获取推荐音乐"),
                  {},
                  {QStringLiteral("scene"), QStringLiteral("limit"), QStringLiteral("excludePlayed")},
                  true,
                  false});

    registerTool({QStringLiteral("getSimilarRecommendations"),
                  QStringLiteral("获取相似推荐"),
                  {QStringLiteral("songId")},
                  {QStringLiteral("limit")},
                  true,
                  false});

    registerTool({QStringLiteral("submitRecommendationFeedback"),
                  QStringLiteral("提交推荐反馈"),
                  {QStringLiteral("songId"), QStringLiteral("eventType")},
                  {QStringLiteral("scene"), QStringLiteral("requestId"), QStringLiteral("modelVersion"),
                   QStringLiteral("playMs"), QStringLiteral("durationMs")},
                  false,
                  false});

    registerTool({QStringLiteral("pausePlayback"),
                  QStringLiteral("暂停播放"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("resumePlayback"),
                  QStringLiteral("恢复播放"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("stopPlayback"),
                  QStringLiteral("停止播放"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("seekPlayback"),
                  QStringLiteral("跳转播放进度"),
                  {QStringLiteral("positionMs")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("playNext"),
                  QStringLiteral("播放下一首"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("playPrevious"),
                  QStringLiteral("播放上一首"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("playAtIndex"),
                  QStringLiteral("按队列索引播放"),
                  {QStringLiteral("index")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("setVolume"),
                  QStringLiteral("设置音量"),
                  {QStringLiteral("volume")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("setPlayMode"),
                  QStringLiteral("设置播放模式"),
                  {QStringLiteral("mode")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getPlaybackQueue"),
                  QStringLiteral("获取播放队列"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("setPlaybackQueue"),
                  QStringLiteral("重置播放队列"),
                  {QStringLiteral("trackIds")},
                  {QStringLiteral("playNow"), QStringLiteral("startIndex")},
                  false,
                  true});

    registerTool({QStringLiteral("addToPlaybackQueue"),
                  QStringLiteral("加入播放队列"),
                  {},
                  {QStringLiteral("trackId"), QStringLiteral("musicPath")},
                  false,
                  false});

    registerTool({QStringLiteral("removeFromPlaybackQueue"),
                  QStringLiteral("从播放队列移除"),
                  {QStringLiteral("index")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("clearPlaybackQueue"),
                  QStringLiteral("清空播放队列"),
                  {},
                  {},
                  false,
                  true});

    registerTool({QStringLiteral("getLocalTracks"),
                  QStringLiteral("获取本地音乐列表"),
                  {},
                  {QStringLiteral("limit")},
                  true,
                  false});

    registerTool({QStringLiteral("addLocalTrack"),
                  QStringLiteral("导入本地音乐到本地列表"),
                  {QStringLiteral("filePath")},
                  {QStringLiteral("fileName"), QStringLiteral("artist")},
                  false,
                  false});

    registerTool({QStringLiteral("removeLocalTrack"),
                  QStringLiteral("从本地音乐列表移除歌曲"),
                  {QStringLiteral("filePath")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getDownloadTasks"),
                  QStringLiteral("获取下载任务列表"),
                  {},
                  {QStringLiteral("scope")},
                  true,
                  false});

    registerTool({QStringLiteral("pauseDownloadTask"),
                  QStringLiteral("暂停下载任务"),
                  {QStringLiteral("taskId")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("resumeDownloadTask"),
                  QStringLiteral("恢复下载任务"),
                  {QStringLiteral("taskId")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("cancelDownloadTask"),
                  QStringLiteral("取消下载任务"),
                  {QStringLiteral("taskId")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("removeDownloadTask"),
                  QStringLiteral("删除下载任务记录"),
                  {QStringLiteral("taskId")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getVideoWindowState"),
                  QStringLiteral("获取视频播放窗口状态"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("playVideo"),
                  QStringLiteral("在视频窗口播放视频"),
                  {},
                  {QStringLiteral("videoPath"), QStringLiteral("videoUrl")},
                  false,
                  false});

    registerTool({QStringLiteral("pauseVideoPlayback"),
                  QStringLiteral("暂停视频播放"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("resumeVideoPlayback"),
                  QStringLiteral("恢复视频播放"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("seekVideoPlayback"),
                  QStringLiteral("调整视频播放进度"),
                  {QStringLiteral("positionMs")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("setVideoFullScreen"),
                  QStringLiteral("切换视频窗口全屏状态"),
                  {QStringLiteral("enabled")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("setVideoPlaybackRate"),
                  QStringLiteral("设置视频倍速"),
                  {QStringLiteral("rate")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("setVideoQualityPreset"),
                  QStringLiteral("设置视频画质预设"),
                  {QStringLiteral("preset")},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("closeVideoWindow"),
                  QStringLiteral("关闭视频播放窗口"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("getDesktopLyricsState"),
                  QStringLiteral("获取桌面歌词状态"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("showDesktopLyrics"),
                  QStringLiteral("显示桌面歌词"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("hideDesktopLyrics"),
                  QStringLiteral("隐藏桌面歌词"),
                  {},
                  {},
                  false,
                  false});

    registerTool({QStringLiteral("setDesktopLyricsStyle"),
                  QStringLiteral("设置桌面歌词样式"),
                  {},
                  {QStringLiteral("color"), QStringLiteral("fontSize"), QStringLiteral("fontFamily")},
                  false,
                  false});

    registerTool({QStringLiteral("getPlugins"),
                  QStringLiteral("获取插件列表"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("getPluginDiagnostics"),
                  QStringLiteral("获取插件诊断信息"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("reloadPlugins"),
                  QStringLiteral("重新加载插件目录"),
                  {},
                  {},
                  false,
                  true});

    registerTool({QStringLiteral("unloadPlugin"),
                  QStringLiteral("卸载指定插件"),
                  {QStringLiteral("pluginKey")},
                  {},
                  false,
                  true});

    registerTool({QStringLiteral("unloadAllPlugins"),
                  QStringLiteral("卸载全部插件"),
                  {},
                  {},
                  false,
                  true});

    registerTool({QStringLiteral("getSettingsSnapshot"),
                  QStringLiteral("获取客户端设置快照"),
                  {},
                  {},
                  true,
                  false});

    registerTool({QStringLiteral("updateSetting"),
                  QStringLiteral("更新单个客户端设置"),
                  {QStringLiteral("key"), QStringLiteral("value")},
                  {},
                  false,
                  true});
}

void ToolRegistry::registerTool(const AgentToolDefinition& def)
{
    if (def.name.trimmed().isEmpty()) {
        return;
    }
    m_tools.push_back(def);
}
