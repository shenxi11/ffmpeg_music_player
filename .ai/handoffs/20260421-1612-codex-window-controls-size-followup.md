## Goal
- Enlarge only the three top-right window control buttons from 32x32 to 36x36 without reopening the rest of the top-bar redesign.

## Completed
- Updated `MainWidget` to size the minimize, maximize/restore, and close buttons at `36x36`.
- Updated shared `netease.qss` so those three buttons use `36px` min/max dimensions, `18px` radius, and slightly larger glyph sizes.
- Kept the menu button unchanged.
- Ran a Debug build successfully in `E:/FFmpeg_whisper/ffmpeg_music_player_build`; only the existing `VCINSTALLDIR is not set` deployment warning remained.

## Changed Scope
- `src/app/main_widget.cpp`
- `styles/netease.qss`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Runtime visual verification is still recommended to confirm the larger buttons feel balanced against the menu button and title-bar spacing.

## Risks
- This is a style-only follow-up in shared top-bar QSS, so any later global title-bar restyle could override the exact `36x36` sizing if it touches the same selectors.

## Next Entry Point
- Start from `src/app/main_widget.cpp` for widget sizing and `styles/netease.qss` for the shared top-bar button geometry.
