from __future__ import annotations

"""
模块名: control_gateways
功能概述: 封装本地控制模型与显式远程助手模式的网关，统一处理 OpenAI 兼容接口调用与返回值解析。
对外接口: LocalModelGateway、RemoteAssistantGateway
依赖关系: openai.AsyncOpenAI、config、control_models、control_prompts
输入输出: 输入为用户消息、宿主快照与会话记忆；输出为 ControlIntent 或简短助手文本
异常与错误: 网络错误、JSON 解析错误向上抛出，由运行时转成用户可理解的限制或降级提示
维护说明: 本地模型默认假设对接 llama.cpp 的 OpenAI 兼容端点，避免启用自由 tool calling
"""

import json
import logging
from typing import Any

from openai import APIConnectionError, APIStatusError, APITimeoutError, AsyncOpenAI
from pydantic import ValidationError

from .capability_catalog import build_control_compiler_capability_projection
from .config import Settings
from .control_models import ControlIntent
from .control_prompts import (
    CONTROL_COMPILER_SYSTEM_PROMPT,
    REMOTE_ASSISTANT_SYSTEM_PROMPT,
    build_control_user_payload,
    estimate_control_payload_lengths,
)

logger = logging.getLogger(__name__)


class LocalModelGatewayError(RuntimeError):
    """封装本地控制模型调用失败，并提供稳定的错误分类。"""

    def __init__(self, kind: str, message: str, *, detail: str = "") -> None:
        super().__init__(message)
        self.kind = kind
        self.detail = detail


def _extract_text(content: str | list | None) -> str:
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        parts: list[str] = []
        for item in content:
            if isinstance(item, dict) and item.get("type") == "text":
                parts.append(str(item.get("text", "")))
                continue
            text_value = getattr(item, "text", None)
            if text_value:
                parts.append(str(text_value))
        return "".join(parts)
    return ""


def _extract_json_object(text: str) -> dict[str, Any]:
    stripped = text.strip()
    if not stripped:
        raise ValueError("model returned empty content")

    try:
        payload = json.loads(stripped)
        if isinstance(payload, dict):
            return payload
    except json.JSONDecodeError:
        pass

    start = stripped.find("{")
    end = stripped.rfind("}")
    if start < 0 or end <= start:
        raise ValueError("model response does not contain a JSON object")

    payload = json.loads(stripped[start : end + 1])
    if not isinstance(payload, dict):
        raise ValueError("model response JSON must be an object")
    return payload


class LocalModelGateway:
    def __init__(self, settings: Settings) -> None:
        self._settings = settings
        self._client = AsyncOpenAI(
            api_key="local-model",
            base_url=settings.local_model_base_url,
            timeout=settings.local_model_timeout_seconds,
        )

    async def compile_intent(
        self,
        user_message: str,
        host_context: dict[str, Any],
        capability_snapshot: dict[str, Any],
        memory_snapshot: dict[str, Any],
    ) -> ControlIntent:
        slim_snapshot = build_control_compiler_capability_projection(capability_snapshot)
        metrics = estimate_control_payload_lengths(
            user_message=user_message,
            host_context=host_context,
            capability_snapshot=slim_snapshot,
            memory_snapshot=memory_snapshot,
        )
        logger.info(
            "control_compile_request model=%s mode=control system_prompt_length=%s "
            "user_payload_length=%s total_prompt_length=%s capability_item_count=%s",
            self._settings.local_model_name,
            metrics["systemPromptLength"],
            metrics["userPayloadLength"],
            metrics["totalPromptLength"],
            metrics["capabilityItemCount"],
        )
        try:
            response = await self._client.chat.completions.create(
                model=self._settings.local_model_name,
                messages=[
                    {"role": "system", "content": CONTROL_COMPILER_SYSTEM_PROMPT},
                    {
                        "role": "user",
                        "content": build_control_user_payload(
                            user_message=user_message,
                            host_context=host_context,
                            capability_snapshot=slim_snapshot,
                            memory_snapshot=memory_snapshot,
                        ),
                    },
                ],
                temperature=0.1,
                response_format={"type": "json_object"},
            )
        except (APIConnectionError, APITimeoutError) as exc:
            raise LocalModelGatewayError(
                "model_unavailable",
                "local model request failed",
                detail=str(exc),
            ) from exc
        except APIStatusError as exc:
            response = getattr(exc, "response", None)
            body = getattr(exc, "body", None)
            response_text = ""
            if body is not None:
                response_text = str(body)
            elif response is not None:
                response_text = getattr(response, "text", "") or ""
            detail = f"{exc}; body={response_text}".strip()
            if "Context size has been exceeded" in detail:
                logger.warning(
                    "control_compile_context_overflow model=%s total_prompt_length=%s detail=%s",
                    self._settings.local_model_name,
                    metrics["totalPromptLength"],
                    detail,
                )
                raise LocalModelGatewayError(
                    "context_overflow",
                    "local model context size exceeded",
                    detail=detail,
                ) from exc
            raise LocalModelGatewayError(
                "model_unavailable",
                "local model request failed",
                detail=detail,
            ) from exc

        try:
            content = _extract_text(response.choices[0].message.content)
            return ControlIntent.model_validate(_extract_json_object(content))
        except (ValueError, ValidationError) as exc:
            raise LocalModelGatewayError(
                "format_error",
                "local model returned invalid control json",
                detail=str(exc),
            ) from exc


class RemoteAssistantGateway:
    def __init__(self, settings: Settings) -> None:
        self._settings = settings
        self._client = AsyncOpenAI(
            api_key=settings.openai_api_key or "remote-model",
            base_url=settings.effective_remote_base_url,
            timeout=settings.openai_timeout_seconds,
        )

    async def reply(
        self,
        user_message: str,
        host_context: dict[str, Any],
    ) -> str:
        response = await self._client.chat.completions.create(
            model=self._settings.effective_remote_model,
            messages=[
                {"role": "system", "content": REMOTE_ASSISTANT_SYSTEM_PROMPT},
                {
                    "role": "user",
                    "content": json.dumps(
                        {
                            "userMessage": user_message,
                            "hostContext": host_context,
                        },
                        ensure_ascii=False,
                    ),
                },
            ],
            temperature=0.3,
        )
        return _extract_text(response.choices[0].message.content).strip()
