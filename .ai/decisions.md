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
