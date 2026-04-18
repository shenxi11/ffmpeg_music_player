Goal
- Rebuild the embedded settings page so it follows the HTML design draft at `E:\FFmpeg_whisper\提示词\设置页面设计稿.txt` instead of the previous card-heavy settings-center layout.

Completed
- Rewrote [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) into a QQ Music style settings page with title, horizontal tabs, one scrollable content panel, and row-based sections that match the HTML draft.
- Expanded [settings_manager.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/common/settings_manager.h) and [settings_manager.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/common/settings_manager.cpp) to persist the extra design-only settings the draft needs, including playback preference radios, download classification, lyric layout/color values, proxy form fields, plugin directory, and audio device form values.
- Expanded [SettingsViewModel.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/SettingsViewModel.h) and [SettingsViewModel.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/SettingsViewModel.cpp) so the QML page can bind every editable item through the viewmodel instead of carrying temporary local-only state.
- Added reset actions for desktop lyrics, shortcuts, and audio device groups through the viewmodel/settings manager.
- Rebuilt successfully with `cmake --build --preset vs2022-x64-debug --target ffmpeg_music_player`.

Changed Scope
- `settings-and-theme`
- Shared collaboration state in [.ai/tasks.yaml](E:/FFmpeg_whisper/ffmpeg_music_player/.ai/tasks.yaml) and [.ai/ownership.yaml](E:/FFmpeg_whisper/ffmpeg_music_player/.ai/ownership.yaml)

Open Work
- Do a manual in-app visual pass to verify the new QML page matches the HTML draft closely under the real runtime theme and font rendering, especially button styling and combobox appearance.
- If visual drift remains, continue refining only [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) unless a real runtime behavior needs another module.
- Decide whether the now-hidden legacy settings like server host/port and log path need a dedicated advanced subsection later, because they are not exposed in the HTML draft version.

Risks
- The repo worktree is globally dirty with unrelated online-comments changes. Do not assume every modified file around this session belongs to the settings redesign.
- The new settings page relies on many newly added persisted keys. If another session refactors `SettingsManager` without reading this handoff, it can silently drop design-only fields from the UI.
- The build passed, but QML visual parity against the HTML draft was not screenshot-verified in a live app session in this turn.

Next Entry Point
- Start from [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) for visual adjustments.
- Use [settings_manager.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/common/settings_manager.h) and [SettingsViewModel.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/SettingsViewModel.h) if any additional design-draft control still needs persistence.
- Treat `observed-worktree-settings-and-theme` as `review` and the module ownership as `handoff`.
