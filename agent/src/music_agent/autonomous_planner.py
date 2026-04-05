from __future__ import annotations

from typing import Any

from .command_templates import resolve_command_template
from .capability_reasoner import evaluate_capability
from .semantic_models import SemanticParseResult
from .task_templates import TaskTemplateStep
from .workflow_memory import WorkflowState


class AutonomousPlanner:
    def __init__(self, *, allow_direct_write_actions: bool) -> None:
        self._allow_direct_write_actions = allow_direct_write_actions

    def build_action_candidates(self, semantic: SemanticParseResult, state: WorkflowState) -> list[dict[str, Any]]:
        template = resolve_command_template(semantic, state)
        if template.status != "selected":
            return []
        return self._tool_steps(list(template.steps), template_reason=template.reason)

    def _tool_steps(self, steps: list[TaskTemplateStep], *, template_reason: str) -> list[dict[str, Any]]:
        candidates: list[dict[str, Any]] = []
        previous_step_id: str | None = None
        for index, step in enumerate(steps, start=1):
            tool = step.tool
            args = step.args
            decision = evaluate_capability(tool, allow_direct_write_actions=self._allow_direct_write_actions)
            if decision is None or not decision.executable:
                return []
            candidate = {
                "stepId": f"planner-{index}",
                "tool": tool,
                "args": {key: value for key, value in args.items() if value is not None},
                "kind": "tool",
                "dependsOn": [previous_step_id] if previous_step_id else [],
                "reason": f"planner:{template_reason}:{decision.reason}",
                "requiresApproval": decision.requires_confirmation and not decision.capability.read_only,
                "mayNeedClarification": False,
            }
            candidates.append(candidate)
            previous_step_id = candidate["stepId"]
        return candidates
