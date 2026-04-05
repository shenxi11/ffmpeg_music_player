# Qt Host API 初版清单与 JSON 协议草案

## 1. 文档目的

这份文档用于定义 Qt 音乐软件向 Agent 暴露的第一版 Host API，以及对应的 `tool_call / tool_result` JSON 协议草案。

目标是让：

- Agent 端知道可以调用哪些能力
- Qt 端知道要实现哪些工具接口
- 双方在参数、返回值、错误结构上保持一致

这份文档聚焦第一阶段最小可用工具集，不追求覆盖所有业务能力。

---

## 2. 设计原则

Host API 设计必须遵循以下原则。

## 2.1 面向业务，不面向界面

工具必须是业务动作，而不是界面动作。

错误示例：

- `clickPlayButton`
- `selectPlaylistRow`
- `openFavoritePage`

正确示例：

- `playTrack`
- `searchTracks`
- `createPlaylist`

## 2.2 参数必须可验证

每个工具的输入必须：

- 字段明确
- 类型明确
- 是否必填明确

## 2.3 返回结果必须结构化

不能只返回一句自由文本。

必须让 Agent 能继续做推理和后续调用。

## 2.4 错误必须可机器识别

工具失败时，不能只返回 UI 文案。

必须包含：

- 错误码
- 错误消息
- 是否可重试

---

## 3. 协议总览

Agent 与 Qt 在工具调用阶段使用两类消息：

- `tool_call`
- `tool_result`

## 3.1 tool_call

```json
{
  "type": "tool_call",
  "toolCallId": "tool-1",
  "tool": "searchTracks",
  "args": {
    "keyword": "晴天",
    "artist": "周杰伦",
    "limit": 5
  }
}
```

字段说明：

- `type`：固定为 `tool_call`
- `toolCallId`：本次调用唯一 id
- `tool`：工具名
- `args`：工具参数对象

## 3.2 tool_result

成功时：

```json
{
  "type": "tool_result",
  "toolCallId": "tool-1",
  "ok": true,
  "result": {
    "items": [
      {
        "trackId": "track-123",
        "title": "晴天",
        "artist": "周杰伦",
        "album": "叶惠美",
        "durationMs": 269000
      }
    ]
  }
}
```

失败时：

```json
{
  "type": "tool_result",
  "toolCallId": "tool-1",
  "ok": false,
  "error": {
    "code": "track_not_found",
    "message": "未找到符合条件的歌曲",
    "retryable": false
  }
}
```

---

## 4. 通用数据模型建议

为了避免各工具返回风格混乱，建议先统一几类核心对象。

## 4.1 Track

```json
{
  "trackId": "track-123",
  "title": "晴天",
  "artist": "周杰伦",
  "album": "叶惠美",
  "durationMs": 269000,
  "isFavorite": false
}
```

建议字段：

- `trackId`
- `title`
- `artist`
- `album`
- `durationMs`
- `isFavorite`

## 4.2 Playlist

```json
{
  "playlistId": "playlist-1",
  "name": "夜跑歌单",
  "trackCount": 20
}
```

建议字段：

- `playlistId`
- `name`
- `trackCount`

## 4.3 CurrentPlayback

```json
{
  "trackId": "track-123",
  "title": "晴天",
  "artist": "周杰伦",
  "playlistId": "playlist-1",
  "positionMs": 12000,
  "durationMs": 269000,
  "playing": true
}
```

---

## 5. 第一批 Host API 清单

建议第一批只开放以下工具。

## 5.1 searchTracks

### 用途

根据条件搜索歌曲。

### 输入

```json
{
  "keyword": "晴天",
  "artist": "周杰伦",
  "album": "",
  "limit": 10
}
```

### 参数说明

- `keyword`: string，可选
- `artist`: string，可选
- `album`: string，可选
- `limit`: integer，可选，默认 10，最大建议 50

### 输出

```json
{
  "items": [
    {
      "trackId": "track-123",
      "title": "晴天",
      "artist": "周杰伦",
      "album": "叶惠美",
      "durationMs": 269000,
      "isFavorite": false
    }
  ]
}
```

### 错误建议

- `invalid_args`
- `track_not_found`
- `search_failed`

---

## 5.2 getCurrentTrack

### 用途

获取当前播放歌曲信息。

### 输入

```json
{}
```

### 输出

```json
{
  "trackId": "track-123",
  "title": "晴天",
  "artist": "周杰伦",
  "album": "叶惠美",
  "playlistId": "playlist-1",
  "positionMs": 12000,
  "durationMs": 269000,
  "playing": true
}
```

### 错误建议

- `nothing_playing`

---

## 5.3 getRecentTracks

### 用途

获取最近播放歌曲列表。

### 输入

```json
{
  "limit": 20
}
```

### 输出

```json
{
  "items": [
    {
      "trackId": "track-123",
      "title": "晴天",
      "artist": "周杰伦",
      "album": "叶惠美",
      "durationMs": 269000,
      "isFavorite": false
    }
  ]
}
```

---

## 5.4 getTopPlayedTracks

### 用途

获取最常播放歌曲列表。

### 输入

```json
{
  "limit": 20
}
```

### 输出

与 `getRecentTracks` 相同。

---

## 5.5 getPlaylists

### 用途

获取歌单列表。

### 输入

```json
{}
```

### 输出

```json
{
  "items": [
    {
      "playlistId": "playlist-1",
      "name": "夜跑歌单",
      "trackCount": 20
    }
  ]
}
```

---

## 5.6 getPlaylistTracks

### 用途

获取歌单中的歌曲列表。

### 输入

```json
{
  "playlistId": "playlist-1"
}
```

### 输出

```json
{
  "playlist": {
    "playlistId": "playlist-1",
    "name": "夜跑歌单",
    "trackCount": 20
  },
  "items": [
    {
      "trackId": "track-123",
      "title": "晴天",
      "artist": "周杰伦",
      "album": "叶惠美",
      "durationMs": 269000,
      "isFavorite": false
    }
  ]
}
```

### 错误建议

- `playlist_not_found`

---

## 5.7 playTrack

### 用途

播放指定歌曲。

### 输入

```json
{
  "trackId": "track-123"
}
```

### 输出

```json
{
  "played": true,
  "track": {
    "trackId": "track-123",
    "title": "晴天",
    "artist": "周杰伦",
    "album": "叶惠美",
    "durationMs": 269000,
    "isFavorite": false
  }
}
```

### 错误建议

- `track_not_found`
- `play_failed`

---

## 5.8 playPlaylist

### 用途

播放指定歌单。

### 输入

```json
{
  "playlistId": "playlist-1"
}
```

### 输出

```json
{
  "played": true,
  "playlist": {
    "playlistId": "playlist-1",
    "name": "夜跑歌单",
    "trackCount": 20
  }
}
```

### 错误建议

- `playlist_not_found`
- `play_failed`

---

## 5.9 createPlaylist

### 用途

创建歌单。

### 输入

```json
{
  "name": "夜跑歌单"
}
```

### 输出

```json
{
  "created": true,
  "playlist": {
    "playlistId": "playlist-1",
    "name": "夜跑歌单",
    "trackCount": 0
  }
}
```

### 错误建议

- `invalid_args`
- `playlist_already_exists`
- `create_failed`

---

## 5.10 addTracksToPlaylist

### 用途

批量把歌曲加入歌单。

### 输入

```json
{
  "playlistId": "playlist-1",
  "trackIds": ["track-1", "track-2", "track-3"]
}
```

### 输出

```json
{
  "playlistId": "playlist-1",
  "addedCount": 3,
  "skippedCount": 0
}
```

### 错误建议

- `playlist_not_found`
- `invalid_track_ids`
- `add_failed`

---

## 5.11 favoriteTracks

### 用途

批量加入喜欢。

### 输入

```json
{
  "trackIds": ["track-1", "track-2"]
}
```

### 输出

```json
{
  "updatedCount": 2
}
```

---

## 5.12 unfavoriteTracks

### 用途

批量取消喜欢。

### 输入

```json
{
  "trackIds": ["track-1", "track-2"]
}
```

### 输出

```json
{
  "updatedCount": 2
}
```

---

## 6. 错误对象统一格式

建议所有 `tool_result` 失败时统一使用：

```json
{
  "type": "tool_result",
  "toolCallId": "tool-1",
  "ok": false,
  "error": {
    "code": "playlist_not_found",
    "message": "未找到目标歌单",
    "retryable": false
  }
}
```

字段说明：

- `code`：稳定错误码，供 Agent 分支判断
- `message`：用户可读错误
- `retryable`：是否建议重试

---

## 7. Qt 端工具注册建议

Qt 端建议统一有一份工具注册表。

例如：

```cpp
struct ToolDefinition {
    QString name;
    QString description;
    QStringList requiredArgs;
    QStringList optionalArgs;
    bool readOnly;
};
```

建议注册时明确：

- 工具名
- 是否只读
- 参数校验规则
- 是否需要审批

---

## 8. 审批建议

不是所有工具都要审批。

### 可直接执行

- `searchTracks`
- `getCurrentTrack`
- `getRecentTracks`
- `getTopPlayedTracks`
- `getPlaylists`
- `getPlaylistTracks`
- `playTrack`
- `playPlaylist`

### 建议审批

- `createPlaylist`
- `addTracksToPlaylist`
- `favoriteTracks`
- `unfavoriteTracks`

尤其是批量操作时，应优先审批。

---

## 9. 推荐第一批联调顺序

不要一次性把全部工具做完。

### 第一步

先联调：

- `searchTracks`
- `getCurrentTrack`
- `playTrack`

目标：

- 让 Agent 能完成“搜歌 -> 选歌 -> 播放”

### 第二步

再联调：

- `getPlaylists`
- `createPlaylist`
- `addTracksToPlaylist`

目标：

- 让 Agent 能管理歌单

### 第三步

最后联调：

- `favoriteTracks`
- `unfavoriteTracks`

目标：

- 让 Agent 能做收藏管理

---

## 10. 一句话结论

Qt Host API 的第一版应该聚焦：

**搜歌、取当前状态、播放、歌单管理、收藏管理这几个最核心业务动作，并通过统一的 `tool_call / tool_result` JSON 协议暴露给 Agent。**

