from __future__ import annotations

"""
模块名: runtime_actions
功能概述: 定义音乐 Agent 运行时在 transport 层之间传递的动作对象，解耦 runtime 主逻辑与 server 派发逻辑。
对外接口: ChatRuntimeAction、ToolCallRuntimeAction、Script*RuntimeAction、ClarificationRuntimeAction、AssistantTextRuntimeAction、ProgressRuntimeAction、FinalResultRuntimeAction、PlanApprovalRuntimeAction、RuntimeActionBatch
依赖关系: 无外部服务依赖；由 music_runtime、server、后续 observation/policy 模块共同使用
输入输出: 输入为运行时各阶段的结构化动作；输出为可被 server 派发的 dataclass 对象
异常与错误: 纯数据对象，不抛出运行时异常
维护说明: 后续若继续拆 execution / observation / planning 层，应优先复用本模块，而不是在各处重复定义动作类型
"""

from dataclasses import dataclass
from typing import Literal


@dataclass
class ChatRuntimeAction:
    kind: Literal["chat"] = "chat"


@dataclass
class ToolCallRuntimeAction:
    tool_call_id: str
    tool: str
    args: dict
    request_id: str | None
    kind: Literal["tool_call"] = "tool_call"


@dataclass
class ScriptValidationRuntimeAction:
    script: dict
    request_id: str | None
    persist_user_message: str
    kind: Literal["script_validation"] = "script_validation"


@dataclass
class ScriptDryRunRuntimeAction:
    script: dict
    request_id: str | None
    persist_user_message: str
    audit_event_type: str | None = None
    audit_payload: dict | None = None
    kind: Literal["script_dry_run"] = "script_dry_run"


@dataclass
class ScriptExecutionRuntimeAction:
    script: dict
    request_id: str | None
    persist_user_message: str
    kind: Literal["script_execution"] = "script_execution"


@dataclass
class ScriptCancellationRuntimeAction:
    execution_id: str
    request_id: str | None
    reason: str
    persist_user_message: str
    kind: Literal["script_cancellation"] = "script_cancellation"


@dataclass
class ClarificationRuntimeAction:
    question: str
    options: list[str]
    request_id: str | None
    persist_user_message: str
    kind: Literal["clarification"] = "clarification"


@dataclass
class AssistantTextRuntimeAction:
    text: str
    request_id: str | None
    persist_user_message: str
    audit_event_type: str | None = None
    audit_payload: dict | None = None
    kind: Literal["assistant_text"] = "assistant_text"


@dataclass
class ProgressRuntimeAction:
    plan_id: str
    step_id: str
    message: str
    kind: Literal["progress"] = "progress"


@dataclass
class FinalResultRuntimeAction:
    plan_id: str
    session_id: str
    ok: bool
    summary: str
    request_id: str | None
    persist_user_message: str
    kind: Literal["final_result"] = "final_result"


@dataclass
class PlanApprovalRuntimeAction:
    plan_id: str
    session_id: str
    summary: str
    risk_level: Literal["low", "medium", "high"]
    steps: list[dict]
    approval_message: str
    request_id: str | None
    kind: Literal["plan_approval"] = "plan_approval"


@dataclass
class RuntimeActionBatch:
    actions: list[object]
    kind: Literal["batch"] = "batch"
