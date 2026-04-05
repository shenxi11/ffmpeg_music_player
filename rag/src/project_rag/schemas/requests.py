"""
???: schemas.requests
????: ?? RAG ???????????????
????: QueryRequest?AgentContextRequest?QueryResponse ?
????: pydantic
????: ?? HTTP ??????????? Agent ????
?????: ???????? API ???????
????: mode ? filters ??????????????
"""

from __future__ import annotations

from typing import Any, Literal

from pydantic import BaseModel, Field

QueryMode = Literal["architecture", "code_lookup", "protocol_lookup", "incident_lookup"]


class QueryRequest(BaseModel):
    query: str
    mode: QueryMode = "architecture"
    filters: dict[str, list[str]] = Field(default_factory=dict)
    top_k: int = Field(default=8, alias="topK")


class AgentContextRequest(BaseModel):
    task: str
    question: str
    target_agent: str = Field(alias="targetAgent")
    filters: dict[str, list[str]] = Field(default_factory=dict)
    top_k: int = Field(default=8, alias="topK")


class CitationItem(BaseModel):
    file_path: str
    symbol: str = ""
    heading_path: list[str] = Field(default_factory=list)
    snippet: str
    source_type: str


class EvidenceItem(BaseModel):
    stable_id: str
    score: float
    summary: str
    metadata: dict[str, Any]


class QueryResponse(BaseModel):
    summary: str
    evidence: list[EvidenceItem]
    citations: list[CitationItem]
    related_symbols: list[str] = Field(default_factory=list, alias="relatedSymbols")
    related_paths: list[str] = Field(default_factory=list, alias="relatedPaths")
    suggested_followup_queries: list[str] = Field(default_factory=list, alias="suggestedFollowupQueries")


class IndexRequest(BaseModel):
    changed_paths: list[str] = Field(default_factory=list, alias="changedPaths")
