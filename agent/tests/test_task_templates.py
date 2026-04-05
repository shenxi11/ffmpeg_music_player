from music_agent.semantic_models import (
    PlaylistSemanticEntity,
    SemanticEntities,
    SemanticParseResult,
    TrackSelectionEntity,
)
from music_agent.task_templates import resolve_task_template
from music_agent.workflow_memory import WorkflowState


def test_task_template_builds_playlist_track_inspection_chain():
    semantic = SemanticParseResult(
        mode="tool",
        intent="inspect_playlist_tracks",
        targetDomain="playlist",
        shouldAutoExecute=True,
        entities=SemanticEntities(
            playlist=PlaylistSemanticEntity(rawQuery="流行歌单", normalizedQuery="流行"),
        ),
    )

    plan = resolve_task_template(semantic, WorkflowState(session_id="task-template-1"))

    assert plan.status == "selected"
    assert [step.tool for step in plan.steps] == ["getPlaylists", "getPlaylistTracks"]


def test_task_template_builds_playlist_subset_transfer_chain():
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

    plan = resolve_task_template(semantic, WorkflowState(session_id="task-template-2"))

    assert plan.status == "selected"
    assert [step.tool for step in plan.steps] == [
        "createPlaylist",
        "getPlaylists",
        "getPlaylistTracks",
        "addTracksToPlaylist",
    ]
    assert plan.steps[-1].args == {"playlistRole": "target", "selectionMode": "first_n", "count": 3}
