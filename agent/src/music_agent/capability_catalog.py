from __future__ import annotations

"""
模块名: capability_catalog
功能概述: 维护服务端可见的客户端能力目录，并同步当前控制型 Agent 的真实工具暴露面。
对外接口: CapabilityDefinition、get_capability()、capability_catalog_summary()、semantic_capability_hints()
依赖关系: 无外部服务依赖；由 capability_reasoner、server、控制/兼容 runtime 读取
输入输出: 输入为能力名；输出为能力定义、目录摘要和语义提示摘要
异常与错误: 未知能力返回 None；目录摘要始终返回可序列化字典
维护说明: 本文件应与 Qt 侧 ToolRegistry 的真实工具注册保持同步；除登录外，客户端主工具面默认全部对 Agent 暴露
"""

from dataclasses import asdict, dataclass
from typing import Any, Literal


CapabilityStability = Literal["stable", "partial", "reserved"]
AutomationPolicy = Literal["auto", "confirm", "restricted"]


@dataclass(frozen=True)
class CapabilityDefinition:
    name: str
    category: str
    description: str
    read_only: bool
    exposed_to_backend: bool
    stability: CapabilityStability
    automation_policy: AutomationPolicy
    requires_login: bool = False
    prerequisite_objects: tuple[str, ...] = ()
    produces_objects: tuple[str, ...] = ()
    notes: str = ""
    requires_structured_object: bool = False
    client_entry_point: str = "AgentCapabilityFacade"
    invocation_paths: tuple[str, ...] = ("tool_call", "script_step")
    backing_executor: str = "AgentToolExecutor"


def cap(
    name: str,
    category: str,
    description: str,
    *,
    read_only: bool,
    automation_policy: AutomationPolicy,
    requires_login: bool = False,
    stability: CapabilityStability = "stable",
    prerequisite_objects: tuple[str, ...] = (),
    produces_objects: tuple[str, ...] = (),
    notes: str = "",
    requires_structured_object: bool = False,
) -> CapabilityDefinition:
    return CapabilityDefinition(
        name=name,
        category=category,
        description=description,
        read_only=read_only,
        exposed_to_backend=True,
        stability=stability,
        automation_policy=automation_policy,
        requires_login=requires_login,
        prerequisite_objects=prerequisite_objects,
        produces_objects=produces_objects,
        notes=notes,
        requires_structured_object=requires_structured_object,
    )


CAPABILITY_CATALOG_VERSION = "2026-04-09-control-all-tools"

CLIENT_CAPABILITY_ARCHITECTURE = {
    "phase": 7,
    "entryPoint": "AgentCapabilityFacade",
    "entryPointKind": "facade",
    "backingExecutor": "AgentToolExecutor",
    "toolCallPath": "tool_call -> AgentChatViewModel -> AgentCapabilityFacade -> AgentToolExecutor",
    "scriptStepPath": "script_step -> AgentScriptExecutor -> AgentCapabilityFacade -> AgentToolExecutor",
    "notes": [
        "客户端默认主链已切到本地控制型 Agent，工具调用统一经由 AgentCapabilityFacade 和 AgentToolExecutor。",
        "当前工具面默认对 Agent 暴露除登录外的全部主要客户端接口，登录与认证握手仍明确排除。",
        "assistant 模式只允许解释，不执行写操作；control 模式才允许确定性模板执行。",
    ],
}


CAPABILITY_CATALOG: tuple[CapabilityDefinition, ...] = (
    cap("searchTracks", "search", "根据关键字搜索歌曲。", read_only=True, automation_policy="auto", produces_objects=("Track", "SearchResult")),
    cap("getLyrics", "search", "根据歌曲路径获取歌词文本。", read_only=True, automation_policy="auto", prerequisite_objects=("musicPath",), produces_objects=("lyrics",)),
    cap("getVideoList", "video", "查询视频列表。", read_only=True, automation_policy="auto", produces_objects=("VideoList",)),
    cap("getVideoStream", "video", "获取视频流地址。", read_only=True, automation_policy="auto", prerequisite_objects=("videoId",), produces_objects=("streamUrl",)),
    cap("searchArtist", "search", "查询歌手是否存在。", read_only=True, automation_policy="auto", produces_objects=("artistExists",)),
    cap("getTracksByArtist", "search", "获取指定歌手歌曲。", read_only=True, automation_policy="auto", prerequisite_objects=("artist",), produces_objects=("Track", "SearchResult")),
    cap("getCurrentTrack", "playback", "获取当前播放快照。", read_only=True, automation_policy="auto", produces_objects=("CurrentPlayback",)),
    cap("getRecentTracks", "library", "获取最近播放列表。", read_only=True, automation_policy="auto", requires_login=True, produces_objects=("RecentTrackList", "Track")),
    cap("addRecentTrack", "library", "写入最近播放。", read_only=False, automation_policy="confirm", prerequisite_objects=("Track",), produces_objects=("result",), requires_structured_object=True),
    cap("removeRecentTracks", "library", "删除最近播放。", read_only=False, automation_policy="confirm", prerequisite_objects=("Track",), produces_objects=("result",)),
    cap("getFavorites", "library", "获取喜欢音乐列表。", read_only=True, automation_policy="auto", requires_login=True, produces_objects=("FavoriteTrackList", "Track")),
    cap("addFavorite", "library", "添加喜欢音乐。", read_only=False, automation_policy="auto", prerequisite_objects=("Track",), produces_objects=("result",), requires_structured_object=True),
    cap("removeFavorites", "library", "移除喜欢音乐。", read_only=False, automation_policy="confirm", prerequisite_objects=("Track",), produces_objects=("result",), requires_structured_object=True),
    cap("getPlaylists", "playlist", "获取歌单列表。", read_only=True, automation_policy="auto", requires_login=True, produces_objects=("PlaylistList", "Playlist")),
    cap("getPlaylistTracks", "playlist", "获取歌单歌曲内容。", read_only=True, automation_policy="auto", requires_login=True, prerequisite_objects=("playlistId",), produces_objects=("PlaylistTrackList", "Track")),
    cap("createPlaylist", "playlist", "创建歌单。", read_only=False, automation_policy="confirm", requires_login=True, produces_objects=("Playlist",)),
    cap("updatePlaylist", "playlist", "修改歌单元数据。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("playlistId",), produces_objects=("result",)),
    cap("deletePlaylist", "playlist", "删除歌单。", read_only=False, automation_policy="restricted", requires_login=True, prerequisite_objects=("playlistId",), produces_objects=("result",), notes="高风险破坏性动作。"),
    cap("addPlaylistItems", "playlist", "向歌单加入歌曲。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("playlistId", "Track"), produces_objects=("result",)),
    cap("addTracksToPlaylist", "playlist", "向歌单加入多首歌曲。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("playlistId", "Track"), produces_objects=("result",), notes="Qt 侧当前兼容别名，底层复用 addPlaylistItems 执行链。"),
    cap("removePlaylistItems", "playlist", "从歌单移除歌曲。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("playlistId", "Track"), produces_objects=("result",)),
    cap("reorderPlaylistItems", "playlist", "调整歌单内顺序。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("playlistId",), produces_objects=("result",)),
    cap("playTrack", "playback", "播放指定歌曲。", read_only=False, automation_policy="auto", prerequisite_objects=("trackId|musicPath",), produces_objects=("CurrentPlayback",), requires_structured_object=True),
    cap("playPlaylist", "playback", "播放指定歌单。", read_only=False, automation_policy="confirm", stability="partial", prerequisite_objects=("playlistId",), produces_objects=("CurrentPlayback", "PlaybackQueue"), notes="依赖歌单详情装配，建议带护栏执行。", requires_structured_object=True),
    cap("getRecommendations", "recommendation", "获取推荐歌曲列表。", read_only=True, automation_policy="auto", requires_login=True, produces_objects=("SearchResult", "Track")),
    cap("getSimilarRecommendations", "recommendation", "基于当前歌曲获取相似推荐。", read_only=True, automation_policy="auto", prerequisite_objects=("CurrentPlayback",), produces_objects=("SearchResult", "Track")),
    cap("submitRecommendationFeedback", "recommendation", "提交推荐反馈。", read_only=False, automation_policy="confirm", prerequisite_objects=("songId",), produces_objects=("result",)),
    cap("pausePlayback", "playback", "暂停当前播放。", read_only=False, automation_policy="auto", produces_objects=("CurrentPlayback",)),
    cap("resumePlayback", "playback", "恢复当前播放。", read_only=False, automation_policy="auto", produces_objects=("CurrentPlayback",)),
    cap("stopPlayback", "playback", "停止播放。", read_only=False, automation_policy="auto", produces_objects=("CurrentPlayback",)),
    cap("seekPlayback", "playback", "调整播放进度。", read_only=False, automation_policy="auto", prerequisite_objects=("CurrentPlayback",), produces_objects=("CurrentPlayback",)),
    cap("playNext", "playback", "播放下一首。", read_only=False, automation_policy="auto", prerequisite_objects=("PlaybackQueue",), produces_objects=("CurrentPlayback",)),
    cap("playPrevious", "playback", "播放上一首。", read_only=False, automation_policy="auto", prerequisite_objects=("PlaybackQueue",), produces_objects=("CurrentPlayback",)),
    cap("playAtIndex", "playback", "按播放队列索引播放。", read_only=False, automation_policy="auto", prerequisite_objects=("PlaybackQueue",), produces_objects=("CurrentPlayback",)),
    cap("setVolume", "playback", "设置音量。", read_only=False, automation_policy="auto", produces_objects=("volume",)),
    cap("setPlayMode", "playback", "设置播放模式。", read_only=False, automation_policy="auto", produces_objects=("PlaybackQueue",)),
    cap("getPlaybackQueue", "playback", "获取当前播放队列。", read_only=True, automation_policy="auto", produces_objects=("PlaybackQueue",)),
    cap("setPlaybackQueue", "playback", "重建播放队列。", read_only=False, automation_policy="confirm", prerequisite_objects=("Track",), produces_objects=("PlaybackQueue",), requires_structured_object=True),
    cap("addToPlaybackQueue", "playback", "向播放队列追加歌曲。", read_only=False, automation_policy="auto", prerequisite_objects=("Track",), produces_objects=("PlaybackQueue",), requires_structured_object=True),
    cap("removeFromPlaybackQueue", "playback", "从播放队列移除歌曲。", read_only=False, automation_policy="confirm", prerequisite_objects=("PlaybackQueue",), produces_objects=("PlaybackQueue",), requires_structured_object=True),
    cap("clearPlaybackQueue", "playback", "清空播放队列。", read_only=False, automation_policy="confirm", produces_objects=("PlaybackQueue",)),
    cap("getLocalTracks", "local_library", "获取本地音乐列表。", read_only=True, automation_policy="auto", produces_objects=("LocalTrackList", "Track")),
    cap("addLocalTrack", "local_library", "导入本地音乐。", read_only=False, automation_policy="confirm", prerequisite_objects=("musicPath",), produces_objects=("result",)),
    cap("removeLocalTrack", "local_library", "删除本地音乐记录。", read_only=False, automation_policy="confirm", prerequisite_objects=("musicPath",), produces_objects=("result",)),
    cap("getDownloadTasks", "download", "获取下载任务。", read_only=True, automation_policy="auto", produces_objects=("DownloadTaskList",)),
    cap("pauseDownloadTask", "download", "暂停下载任务。", read_only=False, automation_policy="confirm", prerequisite_objects=("taskId",), produces_objects=("result",)),
    cap("resumeDownloadTask", "download", "恢复下载任务。", read_only=False, automation_policy="confirm", prerequisite_objects=("taskId",), produces_objects=("result",)),
    cap("cancelDownloadTask", "download", "取消下载任务。", read_only=False, automation_policy="confirm", prerequisite_objects=("taskId",), produces_objects=("result",)),
    cap("removeDownloadTask", "download", "移除下载任务。", read_only=False, automation_policy="confirm", prerequisite_objects=("taskId",), produces_objects=("result",)),
    cap("getVideoWindowState", "video", "获取视频窗口状态。", read_only=True, automation_policy="auto", produces_objects=("VideoWindowState",)),
    cap("playVideo", "video", "播放视频。", read_only=False, automation_policy="confirm", stability="partial", prerequisite_objects=("streamUrl",), produces_objects=("VideoWindowState",)),
    cap("pauseVideoPlayback", "video", "暂停视频。", read_only=False, automation_policy="auto", stability="partial", produces_objects=("VideoWindowState",)),
    cap("resumeVideoPlayback", "video", "恢复视频。", read_only=False, automation_policy="auto", stability="partial", produces_objects=("VideoWindowState",)),
    cap("seekVideoPlayback", "video", "调整视频进度。", read_only=False, automation_policy="auto", stability="partial", prerequisite_objects=("VideoWindowState",), produces_objects=("VideoWindowState",)),
    cap("setVideoFullScreen", "video", "设置视频全屏。", read_only=False, automation_policy="auto", stability="partial", prerequisite_objects=("VideoWindowState",), produces_objects=("VideoWindowState",)),
    cap("setVideoPlaybackRate", "video", "设置视频倍速。", read_only=False, automation_policy="auto", stability="partial", prerequisite_objects=("VideoWindowState",), produces_objects=("VideoWindowState",)),
    cap("setVideoQualityPreset", "video", "设置视频画质预设。", read_only=False, automation_policy="confirm", stability="partial", prerequisite_objects=("VideoWindowState",), produces_objects=("VideoWindowState",)),
    cap("closeVideoWindow", "video", "关闭视频窗口。", read_only=False, automation_policy="auto", stability="partial", produces_objects=("VideoWindowState",)),
    cap("getDesktopLyricsState", "desktop_lyrics", "获取桌面歌词状态。", read_only=True, automation_policy="auto", produces_objects=("DesktopLyricsState",)),
    cap("showDesktopLyrics", "desktop_lyrics", "显示桌面歌词。", read_only=False, automation_policy="auto", produces_objects=("DesktopLyricsState",)),
    cap("hideDesktopLyrics", "desktop_lyrics", "隐藏桌面歌词。", read_only=False, automation_policy="auto", produces_objects=("DesktopLyricsState",)),
    cap("setDesktopLyricsStyle", "desktop_lyrics", "设置桌面歌词样式。", read_only=False, automation_policy="confirm", produces_objects=("DesktopLyricsState",)),
    cap("getPlugins", "plugins", "获取插件列表。", read_only=True, automation_policy="auto", produces_objects=("PluginList",)),
    cap("getPluginDiagnostics", "plugins", "获取插件诊断。", read_only=True, automation_policy="auto", produces_objects=("diagnostics",)),
    cap("reloadPlugins", "plugins", "重载插件。", read_only=False, automation_policy="confirm", produces_objects=("PluginList",)),
    cap("unloadPlugin", "plugins", "卸载指定插件。", read_only=False, automation_policy="restricted", produces_objects=("result",), notes="插件卸载属于高风险动作。"),
    cap("unloadAllPlugins", "plugins", "卸载全部插件。", read_only=False, automation_policy="restricted", produces_objects=("result",), notes="全局破坏性动作。"),
    cap("getHostContext", "host", "获取宿主界面上下文快照。", read_only=True, automation_policy="auto", produces_objects=("HostContext",)),
    cap("getVisiblePage", "host", "获取当前可见主页面。", read_only=True, automation_policy="auto", produces_objects=("currentPage",)),
    cap("getSelectedPlaylist", "host", "获取当前选中歌单摘要。", read_only=True, automation_policy="auto", produces_objects=("Playlist",)),
    cap("getSelectedTrackIds", "host", "获取当前页面选中的歌曲 ID 列表。", read_only=True, automation_policy="auto", produces_objects=("trackIds",)),
    cap("getUserProfile", "account", "获取当前用户资料快照。", read_only=True, automation_policy="auto", requires_login=True, produces_objects=("UserProfile",)),
    cap("refreshUserProfile", "account", "刷新当前用户资料。", read_only=False, automation_policy="confirm", requires_login=True, produces_objects=("result",), notes="会触发联网刷新和本地缓存更新。"),
    cap("updateUsername", "account", "修改当前用户名。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("username",), produces_objects=("result",)),
    cap("uploadAvatar", "account", "上传新的用户头像。", read_only=False, automation_policy="confirm", requires_login=True, prerequisite_objects=("filePath",), produces_objects=("result",)),
    cap("logoutUser", "account", "退出当前登录用户。", read_only=False, automation_policy="confirm", requires_login=True, produces_objects=("result",), notes="会结束当前账号会话。"),
    cap("returnToWelcome", "general", "返回欢迎页并结束当前客户端会话。", read_only=False, automation_policy="confirm", produces_objects=("result",), notes="会清空当前播放上下文并关闭主窗口。"),
    cap("getSettingsSnapshot", "settings", "获取客户端设置快照。", read_only=True, automation_policy="auto", produces_objects=("SettingsSnapshot",)),
    cap("updateSetting", "settings", "更新单个客户端设置。", read_only=False, automation_policy="confirm", prerequisite_objects=("key", "value"), produces_objects=("result",)),
)


CAPABILITY_INDEX = {item.name: item for item in CAPABILITY_CATALOG}


def get_capability(name: str) -> CapabilityDefinition | None:
    return CAPABILITY_INDEX.get(name)


def exposed_capabilities() -> list[CapabilityDefinition]:
    return [item for item in CAPABILITY_CATALOG if item.exposed_to_backend]


def capability_architecture_summary() -> dict:
    return dict(CLIENT_CAPABILITY_ARCHITECTURE)


def _capability_payload(capability: CapabilityDefinition) -> dict:
    payload = asdict(capability)
    payload["readOnly"] = payload.pop("read_only")
    payload["exposedToBackend"] = payload.pop("exposed_to_backend")
    payload["automationPolicy"] = payload.pop("automation_policy")
    payload["requiresLogin"] = payload.pop("requires_login")
    payload["prerequisiteObjects"] = list(payload.pop("prerequisite_objects"))
    payload["producesObjects"] = list(payload.pop("produces_objects"))
    payload["requiresStructuredObject"] = payload.pop("requires_structured_object")
    payload["clientEntryPoint"] = payload.pop("client_entry_point")
    payload["invocationPaths"] = list(payload.pop("invocation_paths"))
    payload["backingExecutor"] = payload.pop("backing_executor")
    return payload


def capability_catalog_summary() -> dict:
    items = [_capability_payload(capability) for capability in CAPABILITY_CATALOG]
    return {
        "version": CAPABILITY_CATALOG_VERSION,
        "clientArchitecture": capability_architecture_summary(),
        "items": items,
        "exposedCount": len([item for item in CAPABILITY_CATALOG if item.exposed_to_backend]),
        "totalCount": len(CAPABILITY_CATALOG),
    }


def semantic_capability_hints() -> list[dict]:
    hints: list[dict] = []
    for capability in exposed_capabilities():
        payload = _capability_payload(capability)
        hints.append(
            {
                "name": payload["name"],
                "category": payload["category"],
                "stability": payload["stability"],
                "automationPolicy": payload["automationPolicy"],
                "prerequisiteObjects": payload["prerequisiteObjects"],
                "producesObjects": payload["producesObjects"],
                "notes": payload["notes"],
                "requiresStructuredObject": payload["requiresStructuredObject"],
                "clientEntryPoint": payload["clientEntryPoint"],
                "invocationPaths": payload["invocationPaths"],
            }
        )
    return hints


def build_control_compiler_capability_projection(snapshot: dict[str, Any]) -> dict[str, Any]:
    """将完整 capability snapshot 投影为本地控制模型可消费的轻量结构。"""
    raw_items = snapshot.get("items")
    if not isinstance(raw_items, list):
        raw_items = snapshot.get("tools")
    if not isinstance(raw_items, list):
        raw_items = []

    tool_names = {
        str(item.get("name", "")).strip()
        for item in raw_items
        if isinstance(item, dict) and str(item.get("name", "")).strip()
    }

    def has_tool(*names: str) -> bool:
        return any(name in tool_names for name in names)

    available_intents: list[str] = []
    if has_tool("pausePlayback"):
        available_intents.append("pause_playback")
    if has_tool("resumePlayback"):
        available_intents.append("resume_playback")
    if has_tool("stopPlayback"):
        available_intents.append("stop_playback")
    if has_tool("playNext"):
        available_intents.append("play_next")
    if has_tool("playPrevious"):
        available_intents.append("play_previous")
    if has_tool("setVolume"):
        available_intents.append("set_volume")
    if has_tool("setPlayMode"):
        available_intents.append("set_play_mode")
    if has_tool("getLocalTracks"):
        available_intents.append("query_local_tracks")
    if has_tool("searchTracks", "playTrack"):
        available_intents.append("play_track_by_query")
    if has_tool("getPlaylists", "getPlaylistTracks", "playTrack"):
        available_intents.append("inspect_playlist_then_play_index")
    if has_tool("getCurrentTrack", "getPlaybackQueue"):
        available_intents.append("query_current_playback")
    if has_tool("createPlaylist"):
        available_intents.append("create_playlist_with_confirmation")
    if has_tool("addPlaylistItems", "addTracksToPlaylist"):
        available_intents.append("add_tracks_to_playlist_with_confirmation")

    available_domains = sorted(
        {
            str(item.get("domain") or item.get("category") or "").strip()
            for item in raw_items
            if isinstance(item, dict) and str(item.get("domain") or item.get("category") or "").strip()
        }
    )

    writes_require_confirmation = sorted(
        {
            str(item.get("name", "")).strip()
            for item in raw_items
            if isinstance(item, dict)
            and not bool(item.get("readOnly", False))
            and (
                str(item.get("confirmPolicy", "")).strip().lower() not in ("", "none")
                or str(item.get("automationPolicy", "")).strip().lower() == "confirm"
                or bool(item.get("requireApproval", False))
            )
            and str(item.get("name", "")).strip()
        }
    )

    requires_login_domains = sorted(
        {
            str(item.get("domain") or item.get("category") or "").strip()
            for item in raw_items
            if isinstance(item, dict)
            and (
                bool(item.get("requiresLogin", False))
                or str(item.get("availabilityPolicy", "")).strip().lower() == "login_required"
            )
            and str(item.get("domain") or item.get("category") or "").strip()
        }
    )

    return {
        "catalogVersion": str(snapshot.get("catalogVersion") or snapshot.get("version") or ""),
        "availableIntents": available_intents,
        "availableDomains": available_domains,
        "writesRequireConfirmation": writes_require_confirmation,
        "requiresLoginDomains": requires_login_domains,
        "assistantWriteBlocked": True,
    }
