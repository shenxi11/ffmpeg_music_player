@echo off
REM CMake 配置检查脚本

echo ================================================
echo CMake Configuration Checker
echo ================================================
echo.

REM 检查 CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found in PATH
    echo Please install CMake from: https://cmake.org/download/
    echo.
) else (
    echo [OK] CMake found
    cmake --version | findstr /C:"cmake version"
    echo.
)

REM 检查 Visual Studio 2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" (
    echo [OK] Visual Studio 2022 Community found
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe" (
    echo [OK] Visual Studio 2022 Professional found
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe" (
    echo [OK] Visual Studio 2022 Enterprise found
) else (
    echo [WARNING] Visual Studio 2022 not found in default location
    echo Please ensure VS2022 is installed
)
echo.

REM 检查 Qt
set QT_DIRS=C:\Qt\6.6.0\msvc2019_64 C:\Qt\6.5.0\msvc2019_64 C:\Qt\5.15.2\msvc2019_64
set QT_FOUND=0

for %%d in (%QT_DIRS%) do (
    if exist "%%d\bin\qmake.exe" (
        echo [OK] Qt found at: %%d
        "%%d\bin\qmake.exe" --version
        set QT_FOUND=1
        goto :qt_done
    )
)

:qt_done
if %QT_FOUND%==0 (
    echo [ERROR] Qt not found in common locations
    echo Checked: %QT_DIRS%
    echo Please set QT_DIR in build_cmake.bat
)
echo.

REM 检查 FFmpeg
set FFMPEG_DIR=E:\ffmpeg-4.4
if exist "%FFMPEG_DIR%\include\libavcodec\avcodec.h" (
    echo [OK] FFmpeg found at: %FFMPEG_DIR%
) else (
    echo [WARNING] FFmpeg not found at: %FFMPEG_DIR%
    echo Please update FFMPEG_DIR in CMakeLists.txt
)
echo.

REM 检查 Whisper
set WHISPER_DIR=E:\whisper.cpp\whisper.cpp-master\whisper.cpp-master
if exist "%WHISPER_DIR%\whisper.h" (
    echo [OK] Whisper.cpp found at: %WHISPER_DIR%
) else (
    echo [WARNING] Whisper.cpp not found at: %WHISPER_DIR%
    echo Please update WHISPER_DIR in CMakeLists.txt
)
echo.

REM 检查构建目录
if exist "build" (
    echo [INFO] Build directory exists
) else (
    echo [INFO] Build directory will be created on first build
)
echo.

REM 检查项目文件
if exist "CMakeLists.txt" (
    echo [OK] CMakeLists.txt found
) else (
    echo [ERROR] CMakeLists.txt not found
    echo Please run this script from project root directory
)

if exist "plugins\audio_converter_plugin\CMakeLists.txt" (
    echo [OK] Plugin CMakeLists.txt found
) else (
    echo [WARNING] Plugin CMakeLists.txt not found
)
echo.

echo ================================================
echo Configuration Check Complete
echo ================================================
echo.
echo Next steps:
echo   1. Fix any [ERROR] items above
echo   2. Update paths for [WARNING] items if needed
echo   3. Run build_cmake.bat to build the project
echo.
pause
