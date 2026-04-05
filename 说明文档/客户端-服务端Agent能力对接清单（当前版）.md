# 客户端-服务端 Agent 能力对接清单（当前版）

## 1. 文档目标

这份文档用于给服务端 Agent 侧做当前阶段的能力对接，不是泛泛的架构介绍，而是明确回答下面几个问题：

- Qt 客户端现在到底开放了哪些能力
- 哪些能力已经真实可用，哪些只是注册了入口但仍有边界
- 服务端当前应该优先走脚本、直接工具链还是单工具调用
- 哪些复合任务不能直接假设客户端已经支持
- 下一阶段双端最值得继续补哪些能力

文档基于当前真实代码现状整理，核心依据：

- `src/agent/tool/ToolRegistry.cpp`
- `src/agent/tool/AgentToolExecutor.cpp`
- `src/agent/capability/AgentCapabilityFacade.cpp`
- `src/agent/script/AgentScriptExecutor.cpp`
- `src/agent/protocol/AgentProtocolRouter.cpp`
- `src/agent/AgentChatViewModel.cpp`

同时参考以下对齐文档：

- `agent/服务端-客户端同步改造计划（Claude Code对齐版）.md`
- `agent/Claude Code方法论对当前音乐Agent的架构借鉴清单.md`
- `说明文档/客户端能力模板映射（供Agent路由）.md`
- `说明文档/客户端脚本与能力入口边界说明.md`

---

## 2. 当前协议与执行载体

### 2.1 当前客户端支持的 Agent 协议消息

入站消息：

- `tool_call`
- `validate_script`
- `dry_run_script`
- `execute_script`
- `cancel_script`

出站消息：

- `tool_result`
- `script_validation_result`
- `script_dry_run_result`
- `script_execution_started`
- `script_step_event`
- `script_execution_result`
- `script_cancellation_result`
- 普通聊天流消息：`assistant_*` 系列由服务端主导，客户端这里只做展示与调试归档

代码位置：

- 协议解析：`src/agent/protocol/AgentProtocolRouter.cpp`
- 消息发送：`src/agent/AgentWebSocketClient.cpp`
- 调度入口：`src/agent/AgentChatViewModel.cpp`

### 2.2 当前客户端有三层执行载体

1. `AgentCapabilityFacade`
- 当前统一能力入口外观层
- 代码位置：`src/agent/capability/AgentCapabilityFacade.cpp`
- 作用：统一接住原始 `tool_call` 和脚本步骤执行，并做少量名称归一化

2. `AgentToolExecutor`
- 当前真实能力执行器
- 代码位置：`src/agent/tool/AgentToolExecutor.cpp`
- 作用：把能力落到 `MainShellViewModel`、`AudioService`、`PluginManager`、`MainWidget`、`PlayWidget` 等真实业务/界面宿主

3. `AgentScriptExecutor`
- 当前顺序脚本执行器
- 代码位置：`src/agent/script/AgentScriptExecutor.cpp`
- 作用：解析顺序 DSL、做 dry-run/validate/execute/cancel，并把每一步交给 `AgentCapabilityFacade`

### 2.3 当前必须清楚的边界

- 脚本 DSL 目前只支持**顺序步骤**
- 不支持条件、过滤、循环、并发、事务回滚
- 因此服务端不能把“对象解析”“候选过滤”“歧义澄清”下沉给客户端脚本层
- 这些仍应由服务端在 `Goal Understanding -> Object Resolution -> Command Template -> Execution Substrate Routing` 前几层完成

---

## 3. 当前已支持的能力总表

下表以当前客户端真实代码为准。

| 领域 | 能力 | 工具名 | 当前状态 | 推荐载体 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 搜索 | 搜歌曲 | `searchTracks` | 已支持 | `single_tool` / `tool_chain` | 支持 `keyword/artist/album/limit` |
| 搜索 | 搜歌手 | `searchArtist` | 已支持 | `single_tool` | 用于对象解析 |
| 搜索 | 按歌手取歌 | `getTracksByArtist` | 已支持 | `single_tool` / `tool_chain` | 支持 `limit` |
| 歌词 | 取歌词 | `getLyrics` | 已支持 | `single_tool` | 依赖 `trackId` 或 `musicPath` |
| 播放 | 当前播放 | `getCurrentTrack` | 已支持 | `single_tool` | 只读 |
| 播放 | 播放单曲 | `playTrack` | 已支持 | `single_tool` / `script` | 优先先拿结构化 Track |
| 播放 | 播放歌单 | `playPlaylist` | 部分支持 | `tool_chain` | 不建议作为稳定原子能力 |
| 播放 | 暂停 | `pausePlayback` | 已支持 | `single_tool` | 稳定控制能力 |
| 播放 | 恢复 | `resumePlayback` | 已支持 | `single_tool` | 稳定控制能力 |
| 播放 | 停止 | `stopPlayback` | 已支持 | `single_tool` | 稳定控制能力 |
| 播放 | 跳进度 | `seekPlayback` | 已支持 | `single_tool` | 需要 `positionMs` |
| 播放 | 下一首 | `playNext` | 已支持 | `single_tool` | 依赖非空队列 |
| 播放 | 上一首 | `playPrevious` | 已支持 | `single_tool` | 依赖非空队列 |
| 播放 | 按索引播放 | `playAtIndex` | 已支持 | `single_tool` | 依赖非空队列 |
| 播放 | 设置音量 | `setVolume` | 已支持 | `single_tool` | |
| 播放 | 取播放队列 | `getPlaybackQueue` | 已支持 | `single_tool` | |
| 播放 | 设置播放模式 | `setPlayMode` | 已支持 | `single_tool` | |
| 播放 | 重置播放队列 | `setPlaybackQueue` | 已支持 | `script` / `tool_chain` | 高副作用，带审批标记 |
| 播放 | 加入播放队列 | `addToPlaybackQueue` | 已支持 | `single_tool` / `script` | |
| 播放 | 从播放队列移除 | `removeFromPlaybackQueue` | 已支持 | `single_tool` | |
| 播放 | 清空播放队列 | `clearPlaybackQueue` | 已支持 | `single_tool` / `script` | 高副作用，带审批标记 |
| 最近播放 | 查询最近播放 | `getRecentTracks` | 已支持 | `single_tool` / `tool_chain` | |
| 最近播放 | 添加最近播放 | `addRecentTrack` | 已支持 | `single_tool` / `script` | 通常不建议服务端主动调用 |
| 最近播放 | 删除最近播放 | `removeRecentTracks` | 已支持 | `single_tool` / `script` | |
| 喜欢音乐 | 查询喜欢列表 | `getFavorites` | 已支持 | `single_tool` | |
| 喜欢音乐 | 添加喜欢 | `addFavorite` | 已支持 | `single_tool` / `script` | |
| 喜欢音乐 | 取消喜欢 | `removeFavorites` | 已支持 | `single_tool` / `script` | |
| 歌单 | 查歌单列表 | `getPlaylists` | 已支持 | `single_tool` / `tool_chain` | |
| 歌单 | 查歌单内容 | `getPlaylistTracks` | 已支持 | `single_tool` / `tool_chain` | 需要 `playlistId` |
| 歌单 | 创建歌单 | `createPlaylist` | 已支持 | `single_tool` / `tool_chain` | 带审批标记 |
| 歌单 | 更新歌单 | `updatePlaylist` | 已支持 | `single_tool` / `script` | 带审批标记 |
| 歌单 | 删除歌单 | `deletePlaylist` | 已支持 | `single_tool` / `script` | 带审批标记 |
| 歌单 | 歌单加歌 | `addPlaylistItems` | 已支持 | `tool_chain` / `script` | 需先完成对象解析 |
| 歌单 | 历史别名加歌 | `addTracksToPlaylist` | 已支持（兼容） | `tool_chain` | façade 会映射到 `addPlaylistItems` |
| 歌单 | 歌单删歌 | `removePlaylistItems` | 已支持 | `single_tool` / `script` | |
| 歌单 | 歌单排序 | `reorderPlaylistItems` | 已支持 | `script` | 带审批标记 |
| 推荐 | 取推荐列表 | `getRecommendations` | 已支持 | `single_tool` | |
| 推荐 | 取相似推荐 | `getSimilarRecommendations` | 已支持 | `single_tool` | |
| 推荐 | 提交推荐反馈 | `submitRecommendationFeedback` | 已支持 | `single_tool` / `script` | |
| 本地音乐 | 本地列表 | `getLocalTracks` | 已支持 | `single_tool` | |
| 本地音乐 | 导入本地音乐 | `addLocalTrack` | 已支持 | `single_tool` | |
| 本地音乐 | 移除本地音乐 | `removeLocalTrack` | 已支持 | `single_tool` | |
| 下载 | 查询下载任务 | `getDownloadTasks` | 已支持 | `single_tool` | |
| 下载 | 暂停下载 | `pauseDownloadTask` | 已支持 | `single_tool` | |
| 下载 | 恢复下载 | `resumeDownloadTask` | 已支持 | `single_tool` | |
| 下载 | 取消下载 | `cancelDownloadTask` | 已支持 | `single_tool` | |
| 下载 | 删除下载记录 | `removeDownloadTask` | 已支持 | `single_tool` | |
| 视频 | 查询视频窗口状态 | `getVideoWindowState` | 已支持 | `single_tool` | |
| 视频 | 播放视频 | `playVideo` | 已支持 | `single_tool` | 依赖窗口侧宿主可用 |
| 视频 | 暂停视频 | `pauseVideoPlayback` | 已支持 | `single_tool` | |
| 视频 | 恢复视频 | `resumeVideoPlayback` | 已支持 | `single_tool` | |
| 视频 | 视频跳进度 | `seekVideoPlayback` | 已支持 | `single_tool` | |
| 视频 | 切全屏 | `setVideoFullScreen` | 已支持 | `single_tool` | |
| 视频 | 设置倍速 | `setVideoPlaybackRate` | 已支持 | `single_tool` | |
| 视频 | 设置画质预设 | `setVideoQualityPreset` | 已支持 | `single_tool` | |
| 视频 | 关闭视频窗口 | `closeVideoWindow` | 已支持 | `single_tool` | |
| 桌面歌词 | 查询状态 | `getDesktopLyricsState` | 已支持 | `single_tool` | |
| 桌面歌词 | 显示 | `showDesktopLyrics` | 已支持 | `single_tool` | |
| 桌面歌词 | 隐藏 | `hideDesktopLyrics` | 已支持 | `single_tool` | |
| 桌面歌词 | 设置样式 | `setDesktopLyricsStyle` | 已支持 | `single_tool` | |
| 插件 | 插件列表 | `getPlugins` | 已支持 | `single_tool` | |
| 插件 | 插件诊断 | `getPluginDiagnostics` | 已支持 | `single_tool` | |
| 插件 | 重载插件 | `reloadPlugins` | 已支持 | `single_tool` / `script` | 带审批标记 |
| 插件 | 卸载插件 | `unloadPlugin` | 已支持 | `single_tool` / `script` | 带审批标记 |
| 插件 | 卸载全部插件 | `unloadAllPlugins` | 已支持 | `single_tool` / `script` | 带审批标记 |
| 设置 | 设置快照 | `getSettingsSnapshot` | 已支持 | `single_tool` | |
| 设置 | 修改单项设置 | `updateSetting` | 已支持 | `single_tool` / `script` | 带审批标记 |

---

## 4. 当前已支持但有边界的能力

### 4.1 `playPlaylist`

当前已注册、已实现，但不建议服务端把它当作稳定的“整歌单播放原子能力”。

原因：

- 底层仍较依赖当前歌单详情、当前队列和对象缓存状态
- 相比之下，下面这条链更稳定：
  - `getPlaylistTracks`
  - `setPlaybackQueue`
  - `playAtIndex(0)`

建议：

- 服务端编排时优先使用上面的显式三步链路
- `playPlaylist` 可保留给简化路径或人工触发，不作为最稳模板

### 4.2 `playTrack`

当前是稳定能力，但服务端要注意输入对象要求。

优先输入：

- `musicPath`
- 或当前会话内可解析的 `trackId`

风险点：

- `trackId` 更接近当前会话/当前缓存内的引用键
- 跨客户端重启、跨会话恢复时，不应直接假设旧 `trackId` 仍然可播

建议：

- 服务端恢复旧会话后，如需重播歌曲，优先先重新 `searchTracks` 或直接保留 `musicPath`

### 4.3 歌单复合操作

例如：

- 创建歌单并从来源歌单搬运前 N 首歌曲
- 从最近播放筛选歌手后加入歌单

客户端当前能执行的原子动作足够，但不能承担以下逻辑：

- 在列表结果里按名字模糊匹配再自动选择唯一对象
- 从多候选里自动歧义消解
- 在脚本里做条件分支和过滤表达式

建议：

- 服务端必须先完成对象解析，再下发到客户端执行层
- 当前更适合走：`tool_chain fallback` 或“服务端先解析对象，再生成顺序脚本”

### 4.4 视频和桌面歌词

这两类能力当前可用，但更依赖客户端图形宿主环境。

边界：

- 它们不属于纯数据层能力
- 如果未来要支持无头执行器或更纯粹的脚本环境，需要单独拆能力层

---

## 5. 当前脚本能力边界

### 5.1 当前脚本已经支持

- `validate_script`
- `dry_run_script`
- `execute_script`
- `cancel_script`
- 风险分层、审批标记、dry-run 聚合字段
- 顺序步骤执行
- `$last` / `$steps.alias` 这种简单结果引用

### 5.2 当前脚本不支持

- `if/else`
- 过滤表达式
- 列表切片
- 条件跳转
- 循环
- 并发
- 回滚
- 基于多候选结果自动转澄清

### 5.3 当前服务端不要直接假设的能力

当前不要假设客户端脚本可以直接表达下面这种任务：

- “找到名为 X 的歌单，如果没有就创建”
- “在来源歌单里取前三首周杰伦的歌，如果不够三首就补充最近播放里的周杰伦”
- “若有多个候选歌单则自动选择最近更新的那个”

这些仍应由服务端模板层、对象解析层和路由层完成。

---

## 6. 待支持能力与后续建议

下面这些不是“完全没有能力”，而是当前客户端还没有把它们提升成更稳定、更适合服务端直接依赖的能力形态。

### 6.1 第一优先级：模板友好的 façade 能力

建议继续补成 façade 友好能力，而不只是停留在 `AgentToolExecutor` 分支里：

1. 面向歌单的高频组合能力
- 如：
  - `appendTracksToPlaylistByResolvedTracks`
  - `replacePlaybackQueueByResolvedTracks`
- 目的：减少服务端对底层参数拼装细节的依赖

2. 面向播放的更稳定原子能力
- 明确“按 `musicPath` 播放”与“按结构化 Track 播放”的入口边界
- 明确“播放歌单”走哪条最稳定链路

### 6.2 第二优先级：脚本 DSL 表达能力增强

当前最值得补的不是复杂自治，而是有限增强：

1. 简单过滤
- 例如从结果数组里按 `name` 或 `artist` 取唯一对象

2. 简单切片
- 例如 `first_n(3)`

3. 轻量条件
- 例如“为空则失败并返回结构化原因”

这三项一旦补上，服务端大量高频模板都更容易转向脚本执行。

### 6.3 第三优先级：更稳定的对象持久化引用

建议后续考虑：

- `trackId -> musicPath` 的更稳定映射
- `playlistId -> playlist detail` 的更稳定缓存
- 让跨会话恢复时，不再轻易出现“旧对象引用失效”

### 6.4 第四优先级：无头环境与图形宿主解耦

如果后面要让脚本执行器更像“通用客户端内核”，需要逐步把下面这类能力从窗口对象桥接中解耦：

- 视频窗口控制
- 桌面歌词控制
- 部分 QQuickWidget 宿主型能力

---

## 7. 服务端当前推荐对接方式

### 7.1 推荐优先级

1. 单工具即可完成的任务
- 直接发 `tool_call`
- 例如：暂停播放、恢复播放、下一首、获取最近播放、获取喜欢列表

2. 需要多步但对象已解析完成的任务
- 优先走顺序脚本
- 例如：
  - 已拿到来源歌单和目标歌单对象后，创建歌单并加歌
  - 已拿到一组 Track 后，重置播放队列并开始播放

3. 需要对象解析、歧义消解、过滤选择的任务
- 服务端先走模板层和对象解析层
- 再决定：
  - `script`
  - `tool_chain fallback`
  - `clarify`
  - `approval`

### 7.2 当前不推荐的做法

1. 不要把“脚本 planner 没选中”直接等价成“任务失败”
2. 不要假设客户端脚本能承担过滤、条件、澄清逻辑
3. 不要在恢复旧会话时直接复用旧 `trackId` 做播放
4. 不要把 `playPlaylist` 当作当前最稳的跨场景原子能力

### 7.3 当前服务端最好同步更新的能力认知

至少把下面这些认知同步到服务端：

- 客户端已经支持 `addTracksToPlaylist` 历史别名兼容
- 无脚本场景下，客户端日志会打印直接工具链
- 当前脚本能力仍然是“顺序步骤 DSL”，不是通用流程编排语言
- 当前执行入口已开始收束到 `AgentCapabilityFacade`

---

## 8. 联调时应重点观察的日志与证据

客户端当前调试块会打印：

- 用户问题
- 原始脚本
- Agent 回复
- Agent 脚本
- 直接工具链
- 本轮执行路径

重点判断方式：

1. `原始脚本: <none>` 且 `直接工具链: <none>`
- 说明很可能是服务端提前阻断或纯聊天回复

2. `原始脚本: <none>` 但 `直接工具链` 有多步
- 说明本轮走了 `tool_chain fallback`

3. 有 `原始脚本`
- 说明本轮走脚本路径

4. 同时有脚本和直接工具链
- 说明本轮执行路径发生了混合，服务端需要重点审计 planner / router 决策

代码位置：

- `src/agent/AgentChatViewModel.cpp`

---

## 9. 给服务端的最终结论

### 9.1 现在可以放心直接依赖的能力

- 播放控制单步能力
- 最近播放、喜欢音乐、歌单查询、歌词查询、搜索、推荐、下载、设置、插件、视频窗口基础控制

### 9.2 现在应谨慎依赖的能力

- `playPlaylist`
- 需要对象过滤/歧义解析的复合任务
- 依赖旧 `trackId` 跨重启恢复的播放任务

### 9.3 现在不要误判已支持的能力

- 条件脚本
- 过滤脚本
- 事务回滚
- 通用自治脚本语言
- 客户端独立完成对象解析与多候选澄清

### 9.4 双端最值得继续同步推进的方向

1. 服务端：模板层、对象解析层、执行载体路由层继续独立
2. 客户端：继续把高频能力从 `AgentToolExecutor` 往 `AgentCapabilityFacade` 收束
3. 双端：继续把“已支持的原子能力”和“待支持的复合能力”分开建模，避免误把未来目标当当前事实

---

## 10. 关键代码索引

- 工具注册表：`src/agent/tool/ToolRegistry.cpp`
- 真实工具执行：`src/agent/tool/AgentToolExecutor.cpp`
- façade 统一入口：`src/agent/capability/AgentCapabilityFacade.cpp`
- 脚本执行器：`src/agent/script/AgentScriptExecutor.cpp`
- 协议解析：`src/agent/protocol/AgentProtocolRouter.cpp`
- WebSocket 发送：`src/agent/AgentWebSocketClient.cpp`
- 调试日志与总调度：`src/agent/AgentChatViewModel.cpp`
- 服务端同步计划：`agent/服务端-客户端同步改造计划（Claude Code对齐版）.md`
- 方法论借鉴：`agent/Claude Code方法论对当前音乐Agent的架构借鉴清单.md`
