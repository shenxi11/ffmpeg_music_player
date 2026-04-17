## Goal
Split the mixed local worktree into two reviewable PRs: one for product code and one for repo-backed multi-AI governance files, then publish both without dragging in logs or `third_party/`.

## Completed
- Created dedicated worktrees and branches for the product PR and governance PR.
- Isolated the product file set onto `codex/ui-comments-playback-settings-batch`.
- Isolated `.ai` ownership/task/decision files plus prior handoffs onto `codex/multi-ai-collab-files`.
- Re-ran a Debug configure/build for the product branch in `E:\FFmpeg_whisper\ffmpeg_music_player_build_pr_feature`.
- Verified a short startup smoke test survived for 6 seconds.

## Changed Scope
- Updated `.ai/ownership.yaml` to record this PR-splitting session under `docs-and-governance`.
- Added `workflow-split-submit-feature-and-governance-prs` to `.ai/tasks.yaml`.
- Added this handoff file for the publish/split session.

## Open Work
- Commit the product branch and governance branch with clear messages.
- Push both branches to `origin`.
- Open two draft PRs to `main`.
- If GitHub reports merge conflicts against the current `main`, resolve them on the dedicated branch rather than in the original mixed worktree.

## Risks
- The product branch is rooted at the shared product baseline `73d7edf` so the PR diff stays clean, but `main` has moved on; merge conflicts are still possible in shared shell/settings files.
- The original working tree remains dirty and still contains `third_party/llama.cpp`, `打印日志.txt`, and the local protocol doc; those files were intentionally not included in either PR branch.

## Next Entry Point
- Product branch: `E:\FFmpeg_whisper\ffmpeg_music_player_pr_feature`
- Governance branch: `E:\FFmpeg_whisper\ffmpeg_music_player_pr_governance`
- Start by running `git status --short --branch` in each worktree, then commit/push and open draft PRs.
