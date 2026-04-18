## Goal
- Fix the playlist UI mismatch where switching to another already-queued song changed the wrong row text/cover even though audio playback switched correctly.
- Keep queue order unchanged and only move the current-playing state.
- Limit the fix to playback-and-audio without changing `AudioService` queue semantics.

## Completed
- Removed the eager `AudioService::currentIndexChanged -> PlaybackViewModel::playlistSnapshotChanged` connection so selecting an existing queued song no longer rebuilds the whole playlist immediately on index switch.
- Updated `PlaybackViewModel::buildPlaylistSnapshotItem()` to apply `m_currentTitle`, `m_currentArtist`, and `m_currentAlbumArt` only when the row's normalized path matches the active track path, instead of applying them to whatever row happens to be at `currentIndex`.
- Added `PlaybackViewModel::currentPlaybackMetadataKey()` as the normalized active-track key helper for that ownership check.
- Updated `PlayWidget::setupPlaybackViewModelConnections()` so `currentFilePathChanged` immediately updates `PlaylistHistory` current-path and paused state without rebuilding the list.
- Verified:
- `E:\CMake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`
- short launch smoke test of `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe` for 6 seconds without immediate exit/crash

## Changed Scope
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\viewmodels\PlaybackViewModel.cpp`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\viewmodels\PlaybackViewModel.h`
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\widgets\playback\play_widget.viewmodel_connections.cpp`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\ownership.yaml`
- `E:\FFmpeg_whisper\ffmpeg_music_player\.ai\tasks.yaml`

## Open Work
- Manual regression is still needed for the exact issue path:
- play song A, then play song B so the queue becomes `[A, B]`
- open the playback playlist
- click A/B repeatedly while they are already in the queue
- confirm queue order does not change and only the active row state changes
- Also recheck:
- newly added songs still append to the tail
- album art arrival later still updates the correct current row
- playlist remove/clear behavior is unchanged

## Risks
- The fix relies on normalized-path identity; if a future playback path representation differs between the active track and the queue entry, the metadata-ownership guard may become too strict and leave fallback titles visible longer than expected.
- This change intentionally does not optimize away other later full-snapshot refreshes; it only removes the premature one tied to index switching.

## Next Entry Point
- Queue snapshot ownership:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\viewmodels\PlaybackViewModel.cpp`
- UI-side current row sync:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\widgets\playback\play_widget.viewmodel_connections.cpp`
