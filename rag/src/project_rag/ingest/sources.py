"""
模块: ingest.sources
职责: 维护项目级 RAG 的知识源白名单，并对文件做基础分类。
核心对象: SourceSpec、DEFAULT_SOURCE_SPECS
依赖: pathlib、fnmatch
约束: 第一版只收明确约定的目录和文件，避免把整个仓库无差别入库。
输入: 仓库根目录。
输出: 待索引文件列表，以及每个文件的 project/module/source_type/language。
"""

from __future__ import annotations

from dataclasses import dataclass
from fnmatch import fnmatch
from pathlib import Path


@dataclass(frozen=True)
class SourceSpec:
    path: str
    project: str
    module: str
    patterns: tuple[str, ...]


DEFAULT_SOURCE_SPECS: tuple[SourceSpec, ...] = (
    SourceSpec("说明文档", "client", "docs", ("*.md",)),
    SourceSpec("src/agent", "client", "agent", ("*.h", "*.hpp", "*.cpp", "*.cc")),
    SourceSpec("src/audio", "client", "audio", ("*.h", "*.hpp", "*.cpp", "*.cc")),
    SourceSpec("src/video", "client", "video", ("*.h", "*.hpp", "*.cpp", "*.cc")),
    SourceSpec("src/network", "client", "network", ("*.h", "*.hpp", "*.cpp", "*.cc")),
    SourceSpec("src/viewmodels", "client", "viewmodels", ("*.h", "*.hpp", "*.cpp", "*.cc")),
    SourceSpec("src/app", "client", "app", ("*.h", "*.hpp", "*.cpp", "*.cc")),
    SourceSpec("qml", "client", "ui", ("*.qml",)),
    SourceSpec("agent/src/music_agent", "agent", "agent", ("*.py",)),
    SourceSpec("agent/README.md", "agent", "agent", ("README.md",)),
    SourceSpec("agent/提示文档.md", "agent", "agent", ("提示文档.md",)),
    SourceSpec("agent/Qt端对接建议.md", "agent", "agent", ("Qt端对接建议.md",)),
    SourceSpec(
        "agent/Claude Code方法论对当前音乐Agent的架构借鉴清单.md",
        "agent",
        "agent",
        ("Claude Code方法论对当前音乐Agent的架构借鉴清单.md",),
    ),
    SourceSpec("README.md", "client", "docs", ("README.md",)),
    SourceSpec("AUDIO_ARCHITECTURE.md", "client", "audio", ("AUDIO_ARCHITECTURE.md",)),
    SourceSpec("VIDEO_ARCHITECTURE.md", "client", "video", ("VIDEO_ARCHITECTURE.md",)),
    SourceSpec("VIDEO_AV_SYNC_STRATEGY.md", "client", "video", ("VIDEO_AV_SYNC_STRATEGY.md",)),
    SourceSpec("说明文档/接口与协议/接口变化.md", "server_doc", "protocol", ("接口变化.md",)),
    SourceSpec("说明文档/接口与协议/服务端新增接口.md", "server_doc", "protocol", ("服务端新增接口.md",)),
    SourceSpec("说明文档/接口与协议/在线视频接口新增.md", "server_doc", "protocol", ("在线视频接口新增.md",)),
    SourceSpec("说明文档/测试资料/用户音乐功能测试脚本.md", "server_doc", "case", ("用户音乐功能测试脚本.md",)),
    SourceSpec("rag/cases", "client", "case", ("*.md", "*.json")),
)


def discover_source_files(repo_root: Path) -> list[tuple[Path, SourceSpec]]:
    files: list[tuple[Path, SourceSpec]] = []
    for spec in DEFAULT_SOURCE_SPECS:
        target = repo_root / spec.path
        if target.is_file():
            files.append((target, spec))
            continue
        if not target.exists():
            continue
        for file_path in target.rglob("*"):
            if not file_path.is_file():
                continue
            if any(fnmatch(file_path.name, pattern) for pattern in spec.patterns):
                files.append((file_path, spec))
    return sorted(files, key=lambda item: str(item[0]).lower())


def _looks_like_protocol_doc(file_path: Path, spec: SourceSpec) -> bool:
    if spec.module == "protocol":
        return True
    path_text = file_path.as_posix().lower()
    protocol_keywords = (
        "api",
        "openapi",
        "protocol",
        "schema",
        "message",
        "messages",
        "tool",
        "script",
        "websocket",
        "/ws",
    )
    if any(keyword in path_text for keyword in protocol_keywords):
        return True

    localized_path = file_path.as_posix()
    localized_keywords = ("接口", "协议", "消息", "脚本", "联调", "对接")
    return any(keyword in localized_path for keyword in localized_keywords)


def classify_source(file_path: Path, spec: SourceSpec) -> tuple[str, str]:
    suffix = file_path.suffix.lower()
    if spec.module == "case":
        return "case", suffix.lstrip(".")
    if suffix in {".md", ".txt"}:
        source_type = "protocol" if _looks_like_protocol_doc(file_path, spec) else "doc"
        return source_type, suffix.lstrip(".")
    if suffix == ".qml":
        return "code", "qml"
    if suffix in {".py", ".cpp", ".cc", ".h", ".hpp"}:
        return "code", suffix.lstrip(".")
    return "doc", suffix.lstrip(".")
