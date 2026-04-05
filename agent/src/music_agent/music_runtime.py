from __future__ import annotations

import re
import uuid
from time import monotonic

from .autonomous_planner import AutonomousPlanner
from .capability_catalog import capability_architecture_summary, semantic_capability_hints
from .candidate_runtime import CandidateRuntimeMixin
from .chat_agent import ChatAgent
from .client_script_planner import ClientScriptPlanner
from .execution_router import ExecutionRouter
from .observation_runtime import ObservationRuntimeMixin
from .plan_runtime import PlanRuntimeMixin
from .policy_hooks import (
    looks_like_cancellation_request,
)
from .response_style import (
    TEXT_ASK_CREATE_PLAYLIST_NAME,
    TEXT_ASK_PLAYLIST_FOR_TRACKS,
    TEXT_ASK_PLAYLIST_NAME,
    TEXT_ASK_TRACK_NAME,
    TEXT_NO_TRACK_FOUND,
    TEXT_OPERATION_DONE,
    TEXT_PLAYBACK_STOPPED,
    TEXT_PROGRESS_ADD_TRACKS,
    TEXT_PROGRESS_APPROVED,
    TEXT_PROGRESS_FETCH_TOP_TRACKS,
    TEXT_WAITING_FOR_APPROVAL,
    TEXT_WAITING_FOR_TOOL,
    build_playlist_clarification,
    build_playlist_query_clarification,
    build_playlist_track_clarification,
    build_track_clarification,
    summarize_add_tracks_to_playlist,
    summarize_current_track,
    summarize_create_playlist,
    summarize_playlist_tracks,
    summarize_play_playlist,
    summarize_play_track,
    summarize_playlists,
    summarize_recent_tracks,
    summarize_single_playlist,
)
from .runtime_actions import (
    AssistantTextRuntimeAction,
    ChatRuntimeAction,
    ClarificationRuntimeAction,
    FinalResultRuntimeAction,
    PlanApprovalRuntimeAction,
    ProgressRuntimeAction,
    RuntimeActionBatch,
    ScriptCancellationRuntimeAction,
    ScriptDryRunRuntimeAction,
    ScriptExecutionRuntimeAction,
    ScriptValidationRuntimeAction,
    ToolCallRuntimeAction,
)
from .semantic_models import SemanticParseResult, normalize_playlist_query
from .semantic_refiner import (
    extract_chinese_playlist_query,
    extract_chinese_playlist_subset_transfer,
)
from .semantic_parser import SemanticParser
from .semantic_runtime import SemanticRuntimeMixin
from .tool_registry import ToolRegistry
from .workflow_memory import (
    PendingClarification,
    PendingScriptExecution,
    PendingScriptValidation,
    PendingToolCall,
    WorkflowMemoryStore,
    WorkflowState,
)

TOP_TRACK_LIMIT = 20

CHINESE_INDEX_MAP = {
    "一": 1,
    "二": 2,
    "三": 3,
    "四": 4,
    "五": 5,
    "六": 6,
    "七": 7,
    "八": 8,
    "九": 9,
    "十": 10,
}

KEYWORD_CURRENT_TRACK = ("当前播放", "现在播放", "在放什么", "正在播放")
KEYWORD_STOP_PLAYBACK = ("停止播放", "停止", "停一下", "停掉", "停下来", "别放了", "先停一下")
KEYWORD_LIST_PLAYLISTS = ("有哪些歌单", "歌单列表", "查看歌单", "我的歌单")
KEYWORD_CREATE_PLAYLIST = ("创建", "新建")
KEYWORD_TOP_TRACKS = ("最近常听", "常听", "高频", "最常听")
KEYWORD_RECENT_TRACKS = ("最近播放", "最近听", "最近听了什么", "最近播放了什么", "播放历史")
KEYWORD_LOOKUP = ("查询", "查找", "找一个", "寻找", "查看", "看看", "有没有")
KEYWORD_PLAYLIST_CONTENT = ("里面", "内容", "歌曲", "曲目", "歌有哪些", "有什么歌")
WORD_PLAY = "播放"
WORD_PLAYLIST = "歌单"
WORD_REFERENCE = ("这首", "这首歌", "这个歌单", "那个歌单", "这个", "那个", "刚才那个歌单", "该歌单")

class MusicAgentRuntime(SemanticRuntimeMixin, PlanRuntimeMixin, CandidateRuntimeMixin, ObservationRuntimeMixin):
    TOP_TRACK_LIMIT = TOP_TRACK_LIMIT

    def __init__(
        self,
        chat_agent: ChatAgent,
        tool_registry: ToolRegistry,
        semantic_parser: SemanticParser | None = None,
        memory_store: WorkflowMemoryStore | None = None,
        allow_direct_write_actions: bool = True,
    ) -> None:
        self._chat_agent = chat_agent
        self._tool_registry = tool_registry
        self._semantic_parser = semantic_parser
        self._memory_store = memory_store or WorkflowMemoryStore()
        self._allow_direct_write_actions = allow_direct_write_actions
        self._autonomous_planner = AutonomousPlanner(allow_direct_write_actions=allow_direct_write_actions)
        self._client_script_planner = ClientScriptPlanner()
        self._execution_router = ExecutionRouter(
            self._client_script_planner,
            allow_direct_write_actions=allow_direct_write_actions,
        )

    @property
    def memory_store(self) -> WorkflowMemoryStore:
        return self._memory_store

    async def handle_user_message(self, session_id: str, request_id: str | None, content: str):
        cleaned = content.strip()
        state = self._memory_store.get(session_id)

        if state.pending_tool_call is not None:
            return AssistantTextRuntimeAction(TEXT_WAITING_FOR_TOOL, request_id, cleaned)

        if state.pending_script_execution is not None:
            cancellation_action = self._resolve_script_cancellation_action(state, request_id, cleaned)
            if cancellation_action is not None:
                return cancellation_action
            return AssistantTextRuntimeAction(TEXT_WAITING_FOR_TOOL, request_id, cleaned)

        if state.pending_script_dry_run is not None:
            return AssistantTextRuntimeAction(TEXT_WAITING_FOR_TOOL, request_id, cleaned)

        if state.pending_script_validation is not None:
            return AssistantTextRuntimeAction(TEXT_WAITING_FOR_TOOL, request_id, cleaned)

        if state.pending_approval and state.active_plan is not None:
            return AssistantTextRuntimeAction(TEXT_WAITING_FOR_APPROVAL, request_id, cleaned)

        if state.pending_clarification is not None:
            return self._handle_clarification(session_id, state, request_id, cleaned)

        semantic = await self._parse_semantics(cleaned, state)
        self._hydrate_semantic_action_candidates(semantic, state)
        intent = self._semantic_to_intent_payload(semantic)
        self._apply_semantic_result(state, semantic, intent)
        state.goal_status = "resolving"

        reference_action = self._resolve_semantic_reference_action(session_id, state, request_id, cleaned, semantic, intent)
        if reference_action is not None:
            return reference_action

        script_action = self._resolve_client_script_action(session_id, state, request_id, cleaned, semantic)
        if script_action is not None:
            return script_action

        llm_candidate_action = self._resolve_llm_action_candidates_action(session_id, state, request_id, cleaned, semantic)
        if llm_candidate_action is not None:
            return llm_candidate_action

        llm_tool_action = self._resolve_llm_proposed_tool_action(session_id, state, request_id, cleaned, semantic)
        if llm_tool_action is not None:
            return llm_tool_action

        legacy_reference_action = self._resolve_reference_play_request(session_id, state, request_id, cleaned)
        if legacy_reference_action is not None:
            return legacy_reference_action

        legacy_playlist_reference_action = self._resolve_reference_playlist_request(session_id, state, request_id, cleaned)
        if legacy_playlist_reference_action is not None:
            return legacy_playlist_reference_action

        missing_field_action = self._handle_missing_fields(state, request_id, cleaned, intent)
        if missing_field_action is not None:
            return missing_field_action

        state.workflow_mode = "tool" if intent["intent"] != "chat" else "chat"

        if intent["intent"] == "chat":
            return ChatRuntimeAction()

        if intent["intent"] == "create_playlist":
            playlist_name = intent["entities"].get("playlistName", "").strip()
            if not playlist_name:
                return AssistantTextRuntimeAction(TEXT_ASK_CREATE_PLAYLIST_NAME, request_id, cleaned)
            return self._build_create_playlist_plan(session_id, request_id, cleaned, playlist_name, state)

        if intent["intent"] == "create_playlist_with_top_tracks":
            playlist_name = intent["entities"].get("playlistName", "").strip()
            if not playlist_name:
                return AssistantTextRuntimeAction(TEXT_ASK_CREATE_PLAYLIST_NAME, request_id, cleaned)
            return self._build_create_playlist_with_top_tracks_plan(session_id, request_id, cleaned, playlist_name, state)

        if intent["intent"] == "create_playlist_from_playlist_subset":
            if not self._allow_direct_write_actions:
                return AssistantTextRuntimeAction("当前配置未开启写操作直接执行，这个请求需要改走审批流后再执行。", request_id, cleaned)
            target_playlist_name = str(intent["entities"].get("targetPlaylistName", "") or "").strip()
            source_playlist_name = str(intent["entities"].get("sourcePlaylistName", "") or "").strip()
            if not target_playlist_name:
                return AssistantTextRuntimeAction(TEXT_ASK_CREATE_PLAYLIST_NAME, request_id, cleaned)
            if not source_playlist_name:
                return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_NAME, request_id, cleaned)
            state.goal_status = "running"
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                cleaned,
                "createPlaylist",
                {"name": target_playlist_name},
            )

        if intent["intent"] == "get_current_track":
            return self._issue_tool_call(session_id, state, request_id, cleaned, "getCurrentTrack", {})

        if intent["intent"] == "stop_playback":
            return self._issue_tool_call(session_id, state, request_id, cleaned, "stopPlayback", {})

        if intent["intent"] == "get_playlists":
            return self._issue_tool_call(session_id, state, request_id, cleaned, "getPlaylists", {})

        if intent["intent"] == "query_playlist":
            playlist_name = intent["entities"].get("playlistName", "").strip()
            if not playlist_name and not state.recent_playlist_candidates:
                state.workflow_mode = "chat"
                state.goal_status = "waiting_user"
                return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_NAME, request_id, cleaned)
            return self._issue_tool_call(session_id, state, request_id, cleaned, "getPlaylists", {})

        if intent["intent"] == "inspect_playlist_tracks":
            playlist_name = intent["entities"].get("playlistName", "").strip()
            if playlist_name:
                return self._issue_tool_call(session_id, state, request_id, cleaned, "getPlaylists", {})
            if state.last_named_playlist is not None:
                return self._issue_tool_call(
                    session_id,
                    state,
                    request_id,
                    cleaned,
                    "getPlaylistTracks",
                    {"playlistId": state.last_named_playlist["playlistId"]},
                )
            state.workflow_mode = "chat"
            state.goal_status = "waiting_user"
            return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_FOR_TRACKS, request_id, cleaned)

        if intent["intent"] == "get_recent_tracks":
            return self._issue_tool_call(session_id, state, request_id, cleaned, "getRecentTracks", {"limit": 10})

        if intent["intent"] == "play_track":
            title = intent["entities"].get("title", "").strip()
            if not title:
                state.workflow_mode = "chat"
                state.goal_status = "waiting_user"
                return AssistantTextRuntimeAction(TEXT_ASK_TRACK_NAME, request_id, cleaned)
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                cleaned,
                "searchTracks",
                {
                    "keyword": title,
                    "artist": intent["entities"].get("artist", "").strip(),
                    "limit": 5,
                },
            )

        if intent["intent"] == "play_playlist":
            playlist_name = intent["entities"].get("playlistName", "").strip()
            if not playlist_name and not state.recent_playlist_candidates:
                state.workflow_mode = "chat"
                state.goal_status = "waiting_user"
                return AssistantTextRuntimeAction(TEXT_ASK_PLAYLIST_NAME, request_id, cleaned)
            return self._issue_tool_call(session_id, state, request_id, cleaned, "getPlaylists", {})

        return ChatRuntimeAction()

    async def handle_tool_result(self, session_id: str, tool_result: dict):
        state = self._memory_store.get(session_id)
        pending = state.pending_tool_call
        if pending is None:
            raise ValueError("no pending tool call")
        if tool_result["toolCallId"] != pending.tool_call_id:
            raise ValueError("unknown toolCallId")

        self._integrate_tool_result(state, pending.tool, tool_result)

        if not tool_result["ok"]:
            return self._handle_failed_tool_result(session_id, state, pending, tool_result.get("error") or {})

        result = tool_result.get("result") or {}
        plan_action = self._handle_plan_tool_result(session_id, state, pending, result)
        if plan_action is not None:
            return plan_action

        candidate_action = self._continue_action_candidates_after_observation(session_id, state, pending, result)
        if candidate_action is not None:
            return candidate_action

        return self._decide_next_action_after_observation(session_id, state, pending, result)

    @staticmethod
    def _integrate_tool_result(state: WorkflowState, tool_name: str, tool_result: dict) -> None:
        state.pending_tool_call = None
        state.last_tool_result = tool_result
        state.last_tool_observation = tool_result.get("result") or tool_result.get("error") or {}
        result = tool_result.get("result") or {}
        if tool_name == "getCurrentTrack":
            state.current_playback = result
        elif tool_name in {"playTrack", "playPlaylist"}:
            state.current_playback = result
            if result.get("playlist"):
                state.last_named_playlist = result.get("playlist")
        elif tool_name == "stopPlayback":
            state.current_playback = result or {"playing": False}
        elif tool_name == "getPlaylists":
            state.recent_playlist_candidates = list(result.get("items", []))
        elif tool_name == "getPlaylistTracks":
            state.recent_playlist_tracks = list(result.get("items", []))
            if result.get("playlist"):
                state.last_named_playlist = result.get("playlist")
        elif tool_name == "getRecentTracks":
            state.recent_recent_tracks = list(result.get("items", []))

    async def handle_approval_response(self, session_id: str, approved: bool):
        state = self._memory_store.get(session_id)
        plan = state.active_plan
        if plan is None or not state.pending_approval:
            raise ValueError("no pending approval")

        state.pending_approval = False
        if not approved:
            state.workflow_mode = "chat"
            state.active_plan = None
            state.goal_status = "cancelled"
            plan.status = "cancelled"
            return FinalResultRuntimeAction(
                plan_id=plan.plan_id,
                session_id=session_id,
                ok=False,
                summary=f"已取消计划：{plan.summary}",
                request_id=plan.source_request_id,
                persist_user_message=plan.source_user_message,
            )

        plan.status = "approved"
        first_step = plan.steps[0]
        first_step.status = "waiting_tool"
        state.workflow_mode = "tool"
        state.goal_status = "running"
        return RuntimeActionBatch(
            actions=[
                ProgressRuntimeAction(plan.plan_id, first_step.step_id, TEXT_PROGRESS_APPROVED),
                self._issue_tool_call(
                    session_id,
                    state,
                    plan.source_request_id,
                    plan.source_user_message,
                    first_step.tool,
                    first_step.args,
                ),
            ]
        )

    def persist_turn(self, session_id: str, user_message: str, assistant_message: str) -> None:
        self._chat_agent.persist_turn(session_id, user_message, assistant_message)

    def pending_tool_timed_out(self, session_id: str, timeout_seconds: float) -> PendingToolCall | None:
        state = self._memory_store.get(session_id)
        pending = state.pending_tool_call
        if pending is None:
            return None
        if monotonic() - pending.started_at >= timeout_seconds:
            state.pending_tool_call = None
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            return pending
        return None

    def pending_script_timed_out(self, session_id: str, timeout_seconds: float):
        state = self._memory_store.get(session_id)
        pending = state.pending_script_execution or state.pending_script_validation or state.pending_script_dry_run
        if pending is None:
            return None
        effective_timeout = timeout_seconds
        if state.pending_script_execution is not None:
            effective_timeout = self._script_wait_timeout_seconds(state.pending_script_execution.script, timeout_seconds)
        elif state.pending_script_validation is not None:
            effective_timeout = self._script_wait_timeout_seconds(state.pending_script_validation.script, timeout_seconds)
        elif state.pending_script_dry_run is not None:
            effective_timeout = self._script_wait_timeout_seconds(state.pending_script_dry_run.script, timeout_seconds)
        if monotonic() - pending.started_at >= effective_timeout:
            state.pending_script_execution = None
            state.pending_script_validation = None
            state.pending_script_dry_run = None
            state.workflow_mode = "chat"
            state.goal_status = "failed"
            return pending
        return None

    @staticmethod
    def _script_wait_timeout_seconds(script: dict, default_timeout_seconds: float) -> float:
        timeout_ms = script.get("timeoutMs")
        if not isinstance(timeout_ms, int):
            return default_timeout_seconds
        timeout_ms = max(1, min(timeout_ms, 300_000))
        # 缁欏鎴风鐨勮秴鏃舵敹鍙ｅ拰鏈€缁堢粨鏋滃洖浼犵暀涓€鐐逛綑閲忥紝閬垮厤鏈嶅姟绔厛璇垽瓒呮椂銆?        return max(default_timeout_seconds, timeout_ms / 1000.0 + 5.0)



    def _resolve_script_cancellation_action(
        self,
        state: WorkflowState,
        request_id: str | None,
        user_message: str,
    ):
        pending = state.pending_script_execution
        if pending is None:
            return None
        if not looks_like_cancellation_request(user_message):
            return None
        if not pending.execution_id:
            return AssistantTextRuntimeAction("当前脚本正在启动中，请稍后再取消。", request_id, user_message)
        if pending.cancellation_request_id is not None:
            return AssistantTextRuntimeAction("当前脚本正在取消中，请稍候。", request_id, user_message)
        pending.cancellation_request_id = request_id
        pending.cancellation_user_message = user_message
        pending.cancellation_reason = "用户主动取消"
        return ScriptCancellationRuntimeAction(
            execution_id=pending.execution_id,
            request_id=request_id,
            reason=pending.cancellation_reason,
            persist_user_message=user_message,
        )


    def _issue_tool_call(
        self,
        session_id: str,
        state: WorkflowState,
        request_id: str | None,
        user_message: str,
        tool: str,
        args: dict,
    ) -> ToolCallRuntimeAction:
        definition = self._tool_registry.get(tool)
        if definition is None:
            raise ValueError(f"unsupported tool: {tool}")

        sanitized_args = self._sanitize_tool_args(definition, args)
        missing = [name for name in definition.required_args if name not in sanitized_args]
        if missing:
            raise ValueError(f"missing required args for {tool}: {', '.join(missing)}")

        tool_call_id = f"tool-{uuid.uuid4().hex}"
        state.tool_stack.append(tool)
        state.pending_tool_call = PendingToolCall(tool_call_id, tool, sanitized_args, request_id, user_message)
        state.workflow_mode = "tool"
        state.goal_status = "waiting_tool"
        return ToolCallRuntimeAction(tool_call_id, tool, sanitized_args, request_id)

    def _handle_clarification(
        self,
        session_id: str,
        state: WorkflowState,
        request_id: str | None,
        content: str,
    ):
        clarification = state.pending_clarification
        if clarification is None:
            return ChatRuntimeAction()

        selected = self._select_candidate(clarification.items, content)
        if selected is None:
            return ClarificationRuntimeAction(clarification.question, clarification.options, request_id, content)

        state.pending_clarification = None
        if clarification.kind == "track_selection":
            state.recent_track_candidates = clarification.items
            state.last_named_track = selected
            state.goal_status = "running"
            return self._issue_tool_call(session_id, state, request_id, content, "playTrack", {"trackId": selected["trackId"]})
        state.recent_playlist_candidates = clarification.items
        state.last_named_playlist = selected
        if clarification.resolution_action == "play_playlist":
            state.goal_status = "running"
            return self._issue_tool_call(session_id, state, request_id, content, "playPlaylist", {"playlistId": selected["playlistId"]})
        if clarification.resolution_action == "inspect_playlist_tracks":
            state.goal_status = "running"
            return self._issue_tool_call(session_id, state, request_id, content, "getPlaylistTracks", {"playlistId": selected["playlistId"]})
        if clarification.resolution_action == "query_playlist":
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(self._summarize_single_playlist(selected), request_id, content)
        return AssistantTextRuntimeAction(TEXT_OPERATION_DONE, request_id, content)

    def _resolve_reference_playlist_request(
        self,
        session_id: str,
        state: WorkflowState,
        request_id: str | None,
        content: str,
    ):
        normalized = content.strip()
        if not any(keyword in normalized for keyword in WORD_REFERENCE):
            return None
        if WORD_PLAYLIST not in normalized and state.last_named_playlist is None:
            return None

        if self._looks_like_playlist_track_request(normalized) and state.last_named_playlist is not None:
            state.intent = "inspect_playlist_tracks"
            state.current_goal = "inspect_playlist_tracks"
            state.goal_status = "running"
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                normalized,
                "getPlaylistTracks",
                {"playlistId": state.last_named_playlist["playlistId"]},
            )

        if self._looks_like_playlist_query_request(normalized) and state.last_named_playlist is not None:
            state.intent = "query_playlist"
            state.current_goal = "query_playlist"
            state.workflow_mode = "chat"
            state.goal_status = "completed"
            return AssistantTextRuntimeAction(self._summarize_single_playlist(state.last_named_playlist), request_id, normalized)

        return None

    def _resolve_reference_play_request(
        self,
        session_id: str,
        state: WorkflowState,
        request_id: str | None,
        content: str,
    ):
        normalized = content.strip()
        if WORD_PLAY not in normalized:
            return None

        has_reference = any(keyword in normalized for keyword in WORD_REFERENCE)
        has_ordinal = re.search(r"绗琝s*(?:[1-9]\d*|[涓€浜屼笁鍥涗簲鍏竷鍏節鍗乚)\s*(?:涓獆棣??", normalized) is not None
        has_bare_number = re.fullmatch(r"\s*鎾斁\s*([1-9]\d*)\s*", normalized) is not None
        if not (has_reference or has_ordinal or has_bare_number):
            return None

        if WORD_PLAYLIST in normalized:
            playlist_reference = self._select_candidate(state.recent_playlist_candidates, normalized)
            if playlist_reference is not None:
                return self._issue_tool_call(
                    session_id,
                    state,
                    request_id,
                    normalized,
                    "playPlaylist",
                    {"playlistId": playlist_reference["playlistId"]},
                )

        track_reference = self._select_candidate(state.recent_track_candidates, normalized)
        if track_reference is not None:
            return self._issue_tool_call(
                session_id,
                state,
                request_id,
                normalized,
                "playTrack",
                {"trackId": track_reference["trackId"]},
            )

        if WORD_PLAYLIST in normalized:
            playlist_reference = self._select_candidate(state.recent_playlist_candidates, normalized)
            if playlist_reference is not None:
                return self._issue_tool_call(
                    session_id,
                    state,
                    request_id,
                    normalized,
                    "playPlaylist",
                    {"playlistId": playlist_reference["playlistId"]},
                )
        return None

    @staticmethod
    def _parse_intent(content: str) -> dict:
        normalized = content.strip()
        chinese_subset_transfer = extract_chinese_playlist_subset_transfer(normalized)
        if any(keyword in normalized for keyword in ("当前播放", "现在播放", "在放什么", "正在播放")):
            return {"intent": "get_current_track", "entities": {}, "ambiguities": []}
        if any(keyword in normalized for keyword in ("停止播放", "停止", "停一下", "停掉", "停下来", "别放了", "先停一下")):
            return {"intent": "stop_playback", "entities": {}, "ambiguities": []}
        if any(keyword in normalized for keyword in ("最近播放", "最近听", "最近播放列表", "播放历史", "最近听了什么", "最近播放了什么")):
            return {"intent": "get_recent_tracks", "entities": {}, "ambiguities": []}
        if chinese_subset_transfer is not None:
            return {
                "intent": "create_playlist_from_playlist_subset",
                "entities": {
                    "targetPlaylistName": chinese_subset_transfer["target_raw"],
                    "sourcePlaylistName": chinese_subset_transfer["source_raw"],
                    "trackSelectionMode": chinese_subset_transfer["selection_mode"],
                    "trackSelectionCount": chinese_subset_transfer["selection_count"],
                },
                "ambiguities": [],
            }
        if "歌单" in normalized and any(keyword in normalized for keyword in ("列出", "查看", "看看")) and any(
            keyword in normalized for keyword in ("所有音乐", "全部音乐", "所有歌曲", "全部歌曲", "内容", "里有什么歌", "里面有什么歌")
        ):
            return {
                "intent": "inspect_playlist_tracks",
                "entities": {"playlistName": extract_chinese_playlist_query(normalized) or normalized},
                "ambiguities": [],
            }
        if "歌单" in normalized and any(keyword in normalized for keyword in ("查询", "查找", "找一个", "寻找", "查看", "看看", "有没有")):
            return {
                "intent": "query_playlist",
                "entities": {"playlistName": extract_chinese_playlist_query(normalized) or normalized},
                "ambiguities": [],
            }
        if any(keyword in normalized for keyword in KEYWORD_CURRENT_TRACK):
            return {"intent": "get_current_track", "entities": {}, "ambiguities": []}
        if any(keyword in normalized for keyword in KEYWORD_STOP_PLAYBACK):
            return {"intent": "stop_playback", "entities": {}, "ambiguities": []}
        if any(keyword in normalized for keyword in KEYWORD_RECENT_TRACKS):
            return {"intent": "get_recent_tracks", "entities": {}, "ambiguities": []}
        subset_transfer = MusicAgentRuntime._extract_playlist_subset_transfer(normalized)
        if subset_transfer is not None:
            return {
                "intent": "create_playlist_from_playlist_subset",
                "entities": subset_transfer,
                "ambiguities": [],
            }
        if (
            any(keyword in normalized for keyword in KEYWORD_CREATE_PLAYLIST)
            and WORD_PLAYLIST in normalized
            and any(keyword in normalized for keyword in KEYWORD_TOP_TRACKS)
        ):
            return {
                "intent": "create_playlist_with_top_tracks",
                "entities": {"playlistName": MusicAgentRuntime._extract_playlist_name(normalized)},
                "ambiguities": [],
            }
        if any(keyword in normalized for keyword in KEYWORD_CREATE_PLAYLIST) and WORD_PLAYLIST in normalized:
            return {
                "intent": "create_playlist",
                "entities": {"playlistName": MusicAgentRuntime._extract_playlist_name(normalized)},
                "ambiguities": [],
            }
        if (
            WORD_PLAYLIST in normalized
            and any(keyword in normalized for keyword in KEYWORD_PLAYLIST_CONTENT)
            and WORD_PLAY not in normalized
        ):
            return {
                "intent": "inspect_playlist_tracks",
                "entities": {"playlistName": MusicAgentRuntime._extract_playlist_name(normalized)},
                "ambiguities": [],
            }
        if (
            WORD_PLAYLIST in normalized
            and any(keyword in normalized for keyword in KEYWORD_LOOKUP)
            and WORD_PLAY not in normalized
        ):
            return {
                "intent": "query_playlist",
                "entities": {"playlistName": MusicAgentRuntime._extract_playlist_name(normalized)},
                "ambiguities": [],
            }
        if any(keyword in normalized for keyword in KEYWORD_LIST_PLAYLISTS):
            return {"intent": "get_playlists", "entities": {}, "ambiguities": []}
        if WORD_PLAY in normalized and WORD_PLAYLIST in normalized:
            query = normalized.replace(WORD_PLAY, "").replace(WORD_PLAYLIST, "").strip(" ,，。！？")
            query = query.replace("这个", "").replace("那个", "").strip()
            return {"intent": "play_playlist", "entities": {"playlistName": query}, "ambiguities": []}
        if WORD_PLAY in normalized:
            match = re.search(r"播放\s*(?:(?P<artist>[^\s的]+)的)?(?P<title>.+)", normalized)
            artist = ""
            title = normalized.replace(WORD_PLAY, "").strip()
            if match:
                artist = (match.group("artist") or "").strip()
                title = (match.group("title") or "").strip()
            title = title.replace("这首歌", "").replace("这首", "").strip() or normalized.replace(WORD_PLAY, "").strip()
            return {"intent": "play_track", "entities": {"artist": artist, "title": title, "keyword": title}, "ambiguities": []}
        return {"intent": "chat", "entities": {}, "ambiguities": []}

    @staticmethod
    def _extract_playlist_name(normalized: str) -> str:
        extracted = extract_chinese_playlist_query(normalized)
        if extracted:
            return extracted
        playlist_name = normalized
        for token in (
            "创建",
            "新建",
            "一个",
            "帮我",
            "我的",
            "查询",
            "查找",
            "找一个",
            "寻找",
            "查看",
            "看看",
            "有没有",
            "里面有什么歌",
            "里有什么歌",
            "里面的歌曲",
            "里面的歌",
            "歌曲列表",
            "内容",
            "并添加最近常听歌曲",
            "添加最近常听歌曲",
            "加上最近常听歌曲",
            "加到最近常听歌曲",
        ):
            playlist_name = playlist_name.replace(token, "")
        playlist_name = playlist_name.replace("这个", "").replace("那个", "").replace("刚才那个", "")
        return playlist_name.strip("“”\"' ,，。！？")

    @staticmethod
    def _extract_playlist_subset_transfer(normalized: str) -> dict | None:
        extracted = extract_chinese_playlist_subset_transfer(normalized)
        if extracted is not None:
            return {
                "targetPlaylistName": extracted["target_raw"],
                "sourcePlaylistName": extracted["source_raw"],
                "trackSelectionMode": extracted["selection_mode"],
                "trackSelectionCount": extracted["selection_count"],
            }
        if "歌单" not in normalized:
            return None
        if not any(keyword in normalized for keyword in KEYWORD_CREATE_PLAYLIST):
            return None
        if not any(keyword in normalized for keyword in ("放到", "加入", "加到", "添加到", "放进")):
            return None

        target_match = re.search(r"(?:叫做|叫|歌单名为)\s*([^，。！？]+?)(?:歌单|，|。|$)", normalized)
        source_match = re.search(r"把\s*([^，。！？]*?歌单)", normalized)
        count_match = re.search(r"(前|后)(\d+|[一二三四五六七八九十两]+)首", normalized)
        if "最后一首" in normalized or "倒数第一首" in normalized:
            count_match = None
            selection_mode = "last_n"
            count = 1
        else:
            selection_mode = None
            count = 0
        if not target_match or not source_match or (count_match is None and selection_mode is None):
            return None

        target_name = target_match.group(1).strip("“”\"' ,，。！？")
        source_name = source_match.group(1).strip("“”\"' ,，。！？")
        if count_match is not None:
            direction = count_match.group(1)
            raw_count = count_match.group(2)
            count = int(raw_count) if raw_count.isdigit() else CHINESE_INDEX_MAP.get(raw_count, 0)
            selection_mode = "first_n" if direction == "前" else "last_n"
        if not target_name or not source_name or count <= 0:
            return None
        return {
            "targetPlaylistName": target_name,
            "sourcePlaylistName": normalize_playlist_query(source_name),
            "trackSelectionMode": selection_mode,
            "trackSelectionCount": count,
        }

    @staticmethod
    def _sanitize_tool_args(definition, args: dict) -> dict:
        allowed_names = set(definition.required_args) | set(definition.optional_args)
        sanitized: dict[str, object] = {}
        for key, value in args.items():
            if key not in allowed_names:
                continue
            if value is None:
                continue
            if isinstance(value, str) and not value.strip():
                continue
            sanitized[key] = value.strip() if isinstance(value, str) else value
        return sanitized

    @staticmethod
    def _build_track_clarification(items: list[dict]) -> tuple[str, list[str]]:
        return build_track_clarification(items)

    @staticmethod
    def _build_playlist_clarification(items: list[dict]) -> tuple[str, list[str]]:
        return build_playlist_clarification(items)

    @staticmethod
    def _build_playlist_track_clarification(items: list[dict]) -> tuple[str, list[str]]:
        return build_playlist_track_clarification(items)

    @staticmethod
    def _build_playlist_query_clarification(items: list[dict]) -> tuple[str, list[str]]:
        return build_playlist_query_clarification(items)

    @staticmethod
    def _select_candidate(items: list[dict], content: str) -> dict | None:
        if not items:
            return None

        index_value = MusicAgentRuntime._extract_index(content)
        if index_value is not None:
            index = index_value - 1
            if 0 <= index < len(items):
                return items[index]

        if any(keyword in content for keyword in WORD_REFERENCE):
            return items[0]

        for item in items:
            title = item.get("title")
            name = item.get("name")
            if title and title in content:
                return item
            if name and name in content:
                return item
        return None

    @staticmethod
    def _extract_index(content: str) -> int | None:
        match = re.search(r"第\s*([1-9]\d*)\s*(?:个|首)?", content)
        if match:
            return int(match.group(1))

        bare_number = re.fullmatch(r"\s*([1-9]\d*)\s*", content)
        if bare_number:
            return int(bare_number.group(1))

        chinese_match = re.search(r"第\s*([一二三四五六七八九十两])\s*(?:个|首)?", content)
        if chinese_match:
            return CHINESE_INDEX_MAP.get(chinese_match.group(1))

        return None

    @staticmethod
    def _summarize_current_track(result: dict) -> str:
        return summarize_current_track(result)

    @staticmethod
    def _summarize_playlists(items: list[dict]) -> str:
        return summarize_playlists(items)

    @staticmethod
    def _summarize_single_playlist(playlist: dict) -> str:
        return summarize_single_playlist(playlist)

    @staticmethod
    def _summarize_playlist_tracks(playlist: dict, items: list[dict]) -> str:
        return summarize_playlist_tracks(playlist, items)

    @staticmethod
    def _summarize_recent_tracks(items: list[dict]) -> str:
        return summarize_recent_tracks(items)

    @staticmethod
    def _summarize_play_track(track: dict) -> str:
        return summarize_play_track(track)

    @staticmethod
    def _summarize_play_playlist(playlist: dict) -> str:
        return summarize_play_playlist(playlist)

    @staticmethod
    def _summarize_create_playlist(playlist: dict) -> str:
        return summarize_create_playlist(playlist)

    @staticmethod
    def _summarize_add_tracks_to_playlist(playlist: dict, added_count: int) -> str:
        return summarize_add_tracks_to_playlist(playlist, added_count)

    @staticmethod
    def _playlist_queries_from_state(state: WorkflowState) -> tuple[str, str]:
        raw_query = str(state.entities.get("playlistName", "") or "").strip()
        normalized_query = str(state.entities.get("playlistNormalized", "") or "").strip()
        if not normalized_query and raw_query:
            normalized_query = normalize_playlist_query(raw_query) or ""
        return raw_query, normalized_query

    @staticmethod
    def _playlist_queries_from_candidate_or_state(state: WorkflowState, candidate: dict) -> tuple[str, str]:
        args = dict(candidate.get("args") or {})
        playlist_role = str(args.get("playlistRole") or "")
        raw_query = str(args.get("rawQuery") or "").strip()
        normalized_query = str(args.get("normalizedQuery") or "").strip()
        if raw_query or normalized_query:
            if not normalized_query and raw_query:
                normalized_query = normalize_playlist_query(raw_query) or ""
            return raw_query, normalized_query

        if playlist_role == "target":
            raw_query = str(state.entities.get("targetPlaylistName", "") or "").strip()
            normalized_query = str(state.entities.get("targetPlaylistNormalized", "") or "").strip()
        elif playlist_role == "source":
            raw_query = str(state.entities.get("sourcePlaylistName", "") or "").strip()
            normalized_query = str(state.entities.get("sourcePlaylistNormalized", "") or "").strip()
            if not raw_query and not normalized_query:
                raw_query = str(state.entities.get("playlistName", "") or "").strip()
                normalized_query = str(state.entities.get("playlistNormalized", "") or "").strip()
        else:
            raw_query = str(state.entities.get("playlistName", "") or "").strip()
            normalized_query = str(state.entities.get("playlistNormalized", "") or "").strip()
        if not normalized_query and raw_query:
            normalized_query = normalize_playlist_query(raw_query) or ""
        return raw_query, normalized_query

    @staticmethod
    def _match_playlists(items: list[dict], raw_query: str, normalized_query: str) -> list[dict]:
        cleaned_raw = raw_query.strip()
        cleaned_normalized = normalized_query.strip()
        if not cleaned_raw and not cleaned_normalized:
            return list(items)

        exact_raw = [item for item in items if item.get("name", "").strip() == cleaned_raw]
        if exact_raw:
            return exact_raw

        if cleaned_normalized:
            exact_normalized = [
                item
                for item in items
                if (normalize_playlist_query(item.get("name", "")) or "") == cleaned_normalized
            ]
            if exact_normalized:
                return exact_normalized

        contains_raw = [
            item
            for item in items
            if cleaned_raw and (cleaned_raw in item.get("name", "") or item.get("name", "") in cleaned_raw)
        ]
        if contains_raw:
            return contains_raw

        if cleaned_normalized:
            contains_normalized = [
                item
                for item in items
                if cleaned_normalized
                and (
                    cleaned_normalized in (normalize_playlist_query(item.get("name", "")) or "")
                    or (normalize_playlist_query(item.get("name", "")) or "") in cleaned_normalized
                )
            ]
            if contains_normalized:
                return contains_normalized

        return []

    @staticmethod
    def _looks_like_playlist_track_request(content: str) -> bool:
        return any(keyword in content for keyword in KEYWORD_PLAYLIST_CONTENT)

    @staticmethod
    def _looks_like_playlist_query_request(content: str) -> bool:
        return any(keyword in content for keyword in KEYWORD_LOOKUP)


