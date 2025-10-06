QT += widgets multimedia
CONFIG += plugin c++17
TEMPLATE = lib
TARGET = audio_converter_plugin

# 输出到主程序的plugin目录
DESTDIR = ../../plugin

# 定义插件标志
DEFINES += AUDIO_CONVERTER_PLUGIN

# 包含主程序的头文件路径
INCLUDEPATH += ../../

# FFmpeg 和 whisper 的路径
INCLUDEPATH += E:/ffmpeg-4.4/include

LIBS += -LE:/ffmpeg-4.4/lib \
        -lavcodec \
        -lavformat \
        -lavutil \
        -lavdevice \
        -lswscale \
        -lswresample

HEADERS += \
    audio_converter_plugin.h \
    ../../audio_converter.h

SOURCES += \
    audio_converter_plugin.cpp \
    ../../audio_converter.cpp

# 插件不需要版本号
CONFIG += plugin_no_soname

# Windows下的特殊配置
win32 {
    CONFIG += skip_target_version_ext
}
