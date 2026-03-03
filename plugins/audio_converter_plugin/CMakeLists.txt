# audio_converter_plugin CMakeLists

set(FFMPEG_DIR "E:/ffmpeg4.4/bin")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/../include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/../lib")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/tools
    ${FFMPEG_INCLUDE_DIR}
)

link_directories(
    ${FFMPEG_LIB_DIR}
)

set(PLUGIN_SOURCES
    audio_converter_plugin.cpp
    audio_converter_plugin.h
    ${CMAKE_SOURCE_DIR}/src/tools/audio_converter.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/audio_converter.h
)

set(PLUGIN_TARGET_NAME audio_converter_plugin)

add_library(${PLUGIN_TARGET_NAME} SHARED
    ${PLUGIN_SOURCES}
)

target_compile_definitions(${PLUGIN_TARGET_NAME} PRIVATE
    AUDIO_CONVERTER_PLUGIN
)

target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    avcodec
    avformat
    avutil
    avdevice
    swscale
    swresample
)

set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin"
    PREFIX ""
)

if(WIN32)
    set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
        SUFFIX ".dll"
    )
endif()

add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_converter_plugin.json
    $<TARGET_FILE_DIR:${PLUGIN_TARGET_NAME}>/audio_converter_plugin.json
)

message(STATUS "------------------------------------------------")
message(STATUS "Audio Converter Plugin Configuration")
message(STATUS "------------------------------------------------")
message(STATUS "Plugin Name: ${PLUGIN_TARGET_NAME}")
message(STATUS "Plugin Output: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "------------------------------------------------")
