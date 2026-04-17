from __future__ import annotations

"""
模块名: control_runtime
功能概述: 实现本地控制型 Agent 主运行时，负责编排快路径、本地模型编译、确定性模板执行与显式远程兜底。
对外接口: ControlRuntime
依赖关系: session_store、workflow_memory、runtime_actions、control_gateways、control_models、plan_models
输入输出: 输入为聊天消息、宿主快照、工具结果、审批结果；输出为 transport 层可分发的运行时动作
异常与错误: 参数冲突与非法状态抛出 ValueError，其余异常由上层转成 runtime_error
维护说明: 该运行时默认不接入自由规划与脚本生成，新增能力时优先补 deterministic template
"""

import re
import uuid
from time import monotonic
from typing import Any

from pydantic import ValidationError

from .config import Settings
from .control_gateways import LocalModelGateway, LocalModelGatewayError, RemoteAssistantGateway
from .control_models import CapabilitySnapshot, ControlIntent, HostContextSnapshot, RiskPolicyDecision
from .plan_models import ExecutionPlan, PlanStep
from .runtime_actions import (
    AssistantTextRuntimeAction,
    FinalResultRuntimeAction,
    PlanApprovalRuntimeAction,
    ToolCallRuntimeAction,
)
from .session_store import SessionStore
from .workflow_memory import PendingToolCall, WorkflowMemoryStore, WorkflowState

FAST_PATH_PATTERNS = {
    "pause_playback": ("暂停", "暂停播放", "停一下"),
    "resume_playback": ("继续播放", "恢复播放", "继续", "接着放"),
    "stop_playback": ("停止播放", "停止", "停掉", "别放了"),
    "play_next": ("下一首", "下首", "切到下一首"),
    "play_previous": ("上一首", "上首", "切到上一首"),
}

PLAY_MODE_PATTERNS = {
    "repeat_one": ("单曲循环",),
    "repeat_all": ("列表循环", "顺序循环"),
    "shuffle": ("随机播放", "随机模式", "打乱播放"),
    "sequential": ("顺序播放", "顺序模式"),
}

REMOTE_PREFIXES = ("/assistant", "/remote")


class ControlRuntime:
    def __init__(
        self,
        settings: Settings,
        session_store: SessionStore,
        memory_store: WorkflowMemoryStore | None = None,
        local_model_gateway: LocalModelGateway | None = None,
        remote_assistant_gateway: RemoteAssistantGateway | None = None,
    ) -> None:
        self._settings = settings
        self._session_store = session_store
        self._memory_store = memory_store or WorkflowMemoryStore()
        self._local_model_gateway = local_model_gateway or LocalModelGateway(settings)
        self._remote_assistant_gateway = remote_assistant_gateway

    @property
    def memory_store(self) -> WorkflowMemoryStore:
        return self._memory_store

    def persist_turn(self, session_id: str, user_message: str, assistant_message: str) -> None:
        self._session_store.append_turn(session_id, user_message, assistant_message)

    async def handle_host_snapshot(self, session_id: str, payload: dict[str, Any]) -> None:
        state = self._memory_store.get(session_id)
        host_context = HostContextSnapshot.model_validate(payload.get("hostContext") or {})
        capabilities = CapabilitySnapshot.model_validate(
            {
                "catalogVersion": payload.get("catalogVersion") or "",
                "tools": payload.get("capabilities") or [],
            }
        )
        state.host_context_snapshot = host_context.model_dump(by_alias=True)
        state.capability_snapshot = capabilities.model_dump(by_alias=True)

    def pending_tool_timed_out(self, session_id: str, timeout_seconds: float) -> PendingToolCall | None:
        state = self._memory_store.get(session_id)
        pending = state.pending_tool_call
        if pending is None:
            return None
        if monotonic() - pending.started_at < timeout_seconds:
            return None
        state.pending_tool_call = None
        state.goal_status = "failed"
        if state.active_plan is not None:
            state.active_plan.status = "failed"
            for step in state.active_plan.steps:
                if step.status == "waiting_tool":
                    step.status = "failed"
        return pending

    def pending_script_timed_out(self, session_id: str, timeout_seconds: float):  # noqa: ARG002
        return None

    async def handle_user_message(self, session_id: str, request_id: str | None, content: str):
        cleaned = content.strip()
        state = self._memory_store.get(session_id)
        state.agent_mode = self._settings.agent_default_mode.strip().lower() or "control"

        if state.pending_tool_call is not None:
            return AssistantTextRuntimeAction("当前还有一个控制动作正在执行，请稍等结果返回。", request_id, cleaned)

        remote_payload = self._extract_remote_prompt(cleaned)
        if remote_payload is not None:
            return await self._handle_remote_request(session_id, request_id, cleaned, remote_payload)

        fast_intent = self._match_fast_path(cleaned)
        if fast_intent is not None:
            plan = self._plan_from_intent(session_id, request_id, cleaned, fast_intent)
            approval_block = self._guard_pending_approval_for_new_plan(state, plan, request_id, cleaned)
            if approval_block is not None:
                return approval_block
            return self._dispatch_plan(session_id, state, plan)

        try:
            compiled_intent = await self._local_model_gateway.compile_intent(
                user_message=cleaned,
                host_context=state.host_context_snapshot,
                capability_snapshot=state.capability_snapshot,
                memory_snapshot=self._memory_snapshot(state),
            )
        except LocalModelGatewayError as exc:
            return AssistantTextRuntimeAction(
                self._local_model_error_reply(exc),
                request_id,
                cleaned,
            )
        except Exception:
            return AssistantTextRuntimeAction(
                "本地控制模型当前不可用，请检查 `LOCAL_MODEL_BASE_URL`、模型服务和 `Qwen2.5-3B-Instruct` 是否已启动。",
                request_id,
                cleaned,
            )

        if compiled_intent.route == "restricted" or compiled_intent.intent == "restricted":
            return AssistantTextRuntimeAction(self._restricted_reply(compiled_intent.reason_code), request_id, cleaned)

        plan = self._plan_from_intent(session_id, request_id, cleaned, compiled_intent)
        if plan is None:
            return AssistantTextRuntimeAction(
                "这条话术目前还不在控制白名单里，请换成更明确的软件控制指令。",
                request_id,
                cleaned,
            )
        approval_block = self._guard_pending_approval_for_new_plan(state, plan, request_id, cleaned)
        if approval_block is not None:
            return approval_block
        return self._dispatch_plan(session_id, state, plan)

    async def handle_tool_result(self, session_id: str, tool_result: dict[str, Any]):
        state = self._memory_store.get(session_id)
        pending = state.pending_tool_call
        if pending is None:
            raise ValueError("no pending tool call")
        if tool_result.get("toolCallId") != pending.tool_call_id:
            raise ValueError("tool result does not match the pending tool call")

        plan = state.active_plan
        state.pending_tool_call = None
        state.last_tool_result = tool_result
        self._integrate_tool_result(state, pending.tool, tool_result)

        if plan is None:
            return AssistantTextRuntimeAction("工具执行已完成。", pending.request_id, pending.user_message)

        current_step_index = int(plan.context.get("stepIndex", 0))
        step = plan.steps[current_step_index]
        if not tool_result.get("ok", False):
            step.status = "failed"
            plan.status = "failed"
            summary = self._summarize_tool_failure(pending.tool, tool_result.get("error") or {})
            self._clear_execution_state(state)
            return FinalResultRuntimeAction(
                plan_id=plan.plan_id,
                session_id=session_id,
                ok=False,
                summary=summary,
                request_id=plan.source_request_id,
                persist_user_message=plan.source_user_message,
            )

        step.status = "succeeded"
        next_action = self._advance_plan(session_id, state, plan, current_step_index, tool_result.get("result") or {})
        if next_action is not None:
            if isinstance(next_action, FinalResultRuntimeAction):
                self._clear_execution_state(state)
            return next_action

        summary = self._finalize_plan_summary(state, plan)
        self._clear_execution_state(state)
        return FinalResultRuntimeAction(
            plan_id=plan.plan_id,
            session_id=session_id,
            ok=True,
            summary=summary,
            request_id=plan.source_request_id,
            persist_user_message=plan.source_user_message,
        )

    async def handle_approval_response(self, session_id: str, approved: bool):
        state = self._memory_store.get(session_id)
        plan = state.active_plan
        if plan is None or not state.pending_approval:
            raise ValueError("no pending approval")

        if not approved:
            self._clear_execution_state(state)
            return AssistantTextRuntimeAction("已取消本次操作。", plan.source_request_id, plan.source_user_message)

        state.pending_approval = False
        plan.status = "approved"
        return self._issue_step(session_id, state, plan, 0)

    async def _handle_remote_request(
        self,
        session_id: str,
        request_id: str | None,
        raw_user_message: str,
        remote_prompt: str,
    ):
        if not self._settings.is_remote_model_configured():
            return AssistantTextRuntimeAction(
                "远程助手模式当前未启用，请先开启 `REMOTE_MODEL_ENABLED` 并配置远程模型。",
                request_id,
                raw_user_message,
            )

        gateway = self._remote_assistant_gateway or RemoteAssistantGateway(self._settings)
        try:
            reply = await gateway.reply(
                user_message=remote_prompt,
                host_context=self._memory_store.get(session_id).host_context_snapshot,
            )
        except Exception:
            reply = "远程助手当前不可用，稍后再试，或切回 control 模式继续执行软件控制。"

        if not reply:
            reply = "远程助手没有返回有效内容，请换个问法试试。"
        return AssistantTextRuntimeAction(reply, request_id, raw_user_message)

    def _extract_remote_prompt(self, content: str) -> str | None:
        lowered = content.strip().lower()
        for prefix in REMOTE_PREFIXES:
            if lowered.startswith(prefix):
                return content.strip()[len(prefix) :].strip() or "请简要回答。"
        return None

    def _local_model_error_reply(self, error: LocalModelGatewayError) -> str:
        if error.kind == "context_overflow":
            return (
                "本地模型上下文不足，当前控制快照过长。"
                "请缩短控制快照或提高本地模型上下文配置后再试。"
            )
        if error.kind == "format_error":
            return "本地模型返回格式异常，请重试；若持续出现，请检查控制编译提示词或模型兼容性。"
        return "本地控制模型当前不可用，请检查 `LOCAL_MODEL_BASE_URL`、模型服务和 `Qwen2.5-3B-Instruct` 是否已启动。"

    def _match_fast_path(self, content: str) -> ControlIntent | None:
        normalized = content.strip()
        for intent_name, keywords in FAST_PATH_PATTERNS.items():
            if any(keyword in normalized for keyword in keywords):
                return ControlIntent(
                    route="execute",
                    intent=intent_name,
                    arguments={},
                    entityRefs=[],
                    confirmation=False,
                    reasonCode="fast_path",
                )

        volume_match = re.search(r"(?:音量|声音)\D{0,4}(\d{1,3})", normalized)
        if volume_match is not None:
            return ControlIntent(
                route="execute",
                intent="set_volume",
                arguments={"volume": max(0, min(int(volume_match.group(1)), 100))},
                entityRefs=[],
                confirmation=False,
                reasonCode="fast_path",
            )

        for mode, keywords in PLAY_MODE_PATTERNS.items():
            if any(keyword in normalized for keyword in keywords):
                return ControlIntent(
                    route="execute",
                    intent="set_play_mode",
                    arguments={"mode": mode},
                    entityRefs=[],
                    confirmation=False,
                    reasonCode="fast_path",
                )

        local_library_keywords = ("本地音乐", "本地歌曲", "本地歌", "本地列表")
        local_library_verbs = ("列出", "看看", "显示", "有哪些", "查看", "浏览")
        if any(keyword in normalized for keyword in local_library_keywords) and (
            any(verb in normalized for verb in local_library_verbs) or normalized in local_library_keywords
        ):
            return ControlIntent(
                route="template",
                intent="query_local_tracks",
                arguments={"limit": 20},
                entityRefs=[],
                confirmation=False,
                reasonCode="fast_path",
            )

        if any(keyword in normalized for keyword in ("当前播放", "现在播放", "播放队列", "队列里")):
            return ControlIntent(
                route="template",
                intent="query_current_playback",
                arguments={},
                entityRefs=[],
                confirmation=False,
                reasonCode="fast_path",
            )
        return None

    def _plan_from_intent(
        self,
        session_id: str,
        request_id: str | None,
        user_message: str,
        intent: ControlIntent,
    ) -> ExecutionPlan | None:
        if intent.intent == "pause_playback":
            return self._single_step_plan(session_id, request_id, user_message, intent.intent, "暂停播放", "pausePlayback", {})
        if intent.intent == "resume_playback":
            return self._single_step_plan(session_id, request_id, user_message, intent.intent, "恢复播放", "resumePlayback", {})
        if intent.intent == "stop_playback":
            return self._single_step_plan(session_id, request_id, user_message, intent.intent, "停止播放", "stopPlayback", {})
        if intent.intent == "play_next":
            return self._single_step_plan(session_id, request_id, user_message, intent.intent, "播放下一首", "playNext", {})
        if intent.intent == "play_previous":
            return self._single_step_plan(session_id, request_id, user_message, intent.intent, "播放上一首", "playPrevious", {})
        if intent.intent == "set_volume":
            return self._single_step_plan(
                session_id,
                request_id,
                user_message,
                intent.intent,
                "设置音量",
                "setVolume",
                {"volume": int(intent.arguments.get("volume", 0))},
            )
        if intent.intent == "set_play_mode":
            return self._single_step_plan(
                session_id,
                request_id,
                user_message,
                intent.intent,
                "切换播放模式",
                "setPlayMode",
                {"mode": str(intent.arguments.get("mode") or "sequential")},
            )
        if intent.intent == "query_local_tracks":
            return self._single_step_plan(
                session_id,
                request_id,
                user_message,
                "query_local_tracks",
                "读取本地音乐列表",
                "getLocalTracks",
                {"limit": max(1, min(int(intent.arguments.get("limit", 20) or 20), 50))},
            )
        if intent.intent == "query_current_playback":
            return ExecutionPlan(
                plan_id=str(uuid.uuid4()),
                session_id=session_id,
                summary="查询当前播放状态",
                risk_level="low",
                status="draft",
                kind="query_current_playback",
                source_user_message=user_message,
                source_request_id=request_id,
                approval_message="",
                steps=[
                    PlanStep("current", "读取当前播放", "getCurrentTrack", {}),
                    PlanStep("queue", "读取播放队列", "getPlaybackQueue", {}),
                ],
                context={},
            )
        if intent.intent == "play_track_by_query":
            arguments = dict(intent.arguments)
            keyword = str(arguments.get("keyword") or arguments.get("title") or arguments.get("query") or "").strip()
            artist = str(arguments.get("artist") or "").strip()
            album = str(arguments.get("album") or "").strip()
            return ExecutionPlan(
                plan_id=str(uuid.uuid4()),
                session_id=session_id,
                summary="搜索后播放歌曲",
                risk_level="low",
                status="draft",
                kind="play_track_by_query",
                source_user_message=user_message,
                source_request_id=request_id,
                approval_message="",
                steps=[
                    PlanStep("search", "搜索歌曲", "searchTracks", {"keyword": keyword, "artist": artist, "album": album, "limit": 10}),
                    PlanStep("play", "播放目标歌曲", "playTrack", {}),
                ],
                context={"trackIndex": max(0, int(arguments.get("trackIndex", 1)) - 1)},
            )
        if intent.intent == "inspect_playlist_then_play_index":
            arguments = dict(intent.arguments)
            return ExecutionPlan(
                plan_id=str(uuid.uuid4()),
                session_id=session_id,
                summary="查看歌单后播放指定歌曲",
                risk_level="low",
                status="draft",
                kind="inspect_playlist_then_play_index",
                source_user_message=user_message,
                source_request_id=request_id,
                approval_message="",
                steps=[
                    PlanStep("playlists", "读取歌单列表", "getPlaylists", {}),
                    PlanStep("playlist_tracks", "读取歌单歌曲", "getPlaylistTracks", {}),
                    PlanStep("play", "播放歌单歌曲", "playTrack", {}),
                ],
                context={
                    "playlistQuery": str(arguments.get("playlistQuery") or arguments.get("playlistName") or "").strip(),
                    "trackIndex": max(0, int(arguments.get("trackIndex", 1)) - 1),
                    "entityRefs": list(intent.entity_refs),
                },
            )
        if intent.intent == "create_playlist_with_confirmation":
            playlist_name = str(intent.arguments.get("playlistName") or intent.arguments.get("name") or "").strip()
            return ExecutionPlan(
                plan_id=str(uuid.uuid4()),
                session_id=session_id,
                summary=f"创建歌单 {playlist_name or '未命名歌单'}",
                risk_level="medium",
                status="draft",
                kind="create_playlist_with_confirmation",
                source_user_message=user_message,
                source_request_id=request_id,
                approval_message=f"将创建歌单“{playlist_name}”，确认继续吗？",
                steps=[PlanStep("create_playlist", "创建歌单", "createPlaylist", {"name": playlist_name})],
                context={},
            )
        if intent.intent == "add_tracks_to_playlist_with_confirmation":
            arguments = dict(intent.arguments)
            source = str(arguments.get("source") or "recent_tracks").strip() or "recent_tracks"
            source_tool = {
                "recent_tracks": "getRecentTracks",
                "search_tracks": "searchTracks",
                "playlist_tracks": "getPlaylistTracks",
            }.get(source, "getRecentTracks")
            source_args: dict[str, Any] = {"limit": max(1, min(int(arguments.get("limit", 10)), 50))}
            if source_tool == "searchTracks":
                source_args = {
                    "keyword": str(arguments.get("keyword") or arguments.get("query") or "").strip(),
                    "artist": str(arguments.get("artist") or "").strip(),
                    "album": str(arguments.get("album") or "").strip(),
                    "limit": max(1, min(int(arguments.get("limit", 10)), 50)),
                }
            return ExecutionPlan(
                plan_id=str(uuid.uuid4()),
                session_id=session_id,
                summary="向歌单批量加歌",
                risk_level="medium",
                status="draft",
                kind="add_tracks_to_playlist_with_confirmation",
                source_user_message=user_message,
                source_request_id=request_id,
                approval_message=f"将把选中的歌曲加入歌单“{arguments.get('targetPlaylistName') or arguments.get('playlistName') or '目标歌单'}”，确认继续吗？",
                steps=[
                    PlanStep("playlists", "读取歌单列表", "getPlaylists", {}),
                    PlanStep("source_tracks", "读取待加入歌曲", source_tool, source_args),
                    PlanStep("add_tracks", "写入歌单", "addPlaylistItems", {}),
                ],
                context={
                    "targetPlaylistName": str(arguments.get("targetPlaylistName") or arguments.get("playlistName") or "").strip(),
                    "sourcePlaylistName": str(arguments.get("sourcePlaylistName") or "").strip(),
                    "sourceTool": source_tool,
                    "limit": max(1, min(int(arguments.get("limit", 10)), 50)),
                    "entityRefs": list(intent.entity_refs),
                },
            )
        return None

    def _single_step_plan(
        self,
        session_id: str,
        request_id: str | None,
        user_message: str,
        kind: str,
        summary: str,
        tool: str,
        args: dict[str, Any],
    ) -> ExecutionPlan:
        return ExecutionPlan(
            plan_id=str(uuid.uuid4()),
            session_id=session_id,
            summary=summary,
            risk_level="low",
            status="draft",
            kind=kind,
            source_user_message=user_message,
            source_request_id=request_id,
            approval_message="",
            steps=[PlanStep("execute", summary, tool, args)],
            context={},
        )

    def _dispatch_plan(self, session_id: str, state: WorkflowState, plan: ExecutionPlan):
        state.active_plan = plan
        state.goal_status = "running"
        state.pending_approval = False

        risk_decision = self._evaluate_plan_policy(state, plan)
        if not risk_decision.allowed:
            self._clear_execution_state(state)
            return AssistantTextRuntimeAction(risk_decision.reason, plan.source_request_id, plan.source_user_message)

        if plan.risk_level != risk_decision.risk_level:
            plan.risk_level = risk_decision.risk_level

        if risk_decision.require_approval:
            state.pending_approval = True
            plan.status = "waiting_approval"
            return PlanApprovalRuntimeAction(
                plan_id=plan.plan_id,
                session_id=session_id,
                summary=plan.summary,
                risk_level=plan.risk_level,
                steps=plan.preview_steps(),
                approval_message=plan.approval_message,
                request_id=plan.source_request_id,
            )
        return self._issue_step(session_id, state, plan, 0)

    def _guard_pending_approval_for_new_plan(
        self,
        state: WorkflowState,
        plan: ExecutionPlan,
        request_id: str | None,
        user_message: str,
    ) -> AssistantTextRuntimeAction | None:
        if not state.pending_approval or state.active_plan is None:
            return None
        if self._is_read_only_plan(plan):
            self._clear_execution_state(state)
            return None
        return AssistantTextRuntimeAction(
            "当前有一个待确认的写操作，请先同意或拒绝；如果想改做查询，我会优先执行只读请求。",
            request_id,
            user_message,
        )

    def _issue_step(self, session_id: str, state: WorkflowState, plan: ExecutionPlan, step_index: int):
        step = plan.steps[step_index]
        tool_policy = self._evaluate_tool_policy(state, step.tool)
        if not tool_policy.allowed:
            self._clear_execution_state(state)
            return AssistantTextRuntimeAction(tool_policy.reason, plan.source_request_id, plan.source_user_message)

        step.status = "waiting_tool"
        plan.status = "running"
        plan.context["stepIndex"] = step_index
        tool_call_id = str(uuid.uuid4())
        state.pending_tool_call = PendingToolCall(
            tool_call_id=tool_call_id,
            tool=step.tool,
            args=dict(step.args),
            request_id=plan.source_request_id,
            user_message=plan.source_user_message,
        )
        return ToolCallRuntimeAction(
            tool_call_id=tool_call_id,
            tool=step.tool,
            args=dict(step.args),
            request_id=plan.source_request_id,
        )

    def _advance_plan(
        self,
        session_id: str,
        state: WorkflowState,
        plan: ExecutionPlan,
        current_step_index: int,
        result: dict[str, Any],
    ):
        if plan.kind in {"pause_playback", "resume_playback", "stop_playback", "play_next", "play_previous", "set_volume", "set_play_mode"}:
            return None

        if plan.kind == "play_track_by_query":
            if current_step_index == 0:
                track = self._pick_track(result.get("items") or [], int(plan.context.get("trackIndex", 0)))
                if track is None:
                    return FinalResultRuntimeAction(plan.plan_id, session_id, False, "没有找到可播放的歌曲结果。", plan.source_request_id, plan.source_user_message)
                state.last_named_track = track
                state.last_result_set = list(result.get("items") or [])
                plan.context["selectedTrack"] = track
                plan.steps[1].args = {"trackId": track.get("trackId"), "musicPath": track.get("musicPath")}
                return self._issue_step(session_id, state, plan, 1)
            return None

        if plan.kind == "inspect_playlist_then_play_index":
            if current_step_index == 0:
                playlist = self._resolve_playlist(
                    list(result.get("items") or []),
                    str(plan.context.get("playlistQuery") or ""),
                    state.last_named_playlist if "last_named_playlist" in plan.context.get("entityRefs", []) else None,
                )
                if playlist is None:
                    return FinalResultRuntimeAction(plan.plan_id, session_id, False, "没有找到匹配的歌单。", plan.source_request_id, plan.source_user_message)
                state.last_named_playlist = playlist
                plan.context["selectedPlaylist"] = playlist
                plan.steps[1].args = {"playlistId": playlist.get("playlistId")}
                return self._issue_step(session_id, state, plan, 1)
            if current_step_index == 1:
                track = self._pick_track(result.get("items") or [], int(plan.context.get("trackIndex", 0)))
                if track is None:
                    return FinalResultRuntimeAction(plan.plan_id, session_id, False, "目标歌单里没有可播放的歌曲。", plan.source_request_id, plan.source_user_message)
                state.last_named_track = track
                state.last_result_set = list(result.get("items") or [])
                plan.context["selectedTrack"] = track
                plan.steps[2].args = {"trackId": track.get("trackId"), "musicPath": track.get("musicPath")}
                return self._issue_step(session_id, state, plan, 2)
            return None

        if plan.kind == "query_current_playback":
            if current_step_index == 0:
                plan.context["currentTrack"] = dict(result)
                return self._issue_step(session_id, state, plan, 1)
            plan.context["queueSummary"] = dict(result)
            return None

        if plan.kind == "create_playlist_with_confirmation":
            state.last_named_playlist = {"playlistId": result.get("playlistId"), "name": result.get("name")}
            return None

        if plan.kind == "add_tracks_to_playlist_with_confirmation":
            if current_step_index == 0:
                playlist_items = list(result.get("items") or [])
                target = self._resolve_playlist(
                    playlist_items,
                    str(plan.context.get("targetPlaylistName") or ""),
                    state.last_named_playlist if "last_named_playlist" in plan.context.get("entityRefs", []) else None,
                )
                if target is None:
                    return FinalResultRuntimeAction(plan.plan_id, session_id, False, "没有找到目标歌单，无法继续加歌。", plan.source_request_id, plan.source_user_message)
                plan.context["targetPlaylist"] = target
                if plan.context.get("sourceTool") == "getPlaylistTracks":
                    source = self._resolve_playlist(
                        playlist_items,
                        str(plan.context.get("sourcePlaylistName") or ""),
                        state.last_named_playlist if "last_named_playlist" in plan.context.get("entityRefs", []) else None,
                    )
                    if source is None:
                        return FinalResultRuntimeAction(plan.plan_id, session_id, False, "没有找到来源歌单，无法继续加歌。", plan.source_request_id, plan.source_user_message)
                    plan.context["sourcePlaylist"] = source
                    plan.steps[1].args = {"playlistId": source.get("playlistId")}
                return self._issue_step(session_id, state, plan, 1)
            if current_step_index == 1:
                tracks = list(result.get("items") or [])
                if not tracks:
                    return FinalResultRuntimeAction(plan.plan_id, session_id, False, "没有找到可加入歌单的歌曲。", plan.source_request_id, plan.source_user_message)
                chosen_tracks = tracks[: int(plan.context.get("limit", 10))]
                track_ids = [str(item.get("trackId") or "").strip() for item in chosen_tracks if str(item.get("trackId") or "").strip()]
                if not track_ids:
                    return FinalResultRuntimeAction(plan.plan_id, session_id, False, "解析待加入歌曲失败，缺少可用 trackId。", plan.source_request_id, plan.source_user_message)
                state.last_result_set = chosen_tracks
                plan.context["selectedTracks"] = chosen_tracks
                target_playlist = plan.context.get("targetPlaylist") or {}
                plan.steps[2].args = {"playlistId": target_playlist.get("playlistId"), "trackIds": track_ids}
                return self._issue_step(session_id, state, plan, 2)
            return None

        return None

    def _evaluate_plan_policy(self, state: WorkflowState, plan: ExecutionPlan) -> RiskPolicyDecision:
        require_approval = plan.kind in {"create_playlist_with_confirmation", "add_tracks_to_playlist_with_confirmation"}
        risk_level = "medium" if require_approval else "low"
        if HostContextSnapshot.model_validate(state.host_context_snapshot).offline_mode:
            for step in plan.steps:
                tool_policy = self._evaluate_tool_policy(state, step.tool)
                if not tool_policy.allowed:
                    return tool_policy
        return RiskPolicyDecision(requireApproval=require_approval, allowed=True, riskLevel=risk_level, reason="")

    def _evaluate_tool_policy(self, state: WorkflowState, tool_name: str) -> RiskPolicyDecision:
        capabilities = CapabilitySnapshot.model_validate(state.capability_snapshot)
        host = HostContextSnapshot.model_validate(state.host_context_snapshot)
        tool_def = next((tool for tool in capabilities.tools if str(tool.get("name") or "") == tool_name), None)

        if capabilities.tools and tool_def is None:
            return RiskPolicyDecision(requireApproval=False, allowed=False, riskLevel="high", reason=f"当前客户端没有开放 `{tool_name}` 能力。")

        availability_policy = str((tool_def or {}).get("availabilityPolicy") or "always").strip().lower()
        if availability_policy == "login_required" and not host.logged_in:
            return RiskPolicyDecision(requireApproval=False, allowed=False, riskLevel="medium", reason="当前用户未登录，无法执行这类在线操作。")
        if availability_policy == "online_required" and host.offline_mode:
            return RiskPolicyDecision(requireApproval=False, allowed=False, riskLevel="medium", reason="当前处于离线模式，无法执行需要联网的操作。")
        return RiskPolicyDecision(requireApproval=False, allowed=True, riskLevel="low", reason="")

    @staticmethod
    def _is_read_only_tool(tool_name: str) -> bool:
        return tool_name in {
            "getCurrentTrack",
            "getPlaybackQueue",
            "getLocalTracks",
            "searchTracks",
            "getRecentTracks",
            "getFavorites",
            "getPlaylists",
            "getPlaylistTracks",
            "getHostContext",
            "getVisiblePage",
            "getSelectedPlaylist",
            "getSelectedTrackIds",
            "getUserProfile",
            "refreshUserProfile",
            "getSettingsSnapshot",
        }

    @classmethod
    def _is_read_only_plan(cls, plan: ExecutionPlan) -> bool:
        return all(cls._is_read_only_tool(step.tool) for step in plan.steps)

    def _memory_snapshot(self, state: WorkflowState) -> dict[str, Any]:
        return {
            "lastNamedTrack": state.last_named_track,
            "lastNamedPlaylist": state.last_named_playlist,
            "lastResultSet": state.last_result_set,
            "currentPlayback": state.current_playback,
            "queueSummary": state.playback_queue,
        }

    @staticmethod
    def _resolve_playlist(
        playlists: list[dict[str, Any]],
        query: str,
        fallback_playlist: dict[str, Any] | None,
    ) -> dict[str, Any] | None:
        if query.strip():
            exact = next((item for item in playlists if str(item.get("name") or "").strip() == query.strip()), None)
            if exact is not None:
                return exact
            fuzzy = next((item for item in playlists if query.strip() in str(item.get("name") or "").strip()), None)
            if fuzzy is not None:
                return fuzzy
        if fallback_playlist:
            fallback_id = fallback_playlist.get("playlistId")
            if fallback_id:
                return next((item for item in playlists if item.get("playlistId") == fallback_id), None)
        return None

    @staticmethod
    def _pick_track(items: list[dict[str, Any]], index: int) -> dict[str, Any] | None:
        if not items:
            return None
        bounded = max(0, min(index, len(items) - 1))
        return items[bounded]

    def _integrate_tool_result(self, state: WorkflowState, tool: str, tool_result: dict[str, Any]) -> None:
        if not tool_result.get("ok", False):
            return
        result = tool_result.get("result") or {}
        if tool == "getCurrentTrack":
            state.current_playback = dict(result)
        elif tool == "getPlaybackQueue":
            state.playback_queue = dict(result)
        elif tool in {"searchTracks", "getRecentTracks", "getFavorites", "getPlaylistTracks", "getLocalTracks"}:
            state.last_result_set = list(result.get("items") or [])
        elif tool == "getPlaylists":
            items = list(result.get("items") or [])
            if items:
                state.recent_playlist_candidates = items
        elif tool == "createPlaylist":
            state.last_named_playlist = {"playlistId": result.get("playlistId"), "name": result.get("name")}

    def _finalize_plan_summary(self, state: WorkflowState, plan: ExecutionPlan) -> str:
        if plan.kind == "pause_playback":
            return "已暂停播放。"
        if plan.kind == "resume_playback":
            return "已恢复播放。"
        if plan.kind == "stop_playback":
            return "已停止播放。"
        if plan.kind == "play_next":
            return "已切到下一首。"
        if plan.kind == "play_previous":
            return "已切到上一首。"
        if plan.kind == "set_volume":
            return f"已将音量设置为 {plan.steps[0].args.get('volume')}。"
        if plan.kind == "set_play_mode":
            return f"已切换到 {plan.steps[0].args.get('mode')} 模式。"
        if plan.kind == "play_track_by_query":
            track = plan.context.get("selectedTrack") or state.last_named_track or {}
            title = str(track.get("title") or "目标歌曲")
            artist = str(track.get("artist") or "").strip()
            return f"已开始播放《{title}》{f' - {artist}' if artist else ''}。"
        if plan.kind == "inspect_playlist_then_play_index":
            playlist = plan.context.get("selectedPlaylist") or {}
            track = plan.context.get("selectedTrack") or {}
            return f"已在歌单“{playlist.get('name') or '目标歌单'}”中播放《{track.get('title') or '目标歌曲'}》。"
        if plan.kind == "query_local_tracks":
            tracks = plan.context.get("localTracks") or state.last_result_set or []
            if not tracks:
                return "当前本地音乐列表为空。"
            preview_titles: list[str] = []
            for item in tracks[:5]:
                if not isinstance(item, dict):
                    continue
                title = str(item.get("title") or item.get("fileName") or item.get("name") or "").strip()
                if title:
                    preview_titles.append(f"《{title}》")
            if preview_titles:
                return f"当前本地音乐共有 {len(tracks)} 首，例如：{ '、'.join(preview_titles) }。"
            return f"当前本地音乐共有 {len(tracks)} 首。"
        if plan.kind == "query_current_playback":
            current = plan.context.get("currentTrack") or state.current_playback or {}
            queue = plan.context.get("queueSummary") or state.playback_queue or {}
            if not current or not current.get("playing", False):
                return "当前没有正在播放的歌曲。"
            title = str(current.get("title") or "未知歌曲")
            artist = str(current.get("artist") or "").strip()
            queue_count = int(queue.get("count", 0) or 0)
            return f"当前正在播放《{title}》{f' - {artist}' if artist else ''}，队列里共有 {queue_count} 首。"
        if plan.kind == "create_playlist_with_confirmation":
            playlist = state.last_named_playlist or {}
            return f"已创建歌单“{playlist.get('name') or '新歌单'}”。"
        if plan.kind == "add_tracks_to_playlist_with_confirmation":
            playlist = plan.context.get("targetPlaylist") or {}
            tracks = plan.context.get("selectedTracks") or []
            return f"已将 {len(tracks)} 首歌曲加入歌单“{playlist.get('name') or '目标歌单'}”。"
        return plan.summary

    @staticmethod
    def _summarize_tool_failure(tool: str, error: dict[str, Any]) -> str:
        message = str(error.get("message") or "").strip()
        return message or f"`{tool}` 执行失败。"

    @staticmethod
    def _clear_execution_state(state: WorkflowState) -> None:
        state.pending_tool_call = None
        state.pending_approval = False
        state.active_plan = None
        state.goal_status = "idle"

    @staticmethod
    def _restricted_reply(reason_code: str) -> str:
        reason = reason_code.strip().lower()
        if reason in {"knowledge", "world_knowledge"}:
            return "当前模式只负责控制软件和解释软件状态，不处理宽世界知识问答。需要的话请显式使用 `/assistant`。"
        if reason in {"long_form", "copywriting"}:
            return "当前模式只负责控制软件，不生成长文案或乐评。需要的话请显式使用 `/assistant`。"
        if reason in {"chat", "open_chat"}:
            return "当前模式是控制模式，不做开放聊天。你可以直接告诉我要控制哪个软件功能。"
        return "当前模式只负责控制软件和解释软件状态，这类请求请改成明确的控制指令，或显式使用 `/assistant`。"
