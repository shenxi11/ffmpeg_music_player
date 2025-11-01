#!/bin/bash

# FFmpeg Android 编译脚本 - 音频版本
# 运行方式: bash compile_ffmpeg_android.sh

set -e  # 遇到错误立即退出

echo "========================================="
echo "  Mobile-FFmpeg Android 编译脚本"
echo "  版本: 音频编译（推荐用于音乐播放器）"
echo "========================================="
echo ""

# 设置环境变量
export ANDROID_NDK_ROOT=/mnt/e/Android/Sdk/ndk/29.0.14206865
export ANDROID_HOME=/mnt/e/Android/Sdk
export GIT_DISCOVERY_ACROSS_FILESYSTEM=1

# 显示配置
echo "✓ Android NDK: $ANDROID_NDK_ROOT"
echo "✓ Android SDK: $ANDROID_HOME"
echo ""

# 检查NDK是否存在
if [ ! -d "$ANDROID_NDK_ROOT" ]; then
    echo "❌ 错误: Android NDK 未找到"
    echo "请确认路径: $ANDROID_NDK_ROOT"
    exit 1
fi

echo "检查 ndk-build..."
if [ -f "$ANDROID_NDK_ROOT/ndk-build" ]; then
    echo "✓ ndk-build 存在"
else
    echo "⚠️  警告: ndk-build 未找到，可能需要NDK r21"
fi

echo ""

# 进入mobile-ffmpeg目录
cd /mnt/e/mobile-ffmpeg-4.4.LTS/mobile-ffmpeg-4.4.LTS

echo "当前目录: $(pwd)"
echo ""

# 显示编译选项
echo "编译配置:"
echo "  架构: arm64-v8a, armeabi-v7a, x86_64"
echo "  音频库: lame (MP3), libvorbis (OGG), opus"
echo "  API Level: 16+ (Android 4.1+)"
echo ""

# 询问确认
read -p "开始编译？这将需要约30-40分钟 (y/n): " confirm
if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
    echo "已取消编译"
    exit 0
fi

echo ""
echo "========================================="
echo "  开始编译 FFmpeg..."
echo "  开始时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "========================================="
echo ""

# 开始编译
./android.sh --lts \
    --disable-arm-v7a-neon \
    --disable-x86 \
    --enable-lame \
    --enable-libvorbis \
    --enable-opus

# 检查编译结果
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "  ✅ 编译成功！"
    echo "  完成时间: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "========================================="
    echo ""
    
    # 查找生成的.aar文件
    AAR_DIR="$HOME/mobile-ffmpeg/prebuilt/android-aar/mobile-ffmpeg"
    if [ -d "$AAR_DIR" ]; then
        echo "生成的.aar文件:"
        ls -lh "$AAR_DIR"/*.aar 2>/dev/null || echo "未找到.aar文件"
        echo ""
        echo "文件位置: $AAR_DIR"
    else
        AAR_DIR="./prebuilt/android-aar/mobile-ffmpeg"
        if [ -d "$AAR_DIR" ]; then
            echo "生成的.aar文件:"
            ls -lh "$AAR_DIR"/*.aar 2>/dev/null
            echo ""
            echo "文件位置: $(pwd)/$AAR_DIR"
        fi
    fi
    
    echo ""
    echo "下一步:"
    echo "1. 将.aar文件复制到Windows:"
    echo "   cp prebuilt/android-aar/mobile-ffmpeg/*.aar /mnt/e/FFmpeg_whisper/ffmpeg_music_player/temp_ffmpeg_download/"
    echo ""
    echo "2. 在Windows中提取.aar:"
    echo "   .\\extract_ffmpeg_aar.ps1"
    echo ""
    echo "3. 验证库文件:"
    echo "   .\\verify_ffmpeg_libs.bat"
    
else
    echo ""
    echo "========================================="
    echo "  ❌ 编译失败"
    echo "========================================="
    echo ""
    echo "请查看日志文件:"
    echo "  cat build.log"
    exit 1
fi
