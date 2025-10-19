@echo off
setlocal

REM 设置 Qt5 路径为最高优先级
set PATH=E:\Qt5.14\5.14.2\msvc2017_64\bin;%PATH%

cd /d E:\FFmpeg_whisper\ffmpeg_music_player\build\Debug

echo ============================================
echo FFmpeg Music Player 调试启动脚本
echo ============================================
echo.
echo Qt5 路径: E:\Qt5.14\5.14.2\msvc2017_64\bin
echo 当前目录: %CD%
echo.
echo 检查插件目录:
dir /b plugin\*.dll
echo.
echo ============================================
echo 启动程序...
echo ============================================
echo.

REM 启动程序
ffmpeg_music_player.exe

echo.
echo 程序已退出
pause
endlocal
