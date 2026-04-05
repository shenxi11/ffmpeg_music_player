#!/usr/bin/env python3
"""
模块名称: git_commit_message_check
功能概述: 校验 Git 提交信息格式，推动团队使用清晰、稳定的提交说明。
对外接口: main()
依赖关系: argparse, re, sys, pathlib
输入输出: 输入为 Git commit-msg hook 传入的提交信息文件；输出为校验结果与退出码。
异常与错误: 提交信息文件不存在或格式不满足规则时返回非零退出码并说明原因。
维护说明: 默认采用宽松版 Conventional Commits，兼容中英文摘要，避免对正常开发造成过强摩擦。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ALLOWED_TYPES = (
    "feat",
    "fix",
    "refactor",
    "docs",
    "test",
    "build",
    "chore",
    "style",
    "perf",
    "ci",
    "revert",
)

HEADER_PATTERN = re.compile(
    r"^(?P<type>feat|fix|refactor|docs|test|build|chore|style|perf|ci|revert)"
    r"(?:\((?P<scope>[a-z0-9._/-]+)\))?"
    r"(?P<breaking>!)?: "
    r"(?P<summary>.+)$"
)

ALLOWED_SPECIAL_PREFIXES = ("Merge ", "Revert ", "fixup!", "squash!")


def read_commit_message(path: Path) -> str:
    for encoding in ("utf-8", "utf-8-sig", "gbk", "cp936", "latin-1"):
        try:
            return path.read_text(encoding=encoding)
        except UnicodeDecodeError:
            continue
    return path.read_text(encoding="utf-8", errors="replace")


def get_subject(message: str) -> str:
    for line in message.splitlines():
        stripped = line.strip()
        if stripped and not stripped.startswith("#"):
            return stripped
    return ""


def validate_subject(subject: str) -> list[str]:
    errors: list[str] = []
    if not subject:
        return ["Commit message subject is empty."]

    if subject.startswith(ALLOWED_SPECIAL_PREFIXES):
        return errors

    if len(subject) > 72:
        errors.append("Commit subject is longer than 72 characters.")

    match = HEADER_PATTERN.match(subject)
    if not match:
        allowed = ", ".join(ALLOWED_TYPES)
        errors.append(
            "Commit subject must follow 'type(scope): summary' or 'type: summary'. "
            f"Allowed types: {allowed}."
        )
        return errors

    summary = match.group("summary").strip()
    if len(summary) < 4:
        errors.append("Commit summary is too short; make it more descriptive.")

    if summary.endswith("."):
        errors.append("Commit summary should not end with a period.")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate git commit message format.")
    parser.add_argument("message_file", help="Path to the commit message file")
    args = parser.parse_args()

    message_path = Path(args.message_file)
    if not message_path.exists():
        print(f"Commit message file not found: {message_path}")
        return 1

    subject = get_subject(read_commit_message(message_path))
    errors = validate_subject(subject)

    if not errors:
        print(f"Commit message check passed: {subject}")
        return 0

    print("Commit message check failed:")
    for error in errors:
        print(f"  - {error}")
    print("Examples:")
    print("  - feat(playback): add playlist queue sync")
    print("  - fix(network): handle bootstrap timeout")
    print("  - refactor(ui): split main widget connections")
    print("Commit blocked. Update the commit message or use git commit --no-verify to bypass this hook.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
