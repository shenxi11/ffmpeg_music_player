from music_agent.client_script_planner import ClientScriptPlanner
from music_agent.execution_router import ExecutionRouter
from music_agent.semantic_models import (
    PlaylistSemanticEntity,
    SemanticEntities,
    SemanticParseResult,
    TrackSelectionEntity,
)
from music_agent.workflow_memory import WorkflowState


def test_execution_router_blocks_subset_transfer_when_required_capability_is_not_exposed():
    router = ExecutionRouter(ClientScriptPlanner(), allow_direct_write_actions=True)
    state = WorkflowState(session_id="route-1")
    state.recent_playlist_candidates = [{"playlistId": "pl-1", "name": "流行", "trackCount": 18}]
    semantic = SemanticParseResult(
        mode="tool",
        intent="create_playlist_from_playlist_subset",
        targetDomain="playlist",
        shouldAutoExecute=True,
        entities=SemanticEntities(
            targetPlaylist=PlaylistSemanticEntity(rawQuery="周杰伦", normalizedQuery="周杰伦"),
            sourcePlaylist=PlaylistSemanticEntity(rawQuery="流行歌单", normalizedQuery="流行"),
            trackSelection=TrackSelectionEntity(mode="first_n", count=3),
        ),
    )

    decision = router.route(semantic, state)

    assert decision.status == "blocked"
    assert decision.substrate == "none"
    assert decision.reason.startswith("unsupported_capability:addTracksToPlaylist")
    assert decision.metadata["blockedCapability"] == "addTracksToPlaylist"


def test_execution_router_blocks_subset_transfer_before_tool_chain_fallback_when_capability_missing():
    router = ExecutionRouter(ClientScriptPlanner(), allow_direct_write_actions=True)
    state = WorkflowState(session_id="route-2")
    semantic = SemanticParseResult(
        mode="tool",
        intent="create_playlist_from_playlist_subset",
        targetDomain="playlist",
        shouldAutoExecute=True,
        entities=SemanticEntities(
            targetPlaylist=PlaylistSemanticEntity(rawQuery="周杰伦", normalizedQuery="周杰伦"),
            sourcePlaylist=PlaylistSemanticEntity(rawQuery="流行歌单", normalizedQuery="流行"),
            trackSelection=TrackSelectionEntity(mode="first_n", count=3),
        ),
    )

    decision = router.route(semantic, state)

    assert decision.status == "blocked"
    assert decision.substrate == "none"
    assert decision.reason.startswith("unsupported_capability:addTracksToPlaylist")


def test_execution_router_blocks_compound_write_when_direct_write_disabled():
    router = ExecutionRouter(ClientScriptPlanner(), allow_direct_write_actions=False)
    state = WorkflowState(session_id="route-3")
    semantic = SemanticParseResult(
        mode="tool",
        intent="create_playlist_from_playlist_subset",
        targetDomain="playlist",
        shouldAutoExecute=True,
        entities=SemanticEntities(
            targetPlaylist=PlaylistSemanticEntity(rawQuery="周杰伦", normalizedQuery="周杰伦"),
            sourcePlaylist=PlaylistSemanticEntity(rawQuery="流行歌单", normalizedQuery="流行"),
            trackSelection=TrackSelectionEntity(mode="first_n", count=3),
        ),
    )

    decision = router.route(semantic, state)

    assert decision.status == "blocked"
    assert decision.substrate == "none"
