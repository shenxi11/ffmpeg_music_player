# Qt 5.14.2 专用构建指南

## 项目配置

本项目已配置为 **仅支持 Qt 5.14.2**，所有 Qt6 相关代码已移除。

## 系统要求

- **Qt**: 5.14.2
- **编译器**: MSVC 2017/2019/2022（推荐 MSVC 2017 或 2019）
- **CMake**: 3.16 或更高版本
- **Visual Studio**: 2017/2019/2022
- **FFmpeg**: 4.4
- **Whisper.cpp**: 最新版本

## CMake GUI 配置步骤

### 1. 设置路径

在 CMake GUI 中：

**Where is the source code**:
```
E:/FFmpeg_whisper/ffmpeg_music_player
```

**Where to build the binaries**:
```
E:/FFmpeg_whisper/ffmpeg_music_player/build
```

### 2. Configure

点击 **Configure** 按钮，选择：

- **Generator**: Visual Studio 15 2017（或 16 2019 / 17 2022）
- **Platform**: x64

### 3. 设置 CMake 变量

在配置过程中，需要设置以下变量：

| 变量名 | 值 | 说明 |
|--------|-----|------|
| `CMAKE_PREFIX_PATH` | `E:/Qt5.14/5.14.2/msvc2017_64` | Qt5 安装路径 |
| `FFMPEG_DIR` | `E:/ffmpeg-4.4` | FFmpeg 路径 |
| `WHISPER_CPP_DIR` | `E:/whisper.cpp/...` | Whisper.cpp 路径 |

**如何设置变量**：
1. 点击 Configure 后，会出现错误
2. 在变量列表中找到 `CMAKE_PREFIX_PATH`
3. 双击修改为您的 Qt5 路径
4. 再次点击 Configure

### 4. Generate

配置成功后（没有红色错误），点击 **Generate** 生成 Visual Studio 解决方案。

### 5. 打开 Visual Studio

点击 **Open Project** 或手动打开：
```
build/ffmpeg_music_player.sln
```

## Visual Studio 编译

### 1. 选择配置

在工具栏选择：
- **Debug** 或 **Release**
- **x64** 平台

### 2. 生成解决方案

右键解决方案 → **生成解决方案**

或按 `Ctrl+Shift+B`

### 3. 构建输出

成功后，文件会输出到：

```
build/bin/Debug/
├── ffmpeg_music_player.exe    (主程序)
└── plugin/
    ├── audio_converter_plugin.dll
    └── audio_converter_plugin.json

build/bin/Release/
├── ffmpeg_music_player.exe
└── plugin/
    ├── audio_converter_plugin.dll
    └── audio_converter_plugin.json
```

## 常见问题

### Q1: 找不到 Qt5

**错误**: `Could NOT find Qt5`

**解决**:
1. 确保已安装 Qt 5.14.2
2. 在 CMake GUI 中设置 `CMAKE_PREFIX_PATH` 为 Qt5 路径
3. 例如: `E:/Qt5.14/5.14.2/msvc2017_64`

### Q2: Qt 路径不确定

**解决**: 查看您的 Qt 安装目录，可能的路径：
```
C:/Qt/Qt5.14.2/5.14.2/msvc2017_64
D:/Qt/5.14.2/msvc2017_64
E:/Qt5.14/5.14.2/msvc2017_64
```

使用 `qmake.exe` 的父目录的父目录。例如：
```
E:/Qt5.14/5.14.2/msvc2017_64/bin/qmake.exe
          ↑ 使用这个路径 ↑
```

### Q3: MSVC 版本不匹配

**错误**: 链接错误或运行时崩溃

**解决**:
- 确保 Qt 套件版本与 Visual Studio 版本匹配：
  - `msvc2015_64` → VS 2015
  - `msvc2017_64` → VS 2017/2019/2022 ✅ 推荐
  - `msvc2019_64` → VS 2019/2022

### Q4: 找不到 FFmpeg

**错误**: `Cannot find avcodec, avformat, ...`

**解决**:
1. 确保 FFmpeg 已正确安装
2. 在 CMakeLists.txt 中修改 FFMPEG_DIR 路径
3. 或在 CMake GUI 中添加变量 `FFMPEG_DIR`

### Q5: 运行时缺少 DLL

**错误**: 启动程序时提示缺少 Qt5Core.dll 等

**解决**:

**方法 1: 使用 windeployqt（推荐）**
```powershell
cd build/bin/Debug
E:/Qt5.14/5.14.2/msvc2017_64/bin/windeployqt.exe ffmpeg_music_player.exe
```

**方法 2: 手动复制 DLL**

将以下 DLL 从 Qt5 bin 目录复制到 exe 目录：
```
Qt5Core.dll
Qt5Gui.dll
Qt5Widgets.dll
Qt5Multimedia.dll
Qt5Network.dll
Qt5Concurrent.dll
```

还需要复制 platforms 目录：
```
platforms/qwindows.dll
```

**方法 3: 添加到 PATH**
```powershell
set PATH=E:/Qt5.14/5.14.2/msvc2017_64/bin;%PATH%
```

### Q6: 编译时出现 C++17 相关错误

**错误**: `error C2039` 或 `requires C++17`

**解决**:
- CMakeLists.txt 已设置 `CMAKE_CXX_STANDARD 17`
- 如果仍有问题，清理 CMake 缓存重新配置：
  - CMake GUI → File → Delete Cache
  - 重新 Configure 和 Generate

## 项目结构

```
ffmpeg_music_player/
├── CMakeLists.txt              (主项目配置 - Qt5 专用)
├── plugins/
│   └── audio_converter_plugin/
│       └── CMakeLists.txt      (插件配置 - Qt5 专用)
├── build/                      (构建输出目录)
│   ├── ffmpeg_music_player.sln (VS 解决方案)
│   └── bin/
│       ├── Debug/
│       │   ├── ffmpeg_music_player.exe
│       │   └── plugin/
│       │       └── audio_converter_plugin.dll
│       └── Release/
│           ├── ffmpeg_music_player.exe
│           └── plugin/
│               └── audio_converter_plugin.dll
└── *.cpp, *.h                  (源代码)
```

## CMakeLists.txt 关键配置

### 主项目

```cmake
# 仅查找 Qt5
find_package(Qt5 REQUIRED COMPONENTS 
    Core Gui Widgets Multimedia Network Concurrent LinguistTools
)

# Qt5 可执行文件
add_executable(${PROJECT_NAME} ...)

# 链接 Qt5 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Network
    Qt5::Concurrent
)
```

### 插件项目

```cmake
# 继承主项目的 Qt5 查找结果
# 无需重新 find_package

# 链接 Qt5 库
target_link_libraries(audio_converter_plugin PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
)
```

## 验证安装

### 检查 Qt 安装

运行以下命令检查 Qt 版本：
```powershell
E:/Qt5.14/5.14.2/msvc2017_64/bin/qmake.exe --version
```

应输出：
```
QMake version 3.1
Using Qt version 5.14.2 in E:/Qt5.14/5.14.2/msvc2017_64/lib
```

### 检查 CMake 配置

CMake Configure 成功后，应看到：
```
-- Found Qt5: version 5.14.2
-- Qt5Core found
-- Qt5Gui found
-- Qt5Widgets found
-- Qt5Multimedia found
-- Qt5Network found
-- Qt5Concurrent found
-- Qt5LinguistTools found
-- Configuring done
-- Generating done
```

### 检查编译输出

成功编译后：
```powershell
dir build\bin\Release\ffmpeg_music_player.exe
dir build\bin\Release\plugin\audio_converter_plugin.dll
```

应显示文件存在。

## 快速开始命令

假设您的 Qt5 在 `E:/Qt5.14/5.14.2/msvc2017_64`：

### 使用 CMake GUI（推荐）

1. 打开 CMake GUI
2. 设置源码路径和构建路径
3. Configure → 设置 CMAKE_PREFIX_PATH → Configure
4. Generate
5. Open Project

### 使用命令行

```powershell
# 创建构建目录
mkdir build
cd build

# 配置 (使用 VS2017)
cmake -G "Visual Studio 15 2017" -A x64 ^
  -DCMAKE_PREFIX_PATH=E:/Qt5.14/5.14.2/msvc2017_64 ^
  -DFFMPEG_DIR=E:/ffmpeg-4.4 ^
  ..

# 构建 Release
cmake --build . --config Release

# 或者打开 VS
start ffmpeg_music_player.sln
```

## 总结

✅ **配置要求**:
- Qt 5.14.2 (msvc2017_64 或 msvc2019_64)
- Visual Studio 2017/2019/2022
- CMake 3.16+

✅ **构建流程**:
1. CMake GUI 配置
2. 设置 CMAKE_PREFIX_PATH 为 Qt5 路径
3. Generate 生成 VS 解决方案
4. 在 VS 中编译

✅ **输出位置**:
- 主程序: `build/bin/[Debug|Release]/ffmpeg_music_player.exe`
- 插件: `build/bin/[Debug|Release]/plugin/audio_converter_plugin.dll`

🎉 祝构建成功！如有问题，请检查上述常见问题部分。
