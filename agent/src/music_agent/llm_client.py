from __future__ import annotations

from collections.abc import AsyncIterator
from typing import Any, Protocol

from openai import AsyncOpenAI

from .config import Settings


class ChatModelClient(Protocol):
    async def complete(self, messages: list[dict[str, str]]) -> str:
        ...

    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        ...

    def stream_complete(self, messages: list[dict[str, str]]) -> AsyncIterator[str]:
        ...


class OpenAIChatModelClient:
    def __init__(self, settings: Settings) -> None:
        self._settings = settings
        self._client = AsyncOpenAI(
            api_key=settings.openai_api_key,
            base_url=settings.openai_base_url,
            timeout=settings.openai_timeout_seconds,
        )

    async def complete(self, messages: list[dict[str, str]]) -> str:
        if self._uses_responses_api():
            response = await self._client.responses.create(
                model=self._settings.openai_model,
                input=self._messages_to_responses_input(messages),
            )
            return self._extract_responses_text(response).strip()

        response = await self._client.chat.completions.create(
            model=self._settings.openai_model,
            messages=messages,
        )
        content = response.choices[0].message.content
        return self._extract_text(content).strip()

    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        return await self.complete(messages)

    async def stream_complete(self, messages: list[dict[str, str]]) -> AsyncIterator[str]:
        if self._uses_responses_api():
            content = await self.complete(messages)
            if content != "":
                yield content
            return

        stream = await self._client.chat.completions.create(
            model=self._settings.openai_model,
            messages=messages,
            stream=True,
        )
        async for chunk in stream:
            if not chunk.choices:
                continue
            delta = chunk.choices[0].delta
            content = self._extract_text(getattr(delta, "content", None))
            if content != "":
                yield content

    def _uses_responses_api(self) -> bool:
        return self._settings.openai_wire_api.strip().lower() == "responses"

    @staticmethod
    def _extract_text(content: str | list | None) -> str:
        if isinstance(content, str):
            return content
        if isinstance(content, list):
            text_parts: list[str] = []
            for item in content:
                if isinstance(item, dict) and item.get("type") == "text":
                    text_parts.append(str(item.get("text", "")))
                else:
                    text_value = getattr(item, "text", None)
                    if text_value:
                        text_parts.append(str(text_value))
            return "".join(text_parts)
        return ""

    @staticmethod
    def _messages_to_responses_input(messages: list[dict[str, str]]) -> str:
        parts: list[str] = []
        for message in messages:
            role = str(message.get("role", "user")).strip() or "user"
            content = str(message.get("content", "")).strip()
            if content == "":
                continue
            parts.append(f"{role}: {content}")
        return "\n\n".join(parts)

    def _extract_responses_text(self, response: Any) -> str:
        output_text = getattr(response, "output_text", None)
        if isinstance(output_text, str) and output_text.strip() != "":
            return output_text

        if isinstance(response, str):
            return response

        if isinstance(response, dict):
            return self._extract_responses_text_from_output(response.get("output"))

        output = getattr(response, "output", None)
        return self._extract_responses_text_from_output(output)

    def _extract_responses_text_from_output(self, output: Any) -> str:
        if not isinstance(output, list):
            return ""

        text_parts: list[str] = []
        for item in output:
            if isinstance(item, dict):
                content = item.get("content")
            else:
                content = getattr(item, "content", None)

            if isinstance(content, str):
                text_parts.append(content)
                continue

            if not isinstance(content, list):
                continue

            for content_item in content:
                if isinstance(content_item, dict):
                    item_type = str(content_item.get("type", ""))
                    if item_type in {"text", "output_text"}:
                        text_parts.append(str(content_item.get("text", "")))
                else:
                    item_type = str(getattr(content_item, "type", ""))
                    if item_type in {"text", "output_text"}:
                        text_parts.append(str(getattr(content_item, "text", "")))

        return "".join(text_parts)
