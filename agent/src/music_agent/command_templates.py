from __future__ import annotations

"""
模块名称: command_templates
功能概述: 在任务链步骤资产之上再抽一层命令模板，负责把语义目标映射成稳定的工作流模板，并声明推荐执行载体与后备载体。
对外接口: CommandTemplateDecision, resolve_command_template()
依赖关系: 依赖 task_templates 提供步骤计划，依赖 semantic_models 与 workflow_memory 提供语义和工作记忆
输入输出: 输入为结构化语义结果与会话状态；输出为命令模板决策，包含模板名、步骤链和推荐执行载体
异常与错误: 不主动抛出运行时异常；模板不可用时返回 rejected/not_applicable 状态
维护说明: 这里表达“目标模板”和“推荐执行方式”，不直接承担脚本构造、工具派发或审批执行
"""

from dataclasses import dataclass, field
from typing import Literal

from .semantic_models import SemanticParseResult
from .task_templates import TaskTemplatePlan, TaskTemplateStep, resolve_task_template
from .workflow_memory import WorkflowState


ExecutionSubstrate = Literal["script", "tool_chain", "direct_tool", "clarify", "approval", "none"]


@dataclass(frozen=True)
class CommandTemplateDecision:
    template_name: str
    status: Literal["selected", "rejected", "not_applicable"]
    reason: str
    steps: tuple[TaskTemplateStep, ...] = ()
    preferred_substrate: ExecutionSubstrate = "none"
    fallback_substrates: tuple[ExecutionSubstrate, ...] = field(default_factory=tuple)


def resolve_command_template(semantic: SemanticParseResult, state: WorkflowState) -> CommandTemplateDecision:
    plan = resolve_task_template(semantic, state)
    if plan.status != "selected":
        return CommandTemplateDecision(
            template_name=_template_name_for_intent(semantic.intent),
            status=plan.status,
            reason=plan.reason,
        )

    return CommandTemplateDecision(
        template_name=_template_name_for_intent(semantic.intent),
        status="selected",
        reason=plan.reason,
        steps=tuple(plan.steps),
        preferred_substrate=_preferred_substrate_for_intent(semantic.intent, plan),
        fallback_substrates=_fallback_substrates_for_intent(semantic.intent),
    )


def _template_name_for_intent(intent: str) -> str:
    mapping = {
        "get_current_track": "get_current_track",
        "stop_playback": "stop_playback",
        "get_recent_tracks": "get_recent_tracks",
        "get_playlists": "get_playlists",
        "query_playlist": "query_playlist",
        "inspect_playlist_tracks": "inspect_playlist_tracks",
        "play_track": "play_track_from_search_or_context",
        "play_playlist": "play_playlist_from_lookup_or_context",
        "create_playlist_from_playlist_subset": "create_playlist_from_playlist_subset",
    }
    return mapping.get(intent, intent or "unknown")


def _preferred_substrate_for_intent(intent: str, plan: TaskTemplatePlan) -> ExecutionSubstrate:
    if intent == "play_track" and len(plan.steps) >= 2:
        return "script"
    if intent == "create_playlist_from_playlist_subset":
        return "script"
    if len(plan.steps) == 1:
        return "direct_tool"
    return "tool_chain"


def _fallback_substrates_for_intent(intent: str) -> tuple[ExecutionSubstrate, ...]:
    if intent == "play_track":
        return ("tool_chain",)
    if intent == "create_playlist_from_playlist_subset":
        return ("tool_chain", "clarify")
    if intent in {"inspect_playlist_tracks", "play_playlist", "query_playlist"}:
        return ("tool_chain", "clarify")
    return ()
