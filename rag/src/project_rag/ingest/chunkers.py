"""
???: ingest.chunkers
????: ???????????????????????????
????: chunk_file
????: pathlib?hashlib?json?re?schemas.chunks
????: ?????????????????????
?????: ? UTF-8 ???????????????????????
????: ??????????????????????????????????????
"""

from __future__ import annotations

import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path

from project_rag.schemas.chunks import ChunkMetadata, KnowledgeChunk

_HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$")
_PY_FUNC_RE = re.compile(r"^\s*def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(")
_PY_CLASS_RE = re.compile(r"^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)\b")
_CPP_CLASS_RE = re.compile(r"^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)\b")
_CPP_FUNC_RE = re.compile(r"^\s*(?:[\w:\<\>\*&]+\s+)+([A-Za-z_][A-Za-z0-9_:]*)\s*\([^;]*\)\s*(?:const)?\s*(?:\{|$)")
_HTTP_RE = re.compile(r"\b(GET|POST|PUT|PATCH|DELETE)\s+(/[A-Za-z0-9_\-./{}]+)")
_MESSAGE_TYPE_RE = re.compile(r'"type"\s*:\s*"([A-Za-z0-9_\-]+)"')


@dataclass(frozen=True)
class FileContext:
    repo_root: Path
    file_path: Path
    project: str
    module: str
    source_type: str
    language: str


def _read_text(file_path: Path) -> str:
    return file_path.read_text(encoding="utf-8", errors="replace")


def _relative_path(repo_root: Path, file_path: Path) -> str:
    return file_path.resolve().relative_to(repo_root.resolve()).as_posix()


def _make_hash(*parts: str) -> str:
    digest = hashlib.sha1()
    for part in parts:
        digest.update(part.encode("utf-8"))
    return digest.hexdigest()


def _truncate_summary(text: str, limit: int = 220) -> str:
    normalized = re.sub(r"\s+", " ", text.strip())
    return normalized if len(normalized) <= limit else normalized[: limit - 3] + "..."


def _build_chunk(index_name: str, ctx: FileContext, title: str, text: str, *, symbol: str = "", heading_path: list[str] | None = None, class_name: str = "", function_name: str = "", message_type: str = "", http_method: str = "", http_path: str = "", issue_type: str = "", severity: str = "", tags: list[str] | None = None) -> KnowledgeChunk:
    heading_path = heading_path or []
    tags = tags or []
    relative_path = _relative_path(ctx.repo_root, ctx.file_path)
    chunk_hash = _make_hash(relative_path, title, symbol, text)
    if ctx.source_type == "doc":
        stable_seed = "|".join([relative_path, "/".join(heading_path), chunk_hash])
    elif ctx.source_type == "code":
        stable_seed = "|".join([relative_path, symbol or title, chunk_hash])
    elif ctx.source_type == "protocol":
        stable_seed = "|".join([message_type or f"{http_method} {http_path}".strip(), chunk_hash])
    else:
        stable_seed = "|".join([title, relative_path, chunk_hash])
    stable_id = _make_hash(stable_seed)
    metadata = ChunkMetadata(
        project=ctx.project,
        source_type=ctx.source_type,
        module=ctx.module,
        file_path=relative_path,
        title=title,
        symbol=symbol,
        language=ctx.language,
        heading_path=heading_path,
        tags=tags,
        updated_at=str(ctx.file_path.stat().st_mtime_ns),
        chunk_hash=chunk_hash,
        stable_id=stable_id,
        class_name=class_name,
        function_name=function_name,
        message_type=message_type,
        http_method=http_method,
        http_path=http_path,
        issue_type=issue_type,
        severity=severity,
    )
    return KnowledgeChunk(stable_id=stable_id, index_name=index_name, text=text.strip(), summary=_truncate_summary(text), metadata=metadata)


def _extract_surrounding_text(text: str, marker: str, radius: int = 260) -> str:
    index = text.find(marker)
    if index < 0:
        return marker
    start = max(0, index - radius)
    end = min(len(text), index + len(marker) + radius)
    return text[start:end].strip()


def _chunk_markdown(ctx: FileContext, text: str) -> list[KnowledgeChunk]:
    lines = text.splitlines()
    chunks: list[KnowledgeChunk] = []
    current_heading_path: list[str] = []
    buffer: list[str] = []
    current_title = ctx.file_path.stem

    def flush() -> None:
        nonlocal buffer, current_title
        body = "\n".join(buffer).strip()
        if not body:
            buffer = []
            return
        chunks.append(_build_chunk("protocol_index" if ctx.source_type == "protocol" else "docs_index", ctx, current_title, body, heading_path=list(current_heading_path), tags=[ctx.module, ctx.project]))
        buffer = []

    for line in lines:
        heading_match = _HEADING_RE.match(line)
        if heading_match:
            flush()
            level = len(heading_match.group(1))
            title = heading_match.group(2).strip()
            current_heading_path = current_heading_path[: level - 1] + [title]
            current_title = " / ".join(current_heading_path)
            continue
        buffer.append(line)
    flush()

    if ctx.source_type == "protocol":
        for method, path in _HTTP_RE.findall(text):
            chunks.append(_build_chunk("protocol_index", ctx, f"{method} {path}", f"{method} {path}\n{_extract_surrounding_text(text, f'{method} {path}')}", symbol=f"{method} {path}", http_method=method, http_path=path, tags=["protocol", ctx.module]))
        for message_type in sorted(set(_MESSAGE_TYPE_RE.findall(text))):
            chunks.append(_build_chunk("protocol_index", ctx, f"message:{message_type}", _extract_surrounding_text(text, f'"type": "{message_type}"'), symbol=message_type, message_type=message_type, tags=["protocol", "message_type"]))
    return chunks


def _chunk_python(ctx: FileContext, text: str) -> list[KnowledgeChunk]:
    chunks: list[KnowledgeChunk] = []
    lines = text.splitlines()
    current_block: list[str] = []
    current_symbol = ""
    current_class = ""

    def flush() -> None:
        nonlocal current_block, current_symbol, current_class
        body = "\n".join(current_block).strip()
        if not body:
            current_block = []
            return
        function_name = current_symbol.split(".")[-1] if current_symbol and current_symbol != current_class else ""
        chunks.append(_build_chunk("code_index", ctx, current_symbol or ctx.file_path.stem, body, symbol=current_symbol or ctx.file_path.stem, class_name=current_class, function_name=function_name, tags=["code", ctx.module]))
        current_block = []

    for line in lines:
        class_match = _PY_CLASS_RE.match(line)
        func_match = _PY_FUNC_RE.match(line)
        if class_match:
            flush()
            current_class = class_match.group(1)
            current_symbol = current_class
        elif func_match:
            flush()
            function_name = func_match.group(1)
            current_symbol = f"{current_class}.{function_name}" if current_class else function_name
        current_block.append(line)
    flush()
    return chunks


def _chunk_cpp_like(ctx: FileContext, text: str) -> list[KnowledgeChunk]:
    chunks: list[KnowledgeChunk] = []
    lines = text.splitlines()
    current_block: list[str] = []
    current_symbol = ""
    current_class = ""

    def flush() -> None:
        nonlocal current_block, current_symbol, current_class
        body = "\n".join(current_block).strip()
        if not body:
            current_block = []
            return
        function_name = current_symbol.split("::")[-1] if "::" in current_symbol else ""
        chunks.append(_build_chunk("code_index", ctx, current_symbol or ctx.file_path.stem, body, symbol=current_symbol or ctx.file_path.stem, class_name=current_class, function_name=function_name, tags=["code", ctx.module]))
        current_block = []

    for line in lines:
        class_match = _CPP_CLASS_RE.match(line)
        func_match = _CPP_FUNC_RE.match(line)
        if class_match:
            flush()
            current_class = class_match.group(1)
            current_symbol = current_class
        elif func_match and "(" in line and not line.strip().startswith(("if", "for", "while", "switch")):
            flush()
            current_symbol = func_match.group(1)
        current_block.append(line)
    flush()
    return chunks


def _chunk_case(ctx: FileContext, text: str) -> list[KnowledgeChunk]:
    issue_type = "incident"
    severity = "medium"
    if ctx.file_path.suffix.lower() == ".json":
        try:
            payload = json.loads(text)
            title = payload.get("title", ctx.file_path.stem)
            issue_type = payload.get("issue_type", issue_type)
            severity = payload.get("severity", severity)
            return [_build_chunk("case_index", ctx, title, json.dumps(payload, ensure_ascii=False, indent=2), issue_type=issue_type, severity=severity, tags=["case", issue_type])]
        except json.JSONDecodeError:
            pass
    title = ctx.file_path.stem
    heading_match = _HEADING_RE.search(text)
    if heading_match:
        title = heading_match.group(2).strip()
    return [_build_chunk("case_index", ctx, title, text, issue_type=issue_type, severity=severity, tags=["case", issue_type])]


def chunk_file(repo_root: Path, file_path: Path, *, project: str, module: str, source_type: str, language: str) -> list[KnowledgeChunk]:
    ctx = FileContext(repo_root=repo_root, file_path=file_path, project=project, module=module, source_type=source_type, language=language)
    text = _read_text(file_path)
    if source_type in {"doc", "protocol"}:
        return _chunk_markdown(ctx, text)
    if source_type == "case":
        return _chunk_case(ctx, text)
    if language == "py":
        return _chunk_python(ctx, text)
    return _chunk_cpp_like(ctx, text)
