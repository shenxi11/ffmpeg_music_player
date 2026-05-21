---
name: cloudmusic-server-workflow
description: Use when the task involves CloudMusic service backend development, backend bug fixes, Go microservice changes, VM development workflow, or deployment/testing for the CloudMusic server. This skill enforces that all service code changes and git commits happen only in shen@192.168.1.208:/home/shen/microservice-deploy, while the default deployment/testing directory is the VM checkout at /home/shen/CloudMusic.
---

# CloudMusic Server Workflow

Apply this skill whenever the user asks to modify, build, deploy, test, diagnose, or release the CloudMusic service backend.

For desktop client playback log diagnosis, do not treat this backend workflow skill as the primary log-analysis guide. Use the dedicated `desktop-playback-log-analysis` skill instead. If the user only says "分析日志" and does not name another file, the default desktop log source is `E:\FFmpeg_whisper\ffmpeg_music_player\打印日志.txt`.

## Authoritative Paths

Treat these paths as the only valid locations:

- Development and git commit directory:
  - `shen@192.168.1.208:/home/shen/microservice-deploy`
- VM formal deployment directory:
  - `shen@192.168.1.208:/home/shen/CloudMusic`
- Remote formal deployment directory, only when explicitly requested:
  - `root@122.51.73.110:/root/CloudMusic`

Do not treat any local Windows checkout, any other Linux copy, or any other `CloudMusic` / `microservice-deploy` path as equivalent.

## Non-Negotiable Rules

1. All backend feature work, bug fixes, script changes, and git commits must happen only in:
   - `shen@192.168.1.208:/home/shen/microservice-deploy`
2. `CloudMusic` directories are deployment-only:
   - VM: `/home/shen/CloudMusic`
   - Remote: `/root/CloudMusic`
3. Never develop in a `CloudMusic` directory and never commit from a `CloudMusic` directory.
4. Formal deployment means:
   - pull latest git in the target `CloudMusic` directory
   - build/deploy/run there
5. Default validation target for deployment/testing is the VM deployment directory:
   - `shen@192.168.1.208:/home/shen/CloudMusic`
6. Use the remote formal deployment directory only when the user explicitly asks for remote/public-server release or validation.

## Required Execution Order

For any server-side task, follow this order:

1. Connect to the VM `192.168.1.208`.
2. Enter `/home/shen/microservice-deploy`.
3. Run minimal truth checks before editing:
   - `pwd`
   - `git status --short`
   - `git branch --show-current`
   - `git log --oneline --decorate -10`
4. Inspect the repo-local build/deploy scripts before changing them.
5. Make code changes only in `/home/shen/microservice-deploy`.
6. Build/test there first.
7. Commit and push git from `/home/shen/microservice-deploy`.
8. For default deployment and testing, go to `/home/shen/CloudMusic` and pull/build/deploy.
9. For explicitly requested remote deployment, go to `/root/CloudMusic` and pull/build/deploy.

## Build and Deploy Policy

When deploying, prefer repository scripts already present in the target repo, such as:

- `scripts/`
- `build.sh`
- `start*.sh`
- `docker*.sh`

Do not invent an alternate deployment path if the repo already defines one.

Before any deployment in a `CloudMusic` directory:

- check `git status --short --branch`
- if the worktree is dirty, stop and report instead of overwriting it

## Testing Policy

Defaults:

- Development build verification: in `192.168.1.208:/home/shen/microservice-deploy`
- Default deployment verification: in `192.168.1.208:/home/shen/CloudMusic`
- Remote deployment verification, only when explicitly requested: in `122.51.73.110:/root/CloudMusic`

When reporting results, state explicitly:

- where code was changed
- where git commit happened
- which `CloudMusic` directory was deployed
- which machine was used for final test

## Stop Conditions

Stop and report instead of improvising if any of these are true:

- cannot connect to `192.168.1.208`
- `/home/shen/microservice-deploy` does not exist
- `/home/shen/microservice-deploy` is not the git working repo to be edited
- asked to develop directly inside a `CloudMusic` directory
- the target `CloudMusic` deployment worktree is dirty

If blocked by any stop condition, do not silently substitute:

- a local Windows checkout
- another Linux copy
- `/home/shen/CloudMusic`
- `/root/CloudMusic`

## Output Contract

For server-side tasks, explicitly report:

- current development directory
- whether edits were made in `/home/shen/microservice-deploy`
- whether git commit/push was done there
- which deployment directory was used
- whether final testing was done on the VM or explicitly requested remote server
