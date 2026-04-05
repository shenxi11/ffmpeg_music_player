# AI 理解软件运行逻辑说明

## 1. 文档目的

这份文档用于帮助 Agent 端、客户端联调同学或后续接手的 AI 快速理解「云音乐」Qt 客户端的实际运行逻辑。  
这里不追求把全部代码逐行解释，而是把 AI 真正需要理解的四件事讲清楚：

1. 软件当前有哪些可控能力
2. 当前软件处于什么状态
3. Agent 发出的动作如何落到真实业务链路
4. 哪些地方已经可用，哪些地方只是预留接口

## 2. 整体运行链路

当前 Agent 与客户端的协作方式，不是“AI 直接点按钮”，而是“协议消息驱动业务层”。

完整链路如下：

1. Agent 通过 WebSocket 向 Qt 客户端发送 `tool_call`
2. 协议层解析消息并上抛给聊天视图模型
3. 聊天视图模型校验工具模式、执行器状态
4. 工具执行器把工具调用映射到客户端已有业务入口
5. 客户端业务层执行搜索、查询、播放、歌单操作
6. Qt 端把结构化结果通过 `tool_result` 回传给 Agent

核心入口代码：

- [src/agent/AgentChatViewModel.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/AgentChatViewModel.cpp)
- [src/agent/AgentWebSocketClient.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/AgentWebSocketClient.cpp)
- [src/agent/protocol/AgentProtocolRouter.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/protocol/AgentProtocolRouter.cpp)
- [src/agent/tool/AgentToolExecutor.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/AgentToolExecutor.cpp)

## 3. 关键模块分工

### 3.1 AgentChatViewModel

职责：负责“聊天协议协调”和“工具调用调度”。

关键点：

- 接收 `session_ready / assistant_start / assistant_chunk / assistant_final / tool_call`
- 在 `onToolCallReceived(...)` 中把工具请求交给 `AgentToolExecutor`
- 收到工具结果后调用 `AgentWebSocketClient::sendToolResult(...)` 回传给 Agent

对应代码：

- [src/agent/AgentChatViewModel.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/AgentChatViewModel.cpp)

这意味着：  
AgentChatViewModel 是“协议调度层”，它不直接负责音乐业务，也不直接改 UI 列表数据。

### 3.2 ToolRegistry

职责：描述“Qt 端宣称支持哪些工具，以及每个工具的参数约束”。

对应代码：

- [src/agent/tool/ToolRegistry.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/ToolRegistry.cpp)

这里定义了：

- 工具名
- 工具说明
- 必填参数
- 可选参数
- 是否只读
- 是否需要审批

注意：  
`ToolRegistry` 的存在只代表“协议层已声明这项能力”，不代表 `AgentToolExecutor` 一定已经实现了完整执行逻辑。  
实际可用性必须以执行器是否落地为准。

### 3.3 HostStateProvider

职责：把客户端当前状态与已有数据结构，统一整理成 Agent 可以稳定消费的结构。

对应代码：

- [src/agent/host/HostStateProvider.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/host/HostStateProvider.cpp)

它主要负责：

- 输出当前播放歌曲快照 `currentTrackSnapshot()`
- 把搜索结果 `QList<Music>` 转成标准歌曲列表
- 把最近播放、喜欢音乐、歌单列表、歌单详情转成统一结构
- 为歌曲补齐兜底 `trackId`
- 对时长、路径、封面字段做统一整理

这层是 Agent 理解“当前客户端状态”的关键基础。  
没有这层，Agent 看到的会是 Qt 内部散乱字段，而不是稳定的数据模型。

### 3.4 AgentToolExecutor

职责：把 Agent 发来的工具调用，映射到客户端现有业务层或播放服务。

对应代码：

- [src/agent/tool/AgentToolExecutor.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/AgentToolExecutor.cpp)

它主要连接两类对象：

1. `MainShellViewModel`
2. `AudioService`

#### 面向 MainShellViewModel 的调用

适用于：

- 搜索歌曲
- 查询最近播放
- 查询喜欢音乐
- 查询歌单列表
- 查询歌单详情
- 推荐音乐查询
- 歌单增删改
- 收藏与最近播放写操作

对应类：

- [src/viewmodels/MainShellViewModel.h](e:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.h)

#### 面向 AudioService 的调用

适用于：

- 播放单曲
- 暂停/恢复/停止
- 播放上一首/下一首
- 跳转进度
- 设置音量
- 播放整个歌单
- 读取当前播放队列

对应类：

- [src/audio/AudioService.h](e:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.h)

## 4. 当前软件状态是如何暴露给 AI 的

AI 真正依赖的是“状态快照”，而不是界面外观。

Qt 端当前对 Agent 暴露的主要状态包括：

### 4.1 当前播放状态

来源：

- `AudioService::currentSession()`
- `AudioService::currentUrl()`
- `AudioService::isPlaying()`

由 `HostStateProvider::currentTrackSnapshot()` 统一整理后输出：

- `trackId`
- `musicPath`
- `title`
- `artist`
- `durationMs`
- `positionMs`
- `playing`
- `coverUrl`

### 4.2 歌曲列表类状态

来源：

- 搜索结果 `QList<Music>`
- 最近播放 `QVariantList`
- 喜欢音乐 `QVariantList`
- 歌单详情 `QVariantMap`
- 本地音乐缓存 `LocalMusicCache`

这些数据都被整理成尽量统一的字段：

- `trackId`
- `musicPath`
- `title`
- `artist`
- `album`
- `durationMs`
- `coverUrl`
- `isLocal`

### 4.3 歌单状态

歌单列表与歌单详情会被拆成两层：

1. 歌单元信息
2. 歌单歌曲项

统一字段重点包括：

- `playlistId`
- `name`
- `trackCount`
- `description`
- `coverUrl`

## 5. Agent 动作是如何落到真实业务逻辑上的

下面给出几条最重要的运行主线。

### 5.1 搜索歌曲

运行过程：

1. Agent 发 `searchTracks`
2. `AgentToolExecutor` 调用 `MainShellViewModel::searchMusic(...)`
3. 网络层返回搜索结果
4. `MainShellViewModel::searchResultsReady(...)` 发信号
5. `AgentToolExecutor::onSearchResultsReady(...)` 转成标准歌曲结构
6. 结果通过 `tool_result` 回给 Agent

### 5.2 查询最近播放

运行过程：

1. Agent 发 `getRecentTracks`
2. 执行器先检查是否已登录
3. 调用 `MainShellViewModel::requestHistory(...)`
4. `historyListReady(...)` 返回后，经 `HostStateProvider::convertHistoryList(...)` 标准化
5. 工具结果回传给 Agent

### 5.3 查询歌单与歌单内容

运行过程：

1. Agent 发 `getPlaylists`
2. 执行器调用 `MainShellViewModel::requestPlaylists(...)`
3. 返回后用 `convertPlaylistList(...)` 统一歌单元信息
4. 若 Agent 再发 `getPlaylistTracks`
5. 执行器调用 `requestPlaylistDetail(...)`
6. 返回后用 `convertPlaylistDetail(...)` 输出歌单详情与歌曲列表

这里还有一层缓存：

- `m_playlistMetaById`
- `m_playlistDetailById`

目的是让 Agent 在“查看这个歌单内容”这类上下文指令里，可以复用前一步查到的歌单信息。

### 5.4 播放歌曲

运行过程：

1. Agent 发 `playTrack`
2. 执行器优先使用 `musicPath`，否则尝试用 `trackId` 从缓存中反查歌曲
3. `toPlayableUrl(...)` 把相对路径、在线路径、本地绝对路径统一转成可播放 URL
4. 调用 `AudioService::play(...)`
5. 播放结果通过 `tool_result` 返回

这里有一个非常关键的实现点：  
在线歌曲不一定直接给完整 URL，Qt 端会在 `toPlayableUrl(...)` 中根据当前服务端地址补成可播放地址。

### 5.5 播放歌单

运行过程：

1. Agent 发 `playPlaylist`
2. 如果本地已有歌单详情缓存，则直接取出全部歌曲
3. 如果没有缓存，则先请求歌单详情
4. 将歌单歌曲逐首转换成 `QUrl`
5. 调用 `AudioService::setPlaylist(...)`
6. 再调用 `AudioService::playPlaylist()`

也就是说：  
“播放歌单”不是服务端直接下发一个播放列表对象，而是 Qt 客户端自己把歌单详情转换成播放队列。

## 6. 让 AI 真正理解软件时，最重要的约束

### 6.1 账户相关能力当前不应对 Agent 开放

当前策略是：

- 登录、注册、密码、账号安全等操作，不作为 Agent 的自动化能力暴露
- 原因是这些操作涉及身份与安全边界

### 6.2 工具可用性要以“注册表 + 执行器”共同判断

当前队列控制相关能力已经在执行器中落地，包括：

- `setPlayMode`
- `setPlaybackQueue`
- `addToPlaybackQueue`
- `removeFromPlaybackQueue`
- `clearPlaybackQueue`

但原则上，后续 Agent 端接协议时，仍然必须同时参考：

1. `ToolRegistry.cpp`
2. `AgentToolExecutor.cpp`

不能只看注册表。  
因为未来新增工具时，仍可能出现“注册表已声明，但执行器还未完全跟上”的阶段性状态。

### 6.3 写操作目前是“混合回执模式”

当前 Qt 端写操作分为两类：

1. 已经升级为严格成功/失败回执
2. 仍然保留受理型回执或需要继续细化错误语义

当前已经接入结果信号回执的典型写操作包括：

- 添加喜欢音乐
- 移除喜欢音乐
- 添加最近播放
- 删除最近播放
- 创建歌单
- 更新歌单
- 删除歌单
- 歌单加歌
- 歌单删歌
- 歌单排序
- 提交推荐反馈

这意味着 Agent 在这些操作上，已经可以根据 Qt 返回的最终结果区分成功与失败，而不是只拿到 `accepted = true`。

不过从整体设计上看，后续仍建议继续完善：

1. 更细粒度的失败码
2. 并发写操作下的更强关联能力
3. 服务端业务错误语义的进一步透传

## 7. AI 想理解这个软件，最该看的代码顺序

建议按这个顺序阅读：

1. [src/agent/tool/ToolRegistry.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/ToolRegistry.cpp)
2. [src/agent/tool/AgentToolExecutor.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/AgentToolExecutor.cpp)
3. [src/agent/host/HostStateProvider.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/host/HostStateProvider.cpp)
4. [src/viewmodels/MainShellViewModel.h](e:/FFmpeg_whisper/ffmpeg_music_player/src/viewmodels/MainShellViewModel.h)
5. [src/audio/AudioService.h](e:/FFmpeg_whisper/ffmpeg_music_player/src/audio/AudioService.h)
6. [src/agent/AgentChatViewModel.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/AgentChatViewModel.cpp)

这个顺序的原因是：

- 先理解“暴露了什么”
- 再理解“怎么执行”
- 再理解“状态怎么表达”
- 最后再看“协议怎么接起来”

## 8. 后续建议

如果希望 AI 后续不只是“会调用”，而是真正具备“稳定理解软件运行逻辑”的能力，建议继续补齐三类资产：

1. 工具清单文档：明确每个工具的入参、回参、边界与当前实现状态
2. 典型任务示例：把用户意图到工具调用链路串起来
3. 失败语义表：明确未登录、参数非法、路径非法、歌单为空、服务端失败等错误的标准语义

这三者补齐后，Agent 理解软件的稳定性会明显提升。

补充说明：  
当前客户端除了播放、歌单、收藏、最近播放、推荐之外，还已经向 Agent 暴露了歌词、视频列表、视频流地址、歌手搜索、按歌手查歌、下载任务、本地音乐导入/删除、视频窗口级控制、桌面歌词、插件管理和非账户类设置读写能力。  
当前仍未开放给 Agent 的重点区域主要是账户安全相关能力，以及依赖文件选择器或审批链的高风险交互式操作。
