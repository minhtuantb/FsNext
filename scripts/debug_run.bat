@echo off
REM ============================================
REM debug_run.bat — Run FsNext with full logging
REM Outputs: console log + debug log file
REM ============================================

setlocal

set "BUILD_DIR=%~dp0..\build"
set "EXE=%BUILD_DIR%\Release\fsharetool.exe"
set "LOG_FILE=%~dp0..\debug-run.log"

if not exist "%EXE%" (
    echo [FAIL] Executable not found: %EXE%
    echo        Run build first: cmake --build build --config Release
    exit /b 1
)

echo [INFO] Starting FsNext in debug mode...
echo [INFO] Log file: %LOG_FILE%
echo [INFO] Executable: %EXE%
echo.

REM Enable QML debugging and Qt logging
set QT_LOGGING_RULES=*.debug=true
set QML_IMPORT_TRACE=1
set QSG_INFO=1

"%EXE%" 2>&1 | tee "%LOG_FILE%"

set EXIT_CODE=%ERRORLEVEL%

if %EXIT_CODE% EQU 0 (
    echo.
    echo [PASS] Application exited normally (code 0)
) else (
    echo.
    echo [FAIL] Application exited with code %EXIT_CODE%
)

exit /b %EXIT_CODE%
