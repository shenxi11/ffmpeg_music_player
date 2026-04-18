## Goal
- Rework online music comments so the same comments/details UI can appear in two host modes:
- collapsed playback opens a temporary main-content comments page
- expanded playback opens a right drawer overlay
- keep the comment thread bound to the existing original online `music_path`
- align the comments page hierarchy and styling with `E:\FFmpeg_whisper\提示词\评论设计稿.html`

## Completed
- Added shell-side temporary comments page hosting in `src/app/main_widget*` through the existing `showContentPanel()` flow.
- Added playback-side dual-host routing in `src/ui/widgets/playback/play_widget*`:
- collapsed comment entry now requests a shell-hosted page
- expanded comment entry still opens the drawer overlay
- drawer geometry/layering is re-synced through a public overlay helper
- Reworked `qml/components/playback/CommentPanel.qml` into a shared comments/details surface with `displayMode` support.
- Extended `src/ui/qml_bridge/playback/comment_panel_qml.h` to sync `displayMode` and `threadMeta`.
- Wrote back cross-module governance updates in `.ai/decisions.md`, `.ai/tasks.yaml`, and `.ai/ownership.yaml`.

## Changed Scope
- `shell-core`: `src/app/main_widget.h`, `src/app/main_widget.cpp`, `src/app/main_widget.playback_connections.cpp`, `src/app/main_widget.playback_list_handlers.cpp`, `src/app/main_widget.playback_state_handlers.cpp`
- `playback-and-audio`: `src/ui/widgets/playback/play_widget.h`, `src/ui/widgets/playback/play_widget.cpp`, `src/ui/widgets/playback/play_widget.controls_connections.cpp`, `src/ui/qml_bridge/playback/comment_panel_qml.h`, `qml/components/playback/CommentPanel.qml`

## Open Work
- Runtime verification is still needed for both host modes after the dirty settings worktree issue is resolved.
- Main build is currently blocked before linking by an unrelated missing resource referenced from `qml.qrc`: `qml/components/settings/Settings.qml`.
- Once the settings-side resource issue is cleared, retest:
- collapsed comment entry -> main content page
- expanded comment entry -> drawer overlay
- drawer z-order against playlist/history overlays
- comment/details tab rendering and write actions

## Risks
- The shared comments surface now exists in two `QQuickWidget` hosts. State is synchronized from `PlayWidget`, but tab selection is still per-host instance rather than globally mirrored.
- Because the main project build is blocked by unrelated settings resource churn, C++ integration has not been end-to-end rebuilt in the full target after this round.

## Next Entry Point
- First unblock the unrelated settings resource problem reported by `rcc`:
- `qml.qrc`
- expected missing file: `qml/components/settings/Settings.qml`
- Then resume from:
- `src/app/main_widget.playback_state_handlers.cpp`
- `src/app/main_widget.playback_list_handlers.cpp`
- `src/ui/widgets/playback/play_widget.cpp`
- `qml/components/playback/CommentPanel.qml`
