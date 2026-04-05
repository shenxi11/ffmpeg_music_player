# Qt 客户端 Markdown 与代码块渲染方案

## 1. 文档目的

这份文档用于指导 Qt 客户端正确展示 Agent 返回的带样式内容，尤其是：

- Markdown 段落
- 行内代码
- 多行代码块
- 带语言标记的 fenced code block
- 流式输出过程中不断增长的 Markdown 内容

当前目标不是做一个完整 Markdown 编辑器，而是做一个：

- 能原样保留服务端输出
- 能较稳定地渲染代码与富文本
- 能适应流式 chunk 更新
- 便于后续扩展语法高亮

---

## 2. 核心原则

客户端不要把服务端回复当作“普通纯文本”。

服务端返回的内容应被视为：

```text
原始 Markdown 源文本
```

因此客户端必须同时维护两层数据：

### 2.1 原始层

- `rawText`

含义：

- 完整保存服务端原始返回文本
- 不做清洗
- 不删除反引号
- 不删除换行
- 不重排缩进

### 2.2 渲染层

- `renderedBlocks`

含义：

- 把 `rawText` 解析为段落块、代码块、列表块等
- UI 根据块类型分别渲染

### 为什么一定要分两层

因为只有这样才能做到：

- 原文可追溯
- 流式更新不乱
- 渲染规则可迭代
- 代码块不会被 UI 二次破坏

---

## 3. 不建议的做法

以下做法不建议采用。

### 不建议 1：把所有内容直接塞进一个 `QLabel`

原因：

- 代码块格式容易坏
- 长文本支持差
- 语法高亮无从谈起

### 不建议 2：收到 chunk 就直接往屏幕字符串后面拼

原因：

- 半截 Markdown 标记会让界面状态不一致
- 半截代码块可能导致整段渲染错误

### 不建议 3：客户端自行“清洗” Markdown

包括：

- 去掉三反引号
- 合并多余换行
- 自动 trim 整体空白
- 把行内代码转普通文本

这会破坏服务端输出语义。

---

## 4. 推荐数据结构

建议每条 assistant 消息维护下面结构。

```cpp
struct ChatMessageItem {
    QString id;
    QString requestId;
    QString role;          // user / assistant / error
    QString rawText;       // 服务端原始 Markdown
    QString status;        // pending / streaming / done / error
    QVector<MessageBlock> blocks;
    QDateTime timestamp;
};
```

其中 `blocks` 为解析后的渲染块。

建议定义：

```cpp
struct MessageBlock {
    QString type;          // paragraph / code / list / quote / heading
    QString language;      // cpp / python / qml / json / bash / ...
    QString rawText;       // 当前块原始文本
};
```

说明：

- `rawText` 是块级文本，不是整条消息文本
- `language` 仅对 `code` 类型有意义

---

## 5. 推荐解析策略

第一阶段不需要完整支持全部 Markdown 语法，只需要优先支持最常见部分。

建议优先支持：

- 普通段落
- 行内代码
- fenced code block
- 标题
- 简单列表

## 5.1 代码块识别

核心格式：

````md
```cpp
class Foo {
public:
    void bar();
};
```
````

客户端应识别：

- 起始行是否为 ``` 开头
- 是否带语言标记
- 到下一个 ``` 为止都属于同一个代码块

解析后得到：

```cpp
type = "code"
language = "cpp"
rawText = "class Foo {\npublic:\n    void bar();\n};"
```

## 5.2 段落识别

不在代码块中的连续普通文本，可合并成一个 paragraph block。

## 5.3 行内代码

对于 paragraph block 内的：

```md
请检查 `QWebSocket` 的连接状态
```

建议在 paragraph 渲染器内部处理，不必单独拆成 block。

---

## 6. 流式输出时的处理方式

这是最关键的一部分。

服务端返回顺序为：

- `assistant_start`
- 多次 `assistant_chunk`
- `assistant_final`

### 6.1 收到 assistant_start

客户端应：

- 创建一条新的 assistant 消息
- `rawText = ""`
- `status = streaming`
- `blocks = []`

### 6.2 收到 assistant_chunk

客户端应：

1. 找到对应 `requestId` 的 assistant 消息
2. 将 `delta` 原样追加到 `rawText`
3. 重新解析整条消息的 `rawText`
4. 更新 `blocks`
5. 刷新当前消息 UI

注意：

- 不是只追加渲染结果
- 而是追加原始 Markdown，再整体重算 block

### 6.3 收到 assistant_final

客户端应：

1. 将 `rawText` 替换为 `content`
2. 再完整解析一次
3. `status = done`

这样可确保：

- chunk 阶段若有拼接误差，final 会统一校准
- 最终展示与服务端完整结果一致

### 6.4 收到 error

如果该消息已经有部分 chunk：

- 保留当前 `rawText`
- 保留已渲染内容
- `status = error`

不要清空当前消息。

---

## 7. UI 渲染建议

## 7.1 总体思路

不要试图用一个控件直接渲染整条复杂 Markdown。

建议做法是：

- 先按 block 拆分
- 不同 block 用不同 UI 组件

### 推荐映射

- `paragraph` -> 富文本组件
- `code` -> 独立代码块组件
- `heading` -> 加粗标题组件
- `list` -> 普通富文本组件或列表样式组件

---

## 8. 代码块组件建议

无论是 Widget 还是 QML，代码块都建议单独做一个组件。

### 代码块组件建议显示内容

- 语言标签，例如 `cpp`、`python`
- 代码正文
- 深色或浅色背景
- 等宽字体
- 可选复制按钮

### 推荐样式

- 背景色与普通文本区分明显
- 使用等宽字体
- 保留原始缩进
- 自动换行可先关闭，优先保格式

### Widget 方案

建议使用：

- `QPlainTextEdit` 只读模式
- 或自定义代码视图控件

后续语法高亮可叠加：

- `QSyntaxHighlighter`

### QML 方案

第一版可以先做：

- 带背景的 `TextArea` / `TextEdit` 只读组件
- 等宽字体
- 语言标签

真正语法高亮可后置。

---

## 9. 富文本部分建议

普通 paragraph block 可使用 Markdown 或富文本渲染。

但要注意：

- paragraph 与 code block 必须分开渲染
- 不要让 paragraph 组件吞掉 fenced code block

如果使用 `QTextDocument`：

- 可以处理普通 Markdown 和行内代码
- 但代码块高亮能力有限

所以推荐：

- paragraph 用富文本
- code block 单独组件

---

## 10. 语法高亮策略建议

当前阶段建议分两步。

### 第一步：先做“块级高亮”

即：

- 代码块有专门背景
- 显示语言标签
- 使用等宽字体

这样即使没有真正 token 级高亮，也已经比纯文本好很多。

### 第二步：再做“语言级高亮”

根据 `language` 选择高亮器：

- `cpp`
- `python`
- `qml`
- `json`
- `bash`
- `yaml`

如果语言不认识：

- 仍按普通代码块展示
- 不要丢弃语言标签

---

## 11. 解析器建议

客户端最好单独封一个解析器，不要把 Markdown 拆分逻辑写在 UI 组件里。

建议类名：

```cpp
class MarkdownMessageParser {
public:
    QVector<MessageBlock> parse(const QString& rawText) const;
};
```

这样好处是：

- 可单独测试
- 可反复迭代
- UI 与解析逻辑解耦

### 第一版解析器能力

建议先支持：

- fenced code block
- paragraph
- heading
- list

### 暂时可不支持

- 表格
- Mermaid
- HTML 混排
- 嵌套复杂引用

---

## 12. 推荐时序

```text
assistant_start
-> 创建空 assistant 消息

assistant_chunk
-> rawText += delta
-> parse(rawText)
-> update blocks
-> refresh UI

assistant_final
-> rawText = content
-> parse(rawText)
-> update blocks
-> status = done
```

---

## 13. 推荐开发顺序

### 第一步

实现：

- `rawText`
- `blocks`
- `MarkdownMessageParser`

目标：

- 能把普通段落和代码块分开

### 第二步

实现：

- 代码块独立组件
- paragraph 富文本组件

目标：

- 能稳定显示 Markdown 和代码

### 第三步

实现：

- 流式实时重解析
- final 校准
- error 状态保留

目标：

- 流式输出体验稳定

### 第四步

实现：

- 真正语法高亮
- 复制按钮
- 代码折叠等增强功能

---

## 14. 一句话结论

客户端想“原封原样输出带样式结果”，正确做法不是直接显示字符串，而是：

**把服务端回复当作原始 Markdown 源文本保存下来，再按 block 解析为普通段落和代码块分别渲染，流式更新时始终基于完整 `rawText` 重新解析。**

