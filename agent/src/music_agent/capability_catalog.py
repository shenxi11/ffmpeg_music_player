from __future__ import annotations

"""
模块名: capability_catalog
功能概述: 维护服务端可见的客户端能力目录，并同步客户端当前“统一 façade 入口 + 底层执行器”的执行架构元数据。
对外接口: CapabilityDefinition、get_capability()、capability_catalog_summary()、semantic_capability_hints()
依赖关系: 无外部服务依赖；由 capability_reasoner、server、music_runtime 读取
输入输出: 输入为能力名；输出为能力定义、目录摘要和语义提示摘要
异常与错误: 未知能力返回 None；目录摘要始终返回可序列化字典
维护说明: 客户端内部执行入口已收束到 AgentCapabilityFacade，tool_call 与脚本步骤都应视为 façade 的上层触发方式
"""

from dataclasses import asdict, dataclass
from typing import Literal


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


CAPABILITY_CATALOG_VERSION = "2026-03-30-facade-aware"

CLIENT_CAPABILITY_ARCHITECTURE = {
    "phase": 6,
    "entryPoint": "AgentCapabilityFacade",
    "entryPointKind": "facade",
    "backingExecutor": "AgentToolExecutor",
    "toolCallPath": "tool_call -> AgentChatViewModel -> AgentCapabilityFacade -> AgentToolExecutor",
    "scriptStepPath": "script_step -> AgentScriptExecutor -> AgentCapabilityFacade -> AgentToolExecutor",
    "notes": [
        "客户端当前已完成统一能力入口第一刀，tool_call 和脚本步骤执行都先走 AgentCapabilityFacade。",
        "AgentCapabilityFacade 目前仍是统一外观层，底层实际执行器仍为 AgentToolExecutor。",
        "服务端应把客户端认知从“零散工具集合”逐步切换为“统一 façade 背后的能力图谱”。",
    ],
}


CAPABILITY_CATALOG: tuple[CapabilityDefinition, ...] = (
    CapabilityDefinition(
        "searchTracks",
        "search",
        "根据关键字搜索歌曲。",
        True,
        True,
        "stable",
        "auto",
        False,
        (),
        ("Track", "SearchResult"),
        "基础自动执行能力；负责生成后续歌曲结构化对象。",
    ),
    CapabilityDefinition("getLyrics", "search", "根据歌曲路径获取歌词文本。", True, False, "stable", "auto", False, ("musicPath",), ("lyrics",), "服务端目前未接入。"),
    CapabilityDefinition("getVideoList", "video", "查询视频列表。", True, False, "stable", "auto", False, (), ("videoList",), "服务端目前未接入。"),
    CapabilityDefinition("getVideoStream", "video", "获取视频流地址。", True, False, "stable", "auto", False, ("videoId",), ("streamUrl",), "服务端目前未接入。"),
    CapabilityDefinition("searchArtist", "search", "查询歌手是否存在。", True, False, "stable", "auto", False, (), ("artistExists",), "服务端目前未接入。"),
    CapabilityDefinition("getTracksByArtist", "search", "获取指定歌手歌曲。", True, False, "stable", "auto", False, ("artist",), ("Track", "SearchResult"), "服务端目前未接入。"),
    CapabilityDefinition("getCurrentTrack", "playback", "获取当前播放快照。", True, True, "stable", "auto", False, (), ("CurrentPlayback",), ""),
    CapabilityDefinition("getRecentTracks", "library", "获取最近播放列表。", True, True, "stable", "auto", True, (), ("RecentTrackList", "Track"), "常用于后续引用。"),
    CapabilityDefinition("getTopPlayedTracks", "library", "获取最近常听或高频播放歌曲。", True, True, "stable", "auto", True, (), ("RecentTrackList", "Track"), "服务端使用的高频歌曲读取入口。"),
    CapabilityDefinition("addRecentTrack", "library", "写入最近播放。", False, False, "stable", "confirm", False, ("Track",), ("result",), "服务端目前未接入。", True),
    CapabilityDefinition("removeRecentTracks", "library", "删除最近播放。", False, False, "stable", "confirm", False, ("Track",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("getFavorites", "library", "获取喜欢音乐列表。", True, False, "stable", "auto", True, (), ("FavoriteTrackList", "Track"), "服务端目前未接入。"),
    CapabilityDefinition("addFavorite", "library", "添加喜欢音乐。", False, False, "stable", "auto", False, ("Track",), ("result",), "歌曲域低风险单项动作；应在已有 Track 结构化对象时自动执行。", True),
    CapabilityDefinition("removeFavorites", "library", "移除喜欢音乐。", False, False, "stable", "confirm", False, ("Track",), ("result",), "服务端目前未接入。", True),
    CapabilityDefinition("getPlaylists", "playlist", "获取歌单列表。", True, True, "stable", "auto", True, (), ("PlaylistList", "Playlist"), "歌单操作的入口能力。"),
    CapabilityDefinition("getPlaylistTracks", "playlist", "获取歌单歌曲内容。", True, True, "stable", "auto", True, ("playlistId",), ("PlaylistTrackList", "Track"), ""),
    CapabilityDefinition("createPlaylist", "playlist", "创建歌单。", False, True, "stable", "confirm", True, (), ("Playlist",), "当前已支持直执开关。"),
    CapabilityDefinition("updatePlaylist", "playlist", "修改歌单元数据。", False, False, "stable", "confirm", True, ("playlistId",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("deletePlaylist", "playlist", "删除歌单。", False, False, "stable", "restricted", True, ("playlistId",), ("result",), "高风险能力。"),
    CapabilityDefinition("addPlaylistItems", "playlist", "向歌单加入歌曲。", False, False, "stable", "confirm", True, ("playlistId", "Track"), ("result",), "Qt 真实工具名与当前后端 addTracksToPlaylist 不同。"),
    CapabilityDefinition("addTracksToPlaylist", "playlist", "向歌单加入多首歌曲。", False, False, "stable", "confirm", True, ("playlistId", "Track"), ("result",), "Qt 当前经由 AgentCapabilityFacade / AgentToolExecutor 还未对后端暴露该工具，服务端不应再假设能直接调用。"),
    CapabilityDefinition("removePlaylistItems", "playlist", "从歌单移除歌曲。", False, False, "stable", "confirm", True, ("playlistId", "Track"), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("reorderPlaylistItems", "playlist", "调整歌单内顺序。", False, False, "stable", "confirm", True, ("playlistId",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("getRecommendations", "recommendation", "获取推荐歌曲列表。", True, False, "stable", "auto", True, (), ("SearchResult", "Track"), "服务端目前未接入。"),
    CapabilityDefinition("getSimilarRecommendations", "recommendation", "基于当前歌曲获取相似推荐。", True, False, "stable", "auto", False, ("CurrentPlayback",), ("SearchResult", "Track"), "服务端目前未接入。"),
    CapabilityDefinition("submitRecommendationFeedback", "recommendation", "提交推荐反馈。", False, False, "stable", "confirm", False, (), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("playTrack", "playback", "播放指定歌曲。", False, True, "stable", "auto", False, ("trackId|musicPath",), ("CurrentPlayback",), "歌曲域基础自动执行能力；必须在已有 Track 结构化对象后调用。", True),
    CapabilityDefinition("playPlaylist", "playback", "播放指定歌单。", False, True, "partial", "confirm", False, ("playlistId",), ("CurrentPlayback", "PlaybackQueue"), "客户端文档标注为部分可用，不应假设为稳定原子能力。", True),
    CapabilityDefinition("pausePlayback", "playback", "暂停当前播放。", False, False, "stable", "auto", False, (), ("CurrentPlayback",), "服务端目前未接入。"),
    CapabilityDefinition("resumePlayback", "playback", "恢复当前播放。", False, False, "stable", "auto", False, (), ("CurrentPlayback",), "服务端目前未接入。"),
    CapabilityDefinition("stopPlayback", "playback", "停止播放。", False, True, "stable", "auto", False, (), ("CurrentPlayback",), "基础自动执行能力；停止类动作不依赖歌曲对象。"),
    CapabilityDefinition("seekPlayback", "playback", "调整播放进度。", False, False, "stable", "auto", False, ("CurrentPlayback",), ("CurrentPlayback",), "服务端目前未接入。"),
    CapabilityDefinition("playNext", "playback", "播放下一首。", False, False, "stable", "auto", False, ("PlaybackQueue",), ("CurrentPlayback",), "服务端目前未接入。"),
    CapabilityDefinition("playPrevious", "playback", "播放上一首。", False, False, "stable", "auto", False, ("PlaybackQueue",), ("CurrentPlayback",), "服务端目前未接入。"),
    CapabilityDefinition("playAtIndex", "playback", "按播放队列索引播放。", False, False, "stable", "auto", False, ("PlaybackQueue",), ("CurrentPlayback",), "服务端目前未接入。"),
    CapabilityDefinition("setVolume", "playback", "设置音量。", False, False, "stable", "auto", False, (), ("volume",), "服务端目前未接入。"),
    CapabilityDefinition("setPlayMode", "playback", "设置播放模式。", False, False, "stable", "auto", False, (), ("PlaybackQueue",), "服务端目前未接入。"),
    CapabilityDefinition("getPlaybackQueue", "playback", "获取当前播放队列。", True, False, "stable", "auto", False, (), ("PlaybackQueue",), "服务端目前未接入，但应进入世界状态模型。"),
    CapabilityDefinition("setPlaybackQueue", "playback", "重建播放队列。", False, False, "stable", "confirm", False, ("Track",), ("PlaybackQueue",), "服务端目前未接入。", True),
    CapabilityDefinition("addToPlaybackQueue", "playback", "向播放队列追加歌曲。", False, False, "stable", "auto", False, ("Track",), ("PlaybackQueue",), "歌曲域低风险单项动作；应在已有 Track 结构化对象时自动执行。", True),
    CapabilityDefinition("removeFromPlaybackQueue", "playback", "从播放队列移除歌曲。", False, False, "stable", "confirm", False, ("PlaybackQueue",), ("PlaybackQueue",), "服务端目前未接入。", True),
    CapabilityDefinition("clearPlaybackQueue", "playback", "清空播放队列。", False, False, "stable", "confirm", False, (), ("PlaybackQueue",), "服务端目前未接入。"),
    CapabilityDefinition("getLocalTracks", "local_library", "获取本地音乐列表。", True, False, "stable", "auto", False, (), ("LocalTrackList", "Track"), "服务端目前未接入。"),
    CapabilityDefinition("addLocalTrack", "local_library", "导入本地音乐。", False, False, "stable", "confirm", False, ("musicPath",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("removeLocalTrack", "local_library", "删除本地音乐记录。", False, False, "stable", "confirm", False, ("musicPath",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("getDownloadTasks", "download", "获取下载任务。", True, False, "stable", "auto", False, (), ("DownloadTaskList",), "服务端目前未接入。"),
    CapabilityDefinition("pauseDownloadTask", "download", "暂停下载任务。", False, False, "stable", "confirm", False, ("taskId",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("resumeDownloadTask", "download", "恢复下载任务。", False, False, "stable", "confirm", False, ("taskId",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("cancelDownloadTask", "download", "取消下载任务。", False, False, "stable", "confirm", False, ("taskId",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("removeDownloadTask", "download", "移除下载任务。", False, False, "stable", "confirm", False, ("taskId",), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("getVideoWindowState", "video", "获取视频窗口状态。", True, False, "partial", "auto", False, (), ("VideoWindowState",), "依赖视频窗口已创建。"),
    CapabilityDefinition("playVideo", "video", "播放视频。", False, False, "partial", "confirm", False, ("streamUrl",), ("VideoWindowState",), "依赖视频窗口已创建。"),
    CapabilityDefinition("pauseVideoPlayback", "video", "暂停视频。", False, False, "partial", "auto", False, (), ("VideoWindowState",), "依赖视频窗口已创建。"),
    CapabilityDefinition("resumeVideoPlayback", "video", "恢复视频。", False, False, "partial", "auto", False, (), ("VideoWindowState",), "依赖视频窗口已创建。"),
    CapabilityDefinition("seekVideoPlayback", "video", "调整视频进度。", False, False, "partial", "auto", False, ("VideoWindowState",), ("VideoWindowState",), ""),
    CapabilityDefinition("setVideoFullScreen", "video", "设置视频全屏。", False, False, "partial", "auto", False, ("VideoWindowState",), ("VideoWindowState",), ""),
    CapabilityDefinition("setVideoPlaybackRate", "video", "设置视频倍速。", False, False, "partial", "auto", False, ("VideoWindowState",), ("VideoWindowState",), ""),
    CapabilityDefinition("setVideoQualityPreset", "video", "设置视频画质预设。", False, False, "partial", "auto", False, ("VideoWindowState",), ("VideoWindowState",), ""),
    CapabilityDefinition("closeVideoWindow", "video", "关闭视频窗口。", False, False, "partial", "auto", False, ("VideoWindowState",), ("VideoWindowState",), ""),
    CapabilityDefinition("getDesktopLyricsState", "desktop_lyrics", "获取桌面歌词状态。", True, False, "stable", "auto", False, (), ("DesktopLyricsState",), "服务端目前未接入。"),
    CapabilityDefinition("showDesktopLyrics", "desktop_lyrics", "显示桌面歌词。", False, False, "stable", "auto", False, (), ("DesktopLyricsState",), "服务端目前未接入。"),
    CapabilityDefinition("hideDesktopLyrics", "desktop_lyrics", "隐藏桌面歌词。", False, False, "stable", "auto", False, (), ("DesktopLyricsState",), "服务端目前未接入。"),
    CapabilityDefinition("setDesktopLyricsStyle", "desktop_lyrics", "设置桌面歌词样式。", False, False, "stable", "confirm", False, (), ("DesktopLyricsState",), "服务端目前未接入。"),
    CapabilityDefinition("getPlugins", "plugins", "获取插件列表。", True, False, "stable", "auto", False, (), ("PluginList",), "服务端目前未接入。"),
    CapabilityDefinition("getPluginDiagnostics", "plugins", "获取插件诊断。", True, False, "stable", "auto", False, (), ("diagnostics",), "服务端目前未接入。"),
    CapabilityDefinition("reloadPlugins", "plugins", "重载插件。", False, False, "stable", "confirm", False, (), ("PluginList",), "服务端目前未接入。"),
    CapabilityDefinition("unloadPlugin", "plugins", "卸载插件。", False, False, "stable", "restricted", False, (), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("unloadAllPlugins", "plugins", "卸载全部插件。", False, False, "stable", "restricted", False, (), ("result",), "服务端目前未接入。"),
    CapabilityDefinition("getSettingsSnapshot", "settings", "获取设置快照。", True, False, "stable", "auto", False, (), ("SettingsSnapshot",), "服务端目前未接入。"),
    CapabilityDefinition("updateSetting", "settings", "更新设置项。", False, False, "stable", "confirm", False, (), ("result",), "服务端目前未接入。"),
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
