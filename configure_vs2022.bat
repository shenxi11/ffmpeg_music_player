@echo off
REM CMake 配置脚本 - 仅配置，不构建

echo ================================================
echo CMake Configuration Script
echo ================================================
echo.

REM 设置 Qt 路径（根据实际安装路径修改）
set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
set CMAKE_PREFIX_PATH=%QT_DIR%
set PATH=%QT_DIR%\bin;%PATH%

echo Qt Directory: %QT_DIR%
echo.

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置 CMake
echo Configuring with Visual Studio 2022...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH% ..

cd ..

echo.
echo ================================================
echo Configuration complete!
echo ================================================
echo.
echo Next steps:
echo   1. Open build\ffmpeg_music_player.sln in Visual Studio 2022
echo   2. Build the solution
echo.
pause
