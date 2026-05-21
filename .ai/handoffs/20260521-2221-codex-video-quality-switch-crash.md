Goal
Fix the crash reported after clicking the video quality switch in VideoPlayerPage, where pause/resume after HLS quality reload could hit `QMutex pointer is misaligned`.

Completed
Centralized VideoPlayerPage session teardown through `releaseMediaSession()`, which detaches the current VideoSession renderer and clears both embedded/fullscreen renderers before deleting MediaSession.
Removed the quality button's manual `onQualityPresetChanged(nextIndex)` call so `QComboBox::currentIndexChanged` is the only trigger.
Detached inactive renderers during fullscreen/embedded renderer switching so only the active renderer keeps the VideoBuffer.
Added generation guarding for delayed renderer recovery callbacks.
Stopped the VideoRendererGL render timer when `setVideoBuffer(nullptr)` clears the buffer.
Validated with `cmake --build --preset vs2022-x64-debug`.

Changed Scope
`src/ui/widgets/video/video_player_page.cpp`
`src/ui/widgets/video/video_player_page.h`
`src/video/VideoRendererGL.cpp`
`.ai/ownership.yaml`
`.ai/tasks.yaml`

Open Work
Manual runtime validation should replay the user's scenario: play HLS video, click quality switch, wait for reload, pause/resume, then close the video window.

Risks
The HLS seek itself can still fail on bad server segment boundaries; this fix prevents renderer stale-buffer crashes around that failure path but does not redesign the server playlist or segment map.

Next Entry Point
Start from `VideoPlayerPage::onQualityPresetChanged()`, `VideoPlayerPage::loadSourceAt()`, and `VideoPlayerPage::releaseMediaSession()` if follow-up runtime logs still show HLS quality switch recovery issues.
