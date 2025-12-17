# 条件部署脚本 - 只在需要时运行 windeployqt
# 在目标文件目录中执行

# 参数从环境或命令行传入
set(WINDEPLOYQT_EXE "${CMAKE_ARGV3}")
set(QML_DIR "${CMAKE_ARGV4}")
set(BUILD_CONFIG "${CMAKE_ARGV5}")
set(QT_BIN_DIR "${CMAKE_ARGV6}")
set(TARGET_FILE "${CMAKE_ARGV7}")

# 获取当前目录(应该是可执行文件所在目录)
get_filename_component(CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(TARGET_DIR "${TARGET_FILE}" DIRECTORY)

# 检查 Qt5Core.dll 是否已存在(作为已部署的标志)
set(QT_CORE_DLL "${TARGET_DIR}/Qt5Core.dll")
set(QT_CORE_DLL_D "${TARGET_DIR}/Qt5Cored.dll")

# 根据配置类型检查对应的DLL
if(BUILD_CONFIG STREQUAL "Debug")
    set(MARKER_DLL "${QT_CORE_DLL_D}")
else()
    set(MARKER_DLL "${QT_CORE_DLL}")
endif()

# 检查是否需要部署
if(EXISTS "${MARKER_DLL}")
    message(STATUS "Qt dependencies already deployed (found ${MARKER_DLL}), skipping windeployqt")
    message(STATUS "To force redeployment, delete Qt DLLs from: ${TARGET_DIR}")
else()
    message(STATUS "Qt dependencies not found, deploying now...")
    
    # 设置 windeployqt 参数
    set(DEPLOY_ARGS
        --no-translations
        --no-system-d3d-compiler
        --no-opengl-sw
        --no-webkit2
        --qmldir "${QML_DIR}"
        --verbose 0
    )
    
    # 根据配置类型添加参数
    if(BUILD_CONFIG STREQUAL "Debug")
        list(APPEND DEPLOY_ARGS --debug)
    else()
        list(APPEND DEPLOY_ARGS --release)
    endif()
    
    list(APPEND DEPLOY_ARGS "${TARGET_FILE}")
    
    # 设置环境变量
    set(ENV{PATH} "${QT_BIN_DIR};$ENV{PATH}")
    
    # 执行 windeployqt
    message(STATUS "Running: ${WINDEPLOYQT_EXE} ${DEPLOY_ARGS}")
    execute_process(
        COMMAND "${WINDEPLOYQT_EXE}" ${DEPLOY_ARGS}
        WORKING_DIRECTORY "${TARGET_DIR}"
        RESULT_VARIABLE DEPLOY_RESULT
        OUTPUT_VARIABLE DEPLOY_OUTPUT
        ERROR_VARIABLE DEPLOY_ERROR
    )
    
    # 检查执行结果
    if(DEPLOY_RESULT EQUAL 0)
        message(STATUS "✅ Qt deployment successful!")
    else()
        message(WARNING "Qt deployment failed with code ${DEPLOY_RESULT}")
        if(DEPLOY_OUTPUT)
            message(STATUS "Output: ${DEPLOY_OUTPUT}")
        endif()
        if(DEPLOY_ERROR)
            message(WARNING "Error: ${DEPLOY_ERROR}")
        endif()
        message(WARNING "Note: You may need to run windeployqt manually or check file permissions")
    endif()
endif()
