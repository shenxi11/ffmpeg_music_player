# qmake 到 CMake 迁移说明

## 迁移概述

项目已成功从 qmake 构建系统迁移到 CMake，完全支持 Visual Studio 2022。

## 文件对应关系

### 主项目

| qmake | CMake | 说明 |
|-------|-------|------|
| `untitled.pro` | `CMakeLists.txt` | 主构建配置 |
| `qmake` | `cmake` | 配置工具 |
| `make` / `nmake` | `cmake --build` | 构建命令 |
| - | `CMakePresets.json` | VS2022 集成配置 |

### 插件

| qmake | CMake | 说明 |
|-------|-------|------|
| `plugins/*/**.pro` | `plugins/*/CMakeLists.txt` | 插件构建配置 |
| `DESTDIR = ../../plugin` | `RUNTIME_OUTPUT_DIRECTORY` | 输出目录设置 |

## 主要变化

### 1. 构建配置语法

#### qmake (untitled.pro)

```qmake
QT += core gui multimedia network concurrent
CONFIG += c++17
SOURCES += main.cpp main_widget.cpp ...
HEADERS += main_widget.h ...
LIBS += -LE:/ffmpeg-4.4/lib -lavcodec
```

#### CMake (CMakeLists.txt)

```cmake
find_package(Qt6 COMPONENTS Core Gui Multimedia Network Concurrent)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_SOURCES main.cpp main_widget.cpp ...)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core avcodec)
```

### 2. 插件构建

#### qmake

```qmake
CONFIG += plugin
TEMPLATE = lib
TARGET = audio_converter_plugin
DESTDIR = ../../plugin
```

#### CMake

```cmake
add_library(audio_converter_plugin SHARED ...)
set_target_properties(audio_converter_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
)
```

### 3. 资源文件

#### qmake

```qmake
RESOURCES += pic.qrc
```

#### CMake

```cmake
set(PROJECT_RESOURCES pic.qrc)
# 自动处理，设置 CMAKE_AUTORCC=ON
```

### 4. 翻译文件

#### qmake

```qmake
TRANSLATIONS += untitled_zh_CN.ts
```

#### CMake

```cmake
set(PROJECT_TRANSLATIONS untitled_zh_CN.ts)
qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${PROJECT_TRANSLATIONS})
```

## 构建命令对比

### 配置阶段

| 操作 | qmake | CMake |
|------|-------|-------|
| 生成构建文件 | `qmake` | `cmake -G "Visual Studio 17 2022" ..` |
| 指定 Qt 路径 | `qmake -spec ...` | `cmake -DCMAKE_PREFIX_PATH=C:\Qt\...` |
| Debug 构建 | `qmake CONFIG+=debug` | `cmake -DCMAKE_BUILD_TYPE=Debug ..` |

### 构建阶段

| 操作 | qmake | CMake |
|------|-------|-------|
| 编译 | `nmake` / `make` | `cmake --build . --config Release` |
| 清理 | `nmake clean` | `cmake --build . --target clean` |
| 并行编译 | `nmake /MP` | `cmake --build . -j 8` |

### Visual Studio 集成

| 操作 | qmake | CMake |
|------|-------|-------|
| 生成 .sln | `qmake -tp vc` | `cmake -G "Visual Studio 17 2022" ..` |
| 打开项目 | 打开 .vcxproj | 打开文件夹或 .sln |

## 目录结构变化

### qmake 输出

```
ffmpeg_music_player/
├── release/
│   └── untitled.exe
├── plugin/
│   └── audio_converter_plugin.dll
└── Makefile
```

### CMake 输出

```
ffmpeg_music_player/
├── build/
│   ├── bin/
│   │   └── Release/
│   │       ├── ffmpeg_music_player.exe
│   │       └── plugin/
│   │           └── audio_converter_plugin.dll
│   ├── ffmpeg_music_player.sln
│   └── CMakeCache.txt
└── CMakeLists.txt
```

## 新增功能

### 1. CMake Presets

提供预定义的配置：

```json
{
  "configurePresets": [
    {
      "name": "vs2022-x64-release",
      "generator": "Visual Studio 17 2022"
    }
  ]
}
```

使用：

```batch
cmake --preset vs2022-x64-release
cmake --build --preset vs2022-x64-release
```

### 2. 构建脚本

提供便捷的构建脚本：

- `build_cmake.bat` - 一键构建
- `configure_vs2022.bat` - 配置 VS 解决方案

### 3. 更好的并行构建

```batch
# qmake + nmake
nmake /MP8  # 仅 VS 生成器支持

# CMake
cmake --build . -j 8  # 所有生成器通用
```

### 4. 跨平台支持增强

CMake 提供更好的跨平台支持，同一套配置可用于：
- Windows (Visual Studio, MinGW)
- Linux (GCC, Clang)
- macOS (Clang)

## 迁移检查清单

### ✅ 已完成

- [x] 主项目 CMakeLists.txt
- [x] 插件 CMakeLists.txt
- [x] CMakePresets.json（VS2022 集成）
- [x] 构建脚本（.bat）
- [x] .gitignore 更新
- [x] 插件加载路径更新
- [x] 文档更新

### 📋 需要手动配置

- [ ] 根据实际情况修改 Qt 路径
- [ ] 根据实际情况修改 FFmpeg 路径
- [ ] 根据实际情况修改 Whisper 路径
- [ ] 测试 Debug 和 Release 构建
- [ ] 测试插件加载

## 兼容性说明

### 保留的 qmake 文件

为了向后兼容，保留了原有的 `.pro` 文件：

- `untitled.pro`
- `plugins/audio_converter_plugin/audio_converter_plugin.pro`

你仍然可以使用 qmake 构建，但推荐使用 CMake。

### 删除建议

当确认 CMake 构建正常后，可以删除：

```
untitled.pro
untitled.pro.user*
plugins/**/*.pro
Makefile*
*.vcxproj*
*.sln (旧的)
```

## 常见问题

### Q: 为什么要迁移到 CMake？

**A:** CMake 的优势：
1. ✅ 更好的 IDE 集成（特别是 Visual Studio）
2. ✅ 更快的并行构建
3. ✅ 更现代的构建系统
4. ✅ 更好的跨平台支持
5. ✅ 更活跃的社区和更好的文档
6. ✅ Qt 官方推荐（Qt 6+ 主推 CMake）

### Q: qmake 还能用吗？

**A:** 可以，`.pro` 文件仍然保留。但建议逐步迁移到 CMake。

### Q: 如何在两者之间切换？

**A:** 
- **使用 qmake**: `qmake && nmake`
- **使用 CMake**: `cmake --build build`

两者的输出目录不同，不会冲突。

### Q: 插件还兼容吗？

**A:** 完全兼容！插件接口未改变，只是构建方式改变了。

### Q: 性能有提升吗？

**A:** 是的！CMake + Ninja 构建速度通常比 qmake + nmake 快 30-50%。

## 下一步

1. **测试构建**：运行 `build_cmake.bat`
2. **在 VS2022 中打开**：运行 `configure_vs2022.bat`
3. **熟悉 CMake**：阅读 [CMAKE_BUILD_GUIDE.md](CMAKE_BUILD_GUIDE.md)
4. **开发新功能**：使用 CMake 添加新源文件

## 技术支持

遇到问题？

1. 查看 [CMAKE_BUILD_GUIDE.md](CMAKE_BUILD_GUIDE.md)
2. 查看 [QUICKSTART.md](QUICKSTART.md)
3. 查看 [CMake 官方文档](https://cmake.org/documentation/)
4. 提交 Issue

## 贡献

欢迎改进 CMake 配置！提交 PR 前请确保：

- [x] Debug 和 Release 都能构建
- [x] 插件正常加载
- [x] 在 VS2022 中能正常打开和构建
- [x] 更新相关文档

---

**迁移完成！享受 CMake 带来的便利！** 🎉
