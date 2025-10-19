# Whisper 翻译插件迁移指南

## 概述
将 `TranslateWidget` 从主项目迁移到独立插件 `whisper_translate_plugin`。

## 已完成的工作

### 1. 创建插件结构
- ✅ 创建目录：`plugins/whisper_translate_plugin/`
- ✅ 创建插件文件：
  - `whisper_translate_plugin.h` - 插件接口实现
  - `whisper_translate_plugin.cpp` - 插件实现
  - `whisper_translate_plugin.json` - 插件元数据
  - `translate_widget.h` - 翻译窗口头文件（从主项目复制）
  - `translate_widget.cpp` - 翻译窗口实现（从主项目复制）
  - `CMakeLists.txt` - 插件构建配置

### 2. 修改主项目 CMakeLists.txt
- ✅ 移除 Whisper.cpp 路径配置
- ✅ 移除 Whisper include 目录
- ✅ 移除 Whisper 链接目录
- ✅ 移除 translate_widget 源文件
- ✅ 移除 Whisper 库链接（whisper, ggml, ggml-base, ggml-cpu）
- ✅ 移除 Whisper DLL 复制命令
- ✅ 添加插件子目录：`add_subdirectory(plugins/whisper_translate_plugin)`

### 3. 插件配置
- ✅ 插件包含所有 Whisper 依赖
- ✅ Whisper DLL 会自动复制到插件目录
- ✅ 插件输出到：`build/Debug/plugin/` 和 `build/Release/plugin/`

## 需要手动完成的工作

### 1. 修改 main_widget.h
移除 `translate_widget.h` 的包含，改为通过插件系统加载：

```cpp
// 移除这一行：
#include "translate_widget.h"

// 添加：
#include "plugin_manager.h"

// 将成员变量改为：
private:
    QWidget* translateWidget;  // 不再直接使用 TranslateWidget 类型
```

### 2. 修改 main_widget.cpp
使用插件系统加载 Whisper 翻译插件：

```cpp
// 在构造函数中，替换：
// translateWidget = new TranslateWidget(this);

// 改为：
translateWidget = PluginManager::loadPlugin("whisper_translate_plugin", this);
if (translateWidget) {
    translateWidget->setFixedSize(800, 400);
    translateWidget->move(main_list->pos());
    translateWidget->hide();
} else {
    qWarning() << "Failed to load Whisper Translate Plugin";
    translateWidget = nullptr;
}

// 在所有使用 translateWidget 的地方添加空指针检查：
if (translateWidget) {
    translateWidget->show();  // 或 hide()
}
```

### 3. 修改 music_list_widget_local.h 和 music_list_widget_net.h
如果这些文件也引用了 `translate_widget.h`，需要移除引用或通过信号槽机制通信。

### 4. 检查 take_pcm.cpp 和 take_pcm.h
`take_pcm` 文件被插件使用，需要确保：
- 主项目仍然保留这些文件（因为可能被其他地方使用）
- 插件的 CMakeLists.txt 已包含这些文件的引用

## 构建步骤

### 1. 在 CMake GUI 中重新配置
```
1. 打开 CMake GUI
2. 点击 "Configure"
3. 点击 "Generate"
```

### 2. 在 VS2022 中构建
```
1. 打开 ffmpeg_music_player.sln
2. 选择 Release 配置
3. 右键解决方案 → "清理解决方案"
4. 右键解决方案 → "生成解决方案"
```

### 3. 验证插件生成
检查以下文件是否存在：
- `build/Release/plugin/whisper_translate_plugin.dll`
- `build/Release/plugin/whisper.dll`
- `build/Release/plugin/ggml.dll`
- `build/Release/plugin/ggml-base.dll`
- `build/Release/plugin/ggml-cpu.dll`
- `build/Release/plugin/whisper_translate_plugin.json`

## 插件优势

### 1. 解耦依赖
- Whisper.cpp 依赖只存在于插件中
- 主项目不再依赖 Whisper 库
- 减小主程序体积

### 2. 独立更新
- 可以独立更新 Whisper 插件
- 不影响主程序

### 3. 可选功能
- 用户可以选择是否加载 Whisper 功能
- 如果没有 Whisper 模型，插件可以选择不加载

### 4. 清晰的架构
- 功能模块化
- 便于维护和测试

## 注意事项

1. **DLL 依赖**：Whisper DLL 现在在 `plugin/` 目录下，不在主程序目录
2. **模型文件**：Whisper 模型文件路径可能需要调整
3. **错误处理**：需要处理插件加载失败的情况
4. **线程安全**：插件中的线程管理需要注意（已在 translate_widget 中实现）

## 测试清单

- [ ] 主程序能正常启动
- [ ] 能正常加载 Whisper 翻译插件
- [ ] 翻译功能正常工作
- [ ] 关闭程序时插件正确卸载
- [ ] 没有 Whisper 插件时主程序仍能正常运行
