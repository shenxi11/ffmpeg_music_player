## Goal
Make the embedded video detail page follow `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html` more closely by fixing the wrong layout hierarchy: the control bar was effectively lost, and the lower detail section visually pressed over the video stage.

## Completed
- Reworked `src/ui/widgets/video/video_player_page.cpp` and `src/ui/widgets/video/video_player_page.h`.
- Removed the extra top page header from `VideoPlayerPage`; the page now renders as one white outer card.
- Split the content card into an upper 16:9 video stage and a lower info panel, matching the draft's stacked structure.
- Changed the stage control bar from a separate rounded dark panel into a full-width bottom gradient overlay inside the video stage.
- Added stage hover tracking so the overlay can appear on hover while staying visible when playback is paused.
- Kept the existing `MediaSession + VideoRendererGL` playback path and main-window maximize wiring intact.
- Built Debug successfully and ran a short launch smoke test.

## Changed Scope
- `src/ui/widgets/video/video_player_page.cpp`
- `src/ui/widgets/video/video_player_page.h`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work
- Run a manual in-app visual check against the HTML draft and compare spacing, typography, and control density on the real page.
- If the hover behavior still feels too subtle in practice, consider forcing the stage-bottom controls to remain visible while a video is loaded.
- Decide whether the currently retained `打开本地视频` control belongs inside the stage bar or should move elsewhere for closer draft fidelity.

## Risks
- The current hover behavior is implemented at the QWidget level; if the OpenGL renderer or nested children consume hover transitions differently on some machines, the overlay may still need a follow-up visibility tweak.
- The page no longer creates or shows a back button in `VideoPlayerPage`; that matches the current product direction, but any future “return to list” entry must be added back deliberately.
- This pass focused only on the embedded page itself. The surrounding `VideoListWidget`/host flow was left unchanged.

## Next Entry Point
- Start from `src/ui/widgets/video/video_player_page.cpp`, especially `setupUi()`, `updateStageState()`, and `updateStageLayout()`.
- Compare the running client with `E:\FFmpeg_whisper\提示词\视频窗口设计稿.html`.
- If another visual fix is needed, keep the work inside the `video` module and update `.ai/tasks.yaml` before editing.
