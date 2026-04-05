# 客户端脚本化改造总方案（指导Agent端）

## 1. 文档目标
这份文档不是普通 UI 说明，而是给 Agent 服务端和后续自动化执行链使用的。

它要回答的问题是：
- 客户端为什么需要脚本化
- 当前已经完成到哪一步
- Agent 端后续应该如何配合改造
- 脚本应该长什么样
- 执行器当前能做什么、不能做什么

目标是把客户端从“工具逐条调用”逐步升级成：

> Agent 生成受控脚本  
> 客户端脚本执行器顺序执行脚本  
> 每一步调用统一业务能力入口  
> 再把步骤结果和最终结果回传给 Agent

---

## 2. 当前问题背景

### 2.1 现有工具模式的优点
当前 Qt 客户端已经通过以下链路支持 Agent 调用：
- `AgentWebSocketClient`
- `AgentChatViewModel`
- `AgentToolExecutor`
- `HostStateProvider`
- `MainShellViewModel`
- `AudioService`

这套方案已经能支持大量查询类和控制类动作。

### 2.2 现有工具模式的不足
随着 Agent 开始承担更多多步任务，当前模式暴露出几个结构性问题：

1. 多步任务需要服务端自己维护完整执行上下文  
2. 客户端对 `trackId`、`playlistId`、搜索结果、歌单详情等对象的缓存仍偏局部  
3. Agent 若只做“逐工具调用”，很难稳定表达一段连续任务  
4. 服务端和客户端都要为复杂任务拼装链路，责任边界不清晰  

典型问题就是：
- 服务端恢复旧会话后只保留了 `trackId`
- 客户端新会话里没有对应缓存
- 直接 `playTrack(trackId)` 就失败

因此，需要让 Agent 端把任务组织成脚本，再交给客户端顺序执行。

---

## 3. 目标架构

### 3.1 目标形态
建议逐步形成以下结构：

1. **能力层**
   - 由现有 `AgentToolExecutor` 和后续统一 `CapabilityFacade` 承担
   - 负责真正调用客户端业务能力

2. **脚本执行层**
   - 新增 `AgentScriptExecutor`
   - 负责脚本解析、校验、顺序执行、上下文保存、结果聚合

3. **协议适配层**
   - 由 `AgentChatViewModel` / 协议路由层承接
   - 后续支持服务端直接发 `execute_script`

4. **Agent 规划层**
   - 由服务端 Agent 决定何时直接用单工具，何时生成脚本
   - 负责脚本生成、审批策略、失败后再规划

### 3.2 核心原则
- 客户端执行的是**受控 DSL**，不是任意 Python/Lua。
- 脚本只能调用白名单动作，不能直接访问 Qt 对象。
- 脚本执行器只负责编排，不直接持有业务逻辑。
- 真正的业务动作仍然通过客户端统一能力入口完成。

---

## 4. 当前已完成的客户端改造

### 4.1 已落地模块
- `src/agent/script/AgentScriptExecutor.h`
- `src/agent/script/AgentScriptExecutor.cpp`

### 4.2 已落地能力
当前脚本执行器已支持：
- JSON 脚本解析
- `version=1` 脚本校验
- 顺序步骤执行
- `saveAs` 保存中间结果
- `$last.xxx` 引用
- `$steps.<alias>.xxx` 引用
- 复用现有 `AgentToolExecutor::executeToolCall(...)`
- 聚合每一步结果并形成最终执行报告

### 4.3 当前接入点
已在：
- `src/agent/AgentChatViewModel.h`
- `src/agent/AgentChatViewModel.cpp`

新增接口：
- `validateClientScript(...)`
- `executeClientScript(...)`

当前这些接口主要用于：
- 客户端内部调试
- 后续协议层接入
- 后续 UI 或测试侧接入

### 4.4 当前已完成的协议接入
客户端第二阶段已经接入正式脚本协议入口。

#### 当前支持的入站消息
- `validate_script`
- `execute_script`

#### 当前支持的出站消息
- `script_validation_result`
- `script_execution_started`
- `script_step_event`
- `script_execution_result`

#### 相关实现位置
- `src/agent/protocol/AgentProtocolRouter.cpp`
- `src/agent/AgentWebSocketClient.cpp`
- `src/agent/AgentChatViewModel.cpp`

当前服务端已经可以按这组消息类型与客户端联调脚本执行链。

---

## 5. 当前脚本 DSL 设计

### 5.1 基本结构
```json
{
  "version": 1,
  "title": "把最近播放加入歌单",
  "steps": [
    {
      "id": "recent",
      "action": "getRecentTracks",
      "args": {
        "limit": 5
      },
      "saveAs": "recent_tracks"
    },
    {
      "id": "playlists",
      "action": "getPlaylists",
      "saveAs": "playlists"
    }
  ]
}
```

### 5.2 字段定义
- `version`
  - 当前固定为 `1`
- `title`
  - 可选，用于显示和审计
- `timeoutMs`
  - 可选，脚本超时时间，单位毫秒
  - 当前默认 `60000`
  - 当前允许范围 `1 ~ 300000`
- `steps`
  - 步骤数组，至少 1 项
- `steps[].id`
  - 可选，脚本执行器会兜底生成 `step_N`
- `steps[].action`
  - 必填，对应客户端已注册的工具名
- `steps[].args`
  - 可选，动作参数
- `steps[].saveAs`
  - 可选，把当前步骤结果保存成命名结果，供后续步骤引用

### 5.3 当前支持的变量引用
- `$last.xxx`
  - 引用上一步的 `result`
- `$steps.alias.xxx`
  - 引用前面 `saveAs=alias` 的步骤结果

示例：
```json
{
  "action": "playTrack",
  "args": {
    "trackId": "$steps.search.items.0.trackId"
  }
}
```

### 5.4 当前不支持
- 条件分支
- 循环
- 并发步骤
- 字符串模板拼接
- 脚本内函数
- 本地文件/Qt 对象直接调用
- 回滚和事务语义

---

## 6. Agent 端应如何配合改造

### 6.1 第一阶段建议
当前客户端已经具备脚本执行器骨架，Agent 端下一步建议做：

1. 在服务端新增脚本规划模式
2. 对适合多步任务的用户请求，优先生成脚本
3. 暂时仍然通过旧工具链路做单步执行
4. 等客户端协议接入完成后，再切换到 `execute_script`

### 6.2 适合优先脚本化的任务
优先推荐脚本化这几类任务：

1. 搜索歌曲 -> 选择候选 -> 播放
2. 查询歌单 -> 找到目标歌单 -> 查看内容
3. 最近播放 -> 过滤歌曲 -> 加入歌单
4. 搜索歌曲 -> 加入队列 -> 切到指定索引播放

### 6.3 暂不建议第一批脚本化的任务
- 涉及复杂审批的高风险写操作
- 插件管理类动作
- 设置批量变更
- 视频窗口复杂控制
- 需要事务回滚的跨域操作

---

## 7. 推荐的服务端生成策略

### 7.1 单步任务
如果用户请求本质是一个原子动作，继续走单工具即可，例如：
- 暂停播放
- 下一首
- 查询当前播放
- 查询最近播放

### 7.2 多步任务
如果用户请求包含“先查再做”的链路，建议生成脚本，例如：
- “播放周杰伦最热门的一首歌”
- “把最近播放里三首周杰伦的歌加到流行歌单”
- “找到我喜欢的歌里时长最长的一首并播放”

### 7.3 当前推荐的最小脚本链

#### 场景：搜索并播放
```json
{
  "version": 1,
  "title": "搜索并播放周杰伦",
  "steps": [
    {
      "id": "search",
      "action": "searchTracks",
      "args": {
        "keyword": "周杰伦",
        "limit": 5
      },
      "saveAs": "search"
    },
    {
      "id": "play",
      "action": "playTrack",
      "args": {
        "trackId": "$steps.search.items.0.trackId"
      }
    }
  ]
}
```

#### 场景：查歌单并查看内容
```json
{
  "version": 1,
  "title": "查看流行歌单内容",
  "steps": [
    {
      "id": "playlists",
      "action": "getPlaylists",
      "saveAs": "playlists"
    },
    {
      "id": "tracks",
      "action": "getPlaylistTracks",
      "args": {
        "playlistId": "$steps.playlists.items.0.playlistId"
      }
    }
  ]
}
```

---

## 8. 当前协议建议

### 8.1 validate_script
建议服务端发：

```json
{
  "type": "validate_script",
  "requestId": "req-1",
  "script": {
    "version": 1,
    "title": "测试脚本",
    "steps": [
      {
        "action": "getCurrentTrack"
      }
    ]
  }
}
```

客户端返回：
- `script_validation_result`

### 8.2 execute_script
建议服务端发：

```json
{
  "type": "execute_script",
  "requestId": "req-2",
  "script": {
    "version": 1,
    "title": "搜索并播放",
    "steps": [
      {
        "id": "search",
        "action": "searchTracks",
        "args": {
          "keyword": "周杰伦",
          "limit": 5
        },
        "saveAs": "search"
      },
      {
        "id": "play",
        "action": "playTrack",
        "args": {
          "trackId": "$steps.search.items.0.trackId"
        }
      }
    ]
  }
}
```

客户端会依次返回：
- `script_execution_started`
- `script_step_event`
- `script_step_event`
- `script_execution_result`

### 8.2.1 dry_run_script
建议服务端发：
```json
{
  "type": "dry_run_script",
  "requestId": "req-dry-run-1",
  "script": {
    "version": 1,
    "title": "搜索并播放（预演）",
    "timeoutMs": 30000,
    "steps": [
      {
        "id": "search",
        "action": "searchTracks",
        "args": {
          "keyword": "周杰伦",
          "limit": 5
        },
        "saveAs": "search"
      },
      {
        "id": "play",
        "action": "playTrack",
        "args": {
          "trackId": "$steps.search.items.0.trackId"
        }
      }
    ]
  }
}
```

客户端返回：
- `script_dry_run_result`

当前 dry-run 语义：
- 不真正执行步骤
- 但会完整复用客户端脚本校验逻辑
- 返回脚本是否可执行、整体风险、步骤策略、聚合领域元数据

### 8.3 当前联调注意事项
1. `requestId` 必须提供
2. 当前仅支持 `version=1`
3. 当前只支持顺序步骤，不支持条件和循环
4. 脚本中的 `action` 必须是客户端当前真实已支持的工具名
5. 如果脚本整体 `requiresApproval=true`，服务端需要显式带 `approved=true` 才能执行
6. 如无特殊需求，建议服务端显式带 `timeoutMs`

### 8.4 当前脚本校验结果新增字段
客户端当前在脚本校验结果中会返回：
- `requiresApproval`
- `containsWrite`
- `riskLevel`
- `autoExecutable`
- `domains`
- `mutationKinds`
- `targetKinds`

每个步骤的 `steps[].policy` 中也会带：
- `readOnly`
- `requireApproval`
- `riskLevel`
- `domain`
- `mutationKind`
- `targetKind`

服务端可以直接基于这组字段做：
- 自动执行判断
- 风险分级
- 审批流分支
- 计划器策略调整
- 领域级审批
- fallback/replan 分流

### 8.5 cancel_script
建议服务端发：

```json
{
  "type": "cancel_script",
  "requestId": "req-cancel-1",
  "executionId": "script_exec_3",
  "reason": "用户主动取消"
}
```

客户端返回：
- `script_cancellation_result`

同时，原始执行请求对应的：
- `script_execution_result`

也会以失败结束，并带：
- `code=script_cancelled`

### 8.6 当前执行报告新增审计字段
当前客户端在脚本执行生命周期中会回传这些关键字段：
- `startedAt`
- `finishedAt`
- `durationMs`
- `status`

其中：
- `script_execution_started.summary` 中可拿到：
  - `startedAt`
  - `timeoutMs`
- `script_execution_result.report` 中可拿到：
  - `status`
  - `startedAt`
  - `finishedAt`
  - `durationMs`

---

## 9. 后续阶段建议

### 阶段 6：统一能力入口第一刀
当前客户端已新增：
- `src/agent/capability/AgentCapabilityFacade.h`
- `src/agent/capability/AgentCapabilityFacade.cpp`

当前状态不是完整统一能力内核，但已经完成第一刀收束：
- `tool_call` 链路先走 `AgentCapabilityFacade`
- `AgentScriptExecutor` 的脚本步骤执行也先走 `AgentCapabilityFacade`
- `AgentToolExecutor` 下沉到 façade 背后，暂时继续承担底层实际执行

这一阶段的意义在于：
- 服务端不需要变协议
- 但客户端已经有了后续继续统一能力入口的正式承接层
- 后续 GUI 如果要脚本化或命令化，也有可复用的入口层

### 阶段 7：统一能力内核继续下沉
后续建议逐步把更多业务逻辑从 `AgentToolExecutor` 迁移到：
- `CapabilityFacade`
- 更细分的领域服务

目标是让：
- GUI
- Agent
- 脚本

最终共用同一套业务能力入口，而不是长期维持“façade 外壳 + 巨型工具执行器”的形态。

### 阶段 8：策略和审批增强
后续需要补：
- 脚本审批
- 高风险动作分级
- 写操作限流
- 超时与取消
- 执行审计
- 更细粒度审批域
- 失败后的 fallback/replan

---

## 10. Agent 端改造建议清单

### 10.1 近期建议
1. 新增脚本规划模式，不再只盯着单工具调用
2. 为多步任务输出结构化脚本
3. 脚本里优先使用已稳定可用的工具名
4. 不要假设客户端已经支持复杂 DSL

### 10.2 必须遵守的当前边界
1. 当前客户端只支持顺序步骤
2. 当前变量引用只支持 `$last` 和 `$steps.alias`
3. 当前协议层已经支持脚本入口，但能力模型仍然是第一版
4. 当前“脚本执行”能力不代表已经支持完整事务和回滚
5. 当前高风险脚本默认不会执行，除非服务端请求显式携带 `approved=true`
6. 当前取消是“执行器级取消”，不会回滚已经完成的步骤

### 10.3 推荐的 Agent 输出策略
- 查询型、多步型任务：优先输出脚本
- 原子控制型任务：继续用单工具
- 高风险写操作：仍然走计划/审批

### 10.4 推荐的审批规则
建议服务端 Agent 当前按下面的方式配合客户端：
- `riskLevel=low`
  - 可自动执行
- `riskLevel=medium`
  - 可按服务端策略自动执行，但建议做任务级审计
- `riskLevel=high`
  - 必须先审批，再带 `approved=true` 调 `execute_script`

### 10.5 推荐的执行控制策略
建议服务端 Agent 当前按下面方式使用执行控制：
- 普通查询/轻写操作脚本：
  - `timeoutMs` 设为 `10000 ~ 30000`
- 涉及网络链路或多步写操作：
  - `timeoutMs` 设为 `30000 ~ 60000`
- 用户显式撤销任务时：
  - 立即发 `cancel_script`
- 不要假设取消能自动回滚已经完成的步骤

---

## 11. 当前关键代码索引
- `src/agent/script/AgentScriptExecutor.h`
- `src/agent/script/AgentScriptExecutor.cpp`
- `src/agent/AgentChatViewModel.h`
- `src/agent/AgentChatViewModel.cpp`
- `src/agent/tool/AgentToolExecutor.h`
- `src/agent/tool/AgentToolExecutor.cpp`
- `src/agent/host/HostStateProvider.h`
- `src/agent/host/HostStateProvider.cpp`
- `src/agent/protocol/AgentProtocolRouter.h`
- `src/agent/protocol/AgentProtocolRouter.cpp`
- `src/agent/AgentWebSocketClient.h`
- `src/agent/AgentWebSocketClient.cpp`

---

## 12. 当前阶段总结
当前客户端已经完成前六个阶段脚本化改造：
- 有统一脚本执行入口
- 有最小脚本 DSL
- 能顺序执行多步动作
- 能复用现有工具执行器
- 能返回结构化执行报告
- 已有正式协议入口可供服务端联调
- 已具备最小可用的风险分层与审批门槛
- 已具备最小可用的超时、取消和执行审计字段
- 已具备 dry-run 预演能力
- 已具备步骤级和脚本级领域聚合元数据
- 已具备统一能力入口的第一刀收束（`AgentCapabilityFacade`）

这意味着 Agent 端后续可以正式开始设计：
- 脚本规划器
- `dry_run_script` 预演链路
- `validate_script` / `execute_script` 协议适配
- 多步任务审计
- 客户端世界状态协同
- 审批流和自动执行策略
- 执行取消与超时控制策略
- 领域级审批和 fallback/replan

接下来的核心工作，不再是“再开放几个零散工具”，而是让 Agent 端和客户端都围绕脚本执行来协同升级。
