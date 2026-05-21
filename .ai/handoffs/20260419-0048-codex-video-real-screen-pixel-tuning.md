Goal
- Apply the first pixel-level tuning pass to the embedded video page based on the real runtime screenshot instead of only the HTML draft.

Completed
- Updated `src/ui/widgets/video/video_player_page.h` and `src/ui/widgets/video/video_player_page.cpp`.
- Added `calculateVisibleVideoRect()` so fit-mode overlays can anchor to the actual visible video content area instead of the whole black stage.
- Removed the stage title overlay from the video area to eliminate the white blocks shown in the screenshot.
- Kept the watermark and subtitle overlays, but moved them to visible-video anchoring and added transparent-background attributes for more reliable OpenGL compositing.
- Tightened the lower info/action section: metadata is now one-line elided text with tooltip fallback, action spacing is slightly closer to the draft, and the report button is styled as a lightweight text action instead of another pill chip.
- Rebuilt successfully with `E:\cmake-3.31.0-rc1-windows-x86_64\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player -j 8`.
- Ran a short launch smoke against `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Verify on the real screen that the upper-left white blocks are gone and that the subtitle/watermark now sit inside the visible video content when playing tall videos.
- If there is still a mismatch, the next pass should be small geometry/style tuning only: bottom hover-bar spacing, report-button visual weight, and exact right/bottom offsets for subtitle/watermark.

Risks
- The anchoring now follows fit-mode geometry computed in `VideoPlayerPage`, not a renderer-exposed content rect, so any future renderer-side aspect-rule change must be mirrored here.
- The smoke test confirms startup only; visual parity still depends on user-side runtime confirmation.

Next Entry Point
- Continue in `src/ui/widgets/video/video_player_page.cpp`, mainly `updateInfoPanel()`, `updateStageLayout()`, and `calculateVisibleVideoRect()`.
- Compare the next real screenshot against `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html` before making any further structural changes.
