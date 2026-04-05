# 客户端能力模板映射（供Agent路由）

## 1. 文档目标

这份文档给服务端 Agent 的模板层、执行载体路由层和调试同学使用，不是面向普通用户的功能说明。

它要回答的问题是：

- 当前 Qt 客户端已经稳定支持哪些高频任务模板
- 每个模板更适合走 `script`、`tool_chain`、`单工具` 还是 `澄清/审批`
- 模板执行时依赖哪些对象
- 哪些步骤可以在客户端稳定落地，哪些步骤必须先由服务端完成对象解析

相关代码锚点：

- 能力注册：`src/agent/tool/ToolRegistry.cpp`
- 统一入口：`src/agent/capability/AgentCapabilityFacade.cpp`
- 当前底层执行器：`src/agent/tool/AgentToolExecutor.cpp`
- 脚本执行器：`src/agent/script/AgentScriptExecutor.cpp`

---

## 2. 当前客户端执行载体定义

### 2.1 script

含义：

- 服务端先生成受控 DSL 脚本
- 客户端按顺序执行脚本步骤
- 步骤内部仍然调用 `AgentCapabilityFacade`

适合：

- 多步但路径稳定的任务
- 服务端已经拿到足够对象，或可以在脚本内按顺序补对象

不适合：

- 需要条件分支
- 需要循环/过滤
- 需要回滚
- 需要在脚本里根据运行结果做复杂决策

### 2.2 tool_chain

含义：

- 服务端自己决定一小段有序工具链
- 客户端逐个执行直接工具调用

适合：

- 目标明确，但对象还需要先查再继续
- 当前脚本 DSL 不足以表达中间决策

### 2.3 single_tool

含义：

- 单个原子能力直接执行

适合：

- 暂停、继续、切歌、查最近播放、查当前播放等原子动作

### 2.4 clarification / approval

含义：

- 服务端暂不执行客户端能力，而是先请求澄清或审批

适合：

- 来源对象歧义
- 幂等性差
- 当前脚本 dry-run 已判定高风险

---

## 3. 当前高频模板映射总表

| 模板名 | 典型用户意图 | 推荐执行载体 | 客户端当前可用性 | 前置对象 | 关键能力 |
| --- | --- | --- | --- | --- | --- |
| `inspect_playlist_tracks` | 列出流行歌单的所有音乐 | `tool_chain` | 可用 | `playlistId` 或可解析的歌单名 | `getPlaylists` -> `getPlaylistTracks` |
| `get_recent_tracks` | 列出最近播放列表的所有音乐 | `single_tool` | 可用 | 无 | `getRecentTracks` |
| `search_and_play_track` | 播放周杰伦的晴天 | `script` 或 `tool_chain` | 可用 | 可搜索关键词 | `searchTracks` -> `playTrack` |
| `play_playlist_from_resolved_object` | 播放流行歌单 | `tool_chain` | 部分可用 | `playlistId` | `getPlaylistTracks` -> `setPlaybackQueue` -> `playAtIndex` |
| `create_playlist_from_playlist_subset` | 创建歌单并搬运来源歌单前 N 首 | `script` 优先 | 部分可用 | 目标歌单名、来源歌单对象、选择规则 | `createPlaylist` + `getPlaylists` + `getPlaylistTracks` + `addPlaylistItems` |
| `inspect_favorites` | 查看我喜欢的音乐 | `single_tool` | 可用 | 无 | `getFavorites` |
| `recent_subset_to_queue` | 把最近播放前三首加入队列 | `script` | 可用 | 无 | `getRecentTracks` + `setPlaybackQueue` / `addToPlaybackQueue` |
| `playlist_subset_to_queue` | 把某歌单前几首加入播放队列 | `tool_chain` 或 `script` | 部分可用 | 来源歌单对象 | `getPlaylistTracks` + `setPlaybackQueue` |
| `playback_control` | 暂停/继续/下一首/上一首/停止 | `single_tool` | 可用 | 当前播放上下文更佳，但多数无硬依赖 | `pausePlayback` / `resumePlayback` / `playNext` / `playPrevious` / `stopPlayback` |

---

## 4. 重点模板详细说明

### 4.1 `inspect_playlist_tracks`

目标：

- 查看指定歌单的歌曲列表

推荐执行载体：

- `tool_chain`

推荐链路：

1. `getPlaylists`
2. 服务端按名字做对象解析
3. `getPlaylistTracks(playlistId)`

为什么当前更适合 `tool_chain`：

- 客户端脚本 DSL 目前不支持“在工具结果里按名称过滤后再自动继续”
- 因此“从歌单列表中解析目标歌单”仍然应该由服务端完成

客户端边界：

- 一旦 `playlistId` 已明确，`getPlaylistTracks` 稳定可用
- 如果只给模糊歌单名，客户端不会自行澄清

---

### 4.2 `get_recent_tracks`

目标：

- 直接拿最近播放结果集

推荐执行载体：

- `single_tool`

推荐链路：

1. `getRecentTracks(limit?)`

特点：

- 这是当前最稳定的原子查询能力之一
- 返回结果适合作为后续脚本或工具链的输入

---

### 4.3 `search_and_play_track`

目标：

- 搜索一首歌并开始播放

推荐执行载体：

- 对象足够清晰时：`script`
- 仍需候选选择时：`tool_chain`

推荐脚本：

1. `searchTracks`
2. `playTrack`

当前客户端边界：

- `playTrack` 最稳定的输入是 `musicPath`
- `trackId` 当前更接近“会话内引用键”，跨会话、跨重启不应被服务端假设为稳定
- 如果服务端恢复历史会话后想继续播放，优先重新 `searchTracks` 或保留 `musicPath`

---

### 4.4 `play_playlist_from_resolved_object`

目标：

- 播放指定歌单

推荐执行载体：

- `tool_chain`

当前最稳妥链路：

1. `getPlaylistTracks`
2. `setPlaybackQueue`
3. `playAtIndex(0)`

为什么不建议直接依赖 `playPlaylist`：

- `playPlaylist` 仍是部分可用能力
- 服务端如果想拿稳定播放行为，应该自己展开成“取歌单歌曲 -> 重建队列 -> 播放第一首”

---

### 4.5 `create_playlist_from_playlist_subset`

目标：

- 创建一个新歌单
- 从来源歌单中选前 N 首搬运过去

推荐执行载体：

- 来源对象唯一时：`script`
- 来源对象不唯一时：先 `tool_chain` 做对象解析，再决定是否脚本化

当前客户端可表达的脚本步骤：

- `createPlaylist`
- `getPlaylists`
- `getPlaylistTracks`
- `addPlaylistItems`

当前客户端不能在脚本内表达的部分：

- 从 `getPlaylists` 结果里按名字过滤来源歌单
- 多候选时自动澄清
- 从曲目列表里按复杂规则筛选

因此模板路由建议：

1. 服务端先解析：
   - `targetPlaylistName`
   - `sourcePlaylistId`
   - `selectionRule = first_n(3)`
2. 对象齐备后再生成脚本

---

### 4.6 `playback_control`

目标：

- 对当前播放做原子控制

推荐执行载体：

- `single_tool`

当前稳定能力：

- `pausePlayback`
- `resumePlayback`
- `stopPlayback`
- `playNext`
- `playPrevious`
- `playAtIndex`

说明：

- 这一类动作不需要脚本
- 服务端不应再为了这类动作强行生成脚本

---

## 5. 当前 DSL 能力范围与模板匹配

### 5.1 DSL 当前擅长的模板

1. 先查结果、后执行固定动作
2. 多步动作但每一步都无需条件判断
3. 步骤间只需要 `$last` 或 `$steps.alias` 取值

适合示例：

- 搜歌并播放
- 查最近播放并加入队列
- 查歌单详情并把全部歌曲装入队列

### 5.2 DSL 当前不擅长的模板

1. 要先查多个候选再做比较
2. 要按名字/属性过滤结果集
3. 要在“缺对象”与“对象歧义”之间分叉
4. 需要补救、回滚或重试

因此这些模板当前仍应由服务端主导：

- 歌单名歧义处理
- “这个歌单”“刚才那个歌单”的引用消解
- 复杂推荐筛选
- 高风险批量写操作

---

## 6. 模板到代码入口的映射

| 模板 | 客户端入口 | 备注 |
| --- | --- | --- |
| `single_tool` | `AgentChatViewModel::onToolCallReceived()` -> `AgentCapabilityFacade::executeCapability()` | 直接工具调用入口 |
| `script` | `AgentChatViewModel::onScriptExecutionRequestReceived()` -> `AgentScriptExecutor::executeScript()` -> `AgentCapabilityFacade::executeCapability()` | 顺序脚本入口 |
| `tool_chain` | 服务端连续下发多次 `tool_call`，客户端每次都走 `AgentCapabilityFacade` | 当前无本地条件路由 |
| `dry_run` | `AgentChatViewModel::onScriptDryRunRequestReceived()` -> `AgentScriptExecutor::dryRunScript()` | 仅预演，不执行 |
| `validate` | `AgentChatViewModel::onScriptValidationRequestReceived()` -> `AgentScriptExecutor::validateScript()` | 校验入口 |

---

## 7. 对服务端路由层的建议

### 7.1 先选模板，再选执行载体

不要让“脚本能不能立刻生成”决定整轮任务成败。

推荐顺序：

1. 识别目标模板
2. 解析对象
3. 判断对象状态：
   - `complete`
   - `partial`
   - `ambiguous`
   - `missing`
4. 再决定是：
   - `script`
   - `tool_chain`
   - `clarification`
   - `approval`

### 7.2 不要把客户端当前边界误判成“任务不可做”

例如：

- 客户端 DSL 不能自己按歌单名过滤，并不等于“创建歌单并搬运前三首”这个目标不可做
- 正确做法是服务端先补对象，再把剩余稳定步骤脚本化

---

## 8. 当前最适合同步联调的模板

### 8.1 `inspect_playlist_tracks`

用户句子：

- `列出流行歌单的所有音乐`

推荐：

- 走 `tool_chain`
- `getPlaylists -> getPlaylistTracks`

### 8.2 `get_recent_tracks`

用户句子：

- `列出最近播放列表的所有音乐`

推荐：

- 走 `single_tool`
- `getRecentTracks`

### 8.3 `create_playlist_from_playlist_subset`

用户句子：

- `创建一个歌单，歌单名为周杰伦，周杰伦歌单里面添加流行歌单的前三首音乐`

推荐：

1. 服务端先完成对象解析
2. 来源歌单唯一后再走 `script`
3. 不要再直接因“当前没有安全脚本”而终止

---

## 9. 一句话结论

当前客户端已经足以支撑“模板驱动 + 载体路由”的双端协作，但前提是：

**服务端先选对模板、补齐对象，再让客户端在 `script / tool_chain / single_tool` 三类执行载体里稳定落地，而不是把“脚本现在能不能独立完成”当成整轮任务的唯一裁判。**
