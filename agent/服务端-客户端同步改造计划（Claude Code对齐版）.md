# 服务端-客户端同步改造计划（Claude Code对齐版）

## 1. 文档目标

这份文档用于指导音乐 Agent 的下一阶段同步重构，目标不是继续做局部修补，而是让服务端与客户端同时朝更接近 Claude Code 方法论的结构演进。

当前系统已经暴露出一个很明确的问题：

- 服务端开始具备一定的复合语义理解能力
- 但任务一进入执行阶段，就过早绑定到了“脚本现在能不能安全生成”
- 一旦脚本 planner 没有选中，就直接阻断任务，或历史上曾错误退化成普通工具调用

因此，下一阶段的核心目标不是继续堆更多 heuristic，而是同步建立：

1. 服务端的 `Goal Understanding -> Object Resolution -> Command Template -> Execution Substrate Routing`
2. 客户端的 `AgentCapabilityFacade -> Script DSL -> Tool Executor` 统一能力入口
3. 双端共享的任务模板与执行边界认知
4. 更清晰的调试审计与联调证据链

## 2. 当前问题判断

### 2.1 服务端当前主要问题

当前服务端的问题，已经不再只是“模型把中文理解错了”，而是更偏架构层：

1. 语义层虽然开始角色化拆分，但还没有形成独立的“命令模板层”
2. `client_script_planner` 现在既承担脚本构造，又事实承担“整轮任务能否继续”的裁判角色
3. 一旦来源对象没有在状态里唯一解析，复合任务就会被脚本安全门提前拦截
4. 这不符合 Claude Code 风格的分层流程

当前典型问题链如下：

`user_message -> semantic -> script_planner -> selected? -> no -> 直接阻断`

这会让系统在“目标已经理解，但对象尚未完成解析”时，过早宣告失败。

### 2.2 客户端当前主要问题

客户端当前不是协议错误，也不是执行器失效。当前短板主要在于：

1. 已经有 `AgentCapabilityFacade`，但服务端还没有把它真正当成统一能力入口来建模
2. 脚本 DSL 仍然只支持顺序步骤，不支持条件、过滤、循环、回滚
3. 客户端调试日志目前能够反映“有没有脚本”，但还不够充分反映“如果没有脚本，那这一轮实际走了什么 fallback 路径”
4. CapabilityFacade 虽然已经落点，但高频业务链路仍主要沉在 `AgentToolExecutor`

### 2.3 双端当前最核心的耦合问题

服务端当前把“任务模板选择”和“执行载体选择”耦合得太早，客户端又还没把脚本与 façade 的边界沉淀成稳定资产，导致：

- 服务端不敢继续推进
- 客户端也无法从日志快速判断失败到底在哪一层

## 3. Claude Code 对齐后的目标结构

本轮同步改造建议对齐为如下结构：

1. `Goal Understanding`
2. `Object Resolution`
3. `Command Template`
4. `Capability Routing`
5. `Execution Substrate Selection`
6. `Execution`
7. `Observation / Audit`
8. `Response Style`

其中：

- 服务端重点负责前 1 到 5 层
- 客户端重点负责 5 到 7 层的统一能力入口与执行反馈

### 3.1 服务端的角色

服务端要从“会发工具/脚本的运行时”升级成“会理解目标、解析对象、选择模板、再选择执行载体的编排器”。

### 3.2 客户端的角色

客户端要从“工具执行宿主 + 脚本执行宿主”继续收束成“统一能力入口的执行宿主”，让：

- tool_call
- script steps
- 后续 GUI 自动化或其他入口

都逐步汇聚到 `AgentCapabilityFacade` 背后。

## 4. 总体改造原则

### 4.1 服务端原则

1. 不再把 `script planner` 当成整轮任务成败的总裁判
2. 先选择 `command template`，再决定用 `script / tool_chain / clarify / approval`
3. 复合任务如果对象尚未唯一解析，优先自动补对象解析，而不是直接失败
4. 继续把运行时大文件往“协调层”压缩
5. 让语义、模板、路由、风控、响应层职责进一步分离

### 4.2 客户端原则

1. 不改已有协议形态的前提下，继续把能力收束到 façade
2. 用文档和调试日志明确“哪些模板适合脚本，哪些更适合 tool_call”
3. 对脚本执行保持顺序步骤边界，不在当前阶段假装已经支持条件/过滤/事务
4. 调试日志要能说明：
   - 这轮有没有脚本
   - 没有脚本时实际走了什么链
   - 这轮是否被服务端阻断

### 4.3 双端共同原则

1. 不让“脚本不可立即生成”直接等于“任务失败”
2. 先完成模板层，再继续扩脚本能力
3. 先把审计链和证据链补清，再谈更复杂的自治执行

## 5. 第一阶段：建立命令模板层

这是当前最优先的同步改造阶段。

### 5.1 服务端改造项

建议新增：

- `src/music_agent/command_templates.py`
- 或 `src/music_agent/goal_workflows.py`

其职责不是直接发工具，也不是直接发脚本，而是定义稳定任务模板。

首批模板建议只落这 3 条：

1. `inspect_playlist_tracks`
2. `get_recent_tracks`
3. `create_playlist_from_playlist_subset`

每个模板至少包含：

- `goal`
- `requiredObjects`
- `optionalObjects`
- `autoResolutionSteps`
- `preferredSubstrates`
- `fallbackStrategy`
- `approvalPolicy`

### 5.2 服务端对现有模块的影响

- `semantic_goal_router.py`
  - 负责输出目标，不再直接隐含后续执行方式
- `semantic_object_resolver.py`
  - 负责产出模板输入对象
- `client_script_planner.py`
  - 降级为脚本构造器，不再承担整轮任务裁判
- `semantic_runtime.py`
  - 只负责把目标和对象交给模板层，不再直接决定阻断或执行

### 5.3 客户端改造项

客户端新增一份明确文档：

- `说明文档/客户端能力模板映射（供Agent路由）.md`

文档至少要覆盖：

- 现有高频任务模板
- 对应更适合的执行载体
- 当前 DSL 能表达的范围
- 当前仍然必须由服务端先做对象解析的范围

比如：

#### create_playlist_from_playlist_subset

- 当前脚本 DSL 可表达：
  - `createPlaylist`
  - `getPlaylists`
  - `getPlaylistTracks`
  - `addTracksToPlaylist`
- 当前脚本 DSL 不可表达：
  - “在 `getPlaylists` 结果里按名字过滤再继续”
  - “如果多候选则自动转澄清”

### 5.4 第一阶段验收标准

1. 服务端不再把复合任务直接绑死在 `script selected?`
2. 客户端与服务端对每个高频模板的执行边界有统一文档
3. 典型复合任务至少可以进入“模板已选中，但待补对象”状态

## 6. 第二阶段：对象解析层独立与结果分级

### 6.1 服务端改造项

建议新增中间结构：

- `ResolvedGoal`
- `ResolvedObjects`
- `ResolutionStatus`

目标是让对象解析结果不再只是挂在 `semantic.entities.*` 下，而是形成更明确的结果层。

例如：

- `goal = create_playlist_from_playlist_subset`
- `objects.targetPlaylist = 周杰伦`
- `objects.sourcePlaylist = 流行`
- `objects.trackSelection = first_n(3)`
- `resolutionStatus = partial`

### 6.2 分级建议

对象解析结果至少区分：

1. `complete`
2. `partial`
3. `ambiguous`
4. `missing`

这样模板层可以决定：

- 自动继续
- 先自动查对象
- 发澄清
- 阻断

### 6.3 客户端改造项

客户端补强对象能力文档，重点说明：

- `playlistId` 是否稳定
- `trackId` 是否稳定
- 哪些返回结果适合被后续步骤稳定消费
- 哪些结果只是展示数据，不应直接作为后续动作输入

建议新增文档：

- `说明文档/客户端对象稳定性与结果集消费说明.md`

### 6.4 第二阶段验收标准

1. 服务端能明确区分“对象缺失”和“对象歧义”
2. 不再出现把整句脏片段误当 playlist 名称的情况
3. 客户端文档能支撑服务端做对象级路由判断

## 7. 第三阶段：执行载体路由层独立

这是最关键的一步。

### 7.1 服务端改造项

建议新增：

- `src/music_agent/execution_router.py`
- 或 `src/music_agent/execution_substrate_router.py`

职责：

- 在模板已确定后，再决定当前任务该走：
  - `script`
  - `tool_chain`
  - `clarification`
  - `approval`

### 7.2 典型例子

对于 `create_playlist_from_playlist_subset`：

#### 情况 A：来源歌单已唯一解析

- 执行载体：`script`

#### 情况 B：来源歌单尚未唯一解析，但可自动查询

- 执行载体：`tool_chain(getPlaylists)` 或 `clarification`

#### 情况 C：dry-run 返回高风险

- 执行载体：`approval`

### 7.3 客户端改造项

客户端要补“脚本能力边界声明”文档：

- `说明文档/客户端脚本与能力入口边界说明.md`

至少明确：

- 哪些模板适合脚本
- 哪些模板适合 façade 下 tool 执行
- 哪些模板当前 DSL 还不应该脚本化

### 7.4 第三阶段验收标准

1. 同一个目标在不同对象状态下可以走不同执行载体
2. 复合任务不再因为“脚本不能立即生成”而直接终止
3. `client_script_planner.py` 不再承担总路由职责

## 8. 第四阶段：双端审计与调试链统一

### 8.1 服务端改造项

建议新增显式审计事件：

- `semantic_goal_selected`
- `semantic_object_resolution`
- `command_template_selected`
- `execution_substrate_selected`
- `execution_substrate_rejected`
- `fallback_reason`
- `clarification_required`

### 8.2 客户端改造项

客户端在 `[AgentDebug]` 中补充两类输出：

1. 没有脚本但收到了 `tool_call` 时：
   - 打印“本轮直接工具调用链”
2. 没有脚本也没有 `tool_call` 时：
   - 打印“本轮被服务端阻断/澄清”

### 8.3 第四阶段验收标准

联调时只看一份客户端日志和一份服务端事件流，就能判断问题是在：

- 目标理解
- 对象解析
- 模板选择
- 执行载体选择
- 风控
- 客户端执行

## 9. 第五阶段：资产层与知识消费分层

这是更接近 Claude Code 方法论的一步。

### 9.1 服务端改造项

建议继续新增或完善：

- `goal_catalog.py`
- `command_templates.py`
- `execution_policy_hooks.py`
- `response_style_hooks.py`
- `semantic_audit.py`

同时把现有：

- `capability_catalog.py`
- `task_templates.py`
- `policy_hooks.py`
- `response_style.py`

继续往“稳定资产层”推进。

### 9.2 客户端改造项

客户端文档组织建议收束成三层：

1. `Capability Summary`
2. `Execution Boundary`
3. `Evidence / Code Anchors`

这对应 Claude Code 常见的：

- `router`
- `summary`
- `evidence`

### 9.3 第五阶段验收标准

1. 双端都能以资产层而不是零散经验来协作
2. 不再需要每轮联调都重新口头解释能力边界

## 10. 双端当前优先级排序

### 10.1 服务端优先级

1. 新增 `command_templates.py`
2. 新增 `execution_router.py`
3. 把 `client_script_planner.py` 降级成脚本构造器
4. 增加语义/模板/载体审计事件
5. 清理剩余 legacy helper 与编码污染

### 10.2 客户端优先级

1. 新增“能力模板映射文档”
2. 新增“脚本与 façade 边界文档”
3. `[AgentDebug]` 补无脚本时的 fallback 打印
4. 继续把高频主链从 `AgentToolExecutor` 下沉到 `AgentCapabilityFacade`
5. 为后续统一能力入口提供更细粒度的能力分类

## 11. 优先同步联调的三条模板

### 11.1 inspect_playlist_tracks

用户句子：

- `列出流行歌单的所有音乐`

预期：

- 服务端模板：`inspect_playlist_tracks`
- 执行载体：`tool_chain`
- 工具链：`getPlaylists -> getPlaylistTracks`

### 11.2 get_recent_tracks

用户句子：

- `列出最近播放列表的所有音乐`

预期：

- 服务端模板：`get_recent_tracks`
- 执行载体：`tool_call`
- 工具链：`getRecentTracks`

### 11.3 create_playlist_from_playlist_subset

用户句子：

- `创建一个歌单，歌单名为周杰伦，周杰伦歌单里面添加流行歌单的前三首音乐`

预期：

- 服务端模板：`create_playlist_from_playlist_subset`
- 来源歌单唯一时：
  - 优先脚本化
- 来源歌单不唯一时：
  - 先自动解析来源歌单或发澄清
- 不允许再直接终止于“没有安全脚本”

## 12. 风险与注意事项

### 12.1 当前不要做的事

1. 不要让服务端为了“先跑起来”继续把复合任务硬退化成普通工具调用
2. 不要让客户端假装当前 DSL 已经支持条件、过滤、事务回滚
3. 不要在没有统一模板层之前继续扩更多零散 heuristic
4. 不要把 `AgentCapabilityFacade` 当成简单转发层就停住不动

### 12.2 兼容性原则

1. 当前协议优先保持兼容
2. 新逻辑优先加在模板层、路由层、审计层
3. 不需要为了这轮改造立刻新增一堆新协议类型

### 12.3 当前最重要的判断

当前系统的主矛盾已经不是：

- “模型懂不懂中文”

而是：

- “双端有没有围绕同一个任务模板和执行边界工作”

## 13. 推荐文档清单

### 服务端建议新增

- `服务端-客户端同步改造计划（Claude Code对齐版）.md`
- `command_templates.py` 对应说明文档
- `execution_router.py` 对应说明文档

### 客户端建议新增

- `说明文档/客户端能力模板映射（供Agent路由）.md`
- `说明文档/客户端脚本与能力入口边界说明.md`
- `说明文档/客户端对象稳定性与结果集消费说明.md`

## 14. 一句话总括

这一轮双端同步改造的真正目标不是“让 AI 更会猜”，而是：

**让服务端先选对目标模板，再由客户端统一能力入口稳定执行，让脚本、工具链、澄清、审批都成为模板驱动下的不同执行载体，而不是彼此竞争的 fallback。**
