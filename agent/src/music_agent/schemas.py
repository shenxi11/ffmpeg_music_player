from typing import Literal

from pydantic import BaseModel, ConfigDict, Field

RiskLevel = Literal["low", "medium", "high"]


class MessageEnvelope(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    type: str


class SessionReadyMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    session_id: str = Field(alias="sessionId")
    capabilities: list[str] = Field(default_factory=list)
    type: Literal["session_ready"] = "session_ready"


class UserMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["user_message"]
    content: str
    request_id: str | None = Field(default=None, alias="requestId")


class AssistantStartMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["assistant_start"] = "assistant_start"
    session_id: str = Field(alias="sessionId")
    request_id: str | None = Field(default=None, alias="requestId")


class AssistantChunkMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["assistant_chunk"] = "assistant_chunk"
    session_id: str = Field(alias="sessionId")
    request_id: str | None = Field(default=None, alias="requestId")
    delta: str


class AssistantFinalMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["assistant_final"] = "assistant_final"
    session_id: str = Field(alias="sessionId")
    request_id: str | None = Field(default=None, alias="requestId")
    content: str


class ErrorMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["error"] = "error"
    code: str
    message: str
    session_id: str | None = Field(default=None, alias="sessionId")
    request_id: str | None = Field(default=None, alias="requestId")


class ToolCallMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["tool_call"] = "tool_call"
    tool_call_id: str = Field(alias="toolCallId")
    session_id: str = Field(alias="sessionId")
    tool: str
    args: dict


class ToolResultError(BaseModel):
    code: str
    message: str
    retryable: bool = False


class ToolResultMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["tool_result"]
    tool_call_id: str = Field(alias="toolCallId")
    ok: bool
    result: dict | None = None
    error: ToolResultError | None = None


class ValidateScriptMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["validate_script"] = "validate_script"
    request_id: str = Field(alias="requestId")
    script: dict


class DryRunScriptMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["dry_run_script"] = "dry_run_script"
    request_id: str = Field(alias="requestId")
    script: dict


class ExecuteScriptMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["execute_script"] = "execute_script"
    request_id: str = Field(alias="requestId")
    script: dict


class CancelScriptMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["cancel_script"] = "cancel_script"
    request_id: str = Field(alias="requestId")
    execution_id: str = Field(alias="executionId")
    reason: str | None = None


class ScriptValidationResultMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["script_validation_result"]
    request_id: str = Field(alias="requestId")
    ok: bool
    result: dict | None = None
    error: ToolResultError | None = None


class ScriptDryRunResultMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["script_dry_run_result"]
    request_id: str = Field(alias="requestId")
    ok: bool
    result: dict | None = None
    error: ToolResultError | None = None


class ScriptExecutionStartedMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["script_execution_started"]
    request_id: str = Field(alias="requestId")
    execution_id: str = Field(alias="executionId")
    summary: dict = Field(default_factory=dict)


class ScriptStepEventMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["script_step_event"]
    request_id: str = Field(alias="requestId")
    execution_id: str = Field(alias="executionId")
    step_index: int = Field(alias="stepIndex")
    status: str
    payload: dict = Field(default_factory=dict)


class ScriptExecutionResultMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["script_execution_result"]
    request_id: str = Field(alias="requestId")
    execution_id: str | None = Field(default=None, alias="executionId")
    ok: bool
    report: dict | None = None
    error: ToolResultError | None = None


class ScriptCancellationResultMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["script_cancellation_result"]
    request_id: str = Field(alias="requestId")
    execution_id: str = Field(alias="executionId")
    ok: bool
    result: dict | None = None
    error: ToolResultError | None = None


class ClarificationRequestMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["clarification_request"] = "clarification_request"
    session_id: str = Field(alias="sessionId")
    request_id: str | None = Field(default=None, alias="requestId")
    question: str
    options: list[str] | None = None


class PlanPreviewMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["plan_preview"] = "plan_preview"
    plan_id: str = Field(alias="planId")
    session_id: str = Field(alias="sessionId")
    summary: str
    risk_level: RiskLevel = Field(alias="riskLevel")
    steps: list["PlanStepPreview"]


class PlanStepPreview(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    step_id: str = Field(alias="stepId")
    title: str
    status: Literal["pending", "waiting_tool", "succeeded", "failed", "skipped", "cancelled"] | None = None


class ApprovalRequestMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["approval_request"] = "approval_request"
    plan_id: str = Field(alias="planId")
    session_id: str = Field(alias="sessionId")
    message: str
    risk_level: RiskLevel = Field(alias="riskLevel")


class ApprovalResponseMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["approval_response"]
    plan_id: str = Field(alias="planId")
    approved: bool


class ProgressMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["progress"] = "progress"
    plan_id: str = Field(alias="planId")
    step_id: str = Field(alias="stepId")
    message: str


class FinalResultMessage(MessageEnvelope):
    model_config = ConfigDict(populate_by_name=True)

    type: Literal["final_result"] = "final_result"
    plan_id: str = Field(alias="planId")
    session_id: str = Field(alias="sessionId")
    ok: bool
    summary: str


class HealthResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    status: Literal["ok", "degraded"]
    model_configured: bool = Field(alias="modelConfigured")
    missing_config: list[str] = Field(alias="missingConfig")
    openai_base_url: str | None = Field(alias="openaiBaseUrl")
    openai_model: str | None = Field(alias="openaiModel")
    openai_wire_api: str = Field(alias="openaiWireApi")
    session_history_limit: int = Field(alias="sessionHistoryLimit")
    storage_path: str = Field(alias="storagePath")
    protocol_version: str = Field(alias="protocolVersion")
    capabilities: list[str]
    tool_mode_enabled: bool = Field(alias="toolModeEnabled")
    audit_enabled: bool = Field(alias="auditEnabled")
    direct_write_actions_enabled: bool = Field(alias="directWriteActionsEnabled")
    capability_catalog_version: str = Field(alias="capabilityCatalogVersion")
    capability_execution_model: dict = Field(alias="capabilityExecutionModel")
    world_state_enabled: bool = Field(alias="worldStateEnabled")


class SessionSummaryResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    session_id: str = Field(alias="sessionId")
    title: str
    created_at: str = Field(alias="createdAt")
    updated_at: str = Field(alias="updatedAt")
    last_preview: str = Field(alias="lastPreview")
    message_count: int = Field(alias="messageCount")


class SessionListResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    items: list[SessionSummaryResponse]


class CreateSessionRequest(BaseModel):
    title: str | None = None


class UpdateSessionRequest(BaseModel):
    title: str


class StoredMessageResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    message_id: str = Field(alias="messageId")
    session_id: str = Field(alias="sessionId")
    role: str
    content: str
    created_at: str = Field(alias="createdAt")


class StoredEventResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    event_id: str = Field(alias="eventId")
    session_id: str = Field(alias="sessionId")
    plan_id: str | None = Field(default=None, alias="planId")
    event_type: str = Field(alias="eventType")
    payload: dict
    created_at: str = Field(alias="createdAt")


class SessionMessagesResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    session: SessionSummaryResponse
    items: list[StoredMessageResponse]


class SessionEventsResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    session: SessionSummaryResponse
    items: list[StoredEventResponse]


class PlanEventsResponse(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    plan_id: str = Field(alias="planId")
    items: list[StoredEventResponse]
