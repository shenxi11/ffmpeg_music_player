## Goal
- Restore the video control bar as a transparent overlay over the video picture instead of a fixed white-backed region below the stage.
- Keep the bar always visible.
- Preserve the previously added video-only fullscreen renderer rebuild and recovery path.

## Completed
- Added `m_stageOverlayHost` as a dedicated transparent overlay container above the stage.
- Reparented the control bar into that overlay host and removed it from the normal `playerLayout` flow.
- Updated stage layout logic so the overlay host follows `m_stageCard` in both embedded mode and video-only fullscreen mode.
- Updated fullscreen reparent logic so both `m_playerContainer` and `m_stageOverlayHost` move between `m_contentCard` and `m_videoFullscreenHost`.
- Kept the existing renderer rebuild and renderer recovery scheduling intact.
- Built `Debug` successfully with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`

## Open Work
- Run the client and visually verify that the control bar now sits flush on the bottom of the video stage with no white backing.
- If the overlay is still visually off, continue pixel-level tuning only inside `src/ui/widgets/video/video_player_page.cpp`.
- If any fullscreen z-order issue remains, inspect `updateStageLayout()` and `applyHostWindowState()`.

## Risks
- The control bar is now a sibling overlay above the stage instead of a layout child, so any later resize/path changes must keep `updateStageLayout()` in sync.
- Manual runtime verification is still outstanding; this handoff only confirms a successful Debug build.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on `setupUi()`, `updateStageLayout()`, `applyHostWindowState()`, and `eventFilter()`.
