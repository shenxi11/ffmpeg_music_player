from __future__ import annotations

import asyncio
import json
import uuid
from contextlib import suppress

import uvicorn
from fastapi import FastAPI, HTTPException, Query, WebSocket, WebSocketDisconnect
from pydantic import ValidationError

from .chat_agent import ChatAgent, StreamInterruptedError
from .capability_catalog import (
    CAPABILITY_CATALOG_VERSION,
    capability_architecture_summary,
    capability_catalog_summary,
)
from .config import Settings
from .control_runtime import ControlRuntime
from .runtime_actions import (
    AssistantTextRuntimeAction,
    ChatRuntimeAction,
    ClarificationRuntimeAction,
    FinalResultRuntimeAction,
    PlanApprovalRuntimeAction,
    ProgressRuntimeAction,
    RuntimeActionBatch,
    ScriptCancellationRuntimeAction,
    ScriptDryRunRuntimeAction,
    ScriptExecutionRuntimeAction,
    ScriptValidationRuntimeAction,
    ToolCallRuntimeAction,
)
from .schemas import (
    ApprovalRequestMessage,
    ApprovalResponseMessage,
    AssistantChunkMessage,
    AssistantFinalMessage,
    AssistantStartMessage,
    CancelScriptMessage,
    ClarificationRequestMessage,
    CreateSessionRequest,
    DryRunScriptMessage,
    ErrorMessage,
    ExecuteScriptMessage,
    FinalResultMessage,
    HealthResponse,
    HostSnapshotMessage,
    PlanPreviewMessage,
    PlanEventsResponse,
    ProgressMessage,
    ScriptCancellationResultMessage,
    ScriptDryRunResultMessage,
    ScriptExecutionResultMessage,
    ScriptExecutionStartedMessage,
    ScriptStepEventMessage,
    ScriptValidationResultMessage,
    SessionListResponse,
    SessionEventsResponse,
    SessionMessagesResponse,
    SessionReadyMessage,
    SessionSummaryResponse,
    StoredEventResponse,
    StoredMessageResponse,
    ToolCallMessage,
    ToolResultMessage,
    UpdateSessionRequest,
    UserMessage,
    ValidateScriptMessage,
)
from .session_store import SessionSummary, SQLiteSessionStore, StoredEvent
from .workflow_memory import PendingScriptDryRun, PendingScriptExecution, PendingScriptValidation, WorkflowMemoryStore


PROTOCOL_VERSION = "1.6"
BASE_CAPABILITIES = ["chat", "streaming", "sessions", "storage"]
OPTIONAL_CAPABILITIES = ["tools", "plans", "approval", "audit", "scripts"]


def create_app(settings: Settings | None = None, chat_agent: ChatAgent | None = None) -> FastAPI:
    resolved_settings = settings or Settings()
    store = SQLiteSessionStore(
        db_path=resolved_settings.agent_storage_path,
        max_history_messages=resolved_settings.agent_max_history_messages,
    )
    resolved_chat_agent = chat_agent
    if resolved_chat_agent is not None:
        store = resolved_chat_agent.session_store

    music_runtime = ControlRuntime(
        settings=resolved_settings,
        session_store=store,
        memory_store=WorkflowMemoryStore(),
    )

    app = FastAPI(title="Music Agent Backend", version="0.3.0")
    app.state.settings = resolved_settings
    app.state.session_store = store
    app.state.chat_agent = resolved_chat_agent
    app.state.music_runtime = music_runtime

    @app.get("/healthz")
    async def healthz() -> dict:
        payload = HealthResponse(
            status="ok" if resolved_settings.is_local_model_configured() else "degraded",
            modelConfigured=resolved_settings.is_local_model_configured(),
            missingConfig=resolved_settings.missing_local_model_config(),
            localModelBaseUrl=resolved_settings.local_model_base_url,
            localModelName=resolved_settings.local_model_name,
            remoteModelEnabled=resolved_settings.remote_model_enabled,
            remoteModelBaseUrl=resolved_settings.effective_remote_base_url,
            remoteModelName=resolved_settings.effective_remote_model,
            defaultMode=resolved_settings.agent_default_mode,
            openaiBaseUrl=resolved_settings.effective_remote_base_url,
            openaiModel=resolved_settings.effective_remote_model,
            openaiWireApi=resolved_settings.openai_wire_api,
            sessionHistoryLimit=resolved_settings.agent_max_history_messages,
            storagePath=resolved_settings.agent_storage_path,
            protocolVersion=PROTOCOL_VERSION,
            capabilities=_capabilities_for_runtime(music_runtime),
            toolModeEnabled=music_runtime is not None,
            auditEnabled=True,
            directWriteActionsEnabled=resolved_settings.agent_allow_direct_write_actions,
            capabilityCatalogVersion=CAPABILITY_CATALOG_VERSION,
            capabilityExecutionModel=capability_architecture_summary(),
            worldStateEnabled=music_runtime is not None,
        )
        return payload.model_dump(by_alias=True)

    @app.get("/capabilities")
    async def get_capabilities() -> dict:
        return capability_catalog_summary()

    @app.get("/sessions")
    async def list_sessions(query: str | None = Query(default=None), limit: int = Query(default=100, ge=1, le=500)) -> dict:
        items = [_session_summary_response(item) for item in store.list_sessions(query=query, limit=limit)]
        return SessionListResponse(items=items).model_dump(by_alias=True)

    @app.post("/sessions")
    async def create_session(payload: CreateSessionRequest | None = None) -> dict:
        session = store.create_session(title=payload.title if payload else None)
        return _session_summary_response(session).model_dump(by_alias=True)

    @app.get("/sessions/{session_id}")
    async def get_session(session_id: str) -> dict:
        session = store.get_session(session_id)
        if session is None:
            raise HTTPException(status_code=404, detail="session not found")
        return _session_summary_response(session).model_dump(by_alias=True)

    @app.patch("/sessions/{session_id}")
    async def update_session(session_id: str, payload: UpdateSessionRequest) -> dict:
        session = store.update_session_title(session_id, payload.title)
        if session is None:
            raise HTTPException(status_code=404, detail="session not found")
        return _session_summary_response(session).model_dump(by_alias=True)

    @app.delete("/sessions/{session_id}")
    async def delete_session(session_id: str) -> dict:
        deleted = store.delete_session(session_id)
        if not deleted:
            raise HTTPException(status_code=404, detail="session not found")
        return {"ok": True, "sessionId": session_id}

    @app.get("/sessions/{session_id}/messages")
    async def get_session_messages(session_id: str) -> dict:
        session = store.get_session(session_id)
        if session is None:
            raise HTTPException(status_code=404, detail="session not found")
        items = [
            StoredMessageResponse(
                messageId=record.message_id,
                sessionId=record.session_id,
                role=record.role,
                content=record.content,
                createdAt=record.created_at,
            )
            for record in store.get_message_records(session_id)
        ]
        payload = SessionMessagesResponse(session=_session_summary_response(session), items=items)
        return payload.model_dump(by_alias=True)

    @app.get("/sessions/{session_id}/events")
    async def get_session_events(session_id: str, limit: int = Query(default=200, ge=1, le=1000)) -> dict:
        session = store.get_session(session_id)
        if session is None:
            raise HTTPException(status_code=404, detail="session not found")
        items = [_stored_event_response(record) for record in store.get_event_records(session_id, limit=limit)]
        payload = SessionEventsResponse(session=_session_summary_response(session), items=items)
        return payload.model_dump(by_alias=True)

    @app.get("/plans/{plan_id}/events")
    async def get_plan_events(plan_id: str, limit: int = Query(default=200, ge=1, le=1000)) -> dict:
        items = [_stored_event_response(record) for record in store.get_plan_event_records(plan_id, limit=limit)]
        payload = PlanEventsResponse(planId=plan_id, items=items)
        return payload.model_dump(by_alias=True)

    @app.websocket("/ws/chat")
    async def chat_socket(websocket: WebSocket) -> None:
        session_id = websocket.query_params.get("session_id") or str(uuid.uuid4())
        session = store.ensure_session(session_id)
        await websocket.accept()
        ready_message = SessionReadyMessage(
            sessionId=session.session_id,
            capabilities=_capabilities_for_runtime(app.state.music_runtime),
        ).model_dump(by_alias=True)
        ready_message["title"] = session.title
        await websocket.send_json(ready_message)

        while True:
            try:
                raw_message = await _receive_ws_message(websocket, session_id, app.state.music_runtime, resolved_settings)
            except WebSocketDisconnect:
                break

            if raw_message is None:
                continue

            payload, payload_error = _parse_json_object(raw_message)
            if payload_error is not None:
                await _send_error(websocket, session_id, None, payload_error["code"], payload_error["message"])
                continue

            message_type = payload.get("type")
            if message_type == "user_message":
                await _handle_user_message(app, websocket, session_id, payload)
                continue
            if message_type == "host_snapshot":
                await _handle_host_snapshot(app, websocket, session_id, payload)
                continue
            if message_type == "tool_result":
                await _handle_tool_result(app, websocket, session_id, payload)
                continue
            if message_type == "approval_response":
                await _handle_approval_response(app, websocket, session_id, payload)
                continue
            if message_type == "script_validation_result":
                await _handle_script_validation_result(app, websocket, session_id, payload)
                continue
            if message_type == "script_dry_run_result":
                await _handle_script_dry_run_result(app, websocket, session_id, payload)
                continue
            if message_type == "script_execution_started":
                await _handle_script_execution_started(app, websocket, session_id, payload)
                continue
            if message_type == "script_step_event":
                await _handle_script_step_event(app, websocket, session_id, payload)
                continue
            if message_type == "script_execution_result":
                await _handle_script_execution_result(app, websocket, session_id, payload)
                continue
            if message_type == "script_cancellation_result":
                await _handle_script_cancellation_result(app, websocket, session_id, payload)
                continue

            await _send_error(
                websocket,
                session_id,
                payload.get("requestId"),
                "unsupported_message_type",
                "supported message types are user_message, host_snapshot, tool_result, approval_response, script_dry_run_result, script_validation_result, script_execution_started, script_step_event, script_execution_result, and script_cancellation_result",
            )

        with suppress(Exception):
            await websocket.close()

    return app


async def _receive_ws_message(
    websocket: WebSocket,
    session_id: str,
    music_runtime: object | None,
    settings: Settings,
) -> str | None:
    if music_runtime is None:
        return await websocket.receive_text()

    state = music_runtime.memory_store.get(session_id)
    if state.pending_tool_call is None and state.pending_script_dry_run is None and state.pending_script_validation is None and state.pending_script_execution is None:
        return await websocket.receive_text()

    try:
        return await asyncio.wait_for(websocket.receive_text(), timeout=settings.agent_tool_timeout_seconds)
    except asyncio.TimeoutError:
        pending = music_runtime.pending_tool_timed_out(session_id, settings.agent_tool_timeout_seconds)
        if pending is not None:
            await _send_error(
                websocket,
                session_id,
                pending.request_id,
                "tool_result_timeout",
                f"等待工具 {pending.tool} 的结果超时",
            )
            return None
        pending_script = music_runtime.pending_script_timed_out(session_id, settings.agent_tool_timeout_seconds)
        if pending_script is not None:
            await _send_error(
                websocket,
                session_id,
                pending_script.request_id,
                "script_result_timeout",
                "等待客户端脚本执行结果超时",
            )
        return None


def _parse_json_object(raw_message: str) -> tuple[dict, dict | None]:
    try:
        payload = json.loads(raw_message)
    except json.JSONDecodeError:
        return {}, {"code": "invalid_json", "message": "message must be valid JSON"}

    if not isinstance(payload, dict):
        return {}, {"code": "invalid_message", "message": "message must be a JSON object"}

    return payload, None


def _validation_error_summary(prefix: str, exc: ValidationError) -> str:
    errors = exc.errors()
    if not errors:
        return prefix
    first = errors[0]
    location = ".".join(str(part) for part in first.get("loc") or [])
    message = str(first.get("msg") or "校验失败").strip()
    if location:
        return f"{prefix}: {location} {message}"
    return f"{prefix}: {message}"


async def _handle_user_message(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    try:
        message = UserMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, payload.get("requestId"), "invalid_message", "message format is invalid")
        return

    if not isinstance(message.content, str) or not message.content.strip():
        await _send_error(websocket, session_id, message.request_id, "invalid_message", "content must be a non-empty string")
        return

    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, message.request_id, "model_not_configured", "model configuration is incomplete")
        return

    try:
        action = await runtime.handle_user_message(session_id, message.request_id, message.content.strip())
        await _dispatch_runtime_action(app, websocket, session_id, message.content.strip(), action, message.request_id)
    except Exception as exc:
        await _send_error(websocket, session_id, message.request_id, "runtime_error", str(exc))


async def _handle_host_snapshot(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        return

    try:
        message = HostSnapshotMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "host_snapshot format is invalid")
        return

    try:
        await runtime.handle_host_snapshot(session_id, message.model_dump(by_alias=True))
    except ValidationError as exc:
        await _send_error(
            websocket,
            session_id,
            None,
            "host_snapshot_validation_error",
            _validation_error_summary("host_snapshot 校验失败", exc),
        )
    except Exception as exc:
        await _send_error(websocket, session_id, None, "runtime_error", str(exc))


async def _handle_tool_result(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "tool_result is not supported before tool mode is enabled")
        return

    try:
        message = ToolResultMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "tool_result format is invalid")
        return

    if not message.ok and message.error is None:
        await _send_error(websocket, session_id, None, "invalid_message", "tool_result.error is required when ok is false")
        return

    try:
        before_state = runtime.memory_store.get(session_id)
        observation_count_before = len(before_state.action_observation_history)
        completed_before = set(before_state.completed_action_candidate_ids)
        _append_audit_event(
            app,
            session_id,
            "tool_result",
            message.model_dump(by_alias=True),
            plan_id=_current_plan_id(app.state.music_runtime, session_id),
        )
        action = await runtime.handle_tool_result(session_id, message.model_dump(by_alias=True))
        _append_candidate_observation_events(
            app,
            session_id,
            runtime,
            observation_count_before,
            completed_before,
        )
        await _dispatch_runtime_action(app, websocket, session_id, None, action, None)
    except ValueError as exc:
        await _send_error(websocket, session_id, None, "invalid_tool_result", str(exc))
    except Exception as exc:
        await _send_error(websocket, session_id, None, "runtime_error", str(exc))


async def _handle_approval_response(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "approval_response is not supported before tool mode is enabled")
        return

    try:
        message = ApprovalResponseMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "approval_response format is invalid")
        return

    try:
        _append_audit_event(
            app,
            session_id,
            "approval_response",
            message.model_dump(by_alias=True),
            plan_id=message.plan_id,
        )
        action = await runtime.handle_approval_response(session_id, message.approved)
        await _dispatch_runtime_action(app, websocket, session_id, None, action, None)
    except ValueError as exc:
        await _send_error(websocket, session_id, None, "invalid_approval_response", str(exc))
    except Exception as exc:
        await _send_error(websocket, session_id, None, "runtime_error", str(exc))


async def _handle_script_dry_run_result(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "script_dry_run_result is not supported before tool mode is enabled")
        return

    try:
        message = ScriptDryRunResultMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "script_dry_run_result format is invalid")
        return

    state = runtime.memory_store.get(session_id)
    pending = state.pending_script_dry_run
    if pending is None or pending.request_id != message.request_id:
        await _send_error(websocket, session_id, message.request_id, "invalid_script_result", "no pending script dry run matches this requestId")
        return

    observation = {
        "source": "script",
        "event": "script_dry_run_result",
        "ok": message.ok,
        "result": message.result,
        "error": message.error.model_dump() if message.error is not None else None,
    }
    state.last_tool_observation = observation
    state.action_observation_history.append(observation)
    if len(state.action_observation_history) > 20:
        del state.action_observation_history[:-20]
    _append_audit_event(app, session_id, "script_dry_run_result", message.model_dump(by_alias=True))

    if not message.ok:
        state.pending_script_dry_run = None
        error = message.error.model_dump() if message.error is not None else {"message": "客户端脚本预演失败"}
        await _send_error(
            websocket,
            session_id,
            message.request_id,
            error.get("code", "script_dry_run_failed"),
            error.get("message", "客户端脚本预演失败"),
        )
        return

    result = message.result or {}
    state.pending_script_dry_run = None
    requires_approval = bool(result.get("requiresApproval", False))
    risk_level = str(result.get("riskLevel") or "low")
    auto_executable = bool(result.get("autoExecutable", True))
    if requires_approval or risk_level == "high" or not auto_executable:
        state.workflow_mode = "chat"
        state.goal_status = "waiting_user"
        domains = ", ".join(result.get("domains", []) or [])
        risk_hint = f"风险级别：{risk_level}"
        domain_hint = f"；涉及域：{domains}" if domains else ""
        await _send_assistant_text(
            websocket,
            session_id,
            message.request_id,
            f"该脚本预演后不适合直接执行。{risk_hint}{domain_hint}。当前后端先停止自动执行，后续可继续接审批或重规划。",
        )
        return

    execute_action = ScriptValidationRuntimeAction(
        script=pending.script,
        request_id=pending.request_id,
        persist_user_message=pending.user_message,
    )
    state.pending_script_validation = PendingScriptValidation(
        request_id=pending.request_id,
        user_message=pending.user_message,
        script=pending.script,
    )
    await _dispatch_runtime_action(app, websocket, session_id, pending.user_message, execute_action, pending.request_id)


async def _handle_script_validation_result(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "script_validation_result is not supported before tool mode is enabled")
        return

    try:
        message = ScriptValidationResultMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "script_validation_result format is invalid")
        return

    state = runtime.memory_store.get(session_id)
    pending = state.pending_script_validation
    if pending is None or pending.request_id != message.request_id:
        await _send_error(websocket, session_id, message.request_id, "invalid_script_result", "no pending script validation matches this requestId")
        return

    _append_audit_event(app, session_id, "script_validation_result", message.model_dump(by_alias=True))

    if not message.ok:
        state.pending_script_validation = None
        error = message.error.model_dump() if message.error is not None else {"message": "客户端脚本校验失败"}
        await _send_error(
            websocket,
            session_id,
            message.request_id,
            error.get("code", "script_validation_failed"),
            error.get("message", "客户端脚本校验失败"),
        )
        return

    state.pending_script_validation = None
    execute_action = ScriptExecutionRuntimeAction(
        script=pending.script,
        request_id=pending.request_id,
        persist_user_message=pending.user_message,
    )
    await _dispatch_runtime_action(app, websocket, session_id, pending.user_message, execute_action, pending.request_id)


async def _handle_script_execution_started(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "script_execution_started is not supported before tool mode is enabled")
        return

    try:
        message = ScriptExecutionStartedMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "script_execution_started format is invalid")
        return

    state = runtime.memory_store.get(session_id)
    pending = state.pending_script_execution
    if pending is None or pending.request_id != message.request_id:
        await _send_error(websocket, session_id, message.request_id, "invalid_script_result", "no pending script execution matches this requestId")
        return

    pending.execution_id = message.execution_id
    pending.started_summary = dict(message.summary)
    state.last_tool_observation = {
        "source": "script",
        "event": "script_execution_started",
        "executionId": message.execution_id,
        "summary": dict(message.summary),
    }
    state.action_observation_history.append(state.last_tool_observation)
    if len(state.action_observation_history) > 20:
        del state.action_observation_history[:-20]
    _append_audit_event(app, session_id, "script_execution_started", message.model_dump(by_alias=True))


async def _handle_script_step_event(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "script_step_event is not supported before tool mode is enabled")
        return

    try:
        message = ScriptStepEventMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "script_step_event format is invalid")
        return

    state = runtime.memory_store.get(session_id)
    pending = state.pending_script_execution
    if pending is None or pending.request_id != message.request_id or (pending.execution_id is not None and pending.execution_id != message.execution_id):
        await _send_error(websocket, session_id, message.request_id, "invalid_script_result", "no pending script execution matches this requestId")
        return

    state.last_tool_observation = message.payload
    state.action_observation_history.append(
        {
            "source": "script",
            "executionId": message.execution_id,
            "stepIndex": message.step_index,
            "status": message.status,
            "payload": message.payload,
        }
    )
    if len(state.action_observation_history) > 20:
        del state.action_observation_history[:-20]
    _append_audit_event(app, session_id, "script_step_event", message.model_dump(by_alias=True))


async def _handle_script_execution_result(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "script_execution_result is not supported before tool mode is enabled")
        return

    try:
        message = ScriptExecutionResultMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "script_execution_result format is invalid")
        return

    state = runtime.memory_store.get(session_id)
    pending = state.pending_script_execution
    if pending is None or pending.request_id != message.request_id or (pending.execution_id is not None and message.execution_id is not None and pending.execution_id != message.execution_id):
        await _send_error(websocket, session_id, message.request_id, "invalid_script_result", "no pending script execution matches this requestId")
        return

    state.pending_script_execution = None
    state.workflow_mode = "chat"
    report = message.report or {}
    execution_status = str(report.get("status") or ("succeeded" if message.ok else "failed"))
    state.last_tool_observation = {
        "source": "script",
        "event": "script_execution_result",
        "executionId": message.execution_id,
        "status": execution_status,
        "startedAt": report.get("startedAt"),
        "finishedAt": report.get("finishedAt"),
        "durationMs": report.get("durationMs"),
        "ok": message.ok,
    }
    state.action_observation_history.append(state.last_tool_observation)
    if len(state.action_observation_history) > 20:
        del state.action_observation_history[:-20]
    _append_audit_event(app, session_id, "script_execution_result", message.model_dump(by_alias=True))

    if message.ok:
        state.goal_status = "completed"
        _integrate_script_report(runtime, session_id, report)
        summary = _summarize_script_report(runtime, report)
        runtime.persist_turn(session_id, pending.user_message, summary)
        await _send_assistant_text(websocket, session_id, message.request_id, summary)
        return

    if message.error is not None and message.error.code == "script_cancelled" and pending.cancellation_acknowledged:
        state.goal_status = "cancelled"
        return

    state.goal_status = "failed"
    error = message.error.model_dump() if message.error is not None else {"message": "客户端脚本执行失败"}
    await _send_error(
        websocket,
        session_id,
        message.request_id,
        error.get("code", "script_execution_failed"),
        error.get("message", "客户端脚本执行失败"),
    )


async def _handle_script_cancellation_result(app: FastAPI, websocket: WebSocket, session_id: str, payload: dict) -> None:
    runtime = app.state.music_runtime
    if runtime is None:
        await _send_error(websocket, session_id, None, "unsupported_message_type", "script_cancellation_result is not supported before tool mode is enabled")
        return

    try:
        message = ScriptCancellationResultMessage.model_validate(payload)
    except ValidationError:
        await _send_error(websocket, session_id, None, "invalid_message", "script_cancellation_result format is invalid")
        return

    state = runtime.memory_store.get(session_id)
    pending = state.pending_script_execution
    if pending is None or pending.execution_id != message.execution_id or pending.cancellation_request_id != message.request_id:
        await _send_error(websocket, session_id, message.request_id, "invalid_script_result", "no pending script cancellation matches this requestId")
        return

    observation = {
        "source": "script",
        "event": "script_cancellation_result",
        "executionId": message.execution_id,
        "ok": message.ok,
        "result": message.result,
        "error": message.error.model_dump() if message.error is not None else None,
    }
    state.last_tool_observation = observation
    state.action_observation_history.append(observation)
    if len(state.action_observation_history) > 20:
        del state.action_observation_history[:-20]
    _append_audit_event(app, session_id, "script_cancellation_result", message.model_dump(by_alias=True))

    if not message.ok:
        pending.cancellation_request_id = None
        pending.cancellation_user_message = None
        pending.cancellation_reason = None
        error = message.error.model_dump() if message.error is not None else {"message": "客户端取消脚本执行失败"}
        await _send_error(
            websocket,
            session_id,
            message.request_id,
            error.get("code", "script_cancellation_failed"),
            error.get("message", "客户端取消脚本执行失败"),
        )
        return

    pending.cancellation_acknowledged = True
    cancellation_message = "已请求取消当前执行，已完成的步骤不会回滚。"
    runtime.persist_turn(session_id, pending.cancellation_user_message or "取消当前执行", cancellation_message)
    await _send_assistant_text(websocket, session_id, message.request_id, cancellation_message)


async def _dispatch_runtime_action(
    app: FastAPI,
    websocket: WebSocket,
    session_id: str,
    user_message: str | None,
    action,
    request_id: str | None,
) -> None:
    runtime = app.state.music_runtime

    if isinstance(action, RuntimeActionBatch):
        for child_action in action.actions:
            await _dispatch_runtime_action(app, websocket, session_id, user_message, child_action, request_id)
        return

    if isinstance(action, ChatRuntimeAction):
        if app.state.chat_agent is None or user_message is None:
            await _send_error(websocket, session_id, request_id, "model_not_configured", "model configuration is incomplete")
            return
        await _stream_chat_response(app.state.chat_agent, websocket, session_id, request_id, user_message)
        return

    if isinstance(action, ScriptDryRunRuntimeAction):
        resolved_request_id = action.request_id or str(uuid.uuid4())
        state = runtime.memory_store.get(session_id)
        if state.pending_script_dry_run is not None and state.pending_script_dry_run.request_id is None:
            state.pending_script_dry_run.request_id = resolved_request_id
        if action.audit_event_type:
            _append_audit_event(
                app,
                session_id,
                action.audit_event_type,
                action.audit_payload or {},
                plan_id=_current_plan_id(runtime, session_id),
            )
        payload = DryRunScriptMessage(requestId=resolved_request_id, script=action.script)
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "dry_run_script", serialized, plan_id=_current_plan_id(runtime, session_id))
        await websocket.send_json(serialized)
        return

    if isinstance(action, ScriptValidationRuntimeAction):
        resolved_request_id = action.request_id or str(uuid.uuid4())
        state = runtime.memory_store.get(session_id)
        if state.pending_script_validation is not None and state.pending_script_validation.request_id is None:
            state.pending_script_validation.request_id = resolved_request_id
        payload = ValidateScriptMessage(requestId=resolved_request_id, script=action.script)
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "validate_script", serialized, plan_id=_current_plan_id(runtime, session_id))
        await websocket.send_json(serialized)
        return

    if isinstance(action, ScriptExecutionRuntimeAction):
        state = runtime.memory_store.get(session_id)
        resolved_request_id = action.request_id or str(uuid.uuid4())
        if state.pending_script_execution is None:
            state.pending_script_execution = PendingScriptExecution(
                request_id=resolved_request_id,
                user_message=action.persist_user_message,
                script=action.script,
            )
        elif state.pending_script_execution.request_id is None:
            state.pending_script_execution.request_id = resolved_request_id
        payload = ExecuteScriptMessage(requestId=resolved_request_id, script=action.script)
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "execute_script", serialized, plan_id=_current_plan_id(runtime, session_id))
        await websocket.send_json(serialized)
        return

    if isinstance(action, ScriptCancellationRuntimeAction):
        state = runtime.memory_store.get(session_id)
        resolved_request_id = action.request_id or str(uuid.uuid4())
        if state.pending_script_execution is not None and state.pending_script_execution.cancellation_request_id is None:
            state.pending_script_execution.cancellation_request_id = resolved_request_id
        payload = CancelScriptMessage(
            requestId=resolved_request_id,
            executionId=action.execution_id,
            reason=action.reason,
        )
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "cancel_script", serialized, plan_id=_current_plan_id(runtime, session_id))
        await websocket.send_json(serialized)
        return

    if isinstance(action, ToolCallRuntimeAction):
        _append_active_candidate_event(app, session_id, runtime)
        payload = ToolCallMessage(toolCallId=action.tool_call_id, sessionId=session_id, tool=action.tool, args=action.args)
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(
            app,
            session_id,
            "tool_call",
            serialized,
            plan_id=_current_plan_id(runtime, session_id),
        )
        await websocket.send_json(serialized)
        return

    if isinstance(action, PlanApprovalRuntimeAction):
        preview_payload = PlanPreviewMessage(
            planId=action.plan_id,
            sessionId=action.session_id,
            summary=action.summary,
            riskLevel=action.risk_level,
            steps=action.steps,
        )
        preview_serialized = preview_payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "plan_preview", preview_serialized, plan_id=action.plan_id)
        await websocket.send_json(preview_serialized)
        approval_payload = ApprovalRequestMessage(
            planId=action.plan_id,
            sessionId=action.session_id,
            message=action.approval_message,
            riskLevel=action.risk_level,
        )
        approval_serialized = approval_payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "approval_request", approval_serialized, plan_id=action.plan_id)
        await websocket.send_json(approval_serialized)
        return

    if isinstance(action, ClarificationRuntimeAction):
        runtime.persist_turn(session_id, action.persist_user_message, action.question)
        payload = ClarificationRequestMessage(
            sessionId=session_id,
            requestId=action.request_id,
            question=action.question,
            options=action.options,
        )
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "clarification_request", serialized)
        await websocket.send_json(serialized)
        return

    if isinstance(action, AssistantTextRuntimeAction):
        if action.audit_event_type:
            _append_audit_event(
                app,
                session_id,
                action.audit_event_type,
                action.audit_payload or {},
                plan_id=_current_plan_id(runtime, session_id),
            )
        runtime.persist_turn(session_id, action.persist_user_message, action.text)
        await _send_assistant_text(websocket, session_id, action.request_id, action.text)
        return

    if isinstance(action, ProgressRuntimeAction):
        payload = ProgressMessage(planId=action.plan_id, stepId=action.step_id, message=action.message)
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "progress", serialized, plan_id=action.plan_id)
        await websocket.send_json(serialized)
        return

    if isinstance(action, FinalResultRuntimeAction):
        payload = FinalResultMessage(
            planId=action.plan_id,
            sessionId=action.session_id,
            ok=action.ok,
            summary=action.summary,
        )
        serialized = payload.model_dump(by_alias=True)
        _append_audit_event(app, session_id, "final_result", serialized, plan_id=action.plan_id)
        await websocket.send_json(serialized)
        runtime.persist_turn(session_id, action.persist_user_message, action.summary)
        await _send_assistant_text(websocket, session_id, action.request_id, action.summary)
        return

    raise RuntimeError(f"unsupported runtime action: {action!r}")


async def _stream_chat_response(
    chat_agent: ChatAgent,
    websocket: WebSocket,
    session_id: str,
    request_id: str | None,
    user_message: str,
) -> None:
    start_message = AssistantStartMessage(sessionId=session_id, requestId=request_id)
    await websocket.send_json(start_message.model_dump(by_alias=True))

    response_chunks: list[str] = []
    try:
        async for delta in chat_agent.stream_response(session_id, user_message):
            response_chunks.append(delta)
            chunk_message = AssistantChunkMessage(sessionId=session_id, requestId=request_id, delta=delta)
            await websocket.send_json(chunk_message.model_dump(by_alias=True))
    except StreamInterruptedError as exc:
        await _send_error(websocket, session_id, request_id, "stream_interrupted", str(exc))
        return
    except Exception as exc:
        await _send_error(websocket, session_id, request_id, "model_error", str(exc))
        return

    response_text = "".join(response_chunks).strip()
    final_message = AssistantFinalMessage(sessionId=session_id, requestId=request_id, content=response_text)
    await websocket.send_json(final_message.model_dump(by_alias=True))


async def _send_assistant_text(
    websocket: WebSocket,
    session_id: str,
    request_id: str | None,
    text: str,
) -> None:
    start_message = AssistantStartMessage(sessionId=session_id, requestId=request_id)
    await websocket.send_json(start_message.model_dump(by_alias=True))
    if text:
        chunk_message = AssistantChunkMessage(sessionId=session_id, requestId=request_id, delta=text)
        await websocket.send_json(chunk_message.model_dump(by_alias=True))
    final_message = AssistantFinalMessage(sessionId=session_id, requestId=request_id, content=text)
    await websocket.send_json(final_message.model_dump(by_alias=True))


async def _send_error(
    websocket: WebSocket,
    session_id: str | None,
    request_id: str | None,
    code: str,
    message: str,
) -> None:
    error_payload = ErrorMessage(sessionId=session_id, requestId=request_id, code=code, message=message)
    await websocket.send_json(error_payload.model_dump(by_alias=True))


def _capabilities_for_runtime(music_runtime: object | None) -> list[str]:
    capabilities = list(BASE_CAPABILITIES)
    if music_runtime is not None:
        capabilities.extend(OPTIONAL_CAPABILITIES)
    return capabilities


def _append_audit_event(
    app: FastAPI,
    session_id: str,
    event_type: str,
    payload: dict,
    plan_id: str | None = None,
) -> None:
    app.state.session_store.append_event(
        session_id=session_id,
        event_type=event_type,
        payload=payload,
        plan_id=plan_id,
    )


def _current_plan_id(music_runtime: object | None, session_id: str) -> str | None:
    if music_runtime is None:
        return None
    state = music_runtime.memory_store.get(session_id)
    if state.active_plan is None:
        return None
    return state.active_plan.plan_id


def _append_active_candidate_event(app: FastAPI, session_id: str, music_runtime: object | None) -> None:
    if music_runtime is None:
        return
    state = music_runtime.memory_store.get(session_id)
    candidate = getattr(state, "active_action_candidate", None)
    if candidate is None:
        return
    payload = dict(candidate)
    payload["completedStepIds"] = list(state.completed_action_candidate_ids)
    _append_audit_event(app, session_id, "action_candidate_selected", payload, plan_id=_current_plan_id(music_runtime, session_id))


def _append_candidate_observation_events(
    app: FastAPI,
    session_id: str,
    music_runtime: object | None,
    observation_count_before: int,
    completed_before: set[str],
) -> None:
    if music_runtime is None:
        return
    state = music_runtime.memory_store.get(session_id)
    new_observations = getattr(state, "action_observation_history", [])[observation_count_before:]
    completed_after = set(getattr(state, "completed_action_candidate_ids", []))
    new_completed = completed_after - completed_before
    for observation in new_observations:
        payload = dict(observation)
        step_id = str(payload.get("stepId") or "")
        payload["completed"] = step_id in new_completed
        _append_audit_event(
            app,
            session_id,
            "action_candidate_observed",
            payload,
            plan_id=_current_plan_id(music_runtime, session_id),
        )


def _integrate_script_report(music_runtime, session_id: str, report: dict) -> None:
    state = music_runtime.memory_store.get(session_id)
    for step in report.get("steps", []):
        if not isinstance(step, dict):
            continue
        if not step.get("ok", False):
            continue
        action = str(step.get("action") or "").strip()
        result = step.get("result")
        if not action or not isinstance(result, dict):
            continue
        if hasattr(music_runtime, "_integrate_tool_result"):
            music_runtime._integrate_tool_result(state, action, {"ok": True, "result": result})


def _summarize_script_report(music_runtime, report: dict) -> str:
    steps = report.get("steps", [])
    if isinstance(steps, list):
        for step in reversed(steps):
            if not isinstance(step, dict) or not step.get("ok", False):
                continue
            action = str(step.get("action") or "").strip()
            result = step.get("result")
            if not isinstance(result, dict):
                continue
            if action == "stopPlayback":
                return "已停止播放。"
            if action == "playTrack":
                title = str((result.get("track") or {}).get("title") or result.get("title") or "目标歌曲").strip()
                return f"已开始播放《{title}》。"
            if action == "playPlaylist":
                name = str((result.get("playlist") or {}).get("name") or result.get("name") or "目标歌单").strip()
                return f"已开始播放歌单“{name}”。"
            if action == "getPlaylistTracks":
                playlist = result.get("playlist") or {}
                name = str(playlist.get("name") or "目标歌单").strip()
                count = len(list(result.get("items", [])))
                return f"已读取歌单“{name}”，共 {count} 首歌曲。"
            if action == "getCurrentTrack":
                title = str(result.get("title") or "当前歌曲").strip()
                if result.get("playing", False):
                    return f"当前正在播放《{title}》。"
                return "当前没有正在播放的歌曲。"
            if action == "getRecentTracks":
                count = len(list(result.get("items", [])))
                return f"已读取最近播放，共 {count} 首。"
    title = str(report.get("title") or "").strip()
    if title:
        return f"脚本执行完成：{title}"
    return "脚本执行完成。"


def main() -> None:
    settings = Settings()
    uvicorn.run(
        "music_agent.server:create_app",
        factory=True,
        host=settings.agent_host,
        port=settings.agent_port,
        reload=False,
    )


if __name__ == "__main__":
    main()


def _session_summary_response(session: SessionSummary) -> SessionSummaryResponse:
    return SessionSummaryResponse(
        sessionId=session.session_id,
        title=session.title,
        createdAt=session.created_at,
        updatedAt=session.updated_at,
        lastPreview=session.last_preview,
        messageCount=session.message_count,
    )


def _stored_event_response(event: StoredEvent) -> StoredEventResponse:
    return StoredEventResponse(
        eventId=event.event_id,
        sessionId=event.session_id,
        planId=event.plan_id,
        eventType=event.event_type,
        payload=event.payload,
        createdAt=event.created_at,
    )
