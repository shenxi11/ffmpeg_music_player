## Goal
Remove the in-repo AI stack from the Qt client, move all AI-specific code and docs out of the repository, and verify the client still configures and builds without the AI plugin dependency chain.

## Completed
- Removed `plugins/ai_assistant_plugin` from the top-level CMake build graph.
- Removed AI-only shell hooks from `MainWidget` and `MainShellViewModel`, including the hidden AI top button, agent-only `Q_INVOKABLE` host methods, and plugin-host service registration that only existed for the AI stack.
- Removed the in-repo Qt AI host stack from `src/agent/` and `qml/components/agent/`.
- Moved the repo-root Python AI runtime directory `agent/` out of the repo.
- Moved Agent-facing docs and AI-specific RAG case files out of the repo.
- Updated non-AI docs/RAG indexing so they no longer point at removed AI paths.
- Verified a clean configure and Debug build in a machine-local out-of-tree build directory for the AI-removed state.
- Verified a 6-second startup smoke test with the corresponding Debug executable from that build directory.

## Changed Scope
- The original removal archive was created in a machine-local location on the author workstation.
- For shared follow-up, use `.ai/archive/20260418-remove-ai-stack/` as the repo-relative archive convention and coordination note.
- The removed AI-related repo content kept its original relative structure under:
  - `plugins\ai_assistant_plugin\`
  - `src\agent\`
  - `qml\components\agent\`
  - `agent\`
  - selected `说明文档\` AI/Agent documents
  - selected `rag\cases\` AI-specific case notes

## Open Work
- If you want this change submitted, the next step is to review the dirty worktree carefully because it still contains unrelated playback/comments/settings changes from earlier work.
- If you want the legacy out-of-tree build directory usable again, clear its stale cache/lock state separately; it failed reconfigure for environment reasons, not because of the AI removal patch.
- If the removed AI payload needs to be reintroduced or audited later, retrieve it from the archive owner or recreate an equivalent local archive that follows `.ai/archive/20260418-remove-ai-stack/`.

## Risks
- Some repository documents still mention “Agent” in a historical or roadmap sense, especially under `rag/` and planning docs; these are no longer tied to an in-repo AI plugin/runtime, but they are not fully purged from all narrative text.
- The current working tree remains mixed with unrelated unsubmitted product changes, so any future PR must be scoped carefully.

## Next Entry Point
- Main repo: repository root on the current branch
- Shared archive note: `.ai/archive/20260418-remove-ai-stack/`
- Validation build: any local out-of-tree build directory for the AI-removed client state
