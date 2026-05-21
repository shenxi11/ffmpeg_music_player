Goal
- Validate whether the missing bottom control bar is a layer/z-order problem by lifting it into its own floating overlay host above the video stage.

Completed
- Updated `src/ui/widgets/video/video_player_page.h` and `src/ui/widgets/video/video_player_page.cpp`.
- Added `m_floatingStageOverlayHost` as a dedicated overlay host above the stage under `m_contentCard`.
- Reparented `m_stageOverlay` into the floating host and enabled always-on-top style attributes on the host.
- Updated `updateStageLayout()` to position the floating host over the stage rect in `m_contentCard` coordinates and keep the bottom control bar pinned to the host bottom.
- Updated `updateStageState()` and hover hit-testing so the lifted overlay still counts as part of the active stage region.
- Rebuilt successfully with `E:\cmake-3.31.0-rc1-windows-x86_64\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player -j 8`.
- Ran a short launch smoke against `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Verify on the real screen whether the bottom control bar is finally visible during hover.
- If it is still invisible, the next pass should add temporary logging for the floating host and `m_stageOverlay` geometry/visibility/opacity to distinguish a geometry issue from a painting issue.

Risks
- `Qt::WA_AlwaysStackOnTop` on the floating host is a deliberate experiment; if it causes any unexpected overlap outside the stage, the next pass should remove it and rely on manual `raise()` plus a different parent/container strategy.
- This smoke test only confirms startup and build stability, not actual z-order parity on the user’s machine.

Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `setupUi()`, `updateStageLayout()`, `updateStageState()`, and `isCursorInsideStage()`.
- If visual failure remains, add temporary qDebug output for `m_floatingStageOverlayHost` and `m_stageOverlay` right after geometry updates.
