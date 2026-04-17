# Goal
修复设置页里两个仍然存在的布局重叠问题，范围限定在 `settings-and-theme` 模块的 `qml/components/settings/Settings.qml`：
- `常规设置` 中“播放单首歌曲（搜索结果列表除外）”第一条长文案单选项重叠
- `音频设备` 中顶部设备设置表单与“本地文件使用内存模式”选项发生覆盖

# Completed
- 将“播放单首歌曲”这一组的单选 delegate 从固定 `height: 24` 改成内容驱动高度。
- 给长文案 `Text` 增加了 `Layout.fillWidth + wrapMode: Text.WordWrap`，不再把文本硬塞进单行。
- 将音频设备顶部四行下拉改成基于 `RowLayout` 和控件 `implicitHeight` 的稳定表单行。
- 将“本地文件使用内存模式 / 无缝播放”改成可换行、内容驱动高度的复选项块。
- 将异常字符串 `本地文件使用内存 模式` 规范为 `本地文件使用内存模式（占用较大内存，拥有较低延迟）`。
- `DSD优选模式` 一行也改成同样的内容驱动布局，避免继续依赖脆弱的固定高度。
- 已完成 `cmake --build --preset vs2022-x64-debug --target ffmpeg_music_player` 构建验证。

# Changed Scope
- `qml/components/settings/Settings.qml`
- `.ai/ownership.yaml`
- `.ai/tasks.yaml`

# Open Work
- 需要实际打开客户端，对这两个位置做一轮运行态视觉确认。
- 如果主窗口被进一步缩窄，还可以继续评估是否需要把“优先播放品质”网格也改成更强的自适应布局，但这不属于本次修复范围。

# Risks
- 这次验证到的是编译和静态布局逻辑，未在当前会话内做运行态截图复核。
- `Settings.qml` 仍然是大文件，后续如果继续加复杂控件，建议抽私有子组件避免再次出现固定高度堆叠问题。

# Next Entry Point
- 先启动客户端并打开“设置 -> 常规设置 / 音频设备”确认两处重叠是否完全消失。
- 若仍有局部视觉问题，从 `qml/components/settings/Settings.qml` 中 `singleTrackQueueOptions` 和 `audioPage -> audioDeviceBlock` 两处开始继续微调。
