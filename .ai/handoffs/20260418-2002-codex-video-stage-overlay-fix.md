## Goal
Remove the floating sibling video control-bar host and make the bottom control bar track the full video stage width and bottom edge reliably.

## Completed
- Removed `m_floatingStageOverlayHost` from `src/ui/widgets/video/video_player_page.*`.
- Reparented `m_stageOverlay` to `VideoStageChromeHost` so the control bar now lives inside the stage.
- Simplified `updateStageLayout()` so the bottom control bar geometry is derived directly from `m_stageCard` size.
- Simplified fullscreen reparenting so only the stage moves into `m_videoFullscreenHost`; the control bar follows automatically.
- Passed `Debug` build with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `src/ui/widgets/video/video_player_page.h`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Run an in-app visual check to confirm the bottom control bar now reaches the stage right edge on the actual video page.
- If there is still a visual mismatch, inspect the stage card width itself rather than reviving a floating overlay host.

## Risks
- The current fix assumes the desired width baseline is the full `VideoStageCard`, not the white content card or the visible video image rect.
- The worktree still contains unrelated dirty changes in shell/build/docs files; future sessions must avoid mixing those into video follow-ups.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on `updateStageLayout()`, `updateStageState()`, and `applyHostWindowState()`.
