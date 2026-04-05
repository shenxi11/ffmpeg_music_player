from __future__ import annotations

"""
模块名: capability_reasoner
功能概述: 根据能力目录对工具/能力做执行资格判断，并为上层规划器提供统一的能力路由结论。
对外接口: CapabilityDecision、evaluate_capability()、evaluate_capability_chain()、capability_hint()、requires_structured_object()
依赖关系: 依赖 capability_catalog 提供能力定义
输入输出: 输入为能力名和直写策略；输出为单个能力或能力链的路由决策
异常与错误: 未知能力返回 None；能力链校验失败时返回第一个不可执行原因
维护说明: 这里是“能力路由层”的起点，后续不应再把大量能力资格判断散落回 runtime 主流程
"""

from dataclasses import dataclass
from typing import Any

from .capability_catalog import CapabilityDefinition, get_capability


@dataclass(frozen=True)
class CapabilityDecision:
    capability: CapabilityDefinition
    executable: bool
    reason: str
    route: str
    requires_confirmation: bool = False


def evaluate_capability(name: str, *, allow_direct_write_actions: bool) -> CapabilityDecision | None:
    capability = get_capability(name)
    if capability is None:
        return None
    if not capability.exposed_to_backend:
        return CapabilityDecision(capability, False, "not_exposed_to_backend", "unavailable")
    if capability.automation_policy == "restricted":
        return CapabilityDecision(capability, False, "restricted", "restricted")
    if capability.automation_policy == "confirm" and not capability.read_only and not allow_direct_write_actions:
        return CapabilityDecision(capability, False, "confirmation_required", "confirm", requires_confirmation=True)
    if capability.stability == "partial" and capability.name == "playPlaylist":
        return CapabilityDecision(
            capability,
            True,
            "partial_capability_use_with_guardrails",
            "guarded_auto",
            requires_confirmation=True,
        )
    return CapabilityDecision(capability, True, "allowed", capability.automation_policy)


def evaluate_capability_chain(
    names: list[str] | tuple[str, ...],
    *,
    allow_direct_write_actions: bool,
) -> tuple[bool, str, list[CapabilityDecision]]:
    decisions: list[CapabilityDecision] = []
    for name in names:
        decision = evaluate_capability(name, allow_direct_write_actions=allow_direct_write_actions)
        if decision is None:
            return False, f"unknown_capability:{name}", decisions
        decisions.append(decision)
        if not decision.executable:
            return False, f"{name}:{decision.reason}", decisions
    return True, "allowed", decisions


def capability_hint(name: str) -> dict[str, Any] | None:
    capability = get_capability(name)
    if capability is None:
        return None
    return {
        "name": capability.name,
        "category": capability.category,
        "stability": capability.stability,
        "automationPolicy": capability.automation_policy,
        "prerequisiteObjects": list(capability.prerequisite_objects),
        "producesObjects": list(capability.produces_objects),
        "readOnly": capability.read_only,
        "notes": capability.notes,
        "requiresStructuredObject": capability.requires_structured_object,
        "clientEntryPoint": capability.client_entry_point,
        "invocationPaths": list(capability.invocation_paths),
        "backingExecutor": capability.backing_executor,
    }


def requires_structured_object(name: str) -> bool:
    capability = get_capability(name)
    if capability is None:
        return False
    return capability.requires_structured_object
