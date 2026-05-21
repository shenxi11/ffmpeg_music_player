## Goal
修复热歌榜头图区域里标题与周期切换按钮互相遮挡的问题，保持现有交互和视觉风格不变。

## Completed
- 调整 `qml/components/library/RecommendMusicWidget.qml` 中热歌榜头图右侧内容布局。
- 将不稳定的 `Column + anchors.bottom` 组合改为稳定的 `ColumnLayout` 纵向排布。
- 给热歌榜标题增加宽度约束和省略策略，避免长标题压住周期按钮。
- 将周期切换按钮容器改为 `Flow`，在窄宽度下优先换行而不是覆盖标题。
- 让头图卡片高度按内容自适应增长，避免说明文案或按钮区挤压重叠。
- 运行 `qmllint qml/components/library/RecommendMusicWidget.qml`，静态检查通过。

## Changed Scope
- `qml/components/library/RecommendMusicWidget.qml`

## Open Work
- 需要实际启动客户端，确认在常用窗口宽度下头图区域没有新的垂直间距问题。
- 如果后续发现顶部标题区仍显拥挤，可继续微调 `spacing` 和按钮宽度，但当前不需要改数据流。

## Risks
- 本次仅做 QML 布局修复，没有同步做像素级视觉复核；不同 DPI 下可能还需要轻微间距微调。
- 当前主工作区本身是混合脏状态，这次修复只是叠加在现有未提交改动之上。

## Next Entry Point
- 先看 `qml/components/library/RecommendMusicWidget.qml` 里热歌榜 `header` 区域。
- 如果要继续验证，从热歌榜页头图的右侧 `hotChartHeroContent` 开始检查标题、切换按钮和底部操作按钮的实际显示。
