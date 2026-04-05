from __future__ import annotations

from collections.abc import AsyncIterator
from typing import TypedDict

from langgraph.graph import END, START, StateGraph

from .llm_client import ChatModelClient
from .prompts import build_system_prompt
from .session_store import SessionStore


class ChatState(TypedDict):
    session_id: str
    messages: list[dict[str, str]]
    model_messages: list[dict[str, str]]
    latest_user_message: str
    assistant_response: str


class StreamInterruptedError(RuntimeError):
    def __init__(self, partial_content: str, cause: Exception) -> None:
        super().__init__(str(cause))
        self.partial_content = partial_content
        self.cause = cause


class ChatAgent:
    def __init__(self, llm_client: ChatModelClient, session_store: SessionStore) -> None:
        self._llm_client = llm_client
        self._session_store = session_store
        self._prepare_graph = self._build_prepare_graph()

    async def respond(self, session_id: str, user_message: str) -> str:
        state = await self._prepare_state(session_id, user_message)
        response = await self._llm_client.complete(state["model_messages"])
        self.persist_turn(session_id, user_message, response)
        return response

    async def stream_response(self, session_id: str, user_message: str) -> AsyncIterator[str]:
        state = await self._prepare_state(session_id, user_message)
        model_messages = state["model_messages"]
        collected_chunks: list[str] = []

        try:
            async for delta in self._llm_client.stream_complete(model_messages):
                if not delta:
                    continue
                collected_chunks.append(delta)
                yield delta
        except Exception as exc:
            if collected_chunks:
                raise StreamInterruptedError("".join(collected_chunks), exc) from exc

            fallback_response = await self._llm_client.complete(model_messages)
            if fallback_response:
                collected_chunks.append(fallback_response)
                yield fallback_response

        final_response = "".join(collected_chunks).strip()
        if not final_response:
            final_response = await self._llm_client.complete(model_messages)
            if final_response:
                yield final_response

        self.persist_turn(session_id, user_message, final_response)

    async def _prepare_state(self, session_id: str, user_message: str) -> ChatState:
        return await self._prepare_graph.ainvoke(
            {
                "session_id": session_id,
                "messages": [],
                "model_messages": [],
                "latest_user_message": user_message,
                "assistant_response": "",
            }
        )

    def persist_turn(self, session_id: str, user_message: str, assistant_response: str) -> list[dict[str, str]]:
        return self._session_store.append_turn(session_id, user_message, assistant_response)

    @property
    def session_store(self) -> SessionStore:
        return self._session_store

    @property
    def llm_client(self) -> ChatModelClient:
        return self._llm_client

    def _build_prepare_graph(self):
        graph = StateGraph(ChatState)
        graph.add_node("load_session_context", self._load_session_context)
        graph.add_node("build_model_messages", self._build_model_messages)

        graph.add_edge(START, "load_session_context")
        graph.add_edge("load_session_context", "build_model_messages")
        graph.add_edge("build_model_messages", END)
        return graph.compile()

    def _load_session_context(self, state: ChatState) -> ChatState:
        state["messages"] = self._session_store.get_messages(
            state["session_id"],
            limit=self._session_store.max_history_messages,
        )
        return state

    def _build_model_messages(self, state: ChatState) -> ChatState:
        messages = [{"role": "system", "content": build_system_prompt()}]
        messages.extend(state["messages"])
        messages.append({"role": "user", "content": state["latest_user_message"]})
        state["model_messages"] = messages
        return state
