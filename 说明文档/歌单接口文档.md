# 私有歌单接口文档

更新时间：2026-03-27

## 1. 背景

当前服务端已支持收藏、播放历史、推荐。为便于客户端新增“新建歌单”“向歌单加歌”“歌单排序”等能力，服务端在 `profile-service` 内新增私有歌单模块。

第一版范围：

1. 仅支持当前用户自己的私有歌单
2. 支持歌单创建、编辑、删除、列表、详情
3. 支持批量加歌、批量删歌、完整排序
4. 支持服务端歌曲与客户端本地歌曲同时入歌单

第一版不包含：

1. 公开歌单
2. 分享码
3. 协作编辑
4. 收藏别人歌单

## 2. 身份传递方式

沿用当前 `profile` 接口风格，二选一即可：

1. Query 参数：`user_account=root`
2. Header：`X-User-Account: root`

如果两者都没有，服务端返回 `400`。

## 3. 字段约定

### 3.1 歌单

- `name`：歌单名称，必填
- `description`：歌单简介，可选
- `cover_path`：歌单封面相对路径，可选
- `cover_url`：服务端返回给客户端的完整访问地址

### 3.2 歌单项

- `music_path`：歌曲主标识，第一版仍沿用现有系统路径
- `is_local=false`：服务端歌曲
- `is_local=true`：客户端本地歌曲
- `cover_art_path`：可选；本地歌曲如果客户端已有封面路径可传
- `cover_art_url`：服务端返回的封面访问地址

说明：

1. 服务端歌曲如果未显式传 `cover_art_path`，服务端会尝试从 `catalog.music_files` 中回填封面
2. 本地歌曲允许没有封面
3. 同一歌单内以 `music_path` 作为去重主键

## 4. 接入流程

推荐客户端流程：

1. 登录成功后拿到 `account`
2. 进入“我的歌单”页面，调用 `GET /user/playlists`
3. 用户点击“新建歌单”，调用 `POST /user/playlists`
4. 进入歌单详情页，调用 `GET /user/playlists/{playlist_id}`
5. 用户添加歌曲时，调用 `POST /user/playlists/{playlist_id}/items/add`
6. 用户删除歌曲时，调用 `POST /user/playlists/{playlist_id}/items/remove`
7. 用户拖拽排序完成后，调用 `POST /user/playlists/{playlist_id}/items/reorder`

## 5. 接口定义

### 5.1 获取歌单列表

- 方法：`GET`
- 路径：`/user/playlists`

查询参数：

- `user_account`：用户账号
- `page`：页码，默认 `1`
- `page_size`：每页数量，默认 `20`

请求示例：

```bash
curl "http://127.0.0.1:8080/user/playlists?user_account=root&page=1&page_size=20"
```

响应示例：

```json
{
  "items": [
    {
      "id": 12,
      "name": "周杰伦循环",
      "description": "写作时听",
      "cover_url": "http://127.0.0.1:8080/uploads/playlist_covers/jay.jpg",
      "track_count": 18,
      "total_duration_sec": 4230,
      "created_at": "2026-03-27 12:00:00",
      "updated_at": "2026-03-27 13:20:00"
    }
  ],
  "page": 1,
  "page_size": 20,
  "total": 1
}
```

### 5.2 创建歌单

- 方法：`POST`
- 路径：`/user/playlists`

请求示例：

```json
{
  "name": "周杰伦循环",
  "description": "写作时听",
  "cover_path": "playlist_covers/jay.jpg"
}
```

响应示例：

```json
{
  "success": true,
  "message": "创建成功",
  "playlist_id": 12
}
```

### 5.3 获取歌单详情

- 方法：`GET`
- 路径：`/user/playlists/{playlist_id}`

请求示例：

```bash
curl "http://127.0.0.1:8080/user/playlists/12?user_account=root"
```

响应示例：

```json
{
  "id": 12,
  "name": "周杰伦循环",
  "description": "写作时听",
  "cover_url": "http://127.0.0.1:8080/uploads/playlist_covers/jay.jpg",
  "track_count": 2,
  "total_duration_sec": 549,
  "created_at": "2026-03-27 12:00:00",
  "updated_at": "2026-03-27 13:20:00",
  "items": [
    {
      "id": 101,
      "position": 1,
      "music_path": "http://127.0.0.1:8080/uploads/花海/花海.mp3",
      "music_title": "花海",
      "artist": "周杰伦",
      "album": "",
      "duration_sec": 264,
      "is_local": false,
      "cover_art_url": "http://127.0.0.1:8080/uploads/covers/huahai.jpg",
      "added_at": "2026-03-27 12:01:00"
    },
    {
      "id": 102,
      "position": 2,
      "music_path": "E:/MP3/爱情废柴/爱情废柴.mp3",
      "music_title": "爱情废柴",
      "artist": "周杰伦",
      "album": "",
      "duration_sec": 285,
      "is_local": true,
      "cover_art_url": "",
      "added_at": "2026-03-27 12:01:30"
    }
  ]
}
```

### 5.4 更新歌单信息

- 方法：`POST`
- 路径：`/user/playlists/{playlist_id}/update`

请求示例：

```json
{
  "name": "周杰伦循环2",
  "description": "晚上听",
  "cover_path": "playlist_covers/jay-night.jpg"
}
```

响应示例：

```json
{
  "success": true,
  "message": "更新成功"
}
```

### 5.5 删除歌单

- 方法：`POST`
- 路径：`/user/playlists/{playlist_id}/delete`

响应示例：

```json
{
  "success": true,
  "message": "删除成功"
}
```

### 5.6 批量加歌

- 方法：`POST`
- 路径：`/user/playlists/{playlist_id}/items/add`

请求示例：

```json
{
  "items": [
    {
      "music_path": "http://127.0.0.1:8080/uploads/花海/花海.mp3",
      "music_title": "花海",
      "artist": "周杰伦",
      "album": "",
      "duration_sec": 264,
      "is_local": false
    },
    {
      "music_path": "E:/MP3/爱情废柴/爱情废柴.mp3",
      "music_title": "爱情废柴",
      "artist": "周杰伦",
      "album": "",
      "duration_sec": 285,
      "is_local": true
    }
  ]
}
```

响应示例：

```json
{
  "success": true,
  "message": "添加成功",
  "added_count": 2,
  "skipped_count": 0
}
```

重复规则：

1. 同一歌单里相同 `music_path` 不会重复写入
2. 如果请求里本身重复，也会被跳过
3. 被跳过的数量累计到 `skipped_count`

### 5.7 批量删歌

- 方法：`POST`
- 路径：`/user/playlists/{playlist_id}/items/remove`

请求示例：

```json
{
  "music_paths": [
    "http://127.0.0.1:8080/uploads/花海/花海.mp3",
    "E:/MP3/爱情废柴/爱情废柴.mp3"
  ]
}
```

响应示例：

```json
{
  "success": true,
  "message": "删除成功",
  "deleted_count": 2
}
```

### 5.8 歌单排序

- 方法：`POST`
- 路径：`/user/playlists/{playlist_id}/items/reorder`

请求示例：

```json
{
  "items": [
    { "music_path": "E:/MP3/爱情废柴/爱情废柴.mp3", "position": 1 },
    { "music_path": "http://127.0.0.1:8080/uploads/花海/花海.mp3", "position": 2 }
  ]
}
```

排序规则：

1. 必须提交完整顺序
2. `position` 必须从 `1` 开始连续递增
3. 不能遗漏当前歌单中的任何一首歌
4. 不能传入不属于该歌单的歌曲

响应示例：

```json
{
  "success": true,
  "message": "排序成功"
}
```

## 6. 常见错误与客户端处理建议

### 400

典型原因：

1. `user_account` 缺失
2. 歌单名称为空
3. 加歌列表为空
4. 删歌列表为空
5. 排序项不完整

建议：

1. 客户端直接提示用户参数不完整
2. 排序失败时不要本地强制提交错误顺序

### 404

典型原因：

1. 歌单不存在
2. 当前用户无权访问该歌单

建议：

1. 返回上一页并刷新歌单列表
2. 提示“歌单不存在或已被删除”

### 500

典型原因：

1. 数据库异常
2. 并发写入冲突

建议：

1. 客户端保留当前编辑状态
2. 提供“重试”入口

## 7. 与现有系统的关系

1. 不影响 `/user/favorites`
2. 不影响 `/user/history`
3. 不影响 `/recommendations/*`
4. 当前仍沿用 `user_account/X-User-Account` 作为身份传递方式

## 8. 后续可扩展方向

1. 从喜欢列表一键导入歌单
2. 从播放历史一键生成歌单
3. 公开歌单与分享能力
4. 协作编辑
5. 统一 `song_id` 后从 `music_path` 迁移到独立曲目标识
