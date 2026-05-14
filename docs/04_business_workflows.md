# Business Workflows — Fshare Tool v5.3.0

## 1. Application Startup

```
App Launch
├─ Initialize OpenSSL threading (mutex locks for CURL)
├─ Initialize CURL globally
├─ Check single instance (QtSingleApplication)
│  ├─ If another instance: send message (link) → exit
│  └─ If first instance: continue
├─ Check for Chrome native messaging mode (argv[1] == "-d")
│  └─ If yes: run as native messaging host → exit
├─ Load QSettings
├─ Configure proxy (none / IE auto / manual)
├─ Set CA certificate path for SSL
├─ Initialize fshare::client with app_key
├─ Initialize GAnalytics
├─ Show LoginForm
│  ├─ If AutoLogin enabled AND saved credentials exist:
│  │  └─ Auto-trigger login → skip user input
│  └─ Else: wait for user input
└─ After successful login → Show MainForm
```

## 2. Authentication Flow

### 2.1 Email/Password Login
```
User enters email + password → clicks Login
├─ Validate email format (basic check)
├─ Validate password not empty
├─ Create ActionThread with USER_LOGIN flag
│  └─ ActionThread::run()
│     └─ fshare::user_api::login(email, password)
│        └─ POST /api/user/login {app_key, user_email, password}
│           ├─ Success (code=200):
│           │  ├─ Extract token, session_id from response
│           │  ├─ Extract PHPSESSID from Set-Cookie header
│           │  ├─ Build httpReqData (Cookie + Content-type headers)
│           │  ├─ Create session object
│           │  └─ emit loginSuccess()
│           └─ Error:
│              ├─ Store errCode, errMsg
│              └─ emit loginError(code, msg)
├─ On loginSuccess:
│  ├─ Save credentials if "Remember Me" checked
│  ├─ Fetch user info: fshare::user_api::getUserInfo()
│  │  └─ GET /api/user/get → parse user data
│  ├─ Populate MegaUser from user data
│  ├─ Check for forced update (version too old → block app)
│  ├─ emit loginFinish(user) → MainForm::initMainForm()
│  └─ Close LoginForm, show MainForm
└─ On loginError:
   └─ Show error message (e.g., "Wrong password", "Account locked")
```

### 2.2 Social Login (OAuth)
```
User clicks Google/Facebook/FPT ID button
├─ Open SocialLoginDialog (QWebEngineView)
│  └─ Navigate to provider auth URL with client_id, redirect_uri, scope
├─ User logs in at provider website
├─ Provider redirects to: https://www.fshare.vn/...?code=XXX&state=YYY
├─ FshareWebEnginePage intercepts redirect URL
│  └─ Extract authorization code from URL
├─ Close SocialLoginDialog
├─ Create ActionThread with SOCIAL_LOGIN flag
│  └─ OAuthHelper::exchangeCodeAndGetEmail(provider, code, redirectUri)
│     ├─ CURL POST to provider token endpoint
│     │  → Exchange code for access_token
│     ├─ CURL GET to provider userinfo endpoint
│     │  → Extract email from user profile
│     └─ Return OAuthResult {success, accessToken, email}
├─ On success:
│  └─ fshare::user_api::loginOAuth(service, accessToken, email)
│     └─ POST /api/user/oauth → same flow as normal login
└─ On error:
   └─ Show error message
```

## 3. Download Workflow

### 3.1 Add Download
```
User adds download link(s) (paste URL, drag&drop, or Chrome extension)
├─ Parse link: extract linkcode from https://www.fshare.vn/file/{linkcode}
├─ Show DialogDownload
│  ├─ Enter link (pre-filled if from paste/drag)
│  ├─ Select save folder
│  ├─ Enter password (optional, for protected files)
│  └─ Click OK
├─ Create DownloadFile object
│  ├─ Set linkcode, localPath, password
│  ├─ Set status = INQUEUE
│  └─ Add to TableDownloadModel
└─ If Auto-download enabled:
   └─ Start download immediately
```

### 3.2 Download Execution
```
TableDownloadModel::startDownloadQueue()
├─ Check running_threads < max_threads
├─ Dequeue next DownloadFile from queue
├─ Start DownloadFile QThread
│  └─ DownloadFile::run()
│     ├─ Step 1: Get real download URL
│     │  └─ fshare::sessionapi::createDownloadSession(linkcode, password)
│     │     └─ POST /api/session/download
│     │        ├─ Success: get location (actual URL)
│     │        └─ Error: emit taskDownloadError
│     │           ├─ FILE_PASS_ERROR → prompt for password
│     │           ├─ REALLINKERROR → retry genLinkDownload
│     │           └─ SYSTEMERROR → abort
│     │
│     ├─ Step 2: Get file info (size, name)
│     │  └─ HTTP HEAD request to real URL → Content-Length
│     │
│     ├─ Step 3: Download file
│     │  ├─ If filesize >= MIN_SEGMENTED (4MB) AND segments > 1:
│     │  │  └─ Multi-segment download (CURL multi)
│     │  │     ├─ Split file into N segments
│     │  │     ├─ Create N CURL easy handles with CURLOPT_RANGE
│     │  │     ├─ Each segment writes to file at correct offset
│     │  │     ├─ Progress callback: segmented_xferinfo()
│     │  │     │  └─ Emit dataChanged() for UI update
│     │  │     └─ curl_multi_perform() loop until done
│     │  └─ Else:
│     │     └─ Single-segment download (CURL easy)
│     │        ├─ CURLOPT_WRITEFUNCTION → write to file
│     │        ├─ CURLOPT_XFERINFOFUNCTION → progress
│     │        └─ CURLOPT_RESUME_FROM_LARGE (if resuming)
│     │
│     ├─ Step 4: Check for pause
│     │  └─ If _abort == true:
│     │     └─ m_abortWait.wait() → thread suspends
│     │        → On resume: m_abortWait.wakeAll() → continue
│     │
│     ├─ Step 5: Complete
│     │  ├─ Verify file size matches expected
│     │  ├─ Emit taskDownloadDone()
│     │  └─ TableDownloadModel moves to completed list
│     │
│     └─ Step 6: Error recovery
│        ├─ If CURL error AND retry < DEFAULT_DOWNLOAD_TRY:
│        │  └─ Wait brief period → retry from Step 1
│        └─ If max retries exceeded:
│           └─ Set status = ERROR, emit error signal
│
└─ After completion: start next queued item
```

### 3.3 Pause/Resume Download
```
Pause:
├─ User clicks Pause (single file or all)
├─ DownloadFile::stopDataTransfer()
│  └─ Set _abort = true
│     └─ Worker thread detects _abort in CURL callback
│        └─ m_abortWait.wait(&m_abortMutex) → thread suspends
└─ UI shows PAUSED status

Resume:
├─ User clicks Resume
├─ DownloadFile::startDownload()
│  └─ Clear _abort, call m_abortWait.wakeAll()
│     └─ Thread wakes, continues CURL transfer
│        (RESUME_FROM_LARGE if file partially written)
└─ UI shows DOWNLOADING status
```

## 4. Upload Workflow

### 4.1 Add Upload
```
User clicks Upload button
├─ Show DialogUpload
│  ├─ Select files (file picker)
│  ├─ Select destination folder (Fshare folder browser)
│  ├─ Enter description (optional)
│  ├─ Enter password (optional)
│  ├─ Toggle secure (optional)
│  ├─ Toggle direct link (optional)
│  └─ Click OK
├─ Check user quota: webspace_used + total_file_size <= webspace
│  └─ If exceeded: show error, abort
├─ For each file:
│  ├─ Create UploadFile object
│  │  ├─ Set localPath, folderId, desc, password, secured, directLink
│  │  └─ Set status = INQUEUE
│  └─ Add to TableUploadModel
└─ Start upload queue
```

### 4.2 Upload Execution
```
TableUploadModel::startUploadQueue()
├─ Check running_threads < max_threads
├─ Dequeue next UploadFile
├─ Start UploadFile QThread
│  └─ UploadFile::run()
│     ├─ Step 1: Get upload URL
│     │  └─ fshare::sessionapi::createUploadSession(name, size, folder, secured)
│     │     └─ POST /api/session/upload
│     │        └─ Response: location (upload target URL)
│     │
│     ├─ Step 2: Upload file in chunks (20MB default)
│     │  ├─ Open local file for reading
│     │  ├─ For each chunk:
│     │  │  ├─ Read chunk from file (readFunc callback)
│     │  │  ├─ CURL POST to upload URL with Content-Range
│     │  │  ├─ Progress callback → _meter update → emit dataChanged()
│     │  │  └─ Check _abort for pause
│     │  └─ Continue until EOF
│     │
│     ├─ Step 3: Complete
│     │  ├─ Emit taskUploadDone()
│     │  └─ Move to completed list
│     │
│     └─ Step 4: Error recovery
│        └─ Retry up to DEFAULT_UPLOAD_TRY (100) times
│
└─ After completion: start next queued item
```

## 5. File Management Workflow

### 5.1 Browse Files
```
User clicks Manage tab
├─ Load folder tree
│  └─ ActionThread: fshare::file_api::getList(dirFlag=1)
│     └─ GET /api/fileops/list/?dirOnly=1&limit=1000
│        └─ Paginate: repeat until all folders loaded
│     └─ Build QTreeWidget with folder hierarchy
├─ User clicks folder in tree
│  └─ Load file list for folder
│     └─ fshare::file_api::getListFileInFolder(linkcode, page, pageSize)
│        └─ POST /api/fileops/getFolderListPaging
│        └─ Display in table (name, size, date, status)
└─ User can navigate, expand folders, refresh
```

### 5.2 File Operations
```
Move:
├─ Select file(s) → Right-click → Move
├─ Show DialogMoveFIle (folder tree picker)
├─ Select destination folder
└─ ActionThread::actionMoveFile(items[], to)
   └─ POST /api/fileops/move → refresh view

Copy:
├─ Similar to Move, using /api/fileops/copy

Delete:
├─ Select file(s) → Right-click → Delete
├─ Confirm dialog
└─ fshare::file_api::remove(items[])
   └─ POST /api/fileops/delete → refresh view

Rename:
├─ Select file → Right-click → Rename
├─ Input new name
└─ fshare::file_api::rename(linkcode, newName)
   └─ POST /api/fileops/rename → refresh view

Create Folder:
├─ Right-click → New Folder
├─ Input folder name
└─ fshare::file_api::createFolder(name, parentId)
   └─ POST /api/fileops/createFolder → refresh tree
```

### 5.3 Security Operations
```
Toggle Secure:
└─ ActionThread::actionSecureFile(items[], 0|1)
   └─ POST /api/fileops/changeSecure

Set Password:
└─ ActionThread::actionCreatePasswordFile(items[], password)
   └─ POST /api/fileops/createFilePass

Toggle Direct Link:
└─ ActionThread::actionCreateDirectLink(items[], 0|1)
   └─ POST /api/share/SetDirectLink
```

## 6. Settings Workflow

```
User opens Settings (menu → Options)
├─ Show DialogOption (4 tabs)
│  ├─ General: language, auto-login, stay-on-top
│  ├─ Connect: proxy mode + manual host:port
│  ├─ Download: threads, segments, auto-download, folder
│  └─ Upload: threads, folder
├─ User modifies settings → OK
├─ Save to QSettings
├─ Apply immediately:
│  ├─ Download threads → TableDownloadModel::setMaxThreads()
│  ├─ Upload threads → TableUploadModel::setMaxThreads()
│  ├─ Proxy → reconfigure Qt proxy + CURL proxy callback
│  ├─ Language → reload translations (.qm file)
│  └─ Stay-on-top → toggle window flag
└─ Restart may be required for some settings (language)
```

## 7. Chrome Extension Integration

```
User clicks Fshare link in Chrome
├─ Chrome extension captures link
├─ Send via native messaging to fsharenativeapp.exe
│  └─ stdin: 4-byte length + JSON {link}
├─ If Fshare Tool running:
│  └─ QtSingleApplication::sendMessage(link)
│     └─ MainForm::handleMessage(link)
│        └─ Add to download queue (same as manual add)
└─ If not running:
   └─ fsharenativeapp checks registry, responds with status
```

## 8. Update Check Workflow

```
On app startup (after login)
├─ fshare::sessionapi::getLatestVersion(platform)
│  └─ GET /api/service/getlatestversion?type=windows
├─ Compare response version with current VERSION
│  ├─ If same: continue normally
│  ├─ If newer (optional update):
│  │  └─ Show notification, offer download
│  └─ If newer (forced update):
│     └─ Block app usage until updated
│        ├─ Show update dialog
│        ├─ Download update binary
│        └─ Launch installer, exit app
└─ Done
```

## 9. RSS Auto-Download Workflow

```
RssManager::processAutoDownload()
├─ Fetch RSS feed from configured source URL
├─ Parse XML: extract categories and episodes
├─ For each episode matching filter rules:
│  ├─ Check if already downloaded (history)
│  ├─ If new: extract Fshare download link
│  ├─ Emit addDownloadLinks(link, folder, password)
│  │  └─ MainForm receives signal
│  │     └─ FormDownload::rss_get_dir(link, folder, password)
│  │        └─ Create DownloadFile → add to queue → auto-start
│  └─ Record as downloaded in history XML
└─ Repeat on configured interval
```
