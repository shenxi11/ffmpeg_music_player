set(PLUGIN_TARGET_NAME ai_assistant_plugin)

set(AI_PLUGIN_SOURCES
    ai_assistant_plugin.cpp
    ai_assistant_plugin.h
)

add_library(${PLUGIN_TARGET_NAME} SHARED ${AI_PLUGIN_SOURCES})

target_include_directories(${PLUGIN_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_TARGET_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
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

add_custom_command(TARGET ${PLUGIN_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/ai_assistant_plugin.json
        $<TARGET_FILE_DIR:${PLUGIN_TARGET_NAME}>/ai_assistant_plugin.json
)
