from music_agent.command_templates import resolve_command_template
from music_agent.semantic_models import (
    PlaylistSemanticEntity,
    SemanticEntities,
    SemanticParseResult,
    TrackSelectionEntity,
)
from music_agent.workflow_memory import WorkflowState


def test_command_template_prefers_script_for_subset_transfer():
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

    decision = resolve_command_template(semantic, WorkflowState(session_id="template-1"))

    assert decision.status == "selected"
    assert decision.template_name == "create_playlist_from_playlist_subset"
    assert decision.preferred_substrate == "script"
    assert "tool_chain" in decision.fallback_substrates


def test_command_template_prefers_tool_chain_for_playlist_tracks():
    semantic = SemanticParseResult(
        mode="tool",
        intent="inspect_playlist_tracks",
        targetDomain="playlist",
        shouldAutoExecute=True,
        entities=SemanticEntities(
            playlist=PlaylistSemanticEntity(rawQuery="流行歌单", normalizedQuery="流行"),
        ),
    )

    decision = resolve_command_template(semantic, WorkflowState(session_id="template-2"))

    assert decision.status == "selected"
    assert decision.preferred_substrate == "tool_chain"
