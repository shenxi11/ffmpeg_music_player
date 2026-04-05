from __future__ import annotations

import asyncio
from types import SimpleNamespace

from music_agent.config import Settings
from music_agent.llm_client import OpenAIChatModelClient


class _FakeResponsesApi:
    def __init__(self, response) -> None:
        self._response = response

    async def create(self, **kwargs):
        return self._response


class _FakeChatCompletionsApi:
    def __init__(self, response) -> None:
        self._response = response

    async def create(self, **kwargs):
        return self._response


def _build_settings(*, wire_api: str) -> Settings:
    return Settings(
        OPENAI_API_KEY="sk-test",
        OPENAI_BASE_URL="http://example.test",
        OPENAI_MODEL="test-model",
        OPENAI_WIRE_API=wire_api,
    )


def test_complete_uses_responses_output_text_when_wire_api_is_responses():
    client = OpenAIChatModelClient(_build_settings(wire_api="responses"))
    client._client = SimpleNamespace(  # type: ignore[attr-defined]
        responses=_FakeResponsesApi(SimpleNamespace(output_text="多态是运行时根据对象实际类型调用函数。"))
    )

    result = asyncio.run(client.complete([{"role": "user", "content": "如何解释C++的多态？"}]))

    assert result == "多态是运行时根据对象实际类型调用函数。"


def test_stream_complete_falls_back_to_single_chunk_in_responses_mode():
    client = OpenAIChatModelClient(_build_settings(wire_api="responses"))
    client._client = SimpleNamespace(  # type: ignore[attr-defined]
        responses=_FakeResponsesApi(SimpleNamespace(output_text="智能指针会自动管理对象生命周期。"))
    )

    async def collect() -> list[str]:
        return [chunk async for chunk in client.stream_complete([{"role": "user", "content": "如何解释C++的智能指针？"}])]

    chunks = asyncio.run(collect())

    assert chunks == ["智能指针会自动管理对象生命周期。"]


def test_complete_keeps_chat_completions_path_when_wire_api_is_chat_completions():
    client = OpenAIChatModelClient(_build_settings(wire_api="chat_completions"))
    fake_response = SimpleNamespace(
        choices=[
            SimpleNamespace(
                message=SimpleNamespace(
                    content="这是 chat.completions 返回的答案。"
                )
            )
        ]
    )
    client._client = SimpleNamespace(  # type: ignore[attr-defined]
        chat=SimpleNamespace(completions=_FakeChatCompletionsApi(fake_response))
    )

    result = asyncio.run(client.complete([{"role": "user", "content": "测试"}]))

    assert result == "这是 chat.completions 返回的答案。"
