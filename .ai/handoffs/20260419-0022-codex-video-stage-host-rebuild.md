Goal
- Fix the embedded video page so the bottom control bar actually renders as the approved HTML-draft hover overlay instead of degrading into a lone progress strip.

Completed
- Rebuilt `src/ui/widgets/video/video_player_page.h` and `src/ui/widgets/video/video_player_page.cpp` around a dedicated `m_stageRenderHost` plus `m_stageChromeHost`.
- Moved the stage title block, watermark, subtitle, center play button, right-edge arrow, and bottom control bar into the single chrome host.
- Simplified stage hover handling so it is driven by the stage hosts instead of the earlier fragmented child-widget event list.
- Kept the existing `MediaSession + VideoRendererGL` playback path intact.
- Rebuilt successfully with `E:\cmake-3.31.0-rc1-windows-x86_64\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player -j 8`.
- Ran a short launch smoke against `E:\FFmpeg_whisper\ffmpeg_music_player_build\Debug\ffmpeg_music_player.exe`.

Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Verify on the real screen that the bottom hover control bar is now fully visible and visually close enough to `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html`.
- If the user still sees a mismatch, the next pass should be visual-only tuning of spacing, icon size, and metadata copy rather than another structural rewrite.

Risks
- `VideoRendererGL` is still a `QOpenGLWidget`, so any remaining visual mismatch is more likely about geometry/style tuning than a missing playback pipeline.
- The automated smoke only proves buildability and launchability; it does not prove hover-layer parity on the user’s machine.

Next Entry Point
- Start with `src/ui/widgets/video/video_player_page.cpp`, specifically `setupUi()`, `updateStageLayout()`, `updateStageState()`, and `eventFilter()`.
- If visual follow-up is needed, compare the live page directly against `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html` and adjust only the chrome-layer layout and styling.
