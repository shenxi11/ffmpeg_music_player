# Agent 典型任务链路示例

## 1. 文档目的

这份文档用于说明：当用户提出自然语言指令时，Agent 应如何理解当前 Qt 音乐客户端的运行逻辑，并组织成合适的工具调用链。

它不是协议文档，而是“任务执行视角”的说明文档。

## 2. 示例一：播放《爱情废柴》

### 用户意图

用户说：

`播放爱情废柴`

### 推荐调用链

1. 调用 `searchTracks(keyword = "爱情废柴", limit = 10)`
2. 从返回结果中挑选最匹配歌曲
3. 调用 `playTrack(trackId = ..., musicPath = ...)`

### Qt 端内部实际发生的事

1. `AgentToolExecutor` 调用 `MainShellViewModel::searchMusic(...)`
2. 搜索结果回到 `searchResultsReady(...)`
3. 结果经 `HostStateProvider::convertMusicList(...)` 标准化
4. 选中目标歌曲后，`playTrack` 调用 `toPlayableUrl(...)`
5. 在线相对路径被补成完整 URL
6. `AudioService::play(...)` 启动播放

### Agent 需要知道的关键点

1. 不建议直接盲播文件名，应先搜索再播
2. `trackId` 更适合做后续上下文引用
3. `musicPath` 才是最终落到播放器的真实路径依据

## 3. 示例二：看看我最近听了什么

### 用户意图

用户说：

`最近听了什么`

### 推荐调用链

1. 调用 `getRecentTracks(limit = 10)`

### Qt 端内部实际发生的事

1. 执行器先检查当前登录态
2. 调用 `MainShellViewModel::requestHistory(...)`
3. 最近播放列表通过 `historyListReady(...)` 返回
4. `HostStateProvider::convertHistoryList(...)` 统一字段
5. 工具结果返回给 Agent

### Agent 需要知道的关键点

1. 这是登录态能力，未登录应先给用户解释
2. 返回结果已经是标准歌曲结构，可直接继续做“播放这首”“加到歌单”之类的动作

## 4. 示例三：查询流行歌单

### 用户意图

用户说：

`查询我的歌单`

或者：

`看一下流行歌单`

### 推荐调用链

1. 调用 `getPlaylists()`
2. 在返回列表中找名为“流行”的歌单

### Qt 端内部实际发生的事

1. 执行器调用 `MainShellViewModel::requestPlaylists(...)`
2. 结果通过 `playlistsListReady(...)` 回来
3. `HostStateProvider::convertPlaylistList(...)` 生成标准歌单元信息
4. 执行器会把歌单元信息缓存到 `m_playlistMetaById`

### Agent 需要知道的关键点

1. 一次 `getPlaylists` 之后，Qt 端已经缓存了歌单 ID 与名称映射
2. 后面用户说“这个歌单”时，可以优先引用刚才命中的歌单

## 5. 示例四：查看这个歌单内容

### 用户意图

用户说：

`查看这个歌单内容`

### 推荐调用链

1. 前提：上一轮已经拿到目标歌单 `playlistId`
2. 调用 `getPlaylistTracks(playlistId = ...)`

### Qt 端内部实际发生的事

1. 若 `m_playlistDetailById` 已有缓存，则直接返回
2. 否则调用 `MainShellViewModel::requestPlaylistDetail(...)`
3. 结果通过 `playlistDetailReady(...)` 返回
4. `HostStateProvider::convertPlaylistDetail(...)` 生成：
   - `playlist`
   - `items`
5. 执行器会缓存歌单详情与歌曲项

### Agent 需要知道的关键点

1. “这个歌单”必须依赖上一轮上下文中的歌单 ID
2. 歌单详情返回后，里面的每首歌也会进入执行器缓存
3. 后续可以直接执行“播放第二首”“把这首加到收藏”

## 6. 示例五：播放这个歌单

### 用户意图

用户说：

`播放这个歌单`

### 推荐调用链

1. 前提：上一轮已确定 `playlistId`
2. 调用 `playPlaylist(playlistId = ...)`

### Qt 端内部实际发生的事

1. 若无歌单详情缓存，则先拉取歌单详情
2. 详情中的每首歌都被转成可播放 URL
3. 执行器调用：
   - `AudioService::setPlaylist(urls)`
   - `AudioService::playPlaylist()`

### Agent 需要知道的关键点

1. 播放歌单依赖“歌单详情 -> 播放队列”的转换
2. 若歌单为空，会收到 `playlist_empty`
3. 返回成功时，会同时附带 `playlist` 元信息

## 7. 示例六：把最近播放的一首歌加入歌单

### 用户意图

用户说：

`把我最近听的第一首歌加到流行歌单`

### 推荐调用链

1. 调用 `getRecentTracks(limit = 10)`
2. 确定第一首歌的 `trackId / musicPath / title / artist`
3. 调用 `getPlaylists()`
4. 找到“流行”歌单的 `playlistId`
5. 调用 `addPlaylistItems(playlistId = ..., trackIds = [...])`

### Qt 端内部实际发生的事

1. 最近播放先进入标准歌曲缓存
2. 歌单列表进入歌单元信息缓存
3. `addPlaylistItems` 会优先尝试从 `trackIds` 反查缓存中的歌曲信息，拼成请求项
4. `MainShellViewModel::addPlaylistItems(...)` 发起请求

### Agent 需要知道的关键点

1. 这类跨列表组合动作，最重要的是先拿结构化列表，再做引用
2. 不建议直接靠歌曲标题字符串做最终写操作
3. 当前返回一般是 `accepted = true`，并非严格成功回执

## 8. 示例七：我想听和这首歌相似的内容

### 用户意图

用户说：

`推荐几首和这首歌相似的`

### 推荐调用链

1. 前提：当前上下文里已知歌曲 `songId`
2. 调用 `getSimilarRecommendations(songId = ..., limit = 6)`

### Qt 端内部实际发生的事

1. 调用 `MainShellViewModel::requestSimilarRecommendations(...)`
2. 返回推荐结果
3. 执行器将推荐项整理为标准歌曲结构

### Agent 需要知道的关键点

1. 这里要求的是 `songId`，不是 `trackId`
2. 推荐结果可以继续衔接到 `playTrack`

## 9. 示例八：暂停、恢复、跳转进度

### 用户意图

用户可能说：

- `暂停音乐`
- `继续播放`
- `快进到两分钟`

### 推荐调用链

1. `pausePlayback()`
2. `resumePlayback()`
3. `seekPlayback(positionMs = 120000)`

### Qt 端内部实际发生的事

这些控制都直接由 `AudioService` 处理，不经过网络层。

### Agent 需要知道的关键点

1. 这是最稳定的一类工具
2. 不依赖登录态
3. 更适合做低风险自动执行

## 10. 对 Agent 规划任务时的建议

### 10.1 先查后写

对于以下动作，不建议直接执行：

- 把某首歌加入歌单
- 删除某个歌单里的某首歌
- 播放“这个歌单”

推荐先执行查询，再做写操作或播放操作。

### 10.2 优先用结构化标识，而不是自然语言字符串

优先级建议：

1. `playlistId`
2. `trackId`
3. `songId`
4. `musicPath`
5. 纯标题/歌单名字符串

### 10.3 对写操作保持谨慎

因为当前一部分写操作仍然是“accepted 模式”，所以 Agent 在回复用户时，建议措辞为：

- `已为你发起添加请求`
- `已提交删除操作`

不要过早说成：

- `已经成功删除`
- `已经成功加歌`

## 11. 推荐作为联调基线的四个场景

建议优先验证这四个场景：

1. `播放爱情废柴`
2. `最近听了什么`
3. `查看流行歌单内容`
4. `播放这个歌单`

这四个场景覆盖了：

- 搜索
- 列表查询
- 上下文引用
- 播放控制
- 歌单详情缓存

只要这四条链路稳定，Agent 对 Qt 客户端的基础理解就算建立起来了。
