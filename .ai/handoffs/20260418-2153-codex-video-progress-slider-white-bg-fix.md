## Goal
- Remove the remaining white background from the integrated video progress slider.
- Keep the compact bottom control bar layout unchanged.
- Avoid touching playback logic or fullscreen behavior.

## Completed
- Added explicit `QSlider#VideoProgressSlider` root styling in the page stylesheet.
- Added explicit `::add-page:horizontal` and disabled-state styling so unplayed track regions no longer fall back to the native white background.
- Applied the same styling directly to `m_progressSlider` to avoid Windows native style fallback even if parent-level stylesheet matching is bypassed.
- Built `Debug` successfully with `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`.

## Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `src/ui/widgets/video/video_player_page.cpp`

## Open Work
- Run the client and confirm the video progress slider no longer shows any white background in embedded mode or video-only fullscreen.
- If the white region is gone but the track contrast still feels too strong, do a follow-up visual pass on `groove/add-page` alpha only; layout should stay untouched.

## Risks
- This handoff confirms compile success, not final runtime visual verification.
- The slider now has both page-level and per-instance styles on purpose; if future refactors centralize video styling, they must keep `add-page` and root background coverage or the white fallback may return.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, focusing on the `VideoProgressSlider` stylesheet block and the `m_progressSlider` setup path in `setupUi()`.
