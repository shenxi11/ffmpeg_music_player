# Windows 安装包上线计划与操作手册

## 1. 目标
- 对外发布一个可下载的 `Setup.exe`。
- 用户通过安装向导可自定义安装目录。
- 支持主程序必装、插件可选安装。
- 安装后可创建桌面/开始菜单快捷方式，并支持标准卸载。

## 2. 采用方案（成熟做法）
- 打包工具：`Inno Setup 6`（Windows 客户端常用方案，支持组件化安装与安装目录自定义）。
- 构建来源：`CMake Release` 产物。
- 分层流程：
  1. 构建层：`cmake --build ... --config Release`
  2. 分发层：将运行时文件整理到 `staging`（过滤调试符号）
  3. 安装层：使用 `ISCC` 编译 `.iss` 生成 `Setup.exe`

## 3. 已落地文件
- 安装器脚本：`packaging/windows/cloudmusic_installer.iss`
- 一键打包脚本：`packaging/windows/build_windows_setup.ps1`

## 4. 安装器能力
- 自定义安装路径。
- 安装类型：`Full` / `Minimal` / `Custom`。
- 组件：
  - 主程序（必装）
  - `audio_converter` 插件（可选）
  - `whisper_translate` 插件（可选）
- 任务：桌面快捷方式、开始菜单快捷方式。

## 5. 发布前检查清单
- 使用 `Release` 配置构建。
- 产物中存在 `ffmpeg_music_player.exe`。
- staging 目录不存在 Debug 运行库（如 `Qt5Cored.dll`）。
- 安装后至少验证：启动、连接服务器、音频播放、卸载。

## 6. 实际打包命令
```powershell
powershell -ExecutionPolicy Bypass -File .\packaging\windows\build_windows_setup.ps1 `
  -BuildDir E:\FFmpeg_whisper\ffmpeg_music_player_build `
  -Config Release `
  -Version 1.0.0
```

可选参数：
- `-SkipBuild`：跳过构建，仅生成安装包。
- `-IsccPath`：指定 `ISCC.exe` 路径。
- `-OutputDir`：指定安装包输出目录。
- `-StageDir`：指定 staging 目录。

## 7. 输出目录
- 安装包：`<BuildDir>\package\output\`
- 中间产物：`<BuildDir>\package\staging\Release\`

## 8. 网站发布建议
- 下载区提供：
  - `YunMusic_Setup_x.y.z.exe`
  - `SHA256` 校验值
  - 当前版本更新说明
- 命名规范：`YunMusic_Setup_1.0.0.exe`

## 9. 下一步优化建议
- 为安装包加入代码签名（降低 SmartScreen 告警概率）。
- 在 CI 中接入自动打包与自动上传产物。
- 增加便携版 zip，覆盖高级用户场景。
