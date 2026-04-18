Goal
Address PR #19 governance review feedback by removing author-local absolute paths from the current PR's `.ai` files and cleaning up ownership/task entries for agent modules that no longer exist in the repository.

Completed
- Rewrote `.ai/handoffs/20260418-1041-codex-remove-ai-stack.md` to describe the removed AI archive and validation build in portable, machine-local terms instead of `E:\...` absolute paths.
- Rewrote `.ai/handoffs/20260418-1115-codex-all-current-work-one-pr.md` to describe the repo worktree, out-of-tree build directory, and external AI archive in portable terms.
- Added `.ai/archive/20260418-remove-ai-stack/README.md` as the repo-relative convention note for future discussions about the removed AI payload.
- Removed `agent-qt-host` and `agent-python-runtime` module entries from `.ai/ownership.yaml`.
- Removed the related bootstrap, observed-worktree, and completed AI-removal task entries from `.ai/tasks.yaml`, and normalized the surviving PR publish task note to avoid author-local build paths.

Changed Scope
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/handoffs/20260418-1041-codex-remove-ai-stack.md`
- `.ai/handoffs/20260418-1115-codex-all-current-work-one-pr.md`
- `.ai/archive/20260418-remove-ai-stack/README.md`

Open Work
- Push the governance cleanup commit to `codex/all-current-work-one-pr` so PR #19 includes these review fixes.
- If you want the GitHub review thread marked resolved or replied to, do that as a separate explicit GitHub write action.

Risks
- Older historical handoff files outside this PR still contain author-local Windows paths. This session intentionally left those untouched.
- `.ai/tasks.yaml` still contains a pre-existing malformed indentation block under `observed-worktree-settings-and-theme`; this cleanup did not normalize unrelated legacy YAML structure.

Next Entry Point
- Continue from branch `codex/all-current-work-one-pr` in the current repo worktree.
- Re-check PR #19 review threads after pushing to confirm the remaining open thread is now only awaiting manual reviewer resolution.
