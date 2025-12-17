# Qt Creator CMake 构建问题解决方案

## 问题描述
在第一次构建后关闭 Qt Creator,重新打开并构建时出现错误:
```
Cannot remove existing file E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\QtQuick\Templates.2\plugins.qmltypes
```

## 根本原因
`windeployqt` 工具在每次构建后运行,尝试重新部署 Qt 依赖项。当文件被占用或权限问题时,无法删除/覆盖已存在的文件导致构建失败。

## 解决方案

### 方案 1: 智能部署(推荐) ✅
项目已更新为使用智能部署脚本,只在必要时运行 `windeployqt`:

1. **首次构建时**自动部署 Qt 依赖
2. **增量编译时**检查是否已部署,避免重复部署
3. **可执行文件更新后**才重新部署

**使用方法**: 已自动启用,无需额外操作

**手动强制重新部署**:
```powershell
# 删除部署标记文件来强制重新部署
Remove-Item E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\.qt_deployed_Debug -ErrorAction SilentlyContinue
Remove-Item E:\QT_CMAKE\ffmpeg_musicer_player_build\Release\.qt_deployed_Release -ErrorAction SilentlyContinue
```

### 方案 2: 快速修复(临时)
如果遇到文件占用错误,执行以下步骤:

#### 步骤 1: 确保程序完全关闭
```powershell
# 查找并结束可能占用文件的进程
Get-Process | Where-Object {$_.Name -like "*ffmpeg_music_player*"} | Stop-Process -Force
```

#### 步骤 2: 清理构建目录
```powershell
# 删除有问题的 Qt 部署文件
Remove-Item E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\QtQuick -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item E:\QT_CMAKE\ffmpeg_musicer_player_build\Release\QtQuick -Recurse -Force -ErrorAction SilentlyContinue

# 或者完全清理构建目录(会触发完整重建)
Remove-Item E:\QT_CMAKE\ffmpeg_musicer_player_build\* -Recurse -Force
```

#### 步骤 3: 重新构建
在 Qt Creator 中点击"重新构建"

### 方案 3: 手动部署(开发时)
如果不想每次都自动部署,可以禁用自动部署并手动运行:

#### 修改 CMakeLists.txt
将 `windeployqt` 相关代码注释掉:

```cmake
# if(WINDEPLOYQT_EXECUTABLE)
#     message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
#     ...
# endif()
```

#### 首次构建后手动部署
```powershell
# Debug 版本
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe `
    --debug `
    --no-translations `
    --qmldir E:\FFmpeg_whisper\ffmpeg_music_player\qml `
    E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\ffmpeg_music_player.exe

# Release 版本
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe `
    --release `
    --no-translations `
    --qmldir E:\FFmpeg_whisper\ffmpeg_music_player\qml `
    E:\QT_CMAKE\ffmpeg_musicer_player_build\Release\ffmpeg_music_player.exe
```

### 方案 4: 使用 Qt Creator 的清理构建
1. 在 Qt Creator 中,点击左侧的 **构建** 按钮
2. 选择 **清理** (Clean)
3. 等待清理完成
4. 点击 **构建** (Build)

## 预防措施

### 1. 构建前确保程序已关闭
- 运行程序后,确保从 Qt Creator 停止程序
- 检查任务管理器中是否还有残留进程

### 2. 使用输出目录隔离
确保构建目录与源代码目录分离(已配置):
- 源代码: `E:\FFmpeg_whisper\ffmpeg_music_player\`
- 构建输出: `E:\QT_CMAKE\ffmpeg_musicer_player_build\`

### 3. 防病毒软件排除
将构建目录添加到防病毒软件的排除列表:
- Windows Defender 排除: 设置 → 更新和安全 → Windows 安全中心 → 病毒和威胁防护 → 管理设置 → 排除项
- 添加路径: `E:\QT_CMAKE\ffmpeg_musicer_player_build\`

### 4. 定期清理
每隔一段时间完全清理构建目录:
```powershell
Remove-Item E:\QT_CMAKE\ffmpeg_musicer_player_build\* -Recurse -Force
```

## 故障排查

### 问题: 仍然出现文件占用错误
**解决**: 
1. 重启计算机
2. 使用 Process Explorer 查找占用文件的进程
3. 以管理员身份运行 Qt Creator

### 问题: windeployqt 找不到 QML 文件
**解决**: 
确保 `qml` 目录存在且包含 QML 文件:
```powershell
Test-Path E:\FFmpeg_whisper\ffmpeg_music_player\qml
```

### 问题: 部署后程序仍然缺少 DLL
**解决**: 
手动检查部署结果:
```powershell
# 检查 Debug 目录
Get-ChildItem E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\*.dll

# 检查 Qt 插件
Get-ChildItem E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\platforms\
Get-ChildItem E:\QT_CMAKE\ffmpeg_musicer_player_build\Debug\QtQuick\
```

## 技术细节

### 智能部署脚本工作原理
1. **检查标记文件**: `.qt_deployed_Debug` 或 `.qt_deployed_Release`
2. **比较时间戳**: 可执行文件 vs 标记文件
3. **条件部署**: 只在必要时运行 `windeployqt`
4. **错误处理**: 失败时创建标记文件,避免无限重试

### 部署包含的内容
- Qt 核心 DLL (Qt5Core.dll, Qt5Gui.dll 等)
- Qt Quick 运行时和 QML 模块
- 平台插件 (platforms/qwindows.dll)
- FFmpeg 库 (手动复制)
- 插件依赖 (whisper.dll, ggml.dll 等)

## 相关文件
- `CMakeLists.txt`: 主构建配置
- `cmake/deploy_qt.cmake`: Qt 智能部署脚本
- 构建输出: `E:\QT_CMAKE\ffmpeg_musicer_player_build\`

## 参考链接
- [Qt windeployqt 文档](https://doc.qt.io/qt-5/windows-deployment.html)
- [CMake add_custom_command](https://cmake.org/cmake/help/latest/command/add_custom_command.html)
