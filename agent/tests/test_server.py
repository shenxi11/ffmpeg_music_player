import json

from fastapi.testclient import TestClient

from music_agent.chat_agent import ChatAgent
from music_agent.config import Settings
from music_agent.server import create_app
from music_agent.session_store import InMemorySessionStore
from music_agent.workflow_memory import PendingScriptExecution


class FakeChatModelClient:
    def __init__(self) -> None:
        self.complete_calls: list[list[dict[str, str]]] = []
        self.structured_calls: list[list[dict[str, str]]] = []
        self.stream_calls: list[list[dict[str, str]]] = []

    async def complete(self, messages: list[dict[str, str]]) -> str:
        self.complete_calls.append(messages)
        latest_user_message = messages[-1]["content"]
        return f"echo:{latest_user_message}"

    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        payload = json.loads(messages[-1]["content"])
        user_message = payload["userMessage"]
        context = payload.get("context", {})
        return json.dumps(self._semantic_result_for(user_message, context), ensure_ascii=False)

    async def stream_complete(self, messages: list[dict[str, str]]):
        self.stream_calls.append(messages)
        latest_user_message = messages[-1]["content"]
        yield "echo:"
        yield latest_user_message

    @staticmethod
    def _semantic_result_for(user_message: str, context: dict) -> dict:
        playlist_ctx = context.get("lastNamedPlaylist")
        if user_message == "查看当前播放歌曲":
            return _semantic_payload("tool", "get_current_track", "playback", auto_execute=True)
        if user_message == "有哪些歌单":
            return _semantic_payload("tool", "get_playlists", "playlist", auto_execute=True)
        if user_message in {"帮我查询我的流行歌单", "帮我查询流行歌单"}:
            return _semantic_payload(
                "tool",
                "query_playlist",
                "playlist",
                playlist={"rawQuery": "流行歌单", "normalizedQuery": "流行"},
                auto_execute=True,
            )
        if user_message == "看看流行歌单里有什么歌":
            return _semantic_payload(
                "tool",
                "inspect_playlist_tracks",
                "playlist",
                playlist={"rawQuery": "流行歌单", "normalizedQuery": "流行"},
                auto_execute=True,
            )
        if user_message == "看看这个歌单里有什么歌":
            references = ["last_named_playlist"] if playlist_ctx else []
            missing = [] if playlist_ctx else ["playlist"]
            return _semantic_payload(
                "tool",
                "inspect_playlist_tracks",
                "playlist",
                references=references,
                missing_fields=missing,
                auto_execute=bool(playlist_ctx),
            )
        if user_message == "最近听了什么":
            return _semantic_payload("tool", "get_recent_tracks", "library", auto_execute=True)
        if user_message == "播放这个歌单":
            references = ["last_named_playlist"] if playlist_ctx else []
            return _semantic_payload(
                "tool",
                "play_playlist",
                "playlist",
                references=references,
                missing_fields=[] if playlist_ctx else ["playlist"],
                auto_execute=bool(playlist_ctx),
            )
        if user_message == "播放周杰伦的晴天":
            return _semantic_payload(
                "tool",
                "play_track",
                "track",
                track={"rawQuery": "晴天", "normalizedQuery": "晴天", "artist": "周杰伦"},
                artist="周杰伦",
                auto_execute=True,
            )
        if user_message == "播放晴天":
            return _semantic_payload(
                "tool",
                "play_track",
                "track",
                track={"rawQuery": "晴天", "normalizedQuery": "晴天"},
                auto_execute=True,
            )
        if user_message in {"第一个", "第一句", "第二句", "one", "two", "three", "hello", "多态是什么", "解释一下 C++ 的多态和虚函数机制", "你好"}:
            return _semantic_payload("chat", "chat", "unknown", auto_execute=False)
        if user_message == "创建一个学习歌单":
            return _semantic_payload(
                "plan",
                "create_playlist",
                "playlist",
                playlist={"rawQuery": "学习歌单", "normalizedQuery": "学习"},
                approval=True,
            )
        if user_message == "新建一个夜跑歌单":
            return _semantic_payload(
                "plan",
                "create_playlist",
                "playlist",
                playlist={"rawQuery": "夜跑歌单", "normalizedQuery": "夜跑"},
                approval=True,
            )
        if user_message == "创建一个学习歌单并添加最近常听歌曲":
            return _semantic_payload(
                "plan",
                "create_playlist_with_top_tracks",
                "playlist",
                playlist={"rawQuery": "学习歌单", "normalizedQuery": "学习"},
                approval=True,
            )
        if user_message == "重新创建一个歌单，叫做周杰伦，把流行歌单前三首音乐放到这个歌单里面":
            return _semantic_payload(
                "tool",
                "create_playlist_from_playlist_subset",
                "playlist",
                target_playlist={"rawQuery": "周杰伦", "normalizedQuery": "周杰伦"},
                source_playlist={"rawQuery": "流行歌单", "normalizedQuery": "流行"},
                track_selection={"mode": "first_n", "count": 3},
                auto_execute=True,
                action_candidates=[
                    {"stepId": "s1", "tool": "createPlaylist", "args": {"playlistRole": "target"}, "kind": "tool"},
                    {"stepId": "s2", "tool": "getPlaylists", "args": {"playlistRole": "source"}, "kind": "tool", "dependsOn": ["s1"]},
                    {"stepId": "s3", "tool": "getPlaylistTracks", "args": {"playlistRole": "source"}, "kind": "tool", "dependsOn": ["s2"]},
                    {
                        "stepId": "s4",
                        "tool": "addTracksToPlaylist",
                        "args": {"playlistRole": "target", "selectionMode": "first_n", "count": 3},
                        "kind": "tool",
                        "dependsOn": ["s3"],
                    },
                ],
            )
        if user_message == "重新创建一个歌单，叫做周杰伦，把流行歌单后两首音乐放到这个歌单里面":
            return _semantic_payload(
                "tool",
                "create_playlist_from_playlist_subset",
                "playlist",
                target_playlist={"rawQuery": "周杰伦", "normalizedQuery": "周杰伦"},
                source_playlist={"rawQuery": "流行歌单", "normalizedQuery": "流行"},
                track_selection={"mode": "last_n", "count": 2},
                auto_execute=True,
                action_candidates=[
                    {"stepId": "s1", "tool": "createPlaylist", "args": {"playlistRole": "target"}, "kind": "tool"},
                    {"stepId": "s2", "tool": "getPlaylists", "args": {"playlistRole": "source"}, "kind": "tool", "dependsOn": ["s1"]},
                    {"stepId": "s3", "tool": "getPlaylistTracks", "args": {"playlistRole": "source"}, "kind": "tool", "dependsOn": ["s2"]},
                    {
                        "stepId": "s4",
                        "tool": "addTracksToPlaylist",
                        "args": {"playlistRole": "target", "selectionMode": "last_n", "count": 2},
                        "kind": "tool",
                        "dependsOn": ["s3"],
                    },
                ],
            )
        return _semantic_payload("chat", "chat", "unknown", auto_execute=False)


def _semantic_payload(
    mode: str,
    intent: str,
    target_domain: str,
    *,
    playlist: dict | None = None,
    target_playlist: dict | None = None,
    source_playlist: dict | None = None,
    track_selection: dict | None = None,
    track: dict | None = None,
    artist: str | None = None,
    album: str | None = None,
    references: list[str] | None = None,
    missing_fields: list[str] | None = None,
    auto_execute: bool = False,
    approval: bool = False,
    proposed_tool: dict | None = None,
    action_candidates: list[dict] | None = None,
) -> dict:
    return {
        "mode": mode,
        "intent": intent,
        "entities": {
            "playlist": playlist,
            "targetPlaylist": target_playlist,
            "sourcePlaylist": source_playlist,
            "trackSelection": track_selection,
            "track": track,
            "artist": artist,
            "album": album,
            "limit": None,
        },
        "references": references or [],
        "missingFields": missing_fields or [],
        "ambiguities": [],
        "targetDomain": target_domain,
        "shouldAutoExecute": auto_execute,
        "requiresApproval": approval,
        "confidence": 0.95,
        "proposedTool": proposed_tool,
        "actionCandidates": action_candidates or [],
    }


class InterruptedStreamChatModelClient(FakeChatModelClient):
    async def stream_complete(self, messages: list[dict[str, str]]):
        self.stream_calls.append(messages)
        yield "partial"
        raise RuntimeError("stream failed")


class FallbackStreamChatModelClient(FakeChatModelClient):
    async def stream_complete(self, messages: list[dict[str, str]]):
        self.stream_calls.append(messages)
        if False:
            yield ""
        raise RuntimeError("stream unavailable")


class InvalidStructuredChatModelClient(FakeChatModelClient):
    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        return "not-json"


class GenericStructuredChatModelClient(FakeChatModelClient):
    def __init__(self, fallback_messages: set[str] | None = None) -> None:
        super().__init__()
        self.fallback_messages = fallback_messages or set()

    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        payload = json.loads(messages[-1]["content"])
        user_message = payload["userMessage"]
        if user_message in self.fallback_messages:
            return json.dumps(
                {
                    "mode": "chat",
                    "intent": "chat",
                    "entities": {
                        "playlist": None,
                        "track": None,
                        "artist": None,
                        "album": None,
                        "limit": None,
                    },
                    "references": [],
                    "missingFields": [],
                    "ambiguities": [],
                    "targetDomain": "unknown",
                    "shouldAutoExecute": False,
                    "requiresApproval": False,
                    "confidence": 0.25,
                },
                ensure_ascii=False,
            )
        return await super().complete_structured(messages)


class ProposedToolChatModelClient(FakeChatModelClient):
    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        payload = json.loads(messages[-1]["content"])
        user_message = payload["userMessage"]
        context = payload.get("context", {})
        if user_message == "就放刚才那个":
            playlist = context.get("lastNamedPlaylist") or {}
            return json.dumps(
                _semantic_payload(
                    "tool",
                    "chat",
                    "playlist",
                    references=["last_named_playlist"] if playlist else [],
                    auto_execute=bool(playlist),
                    proposed_tool={
                        "tool": "playPlaylist",
                        "args": {"playlistId": playlist.get("playlistId")} if playlist else {},
                        "confidence": 0.93,
                        "reason": "用户明确要求播放上文已确认的歌单",
                    },
                ),
                ensure_ascii=False,
            )
        return await super().complete_structured(messages)


class ActionCandidatesChatModelClient(FakeChatModelClient):
    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        payload = json.loads(messages[-1]["content"])
        user_message = payload["userMessage"]
        if user_message == "帮我查一下流行歌单，找到就播放":
            return json.dumps(
                _semantic_payload(
                    "tool",
                    "query_playlist",
                    "playlist",
                    playlist={"rawQuery": "流行歌单", "normalizedQuery": "流行"},
                    auto_execute=True,
                    action_candidates=[
                        {"stepId": "s1", "tool": "getPlaylists", "args": {}, "kind": "tool"},
                        {"stepId": "s2", "tool": "playPlaylist", "args": {}, "kind": "tool", "dependsOn": ["s1"]},
                    ],
                ),
                ensure_ascii=False,
            )
        return await super().complete_structured(messages)


class PlaylistTracksActionCandidatesChatModelClient(FakeChatModelClient):
    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        payload = json.loads(messages[-1]["content"])
        user_message = payload["userMessage"]
        if user_message == "看看流行歌单里有什么歌，然后播放第一首":
            return json.dumps(
                _semantic_payload(
                    "tool",
                    "inspect_playlist_tracks",
                    "playlist",
                    playlist={"rawQuery": "????", "normalizedQuery": "??"},
                    auto_execute=True,
                    action_candidates=[
                        {"stepId": "s1", "tool": "getPlaylists", "args": {}, "kind": "tool"},
                        {"stepId": "s2", "tool": "getPlaylistTracks", "args": {}, "kind": "tool", "dependsOn": ["s1"]},
                        {
                            "stepId": "s3",
                            "tool": "playTrack",
                            "args": {"selectionIndex": 1},
                            "kind": "tool",
                            "dependsOn": ["s2"],
                        },
                    ],
                ),
                ensure_ascii=False,
            )
        return await super().complete_structured(messages)


class DirectWriteActionCandidatesChatModelClient(FakeChatModelClient):
    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.structured_calls.append(messages)
        payload = json.loads(messages[-1]["content"])
        user_message = payload["userMessage"]
        if user_message == "直接创建一个学习歌单":
            return json.dumps(
                _semantic_payload(
                    "tool",
                    "create_playlist",
                    "playlist",
                    playlist={"rawQuery": "学习歌单", "normalizedQuery": "学习"},
                    auto_execute=True,
                    action_candidates=[
                        {"stepId": "s1", "tool": "createPlaylist", "args": {}, "kind": "tool"},
                    ],
                ),
                ensure_ascii=False,
            )
        if user_message == "直接创建一个学习歌单并添加最近常听歌曲":
            return json.dumps(
                _semantic_payload(
                    "tool",
                    "create_playlist_with_top_tracks",
                    "playlist",
                    playlist={"rawQuery": "学习歌单", "normalizedQuery": "学习"},
                    auto_execute=True,
                    action_candidates=[
                        {"stepId": "s1", "tool": "createPlaylist", "args": {}, "kind": "tool"},
                        {"stepId": "s2", "tool": "getTopPlayedTracks", "args": {}, "kind": "tool", "dependsOn": ["s1"]},
                        {"stepId": "s3", "tool": "addTracksToPlaylist", "args": {}, "kind": "tool", "dependsOn": ["s2"]},
                    ],
                ),
                ensure_ascii=False,
            )
        return await super().complete_structured(messages)


def make_app(
    max_history_messages: int = 20,
    tool_timeout_seconds: float = 15.0,
    model: FakeChatModelClient | None = None,
    allow_direct_write_actions: bool = True,
):
    settings = Settings(
        _env_file=None,
        OPENAI_API_KEY="test-key",
        OPENAI_MODEL="test-model",
        AGENT_STORAGE_PATH="test-storage.db",
        AGENT_MAX_HISTORY_MESSAGES=max_history_messages,
        AGENT_TOOL_TIMEOUT_SECONDS=tool_timeout_seconds,
        AGENT_ALLOW_DIRECT_WRITE_ACTIONS=allow_direct_write_actions,
    )
    store = InMemorySessionStore(max_history_messages=max_history_messages)
    model = model or FakeChatModelClient()
    chat_agent = ChatAgent(llm_client=model, session_store=store)
    app = create_app(settings=settings, chat_agent=chat_agent)
    return app, model, store


def _consume_chat_reply(websocket):
    start = websocket.receive_json()
    chunks: list[dict] = []
    while True:
        message = websocket.receive_json()
        if message["type"] == "assistant_chunk":
            chunks.append(message)
            continue
        return start, chunks, message


def _complete_search_and_play_script(websocket, request_id: str, session_id: str):
    dry_run_message = websocket.receive_json()
    assert dry_run_message == {
        "type": "dry_run_script",
        "requestId": request_id,
        "script": dry_run_message["script"],
    }
    assert dry_run_message["script"]["timeoutMs"] == 30000
    websocket.send_json(
        {
            "type": "script_dry_run_result",
            "requestId": request_id,
            "ok": True,
            "result": {
                "autoExecutable": True,
                "requiresApproval": False,
                "riskLevel": "low",
                "domains": ["search", "playback"],
                "mutationKinds": ["playback_control"],
                "targetKinds": ["track"],
            },
        }
    )

    validate_message = websocket.receive_json()
    assert validate_message == {
        "type": "validate_script",
        "requestId": request_id,
        "script": validate_message["script"],
    }
    assert validate_message["script"]["timeoutMs"] == 30000
    websocket.send_json(
        {
            "type": "script_validation_result",
            "requestId": request_id,
            "ok": True,
            "result": {
                "ok": True,
                "stepCount": 2,
                "autoExecutable": True,
            },
        }
    )

    execute_message = websocket.receive_json()
    assert execute_message == {
        "type": "execute_script",
        "requestId": request_id,
        "script": execute_message["script"],
    }
    assert execute_message["script"]["steps"][0]["action"] == "searchTracks"
    assert execute_message["script"]["steps"][1]["action"] == "playTrack"

    websocket.send_json(
        {
            "type": "script_execution_started",
            "requestId": request_id,
            "executionId": f"{request_id}-exec",
            "summary": {
                "title": execute_message["script"].get("title", ""),
                "stepCount": len(execute_message["script"]["steps"]),
                "startedAt": "2026-03-29T10:00:00Z",
                "timeoutMs": execute_message["script"]["timeoutMs"],
            },
        }
    )
    websocket.send_json(
        {
            "type": "script_step_event",
            "requestId": request_id,
            "executionId": f"{request_id}-exec",
            "stepIndex": 0,
            "status": "started",
            "payload": {"action": "searchTracks"},
        }
    )
    websocket.send_json(
        {
            "type": "script_step_event",
            "requestId": request_id,
            "executionId": f"{request_id}-exec",
            "stepIndex": 1,
            "status": "finished",
            "payload": {
                "stepId": "play_first",
                "action": "playTrack",
                "ok": True,
                "result": {"track": {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"}},
            },
        }
    )
    websocket.send_json(
        {
            "type": "script_execution_result",
            "requestId": request_id,
            "executionId": f"{request_id}-exec",
            "ok": True,
            "report": {
                "executionId": f"{request_id}-exec",
                "ok": True,
                "status": "succeeded",
                "title": execute_message["script"].get("title", ""),
                "startedAt": "2026-03-29T10:00:00Z",
                "finishedAt": "2026-03-29T10:00:02Z",
                "durationMs": 2000,
                "steps": [
                    {
                        "stepId": "search",
                        "action": "searchTracks",
                        "ok": True,
                        "result": {
                            "items": [
                                {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"},
                            ]
                        },
                    },
                    {
                        "stepId": "play_first",
                        "action": "playTrack",
                        "ok": True,
                        "result": {"track": {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"}},
                    },
                ],
                "result": {
                    "lastResult": {"track": {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"}},
                    "savedResults": {
                        "search": {
                            "items": [
                                {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"},
                            ]
                        }
                    },
                },
            },
        }
    )
    start, chunks, final = _consume_chat_reply(websocket)
    assert start == {
        "type": "assistant_start",
        "sessionId": session_id,
        "requestId": request_id,
    }
    assert "".join(chunk["delta"] for chunk in chunks) == "已开始播放 **晴天** - **周杰伦**。"
    assert final == {
        "type": "assistant_final",
        "sessionId": session_id,
        "requestId": request_id,
        "content": "已开始播放 **晴天** - **周杰伦**。",
    }


def _start_search_and_play_script(websocket, request_id: str, content: str):
    websocket.send_json({"type": "user_message", "requestId": request_id, "content": content})
    dry_run_message = websocket.receive_json()
    assert dry_run_message["type"] == "dry_run_script"
    assert dry_run_message["requestId"] == request_id
    assert dry_run_message["script"]["timeoutMs"] == 30000
    websocket.send_json(
        {
            "type": "script_dry_run_result",
            "requestId": request_id,
            "ok": True,
            "result": {
                "autoExecutable": True,
                "requiresApproval": False,
                "riskLevel": "low",
                "domains": ["search", "playback"],
                "mutationKinds": ["playback_control"],
                "targetKinds": ["track"],
            },
        }
    )
    validate_message = websocket.receive_json()
    assert validate_message["type"] == "validate_script"
    assert validate_message["requestId"] == request_id
    assert validate_message["script"]["timeoutMs"] == 30000
    websocket.send_json(
        {
            "type": "script_validation_result",
            "requestId": request_id,
            "ok": True,
            "result": {"ok": True, "stepCount": 2, "autoExecutable": True},
        }
    )
    execute_message = websocket.receive_json()
    assert execute_message["type"] == "execute_script"
    assert execute_message["requestId"] == request_id
    websocket.send_json(
        {
            "type": "script_execution_started",
            "requestId": request_id,
            "executionId": f"{request_id}-exec",
            "summary": {
                "title": execute_message["script"].get("title", ""),
                "stepCount": len(execute_message["script"]["steps"]),
                "startedAt": "2026-03-29T10:00:00Z",
                "timeoutMs": execute_message["script"]["timeoutMs"],
            },
        }
    )
    return execute_message


def test_healthz_degraded_without_api_key():
    app = create_app(
        settings=Settings(
            _env_file=None,
            OPENAI_MODEL="test-model",
            AGENT_STORAGE_PATH="test-storage.db",
            AGENT_MAX_HISTORY_MESSAGES=20,
        )
    )
    client = TestClient(app)

    response = client.get("/healthz")

    assert response.status_code == 200
    assert response.json() == {
        "status": "degraded",
        "modelConfigured": False,
        "missingConfig": ["OPENAI_API_KEY"],
        "openaiBaseUrl": None,
        "openaiModel": "test-model",
        "openaiWireApi": "chat_completions",
        "sessionHistoryLimit": 20,
        "storagePath": "test-storage.db",
        "protocolVersion": "1.6",
        "capabilities": ["chat", "streaming", "sessions", "storage"],
        "toolModeEnabled": False,
        "auditEnabled": True,
        "directWriteActionsEnabled": True,
        "capabilityCatalogVersion": "2026-03-30-facade-aware",
        "capabilityExecutionModel": {
            "phase": 6,
            "entryPoint": "AgentCapabilityFacade",
            "entryPointKind": "facade",
            "backingExecutor": "AgentToolExecutor",
            "toolCallPath": "tool_call -> AgentChatViewModel -> AgentCapabilityFacade -> AgentToolExecutor",
            "scriptStepPath": "script_step -> AgentScriptExecutor -> AgentCapabilityFacade -> AgentToolExecutor",
            "notes": [
                "客户端当前已完成统一能力入口第一刀，tool_call 和脚本步骤执行都先走 AgentCapabilityFacade。",
                "AgentCapabilityFacade 目前仍是统一外观层，底层实际执行器仍为 AgentToolExecutor。",
                "服务端应把客户端认知从“零散工具集合”逐步切换为“统一 façade 背后的能力图谱”。",
            ],
        },
        "worldStateEnabled": False,
    }


def test_capabilities_endpoint_exposes_catalog_metadata():
    app, _, _ = make_app()
    client = TestClient(app)

    response = client.get("/capabilities")

    assert response.status_code == 200
    payload = response.json()
    assert payload["version"] == "2026-03-30-facade-aware"
    assert payload["totalCount"] >= payload["exposedCount"] >= 1
    assert payload["clientArchitecture"]["entryPoint"] == "AgentCapabilityFacade"
    assert payload["clientArchitecture"]["backingExecutor"] == "AgentToolExecutor"
    play_playlist = next(item for item in payload["items"] if item["name"] == "playPlaylist")
    assert play_playlist["stability"] == "partial"
    assert play_playlist["automationPolicy"] == "confirm"
    assert play_playlist["exposedToBackend"] is True
    assert play_playlist["requiresStructuredObject"] is True
    assert play_playlist["clientEntryPoint"] == "AgentCapabilityFacade"
    assert play_playlist["invocationPaths"] == ["tool_call", "script_step"]
    play_track = next(item for item in payload["items"] if item["name"] == "playTrack")
    assert play_track["automationPolicy"] == "auto"
    assert play_track["requiresStructuredObject"] is True
    stop_playback = next(item for item in payload["items"] if item["name"] == "stopPlayback")
    assert stop_playback["automationPolicy"] == "auto"
    assert stop_playback["exposedToBackend"] is True
    assert stop_playback["requiresStructuredObject"] is False
    add_favorite = next(item for item in payload["items"] if item["name"] == "addFavorite")
    assert add_favorite["automationPolicy"] == "auto"
    assert add_favorite["requiresStructuredObject"] is True


def test_websocket_streams_start_chunks_and_final_message():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat") as websocket:
        ready = websocket.receive_json()
        assert ready["type"] == "session_ready"
        assert ready["sessionId"]
        assert ready["title"] == "新建会话"
        assert ready["capabilities"] == ["chat", "streaming", "sessions", "storage", "tools", "plans", "approval", "audit", "scripts"]

        websocket.send_json({"type": "user_message", "requestId": "req-1", "content": "你好"})
        start = websocket.receive_json()
        first_chunk = websocket.receive_json()
        second_chunk = websocket.receive_json()
        reply = websocket.receive_json()

    assert start == {
        "type": "assistant_start",
        "sessionId": ready["sessionId"],
        "requestId": "req-1",
    }
    assert first_chunk == {
        "type": "assistant_chunk",
        "sessionId": ready["sessionId"],
        "requestId": "req-1",
        "delta": "echo:",
    }
    assert second_chunk == {
        "type": "assistant_chunk",
        "sessionId": ready["sessionId"],
        "requestId": "req-1",
        "delta": "你好",
    }
    assert reply == {
        "type": "assistant_final",
        "sessionId": ready["sessionId"],
        "requestId": "req-1",
        "content": "echo:你好",
    }


def test_reconnect_with_same_session_keeps_context():
    app, model, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=session-1") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "content": "第一句"})
        _consume_chat_reply(websocket)

    with client.websocket_connect("/ws/chat?session_id=session-1") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "content": "第二句"})
        _consume_chat_reply(websocket)

    assert model.stream_calls[1][1:] == [
        {"role": "user", "content": "第一句"},
        {"role": "assistant", "content": "echo:第一句"},
        {"role": "user", "content": "第二句"},
    ]


def test_history_is_trimmed_to_limit():
    app, _, store = make_app(max_history_messages=4)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=trim-session") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "content": "one"})
        _consume_chat_reply(websocket)
        websocket.send_json({"type": "user_message", "content": "two"})
        _consume_chat_reply(websocket)
        websocket.send_json({"type": "user_message", "content": "three"})
        _consume_chat_reply(websocket)

    assert store.get_messages("trim-session") == [
        {"role": "user", "content": "one"},
        {"role": "assistant", "content": "echo:one"},
        {"role": "user", "content": "two"},
        {"role": "assistant", "content": "echo:two"},
        {"role": "user", "content": "three"},
        {"role": "assistant", "content": "echo:three"},
    ]
    assert store.get_messages("trim-session", limit=4) == [
        {"role": "user", "content": "two"},
        {"role": "assistant", "content": "echo:two"},
        {"role": "user", "content": "three"},
        {"role": "assistant", "content": "echo:three"},
    ]


def test_stop_playback_issues_stop_tool_instead_of_replaying_track():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=stop-playback") as websocket:
        websocket.receive_json()

        websocket.send_json({"type": "user_message", "requestId": "req-play", "content": "播放周杰伦的晴天"})
        _complete_search_and_play_script(websocket, "req-play", "stop-playback")

        websocket.send_json({"type": "user_message", "requestId": "req-stop", "content": "停止播放"})
        stop_call = websocket.receive_json()
        assert stop_call["type"] == "tool_call"
        assert stop_call["tool"] == "stopPlayback"
        assert stop_call["tool"] != "playTrack"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": stop_call["toolCallId"],
                "ok": True,
                "result": {"playing": False},
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start == {
        "type": "assistant_start",
        "sessionId": "stop-playback",
        "requestId": "req-stop",
    }
    assert "".join(chunk["delta"] for chunk in chunks) == "已停止播放。"
    assert final == {
        "type": "assistant_final",
        "sessionId": "stop-playback",
        "requestId": "req-stop",
        "content": "已停止播放。",
    }


def test_invalid_message_returns_structured_error():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=invalid-session") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "content": "   "})
        reply = websocket.receive_json()

    assert reply == {
        "type": "error",
        "sessionId": "invalid-session",
        "requestId": None,
        "code": "invalid_message",
        "message": "content must be a non-empty string",
    }


def test_unknown_type_returns_error():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=bad-type") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "ping"})
        reply = websocket.receive_json()

    assert reply == {
        "type": "error",
        "sessionId": "bad-type",
        "requestId": None,
        "code": "unsupported_message_type",
        "message": "supported message types are user_message, tool_result, approval_response, script_dry_run_result, script_validation_result, script_execution_started, script_step_event, script_execution_result, and script_cancellation_result",
    }


def test_model_not_configured_returns_error():
    app = create_app(
        settings=Settings(
            _env_file=None,
            AGENT_STORAGE_PATH="test-storage.db",
            AGENT_MAX_HISTORY_MESSAGES=20,
        )
    )
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=no-model") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-2", "content": "hello"})
        reply = websocket.receive_json()

    assert reply == {
        "type": "error",
        "sessionId": "no-model",
        "requestId": "req-2",
        "code": "model_not_configured",
        "message": "model configuration is incomplete",
    }


def test_stream_interruption_returns_error_after_partial_chunks():
    settings = Settings(
        _env_file=None,
        OPENAI_API_KEY="test-key",
        OPENAI_MODEL="test-model",
        AGENT_STORAGE_PATH="test-storage.db",
        AGENT_MAX_HISTORY_MESSAGES=20,
    )
    store = InMemorySessionStore(max_history_messages=20)
    chat_agent = ChatAgent(llm_client=InterruptedStreamChatModelClient(), session_store=store)
    app = create_app(settings=settings, chat_agent=chat_agent)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=stream-error") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-3", "content": "hello"})
        start = websocket.receive_json()
        chunk = websocket.receive_json()
        error = websocket.receive_json()

    assert start == {
        "type": "assistant_start",
        "sessionId": "stream-error",
        "requestId": "req-3",
    }
    assert chunk == {
        "type": "assistant_chunk",
        "sessionId": "stream-error",
        "requestId": "req-3",
        "delta": "partial",
    }
    assert error == {
        "type": "error",
        "sessionId": "stream-error",
        "requestId": "req-3",
        "code": "stream_interrupted",
        "message": "stream failed",
    }
    assert store.get_messages("stream-error") == []


def test_stream_failure_before_any_chunk_falls_back_to_complete():
    settings = Settings(
        _env_file=None,
        OPENAI_API_KEY="test-key",
        OPENAI_MODEL="test-model",
        AGENT_STORAGE_PATH="test-storage.db",
        AGENT_MAX_HISTORY_MESSAGES=20,
    )
    store = InMemorySessionStore(max_history_messages=20)
    chat_agent = ChatAgent(llm_client=FallbackStreamChatModelClient(), session_store=store)
    app = create_app(settings=settings, chat_agent=chat_agent)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=stream-fallback") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-4", "content": "hello"})
        start = websocket.receive_json()
        chunk = websocket.receive_json()
        final = websocket.receive_json()

    assert start == {
        "type": "assistant_start",
        "sessionId": "stream-fallback",
        "requestId": "req-4",
    }
    assert chunk == {
        "type": "assistant_chunk",
        "sessionId": "stream-fallback",
        "requestId": "req-4",
        "delta": "echo:hello",
    }
    assert final == {
        "type": "assistant_final",
        "sessionId": "stream-fallback",
        "requestId": "req-4",
        "content": "echo:hello",
    }
    assert store.get_messages("stream-fallback") == [
        {"role": "user", "content": "hello"},
        {"role": "assistant", "content": "echo:hello"},
    ]


def test_session_rest_endpoints_support_list_create_and_history():
    app, _, _ = make_app()
    client = TestClient(app)

    create_response = client.post("/sessions", json={"title": "解释一下多态"})
    assert create_response.status_code == 200
    session = create_response.json()
    session_id = session["sessionId"]
    assert session["title"] == "解释一下多态"
    assert session["messageCount"] == 0

    list_response = client.get("/sessions")
    assert list_response.status_code == 200
    assert any(item["sessionId"] == session_id for item in list_response.json()["items"])

    with client.websocket_connect(f"/ws/chat?session_id={session_id}") as websocket:
        ready = websocket.receive_json()
        assert ready["sessionId"] == session_id
        assert ready["title"] == "解释一下多态"
        websocket.send_json({"type": "user_message", "requestId": "req-5", "content": "多态是什么"})
        _consume_chat_reply(websocket)

    history_response = client.get(f"/sessions/{session_id}/messages")
    assert history_response.status_code == 200
    payload = history_response.json()
    assert payload["session"]["sessionId"] == session_id
    assert [item["role"] for item in payload["items"]] == ["user", "assistant"]
    assert [item["content"] for item in payload["items"]] == ["多态是什么", "echo:多态是什么"]


def test_default_session_title_is_derived_from_first_user_message():
    app, _, store = make_app()
    client = TestClient(app)
    session = store.create_session()

    with client.websocket_connect(f"/ws/chat?session_id={session.session_id}") as websocket:
        ready = websocket.receive_json()
        assert ready["title"] == "新建会话"
        websocket.send_json(
            {
                "type": "user_message",
                "requestId": "req-6",
                "content": "解释一下 C++ 的多态和虚函数机制",
            }
        )
        _consume_chat_reply(websocket)

    updated = store.get_session(session.session_id)
    assert updated is not None
    assert updated.title == "解释一下 C++ 的多态和虚函数机制"
    search_response = client.get("/sessions", params={"query": "多态"})
    assert search_response.status_code == 200
    assert any(item["sessionId"] == session.session_id for item in search_response.json()["items"])


def test_get_current_track_flow_uses_tool_call_and_summarizes_result():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-current-track") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-7", "content": "查看当前播放歌曲"})
        tool_call = websocket.receive_json()
        assert tool_call == {
            "type": "tool_call",
            "toolCallId": tool_call["toolCallId"],
            "sessionId": "tool-current-track",
            "tool": "getCurrentTrack",
            "args": {},
        }

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"title": "晴天", "artist": "周杰伦"},
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start["type"] == "assistant_start"
    assert "".join(chunk["delta"] for chunk in chunks) == "当前播放的是 **晴天** - **周杰伦**。"
    assert final["content"] == "当前播放的是 **晴天** - **周杰伦**。"


def test_play_track_unique_hit_chains_search_and_play():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-play-track") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-8", "content": "播放周杰伦的晴天"})
        _complete_search_and_play_script(websocket, "req-8", "tool-play-track")


def test_script_lifecycle_events_are_written_to_audit_stream():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=script-audit") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-script", "content": "播放周杰伦的晴天"})
        _complete_search_and_play_script(websocket, "req-script", "script-audit")

    events_response = client.get("/sessions/script-audit/events")
    assert events_response.status_code == 200
    event_types = [item["eventType"] for item in events_response.json()["items"]]
    assert event_types == [
        "execution_substrate_selected",
        "dry_run_script",
        "script_dry_run_result",
        "validate_script",
        "script_validation_result",
        "execute_script",
        "script_execution_started",
        "script_step_event",
        "script_step_event",
        "script_execution_result",
    ]
    started_event = next(item for item in events_response.json()["items"] if item["eventType"] == "script_execution_started")
    assert started_event["payload"]["summary"]["startedAt"] == "2026-03-29T10:00:00Z"
    assert started_event["payload"]["summary"]["timeoutMs"] == 30000
    dry_run_event = next(item for item in events_response.json()["items"] if item["eventType"] == "script_dry_run_result")
    assert dry_run_event["payload"]["result"]["domains"] == ["search", "playback"]
    assert dry_run_event["payload"]["result"]["mutationKinds"] == ["playback_control"]
    assert dry_run_event["payload"]["result"]["targetKinds"] == ["track"]
    result_event = next(item for item in events_response.json()["items"] if item["eventType"] == "script_execution_result")
    assert result_event["payload"]["report"]["status"] == "succeeded"
    assert result_event["payload"]["report"]["durationMs"] == 2000


def test_running_script_can_be_cancelled_and_records_cancellation_events():
    app, _, _ = make_app()
    client = TestClient(app)
    runtime = app.state.music_runtime
    state = runtime.memory_store.get("script-cancel")
    state.pending_script_execution = PendingScriptExecution(
        request_id="req-play",
        user_message="播放周杰伦的晴天",
        script={"version": 1, "timeoutMs": 30000, "steps": []},
        execution_id="req-play-exec",
    )
    state.workflow_mode = "tool"
    state.goal_status = "running"

    with client.websocket_connect("/ws/chat?session_id=script-cancel") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-cancel", "content": "取消当前执行"})
        cancel_message = websocket.receive_json()
        assert cancel_message == {
            "type": "cancel_script",
            "requestId": "req-cancel",
            "executionId": "req-play-exec",
            "reason": "用户主动取消",
        }

        websocket.send_json(
            {
                "type": "script_cancellation_result",
                "requestId": "req-cancel",
                "executionId": "req-play-exec",
                "ok": True,
                "result": {
                    "accepted": True,
                    "executionId": "req-play-exec",
                },
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)
        assert start == {
            "type": "assistant_start",
            "sessionId": "script-cancel",
            "requestId": "req-cancel",
        }
        assert final["requestId"] == "req-cancel"

        websocket.send_json(
            {
                "type": "script_execution_result",
                "requestId": "req-play",
                "executionId": "req-play-exec",
                "ok": False,
                "report": {
                    "executionId": "req-play-exec",
                    "status": "cancelled",
                    "startedAt": "2026-03-29T10:00:00Z",
                    "finishedAt": "2026-03-29T10:00:03Z",
                    "durationMs": 3000,
                    "steps": [],
                },
                "error": {
                    "code": "script_cancelled",
                    "message": "脚本执行已取消",
                    "retryable": False,
                },
            }
        )

    events_response = client.get("/sessions/script-cancel/events")
    assert events_response.status_code == 200
    event_types = [item["eventType"] for item in events_response.json()["items"]]
    assert event_types == ["cancel_script", "script_cancellation_result", "script_execution_result"]


def test_play_track_ambiguous_result_requires_clarification_then_plays_selection():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-clarify-track") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-9", "content": "播放晴天"})
        search_call = websocket.receive_json()
        assert search_call["tool"] == "searchTracks"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": search_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"},
                        {"trackId": "track-2", "title": "晴天", "artist": "五月天"},
                    ]
                },
            }
        )
        clarification = websocket.receive_json()
        assert clarification == {
            "type": "clarification_request",
            "sessionId": "tool-clarify-track",
            "requestId": "req-9",
            "question": "我找到了多个候选歌曲，请告诉我你想播放哪一个。",
            "options": ["1. 周杰伦 - 晴天", "2. 五月天 - 晴天"],
        }

        websocket.send_json({"type": "user_message", "requestId": "req-10", "content": "第一个"})
        play_call = websocket.receive_json()
        assert play_call["tool"] == "playTrack"
        assert play_call["args"] == {"trackId": "track-1"}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": play_call["toolCallId"],
                "ok": True,
                "result": {"track": {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"}},
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)

    assert "".join(chunk["delta"] for chunk in chunks) == "已开始播放 **晴天** - **周杰伦**。"
    assert final["content"] == "已开始播放 **晴天** - **周杰伦**。"


def test_get_playlists_and_play_playlist_flow():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-playlist") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-11", "content": "有哪些歌单"})
        list_call = websocket.receive_json()
        assert list_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": list_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"playlistId": "pl-1", "name": "夜跑歌单", "trackCount": 18},
                        {"playlistId": "pl-2", "name": "学习歌单", "trackCount": 26},
                    ]
                },
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)
        assert final["content"] == "当前可用歌单如下：\n1. **夜跑歌单**（18 首）\n2. **学习歌单**（26 首）"
        assert "".join(chunk["delta"] for chunk in chunks) == final["content"]

        websocket.send_json({"type": "user_message", "requestId": "req-12", "content": "播放这个歌单"})
        play_list_call = websocket.receive_json()
        assert play_list_call["tool"] == "playPlaylist"
        assert play_list_call["args"] == {"playlistId": "pl-1"}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": play_list_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "pl-1", "name": "夜跑歌单"}},
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)

    assert "".join(chunk["delta"] for chunk in chunks) == "已开始播放歌单 **夜跑歌单**。"
    assert final["content"] == "已开始播放歌单 **夜跑歌单**。"


def test_query_named_playlist_automatically_uses_get_playlists():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=query-playlist") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-query-1", "content": "帮我查询我的流行歌单"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"playlistId": "pl-pop", "name": "流行歌单", "trackCount": 18},
                        {"playlistId": "pl-rock", "name": "摇滚歌单", "trackCount": 12},
                    ]
                },
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)

    assert "".join(chunk["delta"] for chunk in chunks) == "找到了歌单 **流行歌单**（18 首）。"
    assert final["content"] == "找到了歌单 **流行歌单**（18 首）。"


def test_query_named_playlist_matches_normalized_playlist_name():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=query-playlist-normalized") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-query-1b", "content": "帮我查询流行歌单"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"playlistId": "pl-pop", "name": "流行", "trackCount": 18},
                        {"playlistId": "pl-rock", "name": "摇滚", "trackCount": 12},
                    ]
                },
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)

    assert "".join(chunk["delta"] for chunk in chunks) == "找到了歌单 **流行**（18 首）。"
    assert final["content"] == "找到了歌单 **流行**（18 首）。"


def test_inspect_playlist_tracks_automatically_chains_lookup_and_fetch():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=inspect-playlist-tracks") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-query-2", "content": "看看流行歌单里有什么歌"})
        lookup_call = websocket.receive_json()
        assert lookup_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": lookup_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"playlistId": "pl-pop", "name": "流行歌单", "trackCount": 18},
                        {"playlistId": "pl-rock", "name": "摇滚歌单", "trackCount": 12},
                    ]
                },
            }
        )
        tracks_call = websocket.receive_json()
        assert tracks_call == {
            "type": "tool_call",
            "toolCallId": tracks_call["toolCallId"],
            "sessionId": "inspect-playlist-tracks",
            "tool": "getPlaylistTracks",
            "args": {"playlistId": "pl-pop"},
        }

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tracks_call["toolCallId"],
                "ok": True,
                "result": {
                    "playlist": {"playlistId": "pl-pop", "name": "流行歌单", "trackCount": 18},
                    "items": [
                        {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"},
                        {"trackId": "track-2", "title": "稻香", "artist": "周杰伦"},
                    ],
                },
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)

    assert "".join(chunk["delta"] for chunk in chunks) == (
        "歌单 **流行歌单** 里有这些歌曲：\n"
        "1. **晴天** - **周杰伦**\n"
        "2. **稻香** - **周杰伦**"
    )
    assert final["content"] == "".join(chunk["delta"] for chunk in chunks)


def test_inspect_this_playlist_uses_last_named_playlist_context():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=inspect-this-playlist") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-query-3", "content": "帮我查询我的流行歌单"})
        lookup_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": lookup_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "pl-pop", "name": "流行歌单", "trackCount": 18}]},
            }
        )
        _consume_chat_reply(websocket)

        websocket.send_json({"type": "user_message", "requestId": "req-query-4", "content": "看看这个歌单里有什么歌"})
        tracks_call = websocket.receive_json()
        assert tracks_call["tool"] == "getPlaylistTracks"
        assert tracks_call["args"] == {"playlistId": "pl-pop"}


def test_get_recent_tracks_flow_uses_tool_and_summarizes_result():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=recent-tracks") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-query-5", "content": "最近听了什么"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getRecentTracks"
        assert tool_call["args"] == {"limit": 10}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"},
                        {"trackId": "track-2", "title": "七里香", "artist": "周杰伦"},
                    ]
                },
            }
        )
        _, chunks, final = _consume_chat_reply(websocket)

    assert "".join(chunk["delta"] for chunk in chunks) == (
        "最近播放的歌曲如下：\n"
        "1. **晴天** - **周杰伦**\n"
        "2. **七里香** - **周杰伦**"
    )
    assert final["content"] == "".join(chunk["delta"] for chunk in chunks)


def test_tool_result_without_pending_call_returns_error():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-no-pending") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "tool_result", "toolCallId": "tool-1", "ok": True, "result": {}})
        reply = websocket.receive_json()

    assert reply == {
        "type": "error",
        "sessionId": "tool-no-pending",
        "requestId": None,
        "code": "invalid_tool_result",
        "message": "no pending tool call",
    }


def test_unknown_tool_call_id_returns_error():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-bad-id") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-13", "content": "查看当前播放歌曲"})
        websocket.receive_json()
        websocket.send_json({"type": "tool_result", "toolCallId": "tool-other", "ok": True, "result": {}})
        reply = websocket.receive_json()

    assert reply == {
        "type": "error",
        "sessionId": "tool-bad-id",
        "requestId": None,
        "code": "invalid_tool_result",
        "message": "unknown toolCallId",
    }


def test_tool_result_timeout_returns_error_and_clears_pending_call():
    app, _, _ = make_app(tool_timeout_seconds=0.01)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=tool-timeout") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-14", "content": "查看当前播放歌曲"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getCurrentTrack"
        error = websocket.receive_json()

    assert error == {
        "type": "error",
        "sessionId": "tool-timeout",
        "requestId": "req-14",
        "code": "tool_result_timeout",
        "message": "等待工具 getCurrentTrack 的结果超时",
    }


def test_create_playlist_plan_requires_approval_then_executes_tool():
    app, _, store = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=plan-create-playlist") as websocket:
        ready = websocket.receive_json()
        assert ready["capabilities"] == ["chat", "streaming", "sessions", "storage", "tools", "plans", "approval", "audit", "scripts"]

        websocket.send_json({"type": "user_message", "requestId": "req-15", "content": "创建一个学习歌单"})
        plan_preview = websocket.receive_json()
        approval_request = websocket.receive_json()

        assert plan_preview == {
            "type": "plan_preview",
            "planId": plan_preview["planId"],
            "sessionId": "plan-create-playlist",
            "summary": "创建歌单“学习歌单”",
            "riskLevel": "medium",
            "steps": [{"stepId": "step-1", "title": "创建歌单 学习歌单", "status": "pending"}],
        }
        assert approval_request == {
            "type": "approval_request",
            "planId": plan_preview["planId"],
            "sessionId": "plan-create-playlist",
            "message": "即将创建歌单“学习歌单”，是否继续？",
            "riskLevel": "medium",
        }

        websocket.send_json({"type": "approval_response", "planId": plan_preview["planId"], "approved": True})
        progress = websocket.receive_json()
        tool_call = websocket.receive_json()

        assert progress == {
            "type": "progress",
            "planId": plan_preview["planId"],
            "stepId": "step-1",
            "message": "已批准，开始执行计划",
        }
        assert tool_call == {
            "type": "tool_call",
            "toolCallId": tool_call["toolCallId"],
            "sessionId": "plan-create-playlist",
            "tool": "createPlaylist",
            "args": {"name": "学习歌单"},
        }

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "playlist-1", "name": "学习歌单", "trackCount": 0}},
            }
        )
        final_result = websocket.receive_json()
        start = websocket.receive_json()
        chunk = websocket.receive_json()
        final = websocket.receive_json()

    assert final_result == {
        "type": "final_result",
        "planId": plan_preview["planId"],
        "sessionId": "plan-create-playlist",
        "ok": True,
        "summary": "已创建歌单 **学习歌单**。",
    }
    assert start == {
        "type": "assistant_start",
        "sessionId": "plan-create-playlist",
        "requestId": "req-15",
    }
    assert chunk == {
        "type": "assistant_chunk",
        "sessionId": "plan-create-playlist",
        "requestId": "req-15",
        "delta": "已创建歌单 **学习歌单**。",
    }
    assert final == {
        "type": "assistant_final",
        "sessionId": "plan-create-playlist",
        "requestId": "req-15",
        "content": "已创建歌单 **学习歌单**。",
    }
    assert store.get_messages("plan-create-playlist") == [
        {"role": "user", "content": "创建一个学习歌单"},
        {"role": "assistant", "content": "已创建歌单 **学习歌单**。"},
    ]


def test_create_playlist_plan_can_be_rejected():
    app, _, store = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=plan-reject-playlist") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-16", "content": "新建一个夜跑歌单"})
        plan_preview = websocket.receive_json()
        approval_request = websocket.receive_json()

        assert plan_preview["type"] == "plan_preview"
        assert approval_request["type"] == "approval_request"

        websocket.send_json({"type": "approval_response", "planId": plan_preview["planId"], "approved": False})
        final_result = websocket.receive_json()
        start = websocket.receive_json()
        chunk = websocket.receive_json()
        final = websocket.receive_json()

    assert final_result == {
        "type": "final_result",
        "planId": plan_preview["planId"],
        "sessionId": "plan-reject-playlist",
        "ok": False,
        "summary": "已取消计划：创建歌单“夜跑歌单”",
    }
    assert start["type"] == "assistant_start"
    assert chunk["delta"] == "已取消计划：创建歌单“夜跑歌单”"
    assert final["content"] == "已取消计划：创建歌单“夜跑歌单”"
    assert store.get_messages("plan-reject-playlist") == [
        {"role": "user", "content": "新建一个夜跑歌单"},
        {"role": "assistant", "content": "已取消计划：创建歌单“夜跑歌单”"},
    ]


def test_create_playlist_with_top_tracks_runs_multi_step_plan():
    app, _, store = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=plan-create-with-top") as websocket:
        websocket.receive_json()
        websocket.send_json(
            {
                "type": "user_message",
                "requestId": "req-17",
                "content": "创建一个学习歌单并添加最近常听歌曲",
            }
        )
        plan_preview = websocket.receive_json()
        approval_request = websocket.receive_json()

        assert plan_preview == {
            "type": "plan_preview",
            "planId": plan_preview["planId"],
            "sessionId": "plan-create-with-top",
            "summary": "创建歌单“学习歌单”并添加最近常听歌曲",
            "riskLevel": "high",
            "steps": [
                {"stepId": "step-1", "title": "创建歌单 学习歌单", "status": "pending"},
                {"stepId": "step-2", "title": "获取最近常听 20 首歌曲", "status": "pending"},
                {"stepId": "step-3", "title": "将最近常听歌曲加入歌单", "status": "pending"},
            ],
        }
        assert approval_request == {
            "type": "approval_request",
            "planId": plan_preview["planId"],
            "sessionId": "plan-create-with-top",
            "message": "即将创建歌单“学习歌单”并添加 20 首最近常听歌曲，是否继续？",
            "riskLevel": "high",
        }

        websocket.send_json({"type": "approval_response", "planId": plan_preview["planId"], "approved": True})
        progress_1 = websocket.receive_json()
        create_call = websocket.receive_json()

        assert progress_1 == {
            "type": "progress",
            "planId": plan_preview["planId"],
            "stepId": "step-1",
            "message": "已批准，开始执行计划",
        }
        assert create_call == {
            "type": "tool_call",
            "toolCallId": create_call["toolCallId"],
            "sessionId": "plan-create-with-top",
            "tool": "createPlaylist",
            "args": {"name": "学习歌单"},
        }

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": create_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "playlist-1", "name": "学习歌单", "trackCount": 0}},
            }
        )
        progress_2 = websocket.receive_json()
        top_tracks_call = websocket.receive_json()

        assert progress_2 == {
            "type": "progress",
            "planId": plan_preview["planId"],
            "stepId": "step-2",
            "message": "歌单已创建，正在获取最近常听歌曲",
        }
        assert top_tracks_call == {
            "type": "tool_call",
            "toolCallId": top_tracks_call["toolCallId"],
            "sessionId": "plan-create-with-top",
            "tool": "getTopPlayedTracks",
            "args": {"limit": 20},
        }

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": top_tracks_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"trackId": "track-1", "title": "晴天", "artist": "周杰伦"},
                        {"trackId": "track-2", "title": "七里香", "artist": "周杰伦"},
                    ]
                },
            }
        )
        progress_3 = websocket.receive_json()
        add_call = websocket.receive_json()

        assert progress_3 == {
            "type": "progress",
            "planId": plan_preview["planId"],
            "stepId": "step-3",
            "message": "已拿到最近常听歌曲，正在加入歌单",
        }
        assert add_call == {
            "type": "tool_call",
            "toolCallId": add_call["toolCallId"],
            "sessionId": "plan-create-with-top",
            "tool": "addTracksToPlaylist",
            "args": {"playlistId": "playlist-1", "trackIds": ["track-1", "track-2"]},
        }

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": add_call["toolCallId"],
                "ok": True,
                "result": {"playlistId": "playlist-1", "addedCount": 2, "skippedCount": 0},
            }
        )
        final_result = websocket.receive_json()
        start = websocket.receive_json()
        chunk = websocket.receive_json()
        final = websocket.receive_json()

    assert final_result == {
        "type": "final_result",
        "planId": plan_preview["planId"],
        "sessionId": "plan-create-with-top",
        "ok": True,
        "summary": "已创建歌单 **学习歌单**，并添加 2 首最近常听歌曲。",
    }
    assert start == {
        "type": "assistant_start",
        "sessionId": "plan-create-with-top",
        "requestId": "req-17",
    }
    assert chunk == {
        "type": "assistant_chunk",
        "sessionId": "plan-create-with-top",
        "requestId": "req-17",
        "delta": "已创建歌单 **学习歌单**，并添加 2 首最近常听歌曲。",
    }
    assert final == {
        "type": "assistant_final",
        "sessionId": "plan-create-with-top",
        "requestId": "req-17",
        "content": "已创建歌单 **学习歌单**，并添加 2 首最近常听歌曲。",
    }
    assert store.get_messages("plan-create-with-top") == [
        {"role": "user", "content": "创建一个学习歌单并添加最近常听歌曲"},
        {"role": "assistant", "content": "已创建歌单 **学习歌单**，并添加 2 首最近常听歌曲。"},
    ]


def test_event_endpoints_return_plan_and_tool_audit_trail():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=audit-session") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-18", "content": "创建一个学习歌单"})
        plan_preview = websocket.receive_json()
        approval_request = websocket.receive_json()

        websocket.send_json({"type": "approval_response", "planId": plan_preview["planId"], "approved": True})
        progress = websocket.receive_json()
        tool_call = websocket.receive_json()

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "playlist-2", "name": "学习歌单", "trackCount": 0}},
            }
        )
        websocket.receive_json()
        websocket.receive_json()
        websocket.receive_json()
        websocket.receive_json()

    session_events_response = client.get("/sessions/audit-session/events")
    assert session_events_response.status_code == 200
    session_events = session_events_response.json()["items"]
    assert [item["eventType"] for item in session_events] == [
        "plan_preview",
        "approval_request",
        "approval_response",
        "progress",
        "tool_call",
        "tool_result",
        "final_result",
    ]
    assert session_events[0]["planId"] == plan_preview["planId"]
    assert session_events[4]["payload"]["tool"] == "createPlaylist"
    assert session_events[5]["payload"]["toolCallId"] == tool_call["toolCallId"]

    plan_events_response = client.get(f"/plans/{plan_preview['planId']}/events")
    assert plan_events_response.status_code == 200
    plan_events = plan_events_response.json()
    assert plan_events["planId"] == plan_preview["planId"]
    assert [item["eventType"] for item in plan_events["items"]] == [
        "plan_preview",
        "approval_request",
        "approval_response",
        "progress",
        "tool_call",
        "tool_result",
        "final_result",
    ]


def test_approval_response_without_pending_plan_returns_error():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=plan-no-approval") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "approval_response", "planId": "plan-1", "approved": True})
        reply = websocket.receive_json()

    assert reply == {
        "type": "error",
        "sessionId": "plan-no-approval",
        "requestId": None,
        "code": "invalid_approval_response",
        "message": "no pending approval",
    }


def test_invalid_structured_semantic_parse_falls_back_to_legacy_rules():
    settings = Settings(
        _env_file=None,
        OPENAI_API_KEY="test-key",
        OPENAI_MODEL="test-model",
        AGENT_STORAGE_PATH="test-storage.db",
        AGENT_MAX_HISTORY_MESSAGES=20,
    )
    store = InMemorySessionStore(max_history_messages=20)
    chat_agent = ChatAgent(llm_client=InvalidStructuredChatModelClient(), session_store=store)
    app = create_app(settings=settings, chat_agent=chat_agent)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-fallback") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-fallback-1", "content": "查看当前播放歌曲"})
        tool_call = websocket.receive_json()

    assert tool_call["tool"] == "getCurrentTrack"


def test_generic_semantic_output_is_refined_into_playlist_lookup():
    model = GenericStructuredChatModelClient({"帮我查询流行歌单"})
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-refine-playlist") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-semantic-1", "content": "帮我查询流行歌单"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start["type"] == "assistant_start"
    assert "".join(chunk["delta"] for chunk in chunks) == "找到了歌单 **流行**（18 首）。"
    assert final == {
        "type": "assistant_final",
        "sessionId": "semantic-refine-playlist",
        "requestId": "req-semantic-1",
        "content": "找到了歌单 **流行**（18 首）。",
    }


def test_generic_semantic_output_can_continue_with_playlist_reference():
    model = GenericStructuredChatModelClient({"帮我查询流行歌单", "看看这个歌单里有什么歌"})
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-refine-reference") as websocket:
        websocket.receive_json()

        websocket.send_json({"type": "user_message", "requestId": "req-semantic-2", "content": "帮我查询流行歌单"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getPlaylists"
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        _consume_chat_reply(websocket)

        websocket.send_json({"type": "user_message", "requestId": "req-semantic-3", "content": "看看这个歌单里有什么歌"})
        second_tool_call = websocket.receive_json()

    assert second_tool_call["tool"] == "getPlaylistTracks"
    assert second_tool_call["args"] == {"playlistId": "playlist-1"}


def test_generic_semantic_output_can_continue_with_implicit_followup_phrase():
    model = GenericStructuredChatModelClient({"帮我查询流行歌单", "那里面有什么歌"})
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-refine-implicit-followup") as websocket:
        websocket.receive_json()

        websocket.send_json({"type": "user_message", "requestId": "req-semantic-4", "content": "帮我查询流行歌单"})
        tool_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        _consume_chat_reply(websocket)

        websocket.send_json({"type": "user_message", "requestId": "req-semantic-5", "content": "那里面有什么歌"})
        second_tool_call = websocket.receive_json()

    assert second_tool_call["tool"] == "getPlaylistTracks"
    assert second_tool_call["args"] == {"playlistId": "playlist-1"}


def test_runtime_tracks_goal_and_semantic_history():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-memory-history") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-semantic-6", "content": "帮我查询流行歌单"})
        tool_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        _consume_chat_reply(websocket)

    state = app.state.music_runtime.memory_store.get("semantic-memory-history")
    assert state.goal_history
    assert state.goal_history[-1] == "query_playlist"
    assert state.semantic_history
    assert state.semantic_history[-1]["intent"] == "query_playlist"
    assert state.focus_domain == "playlist"


def test_llm_proposed_tool_can_directly_issue_play_playlist():
    model = ProposedToolChatModelClient()
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-llm-proposal") as websocket:
        websocket.receive_json()

        websocket.send_json({"type": "user_message", "requestId": "req-proposal-1", "content": "帮我查询流行歌单"})
        tool_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        _consume_chat_reply(websocket)

        websocket.send_json({"type": "user_message", "requestId": "req-proposal-2", "content": "就放刚才那个"})
        second_tool_call = websocket.receive_json()

    assert second_tool_call["tool"] == "playPlaylist"
    assert second_tool_call["args"] == {"playlistId": "playlist-1"}


def test_llm_action_candidates_can_chain_playlist_lookup_then_play():
    model = ActionCandidatesChatModelClient()
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-action-candidates") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-candidates-1", "content": "帮我查一下流行歌单，找到就播放"})
        first_tool_call = websocket.receive_json()
        assert first_tool_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": first_tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        second_tool_call = websocket.receive_json()

    assert second_tool_call["tool"] == "playPlaylist"
    assert second_tool_call["args"] == {"playlistId": "playlist-1"}


def test_llm_action_candidates_can_chain_playlist_tracks_then_play_first_track():
    model = PlaylistTracksActionCandidatesChatModelClient()
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=semantic-action-candidates-tracks") as websocket:
        websocket.receive_json()
        websocket.send_json(
            {
                "type": "user_message",
                "requestId": "req-candidates-2",
                "content": "看看流行歌单里有什么歌，然后播放第一首",
            }
        )
        playlists_call = websocket.receive_json()
        assert playlists_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": playlists_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "??", "trackCount": 18}]},
            }
        )
        playlist_tracks_call = websocket.receive_json()
        assert playlist_tracks_call["tool"] == "getPlaylistTracks"
        assert playlist_tracks_call["args"] == {"playlistId": "playlist-1"}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": playlist_tracks_call["toolCallId"],
                "ok": True,
                "result": {
                    "playlist": {"playlistId": "playlist-1", "name": "??"},
                    "items": [
                        {"trackId": "track-1", "title": "???"},
                        {"trackId": "track-2", "title": "???"},
                    ],
                },
            }
        )
        play_track_call = websocket.receive_json()

    assert play_track_call["tool"] == "playTrack"
    assert play_track_call["args"] == {"trackId": "track-1"}


def test_candidate_events_are_written_to_audit_stream():
    model = ActionCandidatesChatModelClient()
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=candidate-audit-session") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-candidates-3", "content": "帮我查一下流行歌单，找到就播放"})
        playlists_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": playlists_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-1", "name": "流行", "trackCount": 18}]},
            }
        )
        play_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": play_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "playlist-1", "name": "流行"}},
            }
        )
        _consume_chat_reply(websocket)

    response = client.get("/sessions/candidate-audit-session/events")
    assert response.status_code == 200
    event_types = [item["eventType"] for item in response.json()["items"]]
    assert event_types == [
        "action_candidate_selected",
        "tool_call",
        "tool_result",
        "action_candidate_observed",
        "action_candidate_selected",
        "tool_call",
        "tool_result",
        "action_candidate_observed",
    ]


def test_llm_action_candidates_can_directly_execute_create_playlist():
    model = DirectWriteActionCandidatesChatModelClient()
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=direct-write-create") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-write-1", "content": "直接创建一个学习歌单"})
        create_call = websocket.receive_json()
        assert create_call["tool"] == "createPlaylist"
        assert create_call["args"] == {"name": "学习歌单"}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": create_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "playlist-write-1", "name": "学习歌单"}},
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start["type"] == "assistant_start"
    assert chunks[-1]["delta"] == "已创建歌单 **学习歌单**。"
    assert final["content"] == "已创建歌单 **学习歌单**。"


def test_llm_action_candidates_can_directly_execute_create_playlist_with_top_tracks():
    model = DirectWriteActionCandidatesChatModelClient()
    app, _, _ = make_app(model=model)
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=direct-write-multistep") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-write-2", "content": "直接创建一个学习歌单并添加最近常听歌曲"})
        create_call = websocket.receive_json()
        assert create_call["tool"] == "createPlaylist"
        assert create_call["args"] == {"name": "学习歌单"}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": create_call["toolCallId"],
                "ok": True,
                "result": {"playlist": {"playlistId": "playlist-write-2", "name": "学习歌单"}},
            }
        )
        top_call = websocket.receive_json()
        assert top_call["tool"] == "getTopPlayedTracks"
        assert top_call["args"] == {"limit": 20}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": top_call["toolCallId"],
                "ok": True,
                "result": {
                    "items": [
                        {"trackId": "track-1", "title": "第一首"},
                        {"trackId": "track-2", "title": "第二首"},
                    ]
                },
            }
        )
        add_call = websocket.receive_json()
        assert add_call["tool"] == "addTracksToPlaylist"
        assert add_call["args"] == {"playlistId": "playlist-write-2", "trackIds": ["track-1", "track-2"]}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": add_call["toolCallId"],
                "ok": True,
                "result": {"addedCount": 2, "playlist": {"playlistId": "playlist-write-2", "name": "学习歌单"}},
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start["type"] == "assistant_start"
    assert chunks[-1]["delta"] == "已向歌单 **学习歌单** 添加 2 首歌曲。"
    assert final["content"] == "已向歌单 **学习歌单** 添加 2 首歌曲。"


def test_direct_write_action_candidates_can_fall_back_to_plan_when_disabled():
    model = DirectWriteActionCandidatesChatModelClient()
    app, _, _ = make_app(model=model, allow_direct_write_actions=False)
    client = TestClient(app)

    healthz = client.get("/healthz")
    assert healthz.status_code == 200
    assert healthz.json()["directWriteActionsEnabled"] is False

    with client.websocket_connect("/ws/chat?session_id=direct-write-disabled") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-write-3", "content": "直接创建一个学习歌单"})
        plan_preview = websocket.receive_json()
        approval_request = websocket.receive_json()

    assert plan_preview["type"] == "plan_preview"
    assert approval_request["type"] == "approval_request"


def test_compound_playlist_subset_request_prefers_script_when_source_playlist_is_uniquely_known():
    app, _, _ = make_app()
    client = TestClient(app)
    state = app.state.music_runtime.memory_store.get("playlist-subset-script")
    state.recent_playlist_candidates = [{"playlistId": "playlist-source-1", "name": "流行", "trackCount": 18}]

    with client.websocket_connect("/ws/chat?session_id=playlist-subset-script") as websocket:
        websocket.receive_json()
        websocket.send_json(
            {
                "type": "user_message",
                "requestId": "req-subset-1",
                "content": "重新创建一个歌单，叫做周杰伦，把流行歌单前三首音乐放到这个歌单里面",
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start["type"] == "assistant_start"
    assert final["content"] == "当前客户端暂未向服务端开放能力：addTracksToPlaylist。本轮未执行任何客户端操作。"
    assert "".join(chunk["delta"] for chunk in chunks) == final["content"]

    events_response = client.get("/sessions/playlist-subset-script/events")
    items = events_response.json()["items"]
    event_types = [item["eventType"] for item in items]
    assert "execution_substrate_rejected" in event_types
    blocked_event = next(item for item in items if item["eventType"] == "execution_substrate_rejected")
    assert blocked_event["payload"]["routeReason"].startswith("unsupported_capability:addTracksToPlaylist")
    assert blocked_event["payload"]["metadata"]["blockedCapability"] == "addTracksToPlaylist"


def test_compound_playlist_subset_request_blocks_before_tool_chain_when_capability_not_supported():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=playlist-subset-transfer-last") as websocket:
        websocket.receive_json()
        websocket.send_json(
            {
                "type": "user_message",
                "requestId": "req-subset-2",
                "content": "重新创建一个歌单，叫做周杰伦，把流行歌单后两首音乐放到这个歌单里面",
            }
        )
        start, chunks, final = _consume_chat_reply(websocket)

    assert start["type"] == "assistant_start"
    assert final["content"] == "当前客户端暂未向服务端开放能力：addTracksToPlaylist。本轮未执行任何客户端操作。"
    assert "".join(chunk["delta"] for chunk in chunks) == final["content"]

    events_response = client.get("/sessions/playlist-subset-transfer-last/events")
    items = events_response.json()["items"]
    event_types = [item["eventType"] for item in items]
    assert "execution_substrate_rejected" in event_types
    assert "tool_call" not in event_types
    blocked_event = next(item for item in items if item["eventType"] == "execution_substrate_rejected")
    assert blocked_event["payload"]["metadata"]["blockedCapability"] == "addTracksToPlaylist"


def test_recent_track_listing_request_does_not_replay_last_named_track():
    app, _, _ = make_app()
    session_id = "recent-listing-guard"
    state = app.state.music_runtime.memory_store.get(session_id)
    state.last_named_track = {"trackId": "track-old-1", "title": "爱情废柴", "artist": "周杰伦"}
    client = TestClient(app)

    with client.websocket_connect(f"/ws/chat?session_id={session_id}") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-recent-list", "content": "列出最近播放列表的所有音乐"})
        tool_call = websocket.receive_json()
        assert tool_call["tool"] == "getRecentTracks"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"trackId": "track-1", "title": "晴天", "artist": "周杰伦"}]},
            }
        )
        _, _, final = _consume_chat_reply(websocket)

    assert "晴天" in final["content"]


def test_playlist_content_listing_request_fetches_playlist_tracks():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=playlist-content-listing") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-playlist-content", "content": "列出流行歌单的所有音乐"})
        playlists_call = websocket.receive_json()
        assert playlists_call["tool"] == "getPlaylists"

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": playlists_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "playlist-source-3", "name": "流行", "trackCount": 4}]},
            }
        )
        playlist_tracks_call = websocket.receive_json()
        assert playlist_tracks_call["tool"] == "getPlaylistTracks"
        assert playlist_tracks_call["args"] == {"playlistId": "playlist-source-3"}

        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": playlist_tracks_call["toolCallId"],
                "ok": True,
                "result": {
                    "playlist": {"playlistId": "playlist-source-3", "name": "流行"},
                    "items": [
                        {"trackId": "track-1", "title": "第一首", "artist": "歌手A"},
                        {"trackId": "track-2", "title": "第二首", "artist": "歌手B"},
                    ],
                },
            }
        )
        _, _, final = _consume_chat_reply(websocket)

    assert "歌单 **流行** 里有这些歌曲" in final["content"]


def test_world_state_summary_tracks_current_playback_and_known_playlists():
    app, _, _ = make_app()
    client = TestClient(app)

    with client.websocket_connect("/ws/chat?session_id=world-state-session") as websocket:
        websocket.receive_json()
        websocket.send_json({"type": "user_message", "requestId": "req-world-1", "content": "有哪些歌单"})
        tool_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": tool_call["toolCallId"],
                "ok": True,
                "result": {"items": [{"playlistId": "pl-1", "name": "流行", "trackCount": 18}]},
            }
        )
        _consume_chat_reply(websocket)

        websocket.send_json({"type": "user_message", "requestId": "req-world-2", "content": "查看当前播放歌曲"})
        current_track_call = websocket.receive_json()
        websocket.send_json(
            {
                "type": "tool_result",
                "toolCallId": current_track_call["toolCallId"],
                "ok": True,
                "result": {
                    "playing": True,
                    "positionMs": 12000,
                    "durationMs": 180000,
                    "track": {"trackId": "track-1", "title": "晴天", "artist": "周杰伦", "musicPath": "C:/music/qingtian.mp3"},
                },
            }
        )
        _consume_chat_reply(websocket)

    state = app.state.music_runtime.memory_store.get("world-state-session")
    assert state.current_playback is not None
    assert state.current_playback["track"]["trackId"] == "track-1"
    assert state.recent_playlist_candidates[0]["playlistId"] == "pl-1"
