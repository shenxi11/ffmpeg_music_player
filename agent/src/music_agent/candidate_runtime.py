from __future__ import annotations

"""
模块名: candidate_runtime
功能概述: 收敛 action candidates 的 observation 续调、参数物化和候选执行推进逻辑。
对外接口: CandidateRuntimeMixin
依赖关系: 依赖 response_style、runtime_actions、workflow_memory；由 MusicAgentRuntime 继承
输入输出: 输入为 pending tool、当前 candidate 队列与工具结果；输出为下一步运行时动作或物化参数
异常与错误: 依赖宿主 runtime 提供 _issue_tool_call()、_match_playlists() 等桥接能力
维护说明: 后续若继续扩 planner/replan，应优先在这里扩 candidate observation，而不是回填到 runtime 主文件
"""

from .capability_catalog import get_capability
from .response_style import TEXT_NO_TRACK_FOUND
from .runtime_actions import AssistantTextRuntimeAction, ClarificationRuntimeAction
from .workflow_memory import PendingClarification


class CandidateRuntimeMixin:
    def _continue_action_candidates_after_observation(
        self,
        session_id: str,
        state,
        pending,
        result: dict,
    ):
        if pending.tool == "createPlaylist":
            playlist = result.get("playlist") or {}
            if playlist:
                state.last_named_playlist = playlist
                state.resolved_entities["playlist"] = playlist
                if state.intent in {
                    "create_playlist",
                    "create_playlist_with_top_tracks",
                    "create_playlist_from_playlist_subset",
                }:
                    state.resolved_entities["target_playlist"] = playlist

        if pending.tool == "getTopPlayedTracks":
            items = list(result.get("items", []))
            state.recent_recent_tracks = items
            state.recent_track_candidates = items

        if pending.tool == "getPlaylistTracks":
            playlist = result.get("playlist") or {}
            items = list(result.get("items", []))
            state.recent_playlist_tracks = items
            if playlist:
                state.last_named_playlist = playlist
                if state.active_action_candidate and str((state.active_action_candidate.get("args") or {}).get("playlistRole") or "") == "source":
                    state.resolved_entities["source_playlist"] = playlist

        if state.active_action_candidate is not None:
            step_id = str(state.active_action_candidate.get("stepId") or "")
            if step_id:
                self._append_bounded_unique(state.completed_action_candidate_ids, step_id, limit=16)
            self._append_bounded(
                state.action_observation_history,
                {
                    "stepId": step_id,
                    "tool": pending.tool,
                    "resultKeys": sorted(result.keys()),
                },
                limit=12,
            )

        if not state.pending_action_candidates:
            state.active_action_candidate = None
            return None

        next_candidate = state.pending_action_candidates[0]
        tool = str(next_candidate.get("tool") or "")

        if pending.tool == "createPlaylist" and tool in {"getTopPlayedTracks", "playPlaylist", "getPlaylists", "getPlaylistTracks", "addTracksToPlaylist"}:
            materialized = self._hydrate_candidate_args_from_state(next_candidate, state)
            return self._dispatch_materialized_candidate(session_id, state, pending, next_candidate, tool, materialized)

        if pending.tool == "getPlaylists" and tool in {"playPlaylist", "getPlaylistTracks"}:
            materialized = self._materialize_playlist_candidate_from_lookup(state, next_candidate, list(result.get("items", [])))
            if materialized is not None:
                if not isinstance(materialized, dict):
                    materialized.request_id = pending.request_id
                    materialized.persist_user_message = pending.user_message
                    return materialized
                return self._dispatch_materialized_candidate(session_id, state, pending, next_candidate, tool, materialized)

        if pending.tool == "searchTracks" and tool == "playTrack":
            materialized = self._materialize_track_candidate_from_lookup(state, next_candidate, list(result.get("items", [])))
            if materialized is not None:
                if not isinstance(materialized, dict):
                    materialized.request_id = pending.request_id
                    materialized.persist_user_message = pending.user_message
                    return materialized
                return self._dispatch_materialized_candidate(session_id, state, pending, next_candidate, tool, materialized)

        if pending.tool == "getTopPlayedTracks" and tool == "addTracksToPlaylist":
            materialized = self._materialize_add_tracks_candidate(state, next_candidate, list(result.get("items", [])))
            if materialized is not None:
                if not isinstance(materialized, dict):
                    materialized.request_id = pending.request_id
                    materialized.persist_user_message = pending.user_message
                    return materialized
                return self._dispatch_materialized_candidate(session_id, state, pending, next_candidate, tool, materialized)

        if pending.tool == "getPlaylistTracks" and tool == "playTrack":
            materialized = self._materialize_track_candidate_from_lookup(state, next_candidate, list(result.get("items", [])))
            if materialized is not None:
                if not isinstance(materialized, dict):
                    materialized.request_id = pending.request_id
                    materialized.persist_user_message = pending.user_message
                    return materialized
                return self._dispatch_materialized_candidate(session_id, state, pending, next_candidate, tool, materialized)

        if pending.tool == "getPlaylistTracks" and tool == "addTracksToPlaylist":
            materialized = self._materialize_add_tracks_candidate(state, next_candidate, list(result.get("items", [])))
            if materialized is not None:
                if not isinstance(materialized, dict):
                    materialized.request_id = pending.request_id
                    materialized.persist_user_message = pending.user_message
                    return materialized
                return self._dispatch_materialized_candidate(session_id, state, pending, next_candidate, tool, materialized)

        state.active_action_candidate = None
        return None

    def _dispatch_materialized_candidate(
        self,
        session_id: str,
        state,
        pending,
        candidate: dict,
        tool: str,
        args: dict,
    ):
        try:
            action = self._issue_tool_call(
                session_id,
                state,
                pending.request_id,
                pending.user_message,
                tool,
                args,
            )
        except ValueError:
            state.active_action_candidate = None
            return None

        state.active_action_candidate = self._build_active_candidate_snapshot(candidate, args, source="observation")
        if state.pending_action_candidates:
            state.pending_action_candidates.pop(0)
        return action

    def _build_active_candidate_snapshot(self, candidate: dict, resolved_args: dict, source: str) -> dict:
        snapshot = dict(candidate)
        snapshot["resolvedArgs"] = dict(resolved_args)
        snapshot["activationSource"] = source
        return snapshot

    def _is_candidate_tool_allowed(self, definition) -> bool:
        capability = get_capability(definition.name)
        if capability is not None and (not capability.exposed_to_backend or capability.automation_policy == "restricted"):
            return False
        return definition.direct_execute or (self._allow_direct_write_actions and not definition.read_only)

    def _hydrate_candidate_args_from_state(self, candidate: dict, state) -> dict:
        hydrated = dict(candidate.get("args") or {})
        tool = str(candidate.get("tool") or "")
        playlist_role = str(hydrated.get("playlistRole") or "")
        if tool in {"playPlaylist", "getPlaylistTracks", "addTracksToPlaylist"} and "playlistId" not in hydrated:
            if playlist_role == "target" and state.resolved_entities.get("target_playlist"):
                hydrated["playlistId"] = state.resolved_entities["target_playlist"].get("playlistId")
            elif playlist_role == "source" and state.resolved_entities.get("source_playlist"):
                hydrated["playlistId"] = state.resolved_entities["source_playlist"].get("playlistId")
            elif state.last_named_playlist is not None:
                hydrated["playlistId"] = state.last_named_playlist.get("playlistId")
        if tool == "addTracksToPlaylist" and "trackIds" not in hydrated:
            if hydrated.get("selectionMode") == "first_n" and hydrated.get("count") and state.recent_playlist_tracks:
                track_ids = [
                    item.get("trackId")
                    for item in state.recent_playlist_tracks[: int(hydrated["count"])]
                    if item.get("trackId")
                ]
            else:
                recent_items = state.recent_recent_tracks or state.recent_track_candidates
                track_ids = [item.get("trackId") for item in recent_items if item.get("trackId")]
            if track_ids:
                hydrated["trackIds"] = track_ids
        if tool == "getTopPlayedTracks" and "limit" not in hydrated:
            hydrated["limit"] = self.TOP_TRACK_LIMIT
        return hydrated

    def _materialize_playlist_candidate_from_lookup(self, state, candidate: dict, items: list[dict]):
        raw_query, normalized_query = self._playlist_queries_from_candidate_or_state(state, candidate)
        matches = self._match_playlists(items, raw_query, normalized_query)
        state.recent_playlist_candidates = matches or items
        if not matches:
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            query = raw_query or normalized_query or "目标"
            state.active_action_candidate = None
            state.pending_action_candidates.clear()
            return AssistantTextRuntimeAction(f"没有找到名为“{query}”的歌单。", None, "")
        if len(matches) > 1:
            question, options = self._build_playlist_clarification(matches)
            resolution_action = "play_playlist" if candidate.get("tool") == "playPlaylist" else "inspect_playlist_tracks"
            state.pending_clarification = PendingClarification("playlist_selection", resolution_action, question, options, matches)
            state.workflow_mode = "chat"
            state.goal_status = "waiting_clarification"
            state.active_action_candidate = None
            state.pending_action_candidates.clear()
            return ClarificationRuntimeAction(question, options, None, "")

        playlist_role = str((candidate.get("args") or {}).get("playlistRole") or "")
        state.last_named_playlist = matches[0]
        state.resolved_entities["playlist"] = matches[0]
        if playlist_role == "source":
            state.resolved_entities["source_playlist"] = matches[0]
        elif playlist_role == "target":
            state.resolved_entities["target_playlist"] = matches[0]
        materialized_args = dict(candidate.get("args") or {})
        materialized_args["playlistId"] = matches[0]["playlistId"]
        return materialized_args

    def _materialize_track_candidate_from_lookup(self, state, candidate: dict, items: list[dict]):
        state.recent_track_candidates = items
        if not items:
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            state.active_action_candidate = None
            state.pending_action_candidates.clear()
            return AssistantTextRuntimeAction(TEXT_NO_TRACK_FOUND, None, "")
        selection_index = self._candidate_selection_index(candidate)
        if selection_index is not None:
            selected = self._select_track_by_index(items, selection_index)
            if selected is None:
                state.workflow_mode = "chat"
                state.goal_status = "failed"
                state.active_action_candidate = None
                state.pending_action_candidates.clear()
                return AssistantTextRuntimeAction(TEXT_NO_TRACK_FOUND, None, "")
            state.last_named_track = selected
            state.resolved_entities["track"] = selected
            materialized_args = dict(candidate.get("args") or {})
            materialized_args["trackId"] = selected["trackId"]
            materialized_args.pop("selectionIndex", None)
            materialized_args.pop("trackIndex", None)
            materialized_args.pop("ordinal", None)
            return materialized_args
        if len(items) > 1:
            question, options = self._build_track_clarification(items)
            state.pending_clarification = PendingClarification("track_selection", "play_track", question, options, items)
            state.workflow_mode = "chat"
            state.goal_status = "waiting_clarification"
            state.active_action_candidate = None
            state.pending_action_candidates.clear()
            return ClarificationRuntimeAction(question, options, None, "")

        state.last_named_track = items[0]
        state.resolved_entities["track"] = items[0]
        materialized_args = dict(candidate.get("args") or {})
        materialized_args["trackId"] = items[0]["trackId"]
        return materialized_args

    def _materialize_add_tracks_candidate(self, state, candidate: dict, items: list[dict]):
        materialized_args = dict(candidate.get("args") or {})
        playlist_role = str(materialized_args.get("playlistRole") or "")
        if playlist_role == "source":
            state.recent_playlist_tracks = items
        else:
            state.recent_recent_tracks = items
            state.recent_track_candidates = items
        if not items:
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            state.active_action_candidate = None
            state.pending_action_candidates.clear()
            return AssistantTextRuntimeAction("最近常听歌曲列表为空，未向歌单添加内容。", None, "")
        target_playlist = state.resolved_entities.get("target_playlist") or state.last_named_playlist
        if target_playlist is None or not target_playlist.get("playlistId"):
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            state.active_action_candidate = None
            state.pending_action_candidates.clear()
            return AssistantTextRuntimeAction("还没有可写入的目标歌单。", None, "")

        materialized_args["playlistId"] = target_playlist["playlistId"]
        selected_items = self._select_tracks_by_selection(
            items,
            str(materialized_args.get("selectionMode") or ""),
            materialized_args.get("count"),
        )
        materialized_args["trackIds"] = [item["trackId"] for item in selected_items if item.get("trackId")]
        return materialized_args

    @staticmethod
    def _candidate_selection_index(candidate: dict) -> int | None:
        args = dict(candidate.get("args") or {})
        for key in ("selectionIndex", "trackIndex", "ordinal"):
            raw_value = args.get(key)
            if raw_value is None:
                continue
            try:
                index = int(raw_value)
            except (TypeError, ValueError):
                continue
            if index >= 1:
                return index
        return None

    @staticmethod
    def _select_track_by_index(items: list[dict], index: int) -> dict | None:
        if index < 1 or index > len(items):
            return None
        return items[index - 1]

    @staticmethod
    def _select_tracks_by_selection(items: list[dict], mode: str, count_raw) -> list[dict]:
        if not items:
            return []
        try:
            count = int(count_raw) if count_raw is not None else len(items)
        except (TypeError, ValueError):
            count = len(items)
        count = max(1, min(count, len(items)))
        if mode == "first_n":
            return items[:count]
        if mode == "last_n":
            return items[-count:]
        return items
