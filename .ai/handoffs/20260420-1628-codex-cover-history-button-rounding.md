Goal
- Restore circular styling for the album-cover hover play button and the recent-play history favorite/delete icon buttons without changing the straight-corner baseline for cards, rows, and panels.

Completed
- Updated [SongCoverAction.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/SongCoverAction.qml) so the central hover play/pause button is circular again while the cover shell, overlay, and playing indicator stay straight.
- Updated [PlayHistoryList.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/PlayHistoryList.qml) so the row-level favorite and delete icon buttons are circular again, with existing hover colors, borders, and click actions preserved.
- Validation passed with `qmllint qml/components/library/SongCoverAction.qml qml/components/playback/PlayHistoryList.qml` and `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

Changed Scope
- Shared cover-action hover button geometry.
- Recent play history row action button geometry.
- `.ai/ownership.yaml` and `.ai/tasks.yaml` status updates for the micro-follow-up.

Open Work
- Runtime visual confirmation is still needed to verify the circular cover button looks correct anywhere else that reuses `SongCoverAction`, such as favorites, local music, playlists, and hot chart rows.
- If more missed icon-only buttons are found, continue under the same "buttons round, structure straight" policy rather than broadening back to rounded cards.

Risks
- `SongCoverAction.qml` is a shared component, so the hover play/pause button shape now changes in every reused list surface by design.
- The repository remains heavily dirty from prior sessions; this handoff only guarantees the intent of the two QML changes above plus the `.ai` status write-back.
- The Debug build still emits the existing `VCINSTALLDIR is not set` warning during `windeployqt`; it did not block the build.

Next Entry Point
- For any follow-up visual tweak, start with [SongCoverAction.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/library/SongCoverAction.qml) and [PlayHistoryList.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/PlayHistoryList.qml), then do a quick runtime pass through recent play plus one other page that reuses the cover-action component.
