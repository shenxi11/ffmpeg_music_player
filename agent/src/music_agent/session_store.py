from __future__ import annotations

import json
import sqlite3
import uuid
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from threading import Lock, RLock
from typing import Protocol


DEFAULT_SESSION_TITLE = "新建会话"


@dataclass(frozen=True)
class SessionSummary:
    session_id: str
    title: str
    created_at: str
    updated_at: str
    last_preview: str
    message_count: int


@dataclass(frozen=True)
class StoredMessage:
    message_id: str
    session_id: str
    role: str
    content: str
    created_at: str


@dataclass(frozen=True)
class StoredEvent:
    event_id: str
    session_id: str
    plan_id: str | None
    event_type: str
    payload: dict
    created_at: str


class SessionStore(Protocol):
    @property
    def max_history_messages(self) -> int:
        ...

    def create_session(self, title: str | None = None) -> SessionSummary:
        ...

    def ensure_session(self, session_id: str, title: str | None = None) -> SessionSummary:
        ...

    def get_session(self, session_id: str) -> SessionSummary | None:
        ...

    def list_sessions(self, query: str | None = None, limit: int = 100) -> list[SessionSummary]:
        ...

    def update_session_title(self, session_id: str, title: str) -> SessionSummary | None:
        ...

    def delete_session(self, session_id: str) -> bool:
        ...

    def get_messages(self, session_id: str, limit: int | None = None) -> list[dict[str, str]]:
        ...

    def get_message_records(self, session_id: str) -> list[StoredMessage]:
        ...

    def append_turn(self, session_id: str, user_message: str, assistant_message: str) -> list[dict[str, str]]:
        ...

    def append_event(
        self,
        session_id: str,
        event_type: str,
        payload: dict,
        plan_id: str | None = None,
    ) -> StoredEvent:
        ...

    def get_event_records(self, session_id: str, limit: int | None = None) -> list[StoredEvent]:
        ...

    def get_plan_event_records(self, plan_id: str, limit: int | None = None) -> list[StoredEvent]:
        ...


def now_iso() -> str:
    return datetime.now(UTC).isoformat()


def generate_session_title(user_message: str) -> str:
    first_line = next((line.strip() for line in user_message.splitlines() if line.strip()), "")
    if not first_line:
        return DEFAULT_SESSION_TITLE
    return first_line[:40]


def build_preview(text: str) -> str:
    condensed = " ".join(part.strip() for part in text.splitlines() if part.strip())
    return condensed[:80]


class InMemorySessionStore:
    def __init__(self, max_history_messages: int = 20) -> None:
        self._max_history_messages = max(1, max_history_messages)
        self._sessions: dict[str, SessionSummary] = {}
        self._messages: dict[str, list[StoredMessage]] = {}
        self._events: dict[str, list[StoredEvent]] = {}
        self._lock = RLock()

    @property
    def max_history_messages(self) -> int:
        return self._max_history_messages

    def create_session(self, title: str | None = None) -> SessionSummary:
        session_id = str(uuid.uuid4())
        return self.ensure_session(session_id, title=title)

    def ensure_session(self, session_id: str, title: str | None = None) -> SessionSummary:
        with self._lock:
            existing = self._sessions.get(session_id)
            if existing is not None:
                return existing
            timestamp = now_iso()
            summary = SessionSummary(
                session_id=session_id,
                title=(title or DEFAULT_SESSION_TITLE).strip() or DEFAULT_SESSION_TITLE,
                created_at=timestamp,
                updated_at=timestamp,
                last_preview="",
                message_count=0,
            )
            self._sessions[session_id] = summary
            self._messages[session_id] = []
            self._events[session_id] = []
            return summary

    def get_session(self, session_id: str) -> SessionSummary | None:
        with self._lock:
            return self._sessions.get(session_id)

    def list_sessions(self, query: str | None = None, limit: int = 100) -> list[SessionSummary]:
        with self._lock:
            items = list(self._sessions.values())
        if query:
            needle = query.strip().lower()
            items = [
                item for item in items
                if needle in item.title.lower() or needle in item.last_preview.lower()
            ]
        items.sort(key=lambda item: item.updated_at, reverse=True)
        return items[: max(1, limit)]

    def update_session_title(self, session_id: str, title: str) -> SessionSummary | None:
        with self._lock:
            summary = self._sessions.get(session_id)
            if summary is None:
                return None
            updated = SessionSummary(
                session_id=summary.session_id,
                title=title.strip() or DEFAULT_SESSION_TITLE,
                created_at=summary.created_at,
                updated_at=now_iso(),
                last_preview=summary.last_preview,
                message_count=summary.message_count,
            )
            self._sessions[session_id] = updated
            return updated

    def delete_session(self, session_id: str) -> bool:
        with self._lock:
            removed = self._sessions.pop(session_id, None)
            self._messages.pop(session_id, None)
            self._events.pop(session_id, None)
            return removed is not None

    def get_messages(self, session_id: str, limit: int | None = None) -> list[dict[str, str]]:
        records = self.get_message_records(session_id)
        if limit is not None:
            records = records[-max(1, limit) :]
        return [{"role": record.role, "content": record.content} for record in records]

    def get_message_records(self, session_id: str) -> list[StoredMessage]:
        with self._lock:
            return list(self._messages.get(session_id, []))

    def append_turn(self, session_id: str, user_message: str, assistant_message: str) -> list[dict[str, str]]:
        with self._lock:
            summary = self._sessions.get(session_id)
            if summary is None:
                summary = self.ensure_session(session_id)

            title = summary.title
            if title == DEFAULT_SESSION_TITLE and user_message.strip():
                title = generate_session_title(user_message)

            timestamp = now_iso()
            existing_messages = list(self._messages.get(session_id, []))
            existing_messages.append(
                StoredMessage(
                    message_id=str(uuid.uuid4()),
                    session_id=session_id,
                    role="user",
                    content=user_message,
                    created_at=timestamp,
                )
            )
            existing_messages.append(
                StoredMessage(
                    message_id=str(uuid.uuid4()),
                    session_id=session_id,
                    role="assistant",
                    content=assistant_message,
                    created_at=timestamp,
                )
            )
            self._messages[session_id] = existing_messages
            updated = SessionSummary(
                session_id=session_id,
                title=title,
                created_at=summary.created_at,
                updated_at=timestamp,
                last_preview=build_preview(assistant_message or user_message),
                message_count=len(existing_messages),
            )
            self._sessions[session_id] = updated
            return [{"role": record.role, "content": record.content} for record in existing_messages[-self._max_history_messages :]]

    def append_event(
        self,
        session_id: str,
        event_type: str,
        payload: dict,
        plan_id: str | None = None,
    ) -> StoredEvent:
        with self._lock:
            self.ensure_session(session_id)
            event = StoredEvent(
                event_id=str(uuid.uuid4()),
                session_id=session_id,
                plan_id=plan_id,
                event_type=event_type,
                payload=dict(payload),
                created_at=now_iso(),
            )
            self._events.setdefault(session_id, []).append(event)
            return event

    def get_event_records(self, session_id: str, limit: int | None = None) -> list[StoredEvent]:
        with self._lock:
            records = list(self._events.get(session_id, []))
        if limit is not None:
            return records[-max(1, limit) :]
        return records

    def get_plan_event_records(self, plan_id: str, limit: int | None = None) -> list[StoredEvent]:
        with self._lock:
            records = [
                event
                for events in self._events.values()
                for event in events
                if event.plan_id == plan_id
            ]
        records.sort(key=lambda event: (event.created_at, event.event_id))
        if limit is not None:
            return records[-max(1, limit) :]
        return records


class SQLiteSessionStore:
    def __init__(self, db_path: str | Path, max_history_messages: int = 20) -> None:
        self._max_history_messages = max(1, max_history_messages)
        self._db_path = Path(db_path)
        self._db_path.parent.mkdir(parents=True, exist_ok=True)
        self._lock = Lock()
        self._conn = sqlite3.connect(self._db_path, check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        with self._conn:
            self._conn.execute("PRAGMA foreign_keys = ON")
        self._initialize_schema()

    @property
    def max_history_messages(self) -> int:
        return self._max_history_messages

    def create_session(self, title: str | None = None) -> SessionSummary:
        return self.ensure_session(str(uuid.uuid4()), title=title)

    def ensure_session(self, session_id: str, title: str | None = None) -> SessionSummary:
        with self._lock, self._conn:
            row = self._conn.execute(
                "SELECT id, title, created_at, updated_at, last_preview, message_count FROM sessions WHERE id = ?",
                (session_id,),
            ).fetchone()
            if row is not None:
                return self._row_to_summary(row)
            timestamp = now_iso()
            self._conn.execute(
                """
                INSERT INTO sessions (id, title, created_at, updated_at, last_preview, message_count)
                VALUES (?, ?, ?, ?, '', 0)
                """,
                (session_id, (title or DEFAULT_SESSION_TITLE).strip() or DEFAULT_SESSION_TITLE, timestamp, timestamp),
            )
            row = self._conn.execute(
                "SELECT id, title, created_at, updated_at, last_preview, message_count FROM sessions WHERE id = ?",
                (session_id,),
            ).fetchone()
            return self._row_to_summary(row)

    def get_session(self, session_id: str) -> SessionSummary | None:
        with self._lock:
            row = self._conn.execute(
                "SELECT id, title, created_at, updated_at, last_preview, message_count FROM sessions WHERE id = ?",
                (session_id,),
            ).fetchone()
        return self._row_to_summary(row) if row is not None else None

    def list_sessions(self, query: str | None = None, limit: int = 100) -> list[SessionSummary]:
        sql = """
            SELECT id, title, created_at, updated_at, last_preview, message_count
            FROM sessions
        """
        params: list[object] = []
        if query and query.strip():
            sql += " WHERE lower(title) LIKE ? OR lower(last_preview) LIKE ?"
            needle = f"%{query.strip().lower()}%"
            params.extend([needle, needle])
        sql += " ORDER BY updated_at DESC LIMIT ?"
        params.append(max(1, limit))
        with self._lock:
            rows = self._conn.execute(sql, tuple(params)).fetchall()
        return [self._row_to_summary(row) for row in rows]

    def update_session_title(self, session_id: str, title: str) -> SessionSummary | None:
        with self._lock, self._conn:
            self._conn.execute(
                "UPDATE sessions SET title = ?, updated_at = ? WHERE id = ?",
                (title.strip() or DEFAULT_SESSION_TITLE, now_iso(), session_id),
            )
            row = self._conn.execute(
                "SELECT id, title, created_at, updated_at, last_preview, message_count FROM sessions WHERE id = ?",
                (session_id,),
            ).fetchone()
        return self._row_to_summary(row) if row is not None else None

    def delete_session(self, session_id: str) -> bool:
        with self._lock, self._conn:
            cursor = self._conn.execute("DELETE FROM sessions WHERE id = ?", (session_id,))
            return cursor.rowcount > 0

    def get_messages(self, session_id: str, limit: int | None = None) -> list[dict[str, str]]:
        records = self.get_message_records(session_id)
        if limit is not None:
            records = records[-max(1, limit) :]
        return [{"role": record.role, "content": record.content} for record in records]

    def get_message_records(self, session_id: str) -> list[StoredMessage]:
        with self._lock:
            rows = self._conn.execute(
                """
                SELECT id, session_id, role, content, created_at
                FROM messages
                WHERE session_id = ?
                ORDER BY created_at ASC, id ASC
                """,
                (session_id,),
            ).fetchall()
        return [self._row_to_message(row) for row in rows]

    def append_turn(self, session_id: str, user_message: str, assistant_message: str) -> list[dict[str, str]]:
        self.ensure_session(session_id)
        with self._lock, self._conn:
            session = self._conn.execute(
                "SELECT id, title, created_at, updated_at, last_preview, message_count FROM sessions WHERE id = ?",
                (session_id,),
            ).fetchone()
            timestamp = now_iso()
            self._conn.execute(
                "INSERT INTO messages (id, session_id, role, content, created_at) VALUES (?, ?, 'user', ?, ?)",
                (str(uuid.uuid4()), session_id, user_message, timestamp),
            )
            self._conn.execute(
                "INSERT INTO messages (id, session_id, role, content, created_at) VALUES (?, ?, 'assistant', ?, ?)",
                (str(uuid.uuid4()), session_id, assistant_message, timestamp),
            )
            title = session["title"]
            if title == DEFAULT_SESSION_TITLE and user_message.strip():
                title = generate_session_title(user_message)
            count_row = self._conn.execute(
                "SELECT COUNT(*) AS count FROM messages WHERE session_id = ?",
                (session_id,),
            ).fetchone()
            self._conn.execute(
                """
                UPDATE sessions
                SET title = ?, updated_at = ?, last_preview = ?, message_count = ?
                WHERE id = ?
                """,
                (
                    title,
                    timestamp,
                    build_preview(assistant_message or user_message),
                    int(count_row["count"]),
                    session_id,
                ),
            )
        return self.get_messages(session_id, limit=self._max_history_messages)

    def append_event(
        self,
        session_id: str,
        event_type: str,
        payload: dict,
        plan_id: str | None = None,
    ) -> StoredEvent:
        self.ensure_session(session_id)
        event_id = str(uuid.uuid4())
        timestamp = now_iso()
        with self._lock, self._conn:
            self._conn.execute(
                """
                INSERT INTO events (id, session_id, plan_id, event_type, payload_json, created_at)
                VALUES (?, ?, ?, ?, ?, ?)
                """,
                (event_id, session_id, plan_id, event_type, json.dumps(payload, ensure_ascii=False), timestamp),
            )
        return StoredEvent(
            event_id=event_id,
            session_id=session_id,
            plan_id=plan_id,
            event_type=event_type,
            payload=dict(payload),
            created_at=timestamp,
        )

    def get_event_records(self, session_id: str, limit: int | None = None) -> list[StoredEvent]:
        sql = """
            SELECT id, session_id, plan_id, event_type, payload_json, created_at
            FROM events
            WHERE session_id = ?
            ORDER BY created_at ASC, id ASC
        """
        params: list[object] = [session_id]
        if limit is not None:
            sql = """
                SELECT id, session_id, plan_id, event_type, payload_json, created_at
                FROM (
                    SELECT id, session_id, plan_id, event_type, payload_json, created_at
                    FROM events
                    WHERE session_id = ?
                    ORDER BY created_at DESC, id DESC
                    LIMIT ?
                )
                ORDER BY created_at ASC, id ASC
            """
            params.append(max(1, limit))
        with self._lock:
            rows = self._conn.execute(sql, tuple(params)).fetchall()
        return [self._row_to_event(row) for row in rows]

    def get_plan_event_records(self, plan_id: str, limit: int | None = None) -> list[StoredEvent]:
        sql = """
            SELECT id, session_id, plan_id, event_type, payload_json, created_at
            FROM events
            WHERE plan_id = ?
            ORDER BY created_at ASC, id ASC
        """
        params: list[object] = [plan_id]
        if limit is not None:
            sql = """
                SELECT id, session_id, plan_id, event_type, payload_json, created_at
                FROM (
                    SELECT id, session_id, plan_id, event_type, payload_json, created_at
                    FROM events
                    WHERE plan_id = ?
                    ORDER BY created_at DESC, id DESC
                    LIMIT ?
                )
                ORDER BY created_at ASC, id ASC
            """
            params.append(max(1, limit))
        with self._lock:
            rows = self._conn.execute(sql, tuple(params)).fetchall()
        return [self._row_to_event(row) for row in rows]

    def _initialize_schema(self) -> None:
        with self._lock, self._conn:
            self._conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS sessions (
                    id TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL,
                    last_preview TEXT NOT NULL DEFAULT '',
                    message_count INTEGER NOT NULL DEFAULT 0
                );

                CREATE TABLE IF NOT EXISTS messages (
                    id TEXT PRIMARY KEY,
                    session_id TEXT NOT NULL,
                    role TEXT NOT NULL,
                    content TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    FOREIGN KEY(session_id) REFERENCES sessions(id) ON DELETE CASCADE
                );

                CREATE TABLE IF NOT EXISTS events (
                    id TEXT PRIMARY KEY,
                    session_id TEXT NOT NULL,
                    plan_id TEXT,
                    event_type TEXT NOT NULL,
                    payload_json TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    FOREIGN KEY(session_id) REFERENCES sessions(id) ON DELETE CASCADE
                );

                CREATE INDEX IF NOT EXISTS idx_sessions_updated_at ON sessions(updated_at DESC);
                CREATE INDEX IF NOT EXISTS idx_messages_session_created ON messages(session_id, created_at, id);
                CREATE INDEX IF NOT EXISTS idx_events_session_created ON events(session_id, created_at, id);
                CREATE INDEX IF NOT EXISTS idx_events_plan_created ON events(plan_id, created_at, id);
                """
            )

    @staticmethod
    def _row_to_summary(row: sqlite3.Row) -> SessionSummary:
        return SessionSummary(
            session_id=row["id"],
            title=row["title"],
            created_at=row["created_at"],
            updated_at=row["updated_at"],
            last_preview=row["last_preview"],
            message_count=int(row["message_count"]),
        )

    @staticmethod
    def _row_to_message(row: sqlite3.Row) -> StoredMessage:
        return StoredMessage(
            message_id=row["id"],
            session_id=row["session_id"],
            role=row["role"],
            content=row["content"],
            created_at=row["created_at"],
        )

    @staticmethod
    def _row_to_event(row: sqlite3.Row) -> StoredEvent:
        return StoredEvent(
            event_id=row["id"],
            session_id=row["session_id"],
            plan_id=row["plan_id"],
            event_type=row["event_type"],
            payload=json.loads(row["payload_json"]),
            created_at=row["created_at"],
        )
