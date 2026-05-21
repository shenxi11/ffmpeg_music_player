Goal
- Restyle the top-bar menu button and the top-right window controls from the approved draft, and align the MainMenu popup behavior with explicit open/close interactions instead of timed auto-hide.

Completed
- Updated `src/app/main_widget.cpp` and `src/app/main_widget.h` so the top-right controls now use corrected semantics: minimize, maximize/restore, and close.
- Added menu-button open-state tracking in `src/app/main_widget.menu_auth.cpp` and wired `MainMenu` visibility back to the button's active style.
- Updated `src/app/main_menu.h` and `src/app/main_menu.cpp` so the popup emits visibility changes, no longer auto-hides on a timer, clamps to the available screen, and uses lighter popup/button styling.
- Updated `styles/netease.qss` for `MainMenuButton`, `WindowMinimizeButton`, `WindowMaximizeButton`, `WindowCloseButton`, and `QToolTip`.
- Recorded the cross-module decision for this shell/theme split in `.ai/decisions.md`.

Changed Scope
- `src/app/main_widget.h`
- `src/app/main_widget.cpp`
- `src/app/main_widget.menu_auth.cpp`
- `src/app/main_menu.h`
- `src/app/main_menu.cpp`
- `styles/netease.qss`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`

Open Work
- Run the app and verify the top bar visually: menu button hover/open state, minimize hover, maximize/restore glyph switching, close hover danger state, and popup positioning.
- If the menu popup still feels too heavy or too narrow after runtime review, tune only `MainMenu::createButtonStyle()`, `MainMenu::paintEvent()`, and the top-bar selectors in `styles/netease.qss`.

Risks
- Full Debug build is currently blocked at link time by an unrelated playback-module symbol error: `AudioCacheManager::onSeekIndexReplyFinished()` is declared in the active `playback-and-audio` work but not defined. The top-bar files compiled before that failure.
- `styles/netease.qss` already contains many unrelated dirty changes from other sessions; keep future edits narrowly scoped to the top-bar selectors unless you intentionally take ownership of broader theme cleanup.

Next Entry Point
- Start with `src/app/main_widget.menu_auth.cpp` to adjust menu toggle behavior.
- Use `src/app/main_menu.cpp` for popup visual/geometry tuning.
- Use `styles/netease.qss` for the final button polish once runtime screenshots confirm the target look.
