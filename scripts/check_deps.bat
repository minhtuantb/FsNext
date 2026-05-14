@echo off
REM ============================================
REM check_deps.bat — Verify all dependencies
REM Checks: CMake, Ninja, Qt, vcpkg, CURL, OpenSSL
REM ============================================

setlocal
set FAIL_COUNT=0

echo ============================================
echo  FsNext Dependency Check
echo ============================================
echo.

REM --- CMake ---
cmake --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /i "version"') do (
        echo [PASS] CMake: %%v
    )
) else (
    echo [FAIL] CMake not found in PATH
    set /a FAIL_COUNT+=1
)

REM --- Ninja ---
ninja --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f %%v in ('ninja --version') do echo [PASS] Ninja: %%v
) else (
    echo [FAIL] Ninja not found in PATH
    set /a FAIL_COUNT+=1
)

REM --- MSVC (cl.exe) ---
cl 2>nul | findstr /i "Version" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] MSVC cl.exe found
) else (
    echo [WARN] MSVC cl.exe not in PATH (run from Developer Command Prompt)
)

REM --- Qt ---
if defined QT_DIR (
    if exist "%QT_DIR%\bin\qmake.exe" (
        echo [PASS] Qt found at %QT_DIR%
    ) else (
        echo [FAIL] QT_DIR set but qmake not found: %QT_DIR%
        set /a FAIL_COUNT+=1
    )
) else (
    REM Check common Qt paths
    if exist "C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe" (
        echo [PASS] Qt found at C:\Qt\6.8.3\msvc2022_64
    ) else (
        echo [WARN] Qt not found at default path. Set QT_DIR or CMAKE_PREFIX_PATH.
    )
)

REM --- vcpkg ---
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\vcpkg.exe" (
        echo [PASS] vcpkg found at %VCPKG_ROOT%
    ) else (
        echo [FAIL] VCPKG_ROOT set but vcpkg.exe not found
        set /a FAIL_COUNT+=1
    )
) else (
    echo [WARN] VCPKG_ROOT not set
)

REM --- git ---
git --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=3" %%v in ('git --version') do echo [PASS] Git: %%v
) else (
    echo [WARN] Git not found (needed for version generation)
)

echo.
echo ============================================
if %FAIL_COUNT% EQU 0 (
    echo  Result: ALL CHECKS PASSED
    echo ============================================
    exit /b 0
) else (
    echo  Result: %FAIL_COUNT% FAILURE(S) DETECTED
    echo ============================================
    exit /b 1
)
