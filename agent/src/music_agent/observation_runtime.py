from __future__ import annotations

"""
模块名: observation_runtime
功能概述: 收敛工具结果观察后的用户向后续决策，把 tool_result -> next action 的主链从 music_runtime 中拆分出来。
对外接口: ObservationRuntimeMixin
依赖关系: 依赖 response_style、runtime_actions、workflow_memory 中的结构；由 MusicAgentRuntime 继承
输入输出: 输入为会话状态、pending tool、工具结果；输出为下一步运行时动作
异常与错误: 依赖宿主 runtime 提供的工具下发与匹配能力；自身不直接做 transport
维护说明: 后续若继续拆分 execution/orchestration，应优先把 observation 相关新增逻辑加到这里
"""

from .response_style import (
    TEXT_NO_TRACK_FOUND,
    TEXT_OPERATION_DONE,
    TEXT_PLAYBACK_STOPPED,
    summarize_add_tracks_to_playlist,
    summarize_create_playlist,
    summarize_current_track,
    summarize_playlist_tracks,
    summarize_play_playlist,
    summarize_play_track,
    summarize_playlists,
    summarize_recent_tracks,
)
from .runtime_actions import AssistantTextRuntimeAction, ClarificationRuntimeAction
from .workflow_memory import PendingClarification


class ObservationRuntimeMixin:
    def _decide_next_action_after_observation(
        self,
        session_id: str,
        state,
        pending,
        result: dict,
    ):
        if pending.tool == "getCurrentTrack":
            state.workflow_mode = "chat"
            return AssistantTextRuntimeAction(summarize_current_track(result), pending.request_id, pending.user_message)

        if pending.tool == "stopPlayback":
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(TEXT_PLAYBACK_STOPPED, pending.request_id, pending.user_message)

        if pending.tool == "getPlaylists" and state.intent == "get_playlists":
            items = list(result.get("items", []))
            state.recent_playlist_candidates = items
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(summarize_playlists(items), pending.request_id, pending.user_message)

        if pending.tool == "getPlaylists" and state.intent == "query_playlist":
            return self._handle_playlist_query_result(session_id, state, pending, list(result.get("items", [])))

        if pending.tool == "getPlaylists" and state.intent == "inspect_playlist_tracks":
            return self._handle_playlist_track_lookup_result(session_id, state, pending, list(result.get("items", [])))

        if pending.tool == "searchTracks":
            items = list(result.get("items", []))
            state.recent_track_candidates = items
            if not items:
                state.workflow_mode = "chat"
                state.goal_status = "failed"
                return AssistantTextRuntimeAction(TEXT_NO_TRACK_FOUND, pending.request_id, pending.user_message)
            if len(items) == 1:
                state.last_named_track = items[0]
                return self._issue_tool_call(
                    session_id,
                    state,
                    pending.request_id,
                    pending.user_message,
                    "playTrack",
                    {"trackId": items[0]["trackId"]},
                )

            question, options = self._build_track_clarification(items)
            state.pending_clarification = PendingClarification("track_selection", "play_track", question, options, items)
            state.workflow_mode = "chat"
            state.goal_status = "waiting_clarification"
            return ClarificationRuntimeAction(question, options, pending.request_id, pending.user_message)

        if pending.tool == "getPlaylists" and state.intent == "play_playlist":
            items = list(result.get("items", []))
            raw_query, normalized_query = self._playlist_queries_from_state(state)
            matches = self._match_playlists(items, raw_query, normalized_query)
            if not raw_query and not normalized_query:
                matches = state.recent_playlist_candidates or items
            state.recent_playlist_candidates = matches or items
            if not matches:
                state.workflow_mode = "chat"
                state.goal_status = "failed"
                missing_name = raw_query or normalized_query or "目标"
                return AssistantTextRuntimeAction(f"没有找到名为“{missing_name}”的歌单。", pending.request_id, pending.user_message)
            if len(matches) == 1:
                state.last_named_playlist = matches[0]
                return self._issue_tool_call(
                    session_id,
                    state,
                    pending.request_id,
                    pending.user_message,
                    "playPlaylist",
                    {"playlistId": matches[0]["playlistId"]},
                )

            question, options = self._build_playlist_clarification(matches)
            state.pending_clarification = PendingClarification("playlist_selection", "play_playlist", question, options, matches)
            state.workflow_mode = "chat"
            state.goal_status = "waiting_clarification"
            return ClarificationRuntimeAction(question, options, pending.request_id, pending.user_message)

        if pending.tool == "getPlaylistTracks":
            playlist = result.get("playlist") or state.last_named_playlist or {}
            items = list(result.get("items", []))
            if playlist:
                state.last_named_playlist = playlist
            state.recent_playlist_tracks = items
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(
                summarize_playlist_tracks(playlist, items),
                pending.request_id,
                pending.user_message,
            )

        if pending.tool == "getRecentTracks":
            items = list(result.get("items", []))
            state.recent_recent_tracks = items
            state.recent_track_candidates = items
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(
                summarize_recent_tracks(items),
                pending.request_id,
                pending.user_message,
            )

        if pending.tool == "playTrack":
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            if result.get("track"):
                state.last_named_track = result.get("track")
            return AssistantTextRuntimeAction(summarize_play_track(result.get("track") or {}), pending.request_id, pending.user_message)

        if pending.tool == "playPlaylist":
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            if result.get("playlist"):
                state.last_named_playlist = result.get("playlist")
            return AssistantTextRuntimeAction(summarize_play_playlist(result.get("playlist") or {}), pending.request_id, pending.user_message)

        if pending.tool == "createPlaylist":
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            playlist = result.get("playlist") or {}
            if playlist:
                state.last_named_playlist = playlist
            return AssistantTextRuntimeAction(summarize_create_playlist(playlist), pending.request_id, pending.user_message)

        if pending.tool == "addTracksToPlaylist":
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            playlist = result.get("playlist") or state.last_named_playlist or {}
            if playlist:
                state.last_named_playlist = playlist
            added_count = int(result.get("addedCount", 0))
            return AssistantTextRuntimeAction(
                summarize_add_tracks_to_playlist(playlist, added_count),
                pending.request_id,
                pending.user_message,
            )

        state.workflow_mode = "chat"
        state.goal_status = "completed"
        return AssistantTextRuntimeAction(TEXT_OPERATION_DONE, pending.request_id, pending.user_message)

    def _handle_playlist_query_result(
        self,
        session_id: str,
        state,
        pending,
        items: list[dict],
    ):
        raw_query, normalized_query = self._playlist_queries_from_state(state)
        matches = self._match_playlists(items, raw_query, normalized_query)
        state.recent_playlist_candidates = matches or items
        if not matches:
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            query = raw_query or normalized_query or "目标"
            return AssistantTextRuntimeAction(f"没有找到名为“{query}”的歌单。", pending.request_id, pending.user_message)
        if len(matches) == 1:
            state.last_named_playlist = matches[0]
            state.resolved_entities["playlist"] = matches[0]
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(self._summarize_single_playlist(matches[0]), pending.request_id, pending.user_message)

        question, options = self._build_playlist_query_clarification(matches)
        state.pending_clarification = PendingClarification(
            "playlist_query_selection",
            "query_playlist",
            question,
            options,
            matches,
        )
        state.workflow_mode = "chat"
        state.goal_status = "waiting_clarification"
        return ClarificationRuntimeAction(question, options, pending.request_id, pending.user_message)

    def _handle_playlist_track_lookup_result(
        self,
        session_id: str,
        state,
        pending,
        items: list[dict],
    ):
        raw_query, normalized_query = self._playlist_queries_from_state(state)
        matches = self._match_playlists(items, raw_query, normalized_query)
        state.recent_playlist_candidates = matches or items
        if not matches:
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            query = raw_query or normalized_query or "目标"
            return AssistantTextRuntimeAction(f"没有找到名为“{query}”的歌单。", pending.request_id, pending.user_message)
        if len(matches) == 1:
            state.last_named_playlist = matches[0]
            state.resolved_entities["playlist"] = matches[0]
            state.goal_status = "running"
            return self._issue_tool_call(
                session_id,
                state,
                pending.request_id,
                pending.user_message,
                "getPlaylistTracks",
                {"playlistId": matches[0]["playlistId"]},
            )

        question, options = self._build_playlist_track_clarification(matches)
        state.pending_clarification = PendingClarification(
            "playlist_tracks_selection",
            "inspect_playlist_tracks",
            question,
            options,
            matches,
        )
        state.workflow_mode = "chat"
        state.goal_status = "waiting_clarification"
        return ClarificationRuntimeAction(question, options, pending.request_id, pending.user_message)
