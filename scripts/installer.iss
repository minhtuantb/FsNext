; ── FsNext Inno Setup script ────────────────────────────────────────────────
; Build with:
;     "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" scripts\installer.iss
; (Inno Setup 6.2+ required for IsNot, ArchitecturesInstallIn64BitMode native.)
;
; Prerequisites:
;   • cmake --preset msvc2022 && cmake --build build --config Release
;     → output\FsNext.exe + output\fsharenativeapp.exe + bundled Qt deps
;   • windeployqt6 has already populated output\ (POST_BUILD step in CMake)
;   • EV cert imported into local cert store under "Fshare Tool Code Signing"
;     (skip the [Code] sign step if no cert).
;
; Output:
;   dist\FsNext-Setup-{version}.exe
;
; Idempotency: re-running over an existing install upgrades in place via
; AppId GUID below.  Uninstaller cleans HKCU registry key for the Chrome
; native messaging host.

#define MyAppName       "Fshare Tool"
#define MyAppNameShort  "FsNext"
#define MyAppPublisher  "FPT"
#define MyAppURL        "https://www.fshare.vn"
#define MyAppExeName    "FsNext.exe"
#define MyHostExeName   "fsharenativeapp.exe"
; Version is read from the build's REPORT.md / git tag.  Bump per release.
#define MyAppVersion    "6.0.0.176"

[Setup]
; Stable AppId — DO NOT regenerate per release; the upgrade path keys off this.
AppId={{1D8E5C4A-3B6F-4D8E-A1F2-9B0C7E2D3F5A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/support
DefaultDirName={autopf}\{#MyAppNameShort}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName} {#MyAppVersion}
OutputDir=..\dist
OutputBaseFilename=FsNext-Setup-{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern
LicenseFile=..\LICENSE.txt
DisableProgramGroupPage=auto
ChangesAssociations=yes
DisableDirPage=auto
WizardImageFile=
WizardSmallImageFile=
; ── Branding ──────────────────────────────────────────────────────────────
; The installer .exe itself shows app.ico in Explorer and the wizard header.
; SetupIconFile picks the largest size from the multi-resolution .ico — we
; ship 16…256 from scripts/generate_app_icon.py.
SetupIconFile=..\resources\icons\app.ico

[Languages]
; Vietnamese is the source language.  Keep English as a fallback for
; international users running Windows in en-US.
Name: "vietnamese"; MessagesFile: "compiler:Languages\Default.isl"
Name: "english";   MessagesFile: "compiler:Languages\Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Tạo biểu tượng trên màn hình"; GroupDescription: "Lối tắt:"; Flags: unchecked
Name: "startupicon"; Description: "Khởi động cùng Windows"; GroupDescription: "Khởi động:"; Flags: unchecked

[Files]
; Main app + native host.  windeployqt6 has already copied Qt6*.dll, qml/, plugins/
; into output\ — we mirror that whole tree so the installed copy is self-contained.
Source: "..\output\{#MyAppExeName}";  DestDir: "{app}"; Flags: ignoreversion
Source: "..\output\{#MyHostExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\output\*.dll";            DestDir: "{app}"; Flags: ignoreversion
Source: "..\output\qml\*";            DestDir: "{app}\qml";          Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\output\platforms\*";      DestDir: "{app}\platforms";    Flags: ignoreversion recursesubdirs
Source: "..\output\plugins\*";        DestDir: "{app}\plugins";      Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "..\output\imageformats\*";   DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\output\iconengines\*";    DestDir: "{app}\iconengines";  Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\output\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\output\sqldrivers\*";     DestDir: "{app}\sqldrivers";   Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\output\tls\*";            DestDir: "{app}\tls";          Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\output\generic\*";        DestDir: "{app}\generic";      Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\output\translations\*";   DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist

; Chrome extension manifest template — used by NMH registration step in [Code].
Source: "..\src\platform\nativehost\manifest.json"; DestDir: "{app}"; DestName: "com.fshare.tool.template.json"; Flags: ignoreversion

; Standalone .ico for Start Menu / Desktop / Startup shortcuts.  The exe
; itself already embeds the icon via app.rc, but Inno's shortcuts can pick
; up a richer multi-size set from a dedicated .ico — useful for HiDPI.
Source: "..\resources\icons\app.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}";              Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"
Name: "{group}\Uninstall {#MyAppName}";    Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}";        Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"; Tasks: desktopicon
Name: "{userstartup}\{#MyAppName}";        Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"; Parameters: "/minimized"; Tasks: startupicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Khởi chạy {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Registry]
; Native messaging host registration — written per-user so install does NOT
; require admin.  Path is patched in by the [Code] PrepareToInstall handler so
; the manifest's `path` field carries the absolute install dir.
Root: HKCU; Subkey: "Software\Google\Chrome\NativeMessagingHosts\com.fshare.tool"; \
    ValueType: string; ValueName: ""; ValueData: "{app}\com.fshare.tool.json"; \
    Flags: uninsdeletekey

; Mirror entry for Microsoft Edge (same NMH protocol, separate registry root).
Root: HKCU; Subkey: "Software\Microsoft\Edge\NativeMessagingHosts\com.fshare.tool"; \
    ValueType: string; ValueName: ""; ValueData: "{app}\com.fshare.tool.json"; \
    Flags: uninsdeletekey

[UninstallRun]
; Stop any running FsNext.exe before file removal so we don't get "file in use".
; /F = force, /T = include child processes (e.g. fsharenativeapp.exe spawned by Chrome).
Filename: "{sys}\taskkill.exe"; Parameters: "/F /IM {#MyAppExeName} /T"; Flags: runhidden; RunOnceId: "KillFsNext"
Filename: "{sys}\taskkill.exe"; Parameters: "/F /IM {#MyHostExeName} /T"; Flags: runhidden; RunOnceId: "KillNativeHost"

[UninstallDelete]
; Remove the patched manifest we wrote in [Code].
Type: files; Name: "{app}\com.fshare.tool.json"
; Don't touch %APPDATA%\FPT\Fshare Tool — user data + history live there and
; we keep them so a reinstall picks up where the user left off.

[Code]

procedure WriteNativeMessagingManifest;
var
  TemplatePath, OutputPath, Manifest: string;
  AppDirEscaped: string;
begin
  TemplatePath := ExpandConstant('{app}\com.fshare.tool.template.json');
  OutputPath   := ExpandConstant('{app}\com.fshare.tool.json');

  if not LoadStringFromFile(TemplatePath, Manifest) then
  begin
    Log('[FsNext] Could not read NMH manifest template at ' + TemplatePath);
    exit;
  end;

  // Path field needs absolute, double-backslash-escaped JSON-safe form.
  AppDirEscaped := ExpandConstant('{app}\{#MyHostExeName}');
  StringChangeEx(AppDirEscaped, '\', '\\', True);

  // The shipped template is "fsharenativeapp.exe" (relative) — replace with absolute.
  StringChangeEx(Manifest, '"path": "fsharenativeapp.exe"',
                           '"path": "' + AppDirEscaped + '"', True);

  // The extension ID is unknown until the user installs the Chrome extension.
  // Leave the placeholder; the user / IT admin runs scripts\bind_extension_id.bat
  // after first install with the real ID.
  // (Documented in extension/README.md.)

  if not SaveStringToFile(OutputPath, Manifest, False) then
    Log('[FsNext] Could not write NMH manifest to ' + OutputPath);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    WriteNativeMessagingManifest;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  // Block Next on the welcome page if FsNext is currently running.  Killing
  // it from an installer is rude; ask the user to close it first.
  if CurPageID = wpWelcome then
  begin
    // Qt class name tracks the version: Qt6.8.3 → "Qt683QWindowIcon".  Keep
    // this in sync with the Qt version pinned in CMakePresets.json — bumping
    // Qt without updating this check makes the installer happily clobber a
    // running FsNext, locking files mid-update.
    if FindWindowByClassName('Qt683QWindowIcon') <> 0 then
    begin
      MsgBox('Vui lòng đóng Fshare Tool trước khi cập nhật.',
             mbInformation, MB_OK);
      Result := False;
    end;
  end;
end;
