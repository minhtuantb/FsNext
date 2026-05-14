@echo off
REM Register fsharenativeapp.exe as a Chrome native-messaging host (per-user).
REM Run AFTER the installer drops fsharenativeapp.exe into INSTALL_DIR.

setlocal
set HOST_NAME=com.fshare.tool
set INSTALL_DIR=%~dp0..\output
set MANIFEST_SRC=%~dp0..\src\platform\nativehost\manifest.json
set MANIFEST_DST=%INSTALL_DIR%\%HOST_NAME%.json
set EXTENSION_ID_FILE=%~dp0..\extension\.extension_id

if not exist "%INSTALL_DIR%\fsharenativeapp.exe" (
    echo [register_native_host] fsharenativeapp.exe not found in %INSTALL_DIR%
    exit /b 1
)

REM Read the extension ID written by Chrome at install time.  Falls back to
REM a placeholder so the manifest is at least syntactically valid for testing.
if exist "%EXTENSION_ID_FILE%" (
    for /f "delims=" %%I in (%EXTENSION_ID_FILE%) do set EXT_ID=%%I
) else (
    set EXT_ID=PLACEHOLDER_EXTENSION_ID
)

REM Build manifest with absolute path + extension origin substituted in.
powershell -NoProfile -Command ^
  "(Get-Content '%MANIFEST_SRC%') -replace '__EXTENSION_ID__', '%EXT_ID%' " ^
  "-replace '\"path\"\: \"fsharenativeapp.exe\"', '\"path\": \"%INSTALL_DIR:\=\\%\\fsharenativeapp.exe\"' " ^
  "| Set-Content '%MANIFEST_DST%'"

reg add "HKCU\Software\Google\Chrome\NativeMessagingHosts\%HOST_NAME%" ^
    /ve /t REG_SZ /d "%MANIFEST_DST%" /f >nul

echo [register_native_host] Registered %HOST_NAME% → %MANIFEST_DST%
endlocal
