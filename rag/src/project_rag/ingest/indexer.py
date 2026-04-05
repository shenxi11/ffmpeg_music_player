"""
???: ingest.indexer
????: ??????????????????????????????
????: RagIndexer?main
????: config?sources?chunkers?manifest?retrieval.service
????: ????????????????????????
?????: ???????? failed_files??????????
????: ?????????? chunk ??????????????????
"""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from datetime import UTC, datetime
from pathlib import Path

from project_rag.config import RagSettings, get_settings
from project_rag.ingest.chunkers import chunk_file
from project_rag.ingest.manifest import ManifestStore
from project_rag.ingest.sources import classify_source, discover_source_files
from project_rag.retrieval.service import RetrievalService
from project_rag.schemas.chunks import IndexRunReport, IndexStats, KnowledgeChunk


class RagIndexer:
    def __init__(self, settings: RagSettings) -> None:
        self._settings = settings
        self._manifest = ManifestStore(settings.rag_manifest_db)
        self._retrieval = RetrievalService(settings)

    def close(self) -> None:
        self._manifest.close()

    def build_full(self) -> IndexRunReport:
        files_with_specs = discover_source_files(self._settings.rag_repo_root)
        return self._index(mode="full", files_with_specs=files_with_specs, changed_only=None)

    def build_incremental(self, changed_paths: list[str] | None = None) -> IndexRunReport:
        files_with_specs = discover_source_files(self._settings.rag_repo_root)
        if changed_paths:
            allow = {Path(path).as_posix() for path in changed_paths}
            filtered = [item for item in files_with_specs if item[0].resolve().relative_to(self._settings.rag_repo_root.resolve()).as_posix() in allow]
            return self._index(mode="incremental", files_with_specs=filtered, changed_only=allow)

        changed = self._manifest.diff_files(self._settings.rag_repo_root, [path for path, _ in files_with_specs])
        allow = {item.path for item in changed}
        filtered = [item for item in files_with_specs if item[0].resolve().relative_to(self._settings.rag_repo_root.resolve()).as_posix() in allow]
        report = self._index(mode="incremental", files_with_specs=filtered, changed_only=allow)
        deleted = [item for item in changed if item.state == "deleted"]
        for item in deleted:
            self._manifest.replace_file_chunks(item.path, None, [])
        return report

    def _index(self, *, mode: str, files_with_specs: list[tuple[Path, object]], changed_only: set[str] | None) -> IndexRunReport:
        started_at = datetime.now(UTC)
        failed_files: list[str] = []
        rebuilt_chunks: dict[str, list[KnowledgeChunk]] = defaultdict(list)
        if mode == "full":
            for index_name in ("docs_index", "code_index", "protocol_index", "case_index"):
                rebuilt_chunks[index_name] = []
        else:
            for index_name in ("docs_index", "code_index", "protocol_index", "case_index"):
                rebuilt_chunks[index_name] = self._retrieval.lexical_store.read_chunks(index_name)

        existing_by_file_and_index: dict[tuple[str, str], list[KnowledgeChunk]] = defaultdict(list)
        for index_name, chunks in rebuilt_chunks.items():
            for chunk in chunks:
                existing_by_file_and_index[(chunk.metadata.file_path, index_name)].append(chunk)

        for file_path, spec in files_with_specs:
            relative_path = file_path.resolve().relative_to(self._settings.rag_repo_root.resolve()).as_posix()
            if changed_only is not None and relative_path not in changed_only and mode != "full":
                continue
            try:
                source_type, language = classify_source(file_path, spec)
                chunks = chunk_file(self._settings.rag_repo_root, file_path, project=spec.project, module=spec.module, source_type=source_type, language=language)
                for index_name in ("docs_index", "code_index", "protocol_index", "case_index"):
                    previous = existing_by_file_and_index.get((relative_path, index_name), [])
                    if previous:
                        rebuilt_chunks[index_name] = [item for item in rebuilt_chunks[index_name] if item.metadata.file_path != relative_path]
                for chunk in chunks:
                    rebuilt_chunks[chunk.index_name].append(chunk)
                file_hash = self._manifest.compute_file_hash(file_path)
                self._manifest.replace_file_chunks(relative_path, file_hash, chunks)
            except Exception:
                failed_files.append(relative_path)

        for index_name, chunks in rebuilt_chunks.items():
            self._retrieval.lexical_store.write_chunks(index_name, chunks)
            if chunks:
                vectors = self._retrieval._embedder.embed([chunk.text for chunk in chunks])  # noqa: SLF001
                self._retrieval.vector_store.replace_index(index_name, list(zip(chunks, vectors, strict=True)))
            else:
                self._retrieval.vector_store.replace_index(index_name, [])

        finished_at = datetime.now(UTC)
        stats = [IndexStats(index_name=index_name, file_count=len({chunk.metadata.file_path for chunk in chunks}), chunk_count=len(chunks)) for index_name, chunks in rebuilt_chunks.items()]
        report = IndexRunReport(started_at=started_at, finished_at=finished_at, mode=mode, stats=stats, failed_files=sorted(failed_files))
        self._manifest.record_run(mode=mode, started_at=started_at.isoformat(), finished_at=finished_at.isoformat(), file_count=sum(item.file_count for item in stats), chunk_count=sum(item.chunk_count for item in stats), failed_files_json=json.dumps(report.failed_files, ensure_ascii=False))
        return report


def main() -> None:
    parser = argparse.ArgumentParser(description="Project RAG index builder")
    parser.add_argument("--mode", choices=("full", "incremental"), default="full")
    parser.add_argument("--changed-path", action="append", default=[])
    args = parser.parse_args()

    settings = get_settings()
    indexer = RagIndexer(settings)
    try:
        report = indexer.build_full() if args.mode == "full" else indexer.build_incremental(args.changed_path)
        print(json.dumps(report.model_dump(mode="json"), indent=2, ensure_ascii=False))
    finally:
        indexer.close()
