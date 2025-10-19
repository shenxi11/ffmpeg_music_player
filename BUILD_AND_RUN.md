# 构建和运行指南

## 构建步骤

### 1. 在 Visual Studio 中选择配置

在 VS2022 工具栏中选择：

**Debug 构建**：
- 配置：**Debug**
- 平台：**x64**

**Release 构建**：
- 配置：**Release**
- 平台：**x64**

### 2. 生成项目

- 右键解决方案 → **生成解决方案** (Ctrl+Shift+B)
- 或右键 `ffmpeg_music_player` 项目 → **生成**

### 3. 输出位置

构建后，文件会输出到：

**Debug 版本**：
```
build/bin/Debug/
├── ffmpeg_music_player.exe        主程序
├── Qt5Core.dll                    Qt DLL（自动复制）
├── Qt5Gui.dll
├── Qt5Widgets.dll
├── Qt5Multimedia.dll
├── Qt5Network.dll
├── Qt5Concurrent.dll
├── avcodec-58.dll                 FFmpeg DLL（自动复制）
├── avformat-58.dll
├── avutil-56.dll
├── ...更多 DLL
├── platforms/                     Qt 插件（自动复制）
│   └── qwindows.dll
├── styles/
└── plugin/                        音乐播放器插件
    ├── audio_converter_plugin.dll
    └── audio_converter_plugin.json
```

**Release 版本**：
```
build/bin/Release/
├── ffmpeg_music_player.exe
├── Qt5Core.dll
├── ...（结构同 Debug）
└── plugin/
    └── audio_converter_plugin.dll
```

## Qt DLL 自动部署

项目已配置为自动使用 `windeployqt` 工具复制 Qt 依赖。

### 自动部署（推荐）

构建时会自动执行 `windeployqt`，复制所有需要的：
- Qt DLL 文件
- Qt 插件（platforms、styles 等）
- 图标缓存
- 其他运行时依赖

### 手动部署（如果自动失败）

如果自动部署失败，可以手动运行：

**Debug 版本**：
```powershell
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe --debug build\bin\Debug\ffmpeg_music_player.exe
```

**Release 版本**：
```powershell
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe --release build\bin\Release\ffmpeg_music_player.exe
```

### windeployqt 参数说明

```powershell
windeployqt.exe [选项] <exe文件路径>
```

常用选项：
- `--debug` - 部署 Debug 版本的 DLL
- `--release` - 部署 Release 版本的 DLL
- `--no-translations` - 不复制翻译文件
- `--no-system-d3d-compiler` - 不复制 D3D 编译器
- `--no-opengl-sw` - 不复制 OpenGL 软件渲染器

## 运行程序

### 在 Visual Studio 中运行

1. 确保选择了正确的配置（Debug 或 Release）
2. 按 **F5**（调试运行）或 **Ctrl+F5**（非调试运行）

### 直接运行可执行文件

**Debug**：
```powershell
.\build\bin\Debug\ffmpeg_music_player.exe
```

**Release**：
```powershell
.\build\bin\Release\ffmpeg_music_player.exe
```

## 常见问题

### Q1: Release 目录为空

**原因**：在 VS 中选择了 Debug 配置，只会构建 Debug 版本。

**解决**：
1. 在 VS 工具栏切换到 **Release** 配置
2. 重新生成解决方案

### Q2: 提示缺少 Qt5Core.dll

**原因**：Qt DLL 未复制到输出目录。

**解决方法 1（自动）**：
- 删除 `build` 目录
- 在 CMake GUI 中重新 Configure 和 Generate
- 在 VS 中重新构建

**解决方法 2（手动）**：
```powershell
# Debug
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe --debug build\bin\Debug\ffmpeg_music_player.exe

# Release
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe --release build\bin\Release\ffmpeg_music_player.exe
```

**解决方法 3（临时）**：
将 Qt bin 目录添加到 PATH：
```powershell
$env:PATH = "E:\Qt5.14\5.14.2\msvc2017_64\bin;$env:PATH"
.\build\bin\Release\ffmpeg_music_player.exe
```

### Q3: 提示缺少 FFmpeg DLL

**原因**：FFmpeg DLL 路径不正确或未找到。

**解决**：
1. 检查 `FFMPEG_DIR` 是否正确（应该是 `E:/ffmpeg-4.4`）
2. 确认 FFmpeg DLL 存在：
   ```powershell
   dir E:\ffmpeg-4.4\bin\*.dll
   ```
3. 手动复制 DLL：
   ```powershell
   copy E:\ffmpeg-4.4\bin\*.dll build\bin\Release\
   ```

### Q4: 提示缺少 platforms/qwindows.dll

**原因**：Qt 平台插件未复制。

**解决**：
运行 windeployqt 会自动复制 platforms 目录：
```powershell
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe build\bin\Release\ffmpeg_music_player.exe
```

### Q5: Debug 和 Release 不能混用 DLL

**错误**：运行时崩溃或链接错误

**原因**：Debug 版本的程序必须使用 Debug 版本的 DLL，Release 同理。

**解决**：
- Debug 程序：使用 `windeployqt --debug`
- Release 程序：使用 `windeployqt --release`
- 不要混用 Debug 和 Release 的 DLL

## 验证部署

### 检查 DLL 依赖

使用 Dependency Walker 或 `dumpbin` 检查：

```powershell
# 查看程序依赖的 DLL
dumpbin /dependents build\bin\Release\ffmpeg_music_player.exe
```

### 检查文件完整性

**Debug 版本**：
```powershell
dir build\bin\Debug\*.dll
dir build\bin\Debug\platforms\
dir build\bin\Debug\plugin\
```

**Release 版本**：
```powershell
dir build\bin\Release\*.dll
dir build\bin\Release\platforms\
dir build\bin\Release\plugin\
```

应该看到：
- ✅ Qt5*.dll（多个）
- ✅ av*.dll（FFmpeg DLL）
- ✅ platforms/qwindows.dll
- ✅ plugin/audio_converter_plugin.dll

## 打包分发

### 创建独立的发布包

1. 构建 Release 版本
2. 复制整个 `build/bin/Release` 目录
3. 重命名为 `ffmpeg_music_player_v1.0`
4. 压缩为 ZIP

**发布包结构**：
```
ffmpeg_music_player_v1.0/
├── ffmpeg_music_player.exe
├── Qt5Core.dll
├── Qt5Gui.dll
├── ...
├── platforms/
│   └── qwindows.dll
└── plugin/
    └── audio_converter_plugin.dll
```

### 最小化包大小

使用 `windeployqt` 的精简选项：
```powershell
windeployqt --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    --no-compiler-runtime ^
    build\bin\Release\ffmpeg_music_player.exe
```

### 创建安装程序（可选）

可以使用：
- **Inno Setup** - 免费的 Windows 安装程序制作工具
- **NSIS** - 开源安装程序制作工具
- **Qt Installer Framework** - Qt 官方的安装程序框架

## 快速命令参考

### 构建 Release 版本
```powershell
# 在 build 目录
cmake --build . --config Release

# 或在 VS 中
# 切换到 Release 配置 → Ctrl+Shift+B
```

### 部署 Qt 依赖
```powershell
# Debug
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe --debug build\bin\Debug\ffmpeg_music_player.exe

# Release
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe --release build\bin\Release\ffmpeg_music_player.exe
```

### 运行程序
```powershell
# Debug
.\build\bin\Debug\ffmpeg_music_player.exe

# Release
.\build\bin\Release\ffmpeg_music_player.exe
```

## 总结

✅ **自动化流程**：
1. CMake 配置 → Visual Studio 生成
2. 选择 Debug 或 Release 配置
3. 构建项目
4. `windeployqt` 自动复制 Qt DLL
5. FFmpeg DLL 自动复制
6. 直接运行

✅ **输出目录**：
- Debug: `build/bin/Debug/`
- Release: `build/bin/Release/`

✅ **所有依赖都会自动复制**：
- Qt5 DLL
- Qt 插件（platforms、styles 等）
- FFmpeg DLL
- 项目插件（audio_converter_plugin.dll）

🎉 现在构建 Release 版本应该可以正常工作，并且所有 DLL 都会自动部署！
