Goal
- Fix the embedded video page so the hover control bar is actually visible at the bottom of the stage instead of disappearing while the right-edge arrow still shows.

Completed
- Updated `src/ui/widgets/video/video_player_page.cpp`.
- Marked the stage overlay as a styled background widget and raised its minimum height to `118`.
- Changed `updateStageLayout()` so the stage children are laid out from the target stage height for the current width instead of relying on the possibly stale `m_stageCard->rect()` height during the same pass.
- Anchored the bottom overlay to `targetHeight - overlayHeight` and re-raised it after geometry updates.
- Added a queued follow-up `updateStageLayout()` + `updateStageState()` pass when the stage height changes so the overlay is corrected after the parent widget settles.
- Rebuilt successfully with `E:\cmake-3.31.0-rc1-windows-x86_64\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player -j 8`.
- Ran a short launch smoke against `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Verify on the real screen that hovering the video now reveals the full bottom control bar, not just the right-edge arrow.
- If the bar is still missing, the next pass should add temporary geometry logging around `updateStageLayout()` and `updateStageState()` to confirm whether the overlay is visible but clipped, or visible and fully transparent.

Risks
- `calculateVisibleVideoRect()` still uses the current stage rect, so watermark/subtitle placement may use the settled geometry one pass later than the first overlay placement when the stage height changes.
- The smoke test confirms buildability and startup only, not visual parity.

Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `updateStageLayout()`, `updateStageState()`, and if needed temporary geometry logs around `m_stageOverlay`.
