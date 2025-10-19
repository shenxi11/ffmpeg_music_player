# Whisper 翻译插件 - 快速参考

## 插件结构

```
plugins/whisper_translate_plugin/
├── whisper_translate_plugin.h          # 插件接口实现
├── whisper_translate_plugin.cpp        # 插件实现代码
├── whisper_translate_plugin.json       # 插件元数据
├── translate_widget.h                  # 翻译窗口头文件
├── translate_widget.cpp                # 翻译窗口实现
└── CMakeLists.txt                      # 插件构建配置
```

## 插件依赖

### FFmpeg 库
- avcodec
- avformat
- avutil
- avdevice
- swscale
- swresample

### Whisper.cpp 库
- whisper
- ggml
- ggml-base
- ggml-cpu

### Qt5 库
- Qt5::Core
- Qt5::Gui
- Qt5::Widgets
- Qt5::Multimedia

## 构建输出

插件会生成到：
- Debug: `build/Debug/plugin/whisper_translate_plugin.dll`
- Release: `build/Release/plugin/whisper_translate_plugin.dll`

Whisper DLL 会自动复制到插件目录：
- `whisper.dll`
- `ggml.dll`
- `ggml-base.dll`
- `ggml-cpu.dll`

## 如何使用插件

### 在主程序中加载

```cpp
#include "plugin_manager.h"

// 加载插件窗口
QWidget* translateWidget = PluginManager::instance()->loadPluginWidget("whisper_translate_plugin", parent);

if (translateWidget) {
    // 插件加载成功，可以使用
    translateWidget->show();
} else {
    // 插件加载失败
    qWarning() << "Failed to load Whisper Translate Plugin";
}
```

### 卸载插件

```cpp
// 插件会在程序退出时自动卸载
// 或者手动卸载：
PluginManager::instance()->unloadAllPlugins();
```

## 主要修改文件

### 主 CMakeLists.txt
- ❌ 移除：Whisper 路径配置
- ❌ 移除：Whisper include 目录
- ❌ 移除：Whisper 链接目录
- ❌ 移除：translate_widget 源文件
- ❌ 移除：Whisper 库链接
- ❌ 移除：Whisper DLL 复制命令
- ✅ 添加：`add_subdirectory(plugins/whisper_translate_plugin)`

### main_widget.h
- ❌ 移除：`#include "translate_widget.h"`
- ✅ 添加：`#include "plugin_manager.h"`
- ✅ 修改：`TranslateWidget* translateWidget` → `QWidget* translateWidget`

### main_widget.cpp
- ❌ 移除：`translateWidget = new TranslateWidget(this);`
- ✅ 添加：通过 PluginManager 加载插件
- ✅ 添加：空指针检查 `if (translateWidget)`

### music_list_widget_local.h
- ❌ 移除：`#include "translate_widget.h"`

### music_list_widget_net.h
- ❌ 移除：`#include "translate_widget.h"`

## 构建步骤

1. **CMake 配置**
   ```
   打开 CMake GUI
   → Configure
   → Generate
   ```

2. **VS2022 构建**
   ```
   打开 ffmpeg_music_player.sln
   → 选择 Release 配置
   → 右键解决方案 → 清理解决方案
   → 右键解决方案 → 生成解决方案
   ```

3. **验证插件**
   ```
   检查文件：
   build/Release/plugin/whisper_translate_plugin.dll
   build/Release/plugin/whisper.dll
   build/Release/plugin/ggml.dll
   build/Release/plugin/ggml-base.dll
   build/Release/plugin/ggml-cpu.dll
   build/Release/plugin/whisper_translate_plugin.json
   ```

## 插件优势

✅ **解耦依赖**：Whisper.cpp 依赖只在插件中
✅ **独立更新**：可以独立更新插件
✅ **可选功能**：用户可选择是否加载
✅ **清晰架构**：功能模块化，便于维护

## 故障排查

### 插件加载失败
1. 检查插件 DLL 是否存在于 `plugin/` 目录
2. 检查 Whisper DLL 是否在插件目录
3. 查看 qDebug 输出的错误信息
4. 确认 `whisper_translate_plugin.json` 存在

### 编译错误
1. 确认 Whisper.cpp 路径正确
2. 确认 FFmpeg 路径正确
3. 清理后重新构建
4. 检查 CMake 配置输出

### 运行时错误
1. 检查所有 Whisper DLL 是否在插件目录
2. 检查 Whisper 模型文件路径
3. 查看控制台输出的 qWarning 信息

## 相关文档

- `WHISPER_PLUGIN_MIGRATION.md` - 详细迁移指南
- `plugins/audio_converter_plugin/` - 参考插件示例
- `plugin_interface.h` - 插件接口定义
- `plugin_manager.h` - 插件管理器
