#!/usr/bin/env python3
"""
模块名称: cloudmusic_remote
功能概述: 为 CloudMusic 虚拟机提供统一的远程探测、执行、文件传输与正式部署入口。
对外接口: main()
依赖关系: argparse, getpass, os, posixpath, shlex, sys, dataclasses, pathlib, paramiko
输入输出: 输入为命令行参数与环境变量；输出为远端命令结果、文件内容和错误信息。
异常与错误: SSH 认证失败、路径不存在或远端命令失败时返回非零退出码并输出原因。
维护说明: 默认目录约定为 microservice-deploy 开发目录和 CloudMusic 正式部署目录，必要时可通过参数覆盖。
"""

from __future__ import annotations

import argparse
import getpass
import os
import posixpath
import shlex
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

try:
    import paramiko
except ImportError as exc:  # pragma: no cover - 环境缺失时直接终止
    print("缺少依赖 paramiko。请先执行 `python -m pip install paramiko`。", file=sys.stderr)
    raise SystemExit(2) from exc


DEFAULT_HOST = "192.168.1.208"
DEFAULT_PORT = 22
DEFAULT_USER = "shen"
DEFAULT_KEY_FILE = Path.home() / ".ssh" / "id_ed25519_cloudmusic_vm"
DEFAULT_REPOS = {
    "dev": "/home/shen/microservice-deploy",
    "prod": "/home/shen/CloudMusic",
}
PASSWORD_ENV_VAR = "CLOUDMUSIC_VM_PASSWORD"


class RemoteCommandError(RuntimeError):
    """远端命令执行失败。"""

    def __init__(self, command: str, exit_status: int, stderr: str) -> None:
        self.command = command
        self.exit_status = exit_status
        self.stderr = stderr
        detail = stderr.strip() or f"远端命令失败，退出码 {exit_status}"
        super().__init__(detail)


@dataclass(slots=True)
class RemoteSettings:
    host: str
    port: int
    user: str
    key_file: Path
    password_env_var: str
    repo_dev: str
    repo_prod: str
    strict_host_key: bool

    def repo_root(self, repo_name: str | None) -> str | None:
        if repo_name is None:
            return None
        if repo_name == "dev":
            return self.repo_dev
        if repo_name == "prod":
            return self.repo_prod
        raise ValueError(f"未知仓库标识: {repo_name}")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="CloudMusic 虚拟机远程运维工具。",
    )
    parser.add_argument("--host", default=os.environ.get("CLOUDMUSIC_VM_HOST", DEFAULT_HOST))
    parser.add_argument("--port", type=int, default=int(os.environ.get("CLOUDMUSIC_VM_PORT", DEFAULT_PORT)))
    parser.add_argument("--user", default=os.environ.get("CLOUDMUSIC_VM_USER", DEFAULT_USER))
    parser.add_argument(
        "--key-file",
        default=os.environ.get("CLOUDMUSIC_VM_KEY_FILE", str(DEFAULT_KEY_FILE)),
        help="SSH 私钥路径。默认优先读取专用 key。",
    )
    parser.add_argument(
        "--password-env",
        default=os.environ.get("CLOUDMUSIC_VM_PASSWORD_ENV", PASSWORD_ENV_VAR),
        help="保存 SSH 密码的环境变量名。",
    )
    parser.add_argument(
        "--repo-dev",
        default=os.environ.get("CLOUDMUSIC_VM_REPO_DEV", DEFAULT_REPOS["dev"]),
        help="开发测试目录。",
    )
    parser.add_argument(
        "--repo-prod",
        default=os.environ.get("CLOUDMUSIC_VM_REPO_PROD", DEFAULT_REPOS["prod"]),
        help="正式部署目录。",
    )
    parser.add_argument(
        "--auth",
        choices=("auto", "key", "password"),
        default=os.environ.get("CLOUDMUSIC_VM_AUTH", "auto"),
        help="认证方式。auto 会先尝试 key，再回退到密码。",
    )
    parser.add_argument(
        "--strict-host-key",
        action="store_true",
        help="启用严格 host key 校验；默认首次连接自动接受。",
    )

    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    probe_parser = subparsers.add_parser("probe", help="检查远端目录、Git 状态与容器状态。")
    probe_parser.add_argument("--timeout", type=int, default=20)

    exec_parser = subparsers.add_parser("exec", help="执行任意远端命令。")
    exec_parser.add_argument("--repo", choices=("dev", "prod"))
    exec_parser.add_argument("--timeout", type=int, default=120)
    exec_parser.add_argument("command", nargs=argparse.REMAINDER, help="要执行的命令。")

    read_parser = subparsers.add_parser("read", help="读取远端文件并输出到标准输出。")
    read_parser.add_argument("--repo", choices=("dev", "prod"))
    read_parser.add_argument("path", help="远端路径；指定 --repo 时可传相对路径。")

    upload_parser = subparsers.add_parser("upload", help="上传本地文件到远端。")
    upload_parser.add_argument("--repo", choices=("dev", "prod"))
    upload_parser.add_argument("--chmod", help="上传后设置八进制权限，例如 644。")
    upload_parser.add_argument("local_path", help="本地文件路径。")
    upload_parser.add_argument("remote_path", help="远端路径；指定 --repo 时可传相对路径。")

    download_parser = subparsers.add_parser("download", help="下载远端文件到本地。")
    download_parser.add_argument("--repo", choices=("dev", "prod"))
    download_parser.add_argument("remote_path", help="远端路径；指定 --repo 时可传相对路径。")
    download_parser.add_argument("local_path", help="本地保存路径。")

    deploy_parser = subparsers.add_parser("deploy", help="在正式目录执行部署脚本。")
    deploy_parser.add_argument("--timeout", type=int, default=600)

    return parser.parse_args(argv)


def build_settings(args: argparse.Namespace) -> RemoteSettings:
    return RemoteSettings(
        host=args.host,
        port=args.port,
        user=args.user,
        key_file=Path(args.key_file).expanduser(),
        password_env_var=args.password_env,
        repo_dev=args.repo_dev,
        repo_prod=args.repo_prod,
        strict_host_key=args.strict_host_key,
    )


def configure_host_key_policy(client: paramiko.SSHClient, strict_host_key: bool) -> None:
    client.load_system_host_keys()
    try:
        client.load_host_keys(str(Path.home() / ".ssh" / "known_hosts"))
    except OSError:
        pass
    if strict_host_key:
        client.set_missing_host_key_policy(paramiko.RejectPolicy())
    else:
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())


def read_password(settings: RemoteSettings) -> str | None:
    password = os.environ.get(settings.password_env_var, "").strip()
    if password:
        return password
    if sys.stdin.isatty():
        return getpass.getpass(
            f"请输入 {settings.user}@{settings.host} 的 SSH 密码（或先设置环境变量 {settings.password_env_var}）: "
        )
    return None


def connect_with_key(settings: RemoteSettings) -> tuple[paramiko.SSHClient | None, str | None]:
    if not settings.key_file.exists():
        return None, f"未找到私钥文件: {settings.key_file}"
    client = paramiko.SSHClient()
    configure_host_key_policy(client, settings.strict_host_key)
    try:
        client.connect(
            hostname=settings.host,
            port=settings.port,
            username=settings.user,
            key_filename=str(settings.key_file),
            look_for_keys=False,
            allow_agent=False,
            timeout=10,
            auth_timeout=10,
            banner_timeout=10,
        )
    except Exception as exc:  # pragma: no cover - 依赖远端环境
        client.close()
        return None, str(exc)
    return client, None


def connect_with_password(settings: RemoteSettings) -> tuple[paramiko.SSHClient | None, str | None]:
    password = read_password(settings)
    if not password:
        return None, f"未获取到 SSH 密码。请设置环境变量 {settings.password_env_var} 或在交互终端中运行。"
    client = paramiko.SSHClient()
    configure_host_key_policy(client, settings.strict_host_key)
    try:
        client.connect(
            hostname=settings.host,
            port=settings.port,
            username=settings.user,
            password=password,
            look_for_keys=False,
            allow_agent=False,
            timeout=10,
            auth_timeout=10,
            banner_timeout=10,
        )
    except Exception as exc:  # pragma: no cover - 依赖远端环境
        client.close()
        return None, str(exc)
    return client, None


def open_client(settings: RemoteSettings, auth_mode: str) -> tuple[paramiko.SSHClient, str]:
    errors: list[str] = []

    if auth_mode in {"auto", "key"}:
        client, error = connect_with_key(settings)
        if client is not None:
            return client, "key"
        if error:
            errors.append(f"key: {error}")
        if auth_mode == "key":
            raise RuntimeError("SSH key 认证失败: " + "; ".join(errors))

    if auth_mode in {"auto", "password"}:
        client, error = connect_with_password(settings)
        if client is not None:
            return client, "password"
        if error:
            errors.append(f"password: {error}")

    raise RuntimeError("SSH 连接失败: " + "; ".join(errors))


def resolve_remote_path(settings: RemoteSettings, repo_name: str | None, raw_path: str) -> str:
    normalized_raw_path = raw_path.replace("\\", "/")
    path = PurePosixPath(normalized_raw_path)
    if path.is_absolute():
        return str(path)
    repo_root = settings.repo_root(repo_name)
    if repo_root is None:
        raise ValueError("未指定 --repo 时，path 必须为绝对路径。")
    return posixpath.normpath(posixpath.join(repo_root, normalized_raw_path))


def build_remote_command(repo_root: str | None, command: str) -> str:
    if repo_root:
        return f"cd {shlex.quote(repo_root)} && {command}"
    return command


def run_remote_command(
    client: paramiko.SSHClient,
    command: str,
    *,
    check: bool = True,
    timeout: int | None = None,
) -> tuple[str, str, int]:
    stdin, stdout, stderr = client.exec_command(command, timeout=timeout)
    stdout_text = stdout.read().decode("utf-8", errors="replace")
    stderr_text = stderr.read().decode("utf-8", errors="replace")
    exit_status = stdout.channel.recv_exit_status()
    if check and exit_status != 0:
        raise RemoteCommandError(command, exit_status, stderr_text)
    return stdout_text, stderr_text, exit_status


def ensure_remote_parent(client: paramiko.SSHClient, remote_path: str) -> None:
    parent = posixpath.dirname(remote_path)
    if parent:
        run_remote_command(client, f"mkdir -p {shlex.quote(parent)}", check=True)


def handle_probe(client: paramiko.SSHClient, settings: RemoteSettings, timeout: int) -> int:
    sections: list[tuple[str, str]] = [
        (
            "REMOTE",
            "printf 'host=%s\\nuser=%s\\npwd=%s\\n' \"$(hostname)\" \"$(whoami)\" \"$(pwd)\"",
        ),
        (
            "DEV_STATUS",
            build_remote_command(
                settings.repo_dev,
                "printf 'repo=%s\\n' \"$PWD\" && git status --short --branch && printf '\\n' && git log -1 --pretty=format:'%h %ad %s' --date=iso",
            ),
        ),
        (
            "PROD_STATUS",
            build_remote_command(
                settings.repo_prod,
                "printf 'repo=%s\\n' \"$PWD\" && git status --short --branch && printf '\\n' && git log -1 --pretty=format:'%h %ad %s' --date=iso",
            ),
        ),
        (
            "DOCKER",
            "docker ps --format '{{.Names}}\\t{{.Status}}'",
        ),
    ]

    for title, command in sections:
        print(f"=== {title} ===")
        stdout_text, stderr_text, _ = run_remote_command(client, command, timeout=timeout)
        if stdout_text:
            print(stdout_text.rstrip())
        if stderr_text:
            print(stderr_text.rstrip(), file=sys.stderr)
        print()
    return 0


def handle_exec(client: paramiko.SSHClient, settings: RemoteSettings, args: argparse.Namespace) -> int:
    command_parts = args.command
    if command_parts and command_parts[0] == "--":
        command_parts = command_parts[1:]
    if not command_parts:
        raise ValueError("exec 子命令后必须提供要执行的命令。")
    command = " ".join(command_parts)
    repo_root = settings.repo_root(args.repo)
    stdout_text, stderr_text, exit_status = run_remote_command(
        client,
        build_remote_command(repo_root, command),
        check=False,
        timeout=args.timeout,
    )
    if stdout_text:
        print(stdout_text, end="")
    if stderr_text:
        print(stderr_text, end="", file=sys.stderr)
    return exit_status


def handle_read(client: paramiko.SSHClient, settings: RemoteSettings, args: argparse.Namespace) -> int:
    remote_path = resolve_remote_path(settings, args.repo, args.path)
    sftp = client.open_sftp()
    try:
        with sftp.open(remote_path, "rb") as fh:
            sys.stdout.buffer.write(fh.read())
    finally:
        sftp.close()
    return 0


def handle_upload(client: paramiko.SSHClient, settings: RemoteSettings, args: argparse.Namespace) -> int:
    local_path = Path(args.local_path).expanduser().resolve()
    if not local_path.is_file():
        raise FileNotFoundError(f"本地文件不存在: {local_path}")
    remote_path = resolve_remote_path(settings, args.repo, args.remote_path)
    ensure_remote_parent(client, remote_path)
    sftp = client.open_sftp()
    try:
        sftp.put(str(local_path), remote_path)
        if args.chmod:
            sftp.chmod(remote_path, int(args.chmod, 8))
    finally:
        sftp.close()
    print(f"uploaded {local_path} -> {remote_path}")
    return 0


def handle_download(client: paramiko.SSHClient, settings: RemoteSettings, args: argparse.Namespace) -> int:
    remote_path = resolve_remote_path(settings, args.repo, args.remote_path)
    local_path = Path(args.local_path).expanduser().resolve()
    local_path.parent.mkdir(parents=True, exist_ok=True)
    sftp = client.open_sftp()
    try:
        sftp.get(remote_path, str(local_path))
    finally:
        sftp.close()
    print(f"downloaded {remote_path} -> {local_path}")
    return 0


def handle_deploy(client: paramiko.SSHClient, settings: RemoteSettings, args: argparse.Namespace) -> int:
    deploy_command = build_remote_command(settings.repo_prod, "./scripts/deploy_from_main.sh")
    stdout_text, stderr_text, exit_status = run_remote_command(
        client,
        deploy_command,
        check=False,
        timeout=args.timeout,
    )
    if stdout_text:
        print(stdout_text, end="")
    if stderr_text:
        print(stderr_text, end="", file=sys.stderr)
    return exit_status


def dispatch(args: argparse.Namespace, client: paramiko.SSHClient, settings: RemoteSettings) -> int:
    if args.subcommand == "probe":
        return handle_probe(client, settings, args.timeout)
    if args.subcommand == "exec":
        return handle_exec(client, settings, args)
    if args.subcommand == "read":
        return handle_read(client, settings, args)
    if args.subcommand == "upload":
        return handle_upload(client, settings, args)
    if args.subcommand == "download":
        return handle_download(client, settings, args)
    if args.subcommand == "deploy":
        return handle_deploy(client, settings, args)
    raise ValueError(f"未知子命令: {args.subcommand}")


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv or sys.argv[1:])
    settings = build_settings(args)
    client, auth_used = open_client(settings, args.auth)
    print(f"[auth] {auth_used}", file=sys.stderr)
    try:
        return dispatch(args, client, settings)
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())
