"""Minimal chat agent backend package."""

from .config import Settings

__all__ = ["ChatAgent", "ControlRuntime", "Settings", "run_terminal_chat"]


def __getattr__(name: str):
    if name == "ChatAgent":
        from .chat_agent import ChatAgent

        return ChatAgent
    if name == "ControlRuntime":
        from .control_runtime import ControlRuntime

        return ControlRuntime
    if name == "run_terminal_chat":
        from .cli import run_terminal_chat

        return run_terminal_chat
    raise AttributeError(f"module 'music_agent' has no attribute {name!r}")
