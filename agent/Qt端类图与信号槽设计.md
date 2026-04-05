# Qt 端类图与信号槽设计

## 1. 文档目的

这份文档是对《Qt端对接最小聊天 Agent 建议》的进一步细化，目标是给 Qt 端开发一个可以直接落地的类设计和信号槽设计稿。

这份设计只覆盖：

- Qt 与 Python Agent sidecar 的最小聊天接入
- 启动、连接、消息发送、消息接收、错误处理
- 插件窗口或独立窗口中的聊天 UI

这份设计不覆盖：

- 工具调用
- 音乐业务 Host API
- 计划审批 UI
- 复杂消息渲染

---

## 2. 推荐类关系

建议 Qt 侧至少包含以下对象：

```text
AgentChatWidget / AgentChatPanel
        |
        v
AgentChatViewModel
        |
        +-------------------+
        |                   |
        v                   v
AgentProcessManager   AgentWebSocketClient
```

依赖方向建议固定为：

- UI 只依赖 `AgentChatViewModel`
- `AgentChatViewModel` 依赖：
  - `AgentProcessManager`
  - `AgentWebSocketClient`
- `AgentWebSocketClient` 不依赖 UI
- `AgentProcessManager` 不依赖 UI

这样可以保证：

- UI 修改不会影响通信层
- 后续把 Widget 换成 QML 时不用重写 Agent 通信逻辑
- ViewModel 可以单独做测试和联调

---

## 3. 类职责与建议接口

## 3.1 AgentProcessManager

### 职责

- 启动 Agent 进程
- 监控 Agent 是否运行
- 在必要时停止 Agent 进程
- 对外汇报启动结果和异常退出

### 建议头文件接口

```cpp
class AgentProcessManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

public:
    explicit AgentProcessManager(QObject* parent = nullptr);

    bool startAgent();
    void stopAgent();
    bool isRunning() const;
    qint64 processId() const;

    QString programPath() const;
    QString workingDirectory() const;

signals:
    void runningChanged();
    void started();
    void startFailed(const QString& reason);
    void exited(int exitCode, QProcess::ExitStatus exitStatus);
    void stdOutReceived(const QString& text);
    void stdErrReceived(const QString& text);
};
```

### 内部成员建议

```cpp
private:
    QProcess* m_process = nullptr;
    QString m_programPath;
    QString m_workingDirectory;
    bool m_running = false;
```

### 实现建议

- 启动命令优先尝试：

```text
agent/.venv/Scripts/music-agent-server.exe
```

- 如果没有该脚本，则回退为：

```text
agent/.venv/Scripts/python.exe -m music_agent.server
```

- `QProcess::setWorkingDirectory()` 指向 `agent/`
- 捕获标准输出和标准错误，转成日志信号发给上层
- 若进程在短时间内退出，触发 `startFailed`

---

## 3.2 AgentWebSocketClient

### 职责

- 建立与 Agent 的 WebSocket 连接
- 管理 `sessionId`
- 负责协议序列化与反序列化
- 对外只抛出结构化信号，不让 UI 直接解析 JSON

### 建议头文件接口

```cpp
class AgentWebSocketClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)

public:
    explicit AgentWebSocketClient(QObject* parent = nullptr);

    void connectToServer();
    void reconnect();
    void disconnectFromServer();

    bool isConnected() const;
    QString sessionId() const;

    void setSessionId(const QString& sessionId);
    void clearSession();

    void sendUserMessage(const QString& content, const QString& requestId);

signals:
    void connectionChanged();
    void sessionIdChanged();

    void connected();
    void disconnected();
    void sessionReady(const QString& sessionId);
    void assistantMessageReceived(const QString& requestId, const QString& content);
    void protocolError(const QString& code, const QString& message);
    void requestError(const QString& requestId, const QString& code, const QString& message);
};
```

### 内部成员建议

```cpp
private:
    QWebSocket m_socket;
    QString m_sessionId;
    bool m_connected = false;
    QUrl buildUrl() const;
    void handleTextMessage(const QString& text);
```

### URL 规则

- 无 session：

```text
ws://127.0.0.1:8765/ws/chat
```

- 有 session：

```text
ws://127.0.0.1:8765/ws/chat?session_id=<sessionId>
```

### JSON 处理建议

收到服务端 JSON 后，根据 `type` 分发：

- `session_ready`
- `assistant_final`
- `error`

不要让 UI 自己解析消息类型。

---

## 3.3 ChatMessageItem / ChatMessageListModel

### 推荐方案

如果 Qt 端是 Widget，可以先用简单 `QVector<ChatMessageItem>` + 手动刷新。

如果 Qt 端是 QML，建议直接上 `QAbstractListModel`，避免后面返工。

### ChatMessageItem 建议字段

```cpp
struct ChatMessageItem {
    QString id;
    QString role;          // user / assistant / system / error
    QString text;
    QString requestId;
    QDateTime timestamp;
    QString status;        // pending / done / error
};
```

### ChatMessageListModel 角色建议

- `id`
- `role`
- `text`
- `requestId`
- `timestamp`
- `status`

---

## 3.4 AgentChatViewModel

### 职责

- 聚合进程管理与 WebSocket 通信
- 给 UI 提供可直接消费的状态和消息
- 承担最小交互编排

### 建议头文件接口

```cpp
class AgentChatViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QString connectionText READ connectionText NOTIFY connectionTextChanged)
    Q_PROPERTY(QObject* messageModel READ messageModel CONSTANT)

public:
    explicit AgentChatViewModel(QObject* parent = nullptr);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void retryConnection();
    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void clearConversation();

    bool connected() const;
    bool starting() const;
    QString sessionId() const;
    QString connectionText() const;
    QObject* messageModel() const;

signals:
    void connectedChanged();
    void startingChanged();
    void sessionIdChanged();
    void connectionTextChanged();
    void toastRequested(const QString& text);
};
```

### 内部成员建议

```cpp
private:
    AgentProcessManager* m_processManager = nullptr;
    AgentWebSocketClient* m_socketClient = nullptr;
    ChatMessageListModel* m_messageModel = nullptr;

    bool m_connected = false;
    bool m_starting = false;
    int m_requestCounter = 0;
```

### 内部辅助方法建议

```cpp
private:
    QString nextRequestId();
    void appendUserMessage(const QString& requestId, const QString& text);
    void appendAssistantMessage(const QString& requestId, const QString& text);
    void appendErrorMessage(const QString& requestId, const QString& text);
    void wireSignals();
```

---

## 4. 信号槽连接建议

下面这套连接关系建议固定。

## 4.1 进程管理到 ViewModel

```cpp
connect(m_processManager, &AgentProcessManager::started,
        this, &AgentChatViewModel::onAgentStarted);

connect(m_processManager, &AgentProcessManager::startFailed,
        this, &AgentChatViewModel::onAgentStartFailed);

connect(m_processManager, &AgentProcessManager::exited,
        this, &AgentChatViewModel::onAgentExited);
```

### 处理逻辑

- `started`：
  - 更新状态为 connecting
  - 调用 `m_socketClient->connectToServer()`

- `startFailed`：
  - 状态切为 error
  - 触发 `toastRequested`

- `exited`：
  - 状态切为 disconnected
  - 如果聊天窗口仍打开，可显示“Agent 已退出”

## 4.2 WebSocket 到 ViewModel

```cpp
connect(m_socketClient, &AgentWebSocketClient::connected,
        this, &AgentChatViewModel::onSocketConnected);

connect(m_socketClient, &AgentWebSocketClient::disconnected,
        this, &AgentChatViewModel::onSocketDisconnected);

connect(m_socketClient, &AgentWebSocketClient::sessionReady,
        this, &AgentChatViewModel::onSessionReady);

connect(m_socketClient, &AgentWebSocketClient::assistantMessageReceived,
        this, &AgentChatViewModel::onAssistantMessageReceived);

connect(m_socketClient, &AgentWebSocketClient::requestError,
        this, &AgentChatViewModel::onRequestError);

connect(m_socketClient, &AgentWebSocketClient::protocolError,
        this, &AgentChatViewModel::onProtocolError);
```

### 处理逻辑

- `connected`：
  - 更新连接状态

- `disconnected`：
  - 更新连接状态
  - 若是异常断开，则提示用户

- `sessionReady`：
  - 保存 sessionId

- `assistantMessageReceived`：
  - 插入 assistant 消息

- `requestError`：
  - 插入 error 消息或 toast

- `protocolError`：
  - 记录日志并提示“协议错误”

---

## 5. 消息流设计

## 5.1 初始化时序

```text
UI 打开聊天窗口
  -> AgentChatViewModel.initialize()
      -> AgentProcessManager.startAgent()
          -> sidecar 进程启动
          -> AgentProcessManager.started()
      -> AgentWebSocketClient.connectToServer()
          -> WebSocket connected
          -> 收到 session_ready
          -> AgentChatViewModel 更新状态
```

## 5.2 发送消息时序

```text
用户点击发送
  -> AgentChatViewModel.sendMessage(text)
      -> 生成 requestId
      -> 插入 user 消息到消息列表
      -> AgentWebSocketClient.sendUserMessage()
          -> 发出 JSON:
             { type: "user_message", requestId, content }
          -> 收到 assistant_final
      -> AgentChatViewModel 插入 assistant 消息
```

## 5.3 出错时序

```text
Qt 发送 user_message
  -> Agent 返回 error
  -> AgentWebSocketClient 解析 error
  -> 发出 requestError(requestId, code, message)
  -> AgentChatViewModel 显示错误
```

---

## 6. 请求 ID 与消息关联建议

虽然当前后端是串行的一问一答，但 Qt 端仍然建议保留 `requestId`。

原因：

- 方便未来接流式输出
- 方便错误归属到具体消息
- 以后如果允许排队发送，不用重构

建议生成规则：

```text
req-1
req-2
req-3
```

或者：

```text
<timestamp>-<counter>
```

当前阶段简单递增即可。

---

## 7. 窗口与 UI 组件建议

## 7.1 如果走插件窗口

建议新增：

- `AgentChatPlugin`
- `AgentChatWidget`
- `AgentChatViewModel`

### AgentChatWidget 内部建议控件

- 顶部状态标签
- 中间消息列表
- 底部输入框
- 发送按钮
- 重试按钮

### Widget 与 ViewModel 关系

- Widget 不直接操作 `QWebSocket`
- Widget 只调用：
  - `viewModel->initialize()`
  - `viewModel->sendMessage()`
  - `viewModel->retryConnection()`

## 7.2 如果走 QML 面板

建议：

- ViewModel 通过 `QObject` 暴露到 QML
- 消息列表用 `ListView`
- 输入框用 `TextArea`
- 发送按钮调用 `sendMessage`

当前阶段两种 UI 都可行，但通信层应共用同一套 C++ 类。

---

## 8. 状态机建议

Qt 端最好维护一个简单连接状态机，不要只靠 bool。

建议状态：

```cpp
enum class AgentConnectionState {
    Idle,
    StartingProcess,
    ConnectingSocket,
    Ready,
    Error
};
```

### 状态含义

- `Idle`：未启动
- `StartingProcess`：正在拉起 sidecar
- `ConnectingSocket`：进程已启动，正在连 WebSocket
- `Ready`：已可聊天
- `Error`：启动失败、连接失败或协议错误

UI 顶部文案可以直接映射：

- `Idle` -> 未启动
- `StartingProcess` -> 正在启动 Agent
- `ConnectingSocket` -> 正在连接 Agent
- `Ready` -> 已连接
- `Error` -> 连接异常

---

## 9. 日志建议

Qt 对接阶段非常建议保留一层调试日志。

### 推荐记录内容

- Agent 启动命令
- Agent 进程 PID
- `healthz` 结果
- WebSocket 连接/断开事件
- 收到的消息类型
- 错误码和错误消息

### 推荐输出位置

- `qDebug()`
- 调试面板
- 插件日志窗口

日志足够清楚会极大降低联调成本。

---

## 10. 不建议的做法

以下做法不建议在当前阶段采用。

### 不建议 1：UI 直接持有 QProcess 和 QWebSocket

这样会导致：

- UI 代码臃肿
- 后续切换 UI 技术栈时难复用

### 不建议 2：Qt 端直接解析所有 JSON 细节到 Widget

这样协议一改，UI 全部跟着改。

### 不建议 3：先上复杂功能

包括：

- 流式逐字输出
- markdown 富文本
- 消息历史持久化
- 工具调用
- 计划编辑

这些都应该在“最小聊天链路稳定”之后再做。

---

## 11. 推荐最小开发顺序

### 第一步

实现：

- `AgentProcessManager`
- `AgentWebSocketClient`

目标：

- 能启动 sidecar
- 能收到 `session_ready`

### 第二步

实现：

- `AgentChatViewModel`
- 最小消息列表

目标：

- 能发消息
- 能收到回复

### 第三步

补充：

- 重连
- 错误提示
- 日志输出

目标：

- 具备可联调和可演示能力

---

## 12. 一句话结论

Qt 端最稳的落地方式是：

**用 `AgentProcessManager + AgentWebSocketClient + AgentChatViewModel` 三层结构，把 Python Agent 当作本地 sidecar 服务接进来，先跑通最小聊天链路，再扩展业务能力。**

