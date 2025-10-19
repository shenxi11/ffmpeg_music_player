@echo off
REM Qt 版本检测脚本

echo ================================================
echo Qt Installation Detector
echo ================================================
echo.

set QT_BASE=E:\Qt5.14

echo Searching for Qt installations in: %QT_BASE%
echo.

if exist "%QT_BASE%" (
    echo Found Qt base directory: %QT_BASE%
    echo.
    echo Available Qt versions:
    dir /b /ad "%QT_BASE%"
    echo.
    
    REM 检查常见的 Qt 5.14 版本
    if exist "%QT_BASE%\5.14.0" (
        echo Found Qt 5.14.0
        echo Available kits:
        dir /b /ad "%QT_BASE%\5.14.0"
    )
    
    if exist "%QT_BASE%\5.14.1" (
        echo Found Qt 5.14.1
        echo Available kits:
        dir /b /ad "%QT_BASE%\5.14.1"
    )
    
    if exist "%QT_BASE%\5.14.2" (
        echo Found Qt 5.14.2
        echo Available kits:
        dir /b /ad "%QT_BASE%\5.14.2"
    )
    
    echo.
    echo Common MSVC kits to look for:
    echo   - msvc2015_64
    echo   - msvc2017_64
    echo   - msvc2019_64
    echo   - mingw73_64
    echo.
    
    echo Please update build_cmake.bat with the correct path:
    echo   set QT_DIR=E:\Qt5.14\[version]\[kit]
    echo.
    echo Example:
    echo   set QT_DIR=E:\Qt5.14\5.14.2\msvc2017_64
    
) else (
    echo [ERROR] Qt directory not found: %QT_BASE%
    echo.
    echo Please check if Qt is installed in this location.
)

echo.
pause
