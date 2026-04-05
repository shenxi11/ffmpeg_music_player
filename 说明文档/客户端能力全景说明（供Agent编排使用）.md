# 客户端能力全景说明（供Agent编排使用）

## 1. 文档目标

这份文档面向 Agent 服务端，而不是普通客户端开发者。目标是帮助服务端 AI 基于 Qt 客户端的真实实现建立能力目录、对象模型、状态依赖和多步行动链规则。

它重点回答以下问题：

- 客户端当前真实具备哪些业务能力
- 每个能力如何调用，调用入口在哪里
- 能力之间如何串联成稳定的多步链路
- 哪些对象和状态必须先拿到，哪些不能凭空假设
- 哪些能力适合自动执行，哪些需要谨慎或审批
- 哪些工具协议上存在，但实际上只是部分可用或未完整落地

本文档严格基于当前 Qt 客户端代码现状撰写，重点服务于服务端以下工作：

- Capability Catalog 建模
- 工具编排器设计
- 多步行动链规划
- 风险分级与自动执行策略
- 世界状态模型与工作记忆设计

## 2. 系统总体架构

### 2.1 Agent 协议入口

关键文件：

- `src/agent/AgentWebSocketClient.h`
- `src/agent/AgentWebSocketClient.cpp`
- `src/agent/protocol/AgentProtocolRouter.h`
- `src/agent/protocol/AgentProtocolRouter.cpp`

`AgentWebSocketClient` 负责与本地 sidecar Agent 服务端建立 WebSocket 连接，发送 `user_message`、`tool_result`、`approval_response`，并接收文本协议帧。收到消息后，会交给 `AgentProtocolRouter::parseMessage(...)` 解析。

`AgentProtocolRouter` 当前已解析以下协议消息：

- `session_ready`
- `assistant_start`
- `assistant_chunk`
- `assistant_final`
- `error`
- `plan_preview`
- `approval_request`
- `clarification_request`
- `progress`
- `final_result`
- `tool_call`

### 2.2 会话控制与工具转发

关键文件：

- `src/agent/AgentChatViewModel.h`
- `src/agent/AgentChatViewModel.cpp`

`AgentChatViewModel` 是 Qt 侧 Agent 会话总控，职责包括：

- 管理 `AgentProcessManager`、`AgentWebSocketClient`
- 管理消息列表和会话列表
- 接收 `tool_call`
- 调用 `AgentToolExecutor::executeToolCall(...)`
- 接收执行结果，再调用 `AgentWebSocketClient::sendToolResult(...)` 回传服务端

主链路：

1. `AgentWebSocketClient` 收到 `tool_call`
2. `AgentProtocolRouter::parseMessage(...)` 解析消息
3. `AgentChatViewModel::onToolCallReceived(...)`
4. `AgentToolExecutor::executeToolCall(...)`
5. `AgentToolExecutor` 发出 `toolResultReady`
6. `AgentChatViewModel::onToolResultReady(...)`
7. `AgentWebSocketClient::sendToolResult(...)`

### 2.3 工具注册表

关键文件：

- `src/agent/tool/ToolRegistry.h`
- `src/agent/tool/ToolRegistry.cpp`

`ToolRegistry` 的职责：

- 注册 Agent 可见工具
- 定义参数 schema
- 标记工具读写属性
- 标记 `requireApproval`

注意：`requireApproval` 当前主要是元数据，便于服务端做风险建模；Qt 侧并没有对所有高风险工具做统一硬拦截，因此服务端不能假设客户端一定会阻止高风险写操作。

### 2.4 工具执行层

关键文件：

- `src/agent/tool/AgentToolExecutor.h`
- `src/agent/tool/AgentToolExecutor.cpp`

`AgentToolExecutor` 是 Qt 客户端能力真正落地的桥接层，负责把协议层工具调用转成客户端内部真实业务调用。它主要桥接：

- `HostStateProvider`
- `MainShellViewModel`
- `AudioService`
- `DownloadManager`
- `LocalMusicCache`
- `PluginManager`
- `SettingsManager`
- `MainWidget`

### 2.5 Host 状态投影层

关键文件：

- `src/agent/host/HostStateProvider.h`
- `src/agent/host/HostStateProvider.cpp`

`HostStateProvider` 负责把客户端内部对象转换成 Agent 更容易消费的结构。关键函数包括：

- `currentTrackSnapshot()`
- `convertMusicList(...)`
- `convertHistoryList(...)`
- `convertFavoriteList(...)`
- `convertPlaylistList(...)`
- `convertPlaylistDetail(...)`
- `convertLocalMusicList(...)`

### 2.6 业务壳层和播放服务

关键文件：

- `src/viewmodels/MainShellViewModel.h`
- `src/viewmodels/MainShellViewModel.cpp`
- `src/viewmodels/MainShellViewModel.connections.cpp`
- `src/audio/AudioService.h`
- `src/audio/AudioService.cpp`

`MainShellViewModel` 负责歌单、搜索、推荐、最近播放、喜欢音乐等业务请求与信号汇总。`AudioService` 负责音频播放、队列、播放模式、音量和位置控制。

### 2.7 总体调用链

```text
Agent 服务端
  -> WebSocket 消息
  -> AgentWebSocketClient
  -> AgentProtocolRouter
  -> AgentChatViewModel::onToolCallReceived
  -> ToolRegistry 参数校验
  -> AgentToolExecutor::executeToolCall
      -> HostStateProvider / MainShellViewModel / AudioService / MainWidget / SettingsManager ...
  -> AgentToolExecutor 发出 toolResultReady
  -> AgentChatViewModel::onToolResultReady
  -> AgentWebSocketClient::sendToolResult
  -> Agent 服务端
```

## 3. 客户端现有能力总表

下表列出当前 `ToolRegistry` 已注册并对 Agent 可见的全部能力，并标记真实落地状态。

| 能力名 | 工具名 | 功能说明 | 读/写 | 是否真实可用 | 是否适合自动执行 | 是否需要前置对象 | 返回核心对象 | 调用入口 | 主要依赖 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 搜索歌曲 | `searchTracks` | 关键字搜索歌曲 | 读 | 是 | 是 | 否 | `SearchResult` | `AgentToolExecutor` | `MainShellViewModel`, `HttpRequestV2` |
| 获取歌词 | `getLyrics` | 通过歌曲路径获取歌词文本 | 读 | 是 | 是 | 建议先有 `musicPath` | `lyrics` | `AgentToolExecutor` | `HttpRequestV2` |
| 获取视频列表 | `getVideoList` | 查询视频列表 | 读 | 是 | 是 | 否 | `videoList` | `AgentToolExecutor` | `HttpRequestV2` |
| 获取视频流地址 | `getVideoStream` | 获取视频可播放流地址 | 读 | 是 | 是 | 需要 `videoId` | `streamUrl` | `AgentToolExecutor` | `HttpRequestV2` |
| 搜索歌手 | `searchArtist` | 查询歌手是否存在 | 读 | 是 | 是 | 否 | `artistExists` | `AgentToolExecutor` | `HttpRequestV2` |
| 获取歌手歌曲 | `getTracksByArtist` | 查询歌手名下歌曲 | 读 | 是 | 是 | 需要 `artist` | `SearchResult` | `AgentToolExecutor` | `HttpRequestV2`, `HostStateProvider` |
| 获取当前播放 | `getCurrentTrack` | 获取当前播放快照 | 读 | 是 | 是 | 否 | `CurrentPlayback` | `AgentToolExecutor` | `HostStateProvider`, `AudioService` |
| 获取最近播放 | `getRecentTracks` | 查询最近播放列表 | 读 | 是 | 是 | 通常需要登录 | `RecentTrackList` | `AgentToolExecutor` | `MainShellViewModel`, `HttpRequestV2` |
| 添加最近播放 | `addRecentTrack` | 写入最近播放 | 写 | 是 | 建议谨慎 | 需要歌曲对象或路径 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 删除最近播放 | `removeRecentTracks` | 删除最近播放项 | 写 | 是 | 建议谨慎 | 需要 `trackId` 或路径 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 获取喜欢音乐 | `getFavorites` | 查询喜欢列表 | 读 | 是 | 是 | 通常需要登录 | `FavoriteTrackList` | `AgentToolExecutor` | `MainShellViewModel`, `HttpRequestV2` |
| 添加喜欢 | `addFavorite` | 加入喜欢列表 | 写 | 是 | 低风险自动执行可考虑 | 需要歌曲对象 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 取消喜欢 | `removeFavorites` | 从喜欢列表移除 | 写 | 是 | 建议谨慎 | 需要 `trackId` / 路径 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 获取歌单列表 | `getPlaylists` | 查询用户歌单 | 读 | 是 | 是 | 通常需要登录 | `PlaylistList` | `AgentToolExecutor` | `MainShellViewModel`, `HttpRequestV2` |
| 获取歌单详情 | `getPlaylistTracks` | 查询歌单歌曲内容 | 读 | 是 | 是 | 需要 `playlistId` | `PlaylistTrackList` | `AgentToolExecutor` | `MainShellViewModel`, `HostStateProvider` |
| 创建歌单 | `createPlaylist` | 创建用户歌单 | 写 | 是 | 建议先确认 | 否 | `Playlist` | `AgentToolExecutor` | `MainShellViewModel` |
| 更新歌单 | `updatePlaylist` | 修改歌单名称/简介等 | 写 | 是 | 建议先确认 | 需要 `playlistId` | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 删除歌单 | `deletePlaylist` | 删除歌单 | 写 | 是 | 高风险 | 需要 `playlistId` | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 歌单加歌 | `addPlaylistItems` | 向歌单中加入歌曲 | 写 | 是 | 建议先确认 | 需要 `playlistId` 和歌曲对象 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 歌单删歌 | `removePlaylistItems` | 从歌单移除歌曲 | 写 | 是 | 建议先确认 | 需要 `playlistId` 和歌曲对象 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 歌单排序 | `reorderPlaylistItems` | 调整歌单内顺序 | 写 | 是 | 建议先确认 | 需要 `playlistId` 和排序参数 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 获取推荐 | `getRecommendations` | 获取推荐歌曲列表 | 读 | 是 | 是 | 通常需要登录 | `SearchResult` | `AgentToolExecutor` | `MainShellViewModel` |
| 获取相似推荐 | `getSimilarRecommendations` | 基于当前歌曲获取相似推荐 | 读 | 是 | 是 | 建议有当前歌曲 | `SearchResult` | `AgentToolExecutor` | `MainShellViewModel` |
| 推荐反馈 | `submitRecommendationFeedback` | 提交推荐反馈 | 写 | 是 | 建议先确认 | 需要推荐对象 | `result` | `AgentToolExecutor` | `MainShellViewModel` |
| 播放歌曲 | `playTrack` | 播放指定歌曲 | 写 | 是 | 是 | 需要 `trackId` 或 `musicPath` | `CurrentPlayback` | `AgentToolExecutor` | `AudioService`, `HostStateProvider` |
| 播放歌单 | `playPlaylist` | 播放歌单 | 写 | 部分可用 | 不建议直接自动执行 | 需要 `playlistId` | `result` | `AgentToolExecutor` | `AudioService`, `playlistDetail` 缓存 |
| 暂停播放 | `pausePlayback` | 暂停当前音频 | 写 | 是 | 是 | 当前有播放会话更合理 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 恢复播放 | `resumePlayback` | 继续播放当前音频 | 写 | 是 | 是 | 当前已有可恢复会话 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 停止播放 | `stopPlayback` | 停止音频播放 | 写 | 是 | 是 | 否 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 音频拖动 | `seekPlayback` | 调整当前音频进度 | 写 | 是 | 是 | 需要当前会话 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 下一首 | `playNext` | 播放队列下一首 | 写 | 是 | 是 | 需要非空播放队列 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 上一首 | `playPrevious` | 播放队列上一首 | 写 | 是 | 是 | 需要非空播放队列 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 按索引播放 | `playAtIndex` | 按播放队列索引播放 | 写 | 是 | 是 | 需要队列和索引 | `CurrentPlayback` | `AgentToolExecutor` | `AudioService` |
| 设置音量 | `setVolume` | 设置音量百分比 | 写 | 是 | 是 | 否 | `volume` | `AgentToolExecutor` | `AudioService` |
| 设置播放模式 | `setPlayMode` | 设置顺序/随机/单曲等模式 | 写 | 是 | 是 | 否 | `PlaybackQueue` | `AgentToolExecutor` | `AudioService` |
| 获取播放队列 | `getPlaybackQueue` | 获取当前播放队列 | 读 | 是 | 是 | 否 | `PlaybackQueue` | `AgentToolExecutor` | `AudioService`, `HostStateProvider` |
| 设置播放队列 | `setPlaybackQueue` | 重建播放队列 | 写 | 是 | 建议先确认 | 需要歌曲对象列表 | `PlaybackQueue` | `AgentToolExecutor` | `AudioService` |
| 队列加歌 | `addToPlaybackQueue` | 向播放队列追加歌曲 | 写 | 是 | 低风险自动执行可考虑 | 需要歌曲对象 | `PlaybackQueue` | `AgentToolExecutor` | `AudioService` |
| 队列删歌 | `removeFromPlaybackQueue` | 从播放队列移除歌曲 | 写 | 是 | 建议谨慎 | 需要队列对象或索引 | `PlaybackQueue` | `AgentToolExecutor` | `AudioService` |
| 清空播放队列 | `clearPlaybackQueue` | 清空当前队列 | 写 | 是 | 建议先确认 | 否 | `PlaybackQueue` | `AgentToolExecutor` | `AudioService` |
| 获取本地音乐 | `getLocalTracks` | 获取本地缓存音乐列表 | 读 | 是 | 是 | 否 | `LocalTrackList` | `AgentToolExecutor` | `LocalMusicCache`, `HostStateProvider` |
| 添加本地音乐 | `addLocalTrack` | 导入本地音乐文件 | 写 | 是 | 建议先确认 | 需要本地文件路径 | `result` | `AgentToolExecutor` | `LocalMusicCache` |
| 删除本地音乐 | `removeLocalTrack` | 删除本地音乐记录 | 写 | 是 | 建议谨慎 | 需要本地文件路径 | `result` | `AgentToolExecutor` | `LocalMusicCache` |
| 获取下载任务 | `getDownloadTasks` | 获取下载队列与状态 | 读 | 是 | 是 | 否 | `DownloadTaskList` | `AgentToolExecutor` | `DownloadManager` |
| 暂停下载 | `pauseDownloadTask` | 暂停下载任务 | 写 | 是 | 是 | 需要任务标识 | `result` | `AgentToolExecutor` | `DownloadManager` |
| 恢复下载 | `resumeDownloadTask` | 恢复下载任务 | 写 | 是 | 是 | 需要任务标识 | `result` | `AgentToolExecutor` | `DownloadManager` |
| 取消下载 | `cancelDownloadTask` | 取消下载任务 | 写 | 是 | 建议先确认 | 需要任务标识 | `result` | `AgentToolExecutor` | `DownloadManager` |
| 移除下载任务 | `removeDownloadTask` | 从下载列表移除任务 | 写 | 是 | 建议谨慎 | 需要任务标识 | `result` | `AgentToolExecutor` | `DownloadManager` |
| 获取视频窗口状态 | `getVideoWindowState` | 获取当前视频窗口状态 | 读 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 播放视频 | `playVideo` | 在视频窗口中播放视频 | 写 | 部分可用 | 建议先确认 | 需要视频窗口和 `streamUrl` | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 暂停视频 | `pauseVideoPlayback` | 暂停视频 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 恢复视频 | `resumeVideoPlayback` | 恢复视频 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 视频拖动 | `seekVideoPlayback` | 视频进度拖动 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 视频全屏 | `setVideoFullScreen` | 设置视频全屏状态 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 视频倍速 | `setVideoPlaybackRate` | 设置视频播放倍率 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 视频画质预设 | `setVideoQualityPreset` | 设置视频渲染画质预设 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 关闭视频窗口 | `closeVideoWindow` | 关闭视频窗口 | 写 | 部分可用 | 是 | 依赖视频窗口已创建 | `VideoWindowState` | `AgentToolExecutor` | `MainWidget`, `VideoPlayerWindow` |
| 获取桌面歌词状态 | `getDesktopLyricsState` | 获取桌面歌词状态 | 读 | 是 | 是 | 否 | `DesktopLyricsState` | `AgentToolExecutor` | `MainWidget`, `PlayWidget` |
| 显示桌面歌词 | `showDesktopLyrics` | 显示桌面歌词窗口 | 写 | 是 | 是 | 否 | `DesktopLyricsState` | `AgentToolExecutor` | `MainWidget`, `PlayWidget`, `DeskLrcQml` |
| 隐藏桌面歌词 | `hideDesktopLyrics` | 隐藏桌面歌词窗口 | 写 | 是 | 是 | 否 | `DesktopLyricsState` | `AgentToolExecutor` | `MainWidget`, `PlayWidget`, `DeskLrcQml` |
| 设置桌面歌词样式 | `setDesktopLyricsStyle` | 设置颜色/字号/字体等 | 写 | 是 | 是 | 否 | `DesktopLyricsState` | `AgentToolExecutor` | `DeskLrcQml` |
| 获取插件列表 | `getPlugins` | 获取插件元数据列表 | 读 | 是 | 是 | 否 | `PluginList` | `AgentToolExecutor` | `PluginManager` |
| 获取插件诊断 | `getPluginDiagnostics` | 获取插件诊断结果 | 读 | 是 | 是 | 否 | `diagnostics` | `AgentToolExecutor` | `PluginManager` |
| 重载插件 | `reloadPlugins` | 重新扫描并加载插件 | 写 | 是 | 建议先确认 | 否 | `PluginList` | `AgentToolExecutor` | `PluginManager` |
| 卸载插件 | `unloadPlugin` | 卸载指定插件 | 写 | 是 | 建议谨慎 | 需要插件名 | `result` | `AgentToolExecutor` | `PluginManager` |
| 卸载全部插件 | `unloadAllPlugins` | 卸载全部插件 | 写 | 是 | 高风险 | 否 | `result` | `AgentToolExecutor` | `PluginManager` |
| 获取设置快照 | `getSettingsSnapshot` | 读取已开放设置项 | 读 | 是 | 是 | 否 | `SettingsSnapshot` | `AgentToolExecutor` | `SettingsManager` |
| 更新设置 | `updateSetting` | 更新单个已开放设置项 | 写 | 是 | 建议先确认 | 需要合法 key/value | `result` | `AgentToolExecutor` | `SettingsManager` |

## 4. 每个能力的详细说明

本节从 Agent 编排角度逐项说明。能力较多，因此按业务域分组，但每个工具都单独给出执行语义。

### 4.1 搜索与媒体发现域

#### 4.1.1 搜索歌曲 / `searchTracks`

- 作用：根据关键词搜索在线歌曲。
- 典型使用场景：用户说“播放七里香”“帮我找周杰伦的歌”。
- 输入参数：`query`。
- 输出结构：`SearchResult`，核心是 `items` 数组，每项为 `Track`。
- 前置条件：无。
- 执行逻辑：`AgentToolExecutor::executeSearchTracks(...)` -> `MainShellViewModel::searchTracks(...)` -> `HttpRequestV2::getMusic(...)` -> `HostStateProvider::convertMusicList(...)`。
- 依赖的缓存/上下文：结果会写入执行器的 `m_trackCacheById`。
- 成功后的状态变化：不改播放状态，只增加会话内可引用歌曲对象。
- 失败语义：请求失败时底层可能只打印日志，不一定会及时返回结构化错误；服务端应自行设置超时。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是，且通常是播放歌曲、加歌单、加入队列的第一步。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/viewmodels/MainShellViewModel.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.1.2 获取歌词 / `getLyrics`

- 作用：按歌曲路径获取歌词文本。
- 典型使用场景：Agent 需要回答歌词内容，或为歌词相关能力补充数据。
- 输入参数：`musicPath`。
- 输出结构：`lyrics` 字段。
- 前置条件：建议先从 `searchTracks`、`getCurrentTrack`、`getPlaylistTracks` 拿到真实 `musicPath`。
- 执行逻辑：`AgentToolExecutor::executeGetLyrics(...)` -> `HttpRequestV2::getLyrics(...)`。
- 依赖的缓存/上下文：无独立缓存。
- 成功后的状态变化：无副作用。
- 失败语义：可能返回空歌词，也可能因网络失败没有及时回执。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.1.3 获取视频列表 / `getVideoList`

- 作用：获取视频列表。
- 典型使用场景：用户说“看看视频列表”。
- 输入参数：当前客户端侧未做复杂筛选增强。
- 输出结构：`videoList`。
- 前置条件：无。
- 执行逻辑：`AgentToolExecutor::executeGetVideoList(...)` -> `HttpRequestV2::getVideoList(...)`。
- 依赖的缓存/上下文：`HttpRequestV2` 有短期响应缓存。
- 成功后的状态变化：无。
- 失败语义：可能返回空列表，或仅记录日志。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是，常用于 `getVideoStream` 前。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.1.4 获取视频流地址 / `getVideoStream`

- 作用：根据 `videoId` 获取可播放流地址。
- 典型使用场景：先查视频，再实际播放视频。
- 输入参数：`videoId`。
- 输出结构：`streamUrl`。
- 前置条件：通常先通过 `getVideoList` 拿到 `videoId`。
- 执行逻辑：`AgentToolExecutor::executeGetVideoStream(...)` -> `HttpRequestV2::getVideoStream(...)`。
- 依赖的缓存/上下文：无。
- 成功后的状态变化：无。
- 失败语义：可能无有效 `streamUrl`。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.1.5 搜索歌手 / `searchArtist`

- 作用：检查歌手是否存在。
- 典型使用场景：用户说“有没有周杰伦”。
- 输入参数：`artist`。
- 输出结构：`artistExists`。
- 前置条件：无。
- 执行逻辑：`AgentToolExecutor::executeSearchArtist(...)` -> `HttpRequestV2::searchArtist(...)`。
- 依赖的缓存/上下文：无。
- 成功后的状态变化：无。
- 失败语义：当前失败与“不存在”在底层都可能表现为 `false`，服务端不能把 `false` 直接等价为业务上不存在。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.1.6 获取歌手歌曲 / `getTracksByArtist`

- 作用：获取某歌手的歌曲列表。
- 典型使用场景：用户说“播放周杰伦的歌”。
- 输入参数：`artist`。
- 输出结构：`SearchResult`。
- 前置条件：通常最好先确认歌手存在。
- 执行逻辑：`AgentToolExecutor::executeGetTracksByArtist(...)` -> `HttpRequestV2::getMusicByArtist(...)` -> `HostStateProvider::convertMusicList(...)`。
- 依赖的缓存/上下文：结果写入 `m_trackCacheById`。
- 成功后的状态变化：无。
- 失败语义：可能空列表，也可能网络失败后无结构化回执。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/network/httprequest_v2.cpp`、`src/agent/host/HostStateProvider.cpp`。

### 4.2 当前播放与播放控制域

#### 4.2.1 获取当前播放 / `getCurrentTrack`

- 作用：获取当前播放世界状态。
- 典型使用场景：回答“现在在播什么”。
- 输入参数：无。
- 输出结构：`CurrentPlayback`。
- 前置条件：无。
- 执行逻辑：`HostStateProvider::currentTrackSnapshot()`。
- 依赖的缓存/上下文：依赖 `AudioService` 当前播放状态。
- 成功后的状态变化：无。
- 失败语义：未播放时返回空标题/空路径/`playing=false`。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/host/HostStateProvider.cpp`。

#### 4.2.2 播放歌曲 / `playTrack`

- 作用：播放指定单曲。
- 典型使用场景：用户说“播放七里香”。
- 输入参数：优先 `trackId`，也可 `musicPath`。
- 输出结构：播放结果和更新后的 `CurrentPlayback`。
- 前置条件：需要能解析到真实歌曲对象；如果只有模糊标题，应该先搜索。
- 执行逻辑：从 `m_trackCacheById` 查对象，或直接读取 `musicPath`，然后通过 `toPlayableUrl(...)` 归一化路径，最终调用 `AudioService::play(url)`。
- 依赖的缓存/上下文：如果用 `trackId`，依赖先前搜索、歌单、最近播放产生的缓存对象。
- 成功后的状态变化：当前播放切换到目标歌曲，播放队列可能发生追加或索引变化，UI 播放状态同步变化。
- 失败时的返回语义：`track_not_found`、`music_path_missing`、`play_failed`，以及路径解析错误导致的无法打开资源。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：通常是最终执行步骤。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/audio/AudioService.cpp`。

#### 4.2.3 播放歌单 / `playPlaylist`

- 作用：按歌单整体播放。
- 典型使用场景：用户说“播放我的流行歌单”。
- 输入参数：`playlistId`。
- 输出结构：结果及可能的队列快照。
- 前置条件：需要先有 `playlistId`，并且歌单详情已获取或可获取。
- 执行逻辑：读取 `m_playlistDetailById`；若无缓存则先请求歌单详情；提取歌曲 URL，尝试设置到 `AudioService`。
- 依赖的缓存/上下文：强依赖歌单详情对象。
- 成功后的状态变化：理论上应替换播放队列并开始播放。
- 失败时的返回语义：当前实现是部分可用。原因是执行器仍调用 `AudioService::setPlaylist(...)`，而 `AudioService` 内该接口已弃用且为 no-op，因此该工具不能视为稳定能力。
- 是否适合 Agent 自动执行：当前不建议。
- 是否适合作为中间步骤：不建议，推荐改为 `getPlaylistTracks -> setPlaybackQueue -> playAtIndex`。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/audio/AudioService.cpp`。

#### 4.2.4 暂停播放 / `pausePlayback`

- 作用：暂停当前音频。
- 典型使用场景：用户说“暂停”。
- 输入参数：无。
- 输出结构：`CurrentPlayback`。
- 前置条件：通常要有活跃会话。
- 执行逻辑：`AudioService::pause()`。
- 依赖的缓存/上下文：当前播放会话。
- 成功后的状态变化：`playing=false`。
- 失败语义：无活跃会话时通常表现为无效操作。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.5 恢复播放 / `resumePlayback`

- 作用：恢复当前音频。
- 输入参数：无。
- 输出结构：`CurrentPlayback`。
- 前置条件：需要存在可恢复的当前会话。
- 执行逻辑：`AudioService::resume()`。
- 成功后的状态变化：`playing=true`。
- 失败语义：若没有可恢复会话则可能无效。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.6 停止播放 / `stopPlayback`

- 作用：停止当前音频。
- 输入参数：无。
- 输出结构：`CurrentPlayback`。
- 前置条件：无。
- 执行逻辑：`AudioService::stop()`。
- 成功后的状态变化：停止会话，`playing=false`。
- 失败语义：无播放时通常无副作用。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.7 音频拖动 / `seekPlayback`

- 作用：调整当前音频播放进度。
- 输入参数：`positionMs`。
- 输出结构：`CurrentPlayback`。
- 前置条件：存在活跃会话。
- 执行逻辑：`AudioService::seekTo(positionMs)`。
- 依赖的缓存/上下文：当前播放会话。
- 成功后的状态变化：位置变化，可能触发缓冲与会话切换。
- 失败语义：无活跃会话时无效；远程流大幅拖动可能进入重新缓冲。
- 是否适合 Agent 自动执行：是，但不适合频繁连续调用。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.8 下一首 / `playNext`

- 作用：播放队列下一首。
- 输入参数：无。
- 输出结构：`CurrentPlayback`。
- 前置条件：播放队列非空。
- 执行逻辑：`AudioService::playNext()`。
- 成功后的状态变化：索引切换，当前播放变化。
- 失败语义：空队列或单项队列时可能无变化。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.9 上一首 / `playPrevious`

- 作用：播放队列上一首。
- 输入参数：无。
- 输出结构：`CurrentPlayback`。
- 前置条件：播放队列非空。
- 执行逻辑：`AudioService::playPrevious()`。
- 成功后的状态变化：索引切换。
- 失败语义：空队列或单项队列时可能无变化。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.10 按索引播放 / `playAtIndex`

- 作用：按队列索引播放。
- 输入参数：`index`。
- 输出结构：`CurrentPlayback`。
- 前置条件：队列非空，索引合法。
- 执行逻辑：`AudioService::playAtIndex(index)`。
- 成功后的状态变化：当前播放切换为队列中的指定项。
- 失败语义：索引越界。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.2.11 设置音量 / `setVolume`

- 作用：设置播放音量。
- 输入参数：`volume` 或等价字段。
- 输出结构：新的音量值。
- 前置条件：无。
- 执行逻辑：`AudioService::setVolume(...)`。
- 成功后的状态变化：仅音量变化。
- 失败语义：越界值通常会被裁剪或无效。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

### 4.3 播放队列域

#### 4.3.1 获取播放队列 / `getPlaybackQueue`

- 作用：获取当前播放队列及当前索引。
- 输入参数：无。
- 输出结构：`PlaybackQueue`。
- 前置条件：无。
- 执行逻辑：`AgentToolExecutor::queueSnapshot()`。
- 依赖的缓存/上下文：依赖 `AudioService` 当前队列。
- 成功后的状态变化：无。
- 失败语义：空队列返回 `count=0`。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.3.2 设置播放队列 / `setPlaybackQueue`

- 作用：用给定歌曲列表重建播放队列。
- 输入参数：`trackIds` 或 `tracks`，可选 `playNow`、`startIndex`。
- 输出结构：新的 `PlaybackQueue`。
- 前置条件：需要可解析的歌曲对象列表。
- 执行逻辑：`AudioService::clearPlaylist()` -> 逐项 `addToPlaylist(...)` -> 可选 `playAtIndex(...)`。
- 依赖的缓存/上下文：如果用 `trackIds`，依赖 `m_trackCacheById`。
- 成功后的状态变化：替换整个队列，可能改变当前播放。
- 失败语义：找不到对象、路径不可解析、队列为空。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：非常适合，是“播放歌单”的更稳定替代方案。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/audio/AudioService.cpp`。

#### 4.3.3 队列加歌 / `addToPlaybackQueue`

- 作用：向队列追加歌曲。
- 输入参数：`trackId` / `musicPath` / `tracks`。
- 输出结构：新的 `PlaybackQueue`。
- 前置条件：需要歌曲对象。
- 执行逻辑：`AudioService::addToPlaylist(...)`。
- 成功后的状态变化：队列长度增加。
- 失败语义：对象不可解析。
- 是否适合 Agent 自动执行：低风险，可自动执行。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.3.4 队列删歌 / `removeFromPlaybackQueue`

- 作用：移除队列中的某项。
- 输入参数：索引或对象。
- 输出结构：新的 `PlaybackQueue`。
- 前置条件：队列非空，目标存在。
- 执行逻辑：`AudioService::removeFromPlaylist(index)`。
- 成功后的状态变化：队列变化，当前索引可能重算。
- 失败语义：索引越界或目标不存在。
- 是否适合 Agent 自动执行：建议谨慎。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.3.5 清空播放队列 / `clearPlaybackQueue`

- 作用：清空播放队列。
- 输入参数：无。
- 输出结构：新的 `PlaybackQueue`。
- 前置条件：无。
- 执行逻辑：`AudioService::clearPlaylist()`。
- 成功后的状态变化：队列清空。
- 失败语义：无。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

#### 4.3.6 设置播放模式 / `setPlayMode`

- 作用：设置顺序、随机、单曲等播放模式。
- 输入参数：`mode`。
- 输出结构：队列快照，含 `playMode` 与 `playModeName`。
- 前置条件：无。
- 执行逻辑：`AudioService::setPlayMode(...)`。
- 成功后的状态变化：后续自动切歌逻辑变化。
- 失败语义：非法模式值。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/audio/AudioService.cpp`。

### 4.4 最近播放与喜欢域

#### 4.4.1 获取最近播放 / `getRecentTracks`

- 作用：获取最近播放列表。
- 输入参数：可选 `limit`。
- 输出结构：`RecentTrackList`。
- 前置条件：通常需要登录。
- 执行逻辑：`MainShellViewModel::requestHistory(...)` -> `HttpRequestV2::getPlayHistory(...)` -> `HostStateProvider::convertHistoryList(...)`。
- 依赖的缓存/上下文：响应缓存约 30 秒。
- 成功后的状态变化：无。
- 失败语义：`not_logged_in`、空列表、网络失败无回执。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/viewmodels/MainShellViewModel.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.4.2 添加最近播放 / `addRecentTrack`

- 作用：写入最近播放记录。
- 输入参数：`trackId` / `musicPath` / 展示字段。
- 输出结构：写入结果。
- 前置条件：通常需要登录；需要可解析歌曲对象。
- 执行逻辑：通过 `MainShellViewModel` 发起请求，等待结果信号回执。
- 依赖的缓存/上下文：若使用 `trackId`，依赖缓存对象。
- 成功后的状态变化：最近播放列表数据变化。
- 失败语义：`not_logged_in`、对象缺失、服务端写入失败。
- 是否适合 Agent 自动执行：建议谨慎。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.4.3 删除最近播放 / `removeRecentTracks`

- 作用：删除最近播放项。
- 输入参数：`trackIds`、路径等。
- 输出结构：写入结果。
- 前置条件：通常需要登录；需要目标对象。
- 执行逻辑：通过 `MainShellViewModel` 写操作并等待结果回执。
- 成功后的状态变化：最近播放列表变化。
- 失败语义：目标不存在、登录缺失、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.4.4 获取喜欢音乐 / `getFavorites`

- 作用：获取用户喜欢列表。
- 输入参数：可选 `limit`。
- 输出结构：`FavoriteTrackList`。
- 前置条件：通常需要登录。
- 执行逻辑：`MainShellViewModel::requestFavorites(...)` -> `HttpRequestV2::getFavorites(...)` -> `HostStateProvider::convertFavoriteList(...)`。
- 依赖的缓存/上下文：响应缓存约 60 秒。
- 成功后的状态变化：无。
- 失败语义：`not_logged_in`、空列表、网络失败无回执。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/network/httprequest_v2.cpp`。

#### 4.4.5 添加喜欢 / `addFavorite`

- 作用：将歌曲加入喜欢列表。
- 输入参数：`trackId` / `musicPath` / 歌曲元数据。
- 输出结构：写入结果。
- 前置条件：通常需要登录；需有可解析歌曲对象。
- 执行逻辑：通过 `MainShellViewModel` 写操作并等待回执。
- 成功后的状态变化：喜欢列表变化。
- 失败语义：重复添加、未登录、对象缺失、服务端失败。
- 是否适合 Agent 自动执行：低风险，可自动执行。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.4.6 取消喜欢 / `removeFavorites`

- 作用：从喜欢列表中移除歌曲。
- 输入参数：目标歌曲对象。
- 输出结构：写入结果。
- 前置条件：通常需要登录。
- 执行逻辑：通过 `MainShellViewModel` 写操作并等待回执。
- 成功后的状态变化：喜欢列表变化。
- 失败语义：目标不存在、未登录、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

### 4.5 歌单域

#### 4.5.1 获取歌单列表 / `getPlaylists`

- 作用：获取用户歌单总表。
- 典型使用场景：用户说“看看我的歌单”。
- 输入参数：可选限制参数。
- 输出结构：`PlaylistList`。
- 前置条件：通常需要登录。
- 执行逻辑：`MainShellViewModel::requestPlaylists(...)` -> `HttpRequestV2::getPlaylists(...)` -> `HostStateProvider::convertPlaylistList(...)`。
- 依赖的缓存/上下文：响应缓存约 30 秒；元数据会写入 `m_playlistMetaById`。
- 成功后的状态变化：无。
- 失败语义：空列表或无回执；未登录返回 `not_logged_in`。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是，且通常是所有歌单操作的第一步。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/agent/host/HostStateProvider.cpp`。

#### 4.5.2 获取歌单详情 / `getPlaylistTracks`

- 作用：获取歌单中的歌曲内容。
- 输入参数：`playlistId`。
- 输出结构：`PlaylistTrackList`。
- 前置条件：必须先有 `playlistId`。
- 执行逻辑：`MainShellViewModel::requestPlaylistDetail(playlistId)` -> `HttpRequestV2::getPlaylistDetail(...)` -> `HostStateProvider::convertPlaylistDetail(...)`。
- 依赖的缓存/上下文：响应缓存约 15 秒；结果会写入 `m_playlistDetailById`。
- 成功后的状态变化：无，但后续可用于播放歌单、歌单二次编辑和队列构建。
- 失败语义：空详情、歌单不存在、登录缺失。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：非常适合。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/agent/host/HostStateProvider.cpp`。

#### 4.5.3 创建歌单 / `createPlaylist`

- 作用：新建歌单。
- 输入参数：`name`，可选描述等。
- 输出结构：新歌单结果。
- 前置条件：通常需要登录。
- 执行逻辑：`MainShellViewModel::createPlaylist(...)`，等待写操作回执。
- 依赖的缓存/上下文：无硬依赖。
- 成功后的状态变化：歌单总表变化。
- 失败语义：重名、未登录、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.5.4 更新歌单 / `updatePlaylist`

- 作用：修改歌单信息。
- 输入参数：`playlistId` 与可更新字段。
- 输出结构：写入结果。
- 前置条件：必须先有稳定 `playlistId`。
- 执行逻辑：`MainShellViewModel::updatePlaylist(...)`。
- 依赖的缓存/上下文：推荐先 `getPlaylists`。
- 成功后的状态变化：歌单元数据变化。
- 失败语义：歌单不存在、未登录、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.5.5 删除歌单 / `deletePlaylist`

- 作用：删除歌单。
- 输入参数：`playlistId`。
- 输出结构：写入结果。
- 前置条件：必须先有稳定 `playlistId`。
- 执行逻辑：`MainShellViewModel::deletePlaylist(...)`。
- 成功后的状态变化：歌单总表变化，原 `playlistId` 失效。
- 失败语义：歌单不存在、未登录、服务端失败。
- 是否适合 Agent 自动执行：高风险，不建议无确认执行。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.5.6 歌单加歌 / `addPlaylistItems`

- 作用：把歌曲加入某个歌单。
- 输入参数：`playlistId` + 曲目对象或 `trackId`。
- 输出结构：写入结果。
- 前置条件：必须先有 `playlistId`，并且有可解析歌曲对象。
- 执行逻辑：`MainShellViewModel::addPlaylistItems(...)`。
- 依赖的缓存/上下文：`playlistId` 建议来自 `getPlaylists`；歌曲建议来自 `searchTracks`、`getRecentTracks`、`getFavorites`、`getPlaylistTracks`。
- 成功后的状态变化：歌单详情变化。
- 失败语义：重复歌曲、歌单不存在、对象不存在、未登录、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.5.7 歌单删歌 / `removePlaylistItems`

- 作用：从歌单移除歌曲。
- 输入参数：`playlistId` + 目标歌曲。
- 输出结构：写入结果。
- 前置条件：需有 `playlistId` 和歌单内歌曲对象。
- 执行逻辑：`MainShellViewModel::removePlaylistItems(...)`。
- 成功后的状态变化：歌单详情变化。
- 失败语义：对象不存在、歌单不存在、未登录、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.5.8 歌单排序 / `reorderPlaylistItems`

- 作用：修改歌单内歌曲顺序。
- 输入参数：`playlistId` 与排序信息。
- 输出结构：写入结果。
- 前置条件：需先知道歌单内容。
- 执行逻辑：`MainShellViewModel::reorderPlaylistItems(...)`。
- 成功后的状态变化：歌单详情顺序变化。
- 失败语义：排序参数不合法、歌单不存在、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

### 4.6 推荐域

#### 4.6.1 获取推荐 / `getRecommendations`

- 作用：获取推荐歌曲列表。
- 输入参数：可选推荐类型、数量。
- 输出结构：`SearchResult`。
- 前置条件：通常需要登录。
- 执行逻辑：`MainShellViewModel::requestRecommendations(...)`。
- 依赖的缓存/上下文：结果会进入 `m_trackCacheById`。
- 成功后的状态变化：无。
- 失败语义：未登录、空列表、服务端失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.6.2 获取相似推荐 / `getSimilarRecommendations`

- 作用：基于当前歌曲或目标歌曲获取相似推荐。
- 输入参数：歌曲相关参数。
- 输出结构：`SearchResult`。
- 前置条件：通常需要歌曲对象。
- 执行逻辑：`MainShellViewModel::requestSimilarRecommendations(...)`。
- 依赖的缓存/上下文：依赖当前或目标歌曲信息。
- 成功后的状态变化：无。
- 失败语义：对象缺失、服务端失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.6.3 推荐反馈 / `submitRecommendationFeedback`

- 作用：提交推荐反馈。
- 输入参数：目标推荐对象与反馈动作。
- 输出结构：写入结果。
- 前置条件：通常需要推荐对象。
- 执行逻辑：`MainShellViewModel::submitRecommendationFeedback(...)`。
- 成功后的状态变化：推荐状态可能变化。
- 失败语义：未登录、对象缺失、服务端失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

### 4.7 本地音乐与下载域

#### 4.7.1 获取本地音乐 / `getLocalTracks`

- 作用：读取本地音乐缓存列表。
- 输入参数：无。
- 输出结构：`LocalTrackList`。
- 前置条件：无。
- 执行逻辑：`LocalMusicCache` 取列表 -> `HostStateProvider::convertLocalMusicList(...)`。
- 成功后的状态变化：无。
- 失败语义：空列表。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`、`src/agent/host/HostStateProvider.cpp`。

#### 4.7.2 添加本地音乐 / `addLocalTrack`

- 作用：导入本地音乐文件。
- 输入参数：本地文件路径。
- 输出结构：写入结果。
- 前置条件：路径必须存在且是可识别媒体文件。
- 执行逻辑：`LocalMusicCache::addLocalMusic(...)`。
- 成功后的状态变化：本地音乐列表变化。
- 失败语义：路径不存在、重复、格式不支持。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.7.3 删除本地音乐 / `removeLocalTrack`

- 作用：从本地音乐记录中移除。
- 输入参数：文件路径。
- 输出结构：写入结果。
- 前置条件：路径必须存在于本地缓存记录中。
- 执行逻辑：`LocalMusicCache::removeLocalMusic(...)`。
- 成功后的状态变化：本地音乐列表变化。
- 失败语义：对象不存在。
- 是否适合 Agent 自动执行：建议谨慎。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.7.4 获取下载任务 / `getDownloadTasks`

- 作用：查询当前下载任务列表。
- 输入参数：无。
- 输出结构：`DownloadTaskList`。
- 前置条件：无。
- 执行逻辑：读取 `DownloadManager` 任务快照。
- 成功后的状态变化：无。
- 失败语义：空列表。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.7.5 暂停下载 / `pauseDownloadTask`

- 作用：暂停指定下载任务。
- 输入参数：任务标识。
- 输出结构：写入结果。
- 前置条件：任务存在。
- 执行逻辑：调用 `DownloadManager`。
- 成功后的状态变化：任务状态切换为暂停。
- 失败语义：任务不存在。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.7.6 恢复下载 / `resumeDownloadTask`

- 作用：恢复指定下载任务。
- 输入参数：任务标识。
- 输出结构：写入结果。
- 前置条件：任务存在。
- 执行逻辑：调用 `DownloadManager`。
- 成功后的状态变化：任务恢复。
- 失败语义：任务不存在。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.7.7 取消下载 / `cancelDownloadTask`

- 作用：取消任务。
- 输入参数：任务标识。
- 输出结构：写入结果。
- 前置条件：任务存在。
- 执行逻辑：调用 `DownloadManager`。
- 成功后的状态变化：任务取消。
- 失败语义：任务不存在。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.7.8 移除下载任务 / `removeDownloadTask`

- 作用：从列表移除下载任务。
- 输入参数：任务标识。
- 输出结构：写入结果。
- 前置条件：任务存在。
- 执行逻辑：调用 `DownloadManager`。
- 成功后的状态变化：下载列表变化。
- 失败语义：任务不存在。
- 是否适合 Agent 自动执行：建议谨慎。
- 是否适合作为中间步骤：一般不是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

### 4.8 视频窗口域

#### 4.8.1 获取视频窗口状态 / `getVideoWindowState`

- 作用：获取视频窗口当前状态。
- 输入参数：无。
- 输出结构：`VideoWindowState`。
- 前置条件：最好视频窗口已创建。
- 执行逻辑：`MainWidget::agentVideoWindowState()` -> `VideoPlayerWindow::snapshot()`。
- 依赖的缓存/上下文：依赖 `MainWidget::videoPlayerWindow`。
- 成功后的状态变化：无。
- 失败语义：若视频窗口尚未创建，则 `available=false`。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/app/main_widget.cpp`、`src/video/VideoPlayerWindow.cpp`。

#### 4.8.2 播放视频 / `playVideo`

- 作用：在视频窗口中播放指定视频流。
- 输入参数：`streamUrl`。
- 输出结构：`VideoWindowState`。
- 前置条件：需要有效 `streamUrl`；当前实现里还依赖 `videoPlayerWindow` 已经存在。
- 执行逻辑：`MainWidget::agentPlayVideo(...)` -> `VideoPlayerWindow::loadVideo(...)`。
- 成功后的状态变化：视频窗口进入播放态，可能触发音视频焦点切换。
- 失败语义：若窗口未创建则失败；若流地址不可用则播放失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：通常是最终执行步骤。
- 相关代码位置：`src/app/main_widget.cpp`、`src/video/VideoPlayerWindow.cpp`。

#### 4.8.3 暂停视频 / `pauseVideoPlayback`

- 作用：暂停视频。
- 输入参数：无。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentPauseVideo()`。
- 成功后的状态变化：视频暂停。
- 失败语义：无视频窗口则失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.8.4 恢复视频 / `resumeVideoPlayback`

- 作用：恢复视频播放。
- 输入参数：无。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentResumeVideo()`。
- 成功后的状态变化：视频恢复。
- 失败语义：无视频窗口则失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.8.5 视频拖动 / `seekVideoPlayback`

- 作用：设置视频播放进度。
- 输入参数：`positionMs`。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentSeekVideo(positionMs)`。
- 成功后的状态变化：视频位置变化。
- 失败语义：无视频窗口则失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.8.6 视频全屏 / `setVideoFullScreen`

- 作用：切换视频全屏。
- 输入参数：`enabled`。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentSetVideoFullScreen(enabled)`。
- 成功后的状态变化：窗口全屏状态变化。
- 失败语义：无视频窗口则失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.8.7 视频倍速 / `setVideoPlaybackRate`

- 作用：设置视频播放倍率。
- 输入参数：`rate`。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentSetVideoPlaybackRate(rate)`。
- 成功后的状态变化：倍速变化。
- 失败语义：无视频窗口或参数不合法。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.8.8 视频画质预设 / `setVideoQualityPreset`

- 作用：设置渲染质量预设。
- 输入参数：预设值。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentSetVideoQualityPreset(...)`。
- 成功后的状态变化：渲染预设变化。
- 失败语义：无视频窗口则失败。当前能力是客户端渲染预设，不代表服务端切换真实码率流。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.8.9 关闭视频窗口 / `closeVideoWindow`

- 作用：关闭视频窗口。
- 输入参数：无。
- 输出结构：`VideoWindowState`。
- 前置条件：视频窗口已创建。
- 执行逻辑：`MainWidget::agentCloseVideoWindow()`。
- 成功后的状态变化：视频窗口关闭。
- 失败语义：无视频窗口则失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

### 4.9 桌面歌词域

#### 4.9.1 获取桌面歌词状态 / `getDesktopLyricsState`

- 作用：获取桌面歌词窗口状态。
- 输入参数：无。
- 输出结构：`DesktopLyricsState`。
- 前置条件：无。
- 执行逻辑：`MainWidget::agentDesktopLyricsState()` -> `PlayWidget::desktopLyricSnapshot()`。
- 成功后的状态变化：无。
- 失败语义：不可用时 `available=false`。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.9.2 显示桌面歌词 / `showDesktopLyrics`

- 作用：显示桌面歌词。
- 输入参数：无。
- 输出结构：`DesktopLyricsState`。
- 前置条件：无。
- 执行逻辑：`MainWidget::agentSetDesktopLyricsVisible(true)`。
- 成功后的状态变化：桌面歌词可见。
- 失败语义：能力不可用时失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.9.3 隐藏桌面歌词 / `hideDesktopLyrics`

- 作用：隐藏桌面歌词。
- 输入参数：无。
- 输出结构：`DesktopLyricsState`。
- 前置条件：无。
- 执行逻辑：`MainWidget::agentSetDesktopLyricsVisible(false)`。
- 成功后的状态变化：桌面歌词隐藏。
- 失败语义：能力不可用时失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.9.4 设置桌面歌词样式 / `setDesktopLyricsStyle`

- 作用：设置桌面歌词颜色、字号、字体等。
- 输入参数：样式 map。
- 输出结构：`DesktopLyricsState`。
- 前置条件：无。
- 执行逻辑：`MainWidget::agentSetDesktopLyricsStyle(...)` -> `DeskLrcQml::setLyricStyle(...)`。
- 成功后的状态变化：桌面歌词样式变化，且部分样式会持久化到设置。
- 失败语义：字段不合法或组件不可用。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

### 4.10 插件域

#### 4.10.1 获取插件列表 / `getPlugins`

- 作用：读取已加载插件列表。
- 输入参数：无。
- 输出结构：`PluginList`。
- 前置条件：无。
- 执行逻辑：读取 `PluginManager`。
- 成功后的状态变化：无。
- 失败语义：空列表。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.10.2 获取插件诊断 / `getPluginDiagnostics`

- 作用：获取插件诊断信息。
- 输入参数：无。
- 输出结构：`diagnostics`。
- 前置条件：无。
- 执行逻辑：读取 `PluginManager`。
- 成功后的状态变化：无。
- 失败语义：空诊断。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。

#### 4.10.3 重载插件 / `reloadPlugins`

- 作用：重新扫描并加载插件。
- 输入参数：无。
- 输出结构：新的插件列表。
- 前置条件：无。
- 执行逻辑：`PluginManager::loadPlugins()` 相关链路。
- 成功后的状态变化：插件状态刷新。
- 失败语义：扫描失败或部分插件失败。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：一般不是。

#### 4.10.4 卸载插件 / `unloadPlugin`

- 作用：卸载指定插件。
- 输入参数：插件名。
- 输出结构：结果。
- 前置条件：插件存在。
- 执行逻辑：`PluginManager::unloadPlugin(name)`。
- 成功后的状态变化：插件被移除。
- 失败语义：插件不存在、卸载失败。
- 是否适合 Agent 自动执行：建议谨慎。
- 是否适合作为中间步骤：一般不是。

#### 4.10.5 卸载全部插件 / `unloadAllPlugins`

- 作用：卸载所有插件。
- 输入参数：无。
- 输出结构：结果。
- 前置条件：无。
- 执行逻辑：`PluginManager::unloadAllPlugins()`。
- 成功后的状态变化：全部插件失活。
- 失败语义：可能出现部分卸载失败。
- 是否适合 Agent 自动执行：高风险，不建议自动执行。
- 是否适合作为中间步骤：不是。

### 4.11 设置域

#### 4.11.1 获取设置快照 / `getSettingsSnapshot`

- 作用：读取当前已开放给 Agent 的设置项。
- 输入参数：无。
- 输出结构：`SettingsSnapshot`。
- 前置条件：无。
- 执行逻辑：`AgentToolExecutor::settingsSnapshot()`。
- 成功后的状态变化：无。
- 失败语义：一般不会失败。
- 是否适合 Agent 自动执行：是。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

#### 4.11.2 更新设置 / `updateSetting`

- 作用：更新单个已开放设置项。
- 输入参数：`key`、`value`。
- 输出结构：结果及最新设置快照。
- 前置条件：必须是已开放 key。
- 当前已开放 key：`downloadPath`、`downloadLyrics`、`downloadCover`、`audioCachePath`、`logPath`、`serverHost`、`serverPort`、`playerPageStyle`。
- 执行逻辑：调用 `SettingsManager` 对应 setter。
- 成功后的状态变化：客户端设置变化，部分设置影响后续请求与渲染。
- 失败语义：`unsupported_setting`，或值类型不匹配。
- 是否适合 Agent 自动执行：建议先确认。
- 是否适合作为中间步骤：是。
- 相关代码位置：`src/agent/tool/AgentToolExecutor.cpp`。

## 5. 核心业务对象模型

### 5.1 Track

这是 Agent 编排最核心的对象。常见字段来源于 `HostStateProvider::convertMusicList(...)`、`convertHistoryList(...)`、`convertFavoriteList(...)`、`convertPlaylistDetail(...)`，通常包括：

- `trackId`
- `musicPath`
- `title`
- `artist`
- `album`
- `durationMs`
- `duration`
- `coverUrl`
- `lyricsPath`
- `isLocal`
- `songId`（部分推荐对象存在）

字段含义：

- `trackId`：客户端当前会话内的可引用标识，但不是绝对稳定主键。
- `musicPath`：真实播放路径或在线路径，是更关键的执行字段。
- `title` / `artist` / `album`：展示字段。
- `durationMs`：可用于排序、比较和位置校验。
- `coverUrl`：封面展示字段。

`trackId` 的生成与映射：

1. 常规在线/本地歌曲：由 `HostStateProvider::fallbackTrackId(path, title, artist)` 生成，本质是 `MD5(path|title|artist)`。
2. 推荐歌曲：执行器会优先使用服务端给出的 `song_id`。
3. 队列临时项：若缺少完整对象，可能退化为 `MD5(path)`。

结论：服务端不应把 `trackId` 当成全局稳定 ID。工作记忆里建议同时保存 `trackId`、`musicPath`、`title`、`artist`、`songId`（如果有）。

### 5.2 Playlist

典型字段：

- `playlistId`
- `name`
- `description`
- `coverUrl`
- `trackCount`
- `totalDurationMs`
- `updatedAt`
- `owner`

`playlistId` 是歌单域最稳定的关键对象标识。`HostStateProvider` 当前兼容 `playlist_id`、`id`、`playlistId` 三类来源，并统一归一为 `playlistId`。

### 5.3 PlaylistTrackList

这是歌单详情对象，通常包含：

- `playlist`
- `items`

其中 `playlist` 是歌单元数据，`items` 是歌单内的 `Track` 列表。它用于：

- 查看歌单内容
- 基于歌单内容继续操作
- 稳定构建播放队列

### 5.4 CurrentPlayback

来源：`HostStateProvider::currentTrackSnapshot()`。

常见字段：

- `trackId`
- `musicPath`
- `title`
- `artist`
- `album`
- `durationMs`
- `positionMs`
- `playing`
- `coverUrl`
- `playlistId`

注意：当前 `playlistId` 在 `currentTrackSnapshot()` 中常为空字符串，不能依赖它反推出“当前歌曲属于哪个歌单”。如果服务端需要当前歌单上下文，应在播放歌单或设置队列时自己维护。

### 5.5 SearchResult

统一用于搜索歌曲、歌手歌曲、推荐歌曲、相似推荐。常见字段：

- `items`
- `count`

后续可把其中的 `Track` 继续用于：`playTrack`、`addFavorite`、`addPlaylistItems`、`addToPlaybackQueue`。

### 5.6 RecentTrackList

常见字段：

- `items`
- `count`

每项依然是 `Track` 投影对象，用于查看最近播放、继续播放、加入喜欢、加入歌单。

### 5.7 PlaybackQueue

来源：`AgentToolExecutor::queueSnapshot()`。常见字段：

- `items`
- `count`
- `currentIndex`
- `playing`
- `volume`
- `playMode`
- `playModeName`

用途：供 Agent 维护当前播放世界状态，也供 `playAtIndex`、`removeFromPlaybackQueue` 等二次操作使用。

## 6. 能力依赖图与推荐调用链

### 6.1 查找歌单

推荐链路：

1. `getPlaylists`
2. 在结果中用名称做匹配
3. 拿到稳定 `playlistId`

不能直接做的事：不能只凭“这个歌单”“我的流行歌单”直接调用 `getPlaylistTracks` 或 `deletePlaylist`，必须先拿到 `playlistId`。

### 6.2 查看歌单内容

推荐链路：

1. `getPlaylists`
2. 选定歌单对象
3. `getPlaylistTracks(playlistId)`

这是查看歌单详情的标准最小链路。

### 6.3 播放歌单

当前不推荐直接用 `playPlaylist`。

稳定推荐链路：

1. `getPlaylists`
2. `getPlaylistTracks`
3. `setPlaybackQueue(items, playNow=true, startIndex=0)`

或：

1. `getPlaylists`
2. `getPlaylistTracks`
3. `setPlaybackQueue(items, playNow=false)`
4. `playAtIndex(0)`

原因：`playPlaylist` 当前实现部分可用，而 `setPlaybackQueue + playAtIndex` 更贴合当前 `AudioService` 真实支持的队列接口。

### 6.4 搜索歌曲

推荐链路：

1. `searchTracks(query)`
2. 选择候选 `Track`

这是“播放歌曲”“加入喜欢”“加入歌单”的标准前置步骤。

### 6.5 播放歌曲

推荐链路：

1. `searchTracks`
2. 选择目标 `Track`
3. `playTrack(trackId 或 musicPath)`

不能直接做的事：不应把模糊标题直接塞给 `playTrack`。`playTrack` 更适合消费结构化对象，而不是自然语言文本。

### 6.6 查询最近播放

推荐链路：

1. `getRecentTracks`
2. 根据结果继续：
   - `playTrack`
   - `addFavorite`
   - `addPlaylistItems`

### 6.7 基于已有歌单上下文继续操作

如果上一步已经做过 `getPlaylists` 或 `getPlaylistTracks`，工作记忆里应保留：

- 当前命中的 `playlistId`
- `playlist.name`
- 最近一次 `PlaylistTrackList.items`

这样后续“查看这个歌单内容”“把这首歌加到这个歌单”才成立。

### 6.8 基于已有歌曲上下文继续操作

如果上一步已经做过 `searchTracks`、`getRecentTracks`、`getFavorites`、`getPlaylistTracks` 或 `getCurrentTrack`，则工作记忆里应保留最近命中的 `Track` 对象，至少包括：

- `trackId`
- `musicPath`
- `title`
- `artist`

这样后续才能继续 `playTrack`、`addFavorite`、`addPlaylistItems`、`addToPlaybackQueue`、`getLyrics`。

## 7. 当前上下文与缓存机制

### 7.1 执行器对象缓存

`AgentToolExecutor` 内部维护三类关键缓存：

- `m_trackCacheById`
- `m_playlistMetaById`
- `m_playlistDetailById`

作用：支持后续用 `trackId`、`playlistId` 做二次操作，也支持“这个歌单”“这首歌”的会话内引用。

### 7.2 搜索结果缓存

搜索、推荐、歌手歌曲等结果会把歌曲对象写入 `m_trackCacheById`。这意味着：

- 当前会话里已经查过的歌曲，后续 `playTrack(trackId)`、`addFavorite(trackId)` 更容易成功
- 但这个缓存不是持久化缓存，应用重启或会话重置后不能假设仍然存在

### 7.3 歌单详情缓存

歌单详情查询后会写入 `m_playlistDetailById`。这对以下操作很关键：

- `getPlaylistTracks`
- `playPlaylist`
- 基于歌单内容继续加删改

### 7.4 当前播放状态缓存

当前播放状态由 `AudioService` 实时持有，`HostStateProvider::currentTrackSnapshot()` 是读取投影。这是世界状态的重要组成部分，但它并不稳定反映当前歌单来源，因此服务端应同时关注：

- `getCurrentTrack`
- `getPlaybackQueue`

### 7.5 网络层短期响应缓存

`HttpRequestV2` 当前对部分查询做了短期缓存：

- `getFavorites` 约 60 秒
- `getPlayHistory` 约 30 秒
- `getPlaylists` 约 30 秒
- `getPlaylistDetail` 约 15 秒
- `getVideoList` 约 5 分钟
- `getMusicByArtist` 约 10 分钟

这意味着写操作之后如果要立即读到新状态，需要注意缓存延迟。

### 7.6 对“这个歌单”“这首歌”的支持现状

客户端当前只是缓存层支持会话内指代，不是语言级解析支持。也就是说：

- 如果服务端自己记住最近命中的 `playlistId` 或 `trackId`
- 再用这些对象去调用后续工具
- 客户端可以正常工作

如果服务端只发送模糊自然语言，而不携带对象上下文，客户端本身不会替服务端理解“这个歌单”。

## 8. 状态变化与副作用

### 8.1 `playTrack`

副作用：

- 当前播放对象切换
- `AudioService` 当前会话切换
- 若歌曲不在队列，可能追加进队列
- UI 播放状态同步变化
- 后续 `getCurrentTrack` 和 `getPlaybackQueue` 都可能变化

### 8.2 `setPlaybackQueue`

副作用：

- 整个播放队列被重建
- 当前队列索引可能变化
- 如果 `playNow=true`，当前播放也会变化

### 8.3 `playAtIndex` / `playNext` / `playPrevious`

副作用：

- 当前播放歌曲变化
- 当前索引变化
- UI 高亮与播放状态变化

### 8.4 `pausePlayback` / `resumePlayback` / `stopPlayback`

副作用：

- 当前 `playing` 状态变化
- UI 控制栏和歌词页状态变化
- 不改变队列内容

### 8.5 `addFavorite` / `removeFavorites`

副作用：

- 喜欢列表变化
- 相关 UI 列表刷新
- 不直接改变当前播放

### 8.6 `createPlaylist` / `updatePlaylist` / `deletePlaylist`

副作用：

- 歌单总表变化
- 当前歌单页可能刷新
- 删除歌单会使对应 `playlistId` 失效

### 8.7 `addPlaylistItems` / `removePlaylistItems` / `reorderPlaylistItems`

副作用：

- 歌单详情变化
- 歌单歌曲数变化或顺序变化
- 如果服务端工作记忆中缓存了旧歌单内容，需要更新

### 8.8 `playVideo`

副作用：

- 视频窗口状态变化
- 可能触发音视频焦点切换
- 后续 `getVideoWindowState` 变化

### 8.9 `setDesktopLyricsStyle`

副作用：

- 桌面歌词样式变化
- 部分设置会持久化到配置

### 8.10 `updateSetting`

副作用：

- 客户端某些运行参数变化
- `serverHost`、`serverPort` 会影响后续请求
- `playerPageStyle` 会影响 UI 风格
- `audioCachePath`、`downloadPath` 会影响后续文件行为

## 9. 前置条件、限制和风险

### 9.1 可直接自动执行

较适合作为自动执行能力：

- `searchTracks`
- `searchArtist`
- `getTracksByArtist`
- `getLyrics`
- `getCurrentTrack`
- `getRecentTracks`（已登录前提下）
- `getFavorites`（已登录前提下）
- `getPlaylists`（已登录前提下）
- `getPlaylistTracks`
- `getRecommendations`
- `getSimilarRecommendations`
- `getPlaybackQueue`
- `getLocalTracks`
- `getDownloadTasks`
- `getVideoList`
- `getVideoStream`
- `getVideoWindowState`
- `getDesktopLyricsState`
- `getPlugins`
- `getPluginDiagnostics`
- `getSettingsSnapshot`
- `playTrack`
- `addToPlaybackQueue`
- `addFavorite`
- `pausePlayback`
- `resumePlayback`
- `playNext`
- `playPrevious`
- `setVolume`
- `setPlayMode`
- `showDesktopLyrics`
- `hideDesktopLyrics`
- `setDesktopLyricsStyle`

### 9.2 建议先确认

虽然真实可用，但存在显著副作用：

- `setPlaybackQueue`
- `removeFromPlaybackQueue`
- `clearPlaybackQueue`
- `removeFavorites`
- `createPlaylist`
- `updatePlaylist`
- `addPlaylistItems`
- `removePlaylistItems`
- `reorderPlaylistItems`
- `reloadPlugins`
- `updateSetting`
- `playVideo`
- `cancelDownloadTask`
- `removeDownloadTask`

### 9.3 高风险/当前不建议自动放开

- `deletePlaylist`
- `unloadAllPlugins`
- `playPlaylist`（原因不是破坏性，而是当前实现部分可用、语义不稳定）

### 9.4 依赖登录的能力

通过 `AgentToolExecutor::requireLogin(...)` 判断，常见包括：

- 最近播放相关
- 喜欢音乐相关
- 歌单相关
- 推荐相关

服务端不应在未登录状态下盲目调用这些工具。

### 9.5 依赖已有缓存/对象的能力

以下能力如果直接给自然语言目标，客户端并不会替 Agent 自动解析对象：

- `playTrack`
- `addFavorite`
- `addPlaylistItems`
- `removePlaylistItems`
- `playAtIndex`
- `removeFromPlaybackQueue`
- `playPlaylist`

它们更适合消费：

- `trackId`
- `musicPath`
- `playlistId`
- 队列索引

### 9.6 路径与资源有效性风险

即使对象存在，也可能出现：

- `musicPath` 可见但资源失效
- `coverUrl` 缺失
- `lyricsPath` 不可读
- 视频 `streamUrl` 已失效

因此服务端需要把“对象存在”和“底层资源可用”区分开。

## 10. 失败语义与错误处理

### 10.1 返回空列表

常见于：

- `getPlaylists`
- `getPlaylistTracks`
- `getFavorites`
- `getRecentTracks`
- `getVideoList`
- `getTracksByArtist`

可能原因：

- 业务上确实为空
- 未登录
- 请求失败后底层退化为空结果

服务端建议：如果是首次查询，可直接向用户解释为空；如果上下文显示“理论上不该为空”，可再做一次澄清或重试。

### 10.2 明确失败结果

执行器当前会明确返回的一些典型错误：

- `not_logged_in`
- `unsupported_setting`
- `track_not_found`
- `music_path_missing`
- `playlist_detail_invalid`

服务端建议：这类错误优先直接解释或改走其他链路，不要盲目重试。

### 10.3 找不到对象

常见场景：

- `trackId` 不在当前 `m_trackCacheById`
- `playlistId` 不在当前歌单缓存
- 队列索引越界

服务端建议：回退到查对象步骤，例如先 `searchTracks`，或先 `getPlaylists`。

### 10.4 播放失败

常见原因：

- `musicPath` 不可解析
- 在线路径未归一化
- 底层资源不可访问
- 当前实现的 `playPlaylist` 路径不稳定

服务端建议：对单曲播放，可尝试重新查对象；对歌单播放，优先改走 `setPlaybackQueue`。

### 10.5 参数缺失

如果工具缺少必填参数，`ToolRegistry` 会拦截参数校验。

服务端建议：这类错误应视为编排器自身问题，应修正调用，而不是重试同一无效调用。

### 10.6 缓存失效或会话对象缺失

常见场景：服务端记忆里有旧 `trackId`，但客户端本轮会话中缓存已不存在。

服务端建议：对 `trackId` 不要过度依赖跨进程复用，需要时重新调用查询工具获取新对象。

### 10.7 状态不同步

尤其需要注意：

- 当前播放对象不一定能反推出当前歌单
- 当前视频窗口状态依赖窗口是否已创建
- 网络层个别失败不一定都转成结构化失败回执

服务端建议：关键链路增加超时和校验，对强依赖状态的动作先查询状态。

## 11. 当前能力边界

### 11.1 当前真正支持的范围

当前真正可依赖的核心范围包括：

- 歌曲搜索与单曲播放
- 当前播放、播放控制、播放队列控制
- 最近播放、喜欢音乐、歌单读写
- 本地音乐列表与下载任务列表
- 视频列表查询、视频流获取、已创建视频窗口的控制
- 桌面歌词状态和样式控制
- 插件列表、插件诊断、插件重载/卸载
- 已开放设置项读写

### 11.2 当前还不支持但容易被误以为支持的范围

- 仅凭自然语言“这首歌”“这个歌单”自动解析对象
- `playPlaylist` 的稳定整歌单播放
- 视频相关能力的“无窗口冷启动播放”
- 设置项的全量读写

### 11.3 当前协议上看似有入口，但实际上没有完整落地的能力

最典型的是：`playPlaylist`。

原因：执行器里仍走 `AudioService::setPlaylist(...)`，而 `AudioService::setPlaylist(const QList<QUrl>&)` 当前为弃用 no-op。因此服务端不要把 `playPlaylist` 作为稳定原子能力。

### 11.4 当前 Agent 绝对不应该假设存在的能力

- 账户相关操作已全面开放
- 客户端能自动理解自然语言里的指代关系
- 视频画质预设代表服务端已切换真实流
- `trackId` 是全局永久主键
- 所有网络失败都会返回结构化 `tool_result`

这些假设当前都不成立。

## 12. 供服务端建模的最终结论

### 12.1 Capability Catalog 推荐分层

建议服务端按以下层次建模客户端能力：

#### A. 查询类能力（可自动执行）

- `searchTracks`
- `searchArtist`
- `getTracksByArtist`
- `getLyrics`
- `getCurrentTrack`
- `getRecentTracks`
- `getFavorites`
- `getPlaylists`
- `getPlaylistTracks`
- `getRecommendations`
- `getSimilarRecommendations`
- `getPlaybackQueue`
- `getLocalTracks`
- `getDownloadTasks`
- `getVideoList`
- `getVideoStream`
- `getVideoWindowState`
- `getDesktopLyricsState`
- `getPlugins`
- `getPluginDiagnostics`
- `getSettingsSnapshot`

#### B. 控制类能力（可自动执行，但需世界状态）

- `pausePlayback`
- `resumePlayback`
- `stopPlayback`
- `seekPlayback`
- `playNext`
- `playPrevious`
- `playAtIndex`
- `setVolume`
- `setPlayMode`
- `showDesktopLyrics`
- `hideDesktopLyrics`
- `setDesktopLyricsStyle`
- 视频窗口控制类

#### C. 计划型写操作（建议先规划，再执行）

- `playTrack`
- `setPlaybackQueue`
- `addToPlaybackQueue`
- `removeFromPlaybackQueue`
- `clearPlaybackQueue`
- `addFavorite`
- `removeFavorites`
- `createPlaylist`
- `updatePlaylist`
- `addPlaylistItems`
- `removePlaylistItems`
- `reorderPlaylistItems`
- `updateSetting`

#### D. 审批型/高风险能力

- `deletePlaylist`
- `unloadAllPlugins`
- 当前部分可用的 `playPlaylist`

### 12.2 推荐哪些能力作为自动执行能力

推荐自动执行：查询类能力、搜索类能力、歌曲级低风险操作、低风险控制类能力、低风险显示类能力。

建议默认放开的歌曲域能力包括：

- `searchTracks`
- `searchArtist`
- `getTracksByArtist`
- `getLyrics`
- `getCurrentTrack`
- `playTrack`
- `addToPlaybackQueue`
- `addFavorite`

这些能力要么是纯查询，要么是单曲级、低破坏性的操作，适合作为 Agent 的基础自动执行能力。

### 12.3 推荐哪些能力作为计划能力

适合作为计划型动作链的有：播放歌单（建议拆解）、加歌到歌单、把一批歌设成新的播放队列、批量调整播放队列、基于当前歌曲做推荐或入歌单。

这里的原则是：搜索和单曲级操作默认放开；涉及集合重写、歌单结构修改、批量副作用的动作再进入计划链。

### 12.4 推荐哪些能力必须走审批

- 删除歌单
- 卸载所有插件
- 清空播放队列
- 重写设置中的关键网络配置

### 12.5 推荐哪些对象进入工作记忆

服务端工作记忆应重点维护：

- 最近命中的 `Track`
- 最近命中的 `Playlist`
- 最近查看的 `PlaylistTrackList`
- 当前播放 `CurrentPlayback`
- 当前播放队列 `PlaybackQueue`
- 当前登录状态

对 `Track` 建议保存：`trackId`、`musicPath`、`title`、`artist`、`songId`（如存在）。

对 `Playlist` 建议保存：`playlistId`、`name`。

### 12.6 推荐哪些客户端状态应暴露给服务端做世界模型

如果后续协议继续扩展，最值得作为世界状态暴露的包括：

- 当前播放对象
- 当前播放队列
- 当前播放模式
- 当前是否有活跃视频窗口
- 当前桌面歌词状态
- 当前已登录账号
- 最近查询到的歌单列表摘要
- 最近查询到的歌单详情摘要

## 13. 关键代码索引

### 13.1 Agent 协议与会话

- `src/agent/AgentWebSocketClient.h`
- `src/agent/AgentWebSocketClient.cpp`
  - 职责：WS 连接、发送 `tool_result`、接收协议消息
- `src/agent/protocol/AgentProtocolRouter.h`
- `src/agent/protocol/AgentProtocolRouter.cpp`
  - 职责：解析协议帧
- `src/agent/AgentChatViewModel.h`
- `src/agent/AgentChatViewModel.cpp`
  - 职责：接收 `tool_call`、调用执行器、回传结果

### 13.2 工具注册与执行

- `src/agent/tool/ToolRegistry.h`
- `src/agent/tool/ToolRegistry.cpp`
  - 职责：注册全部工具、参数 schema、读写属性、审批元数据
- `src/agent/tool/AgentToolExecutor.h`
- `src/agent/tool/AgentToolExecutor.cpp`
  - 职责：核心工具执行器、对象缓存、业务桥接、结果归一

### 13.3 Host 状态投影

- `src/agent/host/HostStateProvider.h`
- `src/agent/host/HostStateProvider.cpp`
  - 职责：把 Qt 内部对象转换成 Agent 友好的结构

### 13.4 业务壳层与网络层

- `src/viewmodels/MainShellViewModel.h`
- `src/viewmodels/MainShellViewModel.cpp`
- `src/viewmodels/MainShellViewModel.connections.cpp`
  - 职责：业务请求与信号汇总
- `src/network/httprequest_v2.h`
- `src/network/httprequest_v2.cpp`
  - 职责：真实 HTTP 请求与部分缓存

### 13.5 音频播放

- `src/audio/AudioService.h`
- `src/audio/AudioService.cpp`
  - 职责：音频播放、队列、播放模式、音量、进度

### 13.6 本地音乐与下载

- `src/cache/LocalMusicCache.h`
- `src/cache/LocalMusicCache.cpp`
  - 职责：本地音乐记录
- `src/download/DownloadManager.h`
- `src/download/DownloadManager.cpp`
  - 职责：下载任务管理

### 13.7 视频窗口桥接

- `src/app/main_widget.h`
- `src/app/main_widget.cpp`
  - 职责：提供 Agent 可调用的视频/桌面歌词桥接方法
- `src/video/VideoPlayerWindow.h`
- `src/video/VideoPlayerWindow.cpp`
  - 职责：视频窗口与状态快照
- `src/ui/widgets/video/video_list_widget.cpp`
  - 职责：视频窗口创建时机，决定视频能力是否可用

### 13.8 桌面歌词

- `src/ui/widgets/playback/play_widget.h`
- `src/ui/widgets/playback/play_widget.cpp`
  - 职责：桌面歌词状态快照
- `src/ui/qml_bridge/lyrics/desklrc_qml.h`
- `src/ui/qml_bridge/lyrics/desklrc_qml.cpp`
  - 职责：桌面歌词显示与样式

### 13.9 插件与设置

- `src/plugins/PluginManager.h`
- `src/plugins/PluginManager.cpp`
  - 职责：插件加载、卸载、诊断
- `src/settings/SettingsManager.h`
- `src/settings/SettingsManager.cpp`
  - 职责：客户端设置项读写

---

这份文档的核心结论可以概括为：Qt 客户端已经具备较完整的媒体查询、播放控制、歌单操作和状态读取能力，但 Agent 服务端必须把对象获取、状态建模和风险分级放在自己这一侧完成。当前最稳定的编排原则是“先查对象，再写状态；先拿稳定 ID，再做副作用动作；能走队列能力就不要依赖部分可用的歌单直接播放能力”。

