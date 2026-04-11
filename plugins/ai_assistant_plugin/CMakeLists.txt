set(PLUGIN_TARGET_NAME ai_assistant_plugin)

file(GLOB_RECURSE AI_AGENT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/agent/*.cpp
    ${CMAKE_SOURCE_DIR}/src/agent/*.h
)

set(AI_PLUGIN_SOURCES
    ai_assistant_plugin.cpp
    ai_assistant_plugin.h
    ai_assistant_plugin_page.cpp
    ai_assistant_plugin_page.h
    ai_assistant_plugin.qrc
    ${CMAKE_SOURCE_DIR}/src/common/settings_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/common/settings_manager.h
    ${AI_AGENT_SOURCES}
)

add_library(${PLUGIN_TARGET_NAME} SHARED ${AI_PLUGIN_SOURCES})

target_include_directories(${PLUGIN_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Network
    Qt5::Concurrent
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Qml
    Qt5::WebSockets
    llama
)

set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugin"
    PREFIX ""
)

if(WIN32)
    set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES SUFFIX ".dll")
endif()

set(RESOURCE_DIR_DEBUG "${CMAKE_BINARY_DIR}/Debug/resource")
set(RESOURCE_DIR_RELEASE "${CMAKE_BINARY_DIR}/Release/resource")

add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/ai_assistant_plugin.json
        $<TARGET_FILE_DIR:${PLUGIN_TARGET_NAME}>/ai_assistant_plugin.json
)

add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DIR_DEBUG}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:llama>
        $<TARGET_FILE:ggml>
        $<TARGET_FILE:ggml-base>
        $<TARGET_FILE:ggml-cpu>
        ${RESOURCE_DIR_DEBUG}
)

add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DIR_RELEASE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:llama>
        $<TARGET_FILE:ggml>
        $<TARGET_FILE:ggml-base>
        $<TARGET_FILE:ggml-cpu>
        ${RESOURCE_DIR_RELEASE}
)
