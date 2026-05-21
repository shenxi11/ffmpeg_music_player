## Goal
Replace the embedded video page fullscreen path that reparents the OpenGL video surface across top-level windows and causes fullscreen black-screen playback.

## Completed
- Reworked `src/ui/widgets/video/video_player_page.*` so the embedded `VideoRendererGL` always stays under the main page stage host.
- Added a dedicated fullscreen stage host plus fullscreen `VideoRendererGL` owned by the fullscreen window from creation time.
- Replaced the old renderer transition/rebuild path with a direct renderer switch via `VideoSession::setVideoRenderer(...)`.
- Changed fullscreen host behavior so only the control bar is reparented between the page and the fullscreen window.
- Verified the client still builds with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `src/ui/widgets/video/video_player_page.h`
- `src/ui/widgets/video/video_player_page.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Manually verify runtime fullscreen playback on Windows:
  - enter fullscreen while playing
  - exit fullscreen back to embedded mode
  - pause in fullscreen and resume
  - seek in fullscreen
  - repeat multiple fullscreen toggles
- Check whether fullscreen still needs mirrored stage overlays beyond the bottom control bar.
- Watch shutdown logs for any remaining `QQuickWidget` / OpenGL context warnings after fullscreen use.

## Risks
- This refactor keeps the fullscreen overlay minimal: the fullscreen window currently guarantees video picture plus bottom control bar, but not a second copy of all embedded stage chrome.
- Runtime behavior still needs confirmation because the bug was previously visible only during real fullscreen interaction, not at compile time.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on `applyHostWindowState()`, `switchActiveRenderer()`, and `updateStageLayout()`.
- Reproduce by opening a video from the video list, clicking the fullscreen button, and checking whether picture, bottom control bar width, and exit behavior all stay correct.
