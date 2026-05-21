## Goal
Restore the video bottom control bar so it appears again on hover after the stage-owned overlay refactor.

## Completed
- Added `VideoRendererGL` to the stage hover event chain in `src/ui/widgets/video/video_player_page.cpp`.
- Enabled hover tracking on `m_renderWidget` and installed the shared event filter on it.
- Updated `showEvent()` and `resizeEvent()` to refresh `updateStageState()` after layout changes so overlay visibility does not stay stale.
- Verified a successful Debug build with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Run the client and confirm that moving the mouse over the actual video image now reveals the bottom control bar.
- If the control bar is still invisible at runtime, the next check should be widget stacking over `QOpenGLWidget`, not hover state propagation.

## Risks
- The current fix assumes the invisibility root cause was hover not firing on `m_renderWidget`; it does not change overlay geometry or restore the old floating sibling host.
- The video player page files are still untracked in the current worktree, so later PR prep must stage them intentionally.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, especially the `setupUi()`, `eventFilter()`, `showEvent()`, and `resizeEvent()` paths.
