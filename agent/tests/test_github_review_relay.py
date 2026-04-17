from fastapi.testclient import TestClient

from music_agent.config import Settings
from music_agent.github_review_relay import create_app


def _settings(**overrides):
    return Settings(
        _env_file=None,
        FEISHU_BOT_WEBHOOK="https://example.invalid/hook",
        **overrides,
    )


def test_github_ping_returns_ok():
    client = TestClient(create_app(_settings()))

    response = client.post(
        "/webhooks/github",
        headers={
            "X-GitHub-Event": "ping",
            "X-GitHub-Delivery": "delivery-1",
        },
        json={"zen": "Keep it logically awesome."},
    )

    assert response.status_code == 200
    assert response.json()["event"] == "ping"
    assert response.json()["zen"] == "Keep it logically awesome."


def test_copilot_review_is_forwarded(monkeypatch):
    sent: dict[str, object] = {}

    def fake_send(webhook_url: str, text: str, timeout_seconds: float):
        sent["webhook_url"] = webhook_url
        sent["text"] = text
        sent["timeout_seconds"] = timeout_seconds
        return 200, '{"ok":true}'

    monkeypatch.setattr("music_agent.github_review_relay._send_feishu_text", fake_send)

    client = TestClient(create_app(_settings()))
    response = client.post(
        "/webhooks/github",
        headers={
            "X-GitHub-Event": "pull_request_review",
            "X-GitHub-Delivery": "delivery-2",
        },
        json={
            "action": "submitted",
            "repository": {"full_name": "shenxi11/ffmpeg_music_player"},
            "pull_request": {
                "title": "feat: add grouped playlist section to sidebar",
                "html_url": "https://github.com/shenxi11/ffmpeg_music_player/pull/2",
            },
            "review": {
                "state": "commented",
                "body": "Please tighten this ownership normalization logic.",
                "html_url": "https://github.com/shenxi11/ffmpeg_music_player/pull/2#pullrequestreview-1",
                "user": {"login": "github-copilot[bot]"},
            },
            "sender": {"login": "github-copilot[bot]"},
        },
    )

    assert response.status_code == 200
    assert response.json()["forwarded"] is True
    assert sent["webhook_url"] == "https://example.invalid/hook"
    assert "GitHub Copilot Review 通知" in str(sent["text"])
    assert "Please tighten this ownership normalization logic." in str(sent["text"])


def test_non_copilot_review_is_filtered_by_default(monkeypatch):
    def fake_send(webhook_url: str, text: str, timeout_seconds: float):
        raise AssertionError("should not forward non-copilot review by default")

    monkeypatch.setattr("music_agent.github_review_relay._send_feishu_text", fake_send)

    client = TestClient(create_app(_settings()))
    response = client.post(
        "/webhooks/github",
        headers={"X-GitHub-Event": "pull_request_review"},
        json={
            "action": "submitted",
            "repository": {"full_name": "shenxi11/ffmpeg_music_player"},
            "pull_request": {
                "title": "feat: add grouped playlist section to sidebar",
                "html_url": "https://github.com/shenxi11/ffmpeg_music_player/pull/2",
            },
            "review": {
                "state": "approved",
                "body": "LGTM",
                "html_url": "https://github.com/shenxi11/ffmpeg_music_player/pull/2#pullrequestreview-2",
                "user": {"login": "octocat"},
            },
            "sender": {"login": "octocat"},
        },
    )

    assert response.status_code == 200
    assert response.json()["skipped"] is True
    assert response.json()["reason"].startswith("filtered_non_copilot")


def test_review_comment_respects_comment_toggle(monkeypatch):
    def fake_send(webhook_url: str, text: str, timeout_seconds: float):
        raise AssertionError("should not forward review comments when disabled")

    monkeypatch.setattr("music_agent.github_review_relay._send_feishu_text", fake_send)

    client = TestClient(create_app(_settings(RELAY_NOTIFY_REVIEW_COMMENTS=False)))
    response = client.post(
        "/webhooks/github",
        headers={"X-GitHub-Event": "pull_request_review_comment"},
        json={
            "action": "created",
            "repository": {"full_name": "shenxi11/ffmpeg_music_player"},
            "pull_request": {
                "title": "feat: add grouped playlist section to sidebar",
                "html_url": "https://github.com/shenxi11/ffmpeg_music_player/pull/2",
            },
            "comment": {
                "body": "Please rename this variable.",
                "html_url": "https://github.com/shenxi11/ffmpeg_music_player/pull/2#discussion_r1",
                "path": "src/app/main_widget.cpp",
                "line": 123,
                "user": {"login": "github-copilot[bot]"},
            },
            "sender": {"login": "github-copilot[bot]"},
        },
    )

    assert response.status_code == 200
    assert response.json()["skipped"] is True
    assert response.json()["reason"] == "review_comments_disabled"
