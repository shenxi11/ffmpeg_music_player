Goal
- Change the bottom video control bar so it matches the full video window/stage width instead of only the visible picture width, while keeping the new video-only fullscreen behavior.

Completed
- Updated `src/ui/widgets/video/video_player_page.cpp`.
- Changed the floating control-bar host geometry back to the full stage/window rect instead of `calculateVisibleVideoRect()`.
- Restored stage-wide hover hit-testing so the control bar stays active across the whole video window area, not just the visible picture area.
- Kept watermark and subtitle positioning on the visible video rect, so content-level overlays still stay attached to the actual picture.
- Rebuilt successfully with `E:\cmake-3.31.0-rc1-windows-x86_64\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player -j 8`.
- Ran a short launch smoke against `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Verify on the real screen that the bottom control bar now spans the full video window width and that the video-only fullscreen behavior still works as intended.
- If the control bar still looks wrong, the next pass should be pure visual tuning: left/right padding, gradient width, and the exact horizontal alignment of the controls within the full-width bar.

Risks
- The control bar now deliberately spans the full stage/window width, so if the user later wants it aligned to the visible picture width again, this change must be reverted independently from the video-only fullscreen work.
- This smoke test only confirms startup and build stability, not final runtime visuals.

Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `updateStageLayout()` and `isCursorInsideStage()`.
- Validate with a fresh runtime screenshot before making any more geometry changes.
