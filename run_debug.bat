@echo off
cd /d E:\FFmpeg_whisper\ffmpeg_music_player\build\Debug
echo ============================================
echo 启动 FFmpeg Music Player (Debug模式)
echo ============================================
echo.
echo 检查插件目录...
dir plugin\*.dll
echo.
echo 启动程序并显示调试输出...
echo.
ffmpeg_music_player.exe 2>&1
echo.
echo ============================================
echo 程序已退出
echo ============================================
pause
