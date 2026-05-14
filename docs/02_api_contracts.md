# API Contracts — Fshare Tool v5.3.0

## Base Configuration

- **API Host**: `https://api.fshare.vn`
- **Content-Type**: `application/json`
- **Authentication**: Token-based (session token + PHPSESSID cookie)
- **Cookie Format**: `Cookie: PHPSESSID={session_id}; key={app_key}`
- **Logging Endpoint**: `https://flog.fshare.vn/`

---

## 1. Authentication

### 1.1 Login
```
POST /api/user/login
```
**Request:**
```json
{
  "app_key": "string",
  "user_email": "string",
  "password": "string"
}
```
**Response (200):**
```json
{
  "code": 200,
  "msg": "string",
  "token": "string",
  "session_id": "string"
}
```
**Response Headers:**
```
Set-Cookie: PHPSESSID=xxx
```
**Error codes:** Non-200 code field = error, msg field contains description

### 1.2 OAuth / Social Login
```
POST /api/user/oauth
```
**Request:**
```json
{
  "app_key": "string",
  "service": "google|facebook|fptid",
  "access_token": "string",
  "user_email": "string"
}
```
**Response:** Same structure as Login

### 1.3 Logout
```
GET /api/user/logout
Headers: Cookie (PHPSESSID + app_key), Content-type: application/json
```
**Response:**
```json
{
  "code": 200,
  "msg": "string"
}
```

### 1.4 Get User Info
```
GET /api/user/get
Headers: Cookie (PHPSESSID + app_key), Content-type: application/json
```
**Response:**
```json
{
  "id": "numeric_string",
  "email": "string",
  "name": "string",
  "level": "numeric_string",
  "expire_vip": "timestamp_string",
  "joindate": "timestamp_string",
  "totalpoints": "numeric_string",
  "traffic": "bytes_string",
  "traffic_used": "bytes_string",
  "webspace": "bytes_string",
  "webspace_used": "bytes_string",
  "webspace_secure": "bytes_string",
  "webspace_secure_used": "bytes_string",
  "amount": "numeric_string",
  "dl_time_avail": "numeric_string",
  "account_type": "string"
}
```

---

## 2. File Operations

### 2.1 List Files in Folder (Paginated)
```
POST /api/fileops/getFolderListPaging
```
**Request:**
```json
{
  "token": "string",
  "url": "folder_linkcode",
  "page_index": 0,
  "page_size": 50
}
```
**Response:** Array of file objects

### 2.2 List Files (Legacy, Full Tree)
```
GET /api/fileops/list/{path}?pageIndex={i}&dirOnly={0|1}&limit={2000}
Headers: Cookie, Content-type
```
**Response:** Array of file objects
- `dirOnly=1`: folders only (limit 1000)
- `dirOnly=0`: all items (limit 2000)
- Pagination: increment pageIndex until response.length < limit

### 2.3 Get Total Files in Folder
```
POST /api/fileops/getTotalFileInFolder
```
**Request:**
```json
{
  "token": "string",
  "url": "folder_linkcode"
}
```
**Response:**
```json
{
  "total": "numeric_string"
}
```

### 2.4 Get File Info
```
POST /api/fileops/get
```
**Request:**
```json
{
  "token": "string",
  "url": "linkcode"
}
```
**Response (single file):**
```json
{
  "id": "numeric_string",
  "linkcode": "string",
  "name": "string",
  "file_type": "string",
  "size": "bytes_string",
  "path": "string",
  "secure": "0|1",
  "public": "0|1",
  "directlink": "0|1",
  "hash_index": "string",
  "owner_id": "numeric_string",
  "pid": "parent_id_string",
  "downloadcount": "numeric_string",
  "deleted": "0|1",
  "description": "string",
  "created": "timestamp_string",
  "lastdownload": "timestamp_string",
  "modified": "timestamp_string",
  "pwd": "boolean_or_string"
}
```

### 2.5 Rename File/Folder
```
POST /api/fileops/rename
```
**Request:**
```json
{
  "token": "string",
  "file": "linkcode",
  "new_name": "string"
}
```
**Response:**
```json
{
  "code": 200,
  "msg": "string"
}
```

### 2.6 Delete Files/Folders
```
POST /api/fileops/delete
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1", "linkcode2"]
}
```
**Response:**
```json
{
  "code": 200,
  "msg": "string"
}
```

### 2.7 Create Folder
```
POST /api/fileops/createFolder
```
**Request:**
```json
{
  "token": "string",
  "name": "string",
  "in_dir": "parent_linkcode"
}
```
**Response:**
```json
{
  "code": 200,
  "msg": "string"
}
```

### 2.8 Create Folder (by Path)
```
POST /api/fileops/createFolderInPath
```
**Request:**
```json
{
  "token": "string",
  "name": "string",
  "in_dir": "path_string"
}
```

### 2.9 Move Files/Folders
```
POST /api/fileops/move
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1", "linkcode2"],
  "to": "destination_linkcode"
}
```
**Response:**
```json
{
  "code": 200,
  "msg": "string"
}
```

### 2.10 Move to Path
```
POST /api/fileops/moveToPath
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1"],
  "to": "destination_path"
}
```

### 2.11 Copy Files/Folders
```
POST /api/fileops/copy
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1", "linkcode2"],
  "to": "destination_linkcode"
}
```

### 2.12 Search Files
```
POST /api/fileops/search?pageIndex={i}
```
**Request:**
```json
{
  "token": "string",
  "keyword": "search_term"
}
```
**Response:** Array of file objects (max 50 per page)

---

## 3. Security & Sharing

### 3.1 Change Secure Status
```
POST /api/fileops/changeSecure
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1"],
  "status": 0 or 1
}
```

### 3.2 Create/Change File Password
```
POST /api/fileops/createFilePass
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1"],
  "pass": "password_string"
}
```

### 3.3 Set Direct Link
```
POST /api/share/SetDirectLink
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1"],
  "status": 0 or 1
}
```

### 3.4 Create Share Link
```
POST /api/share/createsharelink
```
**Request:**
```json
{
  "token": "string",
  "items": ["linkcode1"],
  "emails": ["email1@example.com"],
  "content": "share_message"
}
```
**Response:** Array of share link strings

---

## 4. Sessions (Download/Upload)

### 4.1 Create Download Session
```
POST /api/session/download
```
**Request:**
```json
{
  "token": "string",
  "url": "file_linkcode_or_url",
  "password": "string_or_empty",
  "zipflag": 0 or 1
}
```
**Response:**
```json
{
  "location": "actual_download_url",
  "code": 200,
  "msg": "string"
}
```
**Note:** `location` is the real download URL for CURL to fetch the file.

### 4.2 Create Upload Session
```
POST /api/session/upload
```
**Request:**
```json
{
  "token": "string",
  "name": "filename",
  "size": "file_size_bytes",
  "path": "destination_folder_linkcode",
  "secured": 0 or 1
}
```
**Response:**
```json
{
  "location": "upload_target_url",
  "code": 200,
  "msg": "string"
}
```

---

## 5. Photos

### 5.1 List Photos
```
GET /api/photos/list/{path}?pageIndex={i}
Headers: Cookie, Content-type
```
**Response:** Array of photo objects:
```json
{
  "id": "string",
  "name": "string",
  "path": "string",
  "created": "timestamp",
  "s": "small_image_url",
  "l": "large_image_url"
}
```

---

## 6. Service

### 6.1 Get Latest Version
```
GET /api/service/getlatestversion?type={platform}
```
**Response:**
```json
{
  "version": "string",
  "link": "download_url"
}
```

### 6.2 Send Log
```
POST https://flog.fshare.vn/
```
**Request:**
```json
{
  "account_id": "string",
  "type": "string",
  "level": "string",
  "speed": "numeric",
  "concurrent": "numeric"
}
```

---

## 7. OAuth Provider Endpoints (External)

### 7.1 Google
- **Auth URL**: `https://accounts.google.com/o/oauth2/v2/auth`
- **Token URL**: `https://oauth2.googleapis.com/token`
- **Userinfo URL**: `https://www.googleapis.com/oauth2/v3/userinfo`
- **Scope**: `email profile`

### 7.2 Facebook
- **Auth URL**: `https://www.facebook.com/v18.0/dialog/oauth`
- **Token URL**: `https://graph.facebook.com/v18.0/oauth/access_token`
- **Userinfo URL**: `https://graph.facebook.com/me?fields=email,name`

### 7.3 FPT ID
- **Token URL**: configured in oauthconfig.h
- **Userinfo URL**: configured in oauthconfig.h

---

## 8. Common Data Structures

### File Object (from API)
```json
{
  "id": "numeric_string",
  "linkcode": "string (unique identifier)",
  "name": "string",
  "type": "folder|file",
  "size": "bytes_string (int64)",
  "path": "string",
  "secure": "0|1",
  "public": "0|1",
  "copied": "0|1",
  "shared": "0|1",
  "directlink": "0|1",
  "hash_index": "string",
  "owner_id": "numeric_string",
  "pid": "parent_folder_id",
  "downloadcount": "numeric_string",
  "deleted": "0|1",
  "description": "string",
  "created": "timestamp_string",
  "modified": "timestamp_string",
  "pwd": "boolean or password string"
}
```

### Standard Response Envelope
```json
{
  "code": 200,
  "msg": "Success or error message"
}
```

### Error Codes
| Code | Meaning |
|------|---------|
| 200 | Success |
| 0 | Network error (mapped to 600 in app) |
| 99 | JSON parse failure |
| 502 | Bad gateway (server temporarily unavailable, retry) |
| Other | Server-specific error, `msg` field has description |
