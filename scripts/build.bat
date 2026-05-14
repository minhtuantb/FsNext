@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0.."
set "LOGFILE=%ROOT%\build-log.txt"

set "VCVARSLOG=%ROOT%\vcvars-init.log"

echo [build] Starting...
> "%LOGFILE%" echo [build] Starting...
echo [build] Log file: %LOGFILE%

REM -- Environment --------------------------------------------------
if "%QT_MSVC_ROOT%"=="" set "QT_MSVC_ROOT=C:\Qt\6.8.3\msvc2022_64"
if not exist "%QT_MSVC_ROOT%\bin\qmake.exe" (
    echo [FAIL] Qt not found at %QT_MSVC_ROOT%
    echo [FAIL] Set QT_MSVC_ROOT or install Qt 6.8.3 msvc2022_64
    >> "%LOGFILE%" echo [FAIL] Qt not found at %QT_MSVC_ROOT%
    exit /b 1
)
echo [build] Qt OK: %QT_MSVC_ROOT%
>> "%LOGFILE%" echo [build] QT=%QT_MSVC_ROOT%

if "%VSVARS%"=="" call :find_vsvars
if exist "%VSVARS%" goto :vs_ok
echo [FAIL] vcvars64.bat not found.
echo [FAIL] Tried: !VSVARS!
echo [FAIL] Set VSVARS env var to your vcvars64.bat path.
exit /b 1
:vs_ok
echo [build] VS:    !VSVARS!
echo [build] Loading MSVC env...
REM Redirect vcvars64 to its own log file - it spawns helpers that may keep
REM the handle open and would block subsequent appends to %LOGFILE%.
call "!VSVARS!" > "%VCVARSLOG%" 2>&1
if errorlevel 1 (
    echo [FAIL] vcvars64 failed - see %VCVARSLOG%
    exit /b 1
)

set "PATH=%QT_MSVC_ROOT%\bin;%PATH%"
echo [build] Source=%ROOT%

REM -- Configure ----------------------------------------------------
echo [build] [1/2] Configuring CMake (preset: msvc2022)...
pushd "%ROOT%"
cmake --preset msvc2022 >> "%LOGFILE%" 2>&1
if errorlevel 1 (
    echo [FAIL] CMake configure failed - see %LOGFILE%
    popd
    exit /b 1
)
echo [build] Configure OK

REM -- Build --------------------------------------------------------
echo [build] [2/2] Building (Release, -j 8)... this may take several minutes
cmake --build build --config Release -j 8 >> "%LOGFILE%" 2>&1
if errorlevel 1 (
    echo [FAIL] Build failed - see %LOGFILE%
    popd
    exit /b 1
)

echo [PASS] Build successful!
>> "%LOGFILE%" echo [PASS] Build successful!
popd
exit /b 0

REM -- Helpers ------------------------------------------------------
:find_vsvars
REM Try vswhere first (installed with any VS 2017+)
call :_try_vswhere
if defined VSVARS if exist "%VSVARS%" exit /b 0

REM Probe common install paths one-by-one (avoids "(x86)" parser bug in nested blocks)
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

REM Last resort: legacy hardcoded path
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
