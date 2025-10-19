# Whisper 翻译插件 CMakeLists
# 这是主项目的子项目，不需要重新定义 project

# 注意：CMAKE_CXX_STANDARD, CMAKE_AUTOMOC 等已在父项目中设置
# 注意：Qt 已在父项目中查找，可以直接使用

# FFmpeg 路径配置
set(FFMPEG_DIR "E:/ffmpeg4.4/bin")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/../include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/../lib")

# Whisper.cpp 路径配置
set(WHISPER_DIR "E:/whisper.cpp/whisper.cpp-master/whisper.cpp-master")
set(WHISPER_INCLUDE_DIR 
    "${WHISPER_DIR}"
    "${WHISPER_DIR}/include"
    "${WHISPER_DIR}/ggml/include"
)
set(WHISPER_LIB_DIR "${WHISPER_DIR}/build/src/Release")
set(GGML_LIB_DIR "${WHISPER_DIR}/build/ggml/src/Release")
set(WHISPER_DLL_DIR "${WHISPER_DIR}/build/bin/Release")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}  # 主项目目录，用于访问 plugin_interface.h
    ${FFMPEG_INCLUDE_DIR}
    ${WHISPER_INCLUDE_DIR}
)

# 链接目录
link_directories(
    ${FFMPEG_LIB_DIR}
    ${WHISPER_LIB_DIR}
    ${GGML_LIB_DIR}
)

# 插件源文件
set(PLUGIN_SOURCES
    whisper_translate_plugin.cpp
    whisper_translate_plugin.h
    translate_widget.cpp
    translate_widget.h
    ${CMAKE_SOURCE_DIR}/take_pcm.cpp
    ${CMAKE_SOURCE_DIR}/take_pcm.h
)

# 插件名称
set(PLUGIN_TARGET_NAME whisper_translate_plugin)

# 创建动态库（插件）
add_library(${PLUGIN_TARGET_NAME} SHARED
    ${PLUGIN_SOURCES}
)

# 定义插件标志
target_compile_definitions(${PLUGIN_TARGET_NAME} PRIVATE
    WHISPER_TRANSLATE_PLUGIN
)

# 链接 Qt5 库
target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Concurrent
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

# 链接 Whisper 和 GGML 库
target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    whisper
    ggml
    ggml-base
    ggml-cpu
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
    
    # 复制 Whisper DLL 到 resource 目录（不是 plugin 目录）
    set(WHISPER_DLLS
        "${WHISPER_DLL_DIR}/whisper.dll"
        "${WHISPER_DLL_DIR}/ggml.dll"
        "${WHISPER_DLL_DIR}/ggml-base.dll"
        "${WHISPER_DLL_DIR}/ggml-cpu.dll"
    )
    
    # 确定 resource 目录路径（和主程序在同一级）
    set(RESOURCE_DIR_DEBUG "${CMAKE_BINARY_DIR}/Debug/resource")
    set(RESOURCE_DIR_RELEASE "${CMAKE_BINARY_DIR}/Release/resource")
    
    foreach(DLL ${WHISPER_DLLS})
        if(EXISTS ${DLL})
            # 复制到 Debug/resource
            add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DIR_DEBUG}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL}
                ${RESOURCE_DIR_DEBUG}
                COMMENT "Copying Whisper DLL to resource (Debug): ${DLL}"
            )
            # 复制到 Release/resource
            add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DIR_RELEASE}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL}
                ${RESOURCE_DIR_RELEASE}
                COMMENT "Copying Whisper DLL to resource (Release): ${DLL}"
            )
        else()
            message(WARNING "Whisper DLL not found: ${DLL}")
        endif()
    endforeach()
endif()

# 复制 JSON 元数据文件
add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper_translate_plugin.json
    $<TARGET_FILE_DIR:${PLUGIN_TARGET_NAME}>/whisper_translate_plugin.json
)

# 打印插件信息
message(STATUS "------------------------------------------------")
message(STATUS "Whisper Translate Plugin Configuration")
message(STATUS "------------------------------------------------")
message(STATUS "Plugin Name: ${PLUGIN_TARGET_NAME}")
message(STATUS "Plugin Output: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "Whisper Directory: ${WHISPER_DIR}")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "------------------------------------------------")
