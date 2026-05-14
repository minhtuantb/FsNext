@echo off
REM ============================================
REM clear_cache.bat — Clean build cache
REM Removes: build directory, CMake cache
REM ============================================

setlocal

set "PROJECT_DIR=%~dp0.."
set "BUILD_DIR=%PROJECT_DIR%\build"

echo ============================================
echo  FsNext Cache Cleanup
echo ============================================
echo.

REM --- Build directory ---
if exist "%BUILD_DIR%" (
    echo [INFO] Removing build directory: %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%"
    if %ERRORLEVEL% EQU 0 (
        echo [PASS] Build directory removed
    ) else (
        echo [FAIL] Could not remove build directory (files may be in use)
        exit /b 1
    )
) else (
    echo [INFO] Build directory does not exist (already clean)
)

REM --- CMake user presets ---
if exist "%PROJECT_DIR%\CMakeUserPresets.json" (
    echo [INFO] CMakeUserPresets.json found (not removing — user config)
)

REM --- Qt QML cache ---
set "QML_CACHE=%LOCALAPPDATA%\cache\qmlcache"
if exist "%QML_CACHE%" (
    echo [INFO] Clearing QML cache: %QML_CACHE%
    rmdir /s /q "%QML_CACHE%" 2>nul
    echo [PASS] QML cache cleared
) else (
    echo [INFO] No QML cache found
)

echo.
echo ============================================
echo  Result: CACHE CLEARED
echo ============================================

exit /b 0
