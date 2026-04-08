# Windows 安装包上线计划与操作手册（当前版）

## 1. 目标
本文档用于说明当前 Windows 安装包的构建与上线方式，并补充当前版本启动链特点：首次进入软件后会先显示欢迎页，用户可选择服务器验证进入或离线直进。

## 2. 当前打包方案
- 构建来源：`CMake Release` 产物；
- 安装器工具：`Inno Setup 6`；
- 当前脚本：
  - `packaging/windows/cloudmusic_installer.iss`
  - `packaging/windows/build_windows_setup.ps1`

## 3. 当前安装包上线前检查
### 3.1 构建层
- 使用 `Release` 配置构建；
- 主程序 `ffmpeg_music_player.exe` 存在；
- 运行时依赖齐全；
- 不包含 Debug 运行库。

### 3.2 功能层
安装后至少验证：
1. 应用能正常启动；
2. 欢迎页能正常显示；
3. 可验证服务器进入主窗口；
4. 可使用“离线直进”进入本地模式；
5. 音频播放正常；
6. 设置页与返回欢迎页链路正常；
7. 标准卸载可用。

### 3.3 发布层
- 安装包命名遵循版本号；
- 对外提供 SHA256 校验值；
- 建议附本版本更新说明。

## 4. 当前推荐打包命令
```powershell
powershell -ExecutionPolicy Bypass -File .\packaging\windows\build_windows_setup.ps1 `
  -BuildDir E:\FFmpeg_whisper\ffmpeg_music_player_build `
  -Config Release `
  -Version 1.0.0
```

## 5. 当前版本特别说明
- 客户端不再假定必须连接服务器才能进入主界面；
- 上线验证时必须同时覆盖在线进入和离线直进两条启动路径；
- 如果用于答辩或论文展示，离线直进可作为网络不可用时的兜底演示方案。