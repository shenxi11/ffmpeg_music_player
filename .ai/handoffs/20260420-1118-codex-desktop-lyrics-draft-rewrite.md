Goal
- Rewrite the desktop lyric floating surface from `E:\FFmpeg_whisper\提示词\桌面歌词设计稿.txt`, including the visible UI and the desktop-lyric data flow.

Completed
- Rebuilt [qml/components/lyrics/DeskLyric.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/lyrics/DeskLyric.qml) into a two-line floating lyric overlay with hover toolbar, lock/unlock state, inline settings popup, color picker, and resize handle.
- Reworked [src/ui/qml_bridge/lyrics/desklrc_qml.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/lyrics/desklrc_qml.h) so the desktop lyric host now stores current/next lyric text separately, applies desktop-lyrics style from `SettingsManager`, and updates the QML root through properties instead of the old single-line method flow.
- Updated [src/ui/widgets/playback/play_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp), [src/ui/widgets/playback/play_widget.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.h), and [src/ui/widgets/playback/play_widget.core_connections.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.core_connections.cpp) so PlayWidget now derives current-line plus next-line desktop lyric content and no longer relies on the old `signalCurrentLrc -> setLyricText` chain.
- Updated `.ai/ownership.yaml`, `.ai/tasks.yaml`, and `.ai/decisions.md` for the cross-module lyrics/playback change.
- Validation completed:
  - `qmllint qml/components/lyrics/DeskLyric.qml`
  - `E:/cmake/bin/cmake.exe --build E:/FFmpeg_whisper/ffmpeg_music_player_build --config Debug --target ffmpeg_music_player`

Changed Scope
- `qml/components/lyrics/*`
- `src/ui/qml_bridge/lyrics/*`
- `src/ui/widgets/playback/play_widget*`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`

Open Work
- Launch the client and verify that the new floating lyric overlay matches the draft closely enough in actual desktop rendering.
- Check drag behavior, resize behavior, and inline settings popup hit testing on the real window.
- Check whether the current text-based toolbar glyphs need to be replaced with dedicated SVG icons for better visual parity.
- Decide whether the old [DeskLyricSettings.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/lyrics/DeskLyricSettings.qml) should remain as a fallback shell or be retired later.

Risks
- The new drag behavior moved from the QWidget host to a QML drag surface so the inline popup can be clicked; this needs runtime verification on Windows because hit testing can differ from static checks.
- Toolbar icons are currently text glyphs rather than a full SVG icon set, so visual fidelity is structurally improved but not yet icon-perfect.
- The session did not change `SettingsManager` persistence keys; if future UX wants more desktop-lyric options than the existing keys cover, that would require a separate settings-and-theme change.

Next Entry Point
- Start from [qml/components/lyrics/DeskLyric.qml](/E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/lyrics/DeskLyric.qml) for visual polish and interaction tweaks.
- If playback-linked lyric content looks wrong, inspect [src/ui/widgets/playback/play_widget.cpp](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/widgets/playback/play_widget.cpp) around `resolveDesktopLyricLines()` and `syncDesktopLyricLines()`.
- If style values are not refreshing from the settings center, inspect [src/ui/qml_bridge/lyrics/desklrc_qml.h](/E:/FFmpeg_whisper/ffmpeg_music_player/src/ui/qml_bridge/lyrics/desklrc_qml.h) and its `onSettingsCenterChanged()` path.
