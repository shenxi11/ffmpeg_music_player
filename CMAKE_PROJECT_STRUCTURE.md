# CMake 项目结构说明

## 项目层次关系

```
ffmpeg_music_player/                    (主项目)
├── CMakeLists.txt                      (主项目 CMakeLists)
├── plugins/                            (插件目录)
│   └── audio_converter_plugin/         (音频转换器插件)
│       └── CMakeLists.txt              (插件 CMakeLists - 子项目)
└── build/                              (构建输出目录)
    ├── bin/                            (主程序输出)
    │   ├── ffmpeg_music_player.exe    (主程序)
    │   └── plugin/                     (插件输出目录)
    │       ├── audio_converter_plugin.dll
    │       └── audio_converter_plugin.json
    └── ...
```

## 关键配置

### 主项目 (ffmpeg_music_player/CMakeLists.txt)

- **项目名称**: `ffmpeg_music_player`
- **主程序输出**: `build/bin/ffmpeg_music_player.exe`
- **插件目录**: `build/bin/plugin/` (使用 `PLUGIN_OUTPUT_DIR` 变量)

```cmake
# 定义插件输出目录
set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/plugin" CACHE PATH "Plugin output directory")

# 添加插件子目录
add_subdirectory(plugins/audio_converter_plugin)
```

### 插件子项目 (plugins/audio_converter_plugin/CMakeLists.txt)

- **目标名称**: `audio_converter_plugin`
- **类型**: 动态库 (SHARED)
- **输出位置**: 继承父项目的 `PLUGIN_OUTPUT_DIR`
- **不独立**: 不使用 `project()` 命令，作为主项目的一部分

```cmake
# 不定义独立的 project，使用父项目的设置
# 直接使用父项目定义的变量：QT_VERSION_MAJOR, PLUGIN_OUTPUT_DIR 等

# 创建插件目标
add_library(audio_converter_plugin SHARED ...)

# 设置输出目录
set_target_properties(audio_converter_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
    ...
)
```

## 构建结果

### Visual Studio 解决方案结构

在 VS2022 中打开生成的 `.sln` 文件后，您会看到：

```
解决方案 'ffmpeg_music_player'
├── ffmpeg_music_player              (主程序项目)
├── audio_converter_plugin           (插件项目 - 作为子项目)
└── ALL_BUILD                        (构建所有项目)
```

### 输出目录结构

构建后的文件结构：

```
build/
├── bin/
│   ├── Debug/                       (Debug 配置)
│   │   ├── ffmpeg_music_player.exe
│   │   └── plugin/
│   │       ├── audio_converter_plugin.dll
│   │       └── audio_converter_plugin.json
│   └── Release/                     (Release 配置)
│       ├── ffmpeg_music_player.exe
│       └── plugin/
│           ├── audio_converter_plugin.dll
│           └── audio_converter_plugin.json
└── ffmpeg_music_player.sln          (VS 解决方案文件)
```

## CMake GUI 配置步骤

### 1. 配置路径

- **Where is the source code**: `e:/FFmpeg_whisper/ffmpeg_music_player`
- **Where to build the binaries**: `e:/FFmpeg_whisper/ffmpeg_music_player/build`

### 2. Configure 设置

点击 **Configure** 按钮：
- **Generator**: Visual Studio 17 2022
- **Platform**: x64

### 3. 关键变量

需要设置的 CMake 变量：

| 变量名 | 值 | 说明 |
|--------|-----|------|
| `CMAKE_PREFIX_PATH` | `E:/Qt5.14/5.14.2/msvc2017_64` | Qt 安装路径 |
| `FFMPEG_DIR` | `E:/ffmpeg-4.4` | FFmpeg 路径 |
| `WHISPER_CPP_DIR` | `E:/whisper.cpp/whisper.cpp-master/whisper.cpp-master` | Whisper.cpp 路径 |

### 4. Generate

配置成功后，点击 **Generate** 生成 VS 解决方案。

## 在 Visual Studio 中构建

### 1. 打开解决方案

```
build/ffmpeg_music_player.sln
```

### 2. 选择配置

在工具栏选择：
- **Debug** 或 **Release**
- **x64** 平台

### 3. 构建主项目

右键 `ffmpeg_music_player` → **生成**

这会自动构建所有依赖项，包括 `audio_converter_plugin`。

### 4. 构建所有项目

右键 **解决方案** → **生成解决方案**

或按 **Ctrl+Shift+B**

## 验证插件加载

构建成功后，插件应该在：

```
build/bin/Debug/plugin/audio_converter_plugin.dll
build/bin/Debug/plugin/audio_converter_plugin.json
```

主程序启动时会自动从 `plugin/` 目录加载插件。

## 添加新插件

要添加新插件：

### 1. 创建插件目录

```
plugins/
└── your_new_plugin/
    ├── CMakeLists.txt
    ├── your_new_plugin.h
    ├── your_new_plugin.cpp
    └── your_new_plugin.json
```

### 2. 编写插件 CMakeLists.txt

参考 `plugins/audio_converter_plugin/CMakeLists.txt`：

```cmake
# 不使用 project()，作为子项目

set(PLUGIN_TARGET_NAME your_new_plugin)

add_library(${PLUGIN_TARGET_NAME} SHARED
    your_new_plugin.cpp
    your_new_plugin.h
)

target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
)
```

### 3. 在主 CMakeLists.txt 中添加

在主项目的 CMakeLists.txt 中添加：

```cmake
add_subdirectory(plugins/your_new_plugin)
```

### 4. 重新生成

在 CMake GUI 中点击 **Configure** → **Generate**

## 常见问题

### Q1: 插件没有出现在 VS 解决方案中

**原因**: `add_subdirectory()` 调用位置不对，或插件 CMakeLists.txt 有错误。

**解决**: 
- 检查主 CMakeLists.txt 是否有 `add_subdirectory(plugins/xxx)`
- 检查插件 CMakeLists.txt 语法
- 重新运行 Configure

### Q2: 插件输出到了错误的目录

**原因**: `PLUGIN_OUTPUT_DIR` 未正确传递，或插件使用了独立的 `project()`。

**解决**:
- 确保插件 CMakeLists.txt **不使用** `project()` 命令
- 使用 `${PLUGIN_OUTPUT_DIR}` 而不是硬编码路径
- 添加 Debug/Release 配置的输出目录设置

### Q3: 构建插件时找不到主项目的头文件

**原因**: 包含路径配置不正确。

**解决**:
```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}  # 插件自己的目录
    ${CMAKE_SOURCE_DIR}           # 主项目根目录
)
```

## 项目依赖关系

```
ffmpeg_music_player (主程序)
    ↓ (运行时加载)
audio_converter_plugin (插件 DLL)
    ↓ (编译时链接)
Qt5::Widgets, FFmpeg 等
```

- **主程序** 不直接链接插件，而是运行时通过 `QPluginLoader` 动态加载
- **插件** 需要链接 Qt 和其他依赖库
- **所有子项目** 共享主项目的 Qt 查找结果和编译设置

## 总结

✅ **正确的层次关系**:
- 主项目 `ffmpeg_music_player` 使用 `project()` 定义
- 插件作为子目录，不使用独立的 `project()`
- 插件继承主项目的设置（Qt、编译器标准等）
- 所有输出集中在 `build/bin/` 目录下

✅ **VS2022 解决方案**:
- 主项目和插件都在同一个解决方案中
- 可以单独构建，也可以一起构建
- 插件自动输出到主程序的 `plugin/` 子目录

✅ **便于扩展**:
- 添加新插件只需创建目录和 CMakeLists.txt
- 在主 CMakeLists.txt 中添加一行 `add_subdirectory()`
- 自动继承所有配置，无需重复设置
