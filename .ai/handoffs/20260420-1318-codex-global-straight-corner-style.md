Goal
- Replace the client’s rounded-corner visual language with a straight-corner baseline across active QML, QSS, QWidget, and custom-painted runtime surfaces.

Completed
- Set shared playback/lyrics style radii in `qml/theme/PlayerStyle.js` to zero so expanded playback panels and control bars inherit straight edges by default.
- Flattened active QML rectangle radii across main view, auth, library/search, playback, lyrics, settings, user, and video components to zero.
- Replaced `border-radius` values in `styles/netease.qss` and active QWidget style sheets with `0px`, including login, welcome, search, playback, and video widgets.
- Converted custom-painted rounded rectangles and round clipping paths in active runtime widgets to rectangular paths, including user cards, old desktop lyric painting, rotating cover art, and turntable base/needle drawing.
- Verified `qmllint` on representative main-path QML files and a Debug build of `ffmpeg_music_player`.

Changed Scope
- `qml/MainView.qml`
- `qml/components/**`
- `qml/theme/PlayerStyle.js`
- `styles/netease.qss`
- `src/ui/widgets/auth/*`
- `src/ui/widgets/playback/*`
- `src/ui/widgets/search/*`
- `src/ui/widgets/user/*`
- `src/ui/widgets/video/*`
- `src/lyrics/*`
- `src/app/main_menu.cpp`

Open Work
- Do a runtime visual sweep to catch any remaining rounded corners or circular affordances on rarely used pages.
- If the new square playback discs feel too harsh, the next pass should adjust padding, borders, and shadows rather than reintroducing corner radius.

Risks
- This pass intentionally prioritized broad flattening over bespoke per-screen re-spacing, so some controls may need pixel tuning after visual review.
- Existing unrelated worktree changes remain in the repo; this handoff only covers the straight-corner restyle subset.

Next Entry Point
- Start from `qml/theme/PlayerStyle.js` and `styles/netease.qss` for the shared baseline, then inspect `qml/components/playback/ProcessSlider.qml`, `qml/components/library/RecommendMusicWidget.qml`, and the custom-painted playback widgets under `src/ui/widgets/playback/`.
