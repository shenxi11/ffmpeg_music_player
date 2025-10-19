# 音频转换器插件 CMakeLists
# 这是主项目的子项目，不需要重新定义 project

# 注意：CMAKE_CXX_STANDARD, CMAKE_AUTOMOC 等已在父项目中设置
# 注意：Qt 已在父项目中查找，可以直接使用

# FFmpeg 路径配置
set(FFMPEG_DIR "E:/ffmpeg4.4/bin")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/../include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/../lib")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}  # 主项目目录，用于访问 plugin_interface.h
    ${FFMPEG_INCLUDE_DIR}
)

# 链接目录
link_directories(
    ${FFMPEG_LIB_DIR}
)

# 插件源文件
set(PLUGIN_SOURCES
    audio_converter_plugin.cpp
    audio_converter_plugin.h
    ${CMAKE_SOURCE_DIR}/audio_converter.cpp
    ${CMAKE_SOURCE_DIR}/audio_converter.h
)

# 插件名称
set(PLUGIN_TARGET_NAME audio_converter_plugin)

# 创建动态库（插件）
add_library(${PLUGIN_TARGET_NAME} SHARED
    ${PLUGIN_SOURCES}
)

# 定义插件标志
target_compile_definitions(${PLUGIN_TARGET_NAME} PRIVATE
    AUDIO_CONVERTER_PLUGIN
)

# 链接 Qt5 库
target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
)

# 链接 FFmpeg 库
target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    avcodec
    avformat
    avutil
    avdevice
    swscale
    swresample
)

# 设置插件输出目录（根据配置类型）
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin"
    PREFIX ""  # 移除 lib 前缀
)

# Windows 特定设置
if(WIN32)
    set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
        SUFFIX ".dll"
    )
endif()

# 复制 JSON 元数据文件
add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_converter_plugin.json
    $<TARGET_FILE_DIR:${PLUGIN_TARGET_NAME}>/audio_converter_plugin.json
)

# 打印插件信息
message(STATUS "------------------------------------------------")
message(STATUS "Audio Converter Plugin Configuration")
message(STATUS "------------------------------------------------")
message(STATUS "Plugin Name: ${PLUGIN_TARGET_NAME}")
message(STATUS "Plugin Output: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "------------------------------------------------")
