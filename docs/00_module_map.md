# Module Map — Fshare Tool v5.3.0 (Legacy Reference)

> **Note**: This document describes the architecture of the **old** fsharetool (v5.3.0)
> codebase. It is kept here as a reference for understanding the legacy system that
> FsNext replaces. FsNext does NOT depend on any of these modules at runtime or
> build time — all referenced paths (`megatool/`, `fshareclient/`, `MegaSetting/`,
> `integration/`) belong to the old codebase.

## High-Level Module Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    Fshare Tool (fsharetool)                   │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                  UI Layer (megatool/)                    │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐  │ │
│  │  │  Login    │ │ Download │ │  Upload  │ │   File    │  │ │
│  │  │  Form     │ │  Manager │ │  Manager │ │  Manager  │  │ │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └─────┬─────┘  │ │
│  │       │             │            │              │         │ │
│  │  ┌────┴─────┐ ┌────┴─────┐ ┌────┴─────┐ ┌─────┴─────┐  │ │
│  │  │  User    │ │  RSS     │ │ Settings │ │  Info     │  │ │
│  │  │  Info    │ │  Manager │ │  Dialog  │ │  Panel    │  │ │
│  │  └──────────┘ └──────────┘ └──────────┘ └───────────┘  │ │
│  └─────────────────────┬───────────────────────────────────┘ │
│                        │ signals/slots                        │
│  ┌─────────────────────┴───────────────────────────────────┐ │
│  │              Controller Layer (megatool/)                │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │ │
│  │  │ ActionThread  │  │ AuthController│  │ OAuthHelper  │  │ │
│  │  │ (API dispatch)│  │ (QML bridge) │  │ (OAuth flow) │  │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │ │
│  └─────────┼─────────────────┼─────────────────┼──────────┘ │
│            │                 │                  │             │
│  ┌─────────┴─────────────────┴──────────────────┴──────────┐ │
│  │             Worker Thread Layer (megatool/)              │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │ │
│  │  │ DownloadFile  │  │  UploadFile  │  │ FshareLogger │  │ │
│  │  │ (QThread)     │  │  (QThread)   │  │ (QThread)    │  │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────────────┘  │ │
│  └─────────┼─────────────────┼─────────────────────────────┘ │
│            │                 │                                │
│  ┌─────────┴─────────────────┴─────────────────────────────┐ │
│  │            API Client (fshareclient/)                    │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐  │ │
│  │  │ user_api │ │ file_api │ │sessionapi│ │  client   │  │ │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └─────┬─────┘  │ │
│  └───────┼─────────────┼────────────┼─────────────┼────────┘ │
│          │             │            │              │          │
│  ┌───────┴─────────────┴────────────┴─────────────┴────────┐ │
│  │             HTTP Layer (lib/utils/)                      │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐                │ │
│  │  │ http_util│ │curl_utils│ │asyn_manage│               │ │
│  │  └────┬─────┘ └──────────┘ └───────────┘               │ │
│  └───────┼─────────────────────────────────────────────────┘ │
│          │ libcurl + OpenSSL                                 │
│  ┌───────┴─────────────────────────────────────────────────┐ │
│  │            External: api.fshare.vn (REST API)           │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌───────────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ MegaSetting (lib) │  │ jsoncpp (lib)│  │qtsingleapp   │  │
│  │ Data models       │  │ JSON parsing │  │Single instance│  │
│  └───────────────────┘  └──────────────┘  └──────────────┘  │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  Integration: Chrome Extension (Native Messaging Host)  │ │
│  └─────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

## Module Inventory

### 1. megatool/ — Main Application (Desktop GUI)
**Role**: User interface, business logic orchestration, download/upload workers
**Files**: ~94 .cpp/.h, 23 .ui, 17 .qml
**Key classes**: MainForm, LoginForm, FormDownload, FormUpload, FormManage, FormInfo, ActionThread, DownloadFile, UploadFile, AuthController

### 2. fshareclient/ — API Client Library
**Role**: REST API wrapper for fshare.vn, HTTP communication, auth/session
**Files**: ~16 .cpp/.h (fshare/ + utils/ subdirs)
**Key classes**: fshare::client, fshare::user_api, fshare::file_api, fshare::sessionapi, utils::http_util

### 3. MegaSetting/ — Data Models & Utilities
**Role**: Shared data structures (MegaUser, MegaFile), utility functions
**Files**: megasetting.h/.cpp
**Key classes**: MegaUser, MegaFile, Util, UploadHandler<T>, DownloadHandler<T>

### 4. lib/utils/ — CURL Wrapper
**Role**: CURL initialization, proxy management, SSL/CA config
**Files**: curl_utils.h/.cpp
**Key functions**: set_proxy_cb(), set_ca_path(), _curl_easy_init(), sslctx_function()

### 5. lib/jsoncpp/ — JSON Library
**Role**: JSON parsing for API responses
**Files**: jsoncpp.cpp (single-file, 155KB)

### 6. lib/qtsingleapplication/ — Single Instance
**Role**: Prevent multiple app instances, IPC for link handling
**Files**: ~8 .cpp/.h
**Key classes**: QtSingleApplication, QtLocalPeer

### 7. lib/cppcodec/ — Base64 Encoding
**Role**: Credential/token encoding
**Files**: Header-only library

### 8. integration/chrome_extension/ — Browser Extension Host
**Role**: Native messaging bridge Chrome ↔ Fshare Tool
**Files**: fshare_native_app.cpp
**Output**: fsharenativeapp.exe

### 9. lib/FluentUI/ — QML Component Library
**Role**: Modern Fluent Design QML components (not yet integrated)
**Status**: Vendored but disabled in CMake

## Inter-Module Dependencies

```
fsharetool (executable)
├── megatool/           → fshareclient, MegaSetting, qtsingleapplication, fshare_utils
│                         Qt6::Core/Gui/Widgets/Network/WebEngine/Qml/Quick
├── fshareclient/       → fshare_utils (curl_utils), jsoncpp, cppcodec
│                         CURL::libcurl, OpenSSL, system libs
├── MegaSetting/        → Qt6::Core
├── fshare_utils/       → CURL::libcurl
├── qtsingleapplication → Qt6::Core/Network
└── chromehost/         → Qt6::Core (standalone exe)
```

## Data Flow Summary

```
User Action (UI) 
  → Signal/Slot → Controller (ActionThread / AuthController)
    → fshare::*_api method call
      → utils::http_util::post/get (CURL)
        → HTTPS → api.fshare.vn
          → JSON response
        ← parse JSON (jsoncpp)
      ← http_response
    ← emit success/error signal
  ← UI update (model dataChanged / property notify)
```
