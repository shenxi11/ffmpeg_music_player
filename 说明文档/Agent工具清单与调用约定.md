# Agent 工具清单与调用约定

## 1. 文档目的

这份文档说明当前 Qt 客户端对 Agent 暴露的工具能力、参数约束、执行落点，以及当前实现状态。  
阅读本文件时，请同时参考：

- [src/agent/tool/ToolRegistry.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/ToolRegistry.cpp)
- [src/agent/tool/AgentToolExecutor.cpp](e:/FFmpeg_whisper/ffmpeg_music_player/src/agent/tool/AgentToolExecutor.cpp)

## 2. 总体原则

### 2.1 只看注册表不够

`ToolRegistry` 表示“协议上声明了哪些工具”；  
`AgentToolExecutor` 才表示“Qt 端实际上能不能执行这个工具”。

因此工具状态分为三类：

1. 已声明且已落地
2. 已声明但未落地
3. 暂未声明

### 2.2 写操作当前采用混合回执模式

当前 Qt 端的写操作不再全部是“请求已受理”模式。  
其中一批核心写操作已经接入结果信号，可以向 Agent 返回更接近最终状态的成功/失败结果。

但仍需注意：

1. 并发写操作的关联仍主要依赖执行器内部的 pending 队列
2. 错误码语义还可以继续细化
3. 后续若服务端补充更丰富的失败信息，Qt 端还可以继续向上透传

## 3. 已声明且已落地的工具

### 3.1 查询类工具

#### `getLyrics`

- 作用：获取指定歌曲歌词
- 可选参数：`trackId`、`musicPath`
- 实际执行：
  - 优先使用 `musicPath`
  - 若未提供，则尝试通过 `trackId` 从 Qt 缓存中反查
  - 调用 `HttpRequestV2::getLyrics(...)`
- 返回：
  - `musicPath`
  - `lines`
  - `lineCount`
  - `text`

说明：  
Qt 端已经兼容普通在线路径和 HLS 路径到歌词接口的映射。

#### `getVideoList`

- 作用：获取服务器视频列表
- 参数：无
- 实际执行：
  - `HttpRequestV2::getVideoList()`
- 返回：
  - `items`
  - `count`

#### `getVideoStream`

- 作用：获取视频播放地址
- 必填参数：`videoPath`
- 实际执行：
  - `HttpRequestV2::getVideoStreamUrl(...)`
- 返回：
  - `videoPath`
  - `videoUrl`
  - `resolved`

#### `searchArtist`

- 作用：检查歌手是否存在
- 必填参数：`artist`
- 实际执行：
  - `HttpRequestV2::searchArtist(...)`
- 返回：
  - `artist`
  - `exists`

#### `getTracksByArtist`

- 作用：根据歌手获取歌曲列表
- 必填参数：`artist`
- 可选参数：`limit`
- 实际执行：
  - `HttpRequestV2::getMusicByArtist(...)`
  - `HostStateProvider::convertMusicList(...)`
- 返回：
  - `artist`
  - `items`
  - `count`

#### `searchTracks`

- 作用：按关键词搜索歌曲
- 必填参数：无
- 可选参数：`keyword`、`artist`、`album`、`limit`
- 实际执行：
  - `AgentToolExecutor::executeToolCall(...)`
  - `MainShellViewModel::searchMusic(...)`
- 返回：
  - `items`
  - `count`

说明：  
会优先用 `keyword`，如果为空则尝试 `artist`、`album`。

#### `getCurrentTrack`

- 作用：获取当前播放歌曲快照
- 参数：无
- 实际执行：
  - `HostStateProvider::currentTrackSnapshot()`
- 返回：
  - `trackId`
  - `musicPath`
  - `title`
  - `artist`
  - `durationMs`
  - `positionMs`
  - `playing`
  - `coverUrl`

#### `getRecentTracks`

- 作用：获取最近播放
- 可选参数：`limit`
- 前置条件：必须已登录
- 实际执行：
  - `MainShellViewModel::requestHistory(...)`
  - `HostStateProvider::convertHistoryList(...)`

#### `getFavorites`

- 作用：获取喜欢音乐
- 参数：无
- 前置条件：必须已登录
- 实际执行：
  - `MainShellViewModel::requestFavorites(...)`
  - `HostStateProvider::convertFavoriteList(...)`

#### `getPlaylists`

- 作用：获取用户歌单列表
- 参数：无
- 前置条件：必须已登录
- 实际执行：
  - `MainShellViewModel::requestPlaylists(...)`
  - `HostStateProvider::convertPlaylistList(...)`

#### `getPlaylistTracks`

- 作用：获取指定歌单详情与歌曲列表
- 必填参数：`playlistId`
- 前置条件：必须已登录
- 实际执行：
  - `MainShellViewModel::requestPlaylistDetail(...)`
  - `HostStateProvider::convertPlaylistDetail(...)`
- 返回：
  - `playlist`
  - `items`

#### `getRecommendations`

- 作用：获取推荐音乐
- 可选参数：`scene`、`limit`、`excludePlayed`
- 前置条件：必须已登录
- 实际执行：
  - `MainShellViewModel::requestRecommendations(...)`

#### `getSimilarRecommendations`

- 作用：获取相似推荐
- 必填参数：`songId`
- 可选参数：`limit`
- 实际执行：
  - `MainShellViewModel::requestSimilarRecommendations(...)`

#### `getPlaybackQueue`

- 作用：获取当前播放队列
- 参数：无
- 实际执行：
  - 直接从 `AudioService` 读取
- 返回：
  - `items`
  - `count`
  - `currentIndex`
  - `playing`
  - `volume`
  - `playMode`
  - `playModeName`

#### `getLocalTracks`

- 作用：获取本地音乐缓存列表
- 可选参数：`limit`
- 实际执行：
  - `HostStateProvider::convertLocalMusicList(...)`

#### `getDownloadTasks`

- 作用：获取下载任务列表
- 可选参数：`scope=all/active/completed`
- 实际执行：
  - `DownloadManager::getAllTasks/getActiveTasks/getCompletedTasks`
- 返回：
  - `items`
  - `count`
  - `activeDownloads`
  - `queueSize`

#### `getVideoWindowState`

- 作用：获取视频播放窗口状态
- 参数：无
- 实际执行：
  - 通过 `PluginHostContext` 拿到 `mainWidget` 服务
  - 调用主窗口桥接方法返回视频窗口快照

#### `getDesktopLyricsState`

- 作用：获取桌面歌词窗口状态
- 参数：无
- 实际执行：
  - 调用 `MainWidget -> PlayWidget` 桥接
- 返回：
  - `visible`
  - `songName`
  - `lyricText`
  - `color`
  - `fontSize`
  - `fontFamily`

#### `getPlugins`

- 作用：获取当前插件列表
- 参数：无
- 实际执行：
  - `PluginManager::getPluginInfos()`
  - 同步回传 `serviceKeys`

#### `getPluginDiagnostics`

- 作用：获取插件诊断信息
- 参数：无
- 实际执行：
  - `PluginManager::diagnosticsReport()`
  - `PluginManager::loadFailures()`
  - `PluginHostContext::environmentSnapshot()`

#### `getSettingsSnapshot`

- 作用：获取当前非账户类设置快照
- 参数：无
- 实际执行：
  - 读取 `SettingsManager` 中的下载、缓存、日志、服务端和播放页风格设置

### 3.2 播放控制类工具

#### `playTrack`

- 作用：播放单曲
- 可选参数：`trackId`、`musicPath`
- 实际执行：
  - 先用 `musicPath`
  - 若没有 `musicPath`，则尝试用 `trackId` 从缓存反查
  - 通过 `toPlayableUrl(...)` 转成真实可播放 URL
  - 调用 `AudioService::play(...)`

#### `playPlaylist`

- 作用：播放整张歌单
- 必填参数：`playlistId`
- 前置条件：必须已登录
- 实际执行：
  - 如无缓存，先拉歌单详情
  - 歌曲逐条转 `QUrl`
  - 调用 `AudioService::setPlaylist(...)`
  - 调用 `AudioService::playPlaylist()`

#### `pausePlayback`

- 作用：暂停播放
- 实际执行：`AudioService::pause()`

#### `resumePlayback`

- 作用：恢复播放
- 实际执行：`AudioService::resume()`

#### `stopPlayback`

- 作用：停止播放
- 实际执行：`AudioService::stop()`

#### `seekPlayback`

- 作用：跳转进度
- 必填参数：`positionMs`
- 实际执行：`AudioService::seekTo(...)`

#### `playNext`

- 作用：下一首
- 实际执行：`AudioService::playNext()`

#### `playPrevious`

- 作用：上一首
- 实际执行：`AudioService::playPrevious()`

#### `playAtIndex`

- 作用：按当前播放队列索引播放
- 必填参数：`index`
- 实际执行：`AudioService::playAtIndex(...)`

#### `setVolume`

- 作用：设置音量
- 必填参数：`volume`
- 实际执行：`AudioService::setVolume(...)`

#### `setPlayMode`

- 作用：设置播放模式
- 必填参数：`mode`
- 实际执行：`AudioService::setPlayMode(...)`

#### `setPlaybackQueue / addToPlaybackQueue / removeFromPlaybackQueue / clearPlaybackQueue`

- 作用：重置、追加、删除、清空播放队列
- 实际执行：
  - 直接操作 `AudioService` 队列
  - 返回最新队列快照

#### `playVideo`

- 作用：在视频窗口加载并播放视频
- 可选参数：`videoPath`、`videoUrl`
- 实际执行：
  - 通过 `mainWidget` 桥接到 `VideoPlayerWindow`
  - 支持相对路径补全为服务端视频 URL

#### `pauseVideoPlayback / resumeVideoPlayback / seekVideoPlayback`

- 作用：控制视频暂停、恢复和跳转
- 实际执行：
  - `MainWidget -> VideoPlayerWindow` 窗口级桥接

#### `setVideoFullScreen / setVideoPlaybackRate / setVideoQualityPreset / closeVideoWindow`

- 作用：控制视频全屏、倍速、画质预设和窗口关闭
- 实际执行：
  - `MainWidget -> VideoPlayerWindow` 窗口级桥接

#### `showDesktopLyrics / hideDesktopLyrics / setDesktopLyricsStyle`

- 作用：控制桌面歌词显示状态与样式
- 实际执行：
  - `MainWidget -> PlayWidget -> DeskLrcQml`

### 3.3 写操作类工具

#### `addLocalTrack`

- 作用：把本地文件加入本地音乐缓存列表
- 必填参数：`filePath`
- 可选参数：`fileName`、`artist`
- 执行落点：`LocalMusicCache::addMusic(...)`

#### `removeLocalTrack`

- 作用：从本地音乐缓存列表移除歌曲
- 必填参数：`filePath`
- 执行落点：`LocalMusicCache::removeMusic(...)`

#### `pauseDownloadTask / resumeDownloadTask / cancelDownloadTask / removeDownloadTask`

- 作用：下载任务控制
- 必填参数：`taskId`
- 执行落点：`DownloadManager`

#### `addRecentTrack`

- 作用：添加最近播放记录
- 必填参数：`musicPath`
- 可选参数：`title`、`artist`、`album`、`durationSec`、`isLocal`
- 执行落点：`MainShellViewModel::addPlayHistory(...)`
- 当前结果：严格成功/失败回执

#### `removeRecentTracks`

- 作用：删除最近播放记录
- 必填参数：`musicPaths`
- 执行落点：`MainShellViewModel::removeHistory(...)`
- 当前结果：严格成功/失败回执

#### `addFavorite`

- 作用：添加喜欢音乐
- 必填参数：`musicPath`
- 可选参数：`title`、`artist`、`durationSec`、`isLocal`
- 执行落点：`MainShellViewModel::addFavorite(...)`
- 当前结果：严格成功/失败回执

#### `removeFavorites`

- 作用：移除喜欢音乐
- 必填参数：`musicPaths`
- 执行落点：`MainShellViewModel::removeFavorite(...)`
- 当前结果：严格成功/失败回执

#### `createPlaylist`

- 作用：创建歌单
- 必填参数：`name`
- 可选参数：`description`、`coverPath`
- 执行落点：`MainShellViewModel::createPlaylist(...)`
- 当前结果：严格成功/失败回执

#### `updatePlaylist`

- 作用：更新歌单信息
- 必填参数：`playlistId`、`name`
- 可选参数：`description`、`coverPath`
- 执行落点：`MainShellViewModel::updatePlaylist(...)`
- 当前结果：严格成功/失败回执

#### `deletePlaylist`

- 作用：删除歌单
- 必填参数：`playlistId`
- 执行落点：`MainShellViewModel::deletePlaylist(...)`
- 当前结果：严格成功/失败回执

#### `addPlaylistItems`

- 作用：歌单加歌
- 常用参数：
  - `playlistId`
  - `trackIds`
  - 或直接给 `items`
- 执行落点：`MainShellViewModel::addPlaylistItems(...)`
- 当前结果：严格成功/失败回执

#### `removePlaylistItems`

- 作用：歌单删歌
- 常用参数：
  - `playlistId`
  - `musicPaths`
  - 或 `trackIds`
- 执行落点：`MainShellViewModel::removePlaylistItems(...)`
- 当前结果：严格成功/失败回执

#### `reorderPlaylistItems`

- 作用：歌单排序
- 常用参数：
  - `playlistId`
  - `orderedItems`
  - 或 `orderedPaths`
- 执行落点：`MainShellViewModel::reorderPlaylistItems(...)`
- 当前结果：严格成功/失败回执

#### `submitRecommendationFeedback`

- 作用：提交推荐反馈
- 必填参数：`songId`、`eventType`
- 可选参数：`scene`、`requestId`、`modelVersion`、`playMs`、`durationMs`
- 执行落点：`MainShellViewModel::submitRecommendationFeedback(...)`
- 当前结果：严格成功/失败回执

## 4. 当前扩展工具的实现说明

本轮新增并完成落地的六类能力如下：

1. 下载任务管理与进度读取
2. 本地音乐导入/删除
3. 视频播放器窗口级控制
4. 桌面歌词相关能力
5. 插件管理能力
6. 设置项读写能力

实现说明：

1. 下载任务能力直接复用 `DownloadManager`
2. 本地音乐导入/删除直接复用 `LocalMusicCache`
3. 视频与桌面歌词属于 QWidget/QQuickWidget 窗口级控制，采用 `PluginHostContext -> mainWidget` 桥接，不直接穿透到私有成员
4. 插件管理直接复用 `PluginManager`
5. 设置项当前只开放非账户类设置，账户缓存仍不向 Agent 暴露
6. 队列类能力和上述六类能力一样，已经进入可联调状态

## 5. 当前工具调用的统一约束

### 5.1 登录态约束

这些工具要求当前用户已登录：

- `getRecentTracks`
- `getFavorites`
- `getPlaylists`
- `getPlaylistTracks`
- `playPlaylist`
- 推荐相关接口
- 所有收藏、最近播放、歌单写操作

若未登录，Qt 端会返回：

- `code = not_logged_in`

### 5.2 参数约束

参数合法性由 `ToolRegistry::validateArgs(...)` 先做第一层校验：

- 必填参数存在
- 字符串参数不为空

但更细的业务校验由执行器完成，例如：

- `playlistId > 0`
- `songId` 不为空
- 搜索至少需要 `keyword / artist / album` 之一

### 5.3 路径约束

播放类工具依赖 `toPlayableUrl(...)` 做路径归一化。

当前支持三种路径：

1. 已经是 `http/https` 的完整 URL
2. 本地绝对路径
3. 相对在线路径

相对在线路径会自动按当前服务端地址补成可播放 URL。  
这一步是 Agent 播放在线歌曲能成功的关键。

## 6. 推荐的联调顺序

建议按下面顺序联调：

1. `searchTracks`
2. `playTrack`
3. `getLyrics`
4. `getRecentTracks`
5. `getPlaylists`
6. `getPlaylistTracks`
7. `playPlaylist`
8. `getVideoList`
9. `getVideoStream`
10. `searchArtist`
11. `getTracksByArtist`
12. `getDownloadTasks`
13. `getVideoWindowState`
14. `getDesktopLyricsState`
15. `getPlugins`
16. `getSettingsSnapshot`
17. `addFavorite`
18. `createPlaylist`
19. `addPlaylistItems`

这样可以先打通只读能力，再逐步验证写操作链路和窗口级控制链路。

## 7. 当前仍建议谨慎开放的能力

目前六类主要缺口已经补齐，但以下能力仍建议放在后续阶段：

1. 账户缓存、自动登录、密码与在线状态控制
2. 依赖文件选择对话框的“交互式导入”
3. 更细粒度的桌面歌词设置弹窗交互
4. 插件安装包级别的安装/删除
5. 需要审批链的批量高风险写操作编排

这些能力后续继续开放时，仍建议沿用“ToolRegistry + AgentToolExecutor + 结构化状态返回”的模式接入。
