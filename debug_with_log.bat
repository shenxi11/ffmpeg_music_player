@echo off
chcp 65001 > nul
setlocal

REM 设置 Qt5 路径
set PATH=E:\Qt5.14\5.14.2\msvc2017_64\bin;%PATH%

REM 设置 Qt 调试输出
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=*.debug=true

cd /d E:\FFmpeg_whisper\ffmpeg_music_player\build\Debug

echo ============================================
echo FFmpeg Music Player 详细调试启动
echo ============================================
echo.
echo Qt5 路径: E:\Qt5.14\5.14.2\msvc2017_64\bin
echo 插件调试: 已启用
echo.
echo 检查插件文件:
dir /b plugin\*plugin*.dll
echo.
echo ============================================
echo 启动程序 (输出将保存到 debug_log.txt)
echo ============================================
echo.

REM 启动程序并捕获所有输出
ffmpeg_music_player.exe > debug_log.txt 2>&1

echo.
echo 程序已退出，查看 debug_log.txt 了解详情
echo.
type debug_log.txt
echo.
pause
endlocal
