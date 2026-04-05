# Claude Code 方法论对当前音乐 Agent 的架构借鉴清单

## 1. 文档目标

这份文档不是 Claude Code 仓库的读书笔记，而是把 `E:\claude-code\docs` 和相关插件资料中已经确认可借鉴的方法论，系统性映射到当前音乐 Agent 项目。

文档要解决的问题是：

- Claude Code 的哪些组织方式值得借鉴
- 这些模式在当前音乐 Agent 里已经有什么基础
- 哪些地方还缺关键结构
- 下一阶段服务端应该优先往哪里重构

适用对象：

- 当前音乐 Agent 服务端开发者
- 后续接手重构的其他 AI / 工程师
- 需要理解“为什么后端不应该继续把所有逻辑都堆进 runtime”的联调同学

---

## 2. Claude Code 里最值得借鉴的不是“源码实现”，而是“工程组织方式”

基于以下资料：

- [project-overview.md](e:/claude-code/docs/kb/project-overview.md)
- [component-model.md](e:/claude-code/docs/kb/component-model.md)
- [patterns.md](e:/claude-code/docs/kb/patterns.md)
- [consumption-guide.md](e:/claude-code/docs/kb/consumption-guide.md)
- [feature-dev/README.md](e:/claude-code/plugins/feature-dev/README.md)
- [feature-dev/commands/feature-dev.md](e:/claude-code/plugins/feature-dev/commands/feature-dev.md)
- [code-architect.md](e:/claude-code/plugins/feature-dev/agents/code-architect.md)
- [plugin-dev/README.md](e:/claude-code/plugins/plugin-dev/README.md)
- [Agent Development SKILL.md](e:/claude-code/plugins/plugin-dev/skills/agent-development/SKILL.md)
- [rag/README.md](e:/claude-code/docs/rag/README.md)

可以明确得出结论：

Claude Code 公开资料最值得借鉴的不是 CLI 核心实现，而是这四类方法论：

1. **把 prompt、流程、角色、策略做成资产**
2. **把复杂任务做成分阶段工作流**
3. **把大而全的 agent 拆成职责清晰的专职组件**
4. **把知识消费做成 router -> summary -> evidence 的分层结构**

这四点和当前音乐 Agent 的演进方向高度一致。

---

## 3. 方法论映射清单

### 3.1 `commands` -> 典型任务链模板 / 用户目标工作流

Claude Code 中的原始模式：

- `commands/*.md` 用来定义用户主动发起的一类工作流
- 命令本质上不是单个动作，而是一个多阶段 runbook
- 典型特征是先理解、再探索、再澄清、再设计、再执行

当前音乐 Agent 里已有的对应基础：

- 典型任务链已经隐含存在于 runtime 中
- 如：
  - 查歌单 -> 看歌单内容
  - 搜歌 -> 播放
  - 创建歌单 -> 加歌
  - dry-run -> validate -> execute

当前缺口：

- 任务链还大多散落在 `music_runtime.py` 和 `autonomous_planner.py`
- 这些链路没有被提升成“稳定的任务模板资产”
- 典型目标缺少独立文档和统一命名

推荐落地：

- 为高频任务链建立独立“任务模板层”
- 先不一定新增代码模块，但要先把模板从 runtime 中抽象出来
- 第一批建议资产化的模板：
  - 搜索并播放单曲
  - 查询歌单并查看内容
  - 查询最近播放
  - 创建歌单并从来源歌单搬运前 N 首
  - 停止播放 / 播放控制

一句话：

当前 runtime 里那些“隐式工作流”，后面应逐步演化成和 Claude Code `commands` 类似的任务模板资产。

---

### 3.2 `agents` -> 可拆分的专职决策组件

Claude Code 中的原始模式：

- `agents/*.md` 定义专职角色
- 如 explorer、architect、reviewer
- 每个 agent 有明确职责、触发条件、输入输出预期

当前音乐 Agent 里已有的对应基础：

- 后端已经开始具备“角色拆分雏形”
- 当前主要组件包括：
  - [semantic_parser.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/semantic_parser.py)
  - [semantic_refiner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/semantic_refiner.py)
  - [capability_reasoner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/capability_reasoner.py)
  - [autonomous_planner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/autonomous_planner.py)
  - [client_script_planner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/client_script_planner.py)

当前缺口：

- 这些模块还更像“工具函数集合”，不是清晰的职责角色
- 它们之间的输入输出契约还不够显式
- `music_runtime.py` 仍承担了过多领域判断和分支控制

推荐落地：

- 不需要照搬 Claude Code 的多 agent 并发形式
- 但要继续把当前后端拆成“专职决策组件”
- 推荐未来稳定角色是：
  - `Goal Understanding`
  - `Object Resolution`
  - `Capability Routing`
  - `Capability Reasoner`
  - `Tool Planner`
  - `Script Planner`
  - `Policy Evaluator`
  - `Observation Integrator`

一句话：

Claude Code 的 `agents` 给我们的启发不是“马上上多代理”，而是“不要让一个 runtime 同时承担所有脑力工作”。

---

### 3.3 `skills` -> 可复用的策略、输出风格、语义增强模块

Claude Code 中的原始模式：

- `skills` 通过 progressive disclosure 组织方法论
- skill 不是一次性 prompt，而是一个可复用策略包
- 典型用途：
  - 输出风格
  - agent 创建方法
  - 插件开发规范

当前音乐 Agent 里已有的对应基础：

- 当前已有“回复风格 skill”的思路
- 也已有：
  - 语义精炼
  - 能力策略
  - 风险分级
  - dry-run / approval

当前缺口：

- 这些策略目前更多是散落在 runtime 分支中的代码逻辑
- 缺少清晰的“策略模块化”边界

推荐落地：

- 后续把这类策略逐步抽成可插拔模块，而不是继续往 runtime 堆规则
- 第一批建议 skill 化 / 策略化的内容：
  - 语义增强策略
  - 输出风格策略
  - 脚本风险策略
  - 自动执行资格策略
  - 工具 fallback 策略

一句话：

技能化的本质，是把“行为规则”从主流程中解耦出来。

---

### 3.4 `hooks` -> 行为注入层、风控层、输出风格层

Claude Code 中的原始模式：

- hook 用来在生命周期节点注入额外行为
- 常见用途：
  - 安全守卫
  - 输出风格
  - 会话开始时的上下文注入

当前音乐 Agent 里已有的对应基础：

- 当前项目已经有很多“伪 hook”型逻辑，只是还没被明确抽象出来
- 例如：
  - 执行前 dry-run
  - 风险高则不执行
  - 复杂脚本阻断 fallback
  - 输出风格控制
  - 停止播放优先级守卫

当前缺口：

- 这些逻辑仍是主流程里的条件分支
- 没有统一的“策略注入层”

推荐落地：

- 未来逐步形成这些 hook 化节点：
  - `BeforeSemanticFinalize`
  - `BeforeCapabilitySelect`
  - `BeforeScriptExecute`
  - `AfterToolResult`
  - `BeforeAssistantReply`

第一批最值得抽出的 hook：

- 播放类负面守卫
- 复合任务 fallback 阻断
- 脚本高风险阻断
- 输出风格增强

一句话：

Hook 化的意义不是为了“更复杂”，而是为了让运行时行为不再依赖一堆散乱分支。

---

### 3.5 `kb summaries` -> 能力目录、架构说明、协议文档

Claude Code 中的原始模式：

- 先读 KB summary，再回溯原始证据
- summary 层的作用不是替代源码，而是降低消费成本

当前音乐 Agent 里已有的对应基础：

- 你当前已经拥有一批很强的文档资产：
  - [Qt端最终联调协议稿.md](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/Qt端最终联调协议稿.md)
  - [AI控制Qt音乐软件总体方案.md](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/AI控制Qt音乐软件总体方案.md)
  - [Qt Host API初版清单与JSON协议草案.md](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/Qt%20Host%20API%E5%88%9D%E7%89%88%E6%B8%85%E5%8D%95%E4%B8%8EJSON%E5%8D%8F%E8%AE%AE%E8%8D%89%E6%A1%88.md)
  - [README.md](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/README.md)
  - [联调日志与新版服务端预期对照表.md](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/联调日志与新版服务端预期对照表.md)
  - 客户端说明文档/里的能力全景、脚本化方案等资料

当前缺口：

- 这些资产还没有形成“router / summary / evidence”的有层次知识系统
- planner 和语义层还没有稳定消费这些摘要资产

推荐落地：

- 把当前文档视为 KB summary 的起点
- 后续补一层机器可消费摘要：
  - capability summary
  - task chain summary
  - protocol summary
  - script DSL summary

一句话：

你现在已经有文档资产了，下一步重点不是“再写更多文档”，而是“让这些文档变成后端的稳定知识层”。

---

### 3.6 `rag layering` -> 能力路由、摘要上下文、证据回溯层

Claude Code 中的原始模式：

- `router -> summary -> evidence`
- operational question 不能只靠 summary，必要时要回证据层

当前音乐 Agent 里已有的对应基础：

- 已有：
  - [capability_catalog.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/capability_catalog.py)
  - [world_state.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/world_state.py)
  - 审计流
  - 服务端事件存储

当前缺口：

- 还没有显式的 `router -> summary -> evidence` 层次
- 语义层和 planner 还拿不到“精确裁剪后的必要知识”

推荐落地：

- `router`：能力分类与任务链路由
- `summary`：能力摘要、典型任务模板、对象模型摘要
- `evidence`：真实 Qt 协议、工具定义、审计事件、客户端说明文档

一句话：

后续如果真做知识检索，先做分层消费，不要直接上重型 RAG。

---

## 4. 当前项目的建议重构方向

当前后端不应继续朝“大 runtime + 更多 if/else”发展，而应逐步收束成下面这套结构：

### 4.1 Goal Understanding

职责：

- 从用户话术中提取目标、对象、约束、动作类型
- 输出结构化目标，而不是直接决定工具

当前基础：

- [semantic_parser.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/semantic_parser.py)
- [semantic_models.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/semantic_models.py)

### 4.2 Object Resolution

职责：

- 绑定 `playlist/track/current playback/recent tracks`
- 处理“这个歌单”“刚才那个”“第一首”

当前基础：

- [semantic_refiner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/semantic_refiner.py)
- [workflow_memory.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/workflow_memory.py)
- [world_state.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/world_state.py)

### 4.3 Capability Routing

职责：

- 决定当前目标应该落到：
  - 普通聊天
  - 单步工具
  - 多步工具
  - 脚本
  - 计划 / 审批

当前基础：

- [capability_catalog.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/capability_catalog.py)
- [capability_reasoner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/capability_reasoner.py)

### 4.4 Script Planner / Tool Planner

职责：

- 多步执行链的模板化生成与选择

当前基础：

- [autonomous_planner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/autonomous_planner.py)
- [client_script_planner.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/client_script_planner.py)

### 4.5 Policy Hooks

职责：

- 执行前风险判断
- fallback 阻断
- 自动执行资格判断
- 输出风格注入

当前基础：

- 目前分散在：
  - [music_runtime.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/music_runtime.py)
  - `dry_run/approval/direct write` 相关逻辑

### 4.6 Execution Runtime

职责：

- transport
- 状态推进
- 消息分发
- script / tool 生命周期编排

当前基础：

- [server.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/server.py)
- [music_runtime.py](/e:/FFmpeg_whisper/ffmpeg_music_player/agent/src/music_agent/music_runtime.py)

### 4.7 Observation / Audit

职责：

- tool/script 结果整合
- 执行观察
- 失败原因
- 事件回放

当前基础：

- 审计流
- session events
- script/tool result integration

### 4.8 Response Style Layer

职责：

- 回复风格
- 代码输出风格
- 解释型 / 教学型输出

当前基础：

- 已有“回复 skill”思路
- 当前部分逻辑仍混在 prompt 与 runtime 里

---

## 5. 可执行的分阶段演进路线

### 阶段一：资产化阶段

目标：

- 把现有能力、协议、任务链、策略文档沉淀成稳定知识资产

建议动作：

- 固化任务模板目录
- 固化能力摘要层
- 固化策略摘要层
- 让更多 planner / reasoner 读取摘要，而不是内嵌常识

完成标志：

- 运行时不再完全依赖散落在代码里的隐式规则

---

### 阶段二：组件拆分阶段

目标：

- 把 `music_runtime.py` 中的语义、规划、fallback、策略判断继续分层

建议动作：

- 明确 `goal/object/router/planner/policy/observation` 分工
- 收紧 `music_runtime.py`，让它更像编排壳层

完成标志：

- runtime 负责 orchestration，不再是领域逻辑大杂烩

---

### 阶段三：Hook 化阶段

目标：

- 把行为增强从主流程中抽出来

建议动作：

- 先做逻辑 hook，不一定做成对外插件机制
- 优先抽：
  - 自动执行资格判断
  - fallback 阻断
  - 输出风格增强
  - 执行前审批判断

完成标志：

- 行为规则不再散落在主流程分支中

---

### 阶段四：知识检索阶段

目标：

- 让语义层和 planner 只消费当前必要知识

建议动作：

- 建立 `router -> summary -> evidence`
- 引导语义解析与 planner 只读必要摘要
- operational question 需要时再回证据层

完成标志：

- 不再把全量能力说明直接塞给模型

---

## 6. 当前项目关键模块的下一步角色

### `semantic_refiner.py`

当前角色：

- 规则补丁层

未来角色：

- 可插拔语义策略层

建议：

- 后续不要继续只堆特殊句式
- 要逐步拆成：
  - 中文启发式
  - 引用解析
  - 角色化实体抽取
  - 负面守卫策略

### `capability_catalog.py`

当前角色：

- 工具元数据表 + façade-aware 能力摘要

未来角色：

- 稳定的能力知识目录

建议：

- 继续增加：
  - 能力模板关系
  - 风险分类
  - 典型调用链
  - 证据来源链接

### `capability_reasoner.py`

当前角色：

- 基础能力资格判断

未来角色：

- 能力路由与执行资格中心

建议：

- 后续让更多“为什么能做 / 为什么不能做”的判断集中到这里

### `autonomous_planner.py`

当前角色：

- 多步 action candidate 构造器

未来角色：

- 多步任务模板与执行链选择中心

建议：

- 不要只做 action candidate 拼装
- 要逐步吸收：
  - 任务模板
  - 典型链选择
  - fallback / replan 入口

### `client_script_planner.py`

当前角色：

- 客户端脚本 DSL 适配器

未来角色：

- 脚本模板与脚本安全边界中心

建议：

- 继续按 DSL 能力逐步扩模板
- 保持“安全阻断优先于错误退化”

### `workflow_memory.py`

当前角色：

- 会话级工作记忆结构

未来角色：

- 世界状态与工作记忆中心

建议：

- 继续收束字段语义
- 少加零散字段，多加结构化状态槽位

### `server.py`

当前角色：

- transport + runtime 派发 + REST/WS 协议层

未来角色：

- transport 和 orchestration 壳层

建议：

- 后续尽量不要再把新的领域分支堆进这里

---

## 7. 哪些东西不应该照搬

这一节非常重要。

### 7.1 不把公开示例仓库当成 Claude Code CLI 内核实现

这个仓库更像：

- prompt 资产仓库
- 插件范例仓库
- 方法论仓库

不是：

- Claude Code CLI 核心源码全量泄露版

### 7.2 不把插件/命令/skill 文档误当成完整 runtime 代码

这些资料可以借鉴工作流、结构、资产组织方式，但不能被当成内部实现证据。

### 7.3 不在当前项目里盲目上多 agent 并发

当前项目最需要的是职责拆分，而不是先引入复杂的并行子代理体系。

优先级应是：

- 先拆职责
- 再拆组件
- 最后再考虑是否需要并发子代理

### 7.4 不在没有稳定资产层之前过早做复杂 RAG

如果没有：

- 能力摘要
- 任务模板摘要
- 证据层组织

那直接上复杂 RAG 很容易变成“把噪音检索给模型”。

### 7.5 不把“文档很多”误当成“系统就会更聪明”

重点不是文档数量，而是：

- 是否能被路由
- 是否能被摘要消费
- 是否能被回证据层

---

## 8. 对当前音乐 Agent 的最终建议

如果把 Claude Code 方法论压成一句最适合你当前项目的话，那就是：

**不要再把音乐 Agent 继续做成“越来越复杂的 runtime”，而是把它逐步重构成“任务模板 + 决策组件 + 策略层 + 知识层 + 审计层”协作的系统。**

当前最值得优先推进的不是：

- 再加更多意图 if/else
- 再加更多临时 prompt 补丁
- 再加更多零散 fallback

而是：

1. 把高频任务链资产化
2. 把 runtime 继续拆分
3. 把策略逻辑 hook 化
4. 把能力文档和审计证据纳入分层知识消费

这才是从“会调用工具的聊天后端”走向“可维护的 Agent 系统”的正确路线。

