Goal
- Replace the temporary `设 / 诊 / 关` text placeholders in the left sidebar footer with proper icon-only buttons while keeping the existing actions and avoiding changes to the active `plugins-and-build` module.

Completed
- Updated [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp) so the three footer buttons are created as icon-only `QPushButton`s.
- Added inline vector icon generation in `MainWidget` for:
  - settings
  - plugin diagnostics
  - about
- Preserved the existing click routing and tooltip text for all three buttons.
- Kept the shared button visuals in [styles/netease.qss](/E:/FFmpeg_whisper/ffmpeg_music_player/styles/netease.qss) without introducing new qrc assets.
- Verified with:
  - `E:\cmake\bin\cmake.exe --build E:\FFmpeg_whisper\ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

Changed Scope
- Edited only:
  - `src/app/main_widget.cpp`
  - `styles/netease.qss`
  - `.ai/ownership.yaml`
  - `.ai/tasks.yaml`
- Intentionally did not edit `qml.qrc` or add repo-tracked SVG assets, because `plugins-and-build` is currently owned by another active session.

Open Work
- Run the client and visually verify:
  - icon optical balance inside the 34x34 footer buttons
  - hover-state readability against the red tinted background
  - spacing/alignment with the redesigned left sidebar
- If stricter visual parity is needed later, the next refinement would be replacing the inline generated icons with dedicated SVG assets after `plugins-and-build` becomes free.

Risks
- The icon graphics are generated in code, so future design tweaks require touching `MainWidget` rather than simply swapping an asset file.
- Hover-state icon recoloring currently relies on standard `QIcon` mode behavior; if a platform/theme renders it differently, a follow-up may need explicit event-driven icon swapping.

Next Entry Point
- Start in [src/app/main_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp), search for `SidebarFooterIconKind` and the three `m_sidebar*Button` initializations.
