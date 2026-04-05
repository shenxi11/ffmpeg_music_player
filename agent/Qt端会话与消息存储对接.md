# Qt 端会话与消息存储对接文档

## 1. 文档目的

这份文档用于指导 Qt 客户端对接服务端新增的：

- 会话列表存储
- 会话创建
- 会话标题更新
- 会话删除
- 历史消息加载

适用场景：

- 左侧会话栏
- 新建会话按钮
- 搜索会话
- 点击会话切换聊天内容
- 历史消息恢复显示

当前目标是让客户端 UI 与服务端会话存储保持一致，而不是只依赖本地内存状态。

---

## 2. 服务端当前能力概览

当前服务端已经支持两类接口：

### 2.1 REST 接口

用于会话列表和历史消息管理：

- `GET /sessions`
- `POST /sessions`
- `GET /sessions/{session_id}`
- `PATCH /sessions/{session_id}`
- `DELETE /sessions/{session_id}`
- `GET /sessions/{session_id}/messages`

### 2.2 WebSocket 接口

用于当前活跃会话聊天：

- `WS /ws/chat?session_id=<optional>`

说明：

- 客户端进入某个会话聊天时，连接对应 `session_id`
- 如果该 `session_id` 不存在，服务端会自动创建
- 连接成功后 `session_ready` 会返回当前会话标题

---

## 3. 服务端数据模型

## 3.1 会话摘要

服务端返回的会话项结构：

```json
{
  "sessionId": "uuid",
  "title": "解释一下C++的多态",
  "createdAt": "2026-03-27T09:23:11.799831+00:00",
  "updatedAt": "2026-03-27T09:23:11.799831+00:00",
  "lastPreview": "echo:多态是什么",
  "messageCount": 2
}
```

字段含义：

- `sessionId`：会话唯一 id
- `title`：会话标题
- `createdAt`：创建时间
- `updatedAt`：最后更新时间
- `lastPreview`：最近一条内容摘要，适合左侧列表副标题
- `messageCount`：当前会话消息条数

## 3.2 消息结构

```json
{
  "messageId": "uuid",
  "sessionId": "uuid",
  "role": "user",
  "content": "多态是什么",
  "createdAt": "2026-03-27T09:24:00.000000+00:00"
}
```

字段含义：

- `messageId`：消息唯一 id
- `sessionId`：所属会话 id
- `role`：`user` / `assistant`
- `content`：消息原始 Markdown 文本
- `createdAt`：消息创建时间

说明：

- `content` 是原始消息正文
- assistant 消息可能包含 Markdown、代码块、标题等
- Qt 客户端应结合你们现有 Markdown 渲染方案展示

---

## 4. 推荐 Qt 数据结构

## 4.1 会话列表项

建议 Qt 端定义：

```cpp
struct ChatSessionItem {
    QString sessionId;
    QString title;
    QDateTime createdAt;
    QDateTime updatedAt;
    QString lastPreview;
    int messageCount = 0;
    bool selected = false;
};
```

## 4.2 消息项

建议继续沿用你们现有的消息结构，但补齐后端字段：

```cpp
struct ChatMessageItem {
    QString messageId;
    QString sessionId;
    QString requestId;
    QString role;
    QString rawText;
    QString status;
    QDateTime createdAt;
};
```

其中：

- `rawText` 对应服务端 `content`
- 历史消息加载后，assistant 消息同样走 Markdown 渲染

---

## 5. 左侧会话栏对接方式

## 5.1 初次加载

客户端打开聊天面板时，建议顺序如下：

1. 调 `GET /sessions`
2. 渲染左侧会话列表
3. 如果有最近会话：
   - 默认选中第一项
   - 再调 `GET /sessions/{session_id}/messages`
4. 如果没有会话：
   - 显示空状态
   - 等用户点击“新建会话”

## 5.2 列表显示建议

左侧每个会话项建议显示：

- 主标题：`title`
- 副标题：`lastPreview`
- 时间：`updatedAt`

### 排序建议

按 `updatedAt` 倒序。

服务端当前已经按这个顺序返回。

---

## 6. 新建会话流程

## 推荐方式

用户点击“新建会话”按钮时：

1. 客户端调用 `POST /sessions`
2. 拿到返回的 `sessionId`
3. 将新会话插入左侧列表顶部
4. 选中该会话
5. 清空右侧消息区
6. 之后建立：
   - `WS /ws/chat?session_id=<newSessionId>`

### 请求示例

```json
POST /sessions
{
  "title": "新建会话"
}
```

如果你不想自己传标题，也可以传空 body，由服务端使用默认标题。

### 推荐结论

建议客户端显式调用 `POST /sessions`，不要完全依赖 WebSocket 自动创建。

原因：

- 左侧 UI 更容易先拿到会话 id
- 会话创建时机更清晰
- 更容易做“新建会话后马上选中”

---

## 7. 自动标题机制

服务端当前有一个重要行为：

- 如果会话标题仍为默认值“新建会话”
- 且该会话收到首轮用户消息
- 服务端会自动把标题更新成首条问题的前 40 个字符

例如：

```text
解释一下C++的多态和虚函数机制
```

会自动成为会话标题。

### Qt 端建议

发送首条消息后，客户端不要假设标题不变。

建议在收到本轮 `assistant_final` 后：

1. 调一次 `GET /sessions`
2. 或调 `GET /sessions/{session_id}`
3. 用服务端最新标题刷新左侧会话栏

如果你们想减少请求次数，也可以在后续协议里加一个 `session_updated` 推送消息，但当前版本还没有。

---

## 8. 点击会话切换流程

当用户点击左侧某个会话项时，推荐流程如下：

1. 保存当前会话未提交输入
2. 断开当前 WebSocket
3. 调 `GET /sessions/{session_id}/messages`
4. 用返回的 `items` 重建右侧消息区
5. 连接：
   - `WS /ws/chat?session_id=<selectedSessionId>`
6. 收到 `session_ready`
7. 更新顶部当前会话信息

### 为什么要先 REST 拉历史，再连 WebSocket

原因：

- WebSocket 当前只负责当前轮聊天，不负责回放历史
- 历史消息由 REST 获取更稳定

---

## 9. 历史消息恢复显示

接口：

- `GET /sessions/{session_id}/messages`

返回格式：

```json
{
  "session": {
    "sessionId": "uuid",
    "title": "解释一下多态",
    "createdAt": "...",
    "updatedAt": "...",
    "lastPreview": "...",
    "messageCount": 2
  },
  "items": [
    {
      "messageId": "uuid",
      "sessionId": "uuid",
      "role": "user",
      "content": "多态是什么",
      "createdAt": "..."
    },
    {
      "messageId": "uuid",
      "sessionId": "uuid",
      "role": "assistant",
      "content": "## 核心原理\n\n```cpp\nclass Animal {};\n```",
      "createdAt": "..."
    }
  ]
}
```

Qt 端处理：

- 将每个 `content` 写入消息项的 `rawText`
- assistant 消息重新走 Markdown 解析与代码块渲染

注意：

- 历史消息不是纯文本
- 不要绕过你们现有的 Markdown 渲染逻辑

---

## 10. 搜索会话

接口：

- `GET /sessions?query=多态`

服务端当前匹配：

- `title`
- `lastPreview`

### Qt 端建议

左上搜索框输入时：

- 可做简单 debounce，例如 200ms 到 300ms
- 输入为空时恢复 `GET /sessions`
- 输入非空时请求带 `query`

---

## 11. 会话重命名

接口：

- `PATCH /sessions/{session_id}`

请求体：

```json
{
  "title": "C++ 多态讲解"
}
```

Qt 端建议：

1. 用户修改标题
2. 调用 `PATCH`
3. 用返回的最新会话对象更新左侧列表

---

## 12. 删除会话

接口：

- `DELETE /sessions/{session_id}`

返回：

```json
{
  "ok": true,
  "sessionId": "uuid"
}
```

Qt 端建议：

1. 用户确认删除
2. 调用 `DELETE`
3. 从左侧列表移除该项
4. 如果删除的是当前选中会话：
   - 清空右侧消息区
   - 自动选中下一个会话或进入空状态

---

## 13. 推荐 Qt 端类职责补充

在你们现有聊天结构基础上，建议新增一层 REST 数据访问类。

### 新增类建议

```cpp
class AgentSessionService : public QObject {
    Q_OBJECT
public:
    void fetchSessions(const QString& query = QString());
    void createSession(const QString& title = QString());
    void fetchSessionMessages(const QString& sessionId);
    void renameSession(const QString& sessionId, const QString& title);
    void deleteSession(const QString& sessionId);

signals:
    void sessionsLoaded(const QVector<ChatSessionItem>& sessions);
    void sessionCreated(const ChatSessionItem& session);
    void sessionMessagesLoaded(const QString& sessionId,
                               const QVector<ChatMessageItem>& messages);
    void sessionUpdated(const ChatSessionItem& session);
    void sessionDeleted(const QString& sessionId);
    void requestFailed(const QString& operation, const QString& errorMessage);
};
```

### 现有结构建议变成

```text
AgentChatViewModel
  ├─ AgentSessionService     // REST: 会话与历史
  ├─ AgentWebSocketClient    // WS: 当前会话聊天
  └─ AgentProcessManager     // sidecar 生命周期
```

---

## 14. 推荐初始化时序

```text
打开 AI 面板
-> 启动 Agent sidecar
-> 请求 GET /sessions
-> 渲染左侧会话列表
-> 选中最近会话
-> 请求 GET /sessions/{id}/messages
-> 渲染右侧历史消息
-> 连接 WS /ws/chat?session_id={id}
```

---

## 15. 推荐新建会话时序

```text
点击“新建会话”
-> POST /sessions
-> 左侧列表插入新会话
-> 右侧消息区清空
-> 连接新会话对应的 WebSocket
-> 收到 session_ready
-> 等待用户发送首条消息
-> 收到首轮 assistant_final 后刷新 session 信息
```

---

## 16. 推荐切换会话时序

```text
点击左侧会话项
-> 断开当前 WS
-> GET /sessions/{id}/messages
-> 渲染该会话历史消息
-> WS /ws/chat?session_id={id}
-> session_ready
-> 进入可聊天状态
```

---

## 17. 当前注意事项

### 注意 1：服务端存的是完整历史

服务端数据库保存完整消息历史，不会只保留最近 20 条。

但模型上下文仍只会取最近 `AGENT_MAX_HISTORY_MESSAGES` 条参与推理。

这意味着：

- 客户端可以放心展示完整历史
- 不会因为模型窗口裁剪而丢失会话内容

### 注意 2：WebSocket 不负责补发历史

当前版本：

- 历史消息必须通过 REST 拉取
- WebSocket 只负责当前会话实时聊天

### 注意 3：title 可能被服务端自动更新

尤其是默认新会话，在首轮消息后标题可能变化，客户端要记得刷新。

---

## 18. 一句话结论

Qt 端对接会话与消息存储时，正确方式是：

**用 REST 管左侧会话列表和历史消息，用 WebSocket 管当前选中会话的实时聊天，两者共同组成完整聊天体验。**

