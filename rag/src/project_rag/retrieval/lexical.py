"""
模块: retrieval.lexical
职责: 提供基于 BM25 思路的轻量关键词检索，并负责 JSONL 词法索引落盘与读取。
核心类: LexicalIndexStore
依赖: json、math、re、pathlib
约束: 第一版不依赖外部搜索引擎，适合作为向量检索的补充通道。
输入: 索引名、KnowledgeChunk 列表、查询文本。
输出: JSONL 索引文件，以及按相关性排序的检索结果。
"""

from __future__ import annotations

import json
import math
import re
from collections import Counter, defaultdict
from pathlib import Path

from project_rag.schemas.chunks import KnowledgeChunk


def _tokenize(text: str) -> list[str]:
    return [token for token in re.split(r"[^A-Za-z0-9_\u4e00-\u9fff]+", text.lower()) if token]


class LexicalIndexStore:
    def __init__(self, data_dir: Path) -> None:
        self._data_dir = data_dir

    def _index_path(self, index_name: str) -> Path:
        return self._data_dir / f"{index_name}.jsonl"

    def write_chunks(self, index_name: str, chunks: list[KnowledgeChunk]) -> None:
        path = self._index_path(index_name)
        with path.open("w", encoding="utf-8", newline="") as handle:
            for chunk in chunks:
                handle.write(json.dumps(chunk.model_dump(mode="json"), ensure_ascii=False) + "\n")

    def read_chunks(self, index_name: str) -> list[KnowledgeChunk]:
        path = self._index_path(index_name)
        if not path.exists():
            return []
        chunks: list[KnowledgeChunk] = []
        with path.open("r", encoding="utf-8") as handle:
            for line in handle:
                line = line.strip()
                if not line:
                    continue
                chunks.append(KnowledgeChunk.model_validate(json.loads(line)))
        return chunks

    def search(self, index_names: list[str], query: str, top_k: int) -> list[tuple[KnowledgeChunk, float]]:
        tokens = _tokenize(query)
        if not tokens:
            return []

        chunks: list[KnowledgeChunk] = []
        for index_name in index_names:
            chunks.extend(self.read_chunks(index_name))
        if not chunks:
            return []

        doc_tokens = [_tokenize(chunk.text) for chunk in chunks]
        avgdl = sum(len(item) for item in doc_tokens) / max(len(doc_tokens), 1)
        doc_freq: dict[str, int] = defaultdict(int)
        for tokens_per_doc in doc_tokens:
            for token in set(tokens_per_doc):
                doc_freq[token] += 1

        results: list[tuple[KnowledgeChunk, float]] = []
        total_docs = len(chunks)
        k1 = 1.5
        b = 0.75
        for chunk, tokens_per_doc in zip(chunks, doc_tokens, strict=True):
            term_freq = Counter(tokens_per_doc)
            score = 0.0
            doc_len = len(tokens_per_doc) or 1
            for token in tokens:
                if token not in term_freq:
                    continue
                idf = math.log(1 + (total_docs - doc_freq[token] + 0.5) / (doc_freq[token] + 0.5))
                freq = term_freq[token]
                numerator = freq * (k1 + 1)
                denominator = freq + k1 * (1 - b + b * doc_len / max(avgdl, 1.0))
                score += idf * numerator / denominator
            if score > 0.0:
                results.append((chunk, score))
        return sorted(results, key=lambda item: item[1], reverse=True)[:top_k]
