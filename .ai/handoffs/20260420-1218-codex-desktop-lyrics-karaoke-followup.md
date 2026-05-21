Goal
- Add the same current-line karaoke-style reveal used by the main lyric page to the desktop lyric floating window, without introducing per-word timestamps or new lyric formats.

Completed
- Extended `DeskLrcQml` to store and forward `currentLineProgress` into the desktop lyric QML root.
- Updated `PlayWidget` to keep desktop lyrics in sync with both the current/next line payload and the current-line progress computed from the existing playback lyric timeline.
- Extended `desktopLyricSnapshot()` to include `currentLineProgress` for downstream state sync.
- Reworked the current desktop lyric line in `DeskLyric.qml` into a base text plus clipped highlight overlay so the first line now reveals from left to right like the main lyric page.
- Verified `qmllint qml/components/lyrics/DeskLyric.qml` and a Debug build of `ffmpeg_music_player`.

Changed Scope
- `qml/components/lyrics/DeskLyric.qml`
- `src/ui/qml_bridge/lyrics/desklrc_qml.h`
- `src/ui/widgets/playback/play_widget.cpp`
- `.ai/ownership.yaml`

Open Work
- Run a real playback check for desktop lyrics across pause/resume, seek, and line transitions to confirm the reveal pacing matches the main lyric page visually.
- Decide later whether desktop lyrics should also support true per-character timing; this pass deliberately does not add that capability.

Risks
- The reveal effect depends on `Text.paintedWidth` and the current alignment math; unusual fonts or extreme DPI scaling may still need pixel tuning.
- If future changes alter `resolveCurrentLyricProgress()` semantics for the main lyric page, desktop lyric reveal will change with it because both now share the same progress source.

Next Entry Point
- Start from `PlayWidget::refreshCurrentLyricHighlight()` and `PlayWidget::syncDesktopLyricLines()` in `src/ui/widgets/playback/play_widget.cpp`, then compare against the current-line overlay block in `qml/components/lyrics/DeskLyric.qml`.
