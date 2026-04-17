# Agent 工具清单与调用约定（当前版）

## 1. 文档定位
本文档只承担 **概览与调用约定** 的角色，不再承担完整接口枚举。

当前版客户端开放接口的唯一真相来源是：
- `src/agent/tool/ToolRegistry.cpp`
- `src/agent/tool/AgentToolExecutor.cpp`

完整接口明细、功能树映射和内部能力边界请查看：
- `说明文档/客户端开放接口清单（当前版）.md`

## 2. 当前接口面概览
当前客户端已经开放的工具能力覆盖以下主题：
- 播放控制与播放队列
- 本地音乐与下载任务管理
- 在线搜索 / 推荐 / 歌词 / 视频
- 最近播放 / 喜欢音乐 / 我的歌单
- 用户资料与返回欢迎页
- 桌面歌词
- 设置读取与更新
- 插件列表、诊断与卸载控制
- 宿主快照与页面上下文

当前版已补齐：
- 在线音乐发起下载：`downloadTrack`
- 页面级 UI 查询：`getUiOverview`, `getUiPageState`
- 音乐 tab 统一查询与动作：`getMusicTabItems`, `getMusicTabItem`, `playMusicTabTrack`, `invokeMusicTabAction`

保持内部、不开放为工具接口的能力包括：
- 登录 / 注册 / 重置密码
- 欢迎页 `client/ping` / `client/bootstrap`
- 欢迎页进入在线模式 / 离线直进本身
- 纯 QML 视觉级内部交互信号

## 3. 调用约定
1. 参数应尽量使用结构化字段，不依赖客户端做自由文本推理。
2. 写操作必须遵守 `riskLevel + confirmPolicy`；高副作用操作默认需要审批。
3. `availabilityPolicy` 为 `login_required` 或 `online_required` 的工具，应允许客户端在条件不满足时拒绝执行。
4. 历史兼容别名可以保留，例如 `addTracksToPlaylist`，但文档和新调用应优先使用当前主名称。
5. 新功能若要对 Agent 开放，必须同时更新：
   - `ToolRegistry`
   - `AgentToolExecutor`
   - `说明文档/客户端开放接口清单（当前版）.md`

## 4. 当前版本注意点
- `ToolRegistry + AgentToolExecutor` 才是接口真相，旧主题文档不能单独代表当前开放面。
- 旧 sidecar / websocket / `8765` 相关链路不再是桌面主链开放接口来源。
- `agentLocalModelBaseUrl` 仍保留为兼容字段，但不代表当前桌面主链仍依赖 HTTP 模型服务。
