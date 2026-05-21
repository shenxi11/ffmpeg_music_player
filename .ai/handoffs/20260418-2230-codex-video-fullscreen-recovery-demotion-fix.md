## Goal
- Continue fixing the embedded video page fullscreen black-screen regression after logs showed rebuild now happens once, but recovery still repeatedly rebinds the renderer afterward.
- Keep video-only fullscreen as the product behavior.

## Completed
- Added targeted `VideoPlayerPage` qDebug logs around fullscreen transition start, transition completion, rebuild execution, and recovery execution/skips.
- Removed `scheduleRendererRecovery()` from the end of `recreateRendererInCurrentHost()`.
- Changed `recoverRendererAfterWindowModeChange()` so it now performs only `refreshAfterSurfaceChange()`, `setDisplayMode(...)`, and `update()`; it no longer calls `VideoSession::setVideoRenderer(m_renderWidget)`.
- Added guards so `scheduleRendererRecovery()` and `recoverRendererAfterWindowModeChange()` both skip while a fullscreen renderer transition or rebuild is still pending.
- Stopped `m_observedWindow` `WindowStateChange` events from injecting recovery while video-only fullscreen is active.
- Built `Debug` successfully with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/handoffs/20260418-2230-codex-video-fullscreen-recovery-demotion-fix.md`
- `src/ui/widgets/video/video_player_page.cpp`

## Open Work
- Run the client again and inspect the new `[VideoPlayerPage]` transition logs while reproducing fullscreen black-screen.
- Confirm whether each fullscreen toggle now produces one rebuild and either zero or one light recovery, instead of repeated post-rebuild renderer rebinds.
- If black screen still persists, the next pass should decide whether one transition direction can skip rebuild entirely.

## Risks
- This handoff confirms compile success, not runtime behavior.
- The new logs are intentionally temporary and scoped to the fullscreen lifecycle; remove or downgrade them after the regression is fully resolved.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on `beginRendererTransition()`, `tryFinishRendererTransition()`, `recreateRendererInCurrentHost()`, `scheduleRendererRecovery()`, and `recoverRendererAfterWindowModeChange()`.
