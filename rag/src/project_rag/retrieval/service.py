"""
???: retrieval.service
????: ????????????rerank ? Agent ??????
????: RetrievalService
????: lexical?vector_store?embeddings?schemas.requests
????: ????????? QueryResponse
?????: ?????????????????????????
????: ???????????????????????????
"""

from __future__ import annotations

from collections import OrderedDict
from typing import Iterable

from project_rag.config import RagSettings
from project_rag.retrieval.embeddings import FallbackHashEmbedder, SentenceTransformerEmbedder
from project_rag.retrieval.lexical import LexicalIndexStore
from project_rag.retrieval.vector_store import VectorStore
from project_rag.schemas.requests import CitationItem, EvidenceItem, QueryResponse

MODE_TO_INDEXES = {
    "architecture": ["code_index", "docs_index"],
    "code_lookup": ["code_index", "docs_index"],
    "protocol_lookup": ["protocol_index", "docs_index"],
    "incident_lookup": ["case_index", "code_index"],
}


class RetrievalService:
    def __init__(self, settings: RagSettings) -> None:
        self._settings = settings
        self._lexical = LexicalIndexStore(settings.rag_data_dir)
        self._embedder = self._create_embedder()
        self._vector_store = VectorStore(qdrant_url=settings.rag_qdrant_url, qdrant_api_key=settings.rag_qdrant_api_key, collection_prefix=settings.rag_collection_prefix)

    def _create_embedder(self):
        if self._settings.rag_embedding_provider != "sentence_transformers":
            return FallbackHashEmbedder()
        try:
            return SentenceTransformerEmbedder(self._settings.rag_embedding_model)
        except Exception:
            return FallbackHashEmbedder()

    @property
    def vector_store(self) -> VectorStore:
        return self._vector_store

    @property
    def lexical_store(self) -> LexicalIndexStore:
        return self._lexical

    def query(self, *, query: str, mode: str, filters: dict[str, list[str]], top_k: int) -> QueryResponse:
        index_names = MODE_TO_INDEXES.get(mode, MODE_TO_INDEXES["architecture"])
        lexical_hits = self._lexical.search(index_names, query, top_k=max(top_k, 8))
        query_vector = self._embedder.embed([query])[0]
        vector_hits = self._vector_store.search(index_names, query_vector, top_k=max(top_k, 8))
        merged = self._merge_hits(lexical_hits, vector_hits, filters)[:top_k]
        summary = self._build_summary(query, merged)
        citations = [CitationItem(file_path=item.metadata["file_path"], symbol=item.metadata.get("symbol", ""), heading_path=item.metadata.get("heading_path", []), snippet=item.summary, source_type=item.metadata.get("source_type", "")) for item in merged]
        related_symbols = sorted({item.metadata.get("symbol", "") for item in merged if item.metadata.get("symbol")})
        related_paths = sorted({item.metadata["file_path"] for item in merged})
        return QueryResponse(summary=summary, evidence=merged, citations=citations, relatedSymbols=related_symbols, relatedPaths=related_paths, suggestedFollowupQueries=self._build_followups(mode, merged))

    def _merge_hits(self, lexical_hits: list[tuple], vector_hits: list[tuple], filters: dict[str, list[str]]) -> list[EvidenceItem]:
        merged: OrderedDict[str, dict] = OrderedDict()
        for hits, weight in ((lexical_hits, 1.0), (vector_hits, 0.85)):
            for chunk, score in hits:
                if not self._matches_filters(chunk.metadata.model_dump(mode="json"), filters):
                    continue
                entry = merged.setdefault(chunk.stable_id, {"chunk": chunk, "score": 0.0})
                entry["score"] += float(score) * weight
        ranked = sorted(merged.values(), key=lambda item: item["score"], reverse=True)
        return [EvidenceItem(stable_id=item["chunk"].stable_id, score=round(item["score"], 4), summary=item["chunk"].summary, metadata=item["chunk"].metadata.model_dump(mode="json")) for item in ranked]

    @staticmethod
    def _matches_filters(metadata: dict, filters: dict[str, list[str]]) -> bool:
        for key, allowed_values in filters.items():
            if not allowed_values:
                continue
            value = metadata.get(key)
            if isinstance(value, list):
                if not set(value).intersection(set(allowed_values)):
                    return False
                continue
            if value not in allowed_values:
                return False
        return True

    @staticmethod
    def _build_summary(query: str, evidence: Iterable[EvidenceItem]) -> str:
        evidence_list = list(evidence)
        if not evidence_list:
            return f"??????{query}?????????????"
        top = evidence_list[0]
        return f"???{query}???? {len(evidence_list)} ????????????????? {top.metadata.get('file_path', '')}?"

    @staticmethod
    def _build_followups(mode: str, evidence: list[EvidenceItem]) -> list[str]:
        if not evidence:
            return []
        if mode == "incident_lookup":
            return ["?????????????", "?????????"]
        if mode == "protocol_lookup":
            return ["???????????", "???????????"]
        return ["?????????", "??????????????"]
