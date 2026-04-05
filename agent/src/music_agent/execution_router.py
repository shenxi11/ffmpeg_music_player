from __future__ import annotations

"""
模块名称: execution_router
功能概述: 根据命令模板、脚本规划结果和当前状态，为服务端选择本轮应走的执行载体，例如脚本、工具链、澄清或阻断。
对外接口: ExecutionRouteDecision, ExecutionRouter.route()
依赖关系: 依赖 command_templates、client_script_planner、policy_hooks；由 semantic_runtime 调用
输入输出: 输入为语义结果、会话状态与写操作策略；输出为执行载体路由结果和审计所需元数据
异常与错误: 不主动抛出运行时异常；无法路由时返回 blocked/not_applicable 状态
维护说明: 这里负责“先模板后载体”，不要把脚本构造或工具派发逻辑重新塞回本模块
"""

from dataclasses import dataclass, field
from typing import Any, Literal

from .capability_reasoner import evaluate_capability_chain
from .client_script_planner import ClientScriptPlanner, ScriptPlanningResult
from .command_templates import CommandTemplateDecision, resolve_command_template
from .policy_hooks import is_compound_script_intent, should_block_compound_script_fallback
from .semantic_models import SemanticParseResult
from .workflow_memory import WorkflowState


@dataclass(frozen=True)
class ExecutionRouteDecision:
    status: Literal["selected", "blocked", "not_applicable"]
    substrate: Literal["script", "tool_chain", "direct_tool", "clarify", "approval", "none"]
    reason: str
    template: CommandTemplateDecision
    script_planning: ScriptPlanningResult | None = None
    metadata: dict[str, Any] = field(default_factory=dict)


class ExecutionRouter:
    def __init__(self, script_planner: ClientScriptPlanner, *, allow_direct_write_actions: bool) -> None:
        self._script_planner = script_planner
        self._allow_direct_write_actions = allow_direct_write_actions

    def route(self, semantic: SemanticParseResult, state: WorkflowState) -> ExecutionRouteDecision:
        template = resolve_command_template(semantic, state)
        if template.status != "selected":
            return ExecutionRouteDecision(
                status="not_applicable",
                substrate="none",
                reason=f"template_{template.status}:{template.reason}",
                template=template,
            )

        chain_ok, chain_reason, chain_decisions = evaluate_capability_chain(
            [step.tool for step in template.steps],
            allow_direct_write_actions=self._allow_direct_write_actions,
        )
        if not chain_ok:
            blocked_capability = next((item.capability.name for item in chain_decisions if not item.executable), None)
            return ExecutionRouteDecision(
                status="blocked",
                substrate="none",
                reason=f"unsupported_capability:{chain_reason}",
                template=template,
                metadata={
                    "blockedCapability": blocked_capability,
                    "capabilityChainReason": chain_reason,
                },
            )

        script_planning = self._script_planner.plan_script(semantic, state)

        if template.preferred_substrate == "script" and script_planning.status == "selected":
            return ExecutionRouteDecision(
                status="selected",
                substrate="script",
                reason=script_planning.reason,
                template=template,
                script_planning=script_planning,
                metadata={"scriptPreferred": True},
            )

        if template.preferred_substrate == "script":
            tool_fallback_allowed = "tool_chain" in template.fallback_substrates or "direct_tool" in template.fallback_substrates
            if tool_fallback_allowed and self._can_fallback_to_template_execution(semantic, script_planning):
                return ExecutionRouteDecision(
                    status="selected",
                    substrate="tool_chain" if len(template.steps) > 1 else "direct_tool",
                    reason=f"script_{script_planning.status}:{script_planning.reason}",
                    template=template,
                    script_planning=script_planning,
                    metadata={"scriptPreferred": True, "usedFallbackSubstrate": True},
                )
            if should_block_compound_script_fallback(semantic.intent, semantic.missing_fields):
                return ExecutionRouteDecision(
                    status="blocked",
                    substrate="none",
                    reason=f"script_{script_planning.status}:{script_planning.reason}",
                    template=template,
                    script_planning=script_planning,
                    metadata={"scriptPreferred": True, "blockedCompoundIntent": True},
                )

        if template.preferred_substrate in {"tool_chain", "direct_tool"}:
            return ExecutionRouteDecision(
                status="selected",
                substrate=template.preferred_substrate,
                reason=template.reason,
                template=template,
            )

        return ExecutionRouteDecision(
            status="not_applicable",
            substrate="none",
            reason=f"no_supported_substrate:{template.template_name}",
            template=template,
            script_planning=script_planning,
        )

    def _can_fallback_to_template_execution(
        self,
        semantic: SemanticParseResult,
        script_planning: ScriptPlanningResult,
    ) -> bool:
        if semantic.intent == "create_playlist_from_playlist_subset":
            if not self._allow_direct_write_actions:
                return False
            return script_planning.reason in {
                "source_playlist_not_uniquely_resolved",
                "missing_source_playlist",
                "track_selection_mode_not_supported_for_script",
            }
        if semantic.intent == "play_track":
            return True
        return False
