from __future__ import annotations

from dataclasses import dataclass, field
from typing import Literal


PlanRiskLevel = Literal["low", "medium", "high"]
PlanStatus = Literal["draft", "waiting_approval", "approved", "running", "completed", "cancelled", "failed"]
PlanStepStatus = Literal["pending", "waiting_tool", "succeeded", "failed", "skipped", "cancelled"]


@dataclass
class PlanStep:
    step_id: str
    title: str
    tool: str
    args: dict
    status: PlanStepStatus = "pending"


@dataclass
class ExecutionPlan:
    plan_id: str
    session_id: str
    summary: str
    risk_level: PlanRiskLevel
    status: PlanStatus
    kind: str
    source_user_message: str
    source_request_id: str | None
    approval_message: str
    steps: list[PlanStep] = field(default_factory=list)
    context: dict = field(default_factory=dict)

    def preview_steps(self) -> list[dict]:
        return [
            {
                "stepId": step.step_id,
                "title": step.title,
                "status": step.status,
            }
            for step in self.steps
        ]
