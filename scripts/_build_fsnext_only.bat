@echo off
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "QT_MSVC_ROOT=C:\Qt\6.8.3\msvc2022_64"
set "VSVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
call "%VSVARS%" > nul
if errorlevel 1 exit /b 1
set "PATH=%QT_MSVC_ROOT%\bin;%PATH%"
cmake --build "%ROOT%\build" --config Release --target FsNext -j 8
exit /b %errorlevel%
