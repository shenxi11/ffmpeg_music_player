from __future__ import annotations

"""
模块名称: semantic_refiner
功能概述: 以 Claude Code 式的三层流程整理语义结果：
1. Goal Understanding
2. Object Resolution
3. Semantic Policy
对外接口: refine_semantic_result(), extract_chinese_playlist_query(), extract_chinese_playlist_subset_transfer()
依赖关系: 依赖 semantic_goal_router、semantic_object_resolver、semantic_models。
输入输出: 输入为初始语义结果、用户消息和上下文；输出为修正后的 SemanticParseResult。
异常与错误: 不抛运行时异常；无法确定时保留原结果并尽量修复结构化字段。
维护说明: 这里负责“语义理解收口”，不要混入工具调度和执行逻辑。
"""

from typing import Any

from .semantic_goal_router import HeuristicSemanticIntent, infer_goal_intent
from .semantic_models import (
    ActionCandidate,
    PlaylistSemanticEntity,
    SemanticParseResult,
    TrackSemanticEntity,
    TrackSelectionEntity,
    normalize_playlist_query,
)
from .semantic_object_resolver import (
    PLAYLIST_REFERENCE_HINTS,
    TRACK_REFERENCE_HINTS,
    contains_any,
    extract_playlist_query,
    extract_playlist_subset_transfer,
    extract_track_query,
    normalize_text,
)


def refine_semantic_result(
    result: SemanticParseResult,
    user_message: str,
    context: dict[str, Any],
) -> SemanticParseResult:
    refined = result.model_copy(deep=True)
    heuristic = infer_goal_intent(user_message, context)

    if heuristic is not None and _should_override(refined, heuristic):
        _apply_heuristic(refined, heuristic)
    else:
        _merge_references_from_message(refined, user_message, context)

    _repair_entities(refined, user_message, context)
    _apply_semantic_policy(refined, context)
    _apply_default_action_candidates(refined)
    return refined


def extract_chinese_playlist_query(user_message: str) -> str | None:
    return extract_playlist_query(user_message)


def extract_chinese_playlist_subset_transfer(user_message: str) -> dict[str, Any] | None:
    return extract_playlist_subset_transfer(user_message)


def _should_override(result: SemanticParseResult, heuristic: HeuristicSemanticIntent) -> bool:
    if result.intent == heuristic.intent:
        return True
    if result.intent in {"create_playlist", "create_playlist_with_top_tracks"}:
        return heuristic.intent in {"create_playlist", "create_playlist_with_top_tracks", "create_playlist_from_playlist_subset"}
    if result.intent == "get_playlists":
        return heuristic.intent in {"get_playlists", "query_playlist", "inspect_playlist_tracks", "play_playlist"}
    if result.intent == "chat":
        return True
    if result.target_domain == "unknown":
        return True
    if heuristic.confidence >= 0.97:
        return True
    if heuristic.intent in {"inspect_playlist_tracks", "get_recent_tracks", "create_playlist_from_playlist_subset"}:
        return True
    return False


def _apply_heuristic(result: SemanticParseResult, heuristic: HeuristicSemanticIntent) -> None:
    result.intent = heuristic.intent
    result.mode = heuristic.mode
    result.target_domain = heuristic.target_domain
    result.references = list(heuristic.references)
    result.missing_fields = list(heuristic.missing_fields)
    result.should_auto_execute = heuristic.should_auto_execute
    result.requires_approval = heuristic.requires_approval
    result.confidence = max(result.confidence, heuristic.confidence)

    if heuristic.playlist_raw_query is not None or heuristic.playlist_normalized_query is not None:
        result.entities.playlist = PlaylistSemanticEntity(
            rawQuery=heuristic.playlist_raw_query,
            normalizedQuery=heuristic.playlist_normalized_query or normalize_playlist_query(heuristic.playlist_raw_query),
        )
    if heuristic.target_playlist_raw_query is not None or heuristic.target_playlist_normalized_query is not None:
        result.entities.target_playlist = PlaylistSemanticEntity(
            rawQuery=heuristic.target_playlist_raw_query,
            normalizedQuery=heuristic.target_playlist_normalized_query
            or normalize_playlist_query(heuristic.target_playlist_raw_query),
        )
    if heuristic.source_playlist_raw_query is not None or heuristic.source_playlist_normalized_query is not None:
        result.entities.source_playlist = PlaylistSemanticEntity(
            rawQuery=heuristic.source_playlist_raw_query,
            normalizedQuery=heuristic.source_playlist_normalized_query
            or normalize_playlist_query(heuristic.source_playlist_raw_query),
        )
    if heuristic.track_selection_mode is not None:
        result.entities.track_selection = TrackSelectionEntity(
            mode=heuristic.track_selection_mode,
            count=heuristic.track_selection_count,
        )
    if heuristic.track_raw_query is not None or heuristic.artist is not None:
        result.entities.track = TrackSemanticEntity(
            rawQuery=heuristic.track_raw_query,
            normalizedQuery=heuristic.track_normalized_query or heuristic.track_raw_query,
            artist=heuristic.artist,
        )
        result.entities.artist = heuristic.artist


def _merge_references_from_message(result: SemanticParseResult, user_message: str, context: dict[str, Any]) -> None:
    text = normalize_text(user_message)
    references = set(result.references)
    if context.get("lastNamedPlaylist") and contains_any(text, PLAYLIST_REFERENCE_HINTS):
        references.add("last_named_playlist")
    if context.get("lastNamedTrack") and contains_any(text, TRACK_REFERENCE_HINTS):
        references.add("last_named_track")
    result.references = list(references)


def _repair_entities(result: SemanticParseResult, user_message: str, context: dict[str, Any]) -> None:
    text = normalize_text(user_message)

    playlist = result.entities.playlist
    if result.intent in {"query_playlist", "inspect_playlist_tracks", "play_playlist"}:
        if playlist is None and not result.references:
            playlist_raw = extract_playlist_query(text)
            if playlist_raw:
                result.entities.playlist = PlaylistSemanticEntity(
                    rawQuery=playlist_raw,
                    normalizedQuery=normalize_playlist_query(playlist_raw),
                )
        elif playlist is not None and not playlist.normalized_query:
            playlist.normalized_query = normalize_playlist_query(playlist.raw_query)

    if result.intent == "create_playlist_from_playlist_subset":
        subset_transfer = extract_playlist_subset_transfer(text)
        if subset_transfer is not None:
            if result.entities.target_playlist is None:
                result.entities.target_playlist = PlaylistSemanticEntity(
                    rawQuery=subset_transfer["target_raw"],
                    normalizedQuery=normalize_playlist_query(subset_transfer["target_raw"]),
                )
            if result.entities.source_playlist is None:
                result.entities.source_playlist = PlaylistSemanticEntity(
                    rawQuery=subset_transfer["source_raw"],
                    normalizedQuery=normalize_playlist_query(subset_transfer["source_raw"]),
                )
            if result.entities.track_selection is None:
                result.entities.track_selection = TrackSelectionEntity(
                    mode=subset_transfer["selection_mode"],
                    count=subset_transfer["selection_count"],
                )

    if result.intent == "play_track":
        track = result.entities.track
        if track is None and not result.references:
            raw_track, artist = extract_track_query(text)
            if raw_track:
                result.entities.track = TrackSemanticEntity(
                    rawQuery=raw_track,
                    normalizedQuery=raw_track,
                    artist=artist,
                )
                result.entities.artist = result.entities.artist or artist
        elif track is not None and not track.normalized_query and track.raw_query:
            track.normalized_query = track.raw_query

    if result.references and "last_named_playlist" in result.references and context.get("lastNamedPlaylist"):
        result.should_auto_execute = result.intent in {"inspect_playlist_tracks", "play_playlist", "query_playlist"}
    if result.references and "last_named_track" in result.references and context.get("lastNamedTrack"):
        result.should_auto_execute = result.intent == "play_track"


def _apply_semantic_policy(result: SemanticParseResult, context: dict[str, Any]) -> None:
    if result.intent == "chat":
        result.mode = "chat"
        result.target_domain = "unknown"
        result.should_auto_execute = False
        return

    if result.intent in {"create_playlist", "create_playlist_with_top_tracks"}:
        result.mode = "plan"
        result.requires_approval = True
        result.should_auto_execute = False
        return

    if result.intent == "create_playlist_from_playlist_subset":
        result.mode = "tool"
        missing = []
        if result.entities.target_playlist is None:
            missing.append("targetPlaylist")
        if result.entities.source_playlist is None:
            missing.append("sourcePlaylist")
        if result.entities.track_selection is None:
            missing.append("trackSelection")
        result.missing_fields = _merge_missing_fields(result.missing_fields, missing)
        result.should_auto_execute = not result.missing_fields
        return

    if result.intent in {"query_playlist", "inspect_playlist_tracks", "play_playlist"}:
        has_playlist = result.entities.playlist is not None or "last_named_playlist" in result.references
        result.missing_fields = _merge_missing_fields(result.missing_fields, [] if has_playlist else ["playlist"])
        result.mode = "tool"
        result.should_auto_execute = not result.missing_fields
        return

    if result.intent == "play_track":
        has_track = result.entities.track is not None or "last_named_track" in result.references
        result.missing_fields = _merge_missing_fields(result.missing_fields, [] if has_track else ["track", "title"])
        result.mode = "tool"
        result.should_auto_execute = not result.missing_fields
        return

    result.mode = "tool"


def _apply_default_action_candidates(result: SemanticParseResult) -> None:
    if result.action_candidates:
        return
    if result.intent != "create_playlist_from_playlist_subset":
        return
    selection = result.entities.track_selection
    count = selection.count if selection else None
    mode = selection.mode if selection else None
    result.action_candidates = [
        ActionCandidate(stepId="s1", tool="createPlaylist", args={"playlistRole": "target"}, kind="tool"),
        ActionCandidate(
            stepId="s2",
            tool="getPlaylists",
            args={"playlistRole": "source"},
            kind="tool",
            dependsOn=["s1"],
        ),
        ActionCandidate(
            stepId="s3",
            tool="getPlaylistTracks",
            args={"playlistRole": "source"},
            kind="tool",
            dependsOn=["s2"],
        ),
        ActionCandidate(
            stepId="s4",
            tool="addTracksToPlaylist",
            args={"playlistRole": "target", "selectionMode": mode, "count": count},
            kind="tool",
            dependsOn=["s3"],
        ),
    ]


def _merge_missing_fields(existing: list[str], required: list[str]) -> list[str]:
    merged = list(existing)
    for item in required:
        if item not in merged:
            merged.append(item)
    return merged
