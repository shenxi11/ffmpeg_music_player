from music_agent.autonomous_planner import AutonomousPlanner
from music_agent.semantic_models import (
    PlaylistSemanticEntity,
    SemanticEntities,
    SemanticParseResult,
    TrackSelectionEntity,
)
from music_agent.workflow_memory import WorkflowState


def test_planner_builds_playlist_lookup_then_play_chain():
    planner = AutonomousPlanner(allow_direct_write_actions=True)
    semantic = SemanticParseResult(
        mode="tool",
        intent="play_playlist",
        targetDomain="playlist",
        shouldAutoExecute=True,
        entities=SemanticEntities(
            playlist=PlaylistSemanticEntity(rawQuery="流行歌单", normalizedQuery="流行")
        ),
    )

    candidates = planner.build_action_candidates(semantic, WorkflowState(session_id="test"))

    assert [item["tool"] for item in candidates] == ["getPlaylists", "playPlaylist"]
    assert candidates[1]["dependsOn"] == ["planner-1"]


def test_planner_builds_subset_transfer_chain():
    planner = AutonomousPlanner(allow_direct_write_actions=True)
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

    candidates = planner.build_action_candidates(semantic, WorkflowState(session_id="test"))

    assert candidates == []


def test_planner_blocks_write_chain_when_direct_write_disabled():
    planner = AutonomousPlanner(allow_direct_write_actions=False)
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

    candidates = planner.build_action_candidates(semantic, WorkflowState(session_id="test"))

    assert candidates == []
