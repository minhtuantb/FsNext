# Feature Inventory — Fshare Tool v5.3.0

## Module 1: Authentication & Authorization

### 1.1 Email/Password Login
- **Login form**: email + password input with validation
- **Remember me**: save credentials to QSettings (encrypted password via cppcodec base64)
- **Auto-login**: automatic login on app startup if credentials saved
- **Error handling**: display error codes/messages from API (wrong password, account locked, etc.)
- **Session management**: store token + session_id + cookies after login

### 1.2 OAuth / Social Login
- **Google OAuth**: OAuth2 code flow → token exchange → email extraction
- **Facebook OAuth**: OAuth2 code flow → token exchange → email extraction
- **FPT ID OAuth**: OAuth2 code flow → token exchange → email extraction
- **Web session login**: login via web session ID/token (for browser extension)
- **OAuth config**: client_id, client_secret, tokenUrl, userinfoUrl, scope per provider (oauthconfig.h)
- **SocialLoginDialog**: QWebEngineView captures redirect URL → extract auth code

### 1.3 Session Management
- **Token persistence**: session token stored in memory during app lifetime
- **Cookie management**: PHPSESSID cookie + app_key sent with every request
- **Logout**: API call to /api/user/logout, clear session state

### 1.4 User Info
- **Fetch user profile**: getUserInfo() → id, email, name, level, traffic, webspace
- **VIP status**: level 3 = VIP (more threads, faster speeds, more features)
- **Storage quota**: total/used webspace, secure webspace
- **Traffic tracking**: bandwidth usage display
- **Account expiry**: VIP expiration date display

---

## Module 2: Download Management

### 2.1 Download Queue
- **Add downloads**: paste Fshare links (file/ or folder/ URLs)
- **Queue management**: FIFO with configurable max concurrent threads (default 2)
- **Start/Pause/Resume/Stop**: per-file and batch operations
- **Priority management**: move up/down/first/last in queue
- **Auto-download**: optionally start download immediately on add
- **Drag & drop**: drop links onto main window to add downloads

### 2.2 Download Engine
- **Multi-segment download**: split file into N segments (default 4), download in parallel via CURL multi
- **Single-segment fallback**: for files < 4MB
- **Resume support**: CURLOPT_RESUME_FROM_LARGE for interrupted downloads
- **Real-link generation**: API call to /api/session/download to get actual download URL
- **Password-protected files**: prompt user for password, send to API
- **ZIP download**: option to download as ZIP archive
- **Speed metering**: rolling window speed calculation (SpeedMeter), ETA estimation
- **Progress tracking**: per-segment and aggregate progress percentage
- **Error handling**: retry up to 10 times (DEFAULT_DOWNLOAD_TRY), real-link regeneration on failure
- **Pause/Resume via WaitCondition**: graceful thread suspension without killing

### 2.3 Download History
- **Persist completed downloads**: XML file per user (downloadhistory_{uid}.xml)
- **Display history**: separate table for completed downloads
- **Clear history**: user can clear individual or all history items

### 2.4 Download Settings
- **Max concurrent downloads**: configurable (1-10 threads)
- **Segments per file**: configurable (1-16 segments)
- **Download folder**: default save location
- **Auto-download**: toggle auto-start on link add
- **Speed limit**: CURLOPT_MAX_RECV_SPEED_LARGE (not exposed in current UI)

### 2.5 Video Preview
- **Stream video**: play video from Fshare link via QWebEngineView
- **Detect media player**: find VLC/MPC/KMP on system
- **Launch external player**: open with detected media player + optional subtitle

---

## Module 3: Upload Management

### 3.1 Upload Queue
- **Select files**: file picker dialog
- **Select destination folder**: Fshare folder browser
- **File properties**: description, password, secure flag, direct-link flag
- **Queue management**: FIFO with configurable max concurrent threads (default 2)
- **Start/Pause/Resume/Stop**: per-file and batch operations

### 3.2 Upload Engine
- **Chunked upload**: 20MB default chunk size via CURL
- **Upload session**: API call to /api/session/upload to get upload URL
- **FTP support**: alternative upload protocol
- **Speed metering**: rolling window speed, ETA
- **Progress tracking**: per-chunk and aggregate progress
- **Error handling**: retry up to 100 times (DEFAULT_UPLOAD_TRY)
- **Pause/Resume**: WaitCondition-based thread suspension
- **Quota check**: verify user has sufficient webspace before upload

### 3.3 Upload History
- **Persist completed uploads**: XML file per user (uploadhistory_{uid}.xml)
- **Display history**: separate table
- **Clear history**: individual or batch

### 3.4 Upload Settings
- **Max concurrent uploads**: configurable threads
- **Default folder**: upload destination folder

---

## Module 4: File Management

### 4.1 Folder Browser
- **Folder tree**: hierarchical tree view of user's Fshare folders
- **Lazy loading**: load folder contents on expand/click
- **Refresh**: manual refresh of folder tree
- **Create folder**: create new folder in current location

### 4.2 File Listing
- **Table view**: files in selected folder (name, size, date, status)
- **Pagination**: load files in pages (50 per page max from API, 2000 limit)
- **Sorting**: by column (client-side)
- **Selection**: single and multi-select for bulk operations

### 4.3 File Operations
- **Rename**: rename file/folder → POST /api/fileops/rename
- **Delete**: delete file(s)/folder(s) → POST /api/fileops/delete
- **Move**: move to another folder → POST /api/fileops/move
- **Copy**: copy to another folder → POST /api/fileops/copy
- **Create folder**: → POST /api/fileops/createFolder

### 4.4 File Security
- **Secure/Unsecure**: toggle secure status → POST /api/fileops/changeSecure
- **Set password**: protect file with password → POST /api/fileops/createFilePass
- **Change password**: update existing password
- **Direct link**: toggle direct download link → POST /api/share/SetDirectLink
- **Share link**: generate share links for email recipients → POST /api/share/createsharelink

### 4.5 File Info
- **View properties**: size, access count, shared date, folder path, hash
- **Edit filename**: rename from properties dialog
- **Download count**: display how many times file was downloaded

---

## Module 5: User Info & Statistics

### 5.1 Account Display
- **User profile**: name, email, ID, account level
- **VIP status**: level indicator, expiration date
- **Points**: loyalty/reward points

### 5.2 Storage Visualization
- **Pie chart**: visual representation of used/free space
- **Regular webspace**: total, used, available
- **Secure webspace**: total, used, available
- **Traffic**: bandwidth quota used/remaining

---

## Module 6: Settings & Configuration

### 6.1 General Settings
- **Language**: Vietnamese / English (Qt Linguist .qm files)
- **Auto-login**: toggle automatic login at startup
- **Stay on top**: window always-on-top mode
- **Auto-start**: launch with Windows (registry entry)

### 6.2 Connection Settings
- **Proxy mode**: None / IE auto-detect / Manual
- **Manual proxy**: host + port configuration
- **Applied to**: both Qt (QNetworkProxy) and CURL (proxy callback)

### 6.3 Download Settings
- **Thread count**: max concurrent downloads
- **Segment count**: segments per file download
- **Auto-download**: start immediately on add
- **Default folder**: download save location

### 6.4 Upload Settings
- **Thread count**: max concurrent uploads
- **Default folder**: destination folder on Fshare

---

## Module 7: RSS Auto-Download

### 7.1 Feed Management
- **RSS sources**: configured RSS feed URLs
- **Category parsing**: extract film categories from feed
- **Episode detection**: identify new episodes

### 7.2 Auto-Download
- **Filter rules**: match episodes by category/name
- **Download folder**: per-category download location
- **Automatic**: queue matching episodes for download
- **History**: track already-downloaded episodes

---

## Module 8: Subtitle Search

### 8.1 Online Search
- **Search by keyword**: search subtitle databases online
- **Results display**: list matching subtitles
- **Download subtitle**: fetch and save .srt/.sub files

### 8.2 Local Management
- **Set save folder**: configure subtitle download location
- **Associate with video**: launch player with video + subtitle

---

## Module 9: System Integration

### 9.1 Chrome Extension
- **Native messaging host**: fsharenativeapp.exe
- **Link capture**: Chrome extension sends Fshare links to tool
- **Protocol**: 4-byte length prefix + JSON payload
- **Registry**: HKEY_CURRENT_USER\SOFTWARE\Fshare\Fshare Tool

### 9.2 Single Instance
- **QtSingleApplication**: prevent multiple instances
- **IPC**: route messages (links) from new instance to running one

### 9.3 System Tray
- **Minimize to tray**: continue running in background
- **Tray icon**: show/hide, context menu
- **Notifications**: download complete, errors

### 9.4 Drag & Drop
- **Drop links**: drag Fshare URLs onto window to add downloads
- **Drop files**: drag files to upload queue

### 9.5 Update Checker
- **Version check**: GET /api/service/getlatestversion
- **Force update**: block app if version too old
- **Download update**: fetch and install new version

---

## Module 10: Analytics & Logging

### 10.1 Google Analytics
- **Screen views**: track page/screen navigation
- **Events**: track user actions (login, download start, etc.)
- **User agent**: platform + version identification

### 10.2 Fshare Logger
- **Speed logging**: report download speeds to flog.fshare.vn
- **File count**: report active download/upload counts
- **Background thread**: batch sending to avoid UI blocking
