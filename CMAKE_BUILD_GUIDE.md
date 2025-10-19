# CMake 构建指南

## 概述

项目已从 qmake 迁移到 CMake 构建系统，完全支持 Visual Studio 2022。

## 系统要求

### 必需工具

- **CMake** 3.16 或更高版本
- **Visual Studio 2022** (MSVC v143)
- **Qt 5.15+** 或 **Qt 6.x** (推荐 Qt 6.6.0)
- **FFmpeg 4.4**（已配置路径：`E:/ffmpeg-4.4`）
- **Whisper.cpp**（已配置路径：`E:/whisper.cpp/whisper.cpp-master/whisper.cpp-master`）

### 可选工具

- **Ninja** 构建系统（更快的构建速度）
- **Qt Creator** 或 **Visual Studio Code**（支持 CMake 的 IDE）

## 快速开始

### 方法 1：使用批处理脚本（推荐）

#### 完整构建（配置 + 编译）

```batch
# Release 构建
build_cmake.bat

# Debug 构建
build_cmake.bat debug
```

#### 仅配置（生成 VS 解决方案）

```batch
configure_vs2022.bat
```

然后在 Visual Studio 2022 中打开 `build/ffmpeg_music_player.sln`

### 方法 2：命令行手动构建

#### 步骤 1：设置环境变量

```batch
set QT_DIR=C:\Qt\6.6.0\msvc2019_64
set CMAKE_PREFIX_PATH=%QT_DIR%
set PATH=%QT_DIR%\bin;%PATH%
```

#### 步骤 2：配置 CMake

```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ..
```

#### 步骤 3：构建项目

```batch
# Release 构建
cmake --build . --config Release -j 8

# Debug 构建
cmake --build . --config Debug -j 8
```

### 方法 3：使用 Visual Studio 2022 IDE

1. 打开 Visual Studio 2022
2. 选择 `文件` -> `打开` -> `文件夹`
3. 选择项目根目录（包含 `CMakeLists.txt` 的目录）
4. Visual Studio 会自动检测 CMake 项目
5. 选择配置预设（例如 `vs2022-x64-release`）
6. 点击 `生成` -> `全部生成`

### 方法 4：使用 CMake Presets

```batch
# 列出可用的预设
cmake --list-presets

# 使用预设配置
cmake --preset vs2022-x64-release

# 使用预设构建
cmake --build --preset vs2022-x64-release
```

## 配置选项

### 修改 Qt 路径

编辑 `build_cmake.bat` 或 `configure_vs2022.bat`：

```batch
set QT_DIR=C:\Qt\你的Qt版本\msvc2019_64
```

或在 CMake 命令中指定：

```batch
cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64 ..
```

### 修改 FFmpeg 路径

编辑 `CMakeLists.txt`：

```cmake
set(FFMPEG_DIR "你的FFmpeg路径")
```

### 修改 Whisper.cpp 路径

编辑 `CMakeLists.txt`：

```cmake
set(WHISPER_DIR "你的Whisper路径")
```

### 修改构建类型

```batch
# Release（优化，无调试信息）
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug（调试信息，无优化）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# RelWithDebInfo（优化 + 调试信息）
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

## 目录结构

### 构建输出

```
build/
├── bin/
│   ├── Release/
│   │   ├── ffmpeg_music_player.exe    # 主程序
│   │   ├── *.dll                       # Qt 和 FFmpeg DLL
│   │   └── plugin/
│   │       └── audio_converter_plugin.dll
│   └── Debug/
│       └── ...（同上）
├── CMakeCache.txt
├── ffmpeg_music_player.sln            # VS 解决方案
└── ...（其他 CMake 生成文件）
```

### 源码目录

```
ffmpeg_music_player/
├── CMakeLists.txt                     # 主 CMake 配置
├── CMakePresets.json                  # CMake 预设配置
├── build_cmake.bat                    # 构建脚本
├── configure_vs2022.bat               # 配置脚本
├── *.cpp, *.h                         # 源文件
├── pic.qrc                            # 资源文件
└── plugins/
    └── audio_converter_plugin/
        └── CMakeLists.txt             # 插件 CMake 配置
```

## 插件构建

插件会自动随主项目一起构建。每个插件都有自己的 `CMakeLists.txt`。

### 添加新插件

1. 在 `plugins/` 下创建新目录
2. 创建 `CMakeLists.txt`（参考 `audio_converter_plugin`）
3. 在主 `CMakeLists.txt` 中添加：
   ```cmake
   add_subdirectory(plugins/your_plugin)
   ```
4. 重新配置和构建

### 单独构建插件

```batch
cd build/plugins/audio_converter_plugin
cmake --build . --config Release
```

## Visual Studio 2022 集成

### 打开项目

1. **方法 A**：打开文件夹
   - `文件` -> `打开` -> `文件夹`
   - 选择项目根目录

2. **方法 B**：打开解决方案
   - 运行 `configure_vs2022.bat`
   - 打开 `build/ffmpeg_music_player.sln`

### 配置管理

在 Visual Studio 中可以选择：
- **Debug** - 调试版本
- **Release** - 发布版本
- **RelWithDebInfo** - 带调试信息的发布版本
- **MinSizeRel** - 最小大小发布版本

### 调试设置

1. 右键点击 `ffmpeg_music_player` 项目
2. 选择 `设置为启动项目`
3. 设置断点
4. 按 F5 开始调试

### 生成事件

CMake 会自动处理：
- MOC（Qt 元对象编译）
- RCC（Qt 资源编译）
- UIC（Qt UI 文件编译）
- DLL 复制到输出目录

## 常见问题

### Q: CMake 找不到 Qt？

**A:** 设置 `CMAKE_PREFIX_PATH` 环境变量：

```batch
set CMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
```

或在 CMake 命令中指定：

```batch
cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64 ..
```

### Q: 找不到 FFmpeg 库？

**A:** 检查 `CMakeLists.txt` 中的路径是否正确：

```cmake
set(FFMPEG_DIR "E:/ffmpeg-4.4")
```

确保 FFmpeg 的 `lib` 目录包含 `.lib` 文件。

### Q: 链接错误 LNK2019？

**A:** 可能的原因：
1. FFmpeg 库路径不正确
2. Qt 版本不匹配
3. 编译器版本不匹配（需要 MSVC 2019/2022）

### Q: 运行时找不到 DLL？

**A:** CMake 会自动复制 FFmpeg DLL，但需要手动复制 Qt DLL：

```batch
windeployqt build\bin\Release\ffmpeg_music_player.exe
```

### Q: 如何清理构建？

**A:** 删除 `build` 目录：

```batch
rmdir /s /q build
```

然后重新配置和构建。

### Q: 插件加载失败？

**A:** 检查：
1. 插件 DLL 在 `bin/Release/plugin/` 目录
2. 插件的 Qt 版本与主程序一致
3. 插件的编译器版本与主程序一致

## 性能优化

### 并行编译

```batch
# 使用 8 个线程
cmake --build . --config Release -j 8
```

### 使用 Ninja 构建器

```batch
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

Ninja 通常比 MSBuild 快 30-50%。

## 部署

### 创建发布包

```batch
# 1. Release 构建
build_cmake.bat

# 2. 复制 Qt DLL
cd build\bin\Release
windeployqt ffmpeg_music_player.exe

# 3. 复制插件
# 插件 DLL 已在 plugin 目录

# 4. 打包
# 将整个 Release 目录打包分发
```

### 所需文件

```
Release/
├── ffmpeg_music_player.exe
├── Qt*.dll
├── av*.dll（FFmpeg）
├── platforms/
│   └── qwindows.dll
└── plugin/
    └── audio_converter_plugin.dll
```

## 对比 qmake

### 优势

✅ 更好的 IDE 集成（VS2022）
✅ 更快的构建速度（并行编译）
✅ 更现代的构建系统
✅ 更好的跨平台支持
✅ 更灵活的配置选项
✅ 原生的 Visual Studio 解决方案生成

### 迁移变化

| qmake | CMake |
|-------|-------|
| `.pro` 文件 | `CMakeLists.txt` |
| `qmake` | `cmake` |
| `make` / `nmake` | `cmake --build` |
| `SOURCES +=` | `set(PROJECT_SOURCES ...)` |
| `HEADERS +=` | `set(PROJECT_SOURCES ...)` |
| `LIBS +=` | `target_link_libraries()` |

## 支持

如有问题，请参考：
- [CMake 官方文档](https://cmake.org/documentation/)
- [Qt CMake 手册](https://doc.qt.io/qt-6/cmake-manual.html)
- 项目 Issue 页面

## 更新日志

- **2025-10-06**: 初始 CMake 迁移完成
  - 支持 Visual Studio 2022
  - 插件系统 CMake 化
  - 添加构建脚本和预设
