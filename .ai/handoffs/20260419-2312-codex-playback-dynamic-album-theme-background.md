## Goal
将展开播放页的固定背景渐变替换为基于当前专辑封面提取的主题色渐变，并继续叠加模糊封面背景，提升沉浸感。

## Completed
- 在 `src/ui/widgets/playback/play_widget.cpp` 内新增专辑主题色提取逻辑：
  - 低分辨率采样封面；
  - 选择代表性颜色桶；
  - 生成稳重克制的主题色种子。
- 新增按 `playerPageStyle` 生成动态背景渐变的逻辑，覆盖所有展开播放页样式。
- 保留并复用已有模糊封面背景图层，只改其遮罩和叠加透明度来源。
- 将展开播放页标题/艺人/场景前景色判断从“按样式固定亮暗”改为“按生成后的背景亮度动态选择”。
- 封面加载失败时继续走默认封面兜底，并回退到安全主题色。
- 对 `play_widget.cpp` / `play_widget.h` 运行了 `clang-format`。
- 在 `E:/FFmpeg_whisper/ffmpeg_music_player_build` 完成了 `Debug` 目标构建，编译通过。

## Changed Scope
- `src/ui/widgets/playback/play_widget.cpp`
- `src/ui/widgets/playback/play_widget.h`

## Open Work
- 需要实际运行客户端验证不同封面下的背景观感，重点看浅色封面、深色封面、高饱和封面和灰度封面。
- 如果样式 4 的沉浸感不够强，可以继续调高该样式的模糊层叠加透明度，但当前逻辑已统一改为“动态主题渐变 + 模糊封面”。
- 如果后续要让歌词/控制栏也进一步按背景亮度联动视觉细节，需要继续检查对应 QML 样式层。

## Risks
- 当前主题色提取是仓库内新加的轻量算法，没有引入外部颜色分析库；视觉效果稳定但不追求极致艺术化。
- 在非常复杂或极端高对比封面上，主题色可能偏保守，这是当前“优先可读性”的设计选择。
- 主工作区本身仍是混合脏状态，本次改动只是播放页内的单主题实现。

## Next Entry Point
- 从 `src/ui/widgets/playback/play_widget.cpp` 的匿名命名空间里 `StageThemeSpec`、`StageThemeColors` 和 `extractAlbumThemeColor()` 开始看。
- 背景合成入口是 `PlayWidget::onUpdateBackground()`，最终绘制入口是 `PlayWidget::rebuildStageBackgroundCache()`。
