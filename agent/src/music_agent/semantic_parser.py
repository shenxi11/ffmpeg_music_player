"""模块名: semantic_parser
功能概述: 基于 LLM 对用户消息进行结构化语义解析，并在失败时把异常显式抛给运行时回退。
对外接口: SemanticParser、SemanticParserError
依赖关系: json、re、llm_client、prompts、semantic_models
输入输出: 输入用户消息、工作记忆摘要和工具清单，输出 SemanticParseResult
异常与错误: 模型非 JSON 输出、字段缺失或结构非法时抛出 SemanticParserError
维护说明: 只负责“理解”，不直接生成 tool_call；所有执行决策仍由 MusicAgentRuntime 控制
"""

from __future__ import annotations

import json
import re
from typing import Any

from .llm_client import ChatModelClient
from .prompts import build_semantic_parser_prompt
from .semantic_models import ActionCandidate, SemanticParseResult, normalize_playlist_query
from .semantic_refiner import refine_semantic_result
from .tool_registry import ToolRegistry


class SemanticParserError(RuntimeError):
    pass


class SemanticParser:
    def __init__(self, llm_client: ChatModelClient, tool_registry: ToolRegistry) -> None:
        self._llm_client = llm_client
        self._tool_registry = tool_registry

    async def parse_user_message(
        self,
        user_message: str,
        context: dict[str, Any],
    ) -> SemanticParseResult:
        messages = self._build_messages(user_message, context)
        raw_response = await self._llm_client.complete_structured(messages)
        json_text = self._extract_json_text(raw_response)
        try:
            result = SemanticParseResult.model_validate_json(json_text)
        except Exception as exc:  # noqa: BLE001
            raise SemanticParserError(f"semantic parse validation failed: {exc}") from exc
        return self._post_process(result, user_message, context)

    def _build_messages(self, user_message: str, context: dict[str, Any]) -> list[dict[str, str]]:
        payload = {
            "userMessage": user_message,
            "context": context,
            "enabledTools": self._tool_registry.names(),
        }
        return [
            {"role": "system", "content": build_semantic_parser_prompt(self._tool_registry.names())},
            {"role": "user", "content": json.dumps(payload, ensure_ascii=False, indent=2)},
        ]

    @staticmethod
    def _extract_json_text(raw_response: str) -> str:
        cleaned = raw_response.strip()
        if not cleaned:
            raise SemanticParserError("semantic parser returned empty content")

        if cleaned.startswith("{") and cleaned.endswith("}"):
            return cleaned

        fenced_match = re.search(r"```json\s*(\{.*\})\s*```", cleaned, flags=re.DOTALL)
        if fenced_match:
            return fenced_match.group(1).strip()

        brace_match = re.search(r"(\{.*\})", cleaned, flags=re.DOTALL)
        if brace_match:
            return brace_match.group(1).strip()

        raise SemanticParserError("semantic parser did not return JSON")

    @staticmethod
    def _post_process(
        result: SemanticParseResult,
        user_message: str,
        context: dict[str, Any],
    ) -> SemanticParseResult:
        for playlist in (
            result.entities.playlist,
            result.entities.target_playlist,
            result.entities.source_playlist,
        ):
            if playlist is None:
                continue
            if playlist.raw_query:
                playlist.raw_query = playlist.raw_query.strip()
            if not playlist.normalized_query:
                playlist.normalized_query = normalize_playlist_query(playlist.raw_query)

        track = result.entities.track
        if track is not None:
            if track.raw_query:
                track.raw_query = track.raw_query.strip()
            if not track.normalized_query and track.raw_query:
                track.normalized_query = track.raw_query.strip()
            if track.artist and not result.entities.artist:
                result.entities.artist = track.artist
            if track.album and not result.entities.album:
                result.entities.album = track.album

        result.references = [item.strip() for item in result.references if item and item.strip()]
        result.ambiguities = [item.strip() for item in result.ambiguities if item and item.strip()]
        result.missing_fields = [item.strip() for item in result.missing_fields if item and item.strip()]
        result.confidence = max(0.0, min(1.0, float(result.confidence)))
        if result.proposed_tool is not None:
            result.proposed_tool.tool = result.proposed_tool.tool.strip()
            result.proposed_tool.args = dict(result.proposed_tool.args or {})
            if result.proposed_tool.reason:
                result.proposed_tool.reason = result.proposed_tool.reason.strip()
            if result.proposed_tool.confidence is not None:
                result.proposed_tool.confidence = max(0.0, min(1.0, float(result.proposed_tool.confidence)))
        normalized_candidates = []
        for candidate in result.action_candidates:
            if candidate.tool:
                candidate.tool = candidate.tool.strip()
            candidate.args = dict(candidate.args or {})
            candidate.depends_on = [item.strip() for item in candidate.depends_on if item and item.strip()]
            if candidate.reason:
                candidate.reason = candidate.reason.strip()
            if candidate.confidence is not None:
                candidate.confidence = max(0.0, min(1.0, float(candidate.confidence)))
            normalized_candidates.append(candidate)
        result.action_candidates = normalized_candidates
        if not result.action_candidates and result.proposed_tool is not None:
            result.action_candidates = [
                ActionCandidate(
                    stepId="step-1",
                    tool=result.proposed_tool.tool,
                    args=result.proposed_tool.args,
                    kind="tool",
                    dependsOn=[],
                    confidence=result.proposed_tool.confidence,
                    reason=result.proposed_tool.reason,
                )
            ]
        return refine_semantic_result(result, user_message, context)
