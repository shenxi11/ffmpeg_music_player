# Qt 端最终联调协议稿

## 1. 文档目标

这份文档用于给 Qt 端和服务端联调时作为统一协议基线。

覆盖范围：

- REST 会话接口
- WebSocket 聊天接口
- 流式消息
- 工具调用协议
- 澄清、计划、审批协议
- 第二期扩展保留位

适用阶段：

- 第一期开启：聊天 + 流式 + 会话存储 + 工具直连 MVP
- 第二期扩展：计划、审批、进度、最终结果

补充说明：

- 当前服务端的语义解析已经升级为 `LLM 优先 + 结构化 JSON`
- Qt 端无需新增协议类型，但需要确保工具结果结构稳定，方便服务端继续自动续调

---

## 2. 传输通道

### 2.1 REST

用途：

- 会话列表
- 会话创建
- 历史消息加载
- 会话标题管理
- 健康检查

### 2.2 WebSocket

用途：

- 实时聊天
- 流式回复
- 工具调用
- 工具结果回传
- 澄清
- 计划
- 审批
- 进度

约定：

- 所有实时交互都走同一条 `WS /ws/chat`
- 第一版不拆分独立工具通道

---

## 3. REST 协议

## 3.1 `GET /healthz`

用途：

- 检查服务是否可用
- 检查模型配置
- 检查是否启用了工具模式

响应示例：

```json
{
  "status": "ok",
  "modelConfigured": true,
  "missingConfig": [],
  "openaiBaseUrl": "https://open.bigmodel.cn/api/paas/v4",
  "openaiModel": "glm-5",
  "sessionHistoryLimit": 20,
  "storagePath": "data/music_agent.db",
  "protocolVersion": "1.3",
  "capabilities": ["chat", "streaming", "sessions", "storage", "tools", "plans", "approval", "audit"],
  "toolModeEnabled": true,
  "auditEnabled": true
}
```

字段说明：

- `status`
  - `ok`
  - `degraded`
- `modelConfigured`
  - 模型配置是否完整
- `protocolVersion`
  - 当前协议版本
- `capabilities`
  - 当前启用能力列表
- `toolModeEnabled`
  - 是否支持 `tool_call / tool_result`
- `auditEnabled`
  - 是否支持事件审计接口

## 3.2 `GET /sessions`

用途：

- 加载左侧会话列表

查询参数：

- `query`
- `limit`

响应示例：

```json
{
  "items": [
    {
      "sessionId": "session-1",
      "title": "解释一下 C++ 多态",
      "createdAt": "2026-03-28T10:00:00+00:00",
      "updatedAt": "2026-03-28T10:01:00+00:00",
      "lastPreview": "多态是面向对象中的一种能力",
      "messageCount": 2
    }
  ]
}
```

## 3.3 `POST /sessions`

用途：

- 新建会话

请求示例：

```json
{
  "title": "新会话"
}
```

## 3.4 `GET /sessions/{session_id}`

用途：

- 拉取单个会话信息

## 3.5 `PATCH /sessions/{session_id}`

用途：

- 重命名会话

请求示例：

```json
{
  "title": "新的会话标题"
}
```

## 3.6 `DELETE /sessions/{session_id}`

用途：

- 删除会话

## 3.7 `GET /sessions/{session_id}/messages`

用途：

- 拉取历史消息

响应示例：

```json
{
  "session": {
    "sessionId": "session-1",
    "title": "解释一下 C++ 多态",
    "createdAt": "2026-03-28T10:00:00+00:00",
    "updatedAt": "2026-03-28T10:01:00+00:00",
    "lastPreview": "多态是面向对象中的一种能力",
    "messageCount": 2
  },
  "items": [
    {
      "messageId": "msg-1",
      "sessionId": "session-1",
      "role": "user",
      "content": "解释一下 C++ 多态",
      "createdAt": "2026-03-28T10:00:00+00:00"
    },
    {
      "messageId": "msg-2",
      "sessionId": "session-1",
      "role": "assistant",
      "content": "多态是面向对象中的一种能力",
      "createdAt": "2026-03-28T10:00:01+00:00"
    }
  ]
}
```

## 3.8 `GET /sessions/{session_id}/events`

用途：

- 拉取当前会话的工具调用、审批、计划执行事件

## 3.9 `GET /plans/{plan_id}/events`

用途：

- 按单个计划回放执行事件

---

## 4. WebSocket 连接

地址：

```text
ws://127.0.0.1:8765/ws/chat?session_id=<optional>
```

说明：

- `session_id` 可选
- 不传时由服务端生成
- 传了但不存在时，服务端自动创建

连接成功后，服务端先发送 `session_ready`

---

## 5. 通用消息约定

### 5.1 公共字段

不同消息会复用这些字段：

- `type`
- `sessionId`
- `requestId`
- `toolCallId`
- `planId`
- `stepId`

### 5.2 标识规则

- `sessionId`
  - 会话唯一标识
- `requestId`
  - 一次用户输入的前端请求标识
- `toolCallId`
  - 一次工具调用唯一标识
- `planId`
  - 一次计划执行唯一标识
- `stepId`
  - 计划中的步骤标识

### 5.3 顺序约定

- 一个 `user_message` 可以触发：
  - 纯聊天回复
  - `tool_call`
  - `clarification_request`
  - 第二期的 `plan_preview`
- 一个 `tool_call` 必须对应一个 `tool_result`
- 同一会话第一期默认不支持并发多个 pending `tool_call`
- 服务端会先做结构化语义解析，再决定是否直接进入工具链

---

## 6. 第一期开启的 WebSocket 消息

## 6.1 `session_ready`

服务端 -> Qt

```json
{
  "type": "session_ready",
  "sessionId": "session-1",
  "title": "新建会话",
  "capabilities": ["chat", "streaming", "sessions", "storage", "tools", "plans", "approval", "audit"]
}
```

Qt 端建议动作：

- 保存 `sessionId`
- 更新标题
- 根据 `capabilities` 启用或关闭工具联调分支

## 6.2 `user_message`

Qt -> 服务端

```json
{
  "type": "user_message",
  "requestId": "req-1",
  "content": "播放周杰伦的晴天"
}
```

## 6.3 `assistant_start`

服务端 -> Qt

```json
{
  "type": "assistant_start",
  "sessionId": "session-1",
  "requestId": "req-1"
}
```

Qt 端建议动作：

- 创建一条空 assistant 消息

## 6.4 `assistant_chunk`

服务端 -> Qt

```json
{
  "type": "assistant_chunk",
  "sessionId": "session-1",
  "requestId": "req-1",
  "delta": "已开始播放"
}
```

Qt 端建议动作：

- 追加到当前 assistant 消息的 `rawText`
- 重新渲染 Markdown

## 6.5 `assistant_final`

服务端 -> Qt

```json
{
  "type": "assistant_final",
  "sessionId": "session-1",
  "requestId": "req-1",
  "content": "已开始播放 **晴天** - **周杰伦**。"
}
```

Qt 端建议动作：

- 用 `content` 作为最终文本
- 把消息标记为完成

## 6.6 `error`

服务端 -> Qt

```json
{
  "type": "error",
  "sessionId": "session-1",
  "requestId": "req-1",
  "code": "invalid_message",
  "message": "content must be a non-empty string"
}
```

第一期常见错误码：

- `invalid_json`
- `invalid_message`
- `unsupported_message_type`
- `model_not_configured`
- `stream_interrupted`
- `model_error`
- `invalid_tool_result`
- `tool_result_timeout`
- `runtime_error`

---

## 7. 第一期开启的工具协议

## 7.1 `tool_call`

服务端 -> Qt

```json
{
  "type": "tool_call",
  "toolCallId": "tool-1",
  "sessionId": "session-1",
  "tool": "searchTracks",
  "args": {
    "keyword": "晴天",
    "artist": "周杰伦",
    "limit": 5
  }
}
```

字段说明：

- `tool`
  - 当前工具名
- `args`
  - 已由服务端清洗过的参数对象

Qt 端要求：

- 必须原样带回 `toolCallId`
- 不要自行修改 `tool`
- 按工具注册表执行

## 7.2 `tool_result`

Qt -> 服务端

成功示例：

```json
{
  "type": "tool_result",
  "toolCallId": "tool-1",
  "ok": true,
  "result": {
    "items": [
      {
        "trackId": "track-1",
        "title": "晴天",
        "artist": "周杰伦",
        "album": "叶惠美",
        "durationMs": 269000,
        "isFavorite": false
      }
    ]
  }
}
```

失败示例：

```json
{
  "type": "tool_result",
  "toolCallId": "tool-1",
  "ok": false,
  "error": {
    "code": "track_not_found",
    "message": "未找到符合条件的歌曲",
    "retryable": false
  }
}
```

约束：

- `ok=false` 时必须有 `error`
- `ok=true` 时建议返回稳定结构化 `result`
- 不要只返回自由文本

## 7.3 `clarification_request`

服务端 -> Qt

```json
{
  "type": "clarification_request",
  "sessionId": "session-1",
  "requestId": "req-1",
  "question": "我找到了多个候选歌曲，请告诉我你想播放哪一个。",
  "options": ["1. 周杰伦 - 晴天", "2. 五月天 - 晴天"]
}
```

Qt 端建议动作：

- 渲染成一条系统提示或 assistant 卡片
- 可选做成按钮列表
- 用户确认后仍然回发普通 `user_message`

例如：

```json
{
  "type": "user_message",
  "requestId": "req-2",
  "content": "第一个"
}
```

---

## 8. 第一期开启的 Host API 工具

## 8.1 `searchTracks`

请求参数：

```json
{
  "keyword": "晴天",
  "artist": "周杰伦",
  "album": "",
  "limit": 5
}
```

成功结果：

```json
{
  "items": [
    {
      "trackId": "track-1",
      "title": "晴天",
      "artist": "周杰伦",
      "album": "叶惠美",
      "durationMs": 269000,
      "isFavorite": false
    }
  ]
}
```

## 8.2 `getCurrentTrack`

请求参数：

```json
{}
```

成功结果：

```json
{
  "trackId": "track-1",
  "title": "晴天",
  "artist": "周杰伦",
  "album": "叶惠美",
  "playlistId": "playlist-1",
  "positionMs": 12000,
  "durationMs": 269000,
  "playing": true
}
```

## 8.3 `getPlaylists`

请求参数：

```json
{}
```

成功结果：

```json
{
  "items": [
    {
      "playlistId": "playlist-1",
      "name": "夜跑歌单",
      "trackCount": 20
    }
  ]
}
```

## 8.4 `playTrack`

请求参数：

```json
{
  "trackId": "track-1"
}
```

成功结果：

```json
{
  "played": true,
  "track": {
    "trackId": "track-1",
    "title": "晴天",
    "artist": "周杰伦",
    "album": "叶惠美",
    "durationMs": 269000,
    "isFavorite": false
  }
}
```

## 8.5 `getPlaylistTracks`

请求参数：

```json
{
  "playlistId": "playlist-1"
}
```

成功结果：

```json
{
  "playlist": {
    "playlistId": "playlist-1",
    "name": "夜跑歌单",
    "trackCount": 20
  },
  "items": [
    {
      "trackId": "track-1",
      "title": "晴天",
      "artist": "周杰伦",
      "durationMs": 269000
    }
  ]
}
```

## 8.6 `getRecentTracks`

请求参数：

```json
{
  "limit": 10
}
```

成功结果：

```json
{
  "items": [
    {
      "trackId": "track-1",
      "title": "晴天",
      "artist": "周杰伦",
      "durationMs": 269000
    }
  ]
}
```

## 8.7 `playPlaylist`

请求参数：

```json
{
  "playlistId": "playlist-1"
}
```

成功结果：

```json
{
  "played": true,
  "playlist": {
    "playlistId": "playlist-1",
    "name": "夜跑歌单",
    "trackCount": 20
  }
}
```

---

## 9. 第二期保留协议

这些消息第二期启用，Qt 端现在就可以按此稿预留模型。

## 9.1 `plan_preview`

服务端 -> Qt

```json
{
  "type": "plan_preview",
  "planId": "plan-1",
  "sessionId": "session-1",
  "summary": "创建学习歌单并添加最近常听歌曲",
  "riskLevel": "medium",
  "steps": [
    { "stepId": "step-1", "title": "创建歌单 学习歌单" },
    { "stepId": "step-2", "title": "获取最近常听歌曲" },
    { "stepId": "step-3", "title": "将歌曲加入歌单" }
  ]
}
```

## 9.2 `approval_request`

服务端 -> Qt

```json
{
  "type": "approval_request",
  "planId": "plan-1",
  "sessionId": "session-1",
  "message": "即将创建歌单并批量添加 20 首歌曲，是否继续？",
  "riskLevel": "high"
}
```

## 9.3 `approval_response`

Qt -> 服务端

```json
{
  "type": "approval_response",
  "planId": "plan-1",
  "approved": true
}
```

## 9.4 `progress`

服务端 -> Qt

```json
{
  "type": "progress",
  "planId": "plan-1",
  "stepId": "step-2",
  "message": "已创建歌单，正在获取最近常听歌曲"
}
```

## 9.5 `final_result`

服务端 -> Qt

```json
{
  "type": "final_result",
  "planId": "plan-1",
  "sessionId": "session-1",
  "ok": true,
  "summary": "已创建歌单“学习歌单”，并添加 20 首歌曲"
}
```

---

## 10. Qt 端联调时序建议

## 10.1 纯聊天

```text
Qt -> user_message
Server -> assistant_start
Server -> assistant_chunk...
Server -> assistant_final
```

## 10.2 播放歌曲

```text
Qt -> user_message("播放周杰伦的晴天")
Server -> tool_call(searchTracks)
Qt -> tool_result(searchTracks)
Server -> tool_call(playTrack)
Qt -> tool_result(playTrack)
Server -> assistant_start/chunk/final
```

## 10.3 歧义澄清

```text
Qt -> user_message("播放晴天")
Server -> tool_call(searchTracks)
Qt -> tool_result(多个候选)
Server -> clarification_request
Qt -> user_message("第一个")
Server -> tool_call(playTrack)
Qt -> tool_result
Server -> assistant_final
```

## 10.4 第二期审批

```text
Qt -> user_message("创建学习歌单并加入最近常听歌曲")
Server -> plan_preview
Server -> approval_request
Qt -> approval_response(true)
Server -> progress
Server -> tool_call(createPlaylist)
Qt -> tool_result
Server -> tool_call(getTopPlayedTracks)
Qt -> tool_result
Server -> tool_call(addTracksToPlaylist)
Qt -> tool_result
Server -> final_result
```

## 10.5 查询歌单与查看歌单内容

```text
Qt -> user_message("帮我查询我的流行歌单")
Server -> tool_call(getPlaylists)
Qt -> tool_result(getPlaylists)
Server -> assistant_final
```

说明：

- 服务端会同时使用 `playlist.rawQuery` 和 `playlist.normalizedQuery` 做匹配
- 例如用户说“流行歌单”时，服务端会同时尝试匹配 `流行歌单` 和 `流行`

```text
Qt -> user_message("看看流行歌单里有什么歌")
Server -> tool_call(getPlaylists)
Qt -> tool_result(getPlaylists)
Server -> tool_call(getPlaylistTracks)
Qt -> tool_result(getPlaylistTracks)
Server -> assistant_final
```

```text
Qt -> user_message("看看这个歌单里有什么歌")
Server -> tool_call(getPlaylistTracks)
Qt -> tool_result(getPlaylistTracks)
Server -> assistant_final
```

## 10.6 最近播放查询

```text
Qt -> user_message("最近听了什么")
Server -> tool_call(getRecentTracks)
Qt -> tool_result(getRecentTracks)
Server -> assistant_final
```

---

## 11. Qt 端实现建议

建议 Qt 端按职责拆分：

- `AgentWebSocketClient`
  - 负责发收 JSON
- `AgentSessionService`
  - 负责 REST 会话接口
- `AgentToolExecutor`
  - 负责收到 `tool_call` 后执行本地 Host API
- `ToolRegistry`
  - 负责校验工具名和参数
- `AgentChatViewModel`
  - 负责消息列表和状态驱动
- `PlanViewModel`
  - 第二期负责计划和审批 UI

---

## 12. 版本兼容建议

Qt 端联调时应先读取：

- `/healthz.protocolVersion`
- `/healthz.capabilities`
- `session_ready.capabilities`

此外建议 Qt 端在联调时关注：

- 服务端现在默认每条 `user_message` 都会先做结构化语义解析
- 如果出现“理解错了但协议没错”的问题，优先检查服务端语义解析日志/审计事件，而不是先怀疑 Qt 执行器

建议策略：

- 没有 `tools`
  - 只启用纯聊天
- 有 `tools` 但没有第二期能力
  - 启用工具执行，不显示计划审批 UI
- 后续若服务端增加：
  - `plans`
  - `approval`
  - `progress`
  - 可再按能力位逐步打开 UI

---

## 13. 一句话结论

最终联调协议的核心原则是：

Qt 端负责会话、渲染、工具执行和用户确认，服务端负责理解、规划、调度和总结；双方通过一组稳定的 REST + WebSocket JSON 消息完成协作，而不是让 AI 直接控制界面。
