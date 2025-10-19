# 🚀 快速开始 - VS2022 + CMake

## 一分钟开始

### 前提条件

- ✅ Visual Studio 2022（已安装 C++ 桌面开发工作负载）
- ✅ CMake 3.16+
- ✅ Qt 6.6.0 或 Qt 5.15+（MSVC 2019/2022 版本）

### 步骤 1：配置 Qt 路径

编辑 `build_cmake.bat`，修改第 10 行：

```batch
set QT_DIR=C:\Qt\6.6.0\msvc2019_64
```

改为你的 Qt 安装路径。

### 步骤 2：构建项目

双击运行：

```
build_cmake.bat
```

### 步骤 3：运行程序

```
cd build\bin\Release
ffmpeg_music_player.exe
```

**完成！** 🎉

---

## VS2022 IDE 使用

### 方法 1：直接打开 CMake 项目

1. 打开 Visual Studio 2022
2. `文件` -> `打开` -> `文件夹`
3. 选择项目根目录
4. VS 自动配置 CMake
5. 选择配置：`vs2022-x64-release`
6. `生成` -> `全部生成`

### 方法 2：生成 .sln 解决方案

1. 双击运行 `configure_vs2022.bat`
2. 打开 `build\ffmpeg_music_player.sln`
3. 选择 Release 或 Debug
4. `生成` -> `生成解决方案`

---

## 常见问题速查

### ❌ 找不到 Qt

```batch
set CMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
```

### ❌ 找不到 FFmpeg

编辑 `CMakeLists.txt`：

```cmake
set(FFMPEG_DIR "你的FFmpeg路径")
```

### ❌ 运行时缺少 DLL

```batch
cd build\bin\Release
windeployqt ffmpeg_music_player.exe
```

---

## 目录结构

```
build/
  └── bin/
      └── Release/
          ├── ffmpeg_music_player.exe  ← 主程序
          └── plugin/
              └── audio_converter_plugin.dll  ← 插件
```

---

## 调试

在 Visual Studio 中：

1. 设置 `ffmpeg_music_player` 为启动项目
2. 设置断点
3. 按 `F5` 开始调试

---

## 需要帮助？

详细文档：[CMAKE_BUILD_GUIDE.md](CMAKE_BUILD_GUIDE.md)

---

**祝构建成功！** 🎵
