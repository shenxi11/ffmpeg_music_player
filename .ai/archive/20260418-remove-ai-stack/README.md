# 2026-04-18 AI Stack Removal Archive Convention

This directory is a repo-relative coordination note for the AI stack that was removed from the client repository on 2026-04-18.

The removed payload itself is not stored in this repository. The original archive was created in a machine-local location on the author workstation.

## Intended archive layout

If the removed content needs to be shared or recreated later, preserve the original relative structure under this archive convention:

- `plugins/ai_assistant_plugin/`
- `src/agent/`
- `qml/components/agent/`
- `agent/`
- selected `说明文档/` AI/Agent documents
- selected `rag/cases/` AI-specific case notes

## Retrieval and reuse

- Treat this directory as the canonical repo-relative reference point when another session needs to discuss or restore the removed AI stack.
- If the original archive payload is needed, retrieve it from the archive owner or recreate an equivalent local archive outside the repository.
- Any future reintroduction of the AI stack should happen in a separately scoped branch or PR, not by mixing the archive payload back into unrelated client work.
