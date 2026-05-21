## Goal
- Cancel the floating video control-bar strategy.
- Restore the player as a compact bottom-integrated control bar where the progress slider and transport controls share one unified background.
- Keep the existing video-only fullscreen behavior and renderer rebuild fix.

## Completed
- Removed the temporary floating stage overlay host from `VideoPlayerPage`.
- Reparented the control bar back into `m_playerContainer` as a normal layout child below the video stage.
- Reduced the control bar height to `68px` and tightened row spacing/margins for a more compact layout.
- Changed the progress groove to render inside the same dark control-bar background instead of transparent floating styling.
- Restored fullscreen stage-height calculation to reserve space for the integrated bottom bar while preserving the existing renderer rebuild/recovery path.
- Built `Debug` successfully with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`

## Open Work
- Run the client and visually verify that the control bar is now fully integrated below the video stage, with no floating overlay behavior.
- If the bar still feels too tall or too loose, continue pixel-level tuning in `VideoPlayerPage` within the compact target range of roughly `64px` to `72px`.
- If fullscreen alignment still looks off, inspect `updateStageLayout()` and confirm the player container fills the fullscreen host correctly.

## Risks
- This handoff confirms compile success, not final runtime visual parity against the draft.
- The stage-hover logic still considers the control bar as part of the active player region, which is intentional for stable hover behavior but may need revisiting if stage-only hover behavior changes later.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on `setupUi()`, `updateStageLayout()`, `applyHostWindowState()`, and the stylesheet block that defines `VideoControlBar` and `VideoProgressSlider`.
