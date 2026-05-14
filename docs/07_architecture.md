# Architecture Design — FsNext (Fshare Tool v6.0)

## 1. Architecture Overview

**Pattern**: MVVM (Model-View-ViewModel) with Clean Architecture layers
**UI**: Pure QML (no Qt Widgets)
**Business Logic**: C++17
**Bridge**: Q_PROPERTY / Q_INVOKABLE / Q_ENUM

```
┌──────────────────────────────────────────────────────────┐
│                    QML View Layer                         │
│  Pages, Components, Dialogs, Theme                       │
│  (Declarative UI, data binding, animations)              │
├──────────────────────────────────────────────────────────┤
│                  ViewModel Layer (C++)                    │
│  AuthViewModel, DownloadViewModel, UploadViewModel,      │
│  FileManagerViewModel, SettingsViewModel                 │
│  (Q_PROPERTY, Q_INVOKABLE, signals for UI)              │
├──────────────────────────────────────────────────────────┤
│                   Service Layer (C++)                     │
│  AuthService, TransferService, FileService,              │
│  SettingsService, UpdateService                          │
│  (Business logic, orchestration, error handling)         │
├──────────────────────────────────────────────────────────┤
│                  Repository Layer (C++)                   │
│  ApiRepository, SettingsRepository, HistoryRepository    │
│  (Data access abstraction)                               │
├──────────────────────────────────────────────────────────┤
│                   Data Layer (C++)                        │
│  FshareApiClient (HTTP), LocalStorage (QSettings/JSON),  │
│  TransferEngine (CURL workers)                           │
│  (Raw data operations)                                   │
└──────────────────────────────────────────────────────────┘
```

## 2. Directory Structure

```
FsNext/
├── docs/                           # Documentation (this directory)
├── scripts/                        # Build & debug scripts
├── src/                            # All source code
│   ├── main.cpp                    # Entry point
│   ├── app/                        # Application setup
│   │   ├── Application.h/.cpp      # App initialization, single instance
│   │   └── AppContext.h/.cpp       # Dependency injection container
│   │
│   ├── core/                       # Core domain (no Qt UI dependency)
│   │   ├── models/                 # Data models
│   │   │   ├── User.h              # User data
│   │   │   ├── FileItem.h          # File/folder data
│   │   │   ├── TransferTask.h      # Download/upload task
│   │   │   ├── TransferState.h     # Transfer state enum
│   │   │   └── AppSettings.h       # Settings model
│   │   │
│   │   ├── api/                    # API client
│   │   │   ├── FshareApi.h/.cpp    # REST API wrapper (replaces fshareclient)
│   │   │   ├── HttpClient.h/.cpp   # Modern HTTP client (CURL wrapper)
│   │   │   ├── ApiResponse.h       # Typed response wrapper
│   │   │   ├── ApiError.h          # Error types
│   │   │   └── OAuthProvider.h/.cpp # OAuth flow
│   │   │
│   │   ├── services/               # Business logic
│   │   │   ├── AuthService.h/.cpp  # Login, logout, session
│   │   │   ├── TransferService.h/.cpp # Download/upload orchestration
│   │   │   ├── FileService.h/.cpp  # File operations
│   │   │   ├── SettingsService.h/.cpp # Settings management
│   │   │   └── UpdateService.h/.cpp # Version check & update
│   │   │
│   │   ├── repositories/           # Data access
│   │   │   ├── ApiRepository.h/.cpp # API data access
│   │   │   ├── SettingsRepository.h/.cpp # QSettings wrapper
│   │   │   └── HistoryRepository.h/.cpp # Transfer history (JSON)
│   │   │
│   │   └── transfer/               # Transfer engine
│   │       ├── DownloadEngine.h/.cpp  # Multi-segment CURL downloader
│   │       ├── UploadEngine.h/.cpp    # Chunked CURL uploader
│   │       ├── TransferWorker.h/.cpp  # QThread worker
│   │       ├── TransferQueue.h/.cpp   # Queue management
│   │       └── SpeedMeter.h/.cpp      # Speed/ETA calculation
│   │
│   ├── viewmodels/                 # ViewModels (C++ ↔ QML bridge)
│   │   ├── AuthViewModel.h/.cpp
│   │   ├── DownloadViewModel.h/.cpp
│   │   ├── UploadViewModel.h/.cpp
│   │   ├── FileManagerViewModel.h/.cpp
│   │   ├── SettingsViewModel.h/.cpp
│   │   ├── UserInfoViewModel.h/.cpp
│   │   └── TransferListModel.h/.cpp   # QAbstractListModel
│   │
│   └── platform/                   # Platform-specific code
│       ├── SingleInstance.h/.cpp    # Single instance enforcement
│       ├── NativeMessaging.h/.cpp   # Chrome extension bridge
│       ├── SystemTray.h/.cpp        # System tray integration
│       └── PlatformUtils.h/.cpp     # OS-specific utilities
│
├── qml/                            # QML UI
│   ├── Main.qml                    # Root window
│   ├── Theme/                      # Design system tokens
│   │   ├── FshareTheme.qml
│   │   ├── FshareColors.qml
│   │   └── qmldir
│   ├── Components/                 # Reusable components
│   │   ├── FsButton.qml
│   │   ├── FsCard.qml
│   │   ├── FsTextField.qml
│   │   ├── FsProgressBar.qml
│   │   ├── FsNavigation.qml
│   │   ├── FsBadge.qml
│   │   ├── FsToast.qml
│   │   ├── FsDialog.qml
│   │   ├── FsTransferItem.qml
│   │   ├── FsFileRow.qml
│   │   ├── FsFolderTree.qml
│   │   ├── FsEmptyState.qml
│   │   ├── FsLoadingState.qml
│   │   ├── FsErrorState.qml
│   │   ├── FsSearchBar.qml
│   │   ├── FsDropZone.qml
│   │   ├── FsContextMenu.qml
│   │   └── qmldir
│   ├── Pages/                      # Full-screen pages
│   │   ├── LoginPage.qml
│   │   ├── DownloadPage.qml
│   │   ├── UploadPage.qml
│   │   ├── FileManagerPage.qml
│   │   ├── UserInfoPage.qml
│   │   ├── SettingsPage.qml
│   │   └── qmldir
│   └── Dialogs/                    # Modal dialogs
│       ├── AddDownloadDialog.qml
│       ├── AddUploadDialog.qml
│       ├── FileInfoDialog.qml
│       ├── FolderPickerDialog.qml
│       ├── PasswordDialog.qml
│       ├── ConfirmDialog.qml
│       ├── AboutDialog.qml
│       └── qmldir
│
├── resources/                      # Static resources
│   ├── icons/                      # Phosphor Icons (SVG)
│   ├── images/                     # App images
│   ├── translations/               # .ts/.qm files
│   └── resources.qrc
│
├── lib/                            # Third-party (vendored)
│   ├── jsoncpp/
│   └── cppcodec/
│
├── CMakeLists.txt                  # Root build file
├── CMakePresets.json               # Build presets
├── CHECKLIST.md
├── PLAN.md
├── STATUS.md
└── REPORT.md
```

## 3. Layer Responsibilities

### 3.1 View Layer (QML)
- Pure declarative UI
- Data binding to ViewModel properties
- Animations, transitions, user interactions
- No business logic — only display logic (show/hide, format)
- All styling via FshareTheme tokens

### 3.2 ViewModel Layer (C++)
- One ViewModel per page/feature
- Exposed to QML via `Q_PROPERTY`, `Q_INVOKABLE`, `Q_ENUM`
- Manages UI state (loading, error, data)
- Calls Service layer for business operations
- Transforms domain models to UI-friendly format
- Registered to QML engine in AppContext

### 3.3 Service Layer (C++)
- Business logic and orchestration
- Coordinates between repositories and transfer engine
- Error handling and retry logic
- Session management
- No QML dependency (pure C++ + Qt Core)

### 3.4 Repository Layer (C++)
- Data access abstraction
- API calls via FshareApi
- Local settings via QSettings
- History persistence via JSON files
- Encapsulates data source details

### 3.5 Data Layer (C++)
- Raw HTTP communication (HttpClient → CURL)
- CURL-based transfer engine (download/upload)
- Local file I/O
- No business logic

## 4. Key Architecture Decisions

### 4.1 No Global State
**Problem**: Current codebase uses `extern` globals (client, analytics, megauser).
**Solution**: Dependency injection via `AppContext`. All services and viewmodels receive dependencies through constructor.

```cpp
class AppContext {
public:
    // Singletons (created once, owned by AppContext)
    HttpClient* httpClient();
    FshareApi* api();
    AuthService* authService();
    TransferService* transferService();
    FileService* fileService();
    SettingsService* settingsService();
    
    // Register all viewmodels to QML engine
    void registerQmlTypes(QQmlEngine* engine);
};
```

### 4.2 Typed API Responses
**Problem**: Current code uses raw JSON parsing with manual error checking.
**Solution**: `ApiResponse<T>` template with explicit success/error states.

```cpp
template<typename T>
class ApiResponse {
public:
    bool isSuccess() const;
    T data() const;           // Only valid if isSuccess()
    ApiError error() const;   // Only valid if !isSuccess()
    int httpCode() const;
};
```

### 4.3 Modern Transfer Engine
**Problem**: Current DownloadFile/UploadFile QThread subclasses mix thread management with transfer logic.
**Solution**: Separate TransferWorker (thread) from DownloadEngine/UploadEngine (logic).

```
TransferQueue (manages slots, priority)
  └─ TransferWorker (QThread, owns engine instance)
       └─ DownloadEngine / UploadEngine (pure logic, CURL)
            └─ SpeedMeter (speed, ETA, progress)
```

### 4.4 QAbstractListModel for All Lists
**Problem**: Current code uses QAbstractTableModel (not QML-compatible).
**Solution**: TransferListModel extends QAbstractListModel with role-based data.

```cpp
class TransferListModel : public QAbstractListModel {
    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FileSizeRole,
        ProgressRole,
        SpeedRole,
        EtaRole,
        StatusRole,
        // ... etc
    };
    QHash<int, QByteArray> roleNames() const override;
};
```

### 4.5 History as JSON (not XML)
**Problem**: Current history uses XML, which is verbose and harder to parse.
**Solution**: JSON files for history persistence.

### 4.6 Secure Credential Storage
**Problem**: Current code stores password as base64 in QSettings.
**Solution**: Use OS keychain (Windows Credential Manager / macOS Keychain) via Qt's `QKeychain` or platform APIs.

### 4.7 Single QML Window
**Problem**: Current app has separate LoginForm dialog and MainForm window.
**Solution**: Single QML ApplicationWindow with StackView navigation.

```
Main.qml (ApplicationWindow)
  └─ StackView
       ├─ LoginPage (initial)
       └─ MainLayout (after login)
            ├─ FsNavigation (sidebar)
            └─ StackView (content)
                 ├─ DownloadPage
                 ├─ UploadPage
                 ├─ FileManagerPage
                 ├─ UserInfoPage
                 └─ SettingsPage
```

## 5. Tech Stack

| Component | Technology | Rationale |
|-----------|------------|-----------|
| Language | C++17 | Same as current, mature, performant |
| Build | CMake 3.24+ / Ninja | Same as current, proven |
| UI Framework | Qt 6.8+ QML | Full QML (no Widgets) |
| QML Controls | Qt Quick Controls 2 | Native QML controls |
| Design System | FshareTheme (custom) | Existing design tokens |
| HTTP | libcurl (static) | Same as current, proven |
| TLS | OpenSSL (static) | Same as current |
| JSON | jsoncpp | Same as current |
| Encoding | cppcodec | Same as current |
| Single Instance | Custom (named pipe/socket) | Simplified from qtsingleapplication |
| Package Manager | vcpkg | Same as current |
| Icons | Phosphor Icons (SVG) | Per design system |
| Analytics | TBD (evaluate modern alternatives) | Current GA may be deprecated |

## 6. Threading Model

```
Main Thread (Qt Event Loop)
  ├─ QML Engine + UI rendering
  ├─ ViewModel property updates
  └─ Signal/slot dispatching

Transfer Worker Pool (QThreadPool or fixed threads)
  ├─ Download workers (max N, configurable)
  ├─ Upload workers (max N, configurable)
  └─ Each worker: create CURL handle → execute → emit result

API Thread (single QThread or QtConcurrent)
  └─ All API calls run here, emit results via signals
     (prevents UI blocking for folder listings, file ops, etc.)
```

**Thread Safety Rules:**
- ViewModels: only accessed from main thread
- Services: thread-safe (mutex-protected state)
- Repositories: API calls on dedicated thread
- Transfer engines: each on own worker thread
- Signal/slot: automatic queued connections for cross-thread

## 7. Error Handling Strategy

```cpp
enum class ErrorCategory {
    Network,        // Connection timeout, DNS, proxy
    Auth,           // Token expired, wrong password
    Server,         // 500, 502, API error codes
    Storage,        // Disk full, permission denied
    Transfer,       // Download/upload failure
    Validation      // Invalid input
};

class AppError {
    ErrorCategory category;
    int code;
    QString message;         // User-friendly message
    QString technicalDetail; // For logs/debug
    bool isRetryable;        // Can user retry?
};
```

## 8. State Management

### Application State (SettingsService)
- Persisted in QSettings (same as current)
- Exposed to QML via SettingsViewModel

### Session State (AuthService)
- In-memory: token, user info, session ID
- Login state change triggers navigation

### Transfer State (TransferService)
- In-memory: active transfers, queue, progress
- History persisted as JSON after completion
- Exposed via TransferListModel (QAbstractListModel)

### File Browser State (FileService)
- In-memory: current folder, file list, folder tree
- No caching initially (same as current — fresh API call per navigation)
- Can add caching layer later if needed

## 9. Design System Integration

The existing design system (FshareTheme, FshareColors, all components) is copied into FsNext/qml/Theme/ and FsNext/qml/Components/ as-is. All new pages and dialogs must use these tokens and components exclusively.

**Rules enforced in code review:**
- No hardcoded colors, sizes, or spacing
- All text uses FshareTheme.fontFamily
- All interactive elements ≥ 36×36px touch target
- All states handled: loading, error, empty, data
- Reduce motion respected via FshareTheme.reduceMotion

## 10. Migration Strategy from Current Codebase

### What to Reuse (copy & adapt)
1. **API contracts** — same REST endpoints, same request/response formats
2. **CURL patterns** — multi-segment download, chunked upload logic
3. **SpeedMeter** — speed calculation algorithm
4. **Design system** — FshareTheme, FshareColors, all QML components
5. **OAuth config** — provider URLs, client IDs

### What to Rewrite
1. **UI** — all Qt Widgets → QML
2. **Architecture** — globals → DI, monolithic ActionThread → services
3. **Models** — QAbstractTableModel → QAbstractListModel
4. **History** — XML → JSON
5. **Error handling** — generic messages → typed errors with retry
6. **Threading** — QThread subclass per task → worker pool

### What to Drop
1. RSS auto-download (low usage, can add later if needed)
2. Subtitle search (low usage)
3. Video preview via QWebEngine (reduce binary size)
4. Google Analytics (evaluate modern alternative)
5. Firefox addon support (Chrome-only extension)
