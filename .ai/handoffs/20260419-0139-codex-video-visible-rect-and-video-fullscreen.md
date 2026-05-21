Goal
- Make the bottom video control bar match the actual picture width/bottom edge and switch fullscreen behavior from “maximize the app window” to “video-only fullscreen”.

Completed
- Updated `src/ui/widgets/video/video_player_page.h` and `src/ui/widgets/video/video_player_page.cpp`.
- Added `m_videoFullscreenHost` and `m_videoFullscreenActive` so the video stage can move into a dedicated top-level fullscreen host without resizing the main application window.
- Reworked `applyHostWindowState()` semantics: the fullscreen button now toggles video-only fullscreen, reparents the stage/overlay between `m_contentCard` and `m_videoFullscreenHost`, and updates the button icon/tooltip accordingly.
- Updated `updateStageLayout()` so the floating control-bar host is sized from `calculateVisibleVideoRect()` and therefore matches the actual video picture width and bottom edge instead of the whole stage.
- Updated hover hit-testing and fullscreen host event handling so the lifted overlay stays interactive and `Esc` / `F11` exits video fullscreen.
- Rebuilt successfully with `E:\cmake-3.31.0-rc1-windows-x86_64\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player -j 8`.
- Ran a short launch smoke against `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Verify on the real screen that the bottom control bar width now matches the visible picture width and that the fullscreen button opens a video-only fullscreen surface.
- If the control bar is still missing, the next pass should add temporary qDebug geometry output for `calculateVisibleVideoRect()`, `m_floatingStageOverlayHost`, and `m_stageOverlay` after `updateStageLayout()`.

Risks
- `applyHostWindowState()` still keeps its old name even though it now controls video-only fullscreen; a later cleanup pass may want to rename it for clarity.
- Because the video stage is reparented into a top-level fullscreen host, any future shell-level assumptions about `m_stageCard` always living under `m_contentCard` must account for the fullscreen-active path.

Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `updateStageLayout()`, `applyHostWindowState()`, `eventFilter()`, and `isCursorInsideStage()`.
- Validate with a real runtime screenshot before doing any further polish.
