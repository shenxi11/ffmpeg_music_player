@echo off
REM CMake 构建脚本 - VS2022

echo ================================================
echo FFmpeg Music Player - CMake Build Script
echo ================================================
echo.

REM 设置 Qt 路径（根据实际安装路径修改）
set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
REM 如果使用 Qt6，修改为：
REM set QT_DIR=C:\Qt\6.6.0\msvc2019_64
REM 如果使用其他 Qt5 版本，修改为：
REM set QT_DIR=E:\Qt5.14\5.15.2\msvc2019_64

set CMAKE_PREFIX_PATH=%QT_DIR%
set PATH=%QT_DIR%\bin;%PATH%

echo Qt Directory: %QT_DIR%
echo.

REM 选择构建类型
set BUILD_TYPE=Release
if "%1"=="debug" set BUILD_TYPE=Debug
if "%1"=="Debug" set BUILD_TYPE=Debug

echo Build Type: %BUILD_TYPE%
echo.

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置 CMake（使用 VS2022）
echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Configuration successful!
echo.

REM 构建项目
echo Building project...
cmake --build . --config %BUILD_TYPE% -j 8

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ================================================
echo Build completed successfully!
echo ================================================
echo.
echo Executable: build\bin\%BUILD_TYPE%\ffmpeg_music_player.exe
echo Plugins: build\bin\%BUILD_TYPE%\plugin\
echo.
echo To run the application:
echo   cd build\bin\%BUILD_TYPE%
echo   ffmpeg_music_player.exe
echo.
pause
