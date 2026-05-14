@echo off
REM Verbose build - all output streams to console (no log file redirect).
REM Use when scripts\build.bat appears to hang and you need to see what step
REM is actually running. Press Ctrl+C to abort.

setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0.."

echo === FsNext verbose build ===
echo.

REM -- Qt -----------------------------------------------------------
if "%QT_MSVC_ROOT%"=="" set "QT_MSVC_ROOT=C:\Qt\6.8.3\msvc2022_64"
if not exist "%QT_MSVC_ROOT%\bin\qmake.exe" (
    echo [FAIL] Qt not found at %QT_MSVC_ROOT%
    exit /b 1
)
echo [OK] Qt:    %QT_MSVC_ROOT%

REM -- MSVC ---------------------------------------------------------
if "%VSVARS%"=="" call :find_vsvars
if exist "%VSVARS%" goto :vs_ok
echo [FAIL] vcvars64 not found. Tried: !VSVARS!
echo [FAIL] Set VSVARS env var to your vcvars64.bat path.
exit /b 1
:vs_ok
echo [OK] VS:    !VSVARS!
echo.

echo --- Loading MSVC environment ---
call "!VSVARS!"
if errorlevel 1 (
    echo [FAIL] vcvars64 failed
    exit /b 1
)

set "PATH=%QT_MSVC_ROOT%\bin;%PATH%"

pushd "%ROOT%"

echo.
echo --- [1/2] cmake --preset msvc2022 ---
cmake --preset msvc2022
if errorlevel 1 (
    echo [FAIL] Configure failed
    popd
    exit /b 1
)

echo.
echo --- [2/2] cmake --build build --config Release -j 8 ---
cmake --build build --config Release -j 8 --verbose
if errorlevel 1 (
    echo [FAIL] Build failed
    popd
    exit /b 1
)

echo.
echo [PASS] Build successful
popd
exit /b 0

REM -- Helpers ------------------------------------------------------
:find_vsvars
call :_try_vswhere
if defined VSVARS if exist "%VSVARS%" exit /b 0

call :_probe "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files\Microsoft Visual Studio\2022\Community"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files\Microsoft Visual Studio\2022\Professional"
if defined VSVARS exit /b 0
call :_probe "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
if defined VSVARS exit /b 0

set "VSVARS=D:\Applications\VC\Auxiliary\Build\vcvars64.bat"
exit /b 0

:_try_vswhere
set "_VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%_VSWHERE%" exit /b 0
for /f "usebackq tokens=*" %%i in (`"%_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "_VSPATH=%%i"
if not defined _VSPATH exit /b 0
if not exist "%_VSPATH%\VC\Auxiliary\Build\vcvars64.bat" exit /b 0
set "VSVARS=%_VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
exit /b 0

:_probe
if not exist "%~1\VC\Auxiliary\Build\vcvars64.bat" exit /b 0
set "VSVARS=%~1\VC\Auxiliary\Build\vcvars64.bat"
exit /b 0
