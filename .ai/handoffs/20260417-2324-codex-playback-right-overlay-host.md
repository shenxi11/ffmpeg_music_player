## Goal
- Remove the lyric surface's effective top-most behavior over playback-side drawers.
- Unify expanded-playback playlist and comments under one shared right overlay host.
- Keep collapsed-playback comments entering the main content page unchanged.

## Completed
- Added a shared right overlay host inside `PlayWidget` and reparented the playback playlist and comment drawer widgets into it.
- Switched expanded-playback right panel behavior from "two sibling drawers" to "one host that swaps between playlist/comments pages".
- Removed `Qt::WA_AlwaysStackOnTop` from the lyric display widget so the right overlay can reliably sit above it.
- Centralized right overlay geometry and `raise()` logic in one code path and kept top-bar re-raise behavior for the collapsed shell.
- Verified the client builds successfully with:
- `E:\CMake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

## Changed Scope
- `src/ui/widgets/playback/play_widget.h`
- `src/ui/widgets/playback/play_widget.cpp`
- `src/ui/widgets/playback/play_widget.controls_connections.cpp`
- `.ai/decisions.md`
- `.ai/tasks.yaml`
- `.ai/ownership.yaml`

## Open Work
- Runtime verification is still needed for the visual stacking order in the real app window:
- expanded playback + playlist open
- expanded playback + comments open
- switch playlist -> comments and comments -> playlist repeatedly
- collapse from expanded state while a right overlay page is open
- If any flicker or stale toggle state appears, continue from the new `showRightOverlayPage/closeRightOverlay/syncCommentOverlayGeometry` path in `PlayWidget`.

## Risks
- The shared right overlay currently uses one common width tuned for comments; playlist will now inherit that width as well.
- The comments page in collapsed mode still uses a separate embedded host in `MainWidget`, so comment tab state is not shared between main-content mode and expanded drawer mode.

## Next Entry Point
- Main entry:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\widgets\playback\play_widget.cpp`
- Toggle wiring:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\ui\widgets\playback\play_widget.controls_connections.cpp`
- If shell resize/layering needs adjustment:
- `E:\FFmpeg_whisper\ffmpeg_music_player\src\app\main_widget.cpp`
