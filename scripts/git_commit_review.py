#!/usr/bin/env python3
"""
模块名称: git_commit_review
功能概述: 在 Git 提交前检查暂存区中的代码变更，并输出可执行的 code review 结果。
对外接口: main()
依赖关系: argparse, dataclasses, pathlib, re, subprocess, sys
输入输出: 输入为 Git 暂存区中的文件与变更行；输出为终端审查结果与退出码。
异常与错误: Git 命令失败或仓库状态异常时返回非零退出码并输出原因。
维护说明: 仅检查暂存区内容，优先高价值低误报规则，避免对历史遗留问题产生过多噪音。
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


CPP_EXTENSIONS = {".h", ".hpp", ".hh", ".hxx", ".cpp", ".cc", ".cxx"}
QML_EXTENSIONS = {".qml"}
UI_EXTENSIONS = {".ui"}
SUPPORTED_EXTENSIONS = CPP_EXTENSIONS | QML_EXTENSIONS | UI_EXTENSIONS

GETTER_PATTERN = re.compile(r"\b(get[A-Z][A-Za-z0-9_]*)\s*\(")
SET_IS_PATTERN = re.compile(r"\b(setIs[A-Z][A-Za-z0-9_]*)\s*\(")
SLOT_PREFIX_PATTERN = re.compile(r"\b(slot[A-Z][A-Za-z0-9_]*)\s*\(")
SIGNAL_PREFIX_PATTERN = re.compile(r"\b(sig[A-Z][A-Za-z0-9_]*)\s*\(")
QML_HANDLER_STYLE_PATTERN = re.compile(r"\b(on[A-Z][A-Za-z0-9_]*)\s*\(")
CLASS_PATTERN = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\b")
ENUM_PATTERN = re.compile(r"^\s*enum(?:\s+class)?\s+([A-Za-z_][A-Za-z0-9_]*)\b")
QPROPERTY_PATTERN = re.compile(r"Q_PROPERTY\s*\((?P<body>.*?)\)", re.DOTALL)
NOTIFY_PATTERN = re.compile(r"\bNOTIFY\s+([A-Za-z_][A-Za-z0-9_]*)")
UI_NAME_PATTERN = re.compile(r'\bname="([^"]+)"')
DIFF_HUNK_PATTERN = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


@dataclass(frozen=True)
class Finding:
    severity: str
    rule: str
    path: str
    line: int
    message: str
    suggestion: str


def run_git(args: list[str], cwd: Path, check: bool = True) -> subprocess.CompletedProcess:
    completed = subprocess.run(
        ["git", "-c", "core.quotepath=false", *args],
        cwd=str(cwd),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if check and completed.returncode != 0:
        raise RuntimeError(completed.stderr.strip() or completed.stdout.strip() or f"git {' '.join(args)} failed")
    return completed


def get_repo_root(cwd: Path) -> Path:
    result = run_git(["rev-parse", "--show-toplevel"], cwd)
    return Path(result.stdout.strip())


def get_staged_files(repo_root: Path) -> list[str]:
    result = run_git(["diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z"], repo_root)
    raw = result.stdout
    if not raw:
        return []
    return [path for path in raw.split("\x00") if path]


def get_changed_lines(repo_root: Path, path: str) -> set[int]:
    result = run_git(["diff", "--cached", "--unified=0", "--", path], repo_root)
    changed_lines: set[int] = set()
    for line in result.stdout.splitlines():
        match = DIFF_HUNK_PATTERN.match(line)
        if not match:
            continue
        start = int(match.group(1))
        count = int(match.group(2) or "1")
        if count == 0:
            continue
        changed_lines.update(range(start, start + count))
    return changed_lines


def get_staged_text(repo_root: Path, path: str) -> str:
    result = subprocess.run(
        ["git", "show", f":{path}"],
        cwd=str(repo_root),
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace").strip() or f"Unable to read staged file: {path}")
    data = result.stdout
    for encoding in ("utf-8", "utf-8-sig", "gbk", "cp936", "latin-1"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    return data.decode("utf-8", errors="replace")


def lower_camel_from_getter(symbol: str) -> str:
    stem = symbol[3:]
    return f"{stem[:1].lower()}{stem[1:]}" if stem else symbol


def upper_camel_from_snake(name: str) -> str:
    parts = [part for part in re.split(r"[_\s]+", name) if part]
    if not parts:
        return "TypeName"
    return "".join(part[:1].upper() + part[1:] for part in parts)


def is_upper_camel_case(name: str) -> bool:
    return bool(name) and name[0].isupper() and "_" not in name


def is_lower_camel_case(name: str) -> bool:
    return bool(name) and name[0].islower() and "_" not in name


def line_number_from_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def overlaps_changed_lines(start_line: int, end_line: int, changed_lines: set[int]) -> bool:
    for line in range(start_line, end_line + 1):
        if line in changed_lines:
            return True
    return False


def append_finding(findings: list[Finding], severity: str, rule: str, path: str, line: int, message: str, suggestion: str) -> None:
    findings.append(Finding(severity=severity, rule=rule, path=path, line=line, message=message, suggestion=suggestion))


def audit_cpp_file(path: str, text: str, changed_lines: set[int], findings: list[Finding]) -> None:
    lines = text.splitlines()
    for line_number, line in enumerate(lines, start=1):
        if line_number not in changed_lines:
            continue

        if line.startswith(("<<<<<<<", "=======", ">>>>>>>")):
            append_finding(
                findings,
                "error",
                "merge-conflict-marker",
                path,
                line_number,
                "Conflict marker found in staged content.",
                "Resolve the merge conflict before committing.",
            )

        class_match = CLASS_PATTERN.match(line)
        if class_match:
            symbol = class_match.group(1)
            if symbol not in {"signals", "slots"} and not is_upper_camel_case(symbol):
                append_finding(
                    findings,
                    "warning",
                    "cpp-class-name-case",
                    path,
                    line_number,
                    f"Class or struct name '{symbol}' does not look like UpperCamelCase.",
                    f"Prefer an UpperCamelCase name such as '{upper_camel_from_snake(symbol)}'.",
                )

        enum_match = ENUM_PATTERN.match(line)
        if enum_match:
            symbol = enum_match.group(1)
            if not is_upper_camel_case(symbol):
                append_finding(
                    findings,
                    "warning",
                    "cpp-enum-name-case",
                    path,
                    line_number,
                    f"Enum type name '{symbol}' does not look like UpperCamelCase.",
                    f"Prefer an UpperCamelCase enum type such as '{upper_camel_from_snake(symbol)}'.",
                )

        for match in GETTER_PATTERN.finditer(line):
            symbol = match.group(1)
            append_finding(
                findings,
                "warning",
                "cpp-getter-prefix",
                path,
                line_number,
                f"C++ getter '{symbol}' uses a Java-style 'get' prefix; Qt APIs usually prefer property-style getters.",
                f"Prefer '{lower_camel_from_getter(symbol)}()' if repository style allows.",
            )

        for match in SET_IS_PATTERN.finditer(line):
            symbol = match.group(1)
            stem = symbol[5:]
            append_finding(
                findings,
                "warning",
                "cpp-bool-setter-prefix",
                path,
                line_number,
                f"Bool setter '{symbol}' uses 'setIsXxx'; Qt APIs usually prefer 'setXxx'.",
                f"Prefer 'set{stem}(...)' and keep the getter/notifier family aligned.",
            )

        for match in SLOT_PREFIX_PATTERN.finditer(line):
            symbol = match.group(1)
            append_finding(
                findings,
                "warning",
                "cpp-slot-prefix",
                path,
                line_number,
                f"Method '{symbol}' uses a 'slot' prefix, which is usually unnecessary in Qt naming.",
                "Rename it to an action or handler phrase without the 'slot' prefix.",
            )

        for match in SIGNAL_PREFIX_PATTERN.finditer(line):
            symbol = match.group(1)
            append_finding(
                findings,
                "warning",
                "cpp-signal-prefix",
                path,
                line_number,
                f"Method '{symbol}' uses a 'sig' prefix, which is not idiomatic for Qt signals.",
                "Rename it to an event-style signal such as '<property>Changed' or '<event>Occurred'.",
            )

        for match in QML_HANDLER_STYLE_PATTERN.finditer(line):
            symbol = match.group(1)
            if symbol.startswith("on_"):
                continue
            append_finding(
                findings,
                "info",
                "cpp-qml-handler-style",
                path,
                line_number,
                f"C++ method '{symbol}' looks like a QML handler name.",
                "If this is a manual slot or helper, prefer a normal method name such as 'handleXxx(...)'.",
            )

    for match in QPROPERTY_PATTERN.finditer(text):
        start_line = line_number_from_offset(text, match.start())
        end_line = line_number_from_offset(text, match.end())
        if not overlaps_changed_lines(start_line, end_line, changed_lines):
            continue
        body = match.group("body")
        notify_match = NOTIFY_PATTERN.search(body)
        if not notify_match:
            continue
        notifier = notify_match.group(1)
        if notifier.endswith("Changed"):
            continue
        append_finding(
            findings,
            "warning",
            "qproperty-notify-name",
            path,
            start_line,
            f"Q_PROPERTY notifier '{notifier}' does not end with 'Changed'.",
            "Prefer '<property>Changed' unless compatibility or local conventions require otherwise.",
        )


def audit_qml_file(path: str, changed_lines: set[int], findings: list[Finding]) -> None:
    if 1 not in changed_lines:
        return
    stem = Path(path).stem
    if stem and stem[0].islower():
        append_finding(
            findings,
            "info",
            "qml-type-file-case",
            path,
            1,
            f"QML file '{Path(path).name}' starts with a lowercase letter.",
            "If this file defines a reusable QML type, rename it to 'UpperCamelCase.qml'.",
        )


def audit_ui_file(path: str, text: str, changed_lines: set[int], findings: list[Finding]) -> None:
    for line_number, line in enumerate(text.splitlines(), start=1):
        if line_number not in changed_lines:
            continue
        for match in UI_NAME_PATTERN.finditer(line):
            symbol = match.group(1)
            if is_lower_camel_case(symbol):
                continue
            append_finding(
                findings,
                "info",
                "ui-object-name-case",
                path,
                line_number,
                f"UI object name '{symbol}' does not look like lowerCamelCase.",
                "Prefer lowerCamelCase object names such as 'playButton' for Designer objects.",
            )


def collect_whitespace_errors(repo_root: Path) -> list[Finding]:
    result = run_git(["diff", "--cached", "--check"], repo_root, check=False)
    findings: list[Finding] = []
    if result.returncode == 0:
        return findings
    for raw_line in result.stdout.splitlines():
        parts = raw_line.split(":", 2)
        if len(parts) < 3:
            continue
        path, line_text, message = parts
        try:
            line_number = int(line_text)
        except ValueError:
            line_number = 1
        append_finding(
            findings,
            "error",
            "git-diff-check",
            path,
            line_number,
            message.strip(),
            "Fix the staged diff issue before committing.",
        )
    return findings


def collect_findings(repo_root: Path) -> list[Finding]:
    findings = collect_whitespace_errors(repo_root)
    for path in get_staged_files(repo_root):
        suffix = Path(path).suffix.lower()
        if suffix not in SUPPORTED_EXTENSIONS:
            continue
        changed_lines = get_changed_lines(repo_root, path)
        if not changed_lines:
            continue
        text = get_staged_text(repo_root, path)
        if suffix in CPP_EXTENSIONS:
            audit_cpp_file(path, text, changed_lines, findings)
        elif suffix in QML_EXTENSIONS:
            audit_qml_file(path, changed_lines, findings)
        elif suffix in UI_EXTENSIONS:
            audit_ui_file(path, text, changed_lines, findings)
    findings.sort(key=lambda item: (item.path, item.line, item.severity, item.rule))
    return findings


def print_findings(findings: list[Finding]) -> None:
    if not findings:
        print("Git commit review passed: no staged review findings.")
        return

    print("Git commit review found staged issues:")
    for finding in findings:
        print(f"[{finding.severity}] {finding.path}:{finding.line} {finding.rule}")
        print(f"  {finding.message}")
        print(f"  Suggestion: {finding.suggestion}")


def should_block(findings: list[Finding], fail_on_info: bool) -> bool:
    blocking_levels = {"error", "warning"}
    if fail_on_info:
        blocking_levels.add("info")
    return any(finding.severity in blocking_levels for finding in findings)


def main() -> int:
    parser = argparse.ArgumentParser(description="Review staged Qt/C++ changes before git commit.")
    parser.add_argument("--fail-on-info", action="store_true", help="Treat info-level findings as blocking")
    args = parser.parse_args()

    cwd = Path.cwd()
    try:
        repo_root = get_repo_root(cwd)
        findings = collect_findings(repo_root)
    except RuntimeError as exc:
        print(f"Git commit review failed: {exc}")
        return 1

    print_findings(findings)
    if should_block(findings, args.fail_on_info):
        print("Commit blocked. Fix the findings or use git commit --no-verify to bypass this hook.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
