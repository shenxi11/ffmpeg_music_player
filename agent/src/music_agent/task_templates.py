from __future__ import annotations

"""
模块名: task_templates
功能概述: 沉淀高频用户目标对应的任务链模板，让规划器先基于稳定资产生成步骤，再由能力路由层决定能否执行。
对外接口: TaskTemplateStep、TaskTemplatePlan、resolve_task_template()
依赖关系: 依赖 semantic_models 和 workflow_memory 提供结构化语义与工作记忆
输入输出: 输入为语义结果与会话状态；输出为模板化步骤计划
异常与错误: 不抛出运行时异常；无法匹配时返回 not_applicable/rejected 状态
维护说明: 这里只表达“任务链模板”，不负责能力可执行性、风控或协议派发
"""

from dataclasses import dataclass, field
from typing import Any, Literal

from .semantic_models import SemanticParseResult
from .workflow_memory import WorkflowState


@dataclass(frozen=True)
class TaskTemplateStep:
    tool: str
    args: dict[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class TaskTemplatePlan:
    status: Literal["selected", "not_applicable", "rejected"]
    reason: str
    steps: tuple[TaskTemplateStep, ...] = ()


def resolve_task_template(semantic: SemanticParseResult, state: WorkflowState) -> TaskTemplatePlan:
    intent = semantic.intent

    if intent == "get_current_track":
        return _selected("get_current_track", TaskTemplateStep("getCurrentTrack"))

    if intent == "stop_playback":
        return _selected("stop_playback", TaskTemplateStep("stopPlayback"))

    if intent == "get_recent_tracks":
        limit = semantic.entities.limit or 10
        return _selected("get_recent_tracks", TaskTemplateStep("getRecentTracks", {"limit": limit}))

    if intent == "query_playlist":
        return _selected("query_playlist", TaskTemplateStep("getPlaylists", _playlist_query_args(semantic)))

    if intent == "inspect_playlist_tracks":
        if "last_named_playlist" in semantic.references and state.last_named_playlist is not None:
            return _selected(
                "inspect_playlist_tracks_from_context",
                TaskTemplateStep("getPlaylistTracks", {"playlistId": state.last_named_playlist.get("playlistId")}),
            )
        return _selected(
            "inspect_playlist_tracks_lookup_then_fetch",
            TaskTemplateStep("getPlaylists", {**_playlist_query_args(semantic), "playlistRole": "source"}),
            TaskTemplateStep("getPlaylistTracks", {"playlistRole": "source"}),
        )

    if intent == "play_track":
        if "last_named_track" in semantic.references and state.last_named_track is not None:
            return _selected(
                "play_track_from_context",
                TaskTemplateStep("playTrack", {"trackId": state.last_named_track.get("trackId")}),
            )
        search_args = _track_search_args(semantic)
        if not search_args.get("keyword"):
            return TaskTemplatePlan(status="rejected", reason="missing_track_keyword")
        return _selected(
            "search_then_play_track",
            TaskTemplateStep("searchTracks", search_args),
            TaskTemplateStep("playTrack"),
        )

    if intent == "play_playlist":
        if "last_named_playlist" in semantic.references and state.last_named_playlist is not None:
            return _selected(
                "play_playlist_from_context",
                TaskTemplateStep("playPlaylist", {"playlistId": state.last_named_playlist.get("playlistId")}),
            )
        return _selected(
            "lookup_then_play_playlist",
            TaskTemplateStep("getPlaylists", {**_playlist_query_args(semantic), "playlistRole": "source"}),
            TaskTemplateStep("playPlaylist", {"playlistRole": "source"}),
        )

    if intent == "create_playlist_from_playlist_subset":
        selection = semantic.entities.track_selection
        if selection is None:
            return TaskTemplatePlan(status="rejected", reason="missing_track_selection")
        return _selected(
            "create_playlist_from_playlist_subset",
            TaskTemplateStep("createPlaylist", {"playlistRole": "target"}),
            TaskTemplateStep("getPlaylists", {"playlistRole": "source"}),
            TaskTemplateStep("getPlaylistTracks", {"playlistRole": "source"}),
            TaskTemplateStep(
                "addTracksToPlaylist",
                {
                    "playlistRole": "target",
                    "selectionMode": selection.mode,
                    "count": selection.count,
                },
            ),
        )

    return TaskTemplatePlan(status="not_applicable", reason=f"intent_not_supported:{intent}")


def _playlist_query_args(semantic: SemanticParseResult) -> dict[str, Any]:
    playlist = semantic.entities.playlist
    return {
        "rawQuery": playlist.raw_query if playlist else None,
        "normalizedQuery": playlist.normalized_query if playlist else None,
    }


def _track_search_args(semantic: SemanticParseResult) -> dict[str, Any]:
    track = semantic.entities.track
    return {
        "keyword": (track.raw_query if track else None) or (track.normalized_query if track else None),
        "artist": semantic.entities.artist or (track.artist if track else None),
        "album": semantic.entities.album or (track.album if track else None),
        "limit": 5,
    }


def _selected(reason: str, *steps: TaskTemplateStep) -> TaskTemplatePlan:
    return TaskTemplatePlan(status="selected", reason=reason, steps=tuple(steps))
