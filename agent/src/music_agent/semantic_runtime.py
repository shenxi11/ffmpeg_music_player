from __future__ import annotations

"""
模块名: semantic_runtime
功能概述: 收敛 user_message 进入后的语义解析、上下文构造、语义应用与语义驱动的入口决策。
对外接口: SemanticRuntimeMixin
依赖关系: 依赖 semantic_parser/refiner、world_state、policy_hooks、response_style、runtime_actions；由 MusicAgentRuntime 继承
输入输出: 输入为用户消息与工作记忆；输出为结构化语义结果和入口级运行时动作
异常与错误: LLM 语义解析超时或失败时自动回退到 legacy 规则结果
维护说明: 后续若继续推进 Goal Understanding / Object Resolution / Capability Routing 分层，应优先在本模块继续拆解
"""

import asyncio

from .capability_catalog import capability_architecture_summary, semantic_capability_hints
from .policy_hooks import (
    compound_script_block_message,
    looks_like_non_play_request,
    unsupported_capability_block_message,
)
from .response_style import (
    TEXT_ASK_CREATE_PLAYLIST_NAME,
    TEXT_ASK_PLAYLIST_FOR_TRACKS,
    TEXT_ASK_PLAYLIST_NAME,
    TEXT_ASK_TRACK_NAME,
    summarize_single_playlist,
)
from .runtime_actions import AssistantTextRuntimeAction, ScriptDryRunRuntimeAction
from .semantic_models import ActionCandidate, SemanticParseResult, legacy_intent_to_semantic_result
from .semantic_parser import SemanticParserError
from .semantic_refiner import refine_semantic_result
from .workflow_memory import PendingScriptDryRun
from .world_state import build_world_state_summary


class SemanticRuntimeMixin:
    async def _parse_semantics(self, user_message: str, state) -> SemanticParseResult:
        context = self._build_semantic_context(state)
        legacy_result = refine_semantic_result(
            legacy_intent_to_semantic_result(self._parse_intent(user_message)),
            user_message,
            context,
        )

        if legacy_result.intent == "chat" and not self._should_force_llm_semantics(user_message, state):
            return legacy_result

        if self._semantic_parser is None:
            return legacy_result

        try:
            llm_client = self._chat_agent.llm_client
            settings = getattr(llm_client, "_settings", None)
            semantic_timeout = float(getattr(settings, "agent_semantic_timeout_seconds", 6.0))
            return await asyncio.wait_for(
                self._semantic_parser.parse_user_message(
                    user_message,
                    context,
                ),
                timeout=max(1.0, semantic_timeout),
            )
        except (asyncio.TimeoutError, SemanticParserError):
            return legacy_result

    @staticmethod
    def _should_force_llm_semantics(user_message: str, state) -> bool:
        normalized = user_message.strip()
        if not normalized:
            return False
        operational_keywords = (
            "歌单",
            "播放",
            "停止",
            "停一下",
            "停掉",
            "别放了",
            "放一下",
            "查询",
            "查找",
            "看看",
            "查看",
            "创建",
            "新建",
            "加入",
            "添加",
            "最近",
            "当前播放",
        )
        if any(keyword in normalized for keyword in operational_keywords):
            return True
        contextual_keywords = (
            "这个",
            "那个",
            "刚才",
            "上一个",
            "第一个",
            "第二个",
            "播放",
            "放一下",
            "创建",
            "新建",
            "加入",
            "添加",
        )
        has_context = any(
            (
                state.last_named_playlist,
                state.last_named_track,
                state.recent_playlist_candidates,
                state.recent_track_candidates,
                state.recent_playlist_tracks,
                state.recent_recent_tracks,
                state.current_goal,
            )
        )
        return has_context and any(keyword in normalized for keyword in contextual_keywords)

    def _build_semantic_context(self, state) -> dict:
        return {
            "currentGoal": state.current_goal,
            "goalHistory": list(state.goal_history[-5:]),
            "goalStatus": state.goal_status,
            "currentIntent": state.intent,
            "focusDomain": state.focus_domain,
            "workflowMode": state.workflow_mode,
            "lastNamedPlaylist": self._summarize_playlist_memory(state.last_named_playlist),
            "lastNamedTrack": self._summarize_track_memory(state.last_named_track),
            "recentPlaylistCandidates": [self._summarize_playlist_memory(item) for item in state.recent_playlist_candidates[:5]],
            "recentTrackCandidates": [self._summarize_track_memory(item) for item in state.recent_track_candidates[:5]],
            "recentPlaylistTracks": [self._summarize_track_memory(item) for item in state.recent_playlist_tracks[:5]],
            "recentRecentTracks": [self._summarize_track_memory(item) for item in state.recent_recent_tracks[:5]],
            "resolvedEntities": state.resolved_entities,
            "lastToolObservation": state.last_tool_observation,
            "semanticHistory": list(state.semantic_history[-5:]),
            "worldState": build_world_state_summary(state),
            "capabilityArchitecture": capability_architecture_summary(),
            "capabilityHints": semantic_capability_hints(),
        }

    @staticmethod
    def _summarize_playlist_memory(playlist: dict | None) -> dict | None:
        if not playlist:
            return None
        return {
            "playlistId": playlist.get("playlistId"),
            "name": playlist.get("name"),
            "trackCount": playlist.get("trackCount"),
        }

    @staticmethod
    def _summarize_track_memory(track: dict | None) -> dict | None:
        if not track:
            return None
        return {
            "trackId": track.get("trackId"),
            "title": track.get("title"),
            "artist": track.get("artist"),
            "album": track.get("album"),
        }

    @staticmethod
    def _semantic_to_intent_payload(semantic: SemanticParseResult) -> dict:
        playlist = semantic.entities.playlist
        target_playlist = semantic.entities.target_playlist
        source_playlist = semantic.entities.source_playlist
        track_selection = semantic.entities.track_selection
        track = semantic.entities.track
        entities: dict[str, object] = {
            "playlistName": playlist.raw_query if playlist else "",
            "playlistNormalized": playlist.normalized_query if playlist else "",
            "playlistId": playlist.playlist_id if playlist else "",
            "targetPlaylistName": target_playlist.raw_query if target_playlist else "",
            "targetPlaylistNormalized": target_playlist.normalized_query if target_playlist else "",
            "targetPlaylistId": target_playlist.playlist_id if target_playlist else "",
            "sourcePlaylistName": source_playlist.raw_query if source_playlist else "",
            "sourcePlaylistNormalized": source_playlist.normalized_query if source_playlist else "",
            "sourcePlaylistId": source_playlist.playlist_id if source_playlist else "",
            "trackSelectionMode": track_selection.mode if track_selection else "",
            "trackSelectionCount": track_selection.count if track_selection else None,
            "title": track.raw_query if track else "",
            "keyword": track.normalized_query if track else "",
            "trackId": track.track_id if track else "",
            "artist": semantic.entities.artist or (track.artist if track else "") or "",
            "album": semantic.entities.album or (track.album if track else "") or "",
            "limit": semantic.entities.limit,
            "references": list(semantic.references),
            "missingFields": list(semantic.missing_fields),
            "shouldAutoExecute": semantic.should_auto_execute,
            "requiresApproval": semantic.requires_approval,
            "targetDomain": semantic.target_domain,
            "confidence": semantic.confidence,
        }
        return {
            "mode": semantic.mode,
            "intent": semantic.intent,
            "entities": entities,
            "ambiguities": list(semantic.ambiguities),
        }

    def _apply_semantic_result(self, state, semantic: SemanticParseResult, payload: dict) -> None:
        state.current_goal = semantic.intent
        state.focus_domain = semantic.target_domain
        state.intent = semantic.intent
        state.entities = payload["entities"]
        state.ambiguities = payload["ambiguities"]
        self._append_bounded_unique(state.goal_history, semantic.intent, limit=8)
        self._append_bounded(
            state.semantic_history,
            {
                "intent": semantic.intent,
                "mode": semantic.mode,
                "targetDomain": semantic.target_domain,
                "references": list(semantic.references),
                "missingFields": list(semantic.missing_fields),
                "confidence": semantic.confidence,
                "actionCandidates": [candidate.model_dump(by_alias=True) for candidate in semantic.action_candidates],
            },
            limit=8,
        )
        if semantic.entities.playlist is not None:
            state.resolved_entities["playlist_query"] = {
                "rawQuery": semantic.entities.playlist.raw_query,
                "normalizedQuery": semantic.entities.playlist.normalized_query,
                "playlistId": semantic.entities.playlist.playlist_id,
            }
        if semantic.entities.target_playlist is not None:
            state.resolved_entities["target_playlist_query"] = {
                "rawQuery": semantic.entities.target_playlist.raw_query,
                "normalizedQuery": semantic.entities.target_playlist.normalized_query,
                "playlistId": semantic.entities.target_playlist.playlist_id,
            }
        if semantic.entities.source_playlist is not None:
            state.resolved_entities["source_playlist_query"] = {
                "rawQuery": semantic.entities.source_playlist.raw_query,
                "normalizedQuery": semantic.entities.source_playlist.normalized_query,
                "playlistId": semantic.entities.source_playlist.playlist_id,
            }
        if semantic.entities.track_selection is not None:
            state.resolved_entities["track_selection"] = {
                "mode": semantic.entities.track_selection.mode,
                "count": semantic.entities.track_selection.count,
            }

    def _hydrate_semantic_action_candidates(self, semantic: SemanticParseResult, state) -> None:
        if semantic.action_candidates:
            return
        planned = self._autonomous_planner.build_action_candidates(semantic, state)
        if planned:
            semantic.action_candidates = [ActionCandidate.model_validate(item) for item in planned]
        if semantic.entities.track is not None:
            state.resolved_entities["track_query"] = {
                "rawQuery": semantic.entities.track.raw_query,
                "normalizedQuery": semantic.entities.track.normalized_query,
                "trackId": semantic.entities.track.track_id,
                "artist": semantic.entities.track.artist,
                "album": semantic.entities.track.album,
            }

    @staticmethod
    def _append_bounded(target: list, value, limit: int) -> None:
        target.append(value)
        if len(target) > limit:
            del target[:-limit]

    @staticmethod
    def _append_bounded_unique(target: list[str], value: str | None, limit: int) -> None:
        if not value:
            return
        if target and target[-1] == value:
            return
        target.append(value)
        if len(target) > limit:
            del target[:-limit]

    def _resolve_semantic_reference_action(
        self,
        session_id: str,
        state,
        request_id: str | None,
        user_message: str,
        semantic: SemanticParseResult,
        payload: dict,
    ):
        references = set(payload["entities"].get("references", []))
        playlist_name = str(payload["entities"].get("playlistName", "") or "").strip()

        if semantic.intent == "play_track" and "last_named_track" in references and state.last_named_track is not None:
            if looks_like_non_play_request(user_message):
                return None
            state.goal_status = "running"
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                user_message,
                "playTrack",
                {"trackId": state.last_named_track["trackId"]},
            )

        if semantic.intent == "play_playlist" and "last_named_playlist" in references and state.last_named_playlist is not None:
            state.goal_status = "running"
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                user_message,
                "playPlaylist",
                {"playlistId": state.last_named_playlist["playlistId"]},
            )

        if semantic.intent == "play_playlist" and not playlist_name and state.recent_playlist_candidates:
            selected_playlist = self._select_candidate(state.recent_playlist_candidates, user_message)
            if selected_playlist is not None:
                state.last_named_playlist = selected_playlist
                state.resolved_entities["playlist"] = selected_playlist
                state.goal_status = "running"
                return self._issue_tool_call(
                    session_id,
                    state,
                    request_id,
                    user_message,
                    "playPlaylist",
                    {"playlistId": selected_playlist["playlistId"]},
                )

        if semantic.intent == "inspect_playlist_tracks" and "last_named_playlist" in references and state.last_named_playlist is not None:
            state.goal_status = "running"
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                user_message,
                "getPlaylistTracks",
                {"playlistId": state.last_named_playlist["playlistId"]},
            )

        if semantic.intent == "inspect_playlist_tracks" and not playlist_name and state.recent_playlist_candidates:
            selected_playlist = self._select_candidate(state.recent_playlist_candidates, user_message)
            if selected_playlist is not None:
                state.last_named_playlist = selected_playlist
                state.resolved_entities["playlist"] = selected_playlist
                state.goal_status = "running"
                return self._issue_tool_call(
                    session_id,
                    state,
                    request_id,
                    user_message,
                    "getPlaylistTracks",
                    {"playlistId": selected_playlist["playlistId"]},
                )

        if semantic.intent == "query_playlist" and "last_named_playlist" in references and state.last_named_playlist is not None:
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(summarize_single_playlist(state.last_named_playlist), request_id, user_message)

        return None

    def _resolve_llm_action_candidates_action(
        self,
        session_id: str,
        state,
        request_id: str | None,
        user_message: str,
        semantic: SemanticParseResult,
    ):
        if semantic.missing_fields:
            return None
        if not semantic.action_candidates:
            return None

        state.pending_action_candidates = [candidate.model_dump(by_alias=True) for candidate in semantic.action_candidates]
        state.active_action_candidate = None
        state.completed_action_candidate_ids = []
        state.action_observation_history.clear()
        action = self._issue_next_action_candidate(session_id, state, request_id, user_message, semantic)
        if action is None:
            state.pending_action_candidates.clear()
            state.active_action_candidate = None
            state.completed_action_candidate_ids.clear()
        return action

    def _resolve_client_script_action(
        self,
        session_id: str,
        state,
        request_id: str | None,
        user_message: str,
        semantic: SemanticParseResult,
    ):
        route = self._execution_router.route(semantic, state)
        if route.status == "blocked":
            payload = self._build_execution_route_payload(semantic, route)
            blocked_capability = str((route.metadata or {}).get("blockedCapability") or "").strip() or None
            blocked_message = (
                unsupported_capability_block_message(blocked_capability)
                if route.reason.startswith("unsupported_capability:")
                else compound_script_block_message()
            )
            return AssistantTextRuntimeAction(
                text=blocked_message,
                request_id=request_id,
                persist_user_message=user_message,
                audit_event_type="execution_substrate_rejected",
                audit_payload=payload,
            )

        if route.status != "selected":
            return None

        if route.substrate != "script":
            return None

        planning = route.script_planning
        if planning is None or planning.script is None:
            payload = self._build_execution_route_payload(semantic, route)
            return AssistantTextRuntimeAction(
                text=compound_script_block_message(),
                request_id=request_id,
                persist_user_message=user_message,
                audit_event_type="execution_substrate_rejected",
                audit_payload=payload,
            )

        script = planning.script
        state.workflow_mode = "tool"
        state.goal_status = "running"
        state.pending_script_dry_run = PendingScriptDryRun(
            request_id=request_id,
            user_message=user_message,
            script=script,
        )
        return ScriptDryRunRuntimeAction(
            script=script,
            request_id=request_id,
            persist_user_message=user_message,
            audit_event_type="execution_substrate_selected",
            audit_payload=self._build_execution_route_payload(semantic, route),
        )

    @staticmethod
    def _build_execution_route_payload(semantic: SemanticParseResult, route) -> dict:
        target_playlist = semantic.entities.target_playlist
        source_playlist = semantic.entities.source_playlist
        track_selection = semantic.entities.track_selection
        return {
            "semanticIntent": semantic.intent,
            "routeStatus": route.status,
            "routeSubstrate": route.substrate,
            "routeReason": route.reason,
            "templateName": route.template.template_name,
            "templateReason": route.template.reason,
            "metadata": route.metadata,
            "plannerStatus": route.script_planning.status if route.script_planning else None,
            "plannerReason": route.script_planning.reason if route.script_planning else None,
            "plannerMetadata": route.script_planning.metadata if route.script_planning else {},
            "targetPlaylist": {
                "rawQuery": target_playlist.raw_query if target_playlist else None,
                "normalizedQuery": target_playlist.normalized_query if target_playlist else None,
                "playlistId": target_playlist.playlist_id if target_playlist else None,
            },
            "sourcePlaylist": {
                "rawQuery": source_playlist.raw_query if source_playlist else None,
                "normalizedQuery": source_playlist.normalized_query if source_playlist else None,
                "playlistId": source_playlist.playlist_id if source_playlist else None,
            },
            "trackSelection": {
                "mode": track_selection.mode if track_selection else None,
                "count": track_selection.count if track_selection else None,
            },
        }

    def _resolve_llm_proposed_tool_action(
        self,
        session_id: str,
        state,
        request_id: str | None,
        user_message: str,
        semantic: SemanticParseResult,
    ):
        proposal = semantic.proposed_tool
        if proposal is None:
            return None

        definition = self._tool_registry.get(proposal.tool)
        if definition is None:
            return None

        if not self._is_candidate_tool_allowed(definition):
            return None

        proposed_args = dict(proposal.args)
        proposed_args = self._hydrate_proposed_tool_args(semantic, state, proposal.tool, proposed_args)

        try:
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                user_message,
                proposal.tool,
                proposed_args,
            )
        except ValueError:
            return None

    def _issue_next_action_candidate(
        self,
        session_id: str,
        state,
        request_id: str | None,
        user_message: str,
        semantic: SemanticParseResult,
    ):
        completed_ids = set(state.completed_action_candidate_ids)
        for index, candidate in enumerate(list(state.pending_action_candidates)):
            if not self._candidate_dependencies_met(candidate, completed_ids):
                continue
            if candidate.get("kind", "tool") != "tool":
                continue
            tool = str(candidate.get("tool") or "").strip()
            if not tool:
                continue

            definition = self._tool_registry.get(tool)
            if definition is None:
                continue
            if not self._is_candidate_tool_allowed(definition):
                continue

            args = self._hydrate_candidate_args_from_semantic(candidate, semantic, state)
            try:
                action = self._issue_tool_call(session_id, state, request_id, user_message, tool, args)
            except ValueError:
                continue

            state.active_action_candidate = self._build_active_candidate_snapshot(candidate, args, source="semantic")
            del state.pending_action_candidates[index]
            return action
        return None

    def _handle_missing_fields(
        self,
        state,
        request_id: str | None,
        user_message: str,
        payload: dict,
    ):
        missing_fields = set(payload["entities"].get("missingFields", []))
        intent = payload["intent"]

        if intent == "play_track" and {"track", "title"} & missing_fields:
            state.workflow_mode = "chat"
            state.goal_status = "waiting_user"
            return AssistantTextRuntimeAction(TEXT_ASK_TRACK_NAME, request_id, user_message)

        if intent in {"query_playlist", "play_playlist"} and "playlist" in missing_fields and state.last_named_playlist is None:
            state.workflow_mode = "chat"
            state.goal_status = "waiting_user"
            return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_NAME, request_id, user_message)

        if intent == "inspect_playlist_tracks" and "playlist" in missing_fields and state.last_named_playlist is None:
            state.workflow_mode = "chat"
            state.goal_status = "waiting_user"
            return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_FOR_TRACKS, request_id, user_message)

        if intent in {"create_playlist", "create_playlist_with_top_tracks"} and "playlist" in missing_fields:
            state.workflow_mode = "chat"
            state.goal_status = "waiting_user"
            return AssistantTextRuntimeAction(TEXT_ASK_CREATE_PLAYLIST_NAME, request_id, user_message)

        if intent == "create_playlist_from_playlist_subset":
            if "targetPlaylist" in missing_fields:
                state.workflow_mode = "chat"
                state.goal_status = "waiting_user"
                return AssistantTextRuntimeAction(TEXT_ASK_CREATE_PLAYLIST_NAME, request_id, user_message)
            if "sourcePlaylist" in missing_fields:
                state.workflow_mode = "chat"
                state.goal_status = "waiting_user"
                return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_NAME, request_id, user_message)

        return None

    @staticmethod
    def _candidate_dependencies_met(candidate: dict, completed_ids: set[str]) -> bool:
        depends_on = candidate.get("dependsOn") or []
        return all(step_id in completed_ids for step_id in depends_on)

    def _hydrate_candidate_args_from_semantic(self, candidate: dict, semantic: SemanticParseResult, state) -> dict:
        return self._hydrate_proposed_tool_args(
            semantic,
            state,
            str(candidate.get("tool") or ""),
            dict(candidate.get("args") or {}),
        )

    def _hydrate_proposed_tool_args(
        self,
        semantic: SemanticParseResult,
        state,
        tool: str,
        args: dict,
    ) -> dict:
        hydrated = dict(args)
        playlist = semantic.entities.playlist
        target_playlist = semantic.entities.target_playlist
        source_playlist = semantic.entities.source_playlist
        track = semantic.entities.track
        references = set(semantic.references)
        playlist_role = str(hydrated.get("playlistRole") or "")

        def _playlist_entity_for_role():
            if playlist_role == "target":
                return target_playlist
            if playlist_role == "source":
                return source_playlist
            return playlist

        if tool in {"playPlaylist", "getPlaylistTracks", "addTracksToPlaylist"} and "playlistId" not in hydrated:
            playlist_entity = _playlist_entity_for_role()
            if playlist_entity is not None and playlist_entity.playlist_id:
                hydrated["playlistId"] = playlist_entity.playlist_id
            elif playlist_role == "target" and state.resolved_entities.get("target_playlist"):
                hydrated["playlistId"] = state.resolved_entities["target_playlist"].get("playlistId")
            elif playlist_role == "source" and state.resolved_entities.get("source_playlist"):
                hydrated["playlistId"] = state.resolved_entities["source_playlist"].get("playlistId")
            elif "last_named_playlist" in references and state.last_named_playlist is not None:
                hydrated["playlistId"] = state.last_named_playlist.get("playlistId")

        if tool == "playTrack" and "trackId" not in hydrated:
            if track is not None and track.track_id:
                hydrated["trackId"] = track.track_id
            elif "last_named_track" in references and state.last_named_track is not None:
                hydrated["trackId"] = state.last_named_track.get("trackId")

        if tool == "searchTracks":
            if "keyword" not in hydrated and track is not None:
                hydrated["keyword"] = track.raw_query or track.normalized_query
            if "artist" not in hydrated:
                hydrated["artist"] = semantic.entities.artist or (track.artist if track else None)
            if "album" not in hydrated and track is not None:
                hydrated["album"] = track.album

        if tool == "getRecentTracks" and "limit" not in hydrated and semantic.entities.limit is not None:
            hydrated["limit"] = semantic.entities.limit

        if tool == "getTopPlayedTracks" and "limit" not in hydrated:
            hydrated["limit"] = semantic.entities.limit or self.TOP_TRACK_LIMIT

        if tool == "createPlaylist" and "name" not in hydrated:
            playlist_name = None
            if target_playlist is not None:
                playlist_name = target_playlist.raw_query or target_playlist.normalized_query
            elif playlist is not None:
                playlist_name = playlist.raw_query or playlist.normalized_query
            if playlist_name:
                hydrated["name"] = playlist_name

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

        return hydrated
