from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore",
    )

    openai_api_key: str | None = Field(default=None, alias="OPENAI_API_KEY")
    openai_base_url: str | None = Field(default=None, alias="OPENAI_BASE_URL")
    openai_model: str | None = Field(default=None, alias="OPENAI_MODEL")
    openai_wire_api: str = Field(default="chat_completions", alias="OPENAI_WIRE_API")
    openai_timeout_seconds: float = Field(default=30.0, alias="OPENAI_TIMEOUT_SECONDS")

    agent_host: str = Field(default="127.0.0.1", alias="AGENT_HOST")
    agent_port: int = Field(default=8765, alias="AGENT_PORT")
    agent_max_history_messages: int = Field(default=20, alias="AGENT_MAX_HISTORY_MESSAGES")
    agent_storage_path: str = Field(default="data/music_agent.db", alias="AGENT_STORAGE_PATH")
    agent_tool_timeout_seconds: float = Field(default=15.0, alias="AGENT_TOOL_TIMEOUT_SECONDS")
    agent_semantic_timeout_seconds: float = Field(default=6.0, alias="AGENT_SEMANTIC_TIMEOUT_SECONDS")
    agent_allow_direct_write_actions: bool = Field(default=True, alias="AGENT_ALLOW_DIRECT_WRITE_ACTIONS")

    def missing_model_config(self) -> list[str]:
        missing: list[str] = []
        if not self.openai_api_key:
            missing.append("OPENAI_API_KEY")
        if not self.openai_model:
            missing.append("OPENAI_MODEL")
        return missing

    def is_model_configured(self) -> bool:
        return not self.missing_model_config()
