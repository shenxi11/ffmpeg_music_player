## Goal
- Redesign the settings page inside `settings-and-theme` into a QQ Music style multi-tab settings center.
- Keep the existing settings entry and real download/network/cache behavior intact.
- Expand local persistence so new tabs can save state even when runtime behavior is not wired yet.

## Completed
- Rebuilt [qml/components/settings/Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) into a 7-tab settings center:
  `常规设置 / 下载与缓存 / 桌面歌词 / 快捷键 / 音效插件 / 网络设置 / 音频设备`
- Expanded [src/common/settings_manager.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/common/settings_manager.h) and [src/common/settings_manager.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/common/settings_manager.cpp) with persistence for new settings-center fields.
- Reworked [src/viewmodels/SettingsViewModel.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/SettingsViewModel.h) and [src/viewmodels/SettingsViewModel.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/SettingsViewModel.cpp) so QML binds directly to the ViewModel instead of using the old root-property sync model.
- Simplified [src/ui/widgets/settings/settings_widget.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/settings/settings_widget.cpp), [src/ui/widgets/settings/settings_widget.h](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/settings/settings_widget.h), and [src/ui/widgets/settings/settings_widget.connections.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/settings/settings_widget.connections.cpp) to expose `settingsViewModel` through `rootContext()`.
- Recorded the shared-surface decision in `.ai/decisions.md` and moved `observed-worktree-settings-and-theme` to `review`.

## Changed Scope
- `qml/components/settings/Settings.qml`
- `src/common/settings_manager*`
- `src/viewmodels/SettingsViewModel*`
- `src/ui/widgets/settings/*`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`

## Open Work
- Visually polish control styling if you want even closer checkbox/tab fidelity to the reference image.
- Decide which persist-only settings should become real runtime behavior first:
  shortcuts, proxy, audio device strategy, effect toggles, startup behavior.
- If those behaviors are implemented later, split them into cross-module tasks instead of extending this session silently into `playback-and-audio`, `lyrics`, or `shell-core`.

## Risks
- Full application build is currently blocked by an unrelated existing error in [src/network/httprequest_v2.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/network/httprequest_v2.cpp): missing symbol `rewriteServiceUrlToBase`.
- Because the full build stops in `library-and-search`, this session did not complete a full runtime click-through of the new settings page.
- The new tabs intentionally include several persist-only settings; the UI saves them, but many do not yet affect client runtime behavior.

## Next Entry Point
- Start from [qml/components/settings/Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) for UI refinements.
- Use [src/viewmodels/SettingsViewModel.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/SettingsViewModel.cpp) when wiring new controls to persistence or presence state.
- Use [src/common/settings_manager.cpp](E:/FFmpeg_whisper/ffmpeg_music_player/src/common/settings_manager.cpp) if you need to add or normalize more settings keys.
- Before full verification, resolve or coordinate the unrelated `httprequest_v2.cpp` build failure with the `library-and-search` owner.
