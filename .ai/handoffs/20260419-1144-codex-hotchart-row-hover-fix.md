## Goal
修复热歌榜列表项鼠标悬浮状态失效或不稳定的问题，保证整行背景高亮和操作按钮显示能够持续生效。

## Completed
- 调整 `qml/components/library/RecommendMusicWidget.qml` 中热歌榜行项的 hover 实现。
- 将整行 hover 来源从 `HoverHandler` 改为专用的 `MouseArea` 悬浮传感器。
- 将双击播放从整行 `MouseArea` 中拆出，改为独立 `TapHandler`，避免 hover 与点击职责混杂。
- 去掉 hover 时标题区域宽度动态切换，避免操作区出现时触发布局重排导致 hover 抖动。
- 保留 `SongCoverAction.interactionActive` 与 `SongActionStrip.interactionActive`，让鼠标移动到封面或操作按钮时整行 hover 继续成立。
- 运行 `qmllint qml/components/library/RecommendMusicWidget.qml`，静态检查通过。

## Changed Scope
- `qml/components/library/RecommendMusicWidget.qml`

## Open Work
- 需要实际运行客户端，在热歌榜页验证快速横向移动鼠标、移入封面、移入操作按钮时 hover 是否持续稳定。
- 如果仍有轻微闪烁，可继续把热歌榜歌曲信息区从 `Row` 改成更显式的固定列布局，但当前先不扩改。

## Risks
- 本次只修了热歌榜行项 hover 机制，没有同步做整页像素级视觉回归。
- 当前主工作区仍有大量未提交混合改动，这次修复是叠加在现有脏工作区上的单点修补。

## Next Entry Point
- 从 `qml/components/library/RecommendMusicWidget.qml` 的热歌榜 `delegate: Rectangle { id: hotRow ... }` 开始看。
- 重点检查 `rowHoverArea`、`TapHandler`、`actionStrip.opacity/enabled` 和歌曲信息区宽度分配。
