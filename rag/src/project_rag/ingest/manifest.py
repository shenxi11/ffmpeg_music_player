"""
???: ingest.manifest
????: ???????????????????
????: ManifestStore
????: sqlite3?hashlib?pathlib
????: ????????????????????????
?????: SQLite ??????????????????
????: ?????????????????? chunk ??????????
"""

from __future__ import annotations

import hashlib
import sqlite3
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path

from project_rag.schemas.chunks import KnowledgeChunk


@dataclass(frozen=True)
class ChangedFile:
    path: str
    state: str


class ManifestStore:
    def __init__(self, db_path: Path) -> None:
        self._conn = sqlite3.connect(str(db_path))
        self._conn.row_factory = sqlite3.Row
        self._init_schema()

    def close(self) -> None:
        self._conn.close()

    def _init_schema(self) -> None:
        self._conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS files (
                file_path TEXT PRIMARY KEY,
                file_hash TEXT NOT NULL,
                updated_at TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS chunks (
                stable_id TEXT PRIMARY KEY,
                index_name TEXT NOT NULL,
                file_path TEXT NOT NULL,
                chunk_hash TEXT NOT NULL,
                updated_at TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS runs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                mode TEXT NOT NULL,
                started_at TEXT NOT NULL,
                finished_at TEXT NOT NULL,
                file_count INTEGER NOT NULL,
                chunk_count INTEGER NOT NULL,
                failed_files_json TEXT NOT NULL
            );
            """
        )
        self._conn.commit()

    @staticmethod
    def compute_file_hash(file_path: Path) -> str:
        digest = hashlib.sha1()
        digest.update(file_path.read_bytes())
        return digest.hexdigest()

    def diff_files(self, repo_root: Path, files: list[Path]) -> list[ChangedFile]:
        current = {path.resolve().relative_to(repo_root.resolve()).as_posix(): self.compute_file_hash(path) for path in files}
        existing_rows = self._conn.execute("SELECT file_path, file_hash FROM files").fetchall()
        existing = {row["file_path"]: row["file_hash"] for row in existing_rows}
        changed: list[ChangedFile] = []
        for file_path, file_hash in current.items():
            previous = existing.get(file_path)
            if previous is None:
                changed.append(ChangedFile(file_path, "added"))
            elif previous != file_hash:
                changed.append(ChangedFile(file_path, "modified"))
        for file_path in sorted(set(existing) - set(current)):
            changed.append(ChangedFile(file_path, "deleted"))
        return sorted(changed, key=lambda item: item.path)

    def replace_file_chunks(self, relative_path: str, file_hash: str | None, chunks: list[KnowledgeChunk]) -> None:
        now = datetime.now(UTC).isoformat()
        self._conn.execute("DELETE FROM chunks WHERE file_path = ?", (relative_path,))
        if file_hash is None:
            self._conn.execute("DELETE FROM files WHERE file_path = ?", (relative_path,))
            self._conn.commit()
            return
        self._conn.execute(
            "INSERT INTO files(file_path, file_hash, updated_at) VALUES(?, ?, ?) "
            "ON CONFLICT(file_path) DO UPDATE SET file_hash=excluded.file_hash, updated_at=excluded.updated_at",
            (relative_path, file_hash, now),
        )
        for chunk in chunks:
            self._conn.execute(
                "INSERT OR REPLACE INTO chunks(stable_id, index_name, file_path, chunk_hash, updated_at) VALUES(?, ?, ?, ?, ?)",
                (chunk.stable_id, chunk.index_name, relative_path, chunk.metadata.chunk_hash, now),
            )
        self._conn.commit()

    def record_run(self, *, mode: str, started_at: str, finished_at: str, file_count: int, chunk_count: int, failed_files_json: str) -> None:
        self._conn.execute(
            "INSERT INTO runs(mode, started_at, finished_at, file_count, chunk_count, failed_files_json) VALUES(?, ?, ?, ?, ?, ?)",
            (mode, started_at, finished_at, file_count, chunk_count, failed_files_json),
        )
        self._conn.commit()
