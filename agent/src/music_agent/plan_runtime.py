from __future__ import annotations

"""
模块名: plan_runtime
功能概述: 收纳审批计划与计划执行链的构造、推进、失败收口逻辑。
对外接口: PlanRuntimeMixin
依赖关系: 依赖 plan_models、response_style、runtime_actions；由 MusicAgentRuntime 继承。
输入输出: 输入为会话状态、工具结果与计划参数；输出为计划相关运行时动作。
异常与错误: 依赖宿主 runtime 提供 _issue_tool_call() 等桥接能力。
维护说明: 后续若继续把审批/计划体系独立成 orchestration 层，应优先在本模块扩展。
"""

import uuid

from .plan_models import ExecutionPlan, PlanStep
from .response_style import (
    TEXT_PROGRESS_ADD_TRACKS,
    TEXT_PROGRESS_APPROVED,
    TEXT_PROGRESS_FETCH_TOP_TRACKS,
    UNKNOWN_PLAYLIST,
    summarize_create_playlist,
)
from .runtime_actions import (
    AssistantTextRuntimeAction,
    FinalResultRuntimeAction,
    PlanApprovalRuntimeAction,
    ProgressRuntimeAction,
    RuntimeActionBatch,
)


class PlanRuntimeMixin:
    def _handle_failed_tool_result(
        self,
        session_id: str,
        state,
        pending,
        error: dict,
    ):
        state.workflow_mode = "chat"
        state.pending_approval = False
        state.goal_status = "failed"
        message = error.get("message", "未知错误")
        plan = state.active_plan
        if plan is not None:
            state.active_plan = None
            return FinalResultRuntimeAction(
                plan_id=plan.plan_id,
                session_id=session_id,
                ok=False,
                summary=f"计划执行失败：{message}",
                request_id=plan.source_request_id,
                persist_user_message=plan.source_user_message,
            )
        return AssistantTextRuntimeAction(
            text=f"操作失败：{message}",
            request_id=pending.request_id,
            persist_user_message=pending.user_message,
        )

    def _handle_plan_tool_result(
        self,
        session_id: str,
        state,
        pending,
        result: dict,
    ):
        plan = state.active_plan
        if plan is None:
            return None

        if plan.kind == "create_playlist" and pending.tool == "createPlaylist":
            state.active_plan = None
            state.pending_approval = False
            state.workflow_mode = "chat"
            summary = summarize_create_playlist(result.get("playlist") or {})
            return RuntimeActionBatch(
                actions=[
                    FinalResultRuntimeAction(
                        plan_id=plan.plan_id,
                        session_id=session_id,
                        ok=True,
                        summary=summary,
                        request_id=plan.source_request_id,
                        persist_user_message=plan.source_user_message,
                    )
                ]
            )

        if plan.kind == "create_playlist_with_top_tracks":
            if pending.tool == "createPlaylist":
                playlist = result.get("playlist") or {}
                plan.context["playlist"] = playlist
                plan.steps[0].status = "succeeded"
                plan.steps[1].status = "waiting_tool"
                return RuntimeActionBatch(
                    actions=[
                        ProgressRuntimeAction(plan.plan_id, plan.steps[1].step_id, TEXT_PROGRESS_FETCH_TOP_TRACKS),
                        self._issue_tool_call(
                            session_id,
                            state,
                            plan.source_request_id,
                            plan.source_user_message,
                            "getTopPlayedTracks",
                            {"limit": self.TOP_TRACK_LIMIT},
                        ),
                    ]
                )

            if pending.tool == "getTopPlayedTracks":
                items = list(result.get("items", []))
                track_ids = [item["trackId"] for item in items if item.get("trackId")]
                plan.context["topTracks"] = items
                plan.context["trackIds"] = track_ids
                plan.steps[1].status = "succeeded"
                if not track_ids:
                    state.active_plan = None
                    state.pending_approval = False
                    state.workflow_mode = "chat"
                    return RuntimeActionBatch(
                        actions=[
                            FinalResultRuntimeAction(
                                plan_id=plan.plan_id,
                                session_id=session_id,
                                ok=False,
                                summary="最近常听歌曲列表为空，未向歌单添加内容。",
                                request_id=plan.source_request_id,
                                persist_user_message=plan.source_user_message,
                            )
                        ]
                    )
                plan.steps[2].status = "waiting_tool"
                return RuntimeActionBatch(
                    actions=[
                        ProgressRuntimeAction(plan.plan_id, plan.steps[2].step_id, TEXT_PROGRESS_ADD_TRACKS),
                        self._issue_tool_call(
                            session_id,
                            state,
                            plan.source_request_id,
                            plan.source_user_message,
                            "addTracksToPlaylist",
                            {
                                "playlistId": (plan.context.get("playlist") or {}).get("playlistId", ""),
                                "trackIds": track_ids,
                            },
                        ),
                    ]
                )

            if pending.tool == "addTracksToPlaylist":
                plan.steps[2].status = "succeeded"
                state.active_plan = None
                state.pending_approval = False
                state.workflow_mode = "chat"
                playlist = plan.context.get("playlist") or {}
                added_count = int(result.get("addedCount", 0))
                summary = (
                    f"已创建歌单 **{playlist.get('name', UNKNOWN_PLAYLIST)}**，"
                    f"并添加 {added_count} 首最近常听歌曲。"
                )
                return RuntimeActionBatch(
                    actions=[
                        FinalResultRuntimeAction(
                            plan_id=plan.plan_id,
                            session_id=session_id,
                            ok=True,
                            summary=summary,
                            request_id=plan.source_request_id,
                            persist_user_message=plan.source_user_message,
                        )
                    ]
                )

        return None

    def _build_create_playlist_plan(
        self,
        session_id: str,
        request_id: str | None,
        user_message: str,
        playlist_name: str,
        state,
    ) -> PlanApprovalRuntimeAction:
        plan = ExecutionPlan(
            plan_id=f"plan-{uuid.uuid4().hex}",
            session_id=session_id,
            summary=f"创建歌单“{playlist_name}”",
            risk_level="medium",
            status="waiting_approval",
            kind="create_playlist",
            source_user_message=user_message,
            source_request_id=request_id,
            approval_message=f"即将创建歌单“{playlist_name}”，是否继续？",
            steps=[
                PlanStep(
                    step_id="step-1",
                    title=f"创建歌单 {playlist_name}",
                    tool="createPlaylist",
                    args={"name": playlist_name},
                )
            ],
        )
        state.active_plan = plan
        state.pending_approval = True
        state.workflow_mode = "tool"
        return PlanApprovalRuntimeAction(
            plan_id=plan.plan_id,
            session_id=session_id,
            summary=plan.summary,
            risk_level=plan.risk_level,
            steps=plan.preview_steps(),
            approval_message=plan.approval_message,
            request_id=request_id,
        )

    def _build_create_playlist_with_top_tracks_plan(
        self,
        session_id: str,
        request_id: str | None,
        user_message: str,
        playlist_name: str,
        state,
    ) -> PlanApprovalRuntimeAction:
        plan = ExecutionPlan(
            plan_id=f"plan-{uuid.uuid4().hex}",
            session_id=session_id,
            summary=f"创建歌单“{playlist_name}”并添加最近常听歌曲",
            risk_level="high",
            status="waiting_approval",
            kind="create_playlist_with_top_tracks",
            source_user_message=user_message,
            source_request_id=request_id,
            approval_message=f"即将创建歌单“{playlist_name}”并添加 {self.TOP_TRACK_LIMIT} 首最近常听歌曲，是否继续？",
            steps=[
                PlanStep(
                    step_id="step-1",
                    title=f"创建歌单 {playlist_name}",
                    tool="createPlaylist",
                    args={"name": playlist_name},
                ),
                PlanStep(
                    step_id="step-2",
                    title=f"获取最近常听 {self.TOP_TRACK_LIMIT} 首歌曲",
                    tool="getTopPlayedTracks",
                    args={"limit": self.TOP_TRACK_LIMIT},
                ),
                PlanStep(
                    step_id="step-3",
                    title="将最近常听歌曲加入歌单",
                    tool="addTracksToPlaylist",
                    args={"playlistId": "", "trackIds": []},
                ),
            ],
        )
        state.active_plan = plan
        state.pending_approval = True
        state.workflow_mode = "tool"
        return PlanApprovalRuntimeAction(
            plan_id=plan.plan_id,
            session_id=session_id,
            summary=plan.summary,
            risk_level=plan.risk_level,
            steps=plan.preview_steps(),
            approval_message=plan.approval_message,
            request_id=request_id,
        )
