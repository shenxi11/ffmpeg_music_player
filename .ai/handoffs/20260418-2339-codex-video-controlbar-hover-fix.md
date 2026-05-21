## Goal
Fix the embedded video page so the bottom operation bar appears as one complete hover overlay instead of degrading into a bare always-visible progress line.

## Completed
- Updated `src/ui/widgets/video/video_player_page.cpp` and `src/ui/widgets/video/video_player_page.h`.
- Changed the stage-bottom controls to be a true hover-only overlay instead of mixing pause/hover visibility.
- Added a short delayed hide timer so moving between the video stage and overlay controls does not cause flicker.
- Extended stage hover tracking to the relevant child widgets inside the video stage.
- Removed playback-rate and local-open from the main bottom stage bar.
- Moved `打开本地视频` into the lower info/action area so the bottom bar is closer to the approved HTML draft.
- Kept the existing playback pipeline and maximize logic intact.
- Passed a Debug build and a short launch smoke test.

## Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `src/ui/widgets/video/video_player_page.h`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Run a real manual UI check inside the client and confirm that hovering the video stage shows the full bottom control strip, not only the progress line.
- If the overlay still feels too subtle, increase the gradient/background contrast or slightly extend the hide delay.
- If product wants the “下一项” action visible later, it is still present in code but currently hidden from the main bar.

## Risks
- The hover behavior depends on QWidget/OpenGL child event delivery; if a specific machine still drops hover transitions, the next fix should promote this from passive hover tracking to explicit mouse-move polling on the stage.
- `打开本地视频` now lives in the lower info action row, which is closer to the approved draft but changes the old muscle memory for that action.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, especially `setupUi()`, `updateStageState()`, `setStageHovered()`, and `eventFilter()`.
- Verify the behavior in the running client against `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html`.
