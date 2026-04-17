---
name: multi-ai-module-collab
description: Coordinate parallel Codex sessions that develop different modules in the same repository. Use when multiple Codex conversations work on this project at the same time and need a shared task board, module ownership, handoff records, and rules for avoiding cross-session conflicts before editing code.
---

# Multi-AI Module Collaboration

## Overview

Use this skill when a Codex session joins an already-active project and must coordinate with other Codex sessions instead of acting as the only editor.

Treat the repository files under `.ai/` as the shared source of truth for ownership, active tasks, cross-module decisions, and handoffs.

## Start Workflow

Before editing code:

1. Read `.ai/ownership.yaml`, `.ai/tasks.yaml`, and `.ai/decisions.md`.
2. Run a minimal repo check:
   - `git status --short`
   - `git branch --show-current`
   - `git log --oneline --decorate -10`
3. Identify the module you intend to change by directory or subsystem, not by vague feature wording.
4. Confirm that the module is unclaimed or already assigned to your session.

Use the module entries already defined in `.ai/ownership.yaml` as canonical. Do not invent a new module name unless the current ownership map clearly cannot represent the work.

If the target module is claimed by another active session:

- Stop before editing.
- Limit yourself to read-only analysis, or
- record a dependency / follow-up task instead of modifying that module directly.

## Ownership Rules

Use `.ai/ownership.yaml` to manage module-level ownership.

Follow these rules:

- Claim directories or subsystems, not individual lines.
- Keep ownership narrow enough to avoid blocking unrelated work.
- Prefer one owner per module at a time.
- Release ownership when the session is done or blocked.
- If you must touch another module for a shared interface change, record that decision in `.ai/decisions.md` first and update `.ai/tasks.yaml` to show the cross-module dependency.

Recommended statuses:

- `claimed`
- `active`
- `blocked`
- `handoff`
- `released`

## Task Board Rules

Use `.ai/tasks.yaml` for all active work.

Each task must include:

- a stable `task_id`
- a concise `title`
- one `module`
- an `owner_session`
- a `status`
- expected write scope in `files_expected`
- explicit `acceptance`

Use these statuses:

- `todo`
- `doing`
- `blocked`
- `review`
- `done`

Before you start editing:

- move or create the task as `doing`
- set `owner_session`
- update `updated_at`

When you stop:

- set the next truthful status
- write the blocker or next step in `notes`

Treat placeholder tasks whose `task_id` starts with `observed-worktree-` as observations, not ownership. They only become editable work after a real session sets `owner_session` and moves the task to `doing`.

## Decision Rules

Use `.ai/decisions.md` only for decisions that affect more than one session.

Record a decision when you change:

- shared interfaces
- response shapes
- config precedence
- directory ownership boundaries
- common utility behavior
- schema or persistence expectations

Do not use `decisions.md` as a changelog. Keep it to decisions another session must know before continuing.

## Handoff Rules

Before ending the session, create one file under `.ai/handoffs/`:

- filename: `YYYYMMDD-HHMM-session-label.md`

Include exactly these sections:

- `Goal`
- `Completed`
- `Changed Scope`
- `Open Work`
- `Risks`
- `Next Entry Point`

Write concrete paths and next actions. Another Codex session should be able to continue from the handoff without rereading the whole chat.

## Conflict Handling

If you detect unexpected repo changes while working:

1. Check whether the changed files overlap with your claimed module.
2. If they do not overlap, continue.
3. If they overlap and were made by another session, stop editing that area.
4. Update `.ai/tasks.yaml` and `.ai/handoffs/` to describe the collision and the safe next step.

Never silently overwrite work in a module currently owned by another active session.

## Practical Defaults

Use these defaults unless the user says otherwise:

- prefer module ownership by directory
- prefer one active session per module
- require a handoff file for every substantial session
- require a decision record before changing shared interfaces
- treat `.ai/*` as repo-tracked files, not local scratch notes

## Minimal Output Contract

When this skill is active, your first project-facing update should state:

- the module you intend to own
- whether ownership is free or occupied
- the task you are advancing

Your final project-facing update should state:

- what task status you wrote back
- whether ownership remains claimed or was released
- which handoff file you created
