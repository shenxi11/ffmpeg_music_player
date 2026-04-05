# Qt 端流式输出对接建议

## 1. 目标

这份文档用于指导 Qt 客户端在现有最小聊天接入基础上增加流式输出显示能力。

目标效果：

- 用户发送消息后，不再等待整段回复完成
- Agent 开始生成时，界面立即创建一条 assistant 消息
- 随着服务端持续推送 chunk，UI 实时追加文本
- 回复结束后，UI 将消息状态标记为完成

---

## 2. 服务端消息顺序

Qt 端发出：

```json
{
  "type": "user_message",
  "requestId": "req-1",
  "content": "你好"
}
```

服务端现在会按顺序返回：

### 2.1 assistant_start

```json
{
  "type": "assistant_start",
  "sessionId": "uuid",
  "requestId": "req-1"
}
```

### 2.2 assistant_chunk

```json
{
  "type": "assistant_chunk",
  "sessionId": "uuid",
  "requestId": "req-1",
  "delta": "你好，"
}
```

可能会收到多条。

### 2.3 assistant_final

```json
{
  "type": "assistant_final",
  "sessionId": "uuid",
  "requestId": "req-1",
  "content": "你好，我是一个助手。"
}
```

### 2.4 error

如果生成中断：

```json
{
  "type": "error",
  "sessionId": "uuid",
  "requestId": "req-1",
  "code": "stream_interrupted",
  "message": "stream failed"
}
```

---

## 3. Qt 端消息模型建议

建议在消息模型中增加两类状态字段：

```cpp
struct ChatMessageItem {
    QString id;
    QString role;        // user / assistant / system / error
    QString requestId;
    QString text;
    QString status;      // pending / streaming / done / error
    QDateTime timestamp;
};
```

关键点：

- `requestId` 用来把 chunk 定位到正确消息
- `status` 用来显示“生成中”或“生成失败”

---

## 4. ViewModel 处理规则

## 4.1 发送消息

当用户发送时：

1. 生成 `requestId`
2. 插入一条 user 消息
3. 调用 `sendUserMessage(content, requestId)`

## 4.2 收到 assistant_start

处理方式：

1. 新建一条 assistant 消息
2. `requestId` 设为当前请求 id
3. `text` 初始为空
4. `status` 设为 `streaming`

建议提供方法：

```cpp
void beginAssistantMessage(const QString& requestId);
```

## 4.3 收到 assistant_chunk

处理方式：

1. 根据 `requestId` 找到当前 assistant 消息
2. 将 `delta` 追加到 `text`
3. 通知 UI 刷新

建议提供方法：

```cpp
void appendAssistantChunk(const QString& requestId, const QString& delta);
```

## 4.4 收到 assistant_final

处理方式：

1. 根据 `requestId` 找到当前 assistant 消息
2. 将最终内容校准为 `content`
3. `status` 设为 `done`

建议提供方法：

```cpp
void finalizeAssistantMessage(const QString& requestId, const QString& content);
```

### 为什么还要用 final 覆盖一次

原因：

- 避免 chunk 拼接误差
- 以后如果服务端做内容修正，客户端能以 final 为准

## 4.5 收到 error

如果该 `requestId` 已经创建了 assistant 消息：

- 保留当前已拼出的文本
- 把这条 assistant 消息状态设为 `error`
- 可以额外显示错误提示

如果该 `requestId` 还没有 assistant 消息：

- 显示 toast 或错误气泡

建议提供方法：

```cpp
void markAssistantMessageError(const QString& requestId, const QString& message);
```

---

## 5. AgentWebSocketClient 建议新增信号

在原有信号基础上新增：

```cpp
signals:
    void assistantStartReceived(const QString& requestId);
    void assistantChunkReceived(const QString& requestId, const QString& delta);
    void assistantMessageReceived(const QString& requestId, const QString& content); // final
```

说明：

- `assistantMessageReceived` 现在语义上代表 final
- 如果觉得命名不够清晰，可以改成 `assistantFinalReceived`

---

## 6. AgentChatViewModel 建议新增槽函数

```cpp
private slots:
    void onAssistantStartReceived(const QString& requestId);
    void onAssistantChunkReceived(const QString& requestId, const QString& delta);
    void onAssistantFinalReceived(const QString& requestId, const QString& content);
    void onRequestError(const QString& requestId, const QString& code, const QString& message);
```

推荐逻辑：

- `onAssistantStartReceived` -> 创建空 assistant 消息
- `onAssistantChunkReceived` -> 追加文本
- `onAssistantFinalReceived` -> 设为 done
- `onRequestError` -> 设为 error 或 toast

---

## 7. UI 展示建议

第一版流式 UI 不需要复杂动画，重点是明确状态。

建议：

- assistant 消息在 `streaming` 状态时显示“正在输入”样式
- 可以在消息尾部加一个闪烁光标
- `done` 后去掉光标
- `error` 时显示一个小错误标记

如果是 QML，可根据 `status` 控制：

- 文本颜色
- 尾部光标
- 错误标签

---

## 8. 一条消息的完整生命周期

```text
用户发送消息
-> 本地插入 user 消息
-> 收到 assistant_start
-> 本地创建空 assistant 消息，status=streaming
-> 收到多个 assistant_chunk
-> 每次把 delta 追加到 assistant.text
-> 收到 assistant_final
-> assistant.text = final.content
-> status=done
```

如果中途出错：

```text
收到 assistant_start
-> 收到若干 assistant_chunk
-> 收到 error
-> 保留已有文本
-> status=error
```

---

## 9. 推荐最小实现顺序

### 第一步

让 `AgentWebSocketClient` 能识别：

- `assistant_start`
- `assistant_chunk`
- `assistant_final`

### 第二步

让 `AgentChatViewModel` 支持：

- 创建空 assistant 消息
- 根据 `requestId` 追加 chunk

### 第三步

在 UI 上补：

- streaming 状态
- error 状态

---

## 10. 一句话结论

Qt 端流式接入的关键不是改发送逻辑，而是：

**把同一个 `requestId` 的 `assistant_start`、`assistant_chunk`、`assistant_final` 视为同一条 assistant 消息的生命周期，并在 UI 中逐步更新它。**

