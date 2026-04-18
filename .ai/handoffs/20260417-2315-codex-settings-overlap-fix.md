Goal
- Fix the two confirmed overlap issues in the HTML-draft settings page: the sync/acceleration row in `常规设置` and the first combobox row in `音频设备`.

Completed
- Updated [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) so the `最近播放列表同步云端 / 使用播放加速服务（重启后生效）` area no longer uses one rigid horizontal row. It now reflows responsively and gives the subtitle its own vertical space.
- Updated [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml) so the first audio-device combobox uses a custom content layout with fixed arrow space and text elision, preventing the device text from colliding with the control chrome.
- Rebuilt successfully with `cmake --build --preset vs2022-x64-debug --target ffmpeg_music_player`.

Changed Scope
- `settings-and-theme`
- Shared coordination state in [.ai/tasks.yaml](E:/FFmpeg_whisper/ffmpeg_music_player/.ai/tasks.yaml) and [.ai/ownership.yaml](E:/FFmpeg_whisper/ffmpeg_music_player/.ai/ownership.yaml)

Open Work
- Run the desktop client and confirm the two originally reported overlaps are gone at the real runtime window size.
- If any remaining audio-device combobox rows still feel cramped, apply the same custom combobox content styling to the later rows in the same section for consistency.
- If the sync/acceleration row still feels too dense on narrower widths, reduce the two-column threshold in [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml).

Risks
- This fix was validated by build only, not by a screenshot or live runtime check in this turn.
- The repository is still globally dirty with unrelated settings and online-comments work; do not assume all modified files around this session belong to the overlap fix alone.

Next Entry Point
- Start from [Settings.qml](E:/FFmpeg_whisper/ffmpeg_music_player/qml/components/settings/Settings.qml).
- Treat `observed-worktree-settings-and-theme` as `review` and module ownership as `handoff`.
- Compare the runtime result specifically against the user-reported two overlap locations before changing any broader layout.
