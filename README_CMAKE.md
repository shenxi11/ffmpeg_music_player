# FFmpeg Music Player - CMake 构建版本

[![CMake](https://img.shields.io/badge/CMake-3.16+-blue.svg)](https://cmake.org/)
[![Qt](https://img.shields.io/badge/Qt-5.15%20%7C%206.x-green.svg)](https://www.qt.io/)
[![VS2022](https://img.shields.io/badge/Visual%20Studio-2022-purple.svg)](https://visualstudio.microsoft.com/)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)

基于 Qt + FFmpeg + Whisper.cpp 的多功能音乐播放器，现已支持 CMake 构建和插件系统。

## 🌟 特性

- 🎵 本地和在线音乐播放
- 🎤 语音转文字功能（基于 Whisper.cpp）
- 🎨 桌面歌词显示
- 🔄 音频格式转换（插件）
- 🔌 插件系统架构
- 🖥️ 完整的 Visual Studio 2022 支持
- ⚡ CMake 构建系统

## 🚀 快速开始

### 前提条件

- Visual Studio 2022
- CMake 3.16+
- Qt 6.6.0 或 Qt 5.15+ (MSVC 2019/2022)
- FFmpeg 4.4
- Whisper.cpp

### 一键构建

1. **克隆仓库**
   ```bash
   git clone https://github.com/yourusername/ffmpeg_music_player.git
   cd ffmpeg_music_player
   ```

2. **检查配置**
   ```batch
   check_config.bat
   ```

3. **修改 Qt 路径**
   
   编辑 `build_cmake.bat`，修改第 10 行：
   ```batch
   set QT_DIR=C:\Qt\6.6.0\msvc2019_64
   ```

4. **构建**
   ```batch
   build_cmake.bat
   ```

5. **运行**
   ```batch
   cd build\bin\Release
   ffmpeg_music_player.exe
   ```

详细说明请查看 [QUICKSTART.md](QUICKSTART.md)

## 📖 文档

- [快速开始](QUICKSTART.md) - 一分钟上手指南
- [构建指南](CMAKE_BUILD_GUIDE.md) - 详细的构建文档
- [迁移说明](CMAKE_MIGRATION.md) - 从 qmake 迁移的说明
- [插件系统](PLUGIN_SYSTEM.md) - 插件架构文档
- [插件开发](plugins/PLUGIN_DEVELOPMENT.md) - 如何开发插件
- [实现总结](IMPLEMENTATION_SUMMARY.md) - 技术实现细节

## 🛠️ 构建方式

### 方法 1：批处理脚本（推荐）

```batch
build_cmake.bat          # Release 构建
build_cmake.bat debug    # Debug 构建
```

### 方法 2：Visual Studio 2022

```batch
configure_vs2022.bat     # 生成 .sln 文件
# 然后打开 build\ffmpeg_music_player.sln
```

### 方法 3：VS2022 打开文件夹

```
文件 -> 打开 -> 文件夹 -> 选择项目根目录
```

### 方法 4：CMake 命令行

```batch
cmake -G "Visual Studio 17 2022" -A x64 -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
cmake --build build --config Release -j 8
```

## 🔌 插件系统

项目采用插件化架构，支持动态加载功能模块。

### 现有插件

- **音频转换器** - 支持 MP3、WAV、FLAC、AAC、OGG 等格式互转

### 插件目录结构

```
plugins/
└── audio_converter_plugin/
    ├── CMakeLists.txt
    ├── audio_converter_plugin.h
    ├── audio_converter_plugin.cpp
    └── audio_converter_plugin.json
```

### 开发新插件

查看 [插件开发指南](plugins/PLUGIN_DEVELOPMENT.md)

## 📁 项目结构

```
ffmpeg_music_player/
├── CMakeLists.txt              # 主 CMake 配置
├── CMakePresets.json           # VS2022 预设
├── build_cmake.bat             # 构建脚本
├── configure_vs2022.bat        # VS 配置脚本
├── check_config.bat            # 配置检查脚本
├── main.cpp                    # 主入口
├── main_widget.h/cpp           # 主窗口
├── plugin_manager.h/cpp        # 插件管理器
├── plugin_interface.h          # 插件接口
├── *.cpp, *.h                  # 其他源文件
├── pic.qrc                     # 资源文件
├── plugins/                    # 插件目录
│   └── audio_converter_plugin/
│       └── CMakeLists.txt
└── docs/                       # 文档目录
    ├── QUICKSTART.md
    ├── CMAKE_BUILD_GUIDE.md
    └── ...
```

## 🔧 配置

### Qt 路径

编辑 `build_cmake.bat`：

```batch
set QT_DIR=C:\Qt\你的Qt版本\msvc2019_64
```

### FFmpeg 路径

编辑 `CMakeLists.txt`：

```cmake
set(FFMPEG_DIR "你的FFmpeg路径")
```

### Whisper 路径

编辑 `CMakeLists.txt`：

```cmake
set(WHISPER_DIR "你的Whisper路径")
```

## 🎯 功能特性

### 音乐播放

- ✅ 本地音乐列表
- ✅ 在线音乐搜索和播放
- ✅ 播放控制（播放、暂停、上一曲、下一曲）
- ✅ 进度条控制
- ✅ 音量调节
- ✅ 播放模式切换（顺序、随机、单曲循环）

### 歌词功能

- ✅ 歌词解析和显示
- ✅ 桌面歌词
- ✅ 歌词样式自定义
- ✅ 歌词拖动和缩放

### 语音转文字

- ✅ 基于 Whisper.cpp
- ✅ 支持多种音频格式
- ✅ 实时转换

### 音频转换（插件）

- ✅ 多格式支持（MP3、WAV、FLAC、AAC、OGG）
- ✅ 拖放文件
- ✅ 批量转换
- ✅ 自定义编码参数

### 用户系统

- ✅ 用户登录/注册
- ✅ 用户信息显示
- ✅ 登录状态管理

### 界面

- ✅ 现代化 UI 设计
- ✅ 自定义窗口控件
- ✅ 搜索功能
- ✅ 主菜单系统

## 🧩 依赖库

| 库 | 版本 | 用途 |
|----|------|------|
| Qt | 5.15+ / 6.x | UI 框架 |
| FFmpeg | 4.4 | 音视频处理 |
| Whisper.cpp | latest | 语音识别 |
| CMake | 3.16+ | 构建系统 |

## 📊 构建输出

```
build/
└── bin/
    └── Release/
        ├── ffmpeg_music_player.exe      # 主程序
        ├── Qt6Core.dll                  # Qt 库
        ├── avcodec-58.dll               # FFmpeg 库
        └── plugin/
            └── audio_converter_plugin.dll  # 插件
```

## 🐛 故障排除

### 找不到 Qt

```batch
set CMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
```

### 链接错误

检查库路径是否正确：
- FFmpeg: `E:/ffmpeg-4.4`
- Whisper: `E:/whisper.cpp/...`

### 运行时缺少 DLL

```batch
cd build\bin\Release
windeployqt ffmpeg_music_player.exe
```

更多问题请查看 [CMAKE_BUILD_GUIDE.md](CMAKE_BUILD_GUIDE.md)

## 🤝 贡献

欢迎贡献！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📝 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情

## 👥 作者

- **shenxi11** - *初始工作* - [GitHub](https://github.com/shenxi11)

## 🙏 致谢

- Qt Framework
- FFmpeg Project
- Whisper.cpp
- 所有贡献者

## 📮 联系方式

- 项目地址: [https://github.com/shenxi11/ffmpeg_music_player](https://github.com/shenxi11/ffmpeg_music_player)
- 问题反馈: [Issues](https://github.com/shenxi11/ffmpeg_music_player/issues)

---

**享受使用 FFmpeg Music Player！** 🎵✨
