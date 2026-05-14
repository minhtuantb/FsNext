# Dependency Graph — Fshare Tool v5.3.0 (Legacy Reference)

> **Note**: This document describes the dependency graph of the **old** fsharetool (v5.3.0)
> codebase. FsNext has its own dependency structure — see `07_architecture.md` for the
> current architecture. Legacy modules (`fshareclient/`, `MegaSetting/`, etc.) are not
> used by FsNext.

## Build Dependency Tree

```
fsharetool.exe (main executable)
│
├── Qt 6.8+ (from Qt installer: C:/Qt/6.8.3/msvc2022_64)
│   ├── Qt6::Core          — Strings, collections, settings, threads, signals/slots
│   ├── Qt6::Gui           — Basic GUI types, images, fonts
│   ├── Qt6::Widgets       — QWidget, QDialog, QMainWindow, QTableView
│   ├── Qt6::Network       — QNetworkProxy, QSslCertificate
│   ├── Qt6::PrintSupport  — Print functionality (minimal usage)
│   ├── Qt6::WebEngineCore — Web page rendering engine
│   ├── Qt6::WebEngineWidgets — QWebEngineView, QWebEnginePage
│   ├── Qt6::Qml           — QML engine
│   ├── Qt6::Quick          — QML visual components
│   ├── Qt6::QuickControls2 — QML controls (Button, TextField, etc.)
│   └── Qt6::Svg           — SVG icon rendering
│
├── fshareclient (static lib, internal)
│   ├── CURL::libcurl (static, from vcpkg)
│   │   ├── OpenSSL::SSL (static, from vcpkg)
│   │   ├── OpenSSL::Crypto (static, from vcpkg)
│   │   └── zlib (static, from vcpkg)
│   ├── jsoncpp (compiled from source in lib/jsoncpp/)
│   ├── cppcodec (header-only, in lib/cppcodec/)
│   └── fshare_utils (static lib, internal: lib/utils/)
│       └── CURL::libcurl
│
├── MegaSetting (static lib, internal)
│   └── Qt6::Core
│
├── qtsingleapplication (static lib, internal: lib/qtsingleapplication/)
│   ├── Qt6::Core
│   └── Qt6::Network
│
└── System Libraries (Windows)
    ├── ws2_32.lib      — Winsock (CURL networking)
    ├── crypt32.lib     — Windows crypto API (SSL certificates)
    ├── wldap32.lib     — LDAP (CURL dependency)
    ├── normaliz.lib    — Unicode normalization
    ├── secur32.lib     — Security (SSPI)
    ├── advapi32.lib    — Registry, security tokens
    ├── user32.lib      — Window/UI APIs
    ├── bcrypt.lib      — Hashing (OpenSSL)
    ├── iphlpapi.lib    — Network interfaces (proxy detection)
    └── kernel32.lib    — Process management (IsWow64Process)
```

## Separate Executable

```
fsharenativeapp.exe (Chrome native messaging host)
├── Qt6::Core
└── System: registry access (advapi32)
```

## Runtime Dependency Flow

```
┌────────────────────────────────────────────────────────────┐
│                     User Interface                          │
│  MainForm, LoginForm, FormDownload, FormUpload, FormManage │
│  DialogOption, DialogUpload, DialogDownload, etc.          │
│  QML: App.qml, LoginPage.qml, Components/*                │
├────────────────┬───────────────────────────────────────────┤
│                │ uses (composition)                         │
│                ▼                                            │
│        ┌───────────────┐                                    │
│        │ ActionThread   │ ← dispatches all API operations   │
│        │ AuthController │ ← QML bridge for auth             │
│        └───────┬───────┘                                    │
│                │ calls                                      │
│                ▼                                            │
│  ┌─────────────────────────────┐                           │
│  │       fshareclient          │                           │
│  │  user_api  file_api         │                           │
│  │  sessionapi  client         │                           │
│  └─────────────┬───────────────┘                           │
│                │ HTTP calls                                │
│                ▼                                            │
│  ┌─────────────────────────────┐                           │
│  │       lib/utils             │                           │
│  │  http_util  curl_utils      │                           │
│  │  asyn_manage  HandleItem    │                           │
│  └─────────────┬───────────────┘                           │
│                │ uses                                      │
│                ▼                                            │
│  ┌─────────────────────────────┐                           │
│  │  libcurl + OpenSSL          │                           │
│  └─────────────┬───────────────┘                           │
│                │ HTTPS                                     │
│                ▼                                            │
│  ┌─────────────────────────────┐                           │
│  │  api.fshare.vn (REST API)   │                           │
│  │  flog.fshare.vn (logging)   │                           │
│  └─────────────────────────────┘                           │
│                                                            │
│  ┌────────────────────┐  ┌────────────────────┐           │
│  │ DownloadFile        │  │ UploadFile          │          │
│  │ (direct CURL usage) │  │ (direct CURL usage)  │          │
│  │ bypasses fshareclient│ │ bypasses fshareclient │          │
│  │ for actual transfer │  │ for actual transfer  │          │
│  └────────────────────┘  └────────────────────┘           │
│                                                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ MegaSetting   │  │ jsoncpp      │  │qtsingleapp   │    │
│  │ Data models   │  │ JSON parse   │  │ Single inst  │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
└────────────────────────────────────────────────────────────┘
```

## Data Layer Dependencies

```
MegaUser ← created from fshare::user data after login
MegaFile ← created from fshare::file data for folder tree / file operations

fshare::user ← parsed from JSON (getUserInfo response)
fshare::file ← parsed from JSON (file listing / file info responses)
fshare::session ← created during login (holds token, cookies)

QSettings ← stores user preferences, credentials, history paths
XML files ← stores download/upload history per user
```

## Module Coupling Analysis

### Tight Coupling (needs refactoring for FsNext)
- `global.h` externs → everyone depends on globals (client, analytics, megauser, logger)
- `MainForm` → directly creates/manages all forms (FormDownload, FormUpload, etc.)
- `ActionThread` → single class handles ALL API dispatch (login, file ops, etc.)
- `DownloadFile`/`UploadFile` → use CURL directly (bypassing fshareclient for transfers)

### Loose Coupling (good patterns to keep)
- `fshareclient` → clean API boundary, can be reused as-is
- `MegaSetting` → standalone data models, minimal Qt Core dependency
- `qtsingleapplication` → standalone utility, no app-specific code
- `lib/utils` → generic HTTP wrapper

## Build System Dependencies

```
CMake 3.24+ → configure
Ninja → build generator
MSVC 2022 → compiler (C++17)
vcpkg → package manager
  ├── curl[ssl] x64-windows-static-md
  ├── openssl x64-windows-static-md
  └── zlib x64-windows-static-md
Qt 6.8.3 → framework (msvc2022_64)
git → version generation (rev-list --count HEAD)
```
