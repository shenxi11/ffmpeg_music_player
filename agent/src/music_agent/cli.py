from __future__ import annotations

import argparse
import asyncio
import sys
import uuid
from collections.abc import Callable

from .chat_agent import ChatAgent
from .config import Settings
from .llm_client import OpenAIChatModelClient
from .session_store import InMemorySessionStore

Reader = Callable[[str], str]
Writer = Callable[[str], None]


class TerminalChatRunner:
    def __init__(
        self,
        agent: ChatAgent,
        session_id: str,
        reader: Reader,
        writer: Writer,
    ) -> None:
        self._agent = agent
        self._session_id = session_id
        self._reader = reader
        self._writer = writer

    @property
    def session_id(self) -> str:
        return self._session_id

    async def run(self) -> int:
        self._write_banner()

        while True:
            try:
                user_input = self._reader("you> ")
            except EOFError:
                self._writer("bye")
                return 0
            except KeyboardInterrupt:
                self._writer("\nbye")
                return 0

            content = user_input.strip()
            if not content:
                continue

            if content in {"/exit", "/quit"}:
                self._writer("bye")
                return 0

            if content == "/help":
                self._writer("commands: /help /exit /quit")
                continue

            try:
                response = await self._agent.respond(self._session_id, content)
            except Exception as exc:
                self._writer(f"agent> [error] {exc}")
                continue

            self._writer(f"agent> {response}")

    def _write_banner(self) -> None:
        self._writer("Music Agent Terminal")
        self._writer(f"session: {self._session_id}")
        self._writer("commands: /help /exit /quit")


async def run_terminal_chat(
    settings: Settings | None = None,
    session_id: str | None = None,
    reader: Reader = input,
    writer: Writer = print,
) -> int:
    resolved_settings = settings or Settings()
    if not resolved_settings.is_model_configured():
        missing = ", ".join(resolved_settings.missing_model_config())
        writer(f"missing model config: {missing}")
        return 1

    agent = ChatAgent(
        llm_client=OpenAIChatModelClient(resolved_settings),
        session_store=InMemorySessionStore(resolved_settings.agent_max_history_messages),
    )
    runner = TerminalChatRunner(
        agent=agent,
        session_id=session_id or str(uuid.uuid4()),
        reader=reader,
        writer=writer,
    )
    return await runner.run()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Start a local terminal chat with the music agent.")
    parser.add_argument("--session-id", dest="session_id", help="Reuse a specific chat session id.")
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    raise SystemExit(asyncio.run(run_terminal_chat(session_id=args.session_id)))


if __name__ == "__main__":
    main()
