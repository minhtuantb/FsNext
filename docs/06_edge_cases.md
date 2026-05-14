# Edge Cases & Special Handling — Fshare Tool v5.3.0

## 1. Authentication Edge Cases

### 1.1 Token Expiration
- **Current behavior**: Token stored in memory, no refresh mechanism
- **Edge case**: Long-running session where token expires mid-operation
- **Handling**: API returns error code → user must re-login
- **Impact**: Active downloads/uploads may fail if session expires

### 1.2 Concurrent Login
- **Edge case**: User logs in from another device/browser
- **Handling**: No explicit session invalidation detection
- **Risk**: Operations may silently fail with auth errors

### 1.3 OAuth Redirect Failure
- **Edge case**: QWebEngineView fails to capture redirect URL
- **Handling**: SocialLoginDialog timeout → return to login form
- **Risk**: Browser popup blocked, network timeout, provider down

### 1.4 Force Update Block
- **Current behavior**: If server returns version requiring forced update, app blocks usage
- **Edge case**: Update server unreachable → user cannot use app even with valid version
- **Handling**: Only blocks if version check succeeds AND version is too old

### 1.5 Remember Me Security
- **Current behavior**: Password stored as base64 in QSettings (registry on Windows)
- **Risk**: Base64 is encoding, not encryption — easily reversible
- **Impact**: Credentials exposed if system is compromised

---

## 2. Download Edge Cases

### 2.1 Segmented Download - Segment Failure
- **Edge case**: One of N segments fails mid-download
- **Handling**: Entire download marked as error, retry from scratch
- **Impact**: Large files may waste significant bandwidth on retry

### 2.2 Real Link Expiration
- **Edge case**: Download URL from /api/session/download expires during download
- **Handling**: CURL error → regenerate real link → retry
- **Max retries**: DEFAULT_DOWNLOAD_TRY = 10

### 2.3 Password-Protected File
- **Edge case**: User enters wrong password
- **Handling**: API returns FILE_PASS_ERROR → prompt for password again
- **Risk**: No retry limit on password attempts (API-side limit may exist)

### 2.4 Disk Space Exhaustion
- **Edge case**: Disk fills up during download
- **Current handling**: CURL write callback fails → error status
- **Risk**: Partial file left on disk, no automatic cleanup

### 2.5 File Name Conflicts
- **Edge case**: File with same name exists in download folder
- **Current handling**: Overwrite without prompt (some paths) or append number
- **Risk**: User data loss if overwriting

### 2.6 Network Interruption During Segmented Download
- **Edge case**: Network drops mid-transfer with multi-segment
- **Handling**: curl_multi_perform detects errors → retry mechanism
- **Risk**: Partial segments may leave corrupted file

### 2.7 VIP vs Free Account Download Limits
- **Edge case**: Free user attempts to use features requiring VIP
- **Handling**: Download speed limited server-side, fewer concurrent threads
- **Risk**: Slow downloads frustrate user, no clear UI indication

### 2.8 Very Large Files (>4GB)
- **Current handling**: Uses int64/qint64 for sizes, CURLOPT_RESUME_FROM_LARGE
- **Risk**: 32-bit overflow if wrong type used (some legacy code paths)

---

## 3. Upload Edge Cases

### 3.1 Chunk Upload Failure
- **Edge case**: One chunk fails during upload
- **Handling**: Retry individual chunk, up to DEFAULT_UPLOAD_TRY (100) retries
- **Risk**: Server may not support partial upload recovery for all storage backends

### 3.2 Quota Exceeded During Upload
- **Edge case**: User exceeds quota mid-upload (another device used space)
- **Handling**: Upload session creation fails → error status
- **Risk**: Previous chunks may be orphaned on server

### 3.3 File Modified During Upload
- **Edge case**: Source file changes while uploading
- **Current handling**: No file locking mechanism
- **Risk**: Corrupted upload with mismatched chunks

### 3.4 Upload to Non-Existent Folder
- **Edge case**: Target folder deleted by another client during upload setup
- **Handling**: Upload session creation fails → error

### 3.5 Zero-Byte File Upload
- **Edge case**: User uploads empty file
- **Handling**: Not explicitly handled — may cause division by zero in progress

---

## 4. File Management Edge Cases

### 4.1 Move/Copy to Same Folder
- **Edge case**: User moves file to its current folder
- **Handling**: API may return success (no-op) or error
- **Risk**: Confusing UI state

### 4.2 Recursive Folder Operations
- **Edge case**: Move folder into its own subfolder
- **Handling**: Server-side validation expected (circular reference detection)
- **Risk**: Undefined behavior if server doesn't validate

### 4.3 Concurrent File Operations
- **Edge case**: Multiple operations on same file (e.g., rename while moving)
- **Handling**: ActionThread runs one action at a time, but different threads possible
- **Risk**: Race condition between FormManage operations

### 4.4 Folder Tree Refresh Race
- **Edge case**: Multiple folder tree refresh requests
- **Current handling**: `mtx_callwstree` mutex prevents concurrent tree loads
- **Risk**: UI may show stale data during refresh

### 4.5 Special Characters in File Names
- **Edge case**: Files with unicode, emoji, or special chars in names
- **Handling**: Pass through to API as UTF-8
- **Risk**: Display issues, file system incompatibility on download

### 4.6 Pagination Edge Cases
- **Edge case**: Files added/deleted while paginating through list
- **Handling**: No cursor-based pagination — pages may shift
- **Risk**: Duplicate or missing files in listing

---

## 5. Network Edge Cases

### 5.1 Proxy Configuration
- **Edge case**: IE auto-detect proxy changes during app runtime
- **Handling**: Proxy set once at startup, not dynamically updated
- **Risk**: Network failures if proxy changes

### 5.2 SSL Certificate Issues
- **Edge case**: CA bundle outdated, corporate MITM proxy
- **Handling**: Custom CA path via set_ca_path()
- **Risk**: Connection failures, no user-friendly error

### 5.3 API 502 Bad Gateway
- **Edge case**: Server temporarily unavailable
- **Handling**: File listing has retry logic for 502 (up to 3 retries)
- **Risk**: Other operations (login, file ops) don't have 502 retry

### 5.4 Response Parsing Failure
- **Edge case**: API returns unexpected format (HTML error page instead of JSON)
- **Handling**: jsoncpp parse failure → error code 99
- **Risk**: Unhelpful error message to user

### 5.5 Timeout During Long Operations
- **Edge case**: Slow server response on large file listings
- **Handling**: async_post has 20-second timeout (CURLOPT_TIMEOUT)
- **Risk**: Operations may timeout for large folders (>10000 files)

---

## 6. System Integration Edge Cases

### 6.1 Multiple Fshare Tool Instances
- **Edge case**: User double-clicks app rapidly
- **Handling**: QtSingleApplication sends message to first instance
- **Risk**: Brief window where second instance starts before detecting first

### 6.2 Chrome Extension Not Connected
- **Edge case**: Extension installed but native host not registered
- **Handling**: Extension fails silently, link not sent to tool
- **Risk**: User confusion — "why doesn't it work?"

### 6.3 Registry Access Denied
- **Edge case**: User doesn't have registry write permissions
- **Handling**: QSettings may fail silently on Windows
- **Risk**: Settings not persisted between sessions

### 6.4 System Tray Not Available
- **Edge case**: Linux environments without system tray
- **Handling**: QSystemTrayIcon::isSystemTrayAvailable() check
- **Risk**: Cannot minimize to tray, lose background functionality

---

## 7. Thread Safety Edge Cases

### 7.1 OpenSSL Threading (Legacy)
- **Current handling**: Custom mutex callbacks for OpenSSL < 1.1
- **Risk**: If OpenSSL 3.x is used, legacy locking may conflict
- **Note**: OpenSSL 1.1+ has built-in thread safety

### 7.2 Signal/Slot Cross-Thread
- **Edge case**: UI update from worker thread
- **Handling**: Qt auto-detects cross-thread → queued connection
- **Risk**: If DirectConnection is forced, crash possible

### 7.3 CURL Handle Sharing
- **Edge case**: Multiple threads sharing CURL easy handle
- **Current handling**: Each thread creates own handle
- **Risk**: curl_global_init() must be called before any threads

---

## 8. Data Integrity Edge Cases

### 8.1 History File Corruption
- **Edge case**: App crashes while writing XML history
- **Handling**: No atomic write or backup mechanism
- **Risk**: History lost on crash

### 8.2 Incomplete Download Cleanup
- **Edge case**: Download cancelled/errored → partial file on disk
- **Handling**: Partial files left for potential resume
- **Risk**: User confusion about incomplete files

### 8.3 Settings Migration
- **Edge case**: Upgrading from older version with different settings format
- **Current handling**: QSettings keys assumed stable
- **Risk**: Settings corruption if keys change between versions
