import asyncio
import json

from music_agent.semantic_models import SemanticParseResult, normalize_playlist_query
from music_agent.semantic_parser import SemanticParser, SemanticParserError
from music_agent.tool_registry import DEFAULT_TOOL_REGISTRY


class FakeSemanticModelClient:
    def __init__(self, response: str) -> None:
        self.response = response
        self.calls: list[list[dict[str, str]]] = []

    async def complete(self, messages: list[dict[str, str]]) -> str:
        return self.response

    async def complete_structured(self, messages: list[dict[str, str]]) -> str:
        self.calls.append(messages)
        return self.response

    async def stream_complete(self, messages: list[dict[str, str]]):
        if False:
            yield ""


def test_normalize_playlist_query_removes_suffix_and_fillers():
    assert normalize_playlist_query("帮我查询流行歌单") == "流行"
    assert normalize_playlist_query("列出流行歌单的所有音乐") == "流行"


def test_semantic_parser_normalizes_playlist_query():
    model = FakeSemanticModelClient(
        json.dumps(
            {
                "mode": "tool",
                "intent": "query_playlist",
                "entities": {
                    "playlist": {"rawQuery": "流行歌单"},
                    "track": None,
                    "artist": None,
                    "album": None,
                    "limit": None,
                },
                "references": [],
                "missingFields": [],
                "ambiguities": [],
                "targetDomain": "playlist",
                "shouldAutoExecute": True,
                "requiresApproval": False,
                "confidence": 0.91,
            },
            ensure_ascii=False,
        )
    )
    parser = SemanticParser(model, DEFAULT_TOOL_REGISTRY)

    result = asyncio.run(parser.parse_user_message("帮我查询流行歌单", {"currentGoal": None}))

    assert isinstance(result, SemanticParseResult)
    assert result.intent == "query_playlist"
    assert result.entities.playlist is not None
    assert result.entities.playlist.raw_query == "流行歌单"
    assert result.entities.playlist.normalized_query == "流行"


def test_semantic_parser_refines_playlist_content_request():
    model = FakeSemanticModelClient(
        json.dumps(
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
                "confidence": 0.20,
            },
            ensure_ascii=False,
        )
    )
    parser = SemanticParser(model, DEFAULT_TOOL_REGISTRY)

    result = asyncio.run(parser.parse_user_message("列出流行歌单的所有音乐", {"currentGoal": None}))

    assert result.intent == "inspect_playlist_tracks"
    assert result.entities.playlist is not None
    assert result.entities.playlist.raw_query == "流行歌单"
    assert result.entities.playlist.normalized_query == "流行"
    assert result.should_auto_execute is True


def test_semantic_parser_refines_reference_playlist_request_from_context():
    model = FakeSemanticModelClient(
        json.dumps(
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
                "confidence": 0.28,
            },
            ensure_ascii=False,
        )
    )
    parser = SemanticParser(model, DEFAULT_TOOL_REGISTRY)

    result = asyncio.run(
        parser.parse_user_message(
            "看看这个歌单里有什么歌",
            {"lastNamedPlaylist": {"playlistId": "pl-1", "name": "流行"}},
        )
    )

    assert result.intent == "inspect_playlist_tracks"
    assert result.references == ["last_named_playlist"]
    assert result.should_auto_execute is True


def test_semantic_parser_understands_recent_tracks_request():
    model = FakeSemanticModelClient(
        json.dumps(
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
                "confidence": 0.22,
            },
            ensure_ascii=False,
        )
    )
    parser = SemanticParser(model, DEFAULT_TOOL_REGISTRY)

    result = asyncio.run(parser.parse_user_message("列出最近播放列表的所有音乐", {"currentGoal": None}))

    assert result.intent == "get_recent_tracks"
    assert result.target_domain == "library"
    assert result.should_auto_execute is True


def test_semantic_parser_understands_playlist_subset_transfer():
    model = FakeSemanticModelClient(
        json.dumps(
            {
                "mode": "chat",
                "intent": "chat",
                "entities": {
                    "playlist": None,
                    "targetPlaylist": None,
                    "sourcePlaylist": None,
                    "trackSelection": None,
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
                "confidence": 0.18,
            },
            ensure_ascii=False,
        )
    )
    parser = SemanticParser(model, DEFAULT_TOOL_REGISTRY)

    result = asyncio.run(
        parser.parse_user_message(
            "创建一个歌单，歌单名为周杰伦，周杰伦歌单里面添加流行歌单的前三首音乐",
            {"currentGoal": None},
        )
    )

    assert result.intent == "create_playlist_from_playlist_subset"
    assert result.entities.target_playlist is not None
    assert result.entities.target_playlist.raw_query == "周杰伦"
    assert result.entities.source_playlist is not None
    assert result.entities.source_playlist.raw_query == "流行歌单"
    assert result.entities.track_selection is not None
    assert result.entities.track_selection.mode == "first_n"
    assert result.entities.track_selection.count == 3
    assert result.should_auto_execute is True


def test_semantic_parser_keeps_llm_proposed_tool():
    model = FakeSemanticModelClient(
        json.dumps(
            {
                "mode": "tool",
                "intent": "play_playlist",
                "entities": {
                    "playlist": None,
                    "track": None,
                    "artist": None,
                    "album": None,
                    "limit": None,
                },
                "references": ["last_named_playlist"],
                "missingFields": [],
                "ambiguities": [],
                "targetDomain": "playlist",
                "shouldAutoExecute": True,
                "requiresApproval": False,
                "confidence": 0.88,
                "proposedTool": {
                    "tool": "playPlaylist",
                    "args": {"playlistId": "pl-1"},
                    "confidence": 0.91,
                    "reason": "上下文里已有明确歌单",
                },
            },
            ensure_ascii=False,
        )
    )
    parser = SemanticParser(model, DEFAULT_TOOL_REGISTRY)

    result = asyncio.run(
        parser.parse_user_message(
            "播放这个歌单",
            {"lastNamedPlaylist": {"playlistId": "pl-1", "name": "流行"}},
        )
    )

    assert result.proposed_tool is not None
    assert result.proposed_tool.tool == "playPlaylist"
    assert result.proposed_tool.args == {"playlistId": "pl-1"}


def test_semantic_parser_raises_for_invalid_json():
    parser = SemanticParser(FakeSemanticModelClient("not-json"), DEFAULT_TOOL_REGISTRY)

    try:
        asyncio.run(parser.parse_user_message("最近听了什么", {"currentGoal": None}))
    except SemanticParserError:
        pass
    else:
        raise AssertionError("expected SemanticParserError")
