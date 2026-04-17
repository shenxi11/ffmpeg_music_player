from __future__ import annotations

from dataclasses import dataclass, field
from threading import RLock
from time import monotonic

from .plan_models import ExecutionPlan


@dataclass
class PendingToolCall:
    tool_call_id: str
    tool: str
    args: dict
    request_id: str | None
    user_message: str
    started_at: float = field(default_factory=monotonic)


@dataclass
class PendingScriptDryRun:
    request_id: str | None
    user_message: str
    script: dict
    started_at: float = field(default_factory=monotonic)


@dataclass
class PendingScriptValidation:
    request_id: str | None
    user_message: str
    script: dict
    started_at: float = field(default_factory=monotonic)


@dataclass
class PendingScriptExecution:
    request_id: str | None
    user_message: str
    script: dict
    execution_id: str | None = None
    cancellation_request_id: str | None = None
    cancellation_user_message: str | None = None
    cancellation_reason: str | None = None
    cancellation_acknowledged: bool = False
    started_summary: dict = field(default_factory=dict)
    started_at: float = field(default_factory=monotonic)


@dataclass
class PendingClarification:
    kind: str
    resolution_action: str
    question: str
    options: list[str]
    items: list[dict]


@dataclass
class WorkflowState:
    session_id: str
    workflow_mode: str = "chat"
    agent_mode: str = "control"
    current_goal: str | None = None
    goal_status: str = "idle"
    goal_history: list[str] = field(default_factory=list)
    focus_domain: str | None = None
    intent: str | None = None
    entities: dict = field(default_factory=dict)
    resolved_entities: dict = field(default_factory=dict)
    ambiguities: list[str] = field(default_factory=list)
    semantic_history: list[dict] = field(default_factory=list)
    tool_stack: list[str] = field(default_factory=list)
    pending_action_candidates: list[dict] = field(default_factory=list)
    active_action_candidate: dict | None = None
    completed_action_candidate_ids: list[str] = field(default_factory=list)
    action_observation_history: list[dict] = field(default_factory=list)
    pending_tool_call: PendingToolCall | None = None
    pending_script_dry_run: PendingScriptDryRun | None = None
    pending_script_validation: PendingScriptValidation | None = None
    pending_script_execution: PendingScriptExecution | None = None
    last_tool_result: dict | None = None
    recent_track_candidates: list[dict] = field(default_factory=list)
    recent_playlist_candidates: list[dict] = field(default_factory=list)
    recent_playlist_tracks: list[dict] = field(default_factory=list)
    recent_recent_tracks: list[dict] = field(default_factory=list)
    last_named_playlist: dict | None = None
    last_named_track: dict | None = None
    current_playback: dict | None = None
    playback_queue: dict | None = None
    last_tool_observation: dict | None = None
    pending_clarification: PendingClarification | None = None
    active_plan: ExecutionPlan | None = None
    pending_approval: bool = False
    host_context_snapshot: dict = field(default_factory=dict)
    capability_snapshot: dict = field(default_factory=dict)
    last_result_set: list[dict] = field(default_factory=list)


class WorkflowMemoryStore:
    def __init__(self) -> None:
        self._states: dict[str, WorkflowState] = {}
        self._lock = RLock()

    def get(self, session_id: str) -> WorkflowState:
        with self._lock:
            state = self._states.get(session_id)
            if state is None:
                state = WorkflowState(session_id=session_id)
                self._states[session_id] = state
            return state

    def clear(self, session_id: str) -> None:
        with self._lock:
            self._states.pop(session_id, None)
