cmake_minimum_required(VERSION 3.16)

project(ffmpeg_music_player VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 编译器特定设置
if(MSVC)
    # 设置 UTF-8 源文件编码
    add_compile_options(/utf-8)
endif()

# 自动处理 Qt 的 MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 自动检测 Qt5 路径
if(NOT DEFINED CMAKE_PREFIX_PATH)
    message(STATUS "CMAKE_PREFIX_PATH not set, trying to auto-detect Qt5...")
    
    # 常见的 Qt5 安装路径（根据您的系统）
    set(QT5_SEARCH_PATHS
        "E:/Qt5.15/5.15.2/msvc2019_64"
        "E:/Qt5.14/5.14.2/msvc2017_64"
    )
    
    foreach(QT_PATH ${QT5_SEARCH_PATHS})
        if(EXISTS "${QT_PATH}/lib/cmake/Qt5/Qt5Config.cmake")
            set(CMAKE_PREFIX_PATH ${QT_PATH})
            message(STATUS "Found Qt5 at: ${QT_PATH}")
            break()
        endif()
    endforeach()
    
    if(NOT DEFINED CMAKE_PREFIX_PATH)
        message(FATAL_ERROR "Could not auto-detect Qt5 installation!\n"
                            "Please set CMAKE_PREFIX_PATH manually in CMake GUI.\n"
                            "Example: E:/Qt5.14/5.14.2/msvc2017_64")
    endif()
endif()

# 查找 Qt5
find_package(Qt5 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Multimedia 
    Network 
    Concurrent
    LinguistTools
    Quick
    QuickWidgets
    Qml
)

message(STATUS "Qt5 Version: ${Qt5_VERSION}")
message(STATUS "Qt5 Location: ${Qt5_DIR}")

# FFmpeg 路径配置
set(FFMPEG_DIR "E:/ffmpeg4.4/bin")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/../include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/../lib")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIR}
)

# 链接目录
link_directories(
    ${FFMPEG_LIB_DIR}
)

# 源文件
set(PROJECT_SOURCES
    main.cpp
    controlbar.cpp
    controlbar.h
    desk_lrc_widget.cpp
    desk_lrc_widget.h
    desk_lrc_settings.cpp
    desk_lrc_settings.h
    downloadthread.cpp
    downloadthread.h
    headers.h
    httprequest.cpp
    httprequest.h
    logger.cpp
    logger.h
    loginwidget.cpp
    loginwidget.h
    loginwidget_qml.h
    lrc_analyze.cpp
    lrc_analyze.h
    lyrictextedit.cpp
    lyrictextedit.h
    main_widget.cpp
    main_widget.h
    main_menu.cpp
    main_menu.h
    mini_controlbar.cpp
    mini_controlbar.h
    music.h
    music_item.cpp
    music_item.h
    music_item_qml.h
    music_list_widget_qml.h
    music_list_widget_net_qml.h
    music_list_widget.cpp
    music_list_widget.h
    music_list_widget_local.cpp
    music_list_widget_local.h
    music_list_widget_net.cpp
    music_list_widget_net.h
    pianwidget.cpp
    pianwidget.h
    play_widget.cpp
    play_widget.h
    plugin_interface.h
    plugin_manager.cpp
    plugin_manager.h
    process_slider.cpp
    process_slider.h
    rotatingcircleimage.cpp
    rotatingcircleimage.h
    searchbox.cpp
    searchbox.h
    searchbox_qml.h
    take_pcm.cpp
    take_pcm.h
    user_widget.cpp
    user_widget.h
    userwidget_qml.h
    worker.cpp
    worker.h
)

# 资源文件
set(PROJECT_RESOURCES
    pic.qrc
    qml.qrc
)

# 翻译文件
set(PROJECT_TRANSLATIONS
    untitled_zh_CN.ts
)

# 创建可执行文件（Qt5）
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_RESOURCES}
)

# Qt5 翻译
if(Qt5_LinguistTools_FOUND AND PROJECT_TRANSLATIONS)
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${PROJECT_TRANSLATIONS})
endif()

# 链接 Qt5 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Network
    Qt5::Concurrent
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Qml
)

# 链接 FFmpeg 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    avcodec
    avformat
    avutil
    avdevice
    swscale
    swresample
)

# 设置输出目录（使用 CMake 默认路径）
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# 插件输出目录（根据配置类型）
set(PLUGIN_OUTPUT_DIR_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin")
set(PLUGIN_OUTPUT_DIR_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin")
set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/plugin" CACHE PATH "Plugin output directory")
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_DIR_DEBUG})
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_DIR_RELEASE})

# 添加插件子目录（作为子项目）
add_subdirectory(plugins/audio_converter_plugin)
add_subdirectory(plugins/whisper_translate_plugin)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 复制 FFmpeg DLL 到输出目录（Windows）
if(WIN32)
    set(FFMPEG_DLLS
        "${FFMPEG_DIR}/avcodec-58.dll"
        "${FFMPEG_DIR}/avformat-58.dll"
        "${FFMPEG_DIR}/avutil-56.dll"
        "${FFMPEG_DIR}/avdevice-58.dll"
        "${FFMPEG_DIR}/swscale-5.dll"
        "${FFMPEG_DIR}/swresample-3.dll"
    )
    
    foreach(DLL ${FFMPEG_DLLS})
        if(EXISTS ${DLL})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
    
    # 自动部署 Qt5 DLL
    # 使用 windeployqt 工具自动复制所有需要的 Qt DLL 和插件
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt5_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        
        # 为 Debug 和 Release 分别部署
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies..."
            COMMAND ${CMAKE_COMMAND} -E env "PATH=${Qt5_DIR}/../../../bin;$ENV{PATH}" 
                "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-webkit2
                --qmldir "${CMAKE_SOURCE_DIR}/qml"
                --force
                --verbose 0
                $<$<CONFIG:Debug>:--debug>
                $<$<CONFIG:Release>:--release>
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running windeployqt to copy Qt DLLs..."
        )
    else()
        message(WARNING "windeployqt not found. Qt DLLs will not be automatically copied.")
        message(WARNING "You may need to manually copy Qt5 DLLs or run windeployqt manually.")
        message(WARNING "Example: ${Qt5_DIR}/../../../bin/windeployqt.exe build/bin/Release/ffmpeg_music_player.exe")
    endif()
endif()

# 打印配置信息
message(STATUS "================================================")
message(STATUS "FFmpeg Music Player Configuration")
message(STATUS "================================================")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "FFmpeg Directory: ${FFMPEG_DIR}")
message(STATUS "Plugin Output Directory: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "================================================")
