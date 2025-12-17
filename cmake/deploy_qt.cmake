# Qt 部署脚本 - 智能检测是否需要重新部署
# 用法: cmake -P deploy_qt.cmake <target_file> <windeployqt_exe> <qml_dir> <config> <qt_bin_dir>

# 获取参数 (CMAKE_ARGV[0] = cmake, CMAKE_ARGV[1] = -P, CMAKE_ARGV[2] = script_path)
# 实际参数从 CMAKE_ARGV[3] 开始
set(TARGET_FILE "${CMAKE_ARGV3}")
set(WINDEPLOYQT_EXE "${CMAKE_ARGV4}")
set(QML_DIR "${CMAKE_ARGV5}")
set(BUILD_CONFIG "${CMAKE_ARGV6}")
set(QT_BIN_DIR "${CMAKE_ARGV7}")

# 验证参数
if(NOT TARGET_FILE OR NOT WINDEPLOYQT_EXE OR NOT QML_DIR OR NOT BUILD_CONFIG OR NOT QT_BIN_DIR)
    message(STATUS "Received arguments:")
    message(STATUS "  TARGET_FILE: ${TARGET_FILE}")
    message(STATUS "  WINDEPLOYQT_EXE: ${WINDEPLOYQT_EXE}")
    message(STATUS "  QML_DIR: ${QML_DIR}")
    message(STATUS "  BUILD_CONFIG: ${BUILD_CONFIG}")
    message(STATUS "  QT_BIN_DIR: ${QT_BIN_DIR}")
    message(FATAL_ERROR "Missing required arguments!")
endif()

# 获取目标文件所在目录
get_filename_component(TARGET_DIR "${TARGET_FILE}" DIRECTORY)

# 部署标记文件
set(DEPLOY_MARKER "${TARGET_DIR}/.qt_deployed_${BUILD_CONFIG}")

# 检查是否需要部署
set(NEED_DEPLOY FALSE)

# 1. 检查标记文件是否存在
if(NOT EXISTS "${DEPLOY_MARKER}")
    set(NEED_DEPLOY TRUE)
    message(STATUS "Qt deployment marker not found, will deploy Qt dependencies")
else()
    # 2. 比较标记文件和可执行文件的修改时间
    file(TIMESTAMP "${DEPLOY_MARKER}" MARKER_TIME)
    file(TIMESTAMP "${TARGET_FILE}" TARGET_TIME)
    
    if("${TARGET_TIME}" STRGREATER "${MARKER_TIME}")
        set(NEED_DEPLOY TRUE)
        message(STATUS "Executable newer than deployment marker, will redeploy Qt dependencies")
    else()
        message(STATUS "Qt dependencies already deployed and up-to-date, skipping windeployqt")
    endif()
endif()

# 3. 执行部署
if(NEED_DEPLOY)
    message(STATUS "Deploying Qt dependencies for ${BUILD_CONFIG} configuration...")
    
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
    execute_process(
        COMMAND "${WINDEPLOYQT_EXE}" ${DEPLOY_ARGS}
        WORKING_DIRECTORY "${TARGET_DIR}"
        RESULT_VARIABLE DEPLOY_RESULT
        OUTPUT_VARIABLE DEPLOY_OUTPUT
        ERROR_VARIABLE DEPLOY_ERROR
    )
    
    # 检查执行结果
    if(DEPLOY_RESULT EQUAL 0)
        message(STATUS "Qt deployment successful")
        # 创建/更新标记文件
        file(WRITE "${DEPLOY_MARKER}" "Qt deployed successfully at ${CMAKE_CURRENT_TIMESTAMP}\n")
        file(APPEND "${DEPLOY_MARKER}" "Configuration: ${BUILD_CONFIG}\n")
        file(APPEND "${DEPLOY_MARKER}" "Target: ${TARGET_FILE}\n")
    else()
        message(WARNING "Qt deployment failed with code ${DEPLOY_RESULT}")
        if(DEPLOY_OUTPUT)
            message(STATUS "Output: ${DEPLOY_OUTPUT}")
        endif()
        if(DEPLOY_ERROR)
            message(WARNING "Error: ${DEPLOY_ERROR}")
        endif()
        # 即使失败也创建标记文件，避免每次都尝试（可能是权限问题）
        # 用户可以手动删除此文件来强制重新部署
        file(WRITE "${DEPLOY_MARKER}" "Qt deployment attempted but failed\n")
        file(APPEND "${DEPLOY_MARKER}" "You can delete this file to retry deployment\n")
    endif()
endif()
