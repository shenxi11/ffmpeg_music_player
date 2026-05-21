## Goal
根据 `提示词/视频窗口设计稿.html`，把视频播放从独立顶层窗口改成主页面内嵌页，并保留现有 `MediaSession + VideoRendererGL` 播放链路。

## Completed
- 新增 `src/ui/widgets/video/video_player_page.*`，实现 QWidget 版内嵌视频详情页。
- `VideoListWidget` 改成双态容器：默认展示 `VideoListWidgetQml`，点开视频后切到 `VideoPlayerPage`。
- 视频详情页支持：
  - 16:9 黑色视频舞台
  - 底部固定控制层
  - 返回列表按钮
  - 主窗口最大化/还原联动
  - 打开本地视频
  - 缺失元数据的降级展示
- `MainWidget` 移除了对 `VideoPlayerWindow` 顶层窗口的 ready 回调、成员持有和析构清理逻辑。
- `CMakeLists.txt` 显式注册了 `video_player_page.*`，避免当前 glob 未把新文件带入目标。
- Debug 编译通过；启动 5 秒冒烟成功，进程可正常拉起后停止。

## Changed Scope
- `src/ui/widgets/video/video_player_page.*`
- `src/ui/widgets/video/video_list_widget.*`
- `src/app/main_widget.h`
- `src/app/main_widget.cpp`
- `CMakeLists.txt`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`
- `.ai/decisions.md`

## Open Work
- 需要人工确认视频详情页视觉是否足够贴近设计稿，尤其是：
  - 控制层间距
  - 白色卡片圆角和留白
  - 返回列表按钮位置
  - 占位操作按钮是否要进一步弱化
- 需要人工验证：
  - 列表页进入详情页时的切换体验
  - 返回列表后的播放状态
  - 主窗口最大化后的视频渲染稳定性
  - 切换到其他 tab 时视频暂停是否符合预期
- 旧的 `src/video/VideoPlayerWindow.*` 目前仍在仓库中但不再被主流程使用；后续可决定是否彻底删除或保留作兼容壳。

## Risks
- 这次使用的是新的 QWidget 内嵌播放器页，不是旧 `VideoPlayerWindow` 的直接原地重构，所以视觉和交互还需要人工回归。
- `showEvent()` 目前仍会在每次重新显示视频模块时刷新列表，如果后续希望保留列表滚动位置或减少请求，需要额外优化。
- `CMakeLists.txt` 加了显式源文件注册；如果以后重新整理视频目录，记得同步更新。

## Next Entry Point
- 先从 `src/ui/widgets/video/video_player_page.cpp` 开始做 UI 微调。
- 如果要继续压缩主壳耦合，下一步看 `src/app/main_widget.cpp` 里 `handleVideoPlaybackStateChanged()` 和 `showContentPanel()` 是否还要加更细的切页暂停逻辑。
- 如果确认旧独立窗口彻底废弃，再处理 `src/video/VideoPlayerWindow.*` 的清理策略。
