# AI 控制 Qt 音乐软件总体方案

## 1. 文档目的

这份文档用于定义“聊天 Agent”如何演进成“可安全控制 Qt 音乐软件业务能力的 Agent”。

当前目标不是做一个会点击界面的 AI，也不是做一个只会聊天的问答机器人，而是做一个：

- 能理解用户自然语言意图
- 能把需求拆成结构化步骤
- 能调用 Qt 软件暴露的业务能力
- 能在需要时让用户确认
- 能把执行结果回传给用户

一句话说：

**让 Qt 音乐软件成为 Agent 的业务执行宿主，而不是让 Agent 去操作 Qt 的界面元素。**

---

## 2. 核心原则

## 2.1 AI 不直接操作 UI

Agent 不能做这些事：

- 找按钮并点击
- 模拟鼠标
- 模拟键盘输入
- 操作列表控件
- 依赖界面坐标

Agent 应该做这些事：

- 理解用户意图
- 规划任务步骤
- 调用 Qt 暴露的 Host API
- 根据结果决定下一步

例如：

错误方式：

```text
点击播放按钮
```

正确方式：

```text
playTrack(trackId)
```

## 2.2 复杂任务先规划，再执行

推荐总流程：

```text
用户自然语言
-> 意图识别
-> 结构化计划
-> 用户确认（必要时）
-> 工具调用
-> 返回结果
```

## 2.3 工具调用必须可控

所有真正会影响软件状态的动作，都必须通过 Qt 端显式注册的工具来完成。

Qt 不应开放任意执行入口。

## 2.4 风险分级

不同动作应有不同执行策略：

- 低风险：直接执行
- 中风险：先展示理解或计划
- 高风险：必须审批

---

## 3. 总体架构

推荐整体架构如下：

```text
Qt Music Client
  ├─ Chat UI
  ├─ Session UI
  ├─ Markdown Rendering
  ├─ AgentWebSocketClient
  ├─ AgentSessionService
  ├─ AgentToolExecutor
  ├─ HostStateProvider
  └─ AgentProcessManager

Python Agent Sidecar
  ├─ Session Store
  ├─ Intent Parser
  ├─ Plan Builder
  ├─ Approval Manager
  ├─ Tool Orchestrator
  ├─ LangGraph Runtime
  └─ LLM Client
```

### 职责划分

Qt 端负责：

- 聊天界面
- 会话存储接入
- 流式展示
- 工具执行
- 当前软件状态提供

Agent 端负责：

- 语义理解
- 计划生成
- 工具调用决策
- 审批流程控制
- 最终结果总结

---

## 4. 能力分层

为了让系统逐步演进，建议按以下能力层建设。

## 4.1 聊天层

当前已具备：

- 多轮聊天
- 流式输出
- 会话持久化
- Markdown 输出规范

## 4.2 语义层

需要新增：

- 意图识别
- 实体抽取
- 指代消解
- 歧义识别
- 风险等级判断

例如：

```json
{
  "intent": "play_track",
  "entities": {
    "title": "晴天",
    "artist": "周杰伦"
  },
  "ambiguities": [],
  "riskLevel": "low"
}
```

## 4.3 规划层

复杂任务不直接执行，而先转成结构化计划。

例如：

```json
{
  "planId": "plan-1",
  "summary": "创建歌单并加入最近常听歌曲",
  "steps": [
    {
      "id": "step-1",
      "tool": "createPlaylist",
      "args": { "name": "夜跑歌单" }
    },
    {
      "id": "step-2",
      "tool": "getTopPlayedTracks",
      "args": { "limit": 20 }
    },
    {
      "id": "step-3",
      "tool": "addTracksToPlaylist",
      "args": { "playlistId": "$step-1.playlistId", "trackIds": "$step-2.trackIds" }
    }
  ],
  "needsApproval": true
}
```

## 4.4 工具层

Qt 端暴露标准业务 API，Agent 只调用这些工具。

## 4.5 审批层

对于高风险或批量修改动作，必须支持用户确认。

---

## 5. Qt 端应暴露什么能力

## 5.1 第一批建议暴露的只读工具

推荐最先开放：

- `searchTracks`
- `getCurrentTrack`
- `getRecentTracks`
- `getTopPlayedTracks`
- `getPlaylists`
- `getPlaylistTracks`

这些能力风险低，非常适合做 Agent 的第一批“感知能力”。

## 5.2 第一批建议暴露的低风险动作

- `playTrack`
- `playPlaylist`

这些动作是用户期望最强、但风险相对可控的能力。

## 5.3 第二批写操作

- `createPlaylist`
- `addTracksToPlaylist`
- `favoriteTracks`
- `unfavoriteTracks`

这些能力建议在聊天和播放控制稳定后再开放。

## 5.4 暂缓能力

当前阶段不建议开放：

- 删除歌单
- 批量删除歌曲
- 覆盖歌单内容
- 文件系统改写
- 自动下载和文件落地

---

## 6. Host API 设计原则

Host API 必须满足：

- 明确定义输入输出
- 不依赖 UI 状态细节
- 支持错误返回
- 参数可验证

### 示例：searchTracks

```json
{
  "tool": "searchTracks",
  "args": {
    "keyword": "晴天",
    "artist": "周杰伦",
    "limit": 10
  }
}
```

返回：

```json
{
  "ok": true,
  "result": {
    "items": [
      {
        "trackId": "123",
        "title": "晴天",
        "artist": "周杰伦",
        "album": "叶惠美"
      }
    ]
  }
}
```

### 示例：playTrack

```json
{
  "tool": "playTrack",
  "args": {
    "trackId": "123"
  }
}
```

---

## 7. 协议演进建议

当前已有消息：

- `session_ready`
- `user_message`
- `assistant_start`
- `assistant_chunk`
- `assistant_final`
- `error`

为了支持 Agent 工具调用，建议新增以下消息。

## 7.1 计划类消息

- `plan_preview`
- `approval_request`
- `approval_response`

### plan_preview 示例

```json
{
  "type": "plan_preview",
  "planId": "plan-1",
  "summary": "创建歌单并添加最近常听歌曲",
  "steps": [
    { "id": "step-1", "title": "创建歌单 夜跑歌单" },
    { "id": "step-2", "title": "获取最近常听 20 首歌曲" },
    { "id": "step-3", "title": "将歌曲加入歌单" }
  ]
}
```

### approval_request 示例

```json
{
  "type": "approval_request",
  "planId": "plan-1",
  "message": "即将创建歌单并批量添加 20 首歌曲，是否继续？"
}
```

## 7.2 工具调用消息

- `tool_call`
- `tool_result`

### tool_call 示例

```json
{
  "type": "tool_call",
  "toolCallId": "tool-1",
  "tool": "searchTracks",
  "args": {
    "keyword": "晴天",
    "artist": "周杰伦",
    "limit": 5
  }
}
```

### tool_result 示例

```json
{
  "type": "tool_result",
  "toolCallId": "tool-1",
  "ok": true,
  "result": {
    "items": [
      {
        "trackId": "123",
        "title": "晴天",
        "artist": "周杰伦"
      }
    ]
  }
}
```

## 7.3 进度类消息

- `progress`
- `final_result`

例如：

```json
{
  "type": "progress",
  "message": "已创建歌单，正在添加歌曲"
}
```

---

## 8. Agent 内部推荐流程

建议将 Agent 内部执行图演进为以下节点。

## 8.1 ParseIntent

输入：

- 用户自然语言
- 当前会话上下文
- 最近工具结果

输出：

- 意图
- 实体
- 风险等级
- 是否需要工具调用

## 8.2 ResolveContext

负责补齐上下文，例如：

- “这首歌” -> 当前播放歌曲
- “那个歌单” -> 当前选中歌单
- “刚才那些歌” -> 最近一次搜索结果

## 8.3 BuildPlan

针对复杂任务输出结构化 plan。

## 8.4 NeedApproval

判断：

- 是否是批量写操作
- 是否会创建/修改用户数据
- 是否存在歧义

若满足条件，则进入审批。

## 8.5 ExecuteToolStep

通过 `tool_call` 请求 Qt 端执行工具。

## 8.6 HandleToolResult

根据 `tool_result`：

- 继续执行下一步
- 补充信息
- 出错回退
- 请求用户澄清

## 8.7 Finish

输出：

- 最终结果
- 成功/失败说明
- 如有必要，附带变更摘要

---

## 9. 风险分级建议

## 9.1 低风险

可直接执行：

- `searchTracks`
- `getCurrentTrack`
- `playTrack`
- `playPlaylist`

## 9.2 中风险

建议先展示理解：

- `createPlaylist`
- `favoriteTracks`
- `unfavoriteTracks`

## 9.3 高风险

必须审批：

- 批量添加歌曲
- 批量取消收藏
- 覆盖已有歌单内容
- 删除相关操作

---

## 10. 歧义处理策略

音乐场景天然存在歧义，必须设计清楚。

## 10.1 同名歌曲

例如用户说：

```text
播放晴天
```

如果搜索到多个候选，Agent 不应直接拍板。

应返回：

```text
我找到多个“晴天”，你想播放哪一个？
1. 周杰伦 - 晴天
2. A歌手 - 晴天
3. B歌手 - 晴天
```

## 10.2 指代消解

例如：

- 这首歌
- 这个歌单
- 刚才那些歌

需要结合：

- 当前播放状态
- 当前会话工作记忆
- 最近工具结果

## 10.3 模糊筛选

例如：

```text
放点轻快的歌
```

这类任务要么：

- 借助已有元数据筛选
- 要么向用户补问条件

不能假装理解得非常精确。

---

## 11. Qt 端新增职责建议

为了支持工具调用，Qt 端建议新增以下组件。

## 11.1 AgentToolExecutor

职责：

- 接收 `tool_call`
- 根据工具名路由到对应业务接口
- 返回 `tool_result`

## 11.2 ToolRegistry

职责：

- 注册允许调用的工具
- 定义参数 schema
- 管理工具权限与可用状态

## 11.3 HostStateProvider

职责：

- 提供当前软件状态
- 例如当前播放歌曲、最近播放、当前歌单等

推荐结构：

```text
AgentChatViewModel
  ├─ AgentSessionService
  ├─ AgentWebSocketClient
  ├─ AgentProcessManager
  ├─ AgentToolExecutor
  └─ HostStateProvider
```

---

## 12. 会话与状态存储建议

当前后端已经支持：

- 会话列表
- 历史消息

进入 Agent 控制阶段后，建议会话上下文再额外保存：

- 最近一次结构化意图
- 最近一次候选搜索结果
- 最近一次 plan
- 最近一次 approval 状态
- 最近一次 tool result 摘要

这类信息不一定马上落 SQLite 结构化表，可以先作为服务端会话工作记忆维护。

---

## 13. 推荐落地顺序

建议分阶段推进，不要一步到位。

## 阶段 1：只读能力

开放：

- `searchTracks`
- `getCurrentTrack`
- `getPlaylists`

目标：

- 让 Agent 能“看见”软件状态

## 阶段 2：低风险播放控制

开放：

- `playTrack`
- `playPlaylist`

目标：

- 让 Agent 能执行最核心的播放类动作

## 阶段 3：写操作

开放：

- `createPlaylist`
- `addTracksToPlaylist`
- `favoriteTracks`

目标：

- 让 Agent 能管理用户内容

## 阶段 4：计划与审批

开放：

- `plan_preview`
- `approval_request`
- `approval_response`

目标：

- 真正进入“智能控制台”模式

---

## 14. 推荐第一批用户场景

第一批应尽量选择高频、低歧义、用户价值明确的任务。

推荐：

- “播放晴天”
- “播放周杰伦的晴天”
- “查看当前播放歌曲”
- “创建一个学习歌单”
- “把这首歌加入喜欢”
- “把最近常听的 20 首歌加到学习歌单里”

这些场景既能体现价值，又容易逐步验证协议和工具链。

---

## 15. 一句话结论

实现“用户向 AI 提要求，然后操作 Qt 音乐软件”的正确方式不是让 AI 控制界面，而是：

**让 Qt 软件向 Agent 暴露一组标准业务工具，Agent 负责理解、规划和决策，Qt 负责执行和返回结果。**

