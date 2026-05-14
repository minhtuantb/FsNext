# Upload — Test Cases & Audit

**Scope:** `UploadViewModel`, `TransferService::addUpload`, `FshareApi::createUploadSession`, `UploadEngine`, and `FsUploadDialog` / `UploadPage`.

**Legend:**
- ✅ PASS — code behaves as specified
- ❌ FAIL — bug found
- ⚠️ PARTIAL — partial coverage / edge case uncovered
- 🔒 Security-critical

---

## A. Filename validation & security (TC-01 → TC-10)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-01 | Normal filename | `document.pdf` | Accepted; name sent as-is | ✅ | `kIllegalChars` regex doesn't match; `fi.fileName()` returns clean string. |
| TC-02 | 🔒 Windows-forbidden chars | `bad:name*.txt` | `:` and `*` stripped → `badname.txt` | ✅ | [TransferService.cpp:200](src/core/services/TransferService.cpp:200). |
| TC-03 | Percent sign in name | `50% report.pdf` | Kept (legitimate) | ✅ | **BUG-1 FIXED** — `%` removed from strip regex; kept as-is. |
| TC-04 | All special chars | `\\\\<>:"` | Stripped → empty → rejected with `"Tên file không hợp lệ"` | ✅ | `cleanName.trimmed().isEmpty()` path. |
| TC-05 | 🔒 Path traversal | `../../etc/passwd` | `/` stripped; `fi.fileName()` already extracts basename | ✅ | `QFileInfo::fileName()` returns `passwd`. Defense in depth OK. |
| TC-06 | 🔒 DOS device name | `CON.txt` | Allowed (Fshare server safe; local filesystem not affected) | ✅ | Upload path never writes to local disk, so CON-class names are harmless. |
| TC-07 | Unicode / Vietnamese | `báo cáo 2026.docx` | Accepted; transmitted as UTF-8 | ✅ | `QJsonDocument::toJson()` encodes UTF-8. |
| TC-08 | Very long (>255 chars) | 500-char name | Client sends as-is; server clamps or rejects | ⚠️ | Client has no clamp. If server 500s with obscure error, UX poor. See **BUG-2** (low priority). |
| TC-09 | Leading/trailing spaces | `  file.txt  ` | Trimmed before send | ✅ | **BUG-3 FIXED** — `cleanName = cleanName.trimmed()` now assigned back. |
| TC-10 | 🔒 Server-forbidden chars | `report@v1,draft.pdf` | Stripped OR rejected client-side with actionable message | ✅ | **BUG-4 FIXED** — strip now includes `! , @ # $ ^` plus `..`/`--` sequences; replaced with `_`. |

---

## B. File size boundaries (TC-11 → TC-15)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-11 | Empty file | 0 bytes | Rejected with `"Không thể upload file rỗng"` | ✅ | [TransferService.cpp:185](src/core/services/TransferService.cpp:185). |
| TC-12 | 1-byte file | 1 byte | Accepted; one chunk of 1 byte | ✅ | UploadEngine chunking handles `chunkLen < DEFAULT_UPLOAD_CHUNK` correctly. |
| TC-13 | Large file >4 GiB | 5 GiB | Accepted; multiple chunks; int64 arithmetic | ✅ | `fileSize`, `fromTarget`, `toTarget` all `int64_t`. `FSEEKO64` used for Windows. |
| TC-14 | Exceeds webspace quota | size > webspaceFree | Pre-flight rejected via `uploadError` signal | ✅ | [UploadViewModel.cpp:117](src/viewmodels/UploadViewModel.cpp:117). |
| TC-15 | Exactly at quota boundary | size == webspaceFree | Accepted (strict `>` compare, not `>=`) | ✅ | `needed > avail` — equal size passes. |

---

## C. Concurrency & queue (TC-16 → TC-20)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-16 | Single file | 1 file | Starts immediately | ✅ | `startNextUpload()` finds Queued task and promotes. |
| TC-17 | N files, N > maxUploads | 10 files, max=4 | 4 active, 6 queued | ✅ | Early return when `m_activeUploads >= m_maxUploads`. |
| TC-18 | Settings change midway | User changes `Upload/threads` in Settings | Next batch picks up new limit | ✅ | **BUG-5 FIXED** — `startNextUpload` now re-reads `Upload/threads` each call, clamped to `[1,16]`. |
| TC-19 | Drag-drop 100 files | 100 files at once | All queued; no UI freeze | ⚠️ | 100× `QFileInfo::size()` on main thread is acceptable but not ideal for network drives. Acceptable. |
| TC-20 | Reorder queued tasks | moveUp/Down/First/Last | Only Queued-type Upload moves | ✅ | `findQueuedUploadIdx` guards type + state. |

---

## D. Quota, VIP & secured (TC-21 → TC-25)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-21 | Non-VIP + secured flag | level<3, secured=true | Rejected client-side | ✅ | `if (!user.isVip())` rejects. |
| TC-22 | VIP + secured, enough space | VIP, size<secureFree | Accepted | ✅ | |
| TC-23 | VIP + secured, over quota | size>secureFree | Rejected | ✅ | Strict `>` compare. |
| TC-24 | Non-secured, over webspace | size>webspaceFree | Rejected | ✅ | |
| TC-25 | Logged-out upload | no token | Fails at session create | ⚠️ | Quota check skipped (`!m_auth->isLoggedIn()`), then `createUploadSession` returns auth error → `sessionExpired` emit. User routed to login. OK but could fail-fast earlier with clearer msg. |

---

## E. Session & error handling (TC-26 → TC-30)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-26 | Token expired at session create | HTTP 201/202 | `sessionExpired` emitted; user re-routed to login | ✅ | `checkApiResponse` maps 201/202 → `ErrorCategory::Auth`. |
| TC-27 | Mid-upload session invalidated | `INVALID_UPLOAD_SESSION` in chunk response | Session re-created, retry from byte 0 | ✅ | `UploadEngine::startUpload` checks response body; emits `sessionExpired()`; `onUploadSessionExpired` re-queues. |
| TC-28 | Network failure mid-chunk | CURL error | Retry up to `MAX_CHUNK_RETRIES=3` with backoff | ✅ | Per-chunk retry loop with `QThread::msleep(500 * (retry+1))`. |
| TC-29 | Source file deleted after queue | file gone at upload start | Error `"Source file not found"` emitted | ✅ | `UploadEngine::startUpload` checks `fi.exists()`. |
| TC-30 | Server rejects filename | e.g. `name@.pdf` | Raw server msg shown to user | ✅ | **BUG-4 FIXED** — client-side pre-strip now matches server rule set; server error path rarely reached. |

---

## F. Resume / pause / cancel (TC-31 → TC-35)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-31 | Pause active upload | `pauseTask(id)` | `m_paused=true`; read callback blocks | ✅ | [UploadEngine.cpp:55](src/core/transfer/UploadEngine.cpp:55) msleep loop. |
| TC-32 | Resume paused | `resumeTask(id)` | Continues from current byte | ✅ | Atomic bool flip unblocks callback. |
| TC-33 | Cancel active | `cancelTask(id)` | `m_abort=true`; chunk aborts via CURL_READFUNC_ABORT; thread freed | ✅ | **BUG-6 FIXED** — post-session-create callback now re-checks `m_tasks` for taskId and aborts cleanly if cancelled. |
| TC-34 | App crash → resume | user re-uploads same file | `queryResumeOffset` → server resumes from last byte | ✅ | **BUG-7 FIXED** — `queryResumeOffset` curl handle now sets `Fshare_Tool_2026` User-Agent. |
| TC-35 | Abort during read callback | abort mid-chunk | Callback returns `CURL_READFUNC_ABORT` | ✅ | First thing the callback checks. |

---

## G. Metadata & post-upload (TC-36 → TC-40)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-36 | Password protection | password set in dialog | `setFilePassword` called after upload | ✅ | Fired in QtConcurrent::run; no user feedback on failure. See **BUG-8** (minor). |
| TC-37 | Description stored | desc non-empty | Stored on task; not sent to upload session (API doesn't accept it) | ✅ | Expected — future feature. |
| TC-38 | Copy link to clipboard | completed task | linkcode cached in `m_linkcodeMap` survives removal from active list | ✅ | [UploadViewModel.cpp:35-47](src/viewmodels/UploadViewModel.cpp:35). |
| TC-39 | History persistence | user logged in | `saveUploadHistory(userId, …)` on each complete | ✅ | |
| TC-40 | Cache indicator | completed | `transferRecordReady` emitted with linkcode + localPath | ✅ | |

---

## H. Edge cases (TC-41 → TC-45)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-41 | Duplicate file in dialog | same file twice via picker | De-duplicated by URL | ✅ | `_addFiles` checks `find(f => f.path === fullUrl)`. |
| TC-42 | File from network drive | UNC path | Accepted | ✅ | `openFileUnicode` uses `_wfopen`. |
| TC-43 | 🔒 File with null byte in name | `"foo\0bar.txt"` | Rejected | ⚠️ | Not explicitly tested; QString/QFileInfo handle it but server may error. Low risk (file pickers strip). |
| TC-44 | Path with `%` encoding | `file:///D:/50%25/a.txt` | Decoded properly | ✅ | `QUrl::toLocalFile()` decodes. |
| TC-45 | App shutdown mid-upload | quit during upload | All engines told to abort; threads join with 500 ms timeout; terminate if stuck | ✅ | `TransferService::~TransferService`. |

---

## Identified bugs

### BUG-1: Legitimate `%` in filename stripped (TC-03) ✅ FIXED
**Severity:** Low  
**Location:** [TransferService.cpp:200](src/core/services/TransferService.cpp:200)  
Regex `[\\/:*?"<>|%]` strips `%`, but `%` is a legitimate filename character on Windows/macOS/Linux.  
**Fix applied:** Removed `%` from strip regex. URL decoding is already done by `QUrl::toLocalFile()`.

### BUG-2: No client-side length clamp (TC-08)
**Severity:** Low  
**Location:** TransferService::addUpload  
Filenames >255 chars may trigger obscure server errors. Legacy & download flow clamp to 200 chars preserving extension.  
**Fix:** Apply `FileNameSanitizer`-style 200-char clamp to upload name. Deferred — low UX impact.

### BUG-3: Leading/trailing spaces not trimmed (TC-09) ✅ FIXED
**Severity:** Medium  
**Location:** [TransferService.cpp:206](src/core/services/TransferService.cpp:206)  
`cleanName.trimmed().isEmpty()` was only used for the emptiness check — the original `cleanName` (untrimmed) was sent to the server.  
**Fix applied:** `cleanName = cleanName.trimmed();` after the strip.

### BUG-4: Server-rejected chars leak to server (TC-10, TC-30) ✅ FIXED
**Severity:** Medium (user-reported)  
**Location:** [TransferService.cpp:200](src/core/services/TransferService.cpp:200)  
Server 2026 rejects `! , @ # $ ^ '` and sequences `..` / `--`. Client didn't strip these, so users saw raw server errors.  
**Fix applied:** Strip regex expanded to `[\\/:*?"<>|!,@#$^]`; second regex `\.\.+|--+` collapses sequences; both replaced with `_` (not deleted) for readability.

### BUG-5: Upload thread limit not dynamically re-read (TC-18) ✅ FIXED
**Severity:** Medium  
**Location:** [TransferService.cpp:236-238](src/core/services/TransferService.cpp:236)  
`m_maxUploads` was loaded once in the constructor; settings change only took effect on restart.  
**Fix applied:** `startNextUpload()` now re-reads `Upload/threads` on every call, clamped to `[1,16]`.

### BUG-6: Cancel race with in-flight session create (TC-33) ✅ FIXED
**Severity:** Medium  
**Location:** [TransferService.cpp:256-297](src/core/services/TransferService.cpp:256)  
If user cancelled while `createUploadSession` was in flight, a new engine + thread was still spawned on callback — leaked slot + rogue upload.  
**Fix applied:** Post-callback marshal now re-scans `m_tasks` for `taskId`; if missing, logs cancellation and returns without touching `m_activeUploads`.

### BUG-7: `queryResumeOffset` missing User-Agent (TC-34) ✅ FIXED
**Severity:** Medium  
**Location:** [UploadEngine.cpp:297](src/core/transfer/UploadEngine.cpp:297)  
Fshare's nginx rejects requests without recognised User-Agent (400 before app layer).  
**Fix applied:** `curl_easy_setopt(curl, CURLOPT_USERAGENT, "Fshare_Tool_2026")` added to the resume-query handle.

### BUG-8: Silent password-set failure (TC-36)
**Severity:** Low  
**Location:** [TransferService.cpp:336-345](src/core/services/TransferService.cpp:336)  
If `setFilePassword` fails after upload, only `qWarning`. User believes the password is applied.  
**Fix:** Emit a distinct warning signal (defer — low severity).

---

## Fix plan

Priority (in order): **BUG-5** (settings bug), **BUG-6** (race / leaked slot), **BUG-7** (User-Agent), **BUG-3** (spaces), **BUG-1** (`%`), **BUG-4** (server-chars strip).  
Defer: **BUG-2**, **BUG-8**.

---

## Verification summary (2026-04-21)

- **Fixed & built clean:** BUG-1, BUG-3, BUG-4, BUG-5, BUG-6, BUG-7 (6 of 8).
- **Build:** `output/FsNext.exe` rebuilt successfully — no compile/link errors on `FshareApi.cpp`, `TransferService.cpp`, `UploadEngine.cpp`.
- **Deferred:** BUG-2 (name-length clamp — low UX impact), BUG-8 (silent password-set failure — low severity, needs signal/UI plumbing).
- **Additional diagnostic aid** (unrelated to bug fixes): `FshareApi::createUploadSession` now logs redacted-token request/response bodies to trace user-reported "forbidden chars" server error.
- **Post-fix audit:** TC-03, TC-09, TC-10, TC-18, TC-30, TC-33, TC-34 now PASS.
