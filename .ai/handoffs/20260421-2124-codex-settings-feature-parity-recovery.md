## Goal
Restore the redesigned settings page to old feature parity where runtime support still exists, and make every unsupported control look disabled instead of interactive.

## Completed
- Added a settings support matrix in `src/viewmodels/SettingsViewModel.*` with `isSettingFeatureAvailable()`, `isSettingsActionAvailable()`, and `isSettingsTabAvailable()`.
- Reworked `qml/components/settings/Settings.qml` so unsupported tabs and controls render grey and non-interactive.
- Restored the old online-status block and refresh action inside the settings page.
- Restored footer actions for `关闭设置` and `返回欢迎页`.
- Changed the embedded settings close path so `settingsClosed()` no longer calls widget-close semantics; it now routes back through `MainWidget`.
- `qmllint qml/components/settings/Settings.qml` passed.
- `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player` passed.

## Changed Scope
- `qml/components/settings/Settings.qml`
- `src/viewmodels/SettingsViewModel.h`
- `src/viewmodels/SettingsViewModel.cpp`
- `src/ui/widgets/settings/settings_widget.h`
- `src/ui/widgets/settings/settings_widget.cpp`
- `src/ui/widgets/settings/settings_widget.connections.cpp`
- `src/app/main_widget.h`
- `src/app/main_widget.menu_auth.cpp`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`

## Open Work
- Run the client and manually verify the new footer close action from multiple entry pages:
  - recommend -> settings -> close
  - local -> settings -> close
  - play history -> settings -> close
- Manually verify `返回欢迎页` still clears playback state and returns to the welcome page correctly.
- Visually confirm the unsupported tabs and disabled sections look acceptable under the current theme.

## Risks
- The support matrix is intentionally conservative: some settings are disabled because only persistence exists and no runtime consumer was found. If another module later wires one of those settings into real behavior, the matrix must be updated.
- `MainWidget` now tracks one previous-content pointer for settings. If a future shell flow opens settings from a more complex transient host, that return target may need refinement.

## Next Entry Point
- Start from `qml/components/settings/Settings.qml` to review any remaining visual polish issues.
- If navigation behavior is wrong, continue in `src/app/main_widget.menu_auth.cpp` with `handleMainMenuSettingsRequested()` and `handleSettingsCloseRequested()`.
- If a disabled setting needs to become active later, update the support matrix in `src/viewmodels/SettingsViewModel.cpp`.
