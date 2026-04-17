# Cross-Session Decisions

Use this file only for decisions that affect more than one active Codex session.

## Entry Template

### YYYY-MM-DD - Decision Title
- Scope:
- Decision:
- Reason:
- Constraints:
- Follow-up:

## Current Decisions

### 2026-04-17 - Module Ownership Uses Directory Boundaries
- Scope: Whole repository governance
- Decision: Sessions claim modules by directory or subsystem, not by individual file or line.
- Reason: File-level locking is too brittle for a repo that mixes Qt Widgets, QML, viewmodels, agent runtimes, and build files.
- Constraints: Narrow the claim enough to avoid blocking unrelated work, but do not split a feature across QWidget/QML/viewmodel layers by default.
- Follow-up: Keep `.ai/ownership.yaml` as the only canonical module map.

### 2026-04-17 - Shared Surface Modules Require Explicit Cross-Module Coordination
- Scope: `shell-core`, `plugins-and-build`, `settings-and-theme`
- Decision: Changes to shared shell entrypoints, build graph files, or shared settings/theme surfaces require a decision record plus a dependency task before another module edits them.
- Reason: These files fan out into multiple modules and are the most likely collision points during parallel work.
- Constraints: Normal within-module edits are still allowed once the module is claimed; only cross-module interface changes need extra coordination.
- Follow-up: Treat `src/app/main_widget*`, `src/viewmodels/MainShellViewModel*`, `CMakeLists.txt`, `CMakePresets.json`, `qml.qrc`, `src/common/settings_manager*`, `qml/theme/Theme.js`, and `styles/netease.qss` as shared surfaces.

### 2026-04-17 - Agent Stack Is Split Into Host and Runtime Modules
- Scope: `agent-qt-host`, `agent-python-runtime`
- Decision: The Qt-side agent host and the Python-side agent runtime are separate modules, and any protocol, schema, tool registration, or control model change that crosses them must be recorded in `.ai/decisions.md`.
- Reason: The current repo already has substantial concurrent change volume on both sides, and silent coupling would create high-entropy breakage.
- Constraints: UI-only host changes or Python-only runtime changes can proceed independently once claimed.
- Follow-up: When a change touches both `src/agent/` and `agent/src/music_agent/`, create linked tasks instead of assuming one owner can change both without coordination.

### 2026-04-17 - Feature UI Ownership Stays Vertically Aligned
- Scope: `auth-and-user`, `library-and-search`, `playback-and-audio`, `lyrics`, `video`, `settings-and-theme`
- Decision: `src/ui/widgets/<feature>`, `src/ui/qml_bridge/<feature>`, `qml/components/<feature>`, and the feature's primary viewmodels belong to the same module unless a later decision explicitly splits them.
- Reason: This repo ships hybrid QWidget + QML surfaces, and splitting those layers between sessions creates avoidable conflicts in bindings and state flow.
- Constraints: Shared shell and theme files remain separate modules even when a feature depends on them.
- Follow-up: Keep the ownership map updated when a new feature directory is added under `src/ui` or `qml/components`.

### 2026-04-17 - Every Substantial Session Must Leave a Handoff
- Scope: Whole repository governance
- Decision: Every substantial session that edits or materially redefines shared state must leave a handoff file under `.ai/handoffs/`.
- Reason: The collaboration model is asynchronous and repo-backed; without handoffs, later sessions have to reconstruct intent from git diff and chat history.
- Constraints: The handoff must use the skill's fixed sections and point to concrete next files or tasks.
- Follow-up: New sessions should read the latest relevant handoff before resuming an in-flight module.

### 2026-04-17 - Settings Center Redesign Uses SettingsManager As The Unified Persistence Layer
- Scope: `settings-and-theme`
- Decision: The settings page redesign may extend `src/common/settings_manager*` to persist new settings-center tabs. First-version items that lack runtime behavior are still allowed if they persist cleanly and are clearly labeled in UI as saved-but-not-yet-applied.
- Reason: The new multi-tab settings center needs one canonical client-side store, and duplicating persistence across QML, lyrics widgets, or feature modules would create drift immediately.
- Constraints: This session stays within `settings-and-theme` only. It does not directly wire new runtime behavior into `shell-core`, `playback-and-audio`, or `lyrics` unless a later cross-module task/decision is added.
- Follow-up: Future sessions that make shortcuts, audio device, effect, or startup switches actually take effect must claim their own module and reference this decision before crossing module boundaries.

### 2026-04-17 - Online Music Comments Use Explicit Original music_path Context
- Scope: `shell-core`, `library-and-search`, `playback-and-audio`
- Decision: Client-side online music comments use an explicitly propagated original online `music_path` as the thread key. The playback area must not infer the comment thread from stream URL, song title, or artist metadata.
- Reason: The current online playback chain resolves stream URLs and can lose the original relative path or Jamendo virtual path required by the server comment protocol.
- Constraints: Local absolute paths and `file:///` items are non-commentable in v1. Only server relative paths and Jamendo virtual paths may enable the comments entry.
- Follow-up: Shared-surface wiring through `MainShellViewModel` and `main_widget` must preserve the original online path into the playback comments context before UI work proceeds.

### 2026-04-17 - Online Music Comments Add a Playback QML Drawer Resource
- Scope: `playback-and-audio`, `plugins-and-build`
- Decision: The v1 comments UI is implemented as a dedicated playback-side QML drawer and registered through `qml.qrc` instead of being inlined into unrelated playback QML files.
- Reason: The feature needs a distinct panel with list, reply, and composer state, and packaging it as a separate resource keeps playback controls maintainable.
- Constraints: `qml.qrc` is a shared build surface, so resource registration must stay minimal and limited to the comments drawer assets.
- Follow-up: Keep the new QML resource under `qml/components/playback/` and avoid unrelated qrc churn in the same task.

### 2026-04-17 - Online Music Comments Reuse One Surface Across Main Content And Drawer Hosts
- Scope: `shell-core`, `playback-and-audio`
- Decision: Online comments no longer exist only as a playback-side right drawer. The same comments/details surface is reused in two host modes: a temporary `MainWidget` content page when playback is collapsed, and a right drawer overlay when playback is expanded.
- Reason: The product interaction changed from “always drawer” to “main-content page when collapsed, drawer when expanded”, while the visual hierarchy must stay aligned with the approved HTML design instead of diverging between two UIs.
- Constraints: The temporary main-content comments page does not become a permanent left-nav tab. Drawer mode must stay above content surfaces but below the top bar. The comments/details content structure should remain aligned across both hosts even if two `QQuickWidget` containers are used.
- Follow-up: `shell-core` owns host switching and previous-content restore; `playback-and-audio` owns entry branching, shared comments state, and the unified comments/details QML surface.

### 2026-04-17 - Playback Playlist And Comment Drawer Share One Right Overlay Host
- Scope: `playback-and-audio`
- Decision: In expanded playback mode, the right-side overlay is now a shared host that switches between playlist and comments pages. The standalone playlist and comment drawers no longer compete as separate top-level overlays.
- Reason: The lyrics-only playback surface was previously elevated above the drawers, and independent right-side overlays created unstable z-order. A shared host makes the overlay layer deterministic and keeps playlist/comments mutually exclusive.
- Constraints: This change only affects the expanded-playback right overlay. Collapsed-playback comments still enter the main content area. The lyrics display must no longer rely on `Qt::WA_AlwaysStackOnTop`.
- Follow-up: `PlayWidget` owns the shared host, overlay page switching, and unified geometry/layering sync; any future right-side playback panel should attach to the same host instead of creating another sibling overlay.

### 2026-04-17 - Settings Entry Moves From Standalone Window To Main Content Panel
- Scope: `shell-core`, `settings-and-theme`
- Decision: The settings entry is no longer a standalone top-level `SettingsWidget` window. It is hosted inside `MainWidget` and switched through the existing main content panel flow, like other first-class content pages.
- Reason: The standalone window shape conflicts with the desired product behavior and makes the redesigned settings center render with misleading window semantics inside a page-oriented app shell.
- Constraints: The migration must reuse the existing `showContentPanel()` pattern instead of inventing a second navigation system. The settings page remains the owner of settings UI and persistence, while `shell-core` only hosts and switches it.
- Follow-up: Simplify settings-page footer actions to page semantics and keep shell changes limited to the minimum required entry, visibility, and geometry wiring.
