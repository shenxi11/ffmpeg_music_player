import uuid

from fastapi.testclient import TestClient

from music_agent.config import Settings
from music_agent.control_gateways import LocalModelGatewayError
from music_agent.control_models import ControlIntent
from music_agent.server import create_app


class FakeLocalGateway:
    def __init__(self, intent: ControlIntent) -> None:
        self.intent = intent
        self.calls: list[str] = []

    async def compile_intent(self, user_message, host_context, capability_snapshot, memory_snapshot):
        self.calls.append(user_message)
        return self.intent


class FailingLocalGateway:
    async def compile_intent(self, user_message, host_context, capability_snapshot, memory_snapshot):
        raise RuntimeError("local model unavailable")


class OverflowLocalGateway:
    async def compile_intent(self, user_message, host_context, capability_snapshot, memory_snapshot):
        raise LocalModelGatewayError("context_overflow", "local model context size exceeded")


class FakeRemoteGateway:
    def __init__(self, reply_text: str) -> None:
        self.reply_text = reply_text
        self.calls: list[str] = []

    async def reply(self, user_message, host_context):
        self.calls.append(user_message)
        return self.reply_text


def _settings(tmp_path, **overrides) -> Settings:
    defaults = {
        "AGENT_STORAGE_PATH": str(tmp_path / "music_agent.db"),
        "LOCAL_MODEL_BASE_URL": "http://127.0.0.1:8081/v1",
        "LOCAL_MODEL_NAME": "Qwen2.5-3B-Instruct",
        "REMOTE_MODEL_ENABLED": False,
    }
    defaults.update(overrides)
    return Settings(_env_file=None, **defaults)


def _recv_type(websocket, expected_type: str) -> dict:
    message = websocket.receive_json()
    assert message["type"] == expected_type
    return message


def _drain_assistant_reply(websocket) -> str:
    _recv_type(websocket, "assistant_start")
    chunk = _recv_type(websocket, "assistant_chunk")
    final_message = _recv_type(websocket, "assistant_final")
    assert final_message["content"] == chunk["delta"]
    return final_message["content"]


def test_healthz_reports_local_control_defaults(tmp_path):
    client = TestClient(create_app(_settings(tmp_path)))

    payload = client.get("/healthz").json()

    assert payload["status"] == "ok"
    assert payload["modelConfigured"] is True
    assert payload["localModelName"] == "Qwen2.5-3B-Instruct"
    assert payload["defaultMode"] == "control"


def test_fast_path_pause_executes_without_local_model(tmp_path):
    app = create_app(_settings(tmp_path))
    app.state.music_runtime._local_model_gateway = FailingLocalGateway()
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json({"type": "user_message", "requestId": "req-1", "content": "暂停播放"})
        tool_call = _recv_type(websocket, "tool_call")
        assert tool_call["tool"] == "pausePlayback"

        websocket.send_json({
            "type": "tool_result",
            "toolCallId": tool_call["toolCallId"],
            "ok": True,
            "result": {"ok": True},
        })

        final_result = _recv_type(websocket, "final_result")
        assert final_result["ok"] is True
        assert "暂停" in final_result["summary"]
        assert "暂停" in _drain_assistant_reply(websocket)


def test_control_mode_restricts_world_knowledge(tmp_path):
    app = create_app(_settings(tmp_path))
    app.state.music_runtime._local_model_gateway = FakeLocalGateway(
        ControlIntent(
            route="restricted",
            intent="restricted",
            arguments={},
            entityRefs=[],
            confirmation=False,
            reasonCode="world_knowledge",
        )
    )
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json({"type": "user_message", "requestId": "req-2", "content": "解释一下 AI 发展史"})
        reply = _drain_assistant_reply(websocket)
        assert "控制软件" in reply
        assert "/assistant" in reply


def test_assistant_prefix_uses_explicit_remote_fallback(tmp_path):
    app = create_app(_settings(
        tmp_path,
        REMOTE_MODEL_ENABLED=True,
        OPENAI_API_KEY="sk-test",
        REMOTE_MODEL_NAME="remote-test",
    ))
    remote_gateway = FakeRemoteGateway("这是远程助手回复。")
    app.state.music_runtime._remote_assistant_gateway = remote_gateway
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json({"type": "user_message", "requestId": "req-3", "content": "/assistant 解释一下 AI 发展史"})
        reply = _drain_assistant_reply(websocket)
        assert reply == "这是远程助手回复。"
        assert remote_gateway.calls == ["解释一下 AI 发展史"]


def test_create_playlist_requires_approval_then_executes(tmp_path):
    app = create_app(_settings(tmp_path))
    app.state.music_runtime._local_model_gateway = FakeLocalGateway(
        ControlIntent(
            route="template",
            intent="create_playlist_with_confirmation",
            arguments={"playlistName": "学习歌单"},
            entityRefs=[],
            confirmation=True,
            reasonCode="playlist_write",
        )
    )
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json({
            "type": "host_snapshot",
            "hostContext": {"loggedIn": True},
            "capabilities": [
                {"name": "createPlaylist", "availabilityPolicy": "login_required"},
                {"name": "getLocalTracks", "availabilityPolicy": "always"},
            ],
            "catalogVersion": "test",
        })
        websocket.send_json({"type": "user_message", "requestId": "req-4", "content": "创建学习歌单"})

        preview = _recv_type(websocket, "plan_preview")
        approval = _recv_type(websocket, "approval_request")
        assert preview["riskLevel"] == "medium"
        assert "学习歌单" in approval["message"]

        websocket.send_json({"type": "approval_response", "planId": approval["planId"], "approved": True})
        tool_call = _recv_type(websocket, "tool_call")
        assert tool_call["tool"] == "createPlaylist"

        websocket.send_json({
            "type": "tool_result",
            "toolCallId": tool_call["toolCallId"],
            "ok": True,
            "result": {"playlistId": 7, "name": "学习歌单", "message": "ok"},
        })

        final_result = _recv_type(websocket, "final_result")
        assert final_result["ok"] is True
        assert "学习歌单" in final_result["summary"]
        assert "学习歌单" in _drain_assistant_reply(websocket)


def test_context_overflow_is_reported_as_context_problem(tmp_path):
    app = create_app(_settings(tmp_path))
    app.state.music_runtime._local_model_gateway = OverflowLocalGateway()
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json(
            {
                "type": "user_message",
                "requestId": "req-5",
                "content": "创建一个歌单，歌单名为周杰伦，周杰伦歌单里面添加流行歌单的前三首音乐",
            }
        )
        reply = _drain_assistant_reply(websocket)
        assert "上下文不足" in reply


def test_list_local_tracks_uses_local_library_path(tmp_path):
    app = create_app(_settings(tmp_path))
    app.state.music_runtime._local_model_gateway = FailingLocalGateway()
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json({"type": "user_message", "requestId": "req-6", "content": "列出我的本地音乐"})

        tool_call = _recv_type(websocket, "tool_call")
        assert tool_call["tool"] == "getLocalTracks"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"title": "七里香", "artist": "周杰伦"},
                        {"title": "轨迹", "artist": "周杰伦"},
                    ]
                },
            }
        )

        final_result = _recv_type(websocket, "final_result")
        assert final_result["ok"] is True
        assert "本地音乐共有 2 首" in final_result["summary"]
        assert "七里香" in final_result["summary"]
        assert "轨迹" in _drain_assistant_reply(websocket)


def test_read_only_query_bypasses_pending_approval(tmp_path):
    app = create_app(_settings(tmp_path))
    app.state.music_runtime._local_model_gateway = FakeLocalGateway(
        ControlIntent(
            route="template",
            intent="create_playlist_with_confirmation",
            arguments={"playlistName": "学习歌单"},
            entityRefs=[],
            confirmation=True,
            reasonCode="playlist_write",
        )
    )
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json({
            "type": "host_snapshot",
            "hostContext": {"loggedIn": True},
            "capabilities": [
                {"name": "createPlaylist", "availabilityPolicy": "login_required"},
                {"name": "getLocalTracks", "availabilityPolicy": "always"},
            ],
            "catalogVersion": "test",
        })
        websocket.send_json({"type": "user_message", "requestId": "req-7", "content": "创建学习歌单"})
        _recv_type(websocket, "plan_preview")
        _recv_type(websocket, "approval_request")

        websocket.send_json({"type": "user_message", "requestId": "req-8", "content": "列出我的本地音乐"})
        tool_call = _recv_type(websocket, "tool_call")
        assert tool_call["tool"] == "getLocalTracks"


def test_host_snapshot_validation_error_is_explicit(tmp_path):
    app = create_app(_settings(tmp_path))
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={uuid.uuid4()}") as websocket:
        _recv_type(websocket, "session_ready")
        websocket.send_json(
            {
                "type": "host_snapshot",
                "hostContext": {"selectedTrackIds": [11]},
                "capabilities": [],
                "catalogVersion": "test",
            }
        )
        error = _recv_type(websocket, "error")
        assert error["code"] == "host_snapshot_validation_error"
        assert "selectedTrackIds" in error["message"]
