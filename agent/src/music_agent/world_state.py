from __future__ import annotations

from typing import Any

from .workflow_memory import WorkflowState


def build_world_state_summary(state: WorkflowState) -> dict[str, Any]:
    target_playlist = state.resolved_entities.get("target_playlist")
    source_playlist = state.resolved_entities.get("source_playlist")
    track_selection = state.resolved_entities.get("track_selection")

    playlist_tracks_by_playlist: dict[str, list[dict[str, Any]]] = {}
    if state.last_named_playlist and state.last_named_playlist.get("playlistId") and state.recent_playlist_tracks:
        playlist_tracks_by_playlist[str(state.last_named_playlist["playlistId"])] = [
            _summarize_track(item) for item in state.recent_playlist_tracks[:10]
        ]

    return {
        "currentPlayback": _summarize_playback(state.current_playback),
        "playbackQueue": _summarize_queue(state.playback_queue),
        "knownPlaylists": [_summarize_playlist(item) for item in state.recent_playlist_candidates[:10]],
        "knownTracks": [_summarize_track(item) for item in state.recent_track_candidates[:10]],
        "recentTracks": [_summarize_track(item) for item in state.recent_recent_tracks[:10]],
        "playlistTracksByPlaylist": playlist_tracks_by_playlist,
        "targetPlaylist": _summarize_playlist(target_playlist),
        "sourcePlaylist": _summarize_playlist(source_playlist),
        "lastNamedPlaylist": _summarize_playlist(state.last_named_playlist),
        "lastNamedTrack": _summarize_track(state.last_named_track),
        "trackSelection": track_selection,
        "lastToolObservation": state.last_tool_observation,
    }


def _summarize_playlist(playlist: dict | None) -> dict[str, Any] | None:
    if not playlist:
        return None
    return {
        "playlistId": playlist.get("playlistId"),
        "name": playlist.get("name"),
        "trackCount": playlist.get("trackCount"),
    }


def _summarize_track(track: dict | None) -> dict[str, Any] | None:
    if not track:
        return None
    return {
        "trackId": track.get("trackId"),
        "title": track.get("title"),
        "artist": track.get("artist"),
        "album": track.get("album"),
        "musicPath": track.get("musicPath"),
    }


def _summarize_playback(playback: dict | None) -> dict[str, Any] | None:
    if not playback:
        return None
    track = playback.get("track") if isinstance(playback, dict) else None
    return {
        "playing": playback.get("playing"),
        "positionMs": playback.get("positionMs"),
        "durationMs": playback.get("durationMs"),
        "track": _summarize_track(track if isinstance(track, dict) else playback),
    }


def _summarize_queue(queue: dict | None) -> dict[str, Any] | None:
    if not queue:
        return None
    items = queue.get("items") if isinstance(queue, dict) else None
    return {
        "mode": queue.get("mode"),
        "currentIndex": queue.get("currentIndex"),
        "items": [_summarize_track(item) for item in (items or [])[:10] if isinstance(item, dict)],
    }
