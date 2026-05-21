## Goal
Restore the video control bar using the previously stable standalone-window structure instead of an in-stage hover overlay.

## Completed
- Reworked `src/ui/widgets/video/video_player_page.h` and `src/ui/widgets/video/video_player_page.cpp`.
- Added a dedicated `m_playerContainer` that groups the video stage and a fixed `m_controlBar`.
- Moved progress, transport, quality, and fullscreen controls out of the stage overlay path and into the fixed bottom control bar.
- Updated video-only fullscreen so it reparents the whole player container, not only the stage widget.
- Kept picture-level elements like watermark, center play button, subtitle, and next-arrow inside the stage chrome host.
- Passed `Debug` build with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Run the app and visually verify that the bottom control bar is now always visible below the video stage.
- If the user wants to chase the original draft again later, do it as a separate pass from this stable-bottom-bar baseline.

## Risks
- This pass intentionally prioritizes reliable visibility over strict draft parity; the control bar is no longer a picture overlay.
- The video player page files remain untracked in the current worktree and must be staged intentionally during PR prep.

## Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `setupUi()`, `updateStageLayout()`, and `applyHostWindowState()`.
