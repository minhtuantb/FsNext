@echo off
setlocal EnableExtensions

if "%VSVARS%"=="" call :find_vsvars
call "%VSVARS%" >nul 2>&1
set "PATH=C:\Qt\6.8.3\msvc2022_64\bin;%PATH%"
set "ROOT=%~dp0.."
cd /d "%ROOT%"
cmake --preset msvc2022 > "%ROOT%\configure-log.txt" 2>&1
echo EXIT_CODE=%ERRORLEVEL% >> "%ROOT%\configure-log.txt"
exit /b %ERRORLEVEL%

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
