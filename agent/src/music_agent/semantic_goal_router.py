from __future__ import annotations

"""
模块名称: semantic_goal_router
功能概述: 负责 Goal Understanding，将自然语言优先路由为稳定的语义目标，再把对象提取交给 object resolver。
对外接口: infer_goal_intent()
依赖关系: 依赖 semantic_object_resolver 与 semantic_models。
输入输出: 输入为用户消息和上下文摘要；输出为 HeuristicSemanticIntent。
异常与错误: 无异常抛出；无法判断时返回 None。
维护说明: 这里专注“目标理解”，不要混入执行资格和 tool/script 派发逻辑。
"""

from dataclasses import dataclass, field

from .semantic_models import TargetDomain, normalize_playlist_query
from .semantic_object_resolver import (
    ADD_HINTS,
    CREATE_HINTS,
    CURRENT_TRACK_HINTS,
    LIST_OR_MUTATION_HINTS,
    PLAYLIST_CONTENT_HINTS,
    PLAYLIST_REFERENCE_HINTS,
    PLAY_HINTS,
    QUERY_HINTS,
    RECENT_TRACK_HINTS,
    STOP_HINTS,
    TRACK_REFERENCE_HINTS,
    contains_any,
    extract_playlist_query,
    extract_playlist_subset_transfer,
    extract_track_query,
    looks_like_playlist_content_followup,
    looks_like_playlist_play_followup,
    looks_like_track_play_followup,
    normalize_text,
)


@dataclass
class HeuristicSemanticIntent:
    intent: str
    mode: str
    target_domain: TargetDomain
    playlist_raw_query: str | None = None
    playlist_normalized_query: str | None = None
    target_playlist_raw_query: str | None = None
    target_playlist_normalized_query: str | None = None
    source_playlist_raw_query: str | None = None
    source_playlist_normalized_query: str | None = None
    track_selection_mode: str | None = None
    track_selection_count: int | None = None
    track_raw_query: str | None = None
    track_normalized_query: str | None = None
    artist: str | None = None
    references: list[str] = field(default_factory=list)
    missing_fields: list[str] = field(default_factory=list)
    should_auto_execute: bool = False
    requires_approval: bool = False
    confidence: float = 0.0


def infer_goal_intent(user_message: str, context: dict) -> HeuristicSemanticIntent | None:
    text = normalize_text(user_message)
    if not text:
        return None

    subset_transfer = extract_playlist_subset_transfer(text)
    if subset_transfer is not None:
        return HeuristicSemanticIntent(
            intent="create_playlist_from_playlist_subset",
            mode="tool",
            target_domain="playlist",
            target_playlist_raw_query=subset_transfer["target_raw"],
            target_playlist_normalized_query=normalize_playlist_query(subset_transfer["target_raw"]),
            source_playlist_raw_query=subset_transfer["source_raw"],
            source_playlist_normalized_query=normalize_playlist_query(subset_transfer["source_raw"]),
            track_selection_mode=subset_transfer["selection_mode"],
            track_selection_count=subset_transfer["selection_count"],
            should_auto_execute=True,
            confidence=0.99,
        )

    if contains_any(text, STOP_HINTS):
        return HeuristicSemanticIntent(
            intent="stop_playback",
            mode="tool",
            target_domain="playback",
            should_auto_execute=True,
            confidence=0.99,
        )

    if contains_any(text, CURRENT_TRACK_HINTS):
        return HeuristicSemanticIntent(
            intent="get_current_track",
            mode="tool",
            target_domain="playback",
            should_auto_execute=True,
            confidence=0.99,
        )

    if contains_any(text, RECENT_TRACK_HINTS):
        return HeuristicSemanticIntent(
            intent="get_recent_tracks",
            mode="tool",
            target_domain="library",
            should_auto_execute=True,
            confidence=0.99,
        )

    if "有哪些歌单" in text or "歌单列表" in text or "我的歌单" in text:
        return HeuristicSemanticIntent(
            intent="get_playlists",
            mode="tool",
            target_domain="playlist",
            should_auto_execute=True,
            confidence=0.98,
        )

    last_playlist = context.get("lastNamedPlaylist")
    last_track = context.get("lastNamedTrack")
    current_goal = str(context.get("currentGoal") or "")

    if last_playlist and looks_like_playlist_content_followup(text):
        return HeuristicSemanticIntent(
            intent="inspect_playlist_tracks",
            mode="tool",
            target_domain="playlist",
            references=["last_named_playlist"],
            should_auto_execute=True,
            confidence=0.95,
        )

    if last_playlist and looks_like_playlist_play_followup(text, current_goal):
        return HeuristicSemanticIntent(
            intent="play_playlist",
            mode="tool",
            target_domain="playlist",
            references=["last_named_playlist"],
            should_auto_execute=True,
            confidence=0.94,
        )

    if last_track and looks_like_track_play_followup(text) and not contains_any(text, LIST_OR_MUTATION_HINTS):
        return HeuristicSemanticIntent(
            intent="play_track",
            mode="tool",
            target_domain="track",
            references=["last_named_track"],
            should_auto_execute=True,
            confidence=0.92,
        )

    has_playlist_reference = contains_any(text, PLAYLIST_REFERENCE_HINTS)
    has_playlist_word = "歌单" in text or has_playlist_reference
    has_playlist_content = contains_any(text, PLAYLIST_CONTENT_HINTS)
    has_play = contains_any(text, PLAY_HINTS)
    has_query = contains_any(text, QUERY_HINTS)

    if has_playlist_content and has_playlist_word:
        playlist_raw = None if has_playlist_reference else extract_playlist_query(text)
        references = ["last_named_playlist"] if has_playlist_reference and last_playlist else []
        missing = ["playlist"] if not playlist_raw and not references else []
        return HeuristicSemanticIntent(
            intent="inspect_playlist_tracks",
            mode="tool",
            target_domain="playlist",
            playlist_raw_query=playlist_raw,
            playlist_normalized_query=normalize_playlist_query(playlist_raw),
            references=references,
            missing_fields=missing,
            should_auto_execute=not missing,
            confidence=0.98,
        )

    if has_play and has_playlist_word:
        playlist_raw = None if has_playlist_reference else extract_playlist_query(text)
        references = ["last_named_playlist"] if has_playlist_reference and last_playlist else []
        missing = ["playlist"] if not playlist_raw and not references else []
        return HeuristicSemanticIntent(
            intent="play_playlist",
            mode="tool",
            target_domain="playlist",
            playlist_raw_query=playlist_raw,
            playlist_normalized_query=normalize_playlist_query(playlist_raw),
            references=references,
            missing_fields=missing,
            should_auto_execute=not missing,
            confidence=0.97,
        )

    if (has_query and has_playlist_word) or ("歌单" in text and not has_play and not has_playlist_content):
        playlist_raw = None if has_playlist_reference else extract_playlist_query(text)
        references = ["last_named_playlist"] if has_playlist_reference and last_playlist else []
        missing = ["playlist"] if not playlist_raw and not references else []
        return HeuristicSemanticIntent(
            intent="query_playlist",
            mode="tool",
            target_domain="playlist",
            playlist_raw_query=playlist_raw,
            playlist_normalized_query=normalize_playlist_query(playlist_raw),
            references=references,
            missing_fields=missing,
            should_auto_execute=not missing,
            confidence=0.96,
        )

    if has_play and not contains_any(text, LIST_OR_MUTATION_HINTS):
        track_raw, artist = extract_track_query(text)
        references = ["last_named_track"] if contains_any(text, TRACK_REFERENCE_HINTS) and last_track else []
        missing = []
        if not track_raw and not references:
            missing = ["track", "title"]
        return HeuristicSemanticIntent(
            intent="play_track",
            mode="tool",
            target_domain="track",
            track_raw_query=track_raw,
            track_normalized_query=track_raw.strip() if track_raw else None,
            artist=artist,
            references=references,
            missing_fields=missing,
            should_auto_execute=not missing,
            confidence=0.90,
        )

    return None
