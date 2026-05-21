Goal
- Implement the approved shell redesign from `E:\FFmpeg_whisper\提示词\布局设计稿.txt` by switching the client to a left-sidebar plus right-content layout, while keeping the existing red-white product theme and not touching `src/audio/*`.

Completed
- Rebuilt `MainWidget` into a two-column shell in [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp) and [src/app/main_widget.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.h):
  - added full-height left sidebar geometry and separate right-side topbar/content calculations
  - moved `UserWidgetQml` into the sidebar header
  - added real back/forward content-page history buttons and history stacks
  - added sidebar section labels plus footer tool buttons for settings / diagnostics / about
- Updated the expand/collapse shell behavior in [src/app/main_widget.playback_state_handlers.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.playback_state_handlers.cpp) so the sidebar and topbar hide together during full-player takeover and restore together afterward.
- Added shell-driven collapsed playback docking in [src/ui/widgets/playback/play_widget.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.h) and [src/ui/widgets/playback/play_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp):
  - new `setCollapsedDockRect(const QRect&)`
  - collapsed mask and process-slider geometry now follow the right-column dock instead of spanning the full window
  - right overlay geometry now respects the new shell bounds
- Extended the QML bridge in [src/ui/qml_bridge/playback/process_slider_qml.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/playback/process_slider_qml.h) with `setSongArtist(...)`.
- Updated [src/ui/widgets/playback/play_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp) to push artist metadata into the QML playback bar.
- Restyled [qml/components/playback/ProcessSlider.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/ProcessSlider.qml) into the new three-zone structure:
  - left metadata block with cover, title, artist
  - centered playback controls
  - right utility block
  - retained existing playback/comment/playlist signal semantics
- Added shell styling support in [styles/netease.qss](/E:/FFmpeg_whisper/ffmpeg_music_player/styles/netease.qss) for the new sidebar, topbar history buttons, and footer tool buttons.
- Validation completed:
  - `qmllint qml/components/playback/ProcessSlider.qml`
  - `E:\cmake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

Changed Scope
- Edited only shell, playback UI bridge, playback QML, and shared shell styling files:
  - `src/app/main_widget.h`
  - `src/app/main_widget.cpp`
  - `src/app/main_widget.playback_state_handlers.cpp`
  - `src/ui/widgets/playback/play_widget.h`
  - `src/ui/widgets/playback/play_widget.cpp`
  - `src/ui/qml_bridge/playback/process_slider_qml.h`
  - `qml/components/playback/ProcessSlider.qml`
  - `styles/netease.qss`
- Did not modify `src/audio/*` even though `playback-and-audio` was temporarily claimed for the UI-only docking work.

Open Work
- Run runtime visual verification for:
  - sidebar spacing and section balance
  - back/forward history behavior across recommend / local / history / settings / comment content pages
  - collapsed playback bar spacing on narrow and wide window widths
  - right-side comment / playlist overlay alignment against the new shell
- If the user wants closer pixel parity to the draft, the next likely refinement points are:
  - iconography for sidebar/footer actions
  - tighter spacing and typography in `ProcessSlider.qml`
  - playlist section card treatment inside the sidebar

Risks
- The new history model tracks main content widgets only. It intentionally does not track right-side overlays, playlist drawers, or playback state. If future work adds more content surfaces, they may need explicit inclusion/exclusion in `isTrackableContentWidget(...)`.
- `ProcessSlider.qml` kept the existing signal contract; visual parity improved, but exact draft-level icon/action density may still need runtime tuning.
- `mainWidget` geometry is still manual rather than layout-managed, so unusual window aspect ratios should be tested before more shell changes stack on top.

Next Entry Point
- Start with [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp) for shell geometry and page history.
- Then inspect [qml/components/playback/ProcessSlider.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/playback/ProcessSlider.qml) and [src/ui/widgets/playback/play_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp) for collapsed dock layout and overlay behavior.
