## Goal
Push the embedded video detail page much closer to `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html` by replacing the previous approximate text-button control bar with a stricter draft-faithful stage overlay and icon treatment.

## Completed
- Reworked `src/ui/widgets/video/video_player_page.cpp` and `src/ui/widgets/video/video_player_page.h`.
- Added a stage title block, right-edge hover arrow, and a rebuilt bottom hover overlay with icon controls.
- Replaced the previous text-based bottom controls with draft-style icon buttons and a compact quality pill.
- Moved lower action chips toward the draft by adding icons for favorite, download, share, comment, and open-local.
- Registered the required SVG assets in `qml.qrc` and added new icon files for video controls and action chips.
- Passed a Debug build and a short launch smoke test.

## Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `src/ui/widgets/video/video_player_page.h`
- `qml.qrc`
- `qml/assets/ai/icons/*`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Compare the live page against the HTML draft on the actual client screen and check for remaining spacing/typography drift.
- If the user still wants even stricter fidelity, the next pass should focus on pixel-level spacing, font weight, and the exact placement of the title block and subtitle text.
- The right-edge next arrow and some lower action chips are visual shells only; wire them later if product behavior is defined.

## Risks
- The page is visually much closer to the draft now, but exact font rendering and icon scale may still vary slightly in QWidget compared with the HTML mock.
- The quality button currently fronts the hidden preset combo and uses its text state; if product later wants a fixed “超清” label regardless of engine preset, that should be changed explicitly.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, especially `setupUi()`, `updateInfoPanel()`, `updatePlaybackButtons()`, and `updateStageLayout()`.
- Verify the result against `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html` and the running client.
