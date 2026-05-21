Goal
- Fix the desktop lyric hover regression where moving the mouse from the lyric body onto the top toolbar was treated as leaving the surface, causing the border and toolbar buttons to disappear.

Completed
- Reworked `DeskLyric.qml` hover ownership so the lyric body, top toolbar, settings popup, unlock chip, and resize corner all contribute to one aggregated hovered state.
- Added a short hover-exit delay to avoid flicker while the cursor moves between adjacent interactive zones.
- Moved popup-collapse logic behind the delayed full-surface hover teardown so the settings popup is not closed just by moving from the lyric body into toolbar controls.
- Reused `HoverHandler` for toolbar/popup/unlock tracking so hover sensing no longer depends on transparent `MouseArea` overlays stealing or missing pointer events.
- Verified `qmllint qml/components/lyrics/DeskLyric.qml`.

Changed Scope
- `qml/components/lyrics/DeskLyric.qml`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

Open Work
- Run a real pointer interaction smoke test in the app: lyric body -> toolbar buttons -> settings popup -> resize corner -> outside.
- If runtime still shows edge flicker on very fast diagonal movement, tune the hover-exit timer interval instead of adding more hover flags.

Risks
- The fix is QML-only and relies on `HoverHandler` plus `containsMouse`; runtime behavior should still be checked under the actual transparent top-level desktop lyric window.
- The short delayed teardown intentionally keeps the hover frame alive for a fraction of a second after leaving; if that feels sticky, only the timer interval should be adjusted.

Next Entry Point
- Start from the hover aggregation helpers near the top of `qml/components/lyrics/DeskLyric.qml`, then inspect `bodyHoverArea`, `topToolbarHover`, `settingsPopupHover`, `unlockChipHover`, and `resizeMouseArea`.
