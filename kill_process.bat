严重性	代码	说明	项目	文件	行	禁止显示状态	详细信息
错误	MSB3073	命令“setlocal
E:\CMake\bin\cmake.exe -E copy_if_different E:/ffmpeg-4.4/bin/avcodec-58.dll E:/FFmpeg_whisper/BUILD/bin/Debug
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
setlocal
E:\CMake\bin\cmake.exe -E copy_if_different E:/ffmpeg-4.4/bin/avformat-58.dll E:/FFmpeg_whisper/BUILD/bin/Debug
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
setlocal
E:\CMake\bin\cmake.exe -E copy_if_different E:/ffmpeg-4.4/bin/avutil-56.dll E:/FFmpeg_whisper/BUILD/bin/Debug
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
setlocal
E:\CMake\bin\cmake.exe -E copy_if_different E:/ffmpeg-4.4/bin/avdevice-58.dll E:/FFmpeg_whisper/BUILD/bin/Debug
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
setlocal
E:\CMake\bin\cmake.exe -E copy_if_different E:/ffmpeg-4.4/bin/swscale-5.dll E:/FFmpeg_whisper/BUILD/bin/Debug
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
setlocal
E:\CMake\bin\cmake.exe -E copy_if_different E:/ffmpeg-4.4/bin/swresample-3.dll E:/FFmpeg_whisper/BUILD/bin/Debug
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
setlocal
E:\CMake\bin\cmake.exe -E echo "Deploying Qt dependencies..."
if %errorlevel% neq 0 goto :cmEnd
D:\QT\6.6.0\mingw_64\bin\windeployqt.exe --no-translations --no-system-d3d-compiler --no-opengl-sw --debug  E:/FFmpeg_whisper/BUILD/bin/Debug/ffmpeg_music_player.exe
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
:VCEnd”已退出，代码为 1。	ffmpeg_music_player	E:\VS_2019\MSBuild\Microsoft\VC\v170\Microsoft.CppCommon.targets	166		
@echo off
echo ========================================
echo   关闭 FFmpeg Music Player 进程
echo ========================================
echo.

taskkill /F /IM ffmpeg_music_player.exe 2>nul

if %errorlevel% == 0 (
    echo [成功] 进程已关闭
) else (
    echo [提示] 没有找到运行中的进程
)

echo.
echo ========================================
pause
