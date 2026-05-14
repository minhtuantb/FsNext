# Data Models — Fshare Tool v5.3.0 (Legacy Reference)

> **Note**: This document describes the data models of the **old** fsharetool (v5.3.0)
> codebase. It is kept here as a reference for understanding legacy API contracts and
> data structures. FsNext has its own models in `src/core/models/`.

## 1. API-Level Models (fshareclient/)

### 1.1 fshare::user
User account information from API.
```
id              : string    — Unique user ID
email           : string    — User email
name            : string    — Display name
level           : int       — Account level (2=Member, 3=VIP)
expire_vip      : string    — VIP expiration timestamp
joindate        : string    — Account creation date
totalpoints     : int       — Loyalty points
traffic         : int64     — Total traffic quota (bytes)
traffic_used    : int64     — Traffic used (bytes)
webspace        : int64     — Total storage quota (bytes)
webspace_used   : int64     — Storage used (bytes)
webspace_secure : int64     — Secure storage quota (bytes)
webspace_secure_used : int64 — Secure storage used (bytes)
amount          : int       — Account balance
dl_time_avail   : int       — Available download times
account_type    : string    — Account type label
```

### 1.2 fshare::file
File/folder metadata from API.
```
id              : uint64    — File ID
linkcode        : string    — Unique link code (used in URLs)
name            : string    — File/folder name
type            : string    — "file" or "folder"
size            : int64     — File size in bytes
path            : string    — Full path on Fshare
secure          : bool      — Secure storage flag
is_public       : bool      — Public access flag
copied          : bool      — Has been copied
shared          : bool      — Has been shared
directlink      : bool      — Direct download enabled
hash_index      : string    — File hash
owner_id        : string    — Owner user ID
parent_id       : string    — Parent folder ID
download_count  : int       — Total download count
created         : string    — Creation timestamp
modified        : string    — Last modified timestamp
password        : string    — File password (if set)
```

### 1.3 fshare::photo
Photo metadata from gallery API.
```
id              : string    — Photo ID
name            : string    — Photo filename
path            : string    — Photo path
created         : string    — Upload date
small_image_url : string    — Thumbnail URL
large_image_url : string    — Full-size URL
```

### 1.4 fshare::session
Active API session state.
```
_tokenkey       : string    — Session token (from login response)
_session_id     : string    — Session ID (from login response)
_fuser          : user*     — User object pointer
_httpReq        : httpReqData* — Request headers (Cookie + Content-type)
```

### 1.5 fshare::client
API client configuration.
```
_appkey         : string    — Application API key
_session        : session*  — Current session
_cmdPost        : http_util* — HTTP utility instance
_proxy          : string    — Proxy host (if configured)
_port           : string    — Proxy port (if configured)
_capath         : string    — CA certificate path
```

---

## 2. Application-Level Models (MegaSetting/)

### 2.1 MegaUser
Represents authenticated user in the application layer.
```
userId          : QString   — User ID
userEmail       : QString   — User email
sessionId       : QString   — Session ID
password        : QString   — Stored password (encrypted)
level           : int       — Account level (2=Member, 3=VIP)
available       : qint64    — Available storage (bytes)
maxUpload       : qint64    — Max upload size allowed
secured         : qint64    — Secure storage used
traffic         : qint64    — Traffic used
```

### 2.2 MegaFile
Represents a file/folder for UI display and operations.
```
fileId          : QString   — File ID
linkcode        : QString   — Link code
name            : QString   — Display name
size            : qint64    — File size
desc            : QString   — Description
parentId        : QString   — Parent folder ID
fileDownload    : QString   — Download count
fileCreated     : QString   — Creation date
fileStatus      : int       — Status flags
secured         : bool      — Secure flag
filePassword    : QString   — Password
directLink      : bool      — Direct link flag
```
**Static methods:**
- `getListFolders(xml, keyFlag)` → `QMap<QString, MegaFile>` — Parse folder tree XML
- `getElementData(xml)` → extract XML node text
- `parseMegaFile(xml)` → single file parser

---

## 3. Download/Upload Models (megatool/)

### 3.1 DownloadFile (QThread)
Active download task state.
```
_filename       : QString   — File name
_filesize       : qint64    — Total file size
_linkcode       : QString   — Fshare link code
_localPath      : QString   — Save destination
_password       : QString   — File password (if needed)
_zipflag        : bool      — Download as ZIP
_status         : int       — DownloadStatus enum
_progress       : double    — 0.0 - 100.0
_speed          : double    — Current speed (bytes/sec)
_abort          : bool      — Pause/cancel flag
_segments       : int       — Number of segments
_realLink       : QString   — Actual download URL from API
_meter          : SpeedMeter — Speed/ETA calculator
_retryCount     : int       — Current retry attempt
```
**States:** START=0, INQUEUE=1, PAUSE=2, STOP=3, ERROR=4, SYSTEMERROR=5, REALLINKERROR=6, FILE_PASS_ERROR=7

### 3.2 UploadFile (QThread)
Active upload task state.
```
_filename       : QString   — File name
_filesize       : qint64    — Total file size
_localPath      : QString   — Source file path
_folderId       : QString   — Destination folder linkcode
_desc           : QString   — File description
_password       : QString   — File password
_secured        : bool      — Secure flag
_directLink     : bool      — Direct link flag
_status         : int       — UploadStatus enum
_progress       : double    — 0.0 - 100.0
_speed          : double    — Current speed (bytes/sec)
_abort          : bool      — Pause/cancel flag
_uploadUrl      : QString   — Upload URL from API
_meter          : SpeedMeter — Speed/ETA calculator
_chunkSize      : int       — Chunk size (default 20MB)
```
**States:** START=0, INQUEUE=1, PAUSE=2, STOP=3, ERROR=4

### 3.3 TableDownloadModel (QAbstractTableModel)
Download queue data model for UI table.
```
_listFileDownload : QList<DownloadFile*> — All download items
_queueDownload    : QQueue<DownloadFile*> — Pending queue
max_threads       : int    — Max concurrent downloads
running_threads   : int    — Currently active downloads
```
**Columns (8):** filename, size, status, progress, speed, finish, path, uri

### 3.4 TableUploadModel (QAbstractTableModel)
Upload queue data model for UI table.
```
_listFileUpload   : QList<UploadFile*> — All upload items
max_threads       : int    — Max concurrent uploads
running_threads   : int    — Currently active uploads
```
**Columns (9):** filename, size, status, progress, speed, finish, folder, uri, complete

---

## 4. HTTP Layer Models (lib/utils/)

### 4.1 httpReqData
HTTP request configuration.
```
headerRq        : map<string, string> — Headers (Cookie, Content-type)
data            : string              — Request body (JSON)
```

### 4.2 http_response
HTTP response container.
```
data            : string              — Response body
headers         : map<string, string> — Response headers
httpRespCode    : int                 — HTTP status code
```

### 4.3 HandleItem
CURL handle wrapper for async operations.
```
curl_handle     : CURL*       — Easy handle
response_data   : string      — Accumulated response
response_header : string      — Accumulated headers
callback        : function    — Completion callback
pClass_resp     : void*       — Context pointer (asyn_manage)
```

---

## 5. Persistence Models

### 5.1 QSettings Keys
```
account/email           : QString  — Saved email for auto-login
account/password        : QString  — Encrypted password (base64)
Download_threads        : int      — Max download threads (default 2)
Upload_threads          : int      — Max upload threads (default 2)
Download_segments       : int      — Segments per download (default 4)
useProxy                : int      — 0=none, 1=IE auto, 2=manual
httpProxy               : QString  — Manual proxy host
httpPort                : QString  — Manual proxy port
Auto_download           : bool     — Auto-start on add
Stay_on_top             : bool     — Always-on-top window
AutoLogin               : bool     — Auto-login at startup
Language                : QString  — Language .qm file path
isInstallChrome         : bool     — Chrome extension installed
isInstallFirefox        : bool     — Firefox addon installed
Path                    : QString  — Application install path
Version                 : QString  — Last run version string
```

### 5.2 XML History Format
**Download History (downloadhistory_{uid}.xml):**
```xml
<DownloadHistory>
  <Item>
    <Name>filename.zip</Name>
    <Size>1073741824</Size>
    <Date>2025-01-01 12:00:00</Date>
    <Path>/downloads/filename.zip</Path>
    <URL>https://www.fshare.vn/file/XXXXX</URL>
    <Status>completed</Status>
  </Item>
</DownloadHistory>
```

**Upload History (uploadhistory_{uid}.xml):**
```xml
<UploadHistory>
  <Item>
    <Name>photo.jpg</Name>
    <Size>5242880</Size>
    <Date>2025-01-02 15:30:00</Date>
    <Folder>My Folder</Folder>
    <URL>https://www.fshare.vn/file/YYYYY</URL>
    <Status>completed</Status>
  </Item>
</UploadHistory>
```

---

## 6. Global State

### 6.1 Extern Globals (global.h)
```
client          : fshare::client  — API client instance
analytics       : GAnalytics*     — Google Analytics
fshareLogger    : FshareLogger*   — Speed/usage logger
megauser        : MegaUser*       — Current user info
```

### 6.2 MainForm Static State
```
m_user_id       : QString (static) — Current user ID
m_user_level    : QString (static) — Current user level string
```

### 6.3 Runtime Flags
```
flagCreateFolder    : int   — Folder creation state
isDownloadPause     : bool  — Global download pause
isUploadPause       : bool  — Global upload pause
specialLinkVideo    : QString — Video link for player
specialFileType     : bool  — Special file handling
g_player            : MediaPlayerInfo — Detected media player
```
