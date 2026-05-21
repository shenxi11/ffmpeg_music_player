## Goal
Fix the video-only fullscreen black-screen regression and update the fixed bottom control bar progress groove to a transparent background.

## Completed
- Added renderer lifecycle helpers in `src/ui/widgets/video/video_player_page.h` and `src/ui/widgets/video/video_player_page.cpp`.
- Introduced `configureRendererWidget()`, `scheduleRendererRebuild()`, and `recreateRendererInCurrentHost()` so fullscreen transitions rebuild `VideoRendererGL` after the player container is reparented into the dedicated fullscreen host.
- Rebound the current `VideoSession` to the rebuilt renderer and explicitly restarted / paused / stopped the renderer based on the current `MediaSession` state.
- Kept the existing lightweight `scheduleRendererRecovery()` path as a secondary refresh after the rebuild.
- Updated `QSlider#VideoProgressSlider::groove:horizontal` to use a transparent background.
- Passed `Debug` build with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Run the app and verify that entering video-only fullscreen no longer shows a black picture while audio keeps playing.
- Confirm visually that the progress groove background is now transparent while the played segment and handle remain visible.

## Risks
- This fix assumes the fullscreen black-screen root cause is the `QOpenGLWidget` surface becoming invalid after cross-top-level reparent and therefore rebuilds the renderer rather than changing the fullscreen product behavior.
- The video player page files remain untracked in the current worktree and still need intentional staging during PR prep.

## Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `applyHostWindowState()`, `scheduleRendererRebuild()`, and `recreateRendererInCurrentHost()`.
