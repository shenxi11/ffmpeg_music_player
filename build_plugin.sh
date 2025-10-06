#!/bin/bash
# 音频转换插件编译脚本
echo "Building Audio Converter Plugin..."

cd "$(dirname "$0")"
cd plugins/audio_converter_plugin

# 清理旧的编译文件
if [ -f Makefile ]; then
    make clean
fi

# 运行 qmake 生成 Makefile
qmake audio_converter_plugin.pro

# 编译插件
make

echo ""
echo "Build complete!"
echo "Plugin output: ../../plugin/libaudio_converter_plugin.so"
