Goal
Publish the current mixed client worktree as one PR to `main`, while excluding `打印日志.txt` and `third_party/llama.cpp`, and finish the repo-side AI removal so the branch can build cleanly.

Completed
- Created `codex/all-current-work-one-pr` from `origin/main` and cherry-picked the mixed worktree onto it.
- Resolved cherry-pick conflicts in `.ai/*`, settings, shell, and AI-removal surfaces in favor of the intended final client state.
- Removed the remaining in-repo AI runtime leftovers under `src/agent/AgentEmbeddedLlamaEngine*`, `src/agent/AgentLocalModelGateway*`, `src/agent/AgentLocalRuntime*`, and `src/app/client_automation_host_service*`.
- Fixed `MainShellViewModel::prefetchPlaylistCoverDetail()` to call `HttpRequestV2::getPlaylistDetailForCover()`.
- Removed the dangling `m_stopBtn` / `updateButtonStates()` branch from `src/video/VideoPlayerWindow.connections.cpp` so the current branch compiles.
- Validated a fresh VS2022 Debug configure/build in a fresh machine-local out-of-tree build directory and completed a 6-second smoke launch of the corresponding `Debug\\ffmpeg_music_player.exe`.
- Pushed branch `codex/all-current-work-one-pr` and opened PR #19: https://github.com/shenxi11/ffmpeg_music_player/pull/19

Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `src/viewmodels/MainShellViewModel.cpp`
- `src/video/VideoPlayerWindow.connections.cpp`
- `src/app/client_automation_host_service.cpp`
- `src/app/client_automation_host_service.h`
- `src/agent/AgentEmbeddedLlamaEngine.cpp`
- `src/agent/AgentEmbeddedLlamaEngine.h`
- `src/agent/AgentLocalModelGateway.cpp`
- `src/agent/AgentLocalModelGateway.h`
- `src/agent/AgentLocalRuntime.cpp`
- `src/agent/AgentLocalRuntime.h`

Open Work
- PR #19 still needs normal code review and merge.
- If the team wants the external AI stack reintroduced later, it must come back from the machine-local removal archive described by `.ai/archive/20260418-remove-ai-stack/` as a separately scoped effort rather than being mixed into this PR.

Risks
- `.ai/tasks.yaml` already contains an older malformed indentation block under `observed-worktree-settings-and-theme`; this session did not normalize that legacy YAML structure.
- The temporary PR body helper file used during `gh pr create` is not part of the repo and should remain untracked or be deleted locally after use.

Next Entry Point
- Review PR #19 first. If follow-up changes are needed, continue from branch `codex/all-current-work-one-pr` in the current repo worktree, then re-run the same configure/build steps against any equivalent local out-of-tree build directory.
