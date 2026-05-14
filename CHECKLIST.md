# CHECKLIST — FsNext (Fshare Tool v6.0)

## Infrastructure
- [ ] Infrastructure > Project Setup > CMakeLists.txt root configuration
- [ ] Infrastructure > Project Setup > CMakePresets.json (MSVC + Ninja)
- [ ] Infrastructure > Project Setup > vcpkg integration (CURL, OpenSSL)
- [ ] Infrastructure > Project Setup > Qt 6 QML module configuration
- [ ] Infrastructure > Project Setup > Resource bundling (icons, images, translations)
- [ ] Infrastructure > Application > main.cpp entry point
- [ ] Infrastructure > Application > Single instance enforcement
- [ ] Infrastructure > Application > SSL/TLS initialization
- [ ] Infrastructure > Application > Proxy configuration
- [ ] Infrastructure > Application > AppContext (dependency injection)
- [ ] Infrastructure > Application > QML engine setup and type registration

## Core Layer — Models
- [ ] Core > Models > User model
- [ ] Core > Models > FileItem model (file + folder)
- [ ] Core > Models > TransferTask model (download + upload)
- [ ] Core > Models > TransferState enum (Queued, Active, Paused, Complete, Error)
- [ ] Core > Models > AppSettings model
- [ ] Core > Models > ApiResponse<T> wrapper
- [ ] Core > Models > AppError types

## Core Layer — HTTP Client
- [ ] Core > HttpClient > CURL wrapper (sync GET/POST)
- [ ] Core > HttpClient > Request builder (headers, body, auth)
- [ ] Core > HttpClient > Response parser (JSON → typed)
- [ ] Core > HttpClient > Error handling (network, timeout, parse)
- [ ] Core > HttpClient > Proxy support
- [ ] Core > HttpClient > SSL/CA configuration

## Core Layer — API Client
- [ ] Core > FshareApi > Login (POST /api/user/login)
- [ ] Core > FshareApi > OAuth login (POST /api/user/oauth)
- [ ] Core > FshareApi > Logout (GET /api/user/logout)
- [ ] Core > FshareApi > Get user info (GET /api/user/get)
- [ ] Core > FshareApi > List files in folder (POST /api/fileops/getFolderListPaging)
- [ ] Core > FshareApi > List folders (GET /api/fileops/list)
- [ ] Core > FshareApi > Get file info (POST /api/fileops/get)
- [ ] Core > FshareApi > Rename file (POST /api/fileops/rename)
- [ ] Core > FshareApi > Delete files (POST /api/fileops/delete)
- [ ] Core > FshareApi > Create folder (POST /api/fileops/createFolder)
- [ ] Core > FshareApi > Move files (POST /api/fileops/move)
- [ ] Core > FshareApi > Copy files (POST /api/fileops/copy)
- [ ] Core > FshareApi > Search files (POST /api/fileops/search)
- [ ] Core > FshareApi > Change secure status (POST /api/fileops/changeSecure)
- [ ] Core > FshareApi > Set file password (POST /api/fileops/createFilePass)
- [ ] Core > FshareApi > Set direct link (POST /api/share/SetDirectLink)
- [ ] Core > FshareApi > Create share link (POST /api/share/createsharelink)
- [ ] Core > FshareApi > Create download session (POST /api/session/download)
- [ ] Core > FshareApi > Create upload session (POST /api/session/upload)
- [ ] Core > FshareApi > Get latest version (GET /api/service/getlatestversion)

## Core Layer — Services
- [ ] Core > AuthService > Login with email/password
- [ ] Core > AuthService > OAuth login flow
- [ ] Core > AuthService > Logout
- [ ] Core > AuthService > Session management (token, cookies)
- [ ] Core > AuthService > Auto-login (saved credentials)
- [ ] Core > AuthService > User info fetch and cache
- [ ] Core > TransferService > Download queue management
- [ ] Core > TransferService > Upload queue management
- [ ] Core > TransferService > Start/pause/resume/cancel operations
- [ ] Core > TransferService > Priority management (move up/down)
- [ ] Core > TransferService > Concurrent thread limit enforcement
- [ ] Core > TransferService > Auto-start on add (configurable)
- [ ] Core > TransferService > Transfer history persistence (JSON)
- [ ] Core > TransferService > History load/save/clear
- [ ] Core > FileService > Load folder tree
- [ ] Core > FileService > List files in folder (paginated)
- [ ] Core > FileService > Rename file/folder
- [ ] Core > FileService > Delete files/folders
- [ ] Core > FileService > Move files/folders
- [ ] Core > FileService > Copy files/folders
- [ ] Core > FileService > Create folder
- [ ] Core > FileService > Search files
- [ ] Core > FileService > Change secure status
- [ ] Core > FileService > Set file password
- [ ] Core > FileService > Toggle direct link
- [ ] Core > FileService > Create share link
- [ ] Core > FileService > Get file info
- [ ] Core > SettingsService > Load/save all settings
- [ ] Core > SettingsService > Download settings (threads, segments, folder, auto-download)
- [ ] Core > SettingsService > Upload settings (threads, folder)
- [ ] Core > SettingsService > Connection settings (proxy mode, host, port)
- [ ] Core > SettingsService > General settings (language, auto-login, stay-on-top)
- [ ] Core > UpdateService > Check latest version
- [ ] Core > UpdateService > Force update detection

## Core Layer — Transfer Engine
- [ ] Core > Transfer > DownloadEngine > Single-segment CURL download
- [ ] Core > Transfer > DownloadEngine > Multi-segment parallel download
- [ ] Core > Transfer > DownloadEngine > Resume support (partial file)
- [ ] Core > Transfer > DownloadEngine > Progress callback and reporting
- [ ] Core > Transfer > DownloadEngine > Error handling and retry
- [ ] Core > Transfer > DownloadEngine > Password-protected file support
- [ ] Core > Transfer > UploadEngine > Chunked upload (20MB chunks)
- [ ] Core > Transfer > UploadEngine > Progress callback and reporting
- [ ] Core > Transfer > UploadEngine > Error handling and retry
- [ ] Core > Transfer > TransferWorker > QThread wrapper for engines
- [ ] Core > Transfer > TransferWorker > Pause/resume via signal (no WaitCondition)
- [ ] Core > Transfer > TransferQueue > FIFO queue with priority
- [ ] Core > Transfer > TransferQueue > Max concurrent slots enforcement
- [ ] Core > Transfer > SpeedMeter > Rolling window speed calculation
- [ ] Core > Transfer > SpeedMeter > ETA estimation
- [ ] Core > Transfer > SpeedMeter > Formatted display (KB/s, MB/s, time remaining)

## Core Layer — Repositories
- [ ] Core > Repositories > ApiRepository (wraps FshareApi with caching)
- [ ] Core > Repositories > SettingsRepository (wraps QSettings)
- [ ] Core > Repositories > HistoryRepository (JSON file read/write)

## ViewModel Layer
- [ ] ViewModels > AuthViewModel > Login state (email, password, loading, error)
- [ ] ViewModels > AuthViewModel > Remember me / auto-login
- [ ] ViewModels > AuthViewModel > OAuth triggers (Google, Facebook, FPT ID)
- [ ] ViewModels > AuthViewModel > Logout
- [ ] ViewModels > DownloadViewModel > Transfer list model (QAbstractListModel)
- [ ] ViewModels > DownloadViewModel > Add download (link, folder, password)
- [ ] ViewModels > DownloadViewModel > Start/pause/resume/cancel actions
- [ ] ViewModels > DownloadViewModel > Batch operations (pause all, resume all)
- [ ] ViewModels > DownloadViewModel > History list
- [ ] ViewModels > DownloadViewModel > Aggregate speed/progress
- [ ] ViewModels > UploadViewModel > Transfer list model
- [ ] ViewModels > UploadViewModel > Add upload (files, folder, properties)
- [ ] ViewModels > UploadViewModel > Start/pause/resume/cancel actions
- [ ] ViewModels > UploadViewModel > Batch operations
- [ ] ViewModels > UploadViewModel > History list
- [ ] ViewModels > FileManagerViewModel > Folder tree model
- [ ] ViewModels > FileManagerViewModel > File list model (paginated)
- [ ] ViewModels > FileManagerViewModel > Selection management (single, multi)
- [ ] ViewModels > FileManagerViewModel > Context menu actions
- [ ] ViewModels > FileManagerViewModel > Search
- [ ] ViewModels > UserInfoViewModel > Account info display
- [ ] ViewModels > UserInfoViewModel > Storage quota visualization
- [ ] ViewModels > UserInfoViewModel > Traffic usage
- [ ] ViewModels > SettingsViewModel > All settings categories
- [ ] ViewModels > SettingsViewModel > Apply/revert settings

## QML Theme & Components
- [ ] QML > Theme > FshareTheme.qml (all design tokens)
- [ ] QML > Theme > FshareColors.qml (color definitions)
- [ ] QML > Components > FsButton (5 variants, 3 sizes)
- [ ] QML > Components > FsCard (default + accent)
- [ ] QML > Components > FsTextField (label, error, disabled)
- [ ] QML > Components > FsProgressBar (5 statuses)
- [ ] QML > Components > FsNavigation (sidebar, nav items, user card)
- [ ] QML > Components > FsBadge (7 variants)
- [ ] QML > Components > FsToast (4 variants, auto-dismiss)
- [ ] QML > Components > FsDialog (enter/exit animations)
- [ ] QML > Components > FsTransferItem (file row with progress)
- [ ] QML > Components > FsFileRow (file manager row)
- [ ] QML > Components > FsFolderTree (tree view)
- [ ] QML > Components > FsEmptyState (illustration + message)
- [ ] QML > Components > FsLoadingState (spinner)
- [ ] QML > Components > FsErrorState (error + retry)
- [ ] QML > Components > FsSearchBar (search input)
- [ ] QML > Components > FsDropZone (drag & drop area)
- [ ] QML > Components > FsContextMenu (right-click menu)

## QML Pages
- [ ] QML > Pages > LoginPage > Email/password form
- [ ] QML > Pages > LoginPage > Remember me checkbox
- [ ] QML > Pages > LoginPage > OAuth buttons (Google, Facebook, FPT ID)
- [ ] QML > Pages > LoginPage > Error display
- [ ] QML > Pages > LoginPage > Loading state
- [ ] QML > Pages > LoginPage > Version check notification
- [ ] QML > Pages > DownloadPage > Active downloads list
- [ ] QML > Pages > DownloadPage > Download history list
- [ ] QML > Pages > DownloadPage > Add download button/dialog
- [ ] QML > Pages > DownloadPage > Toolbar (start, pause, remove, clear)
- [ ] QML > Pages > DownloadPage > Aggregate speed display
- [ ] QML > Pages > DownloadPage > Empty state
- [ ] QML > Pages > DownloadPage > Drag & drop support
- [ ] QML > Pages > UploadPage > Active uploads list
- [ ] QML > Pages > UploadPage > Upload history list
- [ ] QML > Pages > UploadPage > Add upload button/dialog
- [ ] QML > Pages > UploadPage > Toolbar
- [ ] QML > Pages > UploadPage > Empty state
- [ ] QML > Pages > FileManagerPage > Folder tree (left panel)
- [ ] QML > Pages > FileManagerPage > File list (right panel)
- [ ] QML > Pages > FileManagerPage > Toolbar (new folder, delete, move, copy, etc.)
- [ ] QML > Pages > FileManagerPage > Context menu (right-click)
- [ ] QML > Pages > FileManagerPage > File info panel
- [ ] QML > Pages > FileManagerPage > Search bar
- [ ] QML > Pages > FileManagerPage > Bulk selection
- [ ] QML > Pages > FileManagerPage > Empty/loading/error states
- [ ] QML > Pages > UserInfoPage > Account details card
- [ ] QML > Pages > UserInfoPage > Storage quota visualization
- [ ] QML > Pages > UserInfoPage > Traffic usage display
- [ ] QML > Pages > UserInfoPage > VIP status / expiry
- [ ] QML > Pages > SettingsPage > General settings section
- [ ] QML > Pages > SettingsPage > Connection settings section
- [ ] QML > Pages > SettingsPage > Download settings section
- [ ] QML > Pages > SettingsPage > Upload settings section
- [ ] QML > Pages > SettingsPage > About section

## QML Dialogs
- [ ] QML > Dialogs > AddDownloadDialog (link input, folder picker, password)
- [ ] QML > Dialogs > AddUploadDialog (file picker, folder picker, properties)
- [ ] QML > Dialogs > FileInfoDialog (file properties display)
- [ ] QML > Dialogs > FolderPickerDialog (folder tree selection)
- [ ] QML > Dialogs > PasswordDialog (password input prompt)
- [ ] QML > Dialogs > ConfirmDialog (generic confirm/cancel)
- [ ] QML > Dialogs > AboutDialog (version, copyright, links)

## Main Window & Navigation
- [ ] QML > Main > ApplicationWindow setup
- [ ] QML > Main > StackView navigation (login → main layout)
- [ ] QML > Main > Sidebar navigation (FsNavigation)
- [ ] QML > Main > Page routing (Download, Upload, Files, Info, Settings)
- [ ] QML > Main > Toast notification system
- [ ] QML > Main > Window chrome (title bar, min/max/close)
- [ ] QML > Main > Minimum window size (800×560)
- [ ] QML > Main > Dark mode toggle

## Platform Integration
- [ ] Platform > Single instance enforcement
- [ ] Platform > System tray (minimize, context menu, notifications)
- [ ] Platform > Chrome extension native messaging host
- [ ] Platform > Drag & drop (links → download, files → upload)
- [ ] Platform > Window always-on-top toggle
- [ ] Platform > Auto-start with OS (registry on Windows)

## Build & Deploy
- [ ] Build > CMake configuration compiles successfully
- [ ] Build > All dependencies resolved (Qt, CURL, OpenSSL, jsoncpp)
- [ ] Build > Application runs without crash for 30+ seconds
- [ ] Build > Release build produces single executable
- [ ] Build > Debug scripts functional (debug_run, check_deps, clear_cache)

## Integration Testing
- [ ] Integration > Login with valid credentials → success
- [ ] Integration > Login with invalid credentials → error display
- [ ] Integration > Download a file → progress → completion
- [ ] Integration > Upload a file → progress → completion
- [ ] Integration > Pause/resume download
- [ ] Integration > Browse folders and files
- [ ] Integration > Create/rename/delete folder
- [ ] Integration > Move/copy files
- [ ] Integration > Settings save and persist across restart
- [ ] Integration > Multiple downloads concurrent (thread limit)
- [ ] Integration > History persists across restart
- [ ] Integration > Window resize and minimum size
- [ ] Integration > Dark mode toggle
- [ ] Integration > System tray minimize and restore
