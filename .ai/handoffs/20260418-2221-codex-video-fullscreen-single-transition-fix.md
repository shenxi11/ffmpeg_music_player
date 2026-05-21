## Goal
- Fix the embedded video page regression where entering video-only fullscreen sometimes produced audio with no visible picture.
- Keep the product behavior as video-only fullscreen, not main-window maximize.
- Reduce the fullscreen renderer lifecycle to one controlled transition per toggle.

## Completed
- Added a pending fullscreen renderer transition state to `VideoPlayerPage` with a target fullscreen flag and transition serial.
- Added `beginRendererTransition()` and `tryFinishRendererTransition()` so a fullscreen toggle only rebuilds the renderer after the target host is visible and both host/stage sizes are valid.
- Removed the old behavior where `applyHostWindowState()` and `m_videoFullscreenHost` Show/Resize both independently stacked `scheduleRendererRebuild()` and `scheduleRendererRecovery()` on the same toggle.
- Moved `scheduleRendererRecovery()` to the end of `recreateRendererInCurrentHost()` so the lighter recovery path now runs after each actual rebuild instead of racing in parallel with it.
- Built `Debug` successfully with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/handoffs/20260418-2221-codex-video-fullscreen-single-transition-fix.md`
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`

## Open Work
- Run the client and verify that entering video-only fullscreen no longer shows a black picture while audio continues.
- Check both play-state entry and paused-state entry into fullscreen.
- If black screen still appears, add targeted qDebug output around `beginRendererTransition()`, `tryFinishRendererTransition()`, and `recreateRendererInCurrentHost()` to inspect actual runtime ordering and sizes.

## Risks
- This handoff confirms compile success, not runtime visual verification.
- The fix still assumes the correct strategy is a controlled renderer rebuild after cross-top-level reparent; if the issue persists, the next pass may need to simplify further by dropping rebuild on one direction of the toggle.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on `beginRendererTransition()`, `tryFinishRendererTransition()`, `recreateRendererInCurrentHost()`, `applyHostWindowState()`, and the `m_videoFullscreenHost` branch in `eventFilter()`.
