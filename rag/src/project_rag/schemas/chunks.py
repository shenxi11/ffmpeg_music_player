"""
???: schemas.chunks
????: ?? RAG ????????????????????
????: ChunkMetadata?KnowledgeChunk?IndexStats?IndexRunReport
????: pydantic
????: ???????????????????
?????: ?????????????????????
????: stable_id ? metadata ??????????? Agent ???????
"""

from __future__ import annotations

from datetime import datetime
from typing import Literal

from pydantic import BaseModel, Field

SourceType = Literal["doc", "code", "protocol", "case"]
ProjectType = Literal["client", "agent", "server_doc"]


class ChunkMetadata(BaseModel):
    project: ProjectType
    source_type: SourceType
    module: str
    file_path: str
    title: str
    symbol: str = ""
    language: str = ""
    heading_path: list[str] = Field(default_factory=list)
    tags: list[str] = Field(default_factory=list)
    updated_at: str
    chunk_hash: str
    stable_id: str
    class_name: str = ""
    function_name: str = ""
    message_type: str = ""
    http_method: str = ""
    http_path: str = ""
    issue_type: str = ""
    severity: str = ""


class KnowledgeChunk(BaseModel):
    stable_id: str
    index_name: str
    text: str
    summary: str
    metadata: ChunkMetadata


class IndexStats(BaseModel):
    index_name: str
    file_count: int = 0
    chunk_count: int = 0


class IndexRunReport(BaseModel):
    started_at: datetime
    finished_at: datetime
    mode: Literal["full", "incremental"]
    stats: list[IndexStats]
    failed_files: list[str] = Field(default_factory=list)
