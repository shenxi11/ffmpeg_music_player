# Qt 5.14 构建指南

## 问题说明

您遇到的错误：
```
Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
CMake Error: Unknown CMake command "qt_create_translation"
```

这是因为：
1. Qt5 和 Qt6 的 CMake 命令不同
2. Vulkan 是可选的，警告可以忽略

## 解决方案

### 第一步：检测 Qt 安装

运行以下脚本查看您的 Qt 安装：

```batch
detect_qt.bat
```

这将显示您的 Qt 版本和可用的编译器套件（msvc2015/2017/2019）。

### 第二步：确定正确的 Qt 路径

根据您的 Qt 安装，路径可能是：

```
E:\Qt5.14\5.14.0\msvc2015_64
E:\Qt5.14\5.14.0\msvc2017_64
E:\Qt5.14\5.14.1\msvc2017_64
E:\Qt5.14\5.14.2\msvc2017_64
E:\Qt5.14\5.14.2\msvc2019_64
```

### 第三步：更新构建脚本

编辑 `build_cmake.bat`，修改第 10 行：

```batch
set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
```

改为您实际的 Qt 路径。

### 第四步：选择正确的 Visual Studio 版本

Qt5.14 的不同编译器套件需要对应的 Visual Studio 版本：

| Qt Kit | 需要的 Visual Studio |
|--------|---------------------|
| msvc2015_64 | Visual Studio 2015 |
| msvc2017_64 | Visual Studio 2017 或 2019 或 2022 |
| msvc2019_64 | Visual Studio 2019 或 2022 |

**推荐**: 使用 `msvc2017_64`，因为它兼容 VS2017/2019/2022。

### 第五步：修改 CMake 生成器（如果需要）

如果您使用的是 Qt 5.14 的 msvc2015 套件，需要修改 `build_cmake.bat`：

#### 对于 msvc2015_64：

```batch
cmake -G "Visual Studio 14 2015" -A x64 ...
```

#### 对于 msvc2017_64（推荐）：

```batch
cmake -G "Visual Studio 15 2017" -A x64 ...
```

或使用 VS2022（向后兼容）：

```batch
cmake -G "Visual Studio 17 2022" -A x64 ...
```

#### 对于 msvc2019_64：

```batch
cmake -G "Visual Studio 16 2019" -A x64 ...
```

或使用 VS2022（向后兼容）：

```batch
cmake -G "Visual Studio 17 2022" -A x64 ...
```

## 快速修复步骤

### 方案 A：自动检测并配置（推荐）

1. **检测 Qt 安装**：
   ```batch
   detect_qt.bat
   ```

2. **记下输出的正确路径**，例如：
   ```
   E:\Qt5.14\5.14.2\msvc2017_64
   ```

3. **编辑 `build_cmake.bat`**，更新第 10 行：
   ```batch
   set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
   ```

4. **运行构建**：
   ```batch
   build_cmake.bat
   ```

### 方案 B：手动配置

1. **查看您的 Qt5.14 目录**：
   ```batch
   dir E:\Qt5.14
   ```

2. **选择一个版本**（如 5.14.2）：
   ```batch
   dir E:\Qt5.14\5.14.2
   ```

3. **选择编译器套件**（推荐 msvc2017_64）：
   ```batch
   set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
   ```

4. **验证 qmake 存在**：
   ```batch
   dir %QT_DIR%\bin\qmake.exe
   ```

5. **运行构建**：
   ```batch
   build_cmake.bat
   ```

## 常见问题

### Q1: 找不到 Qt

**错误**: `Could NOT find Qt5`

**解决**: 
```batch
set CMAKE_PREFIX_PATH=E:\Qt5.14\5.14.2\msvc2017_64
```

### Q2: Vulkan 警告

**警告**: `Could NOT find WrapVulkanHeaders`

**说明**: 这是一个可以忽略的警告。CMakeLists.txt 已更新，禁用了 Vulkan 支持。

### Q3: 编译器版本不匹配

**错误**: 链接错误或运行时崩溃

**解决**: 确保：
- Qt 套件（msvc2017_64）与 Visual Studio 版本匹配
- 使用相同的编译器编译所有组件

### Q4: CMake 版本太旧

**错误**: `CMake 3.x or higher is required`

**解决**: 更新 CMake：
```
https://cmake.org/download/
```

Qt 5.14 建议使用 CMake 3.16 或更高版本。

## 完整示例

假设您的 Qt 安装在 `E:\Qt5.14\5.14.2\msvc2017_64`：

### 步骤 1: 更新 build_cmake.bat

```batch
@echo off
REM ... 其他代码 ...

set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
set CMAKE_PREFIX_PATH=%QT_DIR%
set PATH=%QT_DIR%\bin;%PATH%

REM ... 其他代码 ...

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..
```

### 步骤 2: 运行构建

```batch
build_cmake.bat
```

### 步骤 3: 检查输出

成功的输出应该显示：

```
-- Found Qt5: version 5.14.x
-- Configuring done
-- Generating done
-- Build files have been written to: ...
```

## 验证 Qt 路径的命令

```batch
@echo off
set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64

echo Checking Qt installation...
echo.

if exist "%QT_DIR%\bin\qmake.exe" (
    echo [OK] qmake found
    "%QT_DIR%\bin\qmake.exe" --version
) else (
    echo [ERROR] qmake not found at: %QT_DIR%\bin\qmake.exe
)

if exist "%QT_DIR%\lib\cmake\Qt5\Qt5Config.cmake" (
    echo [OK] Qt5Config.cmake found
) else (
    echo [ERROR] Qt5Config.cmake not found
)

echo.
pause
```

保存为 `verify_qt.bat` 并运行。

## 构建后的检查

成功构建后，检查：

```batch
dir build\bin\Release\ffmpeg_music_player.exe
dir build\bin\Release\plugin\audio_converter_plugin.dll
```

如果缺少 Qt DLL，运行：

```batch
cd build\bin\Release
E:\Qt5.14\5.14.2\msvc2017_64\bin\windeployqt.exe ffmpeg_music_player.exe
```

## 需要帮助？

如果仍有问题，请提供：

1. `detect_qt.bat` 的输出
2. CMake 配置时的完整错误信息
3. Qt 版本和编译器套件

## 总结

✅ 已修复的问题：
- Qt5 和 Qt6 兼容性
- `qt_create_translation` 命令错误
- Vulkan 警告

🔧 需要您做的：
1. 运行 `detect_qt.bat` 确定正确的 Qt 路径
2. 更新 `build_cmake.bat` 中的 `QT_DIR`
3. 运行 `build_cmake.bat` 构建项目

祝构建成功！🎉
