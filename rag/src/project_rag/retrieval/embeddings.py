"""
???: retrieval.embeddings
????: ???? embedding ????????????????????????
????: EmbeddingProvider?SentenceTransformerEmbedder?FallbackHashEmbedder
????: math?hashlib??? sentence_transformers
????: ???????????????
?????: ?????????????????????????
????: ???????????????????????
"""

from __future__ import annotations

import hashlib
import math
from typing import Protocol


class EmbeddingProvider(Protocol):
    def embed(self, texts: list[str]) -> list[list[float]]:
        ...


class FallbackHashEmbedder:
    def __init__(self, dim: int = 64) -> None:
        self._dim = dim

    def embed(self, texts: list[str]) -> list[list[float]]:
        vectors: list[list[float]] = []
        for text in texts:
            vector = [0.0] * self._dim
            tokens = text.split()
            if not tokens:
                vectors.append(vector)
                continue
            for token in tokens:
                digest = hashlib.sha1(token.encode("utf-8")).digest()
                for index in range(self._dim):
                    vector[index] += (digest[index % len(digest)] / 255.0) - 0.5
            norm = math.sqrt(sum(value * value for value in vector)) or 1.0
            vectors.append([value / norm for value in vector])
        return vectors


class SentenceTransformerEmbedder:
    def __init__(self, model_name: str) -> None:
        from sentence_transformers import SentenceTransformer

        self._model = SentenceTransformer(model_name)

    def embed(self, texts: list[str]) -> list[list[float]]:
        return self._model.encode(texts, normalize_embeddings=True).tolist()
