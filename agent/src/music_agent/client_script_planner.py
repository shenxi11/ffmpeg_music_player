from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Literal

from .semantic_models import SemanticParseResult, normalize_playlist_query
from .workflow_memory import WorkflowState

DEFAULT_SCRIPT_TIMEOUT_MS = 30_000


@dataclass
class ScriptPlanningResult:
    status: Literal["selected", "not_applicable", "rejected"]
    reason: str
    script: dict[str, Any] | None = None
    metadata: dict[str, Any] = field(default_factory=dict)


class ClientScriptPlanner:
    def plan_script(self, semantic: SemanticParseResult, state: WorkflowState) -> ScriptPlanningResult:
        if semantic.intent == "play_track":
            script = self._build_search_and_play_script(semantic, state)
            if script is None:
                return ScriptPlanningResult(
                    status="not_applicable",
                    reason="search_and_play_requires_explicit_keyword_and_artist_or_album",
                )
            return ScriptPlanningResult(status="selected", reason="search_and_play_script_selected", script=script)

        if semantic.intent == "create_playlist_from_playlist_subset":
            return self._build_playlist_subset_transfer_script(semantic, state)

        return ScriptPlanningResult(status="not_applicable", reason=f"intent_not_supported:{semantic.intent}")

    @staticmethod
    def _build_search_and_play_script(semantic: SemanticParseResult, state: WorkflowState) -> dict[str, Any] | None:
        if "last_named_track" in semantic.references and state.last_named_track is not None:
            return None

        track = semantic.entities.track
        keyword = (track.raw_query if track else None) or (track.normalized_query if track else None)
        artist = semantic.entities.artist or (track.artist if track else None)
        album = semantic.entities.album or (track.album if track else None)

        # 仅在查询条件相对明确时切到客户端脚本，避免把高歧义请求退化成“搜索第一条就播放”。
        if not keyword or (not artist and not album):
            return None

        title = f"搜索并播放 {artist + ' - ' if artist else ''}{keyword}"
        return {
            "version": 1,
            "title": title,
            "timeoutMs": DEFAULT_SCRIPT_TIMEOUT_MS,
            "steps": [
                {
                    "id": "search",
                    "action": "searchTracks",
                    "args": {
                        "keyword": keyword,
                        "artist": artist,
                        "album": album,
                        "limit": 5,
                    },
                    "saveAs": "search",
                },
                {
                    "id": "play_first",
                    "action": "playTrack",
                    "args": {
                        "trackId": "$steps.search.items.0.trackId",
                    },
                },
            ],
        }

    def _build_playlist_subset_transfer_script(
        self,
        semantic: SemanticParseResult,
        state: WorkflowState,
    ) -> ScriptPlanningResult:
        target_playlist = semantic.entities.target_playlist
        source_playlist = semantic.entities.source_playlist
        selection = semantic.entities.track_selection

        target_name = (target_playlist.raw_query if target_playlist else None) or (
            target_playlist.normalized_query if target_playlist else None
        )
        if not target_name:
            return ScriptPlanningResult(status="rejected", reason="missing_target_playlist")

        if source_playlist is None or not (source_playlist.raw_query or source_playlist.normalized_query):
            return ScriptPlanningResult(status="rejected", reason="missing_source_playlist")

        if selection is None or selection.mode is None or selection.count is None or selection.count <= 0:
            return ScriptPlanningResult(status="rejected", reason="missing_track_selection")

        if selection.mode != "first_n":
            return ScriptPlanningResult(
                status="not_applicable",
                reason="track_selection_mode_not_supported_for_script",
                metadata={"selectionMode": selection.mode, "count": selection.count},
            )

        source = self._resolve_source_playlist(source_playlist, state)
        if source is None:
            return ScriptPlanningResult(
                status="rejected",
                reason="source_playlist_not_uniquely_resolved",
                metadata={
                    "sourcePlaylistRaw": source_playlist.raw_query,
                    "sourcePlaylistNormalized": source_playlist.normalized_query,
                },
            )

        max_count = min(selection.count, 10)
        track_refs = [f"$steps.source_tracks.items.{index}.trackId" for index in range(max_count)]
        script = {
            "version": 1,
            "title": f"创建歌单 {target_name} 并导入 {source.get('name', '来源歌单')} 的前 {max_count} 首歌曲",
            "timeoutMs": DEFAULT_SCRIPT_TIMEOUT_MS,
            "steps": [
                {
                    "id": "create_target",
                    "action": "createPlaylist",
                    "args": {"name": target_name},
                    "saveAs": "target",
                },
                {
                    "id": "list_source_playlists",
                    "action": "getPlaylists",
                    "args": {},
                    "saveAs": "source_playlists",
                },
                {
                    "id": "source_tracks",
                    "action": "getPlaylistTracks",
                    "args": {"playlistId": source["playlistId"]},
                    "saveAs": "source_tracks",
                },
                {
                    "id": "copy_tracks",
                    "action": "addTracksToPlaylist",
                    "args": {
                        "playlistId": "$steps.create_target.playlist.playlistId",
                        "trackIds": track_refs,
                    },
                },
            ],
        }
        return ScriptPlanningResult(
            status="selected",
            reason="playlist_subset_transfer_script_selected",
            script=script,
            metadata={
                "targetPlaylistName": target_name,
                "sourcePlaylistId": source["playlistId"],
                "sourcePlaylistName": source.get("name"),
                "selectionMode": selection.mode,
                "selectionCount": max_count,
            },
        )

    @staticmethod
    def _resolve_source_playlist(source_playlist, state: WorkflowState) -> dict[str, Any] | None:
        desired_raw = (source_playlist.raw_query or "").strip()
        desired_normalized = (source_playlist.normalized_query or "").strip()
        if source_playlist.playlist_id:
            return {
                "playlistId": source_playlist.playlist_id,
                "name": desired_raw or desired_normalized or "来源歌单",
            }

        candidates: list[dict[str, Any]] = []
        if state.last_named_playlist is not None:
            candidates.append(state.last_named_playlist)
        candidates.extend(state.recent_playlist_candidates)
        resolved_source = state.resolved_entities.get("source_playlist")
        if isinstance(resolved_source, dict):
            candidates.append(resolved_source)

        unique: dict[str, dict[str, Any]] = {}
        for item in candidates:
            playlist_id = str(item.get("playlistId") or "").strip()
            if playlist_id:
                unique[playlist_id] = item

        matches = []
        for item in unique.values():
            name = str(item.get("name") or "").strip()
            normalized_name = normalize_playlist_query(name) or ""
            if desired_raw and name == desired_raw:
                matches.append(item)
                continue
            if desired_normalized and normalized_name == desired_normalized:
                matches.append(item)
                continue
            if desired_raw and (desired_raw in name or name in desired_raw):
                matches.append(item)
                continue
            if desired_normalized and (
                desired_normalized in normalized_name or normalized_name in desired_normalized
            ):
                matches.append(item)

        if len(matches) != 1:
            return None
        return matches[0]
