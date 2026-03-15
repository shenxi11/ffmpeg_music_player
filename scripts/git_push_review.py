#!/usr/bin/env python3
"""
模块名称: git_push_review
功能概述: 在 Git 推送前对即将推送的提交范围执行较重的自动审查，包括 QML lint、可选格式检查和 CTest。
对外接口: main()
依赖关系: argparse, os, re, shutil, subprocess, sys, pathlib
输入输出: 输入为 pre-push hook 传入的 ref 信息或命令行参数；输出为审查结果和退出码。
异常与错误: Git、qmllint、ctest 或 clang-format 调用失败时返回非零退出码并说明原因。
维护说明: 优先保证可降级运行；工具缺失或仓库未配置测试时输出跳过信息，不盲目阻断。
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


CPP_EXTENSIONS = {".h", ".hpp", ".hh", ".hxx", ".cpp", ".cc", ".cxx"}
QML_EXTENSIONS = {".qml"}
UI_EXTENSIONS = {".ui"}
FORMAT_EXTENSIONS = CPP_EXTENSIONS
RELEVANT_EXTENSIONS = CPP_EXTENSIONS | QML_EXTENSIONS | UI_EXTENSIONS
ZERO_SHA = "0" * 40


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
    return Path(run_git(["rev-parse", "--show-toplevel"], cwd).stdout.strip())


def parse_hook_updates(stdin_text: str) -> list[tuple[str, str, str, str]]:
    updates = []
    for line in stdin_text.splitlines():
        parts = line.strip().split()
        if len(parts) != 4:
            continue
        updates.append((parts[0], parts[1], parts[2], parts[3]))
    return updates


def rev_parse_or_none(repo_root: Path, rev: str) -> str | None:
    completed = run_git(["rev-parse", rev], repo_root, check=False)
    if completed.returncode != 0:
        return None
    return completed.stdout.strip()


def collect_changed_files_for_update(repo_root: Path, local_sha: str, remote_sha: str) -> set[str]:
    files: set[str] = set()
    if remote_sha != ZERO_SHA:
        result = run_git(["diff", "--name-only", "--diff-filter=ACMR", f"{remote_sha}..{local_sha}"], repo_root)
        files.update(filter(None, result.stdout.splitlines()))
        return files

    rev_list = run_git(["rev-list", "--reverse", "--topo-order", "--not", "--remotes", local_sha], repo_root, check=False)
    commits = [commit for commit in rev_list.stdout.splitlines() if commit]
    if not commits:
        commits = [local_sha]
    for commit in commits:
        diff_tree = run_git(["diff-tree", "--no-commit-id", "--name-only", "--diff-filter=ACMR", "-r", commit], repo_root)
        files.update(filter(None, diff_tree.stdout.splitlines()))
    return files


def collect_changed_files(repo_root: Path, updates: list[tuple[str, str, str, str]]) -> set[str]:
    if updates:
        files: set[str] = set()
        for _, local_sha, _, remote_sha in updates:
            if local_sha == ZERO_SHA:
                continue
            files.update(collect_changed_files_for_update(repo_root, local_sha, remote_sha))
        return files

    upstream_sha = rev_parse_or_none(repo_root, "@{upstream}")
    head_sha = rev_parse_or_none(repo_root, "HEAD")
    if not head_sha:
        return set()
    if upstream_sha:
        result = run_git(["diff", "--name-only", "--diff-filter=ACMR", f"{upstream_sha}..{head_sha}"], repo_root)
        return set(filter(None, result.stdout.splitlines()))

    parent_sha = rev_parse_or_none(repo_root, "HEAD~1")
    if parent_sha:
        result = run_git(["diff", "--name-only", "--diff-filter=ACMR", f"{parent_sha}..{head_sha}"], repo_root)
        return set(filter(None, result.stdout.splitlines()))

    diff_tree = run_git(["diff-tree", "--no-commit-id", "--name-only", "--diff-filter=ACMR", "-r", head_sha], repo_root)
    return set(filter(None, diff_tree.stdout.splitlines()))


def detect_build_dir(repo_root: Path) -> Path | None:
    env_value = os.environ.get("GIT_PUSH_REVIEW_BUILD_DIR", "").strip()
    if env_value:
        candidate = Path(env_value)
        if not candidate.is_absolute():
            candidate = repo_root / candidate
        if (candidate / "CMakeCache.txt").exists():
            return candidate

    direct_build = repo_root / "build"
    if (direct_build / "CMakeCache.txt").exists():
        return direct_build

    if direct_build.exists():
        for cache in direct_build.rglob("CMakeCache.txt"):
            return cache.parent
    return None


def print_section(title: str) -> None:
    print(f"\n[{title}]")


def detect_clang_format_config(repo_root: Path) -> Path | None:
    for name in (".clang-format", "_clang-format"):
        candidate = repo_root / name
        if candidate.exists():
            return candidate
    return None


def run_qmllint(repo_root: Path, changed_files: set[str]) -> bool:
    qmllint = shutil.which("qmllint")
    qml_files = sorted(path for path in changed_files if Path(path).suffix.lower() in QML_EXTENSIONS)
    if not qml_files:
        print("qmllint: skipped (no changed QML files)")
        return True
    if not qmllint:
        print("qmllint: skipped (tool not found)")
        return True

    import_paths = []
    for candidate in (repo_root / "qml", repo_root / "qml" / "components", repo_root / "plugins"):
        if candidate.exists():
            import_paths.extend(["-I", str(candidate)])

    success = True
    for relative_path in qml_files:
        target = repo_root / relative_path
        command = [qmllint, *import_paths, str(target)]
        completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace", check=False)
        if completed.returncode == 0:
            print(f"qmllint: ok {relative_path}")
            continue
        success = False
        print(f"qmllint: failed {relative_path}")
        if completed.stdout.strip():
            print(completed.stdout.strip())
        if completed.stderr.strip():
            print(completed.stderr.strip())
    return success


def run_clang_format_check(repo_root: Path, changed_files: set[str]) -> bool:
    if os.environ.get("GIT_PUSH_REVIEW_SKIP_FORMAT") == "1":
        print("clang-format: skipped by GIT_PUSH_REVIEW_SKIP_FORMAT=1")
        return True

    format_config = detect_clang_format_config(repo_root)
    if not format_config:
        print("clang-format: skipped (no .clang-format or _clang-format in repo root)")
        return True

    clang_format = shutil.which("clang-format")
    cpp_files = sorted(path for path in changed_files if Path(path).suffix.lower() in FORMAT_EXTENSIONS)
    if not cpp_files:
        print("clang-format: skipped (no changed C++ files)")
        return True
    if not clang_format:
        print("clang-format: skipped (tool not found)")
        return True

    success = True
    for relative_path in cpp_files:
        target = repo_root / relative_path
        command = [clang_format, "--style=file", "--dry-run", "--Werror", str(target)]
        completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace", check=False)
        if completed.returncode == 0:
            print(f"clang-format: ok {relative_path}")
            continue
        success = False
        print(f"clang-format: failed {relative_path}")
        if completed.stdout.strip():
            print(completed.stdout.strip())
        if completed.stderr.strip():
            print(completed.stderr.strip())
    return success


def parse_total_tests(output: str) -> int | None:
    match = re.search(r"Total Tests:\s*(\d+)", output)
    if not match:
        return None
    return int(match.group(1))


def run_ctest(repo_root: Path, changed_files: set[str]) -> bool:
    if os.environ.get("GIT_PUSH_REVIEW_SKIP_TESTS") == "1":
        print("ctest: skipped by GIT_PUSH_REVIEW_SKIP_TESTS=1")
        return True

    if not any(Path(path).suffix.lower() in RELEVANT_EXTENSIONS or Path(path).name == "CMakeLists.txt" for path in changed_files):
        print("ctest: skipped (no changed code or build files)")
        return True

    ctest = shutil.which("ctest")
    if not ctest:
        print("ctest: skipped (tool not found)")
        return True

    build_dir = detect_build_dir(repo_root)
    if not build_dir:
        print("ctest: skipped (no build directory with CMakeCache.txt found)")
        return True

    list_result = subprocess.run(
        [ctest, "--test-dir", str(build_dir), "-N"],
        cwd=str(repo_root),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    combined_output = (list_result.stdout or "") + ("\n" + list_result.stderr if list_result.stderr else "")
    total_tests = parse_total_tests(combined_output)
    if list_result.returncode != 0:
        print(f"ctest: failed to enumerate tests in {build_dir}")
        if combined_output.strip():
            print(combined_output.strip())
        return False
    if total_tests == 0:
        print(f"ctest: skipped (build dir {build_dir} has 0 tests)")
        return True

    completed = subprocess.run(
        [ctest, "--test-dir", str(build_dir), "--output-on-failure"],
        cwd=str(repo_root),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode == 0:
        print(f"ctest: ok ({total_tests} tests) in {build_dir}")
        return True

    print(f"ctest: failed in {build_dir}")
    if completed.stdout.strip():
        print(completed.stdout.strip())
    if completed.stderr.strip():
        print(completed.stderr.strip())
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Run heavier automated review checks before git push.")
    parser.add_argument("--stdin-file", help="Read pre-push ref updates from a file instead of stdin")
    args = parser.parse_args()

    repo_root = get_repo_root(Path.cwd())
    if args.stdin_file:
        stdin_text = Path(args.stdin_file).read_text(encoding="utf-8")
    else:
        stdin_text = sys.stdin.read()

    updates = parse_hook_updates(stdin_text)
    changed_files = {
        path
        for path in collect_changed_files(repo_root, updates)
        if Path(path).suffix.lower() in RELEVANT_EXTENSIONS or Path(path).name == "CMakeLists.txt"
    }

    print("Git push review: analyzing committed changes to be pushed.")
    if not changed_files:
        print("No changed code files detected for push review.")
        return 0

    for path in sorted(changed_files):
        print(f"  - {path}")

    results: list[bool] = []

    print_section("QML Lint")
    results.append(run_qmllint(repo_root, changed_files))

    print_section("Format Check")
    results.append(run_clang_format_check(repo_root, changed_files))

    print_section("CTest")
    results.append(run_ctest(repo_root, changed_files))

    if all(results):
        print("\nGit push review passed.")
        return 0

    print("\nGit push review failed. Fix the issues or use git push --no-verify to bypass this hook.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
