# Qt 端对接最小聊天 Agent 建议

## 1. 文档目的

这份文档用于指导 Qt 客户端接入当前已经实现的最小聊天 Agent 后端。

当前目标不是让 Agent 控制音乐软件业务，而是先完成：

- Qt 软件内展示聊天入口
- Qt 侧可启动并连接 Python Agent
- 用户可以在 Qt 内和 Agent 进行多轮聊天
- 为后续接入 Host API、工具调用、计划预览保留扩展空间

当前阶段暂不涉及：

- 播放控制工具调用
- 歌单管理工具调用
- 计划生成与审批 UI
- RAG
- 复杂业务编排

---

## 2. 推荐总体架构

推荐继续使用 sidecar 方案，不把 Python Agent 嵌入 Qt 主进程。

架构如下：

```text
Qt Client
  ├─ Chat UI
  ├─ AgentChatViewModel
  ├─ AgentWebSocketClient
  └─ AgentProcessManager

Python Agent Sidecar
  ├─ FastAPI
  ├─ WebSocket /ws/chat
  ├─ LangGraph ChatAgent
  └─ OpenAI-compatible LLM client
```

职责边界：

- Qt 负责界面、输入、消息展示、连接状态、启动 sidecar
- Python Agent 负责多轮上下文、模型调用、聊天回复
- 双方通过 `WebSocket + JSON` 通信

这样做的优点：

- Qt/C++ 与 Python 运行时解耦
- Agent 崩溃不会拖死主程序
- 便于单独调试和升级模型
- 后续扩展到工具调用时协议可直接演进

---

## 3. Qt 侧模块拆分建议

建议 Qt 端至少拆成下面三个类，不要把进程管理、WebSocket 和界面逻辑写在一个类里。

### 3.1 AgentProcessManager

职责：

- 使用 `QProcess` 启动 Python Agent 进程
- 检查进程是否已启动
- 捕获启动失败和异常退出
- 在主程序退出时清理 Agent 进程

建议接口：

```cpp
class AgentProcessManager : public QObject {
    Q_OBJECT
public:
    bool startAgent();
    void stopAgent();
    bool isRunning() const;
    qint64 processId() const;

signals:
    void started();
    void startFailed(const QString& reason);
    void exited(int exitCode, QProcess::ExitStatus exitStatus);
};
```

建议实现细节：

- 优先从固定路径启动，例如：
  - `agent/.venv/Scripts/music-agent-server.exe`
  - 或 `agent/.venv/Scripts/python.exe -m music_agent.server`
- 工作目录设为 `agent/`
- 等待 1 到 3 秒后轮询 `http://127.0.0.1:8765/healthz`
- 只有 `healthz` 可访问后再认为启动成功

### 3.2 AgentWebSocketClient

职责：

- 使用 `QWebSocket` 连接 `ws://127.0.0.1:8765/ws/chat`
- 发送 `user_message`
- 接收 `session_ready`、`assistant_final`、`error`
- 管理当前 `sessionId`
- 处理断线和重连

建议接口：

```cpp
class AgentWebSocketClient : public QObject {
    Q_OBJECT
public:
    void connectToAgent();
    void disconnectFromAgent();
    bool isConnected() const;

    void sendUserMessage(const QString& content,
                         const QString& requestId = QString());

    QString sessionId() const;

signals:
    void connectionStateChanged(bool connected);
    void sessionReady(const QString& sessionId);
    void assistantMessageReceived(const QString& requestId, const QString& content);
    void errorReceived(const QString& requestId,
                       const QString& code,
                       const QString& message);
};
```

建议实现细节：

- 首次连接地址：
  - `ws://127.0.0.1:8765/ws/chat`
- 如果已有 `sessionId`，重连时使用：
  - `ws://127.0.0.1:8765/ws/chat?session_id=<sessionId>`
- 每条出站消息都带 `requestId`
- 收到 `session_ready` 后保存 `sessionId`

### 3.3 AgentChatViewModel

职责：

- 给 UI 暴露消息列表
- 管理“发送中 / 已连接 / 出错”等状态
- 接收输入并调用 `AgentWebSocketClient`
- 把协议消息转换成 UI 可以直接显示的数据

建议接口：

```cpp
class AgentChatViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)
public:
    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void retryConnection();

    bool connected() const;
    QString sessionId() const;

signals:
    void connectedChanged();
    void sessionIdChanged();
    void messagesChanged();
    void toastRequested(const QString& text);
};
```

建议消息项结构：

```cpp
struct ChatMessageItem {
    QString id;
    QString role;       // "user" or "assistant" or "system"
    QString text;
    QDateTime timestamp;
    QString status;     // "sent", "error"
};
```

---

## 4. UI 入口建议

当前仓库已经有插件体系，所以接入方式有两条。

### 方案 A：先做插件窗口

适合第一阶段快速联调。

优点：

- 不改主页面结构
- 复用现有插件加载机制
- 出问题时隔离性更好

建议做法：

- 新增一个 `agent_chat_plugin`
- 插件窗口中放一个最小聊天界面
- 菜单点击后打开聊天窗口

适配你现有工程时，可参考现有插件链路：

- `src/app/plugin_interface.h`
- `src/app/plugin_manager.cpp`
- `src/app/main_widget.menu_auth.cpp`

### 方案 B：做主界面内置聊天面板

适合后续正式产品化。

优点：

- 用户体验更统一
- 后续更容易和播放页、歌单页联动

缺点：

- 需要改主界面布局
- 初期联调成本更高

### 推荐结论

第一阶段建议先走方案 A，也就是“插件窗口版聊天 UI”。

---

## 5. 当前后端协议

Qt 端第一版只需要支持四种消息。

### 5.1 服务端连接成功

```json
{
  "type": "session_ready",
  "sessionId": "uuid"
}
```

Qt 侧处理：

- 保存 `sessionId`
- 更新连接状态为已连接

### 5.2 Qt 发送用户消息

```json
{
  "type": "user_message",
  "requestId": "req-1",
  "content": "你好"
}
```

Qt 侧处理：

- 本地先插入一条用户消息
- 记录 `requestId`

### 5.3 收到 Agent 回复

```json
{
  "type": "assistant_final",
  "sessionId": "uuid",
  "requestId": "req-1",
  "content": "你好，我是一个助手。"
}
```

Qt 侧处理：

- 插入一条 assistant 消息
- 如果需要，可根据 `requestId` 与本地发送记录关联

### 5.4 收到错误

```json
{
  "type": "error",
  "sessionId": "uuid",
  "requestId": "req-1",
  "code": "model_error",
  "message": "..."
}
```

Qt 侧处理：

- 显示错误提示
- 保留用户输入，不要直接丢掉

---

## 6. 推荐连接流程

建议 Qt 端按下面顺序工作。

### 6.1 打开聊天入口时

1. `AgentProcessManager` 检查 Agent 是否在运行
2. 若未运行，则启动 sidecar
3. 轮询 `/healthz`
4. `AgentWebSocketClient` 连接 WebSocket
5. 收到 `session_ready`
6. UI 显示可聊天状态

### 6.2 用户发送消息时

1. `AgentChatViewModel` 生成本地 `requestId`
2. 本地列表插入用户消息
3. 调用 `sendUserMessage`
4. 等待 `assistant_final`
5. 插入 AI 回复

### 6.3 断线时

1. UI 显示“连接中断”
2. 保留已有消息列表
3. 尝试重连
4. 若已有 `sessionId`，重连时复用

---

## 7. Qt 端第一版 UI 建议

第一版界面不需要复杂设计，重点是把链路跑通。

建议包含：

- 消息列表区
- 输入框
- 发送按钮
- 顶部连接状态提示
- 错误提示区域

建议状态最少包含：

- `Starting Agent`
- `Connecting`
- `Connected`
- `Error`

建议先不做：

- markdown 渲染
- 富文本消息
- 流式逐字输出
- 历史会话列表

---

## 8. 为什么当前阶段不要直接接业务能力

现在不要让 Qt 侧一开始就暴露 `playTrack`、`createPlaylist` 这些接口给 Agent。

原因：

- 当前先验证“聊天链路是否稳定”
- 如果一开始就叠加业务工具，问题边界会混在一起
- 聊天稳定后，再做 Host API 接入会更容易定位问题

当前阶段的目标非常单纯：

```text
Qt 能把消息发给 Agent
Agent 能返回回复
会话可以持续
错误可以正确显示
```

只要这一步稳定，后续扩展就会非常顺。

---

## 9. 第二阶段演进方向

等最小聊天链路稳定后，再开始进入真正的“音乐软件 Agent”阶段。

推荐顺序：

1. Qt 暴露最小 Host API
2. 协议新增 `tool_call` / `tool_result`
3. Agent 接入工具调用
4. 增加结构化计划和审批

届时建议暴露的第一批业务接口：

- `searchTracks`
- `getCurrentTrack`
- `playTrack`
- `playPlaylist`
- `createPlaylist`

重要原则：

- Agent 不直接操作 UI
- Agent 只通过 Host API 执行业务动作

---

## 10. Qt 端落地优先级

建议严格按下面顺序开发，不要跳步。

### 第一优先级

- `AgentProcessManager`
- `AgentWebSocketClient`
- 最小聊天窗口

### 第二优先级

- `AgentChatViewModel`
- 消息列表模型
- 错误与重连体验

### 第三优先级

- 会话复用
- 启动状态提示
- 日志面板或调试输出

### 暂缓事项

- 工具调用
- 计划审批
- 与播放页联动
- 复杂 UI 设计

---

## 11. 联调检查清单

Qt 端开发时，建议按下面清单逐项确认。

### 基础启动

- 能拉起 Python sidecar
- `/healthz` 可访问
- WebSocket 能连接成功

### 协议收发

- 收到 `session_ready`
- 能发送 `user_message`
- 能收到 `assistant_final`
- 能收到 `error`

### 会话能力

- 同一次窗口使用中可多轮聊天
- 断线重连后可复用 `sessionId`

### 用户体验

- 连接失败时有明确提示
- 发送失败时不丢失输入
- sidecar 未启动时可自动启动

---

## 12. 一句话结论

当前最合理的做法是：

**先在 Qt 里接一个最小聊天入口，使用 `QProcess + QWebSocket` 对接 Python Agent sidecar，把聊天链路跑通，再进入业务工具和音乐控制阶段。**

