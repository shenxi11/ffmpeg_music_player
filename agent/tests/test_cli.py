import asyncio

from music_agent.chat_agent import ChatAgent
from music_agent.cli import TerminalChatRunner, run_terminal_chat
from music_agent.config import Settings
from music_agent.session_store import InMemorySessionStore


class FakeChatModelClient:
    async def complete(self, messages: list[dict[str, str]]) -> str:
        return f"echo:{messages[-1]['content']}"


def test_terminal_runner_handles_basic_chat_and_exit():
    agent = ChatAgent(FakeChatModelClient(), InMemorySessionStore())
    inputs = iter(["你好", "/exit"])
    outputs: list[str] = []

    runner = TerminalChatRunner(
        agent=agent,
        session_id="cli-session",
        reader=lambda prompt: next(inputs),
        writer=outputs.append,
    )

    exit_code = asyncio.run(runner.run())

    assert exit_code == 0
    assert outputs == [
        "Music Agent Terminal",
        "session: cli-session",
        "commands: /help /exit /quit",
        "agent> echo:你好",
        "bye",
    ]


def test_terminal_runner_shows_help():
    agent = ChatAgent(FakeChatModelClient(), InMemorySessionStore())
    inputs = iter(["/help", "/quit"])
    outputs: list[str] = []

    runner = TerminalChatRunner(
        agent=agent,
        session_id="cli-help",
        reader=lambda prompt: next(inputs),
        writer=outputs.append,
    )

    exit_code = asyncio.run(runner.run())

    assert exit_code == 0
    assert "commands: /help /exit /quit" in outputs


def test_run_terminal_chat_requires_model_config():
    outputs: list[str] = []

    exit_code = asyncio.run(
        run_terminal_chat(
            settings=Settings(_env_file=None),
            reader=lambda prompt: "/exit",
            writer=outputs.append,
        )
    )

    assert exit_code == 1
    assert outputs == ["missing model config: OPENAI_API_KEY, OPENAI_MODEL"]
