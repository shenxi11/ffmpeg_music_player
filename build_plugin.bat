@echo off
REM 音频转换插件编译脚本
echo Building Audio Converter Plugin...

cd /d %~dp0
cd plugins\audio_converter_plugin

REM 清理旧的编译文件
if exist Makefile (
    nmake clean
)

REM 运行 qmake 生成 Makefile
qmake audio_converter_plugin.pro

REM 编译插件
nmake

echo.
echo Build complete!
echo Plugin output: ..\..\plugin\audio_converter_plugin.dll
pause
