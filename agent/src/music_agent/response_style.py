from __future__ import annotations

"""
模块名: response_style
功能概述: 统一管理音乐 Agent 面向用户的回复文案、澄清提示和工具结果摘要，作为运行时之外的响应风格层。
对外接口: 文案常量、build_*_clarification()、summarize_*() 系列函数
依赖关系: 无外部服务依赖；由 music_runtime 等模块消费
输入输出: 输入为结构化歌曲/歌单/播放结果；输出为面向用户的自然语言文本与选项
异常与错误: 纯函数，不抛出运行时异常
维护说明: 后续若接入不同客户端回复风格或输出 hooks，应优先扩展本模块而不是继续在 runtime 中拼接文本
"""


TEXT_WAITING_FOR_TOOL = "当前正在等待客户端返回工具执行结果，请稍后再试。"
TEXT_WAITING_FOR_APPROVAL = "当前有一个待确认的计划，请先确认或取消。"
TEXT_ASK_TRACK_NAME = "请告诉我你想播放的歌曲名。"
TEXT_ASK_PLAYLIST_NAME = "请告诉我你想播放的歌单名。"
TEXT_ASK_CREATE_PLAYLIST_NAME = "请告诉我你想创建的歌单名。"
TEXT_ASK_PLAYLIST_FOR_TRACKS = "请告诉我你想查看哪个歌单的内容。"
TEXT_NO_TRACK_FOUND = "没有找到符合条件的歌曲，请换个关键词或补充歌手名。"
TEXT_NO_RECENT_TRACKS = "最近播放列表为空。"
TEXT_OPERATION_DONE = "操作已完成。"
TEXT_CLARIFY_TRACK = "我找到了多个候选歌曲，请告诉我你想播放哪一个。"
TEXT_CLARIFY_PLAYLIST = "我找到了多个候选歌单，请告诉我你想播放哪一个。"
TEXT_CLARIFY_PLAYLIST_TRACKS = "我找到了多个候选歌单，请告诉我你想查看哪一个歌单的内容。"
TEXT_CLARIFY_QUERY_PLAYLIST = "我找到了多个候选歌单，请告诉我你要查询哪一个。"
TEXT_NO_CURRENT_TRACK = "当前没有正在播放的歌曲。"
TEXT_PLAYBACK_STOPPED = "已停止播放。"
TEXT_NO_PLAYLISTS = "当前没有可用歌单。"
TEXT_PLAYLIST_LIST_HEADER = "当前可用歌单如下："
TEXT_RECENT_TRACKS_HEADER = "最近播放的歌曲如下："
TEXT_PROGRESS_APPROVED = "已批准，开始执行计划"
TEXT_PROGRESS_FETCH_TOP_TRACKS = "歌单已创建，正在获取最近常听歌曲"
TEXT_PROGRESS_ADD_TRACKS = "已拿到最近常听歌曲，正在加入歌单"

UNKNOWN_SONG = "未知歌曲"
UNKNOWN_ARTIST = "未知歌手"
UNKNOWN_PLAYLIST = "未命名歌单"


def build_track_clarification(items: list[dict]) -> tuple[str, list[str]]:
    options = [f"{index + 1}. {item.get('artist', UNKNOWN_ARTIST)} - {item.get('title', UNKNOWN_SONG)}" for index, item in enumerate(items[:5])]
    return TEXT_CLARIFY_TRACK, options


def build_playlist_clarification(items: list[dict]) -> tuple[str, list[str]]:
    options = [f"{index + 1}. {item.get('name', UNKNOWN_PLAYLIST)}" for index, item in enumerate(items[:5])]
    return TEXT_CLARIFY_PLAYLIST, options


def build_playlist_track_clarification(items: list[dict]) -> tuple[str, list[str]]:
    options = [f"{index + 1}. {item.get('name', UNKNOWN_PLAYLIST)}" for index, item in enumerate(items[:5])]
    return TEXT_CLARIFY_PLAYLIST_TRACKS, options


def build_playlist_query_clarification(items: list[dict]) -> tuple[str, list[str]]:
    options = [f"{index + 1}. {item.get('name', UNKNOWN_PLAYLIST)}" for index, item in enumerate(items[:5])]
    return TEXT_CLARIFY_QUERY_PLAYLIST, options


def summarize_current_track(result: dict) -> str:
    if not result:
        return TEXT_NO_CURRENT_TRACK
    return f"当前播放的是 **{result.get('title', UNKNOWN_SONG)}** - **{result.get('artist', UNKNOWN_ARTIST)}**。"


def summarize_playlists(items: list[dict]) -> str:
    if not items:
        return TEXT_NO_PLAYLISTS
    lines = [TEXT_PLAYLIST_LIST_HEADER]
    for index, item in enumerate(items[:10], start=1):
        lines.append(f"{index}. **{item.get('name', UNKNOWN_PLAYLIST)}**（{item.get('trackCount', 0)} 首）")
    return "\n".join(lines)


def summarize_single_playlist(playlist: dict) -> str:
    return f"找到了歌单 **{playlist.get('name', UNKNOWN_PLAYLIST)}**（{playlist.get('trackCount', 0)} 首）。"


def summarize_playlist_tracks(playlist: dict, items: list[dict]) -> str:
    playlist_name = playlist.get("name", UNKNOWN_PLAYLIST)
    if not items:
        return f"歌单 **{playlist_name}** 里当前还没有歌曲。"
    lines = [f"歌单 **{playlist_name}** 里有这些歌曲："]
    for index, item in enumerate(items[:10], start=1):
        lines.append(f"{index}. **{item.get('title', UNKNOWN_SONG)}** - **{item.get('artist', UNKNOWN_ARTIST)}**")
    return "\n".join(lines)


def summarize_recent_tracks(items: list[dict]) -> str:
    if not items:
        return TEXT_NO_RECENT_TRACKS
    lines = [TEXT_RECENT_TRACKS_HEADER]
    for index, item in enumerate(items[:10], start=1):
        lines.append(f"{index}. **{item.get('title', UNKNOWN_SONG)}** - **{item.get('artist', UNKNOWN_ARTIST)}**")
    return "\n".join(lines)


def summarize_play_track(track: dict) -> str:
    return f"已开始播放 **{track.get('title', UNKNOWN_SONG)}** - **{track.get('artist', UNKNOWN_ARTIST)}**。"


def summarize_play_playlist(playlist: dict) -> str:
    return f"已开始播放歌单 **{playlist.get('name', UNKNOWN_PLAYLIST)}**。"


def summarize_create_playlist(playlist: dict) -> str:
    return f"已创建歌单 **{playlist.get('name', UNKNOWN_PLAYLIST)}**。"


def summarize_add_tracks_to_playlist(playlist: dict, added_count: int) -> str:
    return f"已向歌单 **{playlist.get('name', UNKNOWN_PLAYLIST)}** 添加 {added_count} 首歌曲。"
