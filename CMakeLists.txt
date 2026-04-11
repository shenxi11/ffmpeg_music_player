cmake_minimum_required(VERSION 3.16)

project(ffmpeg_music_player VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 编译器特定设置
if(MSVC)
    # 设置 UTF-8 源文件编码
    add_compile_options(/utf-8 /FS)
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
    OpenGL
    WebSockets
)
find_package(Qt5 REQUIRED COMPONENTS Core)

message(STATUS "Qt5 Version: ${Qt5_VERSION}")
message(STATUS "Qt5 Location: ${Qt5_DIR}")

set(LLAMA_BUILD_COMMON OFF CACHE BOOL "Disable llama common helpers" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama tests" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "Disable llama tools" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama server" FORCE)
set(LLAMA_BUILD_WEBUI OFF CACHE BOOL "Disable llama webui" FORCE)
set(LLAMA_BUILD_HTML OFF CACHE BOOL "Disable llama html output" FORCE)
set(LLAMA_ALL_WARNINGS OFF CACHE BOOL "Disable llama warning policy override" FORCE)
add_subdirectory(third_party/llama.cpp EXCLUDE_FROM_ALL)

# FFmpeg 路径配置
set(FFMPEG_DIR "E:/ffmpeg4.4/bin")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/../include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/../lib")

# 包含目录
# ????????
set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")

# ????????
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${SRC_ROOT}/*.cpp"
    "${SRC_ROOT}/*.h"
)

list(APPEND PROJECT_SOURCES
    "${SRC_ROOT}/common/cover_lookup.cpp"
    "${SRC_ROOT}/common/cover_lookup.h"
    "${SRC_ROOT}/app/main_widget.menu_auth.cpp"
    "${SRC_ROOT}/app/main_widget.library_connections.cpp"
    "${SRC_ROOT}/app/main_widget.playback_connections.cpp"
    "${SRC_ROOT}/app/main_widget.playback_list_handlers.cpp"
    "${SRC_ROOT}/app/main_widget.playback_state_handlers.cpp"
    "${SRC_ROOT}/app/main_widget.playback_utils.cpp"
    "${SRC_ROOT}/ui/widgets/playback/play_widget.viewmodel_connections.cpp"
    "${SRC_ROOT}/ui/widgets/playback/play_widget.controls_connections.cpp"
    "${SRC_ROOT}/ui/widgets/playback/play_widget.core_connections.cpp"
    "${SRC_ROOT}/ui/widgets/settings/settings_widget.connections.cpp"
    "${SRC_ROOT}/ui/widgets/auth/server_welcome_dialog.connections.cpp"
    "${SRC_ROOT}/ui/widgets/auth/loginwidget.connections.cpp"
    "${SRC_ROOT}/ui/widgets/library/music_list_widget_local.connections.cpp"
    "${SRC_ROOT}/ui/widgets/library/music_list_widget_net.connections.cpp"
    "${SRC_ROOT}/ui/widgets/video/video_list_widget.connections.cpp"
    "${SRC_ROOT}/app/main_menu.connections.cpp"
    "${SRC_ROOT}/app/plugin_host_window.connections.cpp"
    "${SRC_ROOT}/ui/widgets/user/user_widget.connections.cpp"
    "${SRC_ROOT}/viewmodels/MainShellViewModel.connections.cpp"
    "${SRC_ROOT}/viewmodels/OnlineMusicListViewModel.connections.cpp"
    "${SRC_ROOT}/download/download_task_model.connections.cpp"
    "${SRC_ROOT}/audio/AudioService.connections.cpp"
    "${SRC_ROOT}/audio/AudioSession.connections.cpp"
    "${SRC_ROOT}/video/MediaService.connections.cpp"
    "${SRC_ROOT}/video/VideoPlayerWindow.connections.cpp"
    "${SRC_ROOT}/viewmodels/MainShellViewModel.cpp"
    "${SRC_ROOT}/viewmodels/MainShellViewModel.h"
    "${SRC_ROOT}/viewmodels/LoginViewModel.cpp"
    "${SRC_ROOT}/viewmodels/LoginViewModel.h"
    "${SRC_ROOT}/viewmodels/LocalMusicListViewModel.cpp"
    "${SRC_ROOT}/viewmodels/LocalMusicListViewModel.h"
    "${SRC_ROOT}/viewmodels/OnlineMusicListViewModel.cpp"
    "${SRC_ROOT}/viewmodels/OnlineMusicListViewModel.h"
    "${SRC_ROOT}/viewmodels/ServerWelcomeViewModel.cpp"
    "${SRC_ROOT}/viewmodels/ServerWelcomeViewModel.h"
    "${SRC_ROOT}/viewmodels/SettingsViewModel.cpp"
    "${SRC_ROOT}/viewmodels/SettingsViewModel.h"
    "${SRC_ROOT}/viewmodels/VideoListViewModel.cpp"
    "${SRC_ROOT}/viewmodels/VideoListViewModel.h"
)

# ?????????????????
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/PlayViewModel\.cpp$")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/main_qml\.cpp$")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/qml_bridge\.cpp$")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/qml_bridge\.h$")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/src/agent/.*")

# include ?????? include + ????
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SRC_ROOT}
)

file(GLOB_RECURSE MODULE_DIRS LIST_DIRECTORIES true
    "${SRC_ROOT}/*"
)
foreach(dir ${MODULE_DIRS})
    if(IS_DIRECTORY "${dir}")
        list(APPEND PROJECT_INCLUDE_DIRS "${dir}")
    endif()
endforeach()
list(REMOVE_DUPLICATES PROJECT_INCLUDE_DIRS)

include_directories(
    ${PROJECT_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
)


# 链接目录
link_directories(
    ${FFMPEG_LIB_DIR}
)

# 源文件
# ????
set(PROJECT_RESOURCES
    pic.qrc
    qml.qrc
    design_ui.qrc
)

if(WIN32)
    list(APPEND PROJECT_RESOURCES app_icon.rc)
endif()

# 翻译文件
set(PROJECT_TRANSLATIONS
    untitled_zh_CN.ts
)

# 创建可执行文件（Qt5）

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_RESOURCES}
)

add_executable(agent_embedded_smoke
    tools/agent_embedded_smoke.cpp
    src/agent/AgentLocalModelGateway.cpp
    src/agent/AgentEmbeddedLlamaEngine.cpp
    src/common/settings_manager.cpp
)


target_link_libraries(ffmpeg_music_player PRIVATE Qt5::Core)
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
    Qt5::OpenGL
    Qt5::WebSockets
)

target_link_libraries(agent_embedded_smoke PRIVATE
    Qt5::Core
    Qt5::Concurrent
    llama
)

target_include_directories(agent_embedded_smoke PRIVATE
    ${PROJECT_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
)

add_custom_command(TARGET agent_embedded_smoke POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:llama>
        $<TARGET_FILE:ggml>
        $<TARGET_FILE:ggml-base>
        $<TARGET_FILE:ggml-cpu>
        $<TARGET_FILE_DIR:agent_embedded_smoke>
)

# 链接 FFmpeg 库

target_link_libraries(${PROJECT_NAME} PRIVATE
    avcodec
    avformat
    avutil
    avfilter
    avdevice
    swscale
    swresample
)

# 链接 OpenGL 库

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
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
add_subdirectory(plugins/ai_assistant_plugin)

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
        "${FFMPEG_DIR}/avfilter-7.dll"
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
            # 使用 PowerShell 脚本强制清理 Qt 部署文件（忽略错误）
            COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/clean_qt_deploy.ps1" -TargetDir "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
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
