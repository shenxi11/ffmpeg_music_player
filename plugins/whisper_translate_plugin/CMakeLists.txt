# whisper_translate_plugin CMakeLists

set(FFMPEG_DIR "E:/ffmpeg4.4/bin")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/../include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/../lib")

set(WHISPER_DIR "E:/whisper.cpp/whisper.cpp-master/whisper.cpp-master")
set(WHISPER_INCLUDE_DIR
    "${WHISPER_DIR}"
    "${WHISPER_DIR}/include"
    "${WHISPER_DIR}/ggml/include"
)
set(WHISPER_LIB_DIR "${WHISPER_DIR}/build/src/Release")
set(GGML_LIB_DIR "${WHISPER_DIR}/build/ggml/src/Release")
set(WHISPER_DLL_DIR "${WHISPER_DIR}/build/bin/Release")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/app
    ${FFMPEG_INCLUDE_DIR}
    ${WHISPER_INCLUDE_DIR}
)

link_directories(
    ${FFMPEG_LIB_DIR}
    ${WHISPER_LIB_DIR}
    ${GGML_LIB_DIR}
)

set(PLUGIN_SOURCES
    whisper_translate_plugin.cpp
    whisper_translate_plugin.h
    translate_widget.cpp
    translate_widget.h
    take_pcm.cpp
    take_pcm.h
)

set(PLUGIN_TARGET_NAME whisper_translate_plugin)

add_library(${PLUGIN_TARGET_NAME} SHARED
    ${PLUGIN_SOURCES}
)

target_compile_definitions(${PLUGIN_TARGET_NAME} PRIVATE
    WHISPER_TRANSLATE_PLUGIN
)

target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Concurrent
    avcodec
    avformat
    avutil
    avdevice
    swscale
    swresample
    whisper
    ggml
    ggml-base
    ggml-cpu
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

    set(WHISPER_DLLS
        "${WHISPER_DLL_DIR}/whisper.dll"
        "${WHISPER_DLL_DIR}/ggml.dll"
        "${WHISPER_DLL_DIR}/ggml-base.dll"
        "${WHISPER_DLL_DIR}/ggml-cpu.dll"
    )

    set(RESOURCE_DIR_DEBUG "${CMAKE_BINARY_DIR}/Debug/resource")
    set(RESOURCE_DIR_RELEASE "${CMAKE_BINARY_DIR}/Release/resource")

    foreach(DLL ${WHISPER_DLLS})
        if(EXISTS ${DLL})
            add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DIR_DEBUG}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL}
                ${RESOURCE_DIR_DEBUG}
                COMMENT "Copying Whisper DLL to resource (Debug): ${DLL}"
            )
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

add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper_translate_plugin.json
    $<TARGET_FILE_DIR:${PLUGIN_TARGET_NAME}>/whisper_translate_plugin.json
)

message(STATUS "------------------------------------------------")
message(STATUS "Whisper Translate Plugin Configuration")
message(STATUS "------------------------------------------------")
message(STATUS "Plugin Name: ${PLUGIN_TARGET_NAME}")
message(STATUS "Plugin Output: ${PLUGIN_OUTPUT_DIR}")
message(STATUS "Whisper Directory: ${WHISPER_DIR}")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "------------------------------------------------")
