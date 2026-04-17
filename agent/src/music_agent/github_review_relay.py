from __future__ import annotations

# 模块名: github_review_relay
# 功能概述: 接收 GitHub PR review / review comment webhook，并将摘要转发到飞书机器人。
# 对外接口: create_app, main
# 依赖关系: FastAPI、uvicorn、music_agent.config.Settings、飞书机器人 webhook
# 输入输出: 输入为 GitHub webhook HTTP 请求；输出为 JSON 响应，并向飞书发送文本消息
# 异常与错误: 缺少 webhook 配置、签名校验失败、飞书请求失败时返回 HTTP 错误
# 维护说明: 本模块刻意保持只读转发，不接触现有聊天 Agent 运行时；适合独立部署验证通知链路

import hashlib
import hmac
import json
from dataclasses import dataclass
from typing import Any
from urllib import error as urllib_error
from urllib import request as urllib_request

import uvicorn
from fastapi import FastAPI, Header, HTTPException, Request

from .config import Settings


@dataclass(slots=True)
class RelayDecision:
    should_send: bool
    title: str = ""
    text: str = ""
    skip_reason: str = ""


def _looks_like_copilot(login: str | None) -> bool:
    return "copilot" in (login or "").strip().lower()


def _verify_github_signature(secret: str | None, payload: bytes, signature_header: str | None) -> None:
    if not secret:
        return
    if not signature_header:
        raise HTTPException(status_code=401, detail="missing github signature")

    expected = "sha256=" + hmac.new(secret.encode("utf-8"), payload, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected, signature_header):
        raise HTTPException(status_code=401, detail="invalid github signature")


def _build_review_text(repository: str, pr_title: str, action: str, review_state: str, reviewer: str, url: str, body: str) -> str:
    lines = [
        "GitHub Copilot Review 通知" if _looks_like_copilot(reviewer) else "GitHub Review 通知",
        f"仓库: {repository}",
        f"PR: {pr_title}",
        f"动作: {action}",
        f"状态: {review_state or 'unknown'}",
        f"评审人: {reviewer}",
        f"链接: {url}",
    ]
    if body:
        lines.append(f"内容: {body}")
    return "\n".join(lines)


def _build_review_comment_text(
    repository: str,
    pr_title: str,
    action: str,
    reviewer: str,
    url: str,
    path: str,
    line: Any,
    body: str,
) -> str:
    lines = [
        "GitHub Copilot Review Comment 通知" if _looks_like_copilot(reviewer) else "GitHub Review Comment 通知",
        f"仓库: {repository}",
        f"PR: {pr_title}",
        f"动作: {action}",
        f"评论人: {reviewer}",
        f"文件: {path or 'unknown'}",
        f"行号: {line if line is not None else 'unknown'}",
        f"链接: {url}",
    ]
    if body:
        lines.append(f"内容: {body}")
    return "\n".join(lines)


def _build_decision(event_name: str, payload: dict[str, Any], settings: Settings) -> RelayDecision:
    repository = str((payload.get("repository") or {}).get("full_name") or "")
    pull_request = payload.get("pull_request") or {}
    pr_title = str(pull_request.get("title") or "")
    pr_url = str(pull_request.get("html_url") or "")
    action = str(payload.get("action") or "")

    if event_name == "pull_request_review":
        review = payload.get("review") or {}
        reviewer = str(((review.get("user") or {}).get("login")) or (payload.get("sender") or {}).get("login") or "")
        if settings.relay_notify_copilot_only and not _looks_like_copilot(reviewer):
            return RelayDecision(should_send=False, skip_reason=f"filtered_non_copilot:{reviewer}")
        return RelayDecision(
            should_send=True,
            title="review",
            text=_build_review_text(
                repository=repository,
                pr_title=pr_title,
                action=action,
                review_state=str(review.get("state") or ""),
                reviewer=reviewer,
                url=str(review.get("html_url") or pr_url),
                body=str(review.get("body") or "").strip(),
            ),
        )

    if event_name == "pull_request_review_comment":
        if not settings.relay_notify_review_comments:
            return RelayDecision(should_send=False, skip_reason="review_comments_disabled")
        comment = payload.get("comment") or {}
        reviewer = str(((comment.get("user") or {}).get("login")) or (payload.get("sender") or {}).get("login") or "")
        if settings.relay_notify_copilot_only and not _looks_like_copilot(reviewer):
            return RelayDecision(should_send=False, skip_reason=f"filtered_non_copilot:{reviewer}")
        return RelayDecision(
            should_send=True,
            title="review_comment",
            text=_build_review_comment_text(
                repository=repository,
                pr_title=pr_title,
                action=action,
                reviewer=reviewer,
                url=str(comment.get("html_url") or pr_url),
                path=str(comment.get("path") or ""),
                line=comment.get("line"),
                body=str(comment.get("body") or "").strip(),
            ),
        )

    return RelayDecision(should_send=False, skip_reason=f"unsupported_event:{event_name}")


def _send_feishu_text(webhook_url: str, text: str, timeout_seconds: float) -> tuple[int, str]:
    payload = json.dumps(
        {
            "msg_type": "text",
            "content": {
                "text": text,
            },
        },
        ensure_ascii=False,
    ).encode("utf-8")
    req = urllib_request.Request(
        webhook_url,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib_request.urlopen(req, timeout=timeout_seconds) as response:
            body = response.read().decode("utf-8", errors="replace")
            return response.status, body
    except urllib_error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise HTTPException(status_code=502, detail=f"feishu_http_error:{exc.code}:{body}") from exc
    except urllib_error.URLError as exc:
        raise HTTPException(status_code=502, detail=f"feishu_network_error:{exc.reason}") from exc


def create_app(settings: Settings | None = None) -> FastAPI:
    resolved_settings = settings or Settings()
    app = FastAPI(title="GitHub Review Relay", version="0.1.0")
    app.state.settings = resolved_settings

    @app.get("/healthz")
    async def healthz() -> dict[str, Any]:
        return {
            "status": "ok",
            "feishuWebhookConfigured": bool(resolved_settings.feishu_bot_webhook),
            "githubWebhookSecretConfigured": bool(resolved_settings.github_webhook_secret),
            "copilotOnly": resolved_settings.relay_notify_copilot_only,
            "reviewCommentsEnabled": resolved_settings.relay_notify_review_comments,
        }

    @app.post("/webhooks/github")
    async def github_webhook(
        request: Request,
        x_github_event: str | None = Header(default=None, alias="X-GitHub-Event"),
        x_github_delivery: str | None = Header(default=None, alias="X-GitHub-Delivery"),
        x_hub_signature_256: str | None = Header(default=None, alias="X-Hub-Signature-256"),
    ) -> dict[str, Any]:
        raw_payload = await request.body()
        _verify_github_signature(resolved_settings.github_webhook_secret, raw_payload, x_hub_signature_256)

        try:
            payload = json.loads(raw_payload.decode("utf-8"))
        except json.JSONDecodeError as exc:
            raise HTTPException(status_code=400, detail="invalid json payload") from exc

        event_name = x_github_event or ""
        if event_name == "ping":
            return {
                "ok": True,
                "event": event_name,
                "delivery": x_github_delivery,
                "zen": payload.get("zen"),
            }

        if not resolved_settings.feishu_bot_webhook:
            raise HTTPException(status_code=503, detail="missing FEISHU_BOT_WEBHOOK")

        decision = _build_decision(event_name, payload, resolved_settings)
        if not decision.should_send:
            return {
                "ok": True,
                "event": event_name,
                "delivery": x_github_delivery,
                "skipped": True,
                "reason": decision.skip_reason,
            }

        status_code, response_body = _send_feishu_text(
            webhook_url=resolved_settings.feishu_bot_webhook,
            text=decision.text,
            timeout_seconds=resolved_settings.github_review_relay_timeout_seconds,
        )
        return {
            "ok": True,
            "event": event_name,
            "delivery": x_github_delivery,
            "forwarded": True,
            "feishuStatus": status_code,
            "feishuBody": response_body,
            "messageType": decision.title,
        }

    return app


def main() -> None:
    settings = Settings()
    uvicorn.run(
        "music_agent.github_review_relay:create_app",
        host=settings.github_review_relay_host,
        port=settings.github_review_relay_port,
        factory=True,
        reload=False,
    )


if __name__ == "__main__":
    main()
