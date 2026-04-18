Goal
- Move the settings entry from a standalone top-level window into the main application content area and make the redesigned settings UI stable for in-page display.

Completed
- Updated [src/app/main_widget.menu_auth.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.menu_auth.cpp) so the main menu settings action creates `SettingsWidget(this)` and routes it through `showContentPanel()` instead of `show()/raise()/activateWindow()`.
- Updated [src/app/main_widget.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.cpp) so `settingsWidget` participates in content visibility switching and receives `computeContentRect()` geometry during adaptive layout updates.
- Updated [src/app/main_widget.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.h) comment semantics from settings window to settings content page.
- Updated [src/ui/widgets/settings/settings_widget.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/settings/settings_widget.cpp) so `SettingsWidget` behaves as an embedded `Qt::Widget` host when parented, while keeping a top-level fallback only for standalone creation.
- Updated [qml/components/settings/Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) to remove the old window-style footer, keep the page-level return action in the header, and compact the oversized player-style preview into a tighter selector suitable for the main content page.
- Rebuilt with `cmake --build --preset vs2022-x64-debug --target ffmpeg_music_player`; build succeeded.

Changed Scope
- `shell-core`
- `settings-and-theme`
- Shared coordination state in `.ai/tasks.yaml`, `.ai/ownership.yaml`, and `.ai/decisions.md`

Open Work
- Run a manual visual pass in the desktop client to confirm the embedded settings page no longer overlaps or feels cramped at the main window default size.
- Decide whether the header-level `返回欢迎页` action is still desirable now that settings behaves like a first-class main content page.
- If further visual cleanup is needed, continue in [qml/components/settings/Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) without reopening standalone-window behavior.

Risks
- The repository worktree is still globally dirty with unrelated online-comments changes; do not assume every modified file belongs to this settings migration.
- The embedded settings page relies on `showContentPanel()` and `computeContentRect()` behavior in shell-core. Future shell layout changes can regress settings sizing if they forget to keep `settingsWidget` in the same content flow.
- The QML signal `settingsClosed()` is still wired in [src/ui/widgets/settings/settings_widget.connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/settings/settings_widget.connections.cpp), but the current page UI no longer emits it. That is harmless now, but it is legacy wiring to keep in mind if someone reintroduces close semantics.

Next Entry Point
- Start from [src/app/main_widget.menu_auth.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/app/main_widget.menu_auth.cpp) and [qml/components/settings/Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml).
- Treat task `feature-settings-main-content-host-migration` and `observed-worktree-settings-and-theme` as `review`.
- The latest coordination decision for this work is in [.ai/decisions.md](E:/FFmpeg_whisper/ffmpeg_music_player/.ai/decisions.md) under `Settings Entry Moves From Standalone Window To Main Content Panel`.
