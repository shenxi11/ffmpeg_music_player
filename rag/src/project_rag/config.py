"""
???: config
????: ?? RAG ????????????????????
????: RagSettings
????: pydantic-settings?pathlib
????: ????????????????
?????: ???????????????????????
????: ???????????????????????
"""

from __future__ import annotations

from pathlib import Path

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class RagSettings(BaseSettings):
    """RAG ????????"""

    model_config = SettingsConfigDict(env_file=".env", env_file_encoding="utf-8", extra="ignore")

    rag_host: str = Field(default="127.0.0.1", alias="RAG_HOST")
    rag_port: int = Field(default=8877, alias="RAG_PORT")
    rag_repo_root: Path = Field(default=Path(__file__).resolve().parents[3], alias="RAG_REPO_ROOT")
    rag_data_dir: Path = Field(default=Path("rag/data"), alias="RAG_DATA_DIR")
    rag_manifest_db: Path = Field(default=Path("rag/data/manifest.db"), alias="RAG_MANIFEST_DB")
    rag_qdrant_url: str = Field(default="http://127.0.0.1:6333", alias="RAG_QDRANT_URL")
    rag_qdrant_api_key: str = Field(default="", alias="RAG_QDRANT_API_KEY")
    rag_collection_prefix: str = Field(default="music_project_rag", alias="RAG_COLLECTION_PREFIX")
    rag_embedding_provider: str = Field(default="sentence_transformers", alias="RAG_EMBEDDING_PROVIDER")
    rag_embedding_model: str = Field(default="BAAI/bge-m3", alias="RAG_EMBEDDING_MODEL")
    rag_reranker_provider: str = Field(default="sentence_transformers", alias="RAG_RERANKER_PROVIDER")
    rag_reranker_model: str = Field(default="BAAI/bge-reranker-v2-m3", alias="RAG_RERANKER_MODEL")
    rag_default_top_k: int = Field(default=8, alias="RAG_DEFAULT_TOP_K")

    def model_post_init(self, __context: object) -> None:
        repo_root = self.rag_repo_root.resolve()
        object.__setattr__(self, "rag_repo_root", repo_root)
        data_dir = self.rag_data_dir if self.rag_data_dir.is_absolute() else (repo_root / self.rag_data_dir)
        manifest_db = self.rag_manifest_db if self.rag_manifest_db.is_absolute() else (repo_root / self.rag_manifest_db)
        data_dir.mkdir(parents=True, exist_ok=True)
        manifest_db.parent.mkdir(parents=True, exist_ok=True)
        object.__setattr__(self, "rag_data_dir", data_dir.resolve())
        object.__setattr__(self, "rag_manifest_db", manifest_db.resolve())


def get_settings() -> RagSettings:
    return RagSettings()
