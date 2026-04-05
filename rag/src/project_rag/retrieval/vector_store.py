"""
???: retrieval.vector_store
????: ?? Qdrant ????????????????????????
????: VectorStore
????: qdrant-client?????pathlib?embeddings
????: ??????????????????
?????: Qdrant ????????????????????
????: ????? Qdrant??????????????????
"""

from __future__ import annotations

import math
from typing import Any

from project_rag.schemas.chunks import KnowledgeChunk


class VectorStore:
    def __init__(self, *, qdrant_url: str, qdrant_api_key: str, collection_prefix: str) -> None:
        self._collection_prefix = collection_prefix
        self._memory_store: dict[str, list[tuple[KnowledgeChunk, list[float]]]] = {}
        self._qdrant_client = None
        self._qdrant_enabled = False
        try:
            from qdrant_client import QdrantClient
            from qdrant_client.http.models import Distance, VectorParams

            client = QdrantClient(url=qdrant_url, api_key=qdrant_api_key or None)
            self._distance_cls = Distance
            self._vector_params_cls = VectorParams
            self._qdrant_client = client
            self._qdrant_enabled = True
        except Exception:
            self._distance_cls = None
            self._vector_params_cls = None

    @property
    def qdrant_enabled(self) -> bool:
        return self._qdrant_enabled

    def replace_index(self, index_name: str, items: list[tuple[KnowledgeChunk, list[float]]]) -> None:
        self._memory_store[index_name] = items
        if not self._qdrant_enabled or self._qdrant_client is None or self._vector_params_cls is None:
            return
        collection_name = f"{self._collection_prefix}_{index_name}"
        vector_size = len(items[0][1]) if items else 64
        self._qdrant_client.recreate_collection(
            collection_name=collection_name,
            vectors_config=self._vector_params_cls(size=vector_size, distance=self._distance_cls.COSINE),
        )
        if not items:
            return
        payloads: list[dict[str, Any]] = []
        vectors: list[list[float]] = []
        ids: list[str] = []
        for chunk, vector in items:
            ids.append(chunk.stable_id)
            vectors.append(vector)
            payloads.append(chunk.model_dump(mode="json"))
        self._qdrant_client.upsert(
            collection_name=collection_name,
            points=[{"id": ids[index], "vector": vectors[index], "payload": payloads[index]} for index in range(len(ids))],
        )

    def search(self, index_names: list[str], query_vector: list[float], top_k: int) -> list[tuple[KnowledgeChunk, float]]:
        if self._qdrant_enabled and self._qdrant_client is not None:
            merged: list[tuple[KnowledgeChunk, float]] = []
            for index_name in index_names:
                collection_name = f"{self._collection_prefix}_{index_name}"
                try:
                    points = self._qdrant_client.search(collection_name=collection_name, query_vector=query_vector, limit=top_k)
                except Exception:
                    points = []
                for point in points:
                    merged.append((KnowledgeChunk.model_validate(point.payload), float(point.score)))
            return sorted(merged, key=lambda item: item[1], reverse=True)[:top_k]

        merged: list[tuple[KnowledgeChunk, float]] = []
        for index_name in index_names:
            for chunk, vector in self._memory_store.get(index_name, []):
                score = _cosine_similarity(query_vector, vector)
                if score > 0.0:
                    merged.append((chunk, score))
        return sorted(merged, key=lambda item: item[1], reverse=True)[:top_k]


def _cosine_similarity(left: list[float], right: list[float]) -> float:
    numerator = sum(a * b for a, b in zip(left, right, strict=False))
    left_norm = math.sqrt(sum(value * value for value in left)) or 1.0
    right_norm = math.sqrt(sum(value * value for value in right)) or 1.0
    return numerator / (left_norm * right_norm)
