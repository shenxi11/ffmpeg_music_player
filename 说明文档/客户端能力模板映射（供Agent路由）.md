# 客户端能力模板映射（当前版）

## 1. 文档目标
本文档用于给 Agent 路由层提供“任务意图 -> 客户端能力模板”的当前版本映射。

## 2. 当前推荐模板
| 意图类型 | 推荐模板 | 当前主要落点 |
|---|---|---|
| 搜索歌曲 | `search_tracks` | 在线搜索、歌手搜索、推荐检索 |
| 播放控制 | `playback_control` | `PlaybackViewModel` / `AudioService` |
| 队列管理 | `queue_management` | `PlaybackViewModel` 队列接口 |
| 收藏 / 历史 | `library_write` | `MainShellViewModel` |
| 歌单管理 | `playlist_management` | `MainShellViewModel` + 歌单接口 |
| 视频浏览 | `video_browse` | `VideoListViewModel` |
| 只读状态查询 | `state_snapshot` | `HostStateProvider` / 当前快照 |

## 3. 当前不建议映射成模板的主题
- 欢迎页模式切换
- 用户资料修改
- 文件选择类动作
- 需要人工确认的高副作用批量操作

## 4. 当前版本结论
模板映射应优先围绕“搜索、播放、队列、歌单、收藏/历史、视频浏览”这些已经稳定落地的业务面，不应将尚未完全抽象好的资料链或欢迎页链强行模板化。