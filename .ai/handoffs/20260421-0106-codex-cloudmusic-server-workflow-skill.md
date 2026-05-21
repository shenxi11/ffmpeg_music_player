## Goal

Add a repo-local skill that forces future CloudMusic backend work to use the correct VM development directory, CloudMusic deployment directories, and default remote-server test target.

## Completed

- Added `.codex/skills/cloudmusic-server-workflow/SKILL.md`.
- Added `.codex/skills/cloudmusic-server-workflow/agents/openai.yaml`.
- Wrote the workflow rules into the skill:
  - develop and commit only in `shen@192.168.1.208:/home/shen/microservice_deploy`
  - deploy-only directories are `shen@192.168.1.208:/home/shen/CloudMusic` and `root@122.51.73.110:/root/CloudMusic`
  - default formal validation target is `122.51.73.110`
  - stop and report if the VM dev directory is unreachable or invalid
- Updated `.ai/ownership.yaml` and `.ai/tasks.yaml` to register and hand off the docs/governance task.

## Changed Scope

- `.codex/skills/cloudmusic-server-workflow/SKILL.md`
- `.codex/skills/cloudmusic-server-workflow/agents/openai.yaml`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

## Open Work

- Trigger the new skill in a future server-side task and verify that it is sufficient in practice.
- If desired later, add a second reference file under the skill for exact SSH/deploy commands, but keep the current workflow constraints as the source of truth.
- The repo still contains unrelated in-progress `.ai` changes from other sessions; do not normalize or revert them in this task.

## Risks

- The skill encodes project-specific IPs and paths. If the VM or remote deployment layout changes, the skill must be updated immediately.
- This task did not validate live SSH reachability to `192.168.1.208`; it only codifies the required workflow.
- There is still an unrelated `playback-and-audio` task left active in `.ai/*` from a previous interrupted session.

## Next Entry Point

- Start with `.codex/skills/cloudmusic-server-workflow/SKILL.md`.
- For future server-side requests, explicitly invoke `$cloudmusic-server-workflow`.
- If you need to adjust the skill metadata shown in UI, update `.codex/skills/cloudmusic-server-workflow/agents/openai.yaml`.
