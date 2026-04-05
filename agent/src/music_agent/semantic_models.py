from __future__ import annotations

"""
模块名称: semantic_models
功能概述: 定义语义理解层使用的结构化数据模型，以及歌单查询归一化逻辑。
对外接口: SemanticParseResult 及其相关实体模型，normalize_playlist_query(), legacy_intent_to_semantic_result()
依赖关系: 依赖标准库 re 与 pydantic。
输入输出: 输入为原始字符串或 legacy intent payload；输出为结构化语义对象。
异常与错误: Pydantic 校验错误由调用方处理；本模块不主动抛运行时异常。
维护说明: 这里专注“模型与归一化”，不要混入规则匹配或执行逻辑。
"""

import re
from typing import Any, Literal

from pydantic import BaseModel, ConfigDict, Field


SemanticMode = Literal["chat", "tool", "plan"]
TargetDomain = Literal["track", "playlist", "playback", "library", "unknown"]

PLAYLIST_FILLER_PATTERNS = (
    "帮我",
    "我的",
    "给我",
    "麻烦",
    "请",
    "请你",
    "查询",
    "查找",
    "找一下",
    "找找",
    "查看",
    "看看",
    "列出",
    "列一下",
    "有没有",
)

PLAYLIST_SUFFIX_PATTERNS = (
    "里面有什么歌",
    "里有什么歌",
    "里面的歌曲",
    "里面的歌",
    "歌曲列表",
    "列表",
    "内容",
    "所有音乐",
    "全部音乐",
    "所有歌曲",
    "全部歌曲",
    "音乐",
    "歌曲",
)


class PlaylistSemanticEntity(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    raw_query: str | None = Field(default=None, alias="rawQuery")
    normalized_query: str | None = Field(default=None, alias="normalizedQuery")
    playlist_id: str | None = Field(default=None, alias="playlistId")


class TrackSelectionEntity(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    mode: str | None = None
    count: int | None = None


class TrackSemanticEntity(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    raw_query: str | None = Field(default=None, alias="rawQuery")
    normalized_query: str | None = Field(default=None, alias="normalizedQuery")
    track_id: str | None = Field(default=None, alias="trackId")
    artist: str | None = None
    album: str | None = None


class ToolProposal(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    tool: str
    args: dict[str, Any] = Field(default_factory=dict)
    confidence: float | None = None
    reason: str | None = None


class ActionCandidate(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    step_id: str = Field(alias="stepId")
    tool: str | None = None
    args: dict[str, Any] = Field(default_factory=dict)
    kind: Literal["tool", "clarify", "plan", "respond"] = "tool"
    depends_on: list[str] = Field(default_factory=list, alias="dependsOn")
    confidence: float | None = None
    reason: str | None = None
    requires_approval: bool = Field(default=False, alias="requiresApproval")
    may_need_clarification: bool = Field(default=False, alias="mayNeedClarification")


class SemanticEntities(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    playlist: PlaylistSemanticEntity | None = None
    target_playlist: PlaylistSemanticEntity | None = Field(default=None, alias="targetPlaylist")
    source_playlist: PlaylistSemanticEntity | None = Field(default=None, alias="sourcePlaylist")
    track_selection: TrackSelectionEntity | None = Field(default=None, alias="trackSelection")
    track: TrackSemanticEntity | None = None
    artist: str | None = None
    album: str | None = None
    limit: int | None = None


class SemanticParseResult(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    mode: SemanticMode = "chat"
    intent: str = "chat"
    entities: SemanticEntities = Field(default_factory=SemanticEntities)
    references: list[str] = Field(default_factory=list)
    missing_fields: list[str] = Field(default_factory=list, alias="missingFields")
    ambiguities: list[str] = Field(default_factory=list)
    target_domain: TargetDomain = Field(default="unknown", alias="targetDomain")
    should_auto_execute: bool = Field(default=False, alias="shouldAutoExecute")
    requires_approval: bool = Field(default=False, alias="requiresApproval")
    confidence: float = 0.0
    proposed_tool: ToolProposal | None = Field(default=None, alias="proposedTool")
    action_candidates: list[ActionCandidate] = Field(default_factory=list, alias="actionCandidates")


def normalize_playlist_query(value: str | None) -> str | None:
    if not value:
        return None

    normalized = str(value).strip()
    normalized = re.sub(r"""["“”‘’\s,，。！？.!?]+""", "", normalized)
    for token in PLAYLIST_FILLER_PATTERNS:
        normalized = normalized.replace(token, "")
    for suffix in PLAYLIST_SUFFIX_PATTERNS:
        normalized = normalized.replace(suffix, "")
    normalized = normalized.replace("歌单名为", "").replace("歌单叫做", "").replace("叫做", "")
    normalized = normalized.replace("这个", "").replace("那个", "").replace("刚才那个", "").replace("该", "")
    normalized = re.sub(r"歌单的?$", "", normalized)
    if normalized.endswith("歌单") and len(normalized) > 2:
        normalized = normalized[:-2]
    return normalized or None


def legacy_intent_to_semantic_result(payload: dict) -> SemanticParseResult:
    intent = str(payload.get("intent", "chat"))
    entities = payload.get("entities", {}) or {}

    playlist_raw = entities.get("playlistName")
    track_raw = entities.get("title") or entities.get("keyword")
    target_playlist_raw = entities.get("targetPlaylistName")
    source_playlist_raw = entities.get("sourcePlaylistName")

    result = SemanticParseResult(
        mode="chat" if intent == "chat" else ("plan" if intent.startswith("create_playlist") else "tool"),
        intent=intent,
        references=[],
        missingFields=[],
        ambiguities=list(payload.get("ambiguities", [])),
        targetDomain=_legacy_target_domain(intent),
        shouldAutoExecute=intent not in {"chat", "create_playlist", "create_playlist_with_top_tracks"},
        requiresApproval=intent in {"create_playlist", "create_playlist_with_top_tracks"},
        confidence=0.2,
    )

    if playlist_raw:
        result.entities.playlist = PlaylistSemanticEntity(
            rawQuery=str(playlist_raw),
            normalizedQuery=normalize_playlist_query(str(playlist_raw)),
        )
    if target_playlist_raw:
        result.entities.target_playlist = PlaylistSemanticEntity(
            rawQuery=str(target_playlist_raw),
            normalizedQuery=normalize_playlist_query(str(target_playlist_raw)),
        )
    if source_playlist_raw:
        result.entities.source_playlist = PlaylistSemanticEntity(
            rawQuery=str(source_playlist_raw),
            normalizedQuery=normalize_playlist_query(str(source_playlist_raw)),
        )
    if entities.get("trackSelectionMode") or entities.get("trackSelectionCount") is not None:
        result.entities.track_selection = TrackSelectionEntity(
            mode=entities.get("trackSelectionMode"),
            count=entities.get("trackSelectionCount"),
        )
    if track_raw or entities.get("artist") or entities.get("album"):
        result.entities.track = TrackSemanticEntity(
            rawQuery=str(track_raw) if track_raw else None,
            normalizedQuery=str(track_raw).strip() if track_raw else None,
            artist=entities.get("artist"),
            album=entities.get("album"),
        )
        result.entities.artist = entities.get("artist")
        result.entities.album = entities.get("album")
    if entities.get("limit") is not None:
        result.entities.limit = int(entities["limit"])

    return result


def _legacy_target_domain(intent: str) -> TargetDomain:
    if intent in {"play_track"}:
        return "track"
    if intent in {
        "get_playlists",
        "query_playlist",
        "inspect_playlist_tracks",
        "play_playlist",
        "create_playlist_from_playlist_subset",
    }:
        return "playlist"
    if intent in {"get_current_track", "stop_playback"}:
        return "playback"
    if intent in {"get_recent_tracks"}:
        return "library"
    return "unknown"
