# Goal

把仓库里的多 AI 协作骨架从空模板升级为长期可复用的模块地图、任务板和固定协作决策。

# Completed

- 将 `.ai/ownership.yaml` 初始化为全仓库长期模块注册表，覆盖 shell、认证、搜索/库、播放、歌词、视频、设置、Agent、构建、文档治理等模块。
- 将 `.ai/tasks.yaml` 初始化为两类长期任务：`module-bootstrap-*` 和 `observed-worktree-*`。
- 在 `.ai/decisions.md` 写入 5 条仓库级固定协作决策。
- 更新 `.codex/skills/multi-ai-module-collab/SKILL.md`，补充“模块注册表是 canonical”和“observed-worktree 不是默认已认领任务”的规则。
- 更新 `README.md` 的多 AI 协作说明，明确 `.ai/ownership.yaml` 是全仓库模块地图入口。

# Changed Scope

- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`
- `.ai/handoffs/20260417-1346-codex-collab-bootstrap.md`
- `.codex/skills/multi-ai-module-collab/SKILL.md`
- `README.md`

# Open Work

- 把真实活跃会话逐步写入 `.ai/tasks.yaml` 的 `owner_session` 和 `status`，不再停留在观察型 placeholder。
- 后续若 `shell-core` 或 `library-and-search` 长期过大，再考虑二次拆分模块。
- 如果团队希望进一步减少人工维护，可以在后续单独规划脚本校验 `.ai/*` 的字段完整性和 owner 冲突。

# Risks

- 当前 `observed-worktree-*` 任务只反映“检测到有未提交改动”，不代表能自动识别真实负责人。
- 模块边界已经足够稳定用于长期协作，但仍可能随着 Agent 栈或 UI 结构继续扩张而需要二次调整。
- 现有工作区很脏，后续新会话如果不先读 `.ai/*`，依然可能绕开这套机制直接改共享入口文件。

# Next Entry Point

- 先读 `.ai/ownership.yaml`，选择模块。
- 再读 `.ai/tasks.yaml`，把对应 `observed-worktree-*` 或业务任务改为 `doing` 并填写 `owner_session`。
- 若要改 `shell-core`、`plugins-and-build`、`settings-and-theme` 的共享面，先补 `.ai/decisions.md` 再动代码。
