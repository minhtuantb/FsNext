@echo off
REM ── Bind installed Chrome extension ID into the NMH manifest ────────────
REM Run AFTER the user installs the FsNext Chrome extension.  Pass the
REM extension's 32-char ID (visible at chrome://extensions in Developer mode)
REM as the only argument:
REM
REM   scripts\bind_extension_id.bat aabbccddeeffgghhiijjkkllmmnnoopp
REM
REM This patches the installed `com.fshare.tool.json` so Chrome will allow
REM messages from your specific extension origin.

setlocal
if "%~1"=="" (
    echo Usage: %~nx0 ^<extension_id^>
    echo   extension_id: 32-char Chrome extension ID from chrome://extensions
    exit /b 1
)
set EXT_ID=%~1
set INSTALL_DIR=%~dp0..\output
if not exist "%INSTALL_DIR%\com.fshare.tool.json" (
    REM Try the installed location too.
    set INSTALL_DIR=%LocalAppData%\Programs\FsNext
)
set MANIFEST=%INSTALL_DIR%\com.fshare.tool.json
if not exist "%MANIFEST%" (
    echo [bind_extension_id] Manifest not found at %MANIFEST%
    echo                     Install FsNext first.
    exit /b 1
)

powershell -NoProfile -Command ^
  "(Get-Content '%MANIFEST%') -replace '__EXTENSION_ID__', '%EXT_ID%' | Set-Content '%MANIFEST%'"

echo [bind_extension_id] Bound extension %EXT_ID% to %MANIFEST%
endlocal
