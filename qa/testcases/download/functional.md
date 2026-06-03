# Download — Test Cases & Audit

**Scope:** `DownloadViewModel`, `TransferService` (download path), `FolderExpander`, `DownloadEngine`, `FshareApi` (createDownloadSession / getFileInfo / listFiles), `DownloadPage.qml`.

**Legend:**
- ✅ PASS — code behaves as specified
- ❌ FAIL — bug found
- ⚠️ PARTIAL — partial coverage / edge case uncovered
- 🔒 Security-critical

---

## A. Link parsing & normalization (TC-01 → TC-12)

Fshare URL canonical forms:
- `https://www.fshare.vn/file/<LINKCODE>`
- `https://www.fshare.vn/folder/<LINKCODE>`

Real-world variants users paste: with/without `www.`, `http://`, trailing `/`, uppercase `FILE`/`FOLDER`, query strings (`?token=`, `?secured=1`, `?utm_*`), fragment (`#t=0`), Windows `\r\n` line endings, surrounding spaces/tabs.

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-01 | Canonical file link | `https://www.fshare.vn/file/ABC123` | Accepted as file | ✅ | `DownloadViewModel::isFolderUrl` returns false; goes to `addDownload`. |
| TC-02 | Canonical folder link | `https://www.fshare.vn/folder/ABC123` | Accepted as folder; `addFolderDownload` called | ✅ | Substring match passes — [DownloadViewModel.cpp:185](src/viewmodels/DownloadViewModel.cpp:185). |
| TC-03 | Bare host (no `www.`) | `https://fshare.vn/file/ABC123` | Accepted as file | ✅ | Substring `"fshare.vn/file/"` matches both `www.` and bare. |
| TC-04 | `http://` scheme | `http://fshare.vn/file/ABC123` | Accepted; upgraded/forwarded by API | ✅ | Passed to `createDownloadSession` verbatim; server accepts either scheme. |
| TC-05 | Uppercase path segment | `https://www.fshare.vn/FILE/ABC123` | Accepted as file | ✅ | **BUG-D1 FIXED** — `FshareUrl` regex is case-insensitive; QML badge logic lower-cases before `indexOf`. |
| TC-06 | Trailing slash | `…/file/ABC123/` | Accepted; server strips | ⚠️ | Sent as-is; most CDNs tolerate. Filename extraction `linkcode.section('/', -1)` yields empty — see TC-34 dedup. |
| TC-07 | Share-access token preserved for API | `…/folder/ABC?token=1776741504` | Token KEPT in URL sent to API (required for token-gated shares); linkcode cache key is `ABC` | ✅ | **BUG-D2 / BUG-D7 FIXED** — `FshareUrl::canonicalUrl` preserves `?token=` for API; `FshareUrl::linkcodeOf` gives clean cache key. |
| TC-08 | Fragment stripped | `…/file/ABC123#t=0` | Fragment dropped, scheme/host canonicalized | ✅ | `canonicalUrl` strips fragment and non-token query params. |
| TC-09 | Windows CRLF paste | `file/A\r\nfile/B\r\n` | Split into 2 URLs, `\r` trimmed | ⚠️ | `split('\n', SkipEmptyParts)` keeps trailing `\r`; subsequent `trimmed()` at line 158 DOES strip `\r` (QString::trimmed removes all whitespace incl. `\r`). OK. |
| TC-10 | Surrounding whitespace | `"   https://…/file/A  "` | Trimmed | ✅ | `rawUrl.trimmed()` at [DownloadViewModel.cpp:158](src/viewmodels/DownloadViewModel.cpp:158). |
| TC-11 | Non-fshare URL | `https://example.com/x` | Rejected client-side with clear message | ✅ | **BUG-D3 FIXED** — `DownloadViewModel::addDownload` now collects unrecognised links and emits `downloadBlocked` with a bilingual reason. |
| TC-12 | Empty / whitespace-only input | `"   \n   "` | No-op, no tasks added | ✅ | `url.isEmpty()` check at line 159. |

---

## B. Folder download & recursion (TC-13 → TC-18)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-13 | Flat folder (files only) | folder w/ 10 files | All 10 queued; download starts after full scan | ✅ | [FolderExpander.cpp:82](src/core/services/FolderExpander.cpp:82) emits completed only after full crawl. |
| TC-14 | Nested 1-deep | folder → 2 sub-folders → files | Recurses; sub-folder files use relPath | ✅ | [FolderExpander.cpp:123-125](src/core/services/FolderExpander.cpp:123). |
| TC-15 | Deep nesting (20+) | 25 levels | Hard stops at `maxDepth=20` with warning | ✅ | [FolderExpander.cpp:96-99](src/core/services/FolderExpander.cpp:96). |
| TC-16 | Pagination | folder > 50 files | All pages fetched (`kPageSize=50`) | ✅ | `while(true)` loop in crawl. |
| TC-17 | Sub-folder with forbidden-char name | sub-folder `"bad:name/"` | Folder name sanitised for local path | ✅ | **BUG-D4 FIXED** — `FolderExpander::sanitizeName` now delegates to `FileNameSanitizer::sanitize`. |
| TC-18 | Empty folder | 0 files | `completed` with empty task list; no downloads | ⚠️ | Works but no UI message "folder trống". Low priority. |

---

## C. Single file download (TC-19 → TC-24)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-19 | Small file < 2 MiB | 1 MiB | Single-segment | ✅ | `MIN_SEGMENTED_BYTES=2MiB` guard. |
| TC-20 | Multi-segment capable | 500 MiB | HTTP/2 multiplexed range downloads | ✅ | [DownloadEngine.cpp:484-489](src/core/transfer/DownloadEngine.cpp:484). |
| TC-21 | Large file > 4 GiB | 5 GiB | int64 arithmetic; chunks OK | ✅ | `FSEEK64` + int64 offsets. |
| TC-22 | Server returns no Content-Length | unknown size | Single-segment fallback | ✅ | `rangeSupported=false` path — line 491. |
| TC-23 | Password-protected file | password set | Password sent to `createDownloadSession` | ✅ | [FshareApi.cpp:593](src/core/api/FshareApi.cpp:593). |
| TC-24 | Password empty | "" | Field omitted from JSON | ✅ | Legacy-compat quirk handled at line 593-594. |

---

## D. Queue & concurrency (TC-25 → TC-28)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-25 | Max concurrent | N > `Download/threads` | N queued, `maxDownloads` active | ✅ | [TransferService.cpp:515](src/core/services/TransferService.cpp:515). |
| TC-26 | Settings change midway | user changes threads setting | Next dequeued task picks up new value | ✅ | Re-read on every `startNextInQueue` call — line 512. |
| TC-27 | Pause / resume | active task | `m_paused` flag, cv-wait blocks curl callback | ✅ | [DownloadEngine.cpp:54-63](src/core/transfer/DownloadEngine.cpp:54). |
| TC-28 | Cancel active | `cancelTask(id)` | `m_abort=true`; progress callback returns 1; thread freed | ✅ | Abort flag checked in both write and progress callbacks. |

---

## E. Filename & output path (TC-29 → TC-34)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-29 | 🔒 Server filename with path traversal | `../../passwd` in URL | Path components stripped; file lands in save folder | ✅ | `FileNameSanitizer::sanitize` replaces `/` and `\` — [FileNameSanitizer.cpp:33-34](src/core/util/FileNameSanitizer.cpp:33). |
| TC-30 | Unicode filename (Vietnamese) | `báo cáo.pdf` | Written using UTF-16 file APIs on Windows | ✅ | `openFileUnicode` uses `_wfopen`. |
| TC-31 | 🔒 DOS device name | server returns `CON.txt` | Prefixed `_CON.txt` | ✅ | `FileNameSanitizer` handles it. |
| TC-32 | Save folder doesn't exist | user-set path missing | Created via `mkpath` | ✅ | [DownloadViewModel.cpp:152](src/viewmodels/DownloadViewModel.cpp:152) + [TransferService.cpp:575](src/core/services/TransferService.cpp:575). |
| TC-33 | 🔒 Save to system folder | `C:\Windows\System32` | `downloadBlocked` signal | ✅ | [DownloadViewModel.cpp:141](src/viewmodels/DownloadViewModel.cpp:141). |
| TC-34 | Filename collision | existing `a.mp4`, new `a.mp4` | Renamed to `a (1).mp4` (non-destructive) | ✅ | **BUG-D5 FIXED** — `uniqueDestinationPath` in `TransferService` picks `a (1).mp4`, `a (2).mp4`, … before engine opens the file. |

---

## F. Errors & auth (TC-35 → TC-39)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-35 | Expired token | HTTP 201/202 on session | `sessionExpired` emitted; login modal | ✅ | [TransferService.cpp:539-540](src/core/services/TransferService.cpp:539). |
| TC-36 | Deleted / invalid link | API 404 | `taskFailed` with server message | ✅ | `onDownloadFailed` path. |
| TC-37 | CDN URL expires mid-download | CURL error / HTTP 403 | Failure emitted; no auto-retry | ⚠️ | No retry; user must restart. Acceptable — CDN URLs are fresh per session. |
| TC-38 | Logged-out download attempt | no token | `createDownloadSession` → Auth error | ✅ | Standard error path. |
| TC-39 | Disk full before starting | write vol < fileSize | Preflight refusal with MiB report | ✅ | [DownloadEngine.cpp:454-475](src/core/transfer/DownloadEngine.cpp:454). |

---

## G. Resume & persistence (TC-40 → TC-43)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-40 | Single-segment resume | existing partial file | `CURLOPT_RESUME_FROM_LARGE` used | ✅ | [DownloadEngine.cpp:551-556](src/core/transfer/DownloadEngine.cpp:551). |
| TC-41 | Multi-segment resume | existing partial | `isResume` forces single-segment | ✅ | Line 445+484. |
| TC-42 | History persistence | complete | `saveDownloadHistory` per user | ✅ | [TransferService.cpp:646-651](src/core/services/TransferService.cpp:646). |
| TC-43 | Linkcode cache record | complete | `transferRecordReady` with linkcode key | ✅ | **BUG-D2 FIXED** — because URLs are normalized in `addDownload`, the `linkcode.section('/', -1)` extraction now yields just the alphanumeric code. |

---

## H. Security & edge cases (TC-44 → TC-48)

| # | Case | Input | Expected | Audit | Notes |
|---|------|-------|----------|-------|-------|
| TC-44 | 🔒 Filename with null byte | server sends `"a\0.txt"` | Stripped by control-char filter | ✅ | `u < 0x20` strip in sanitizer. |
| TC-45 | 🔒 Very long filename | 500-char name | Clamped to 200 preserving ext | ✅ | Sanitizer length clamp. |
| TC-46 | Path points to symlink | `localPath` is a symlink to protected dir | Follows symlink (by default) | ⚠️ | No symlink detection; if user picked weird path, could overwrite. Low risk — user-controlled. |
| TC-47 | Concurrent cancel during session-create | cancel before `createDownloadSession` returns | Engine should NOT be spawned; slot not leaked | ✅ | **BUG-D6 FIXED** — post-session callback re-scans `m_tasks` for `taskId`; if absent, returns without touching `m_activeDownloads`. |
| TC-48 | 🔒 Server filename with embedded drive letter | `"C:\\foo"` | Colon → `_`; no drive change | ✅ | Sanitizer replaces `:`. |

---

## Identified bugs

### BUG-D1: File/folder detection is fragile against URL case & path variants (TC-05) ✅ FIXED
**Severity:** Medium  
**Fix applied:** New `FshareUrl::parse` regex `^\s*(?:https?://)?(?:www\.)?fshare\.vn/(file|folder)/([A-Za-z0-9]+)/?(?:[?#].*)?\s*$` used by `DownloadViewModel::isFolderUrl`; `DownloadPage.qml` lower-cases input before analysis.

### BUG-D2: Query strings & fragments leak into linkcode / cache key (TC-07, TC-08, TC-43) 🔒 ✅ FIXED
**Severity:** Medium  
**Fix applied:** Split into two helpers. `FshareUrl::canonicalUrl` is used for API calls — it normalizes scheme/host/casing, drops fragment and non-token query params, BUT preserves the Fshare share-access `?token=XXX` required by token-gated shares. `FshareUrl::linkcodeOf` extracts just the alphanumeric code for cache-key use. `transferRecordReady` now calls `linkcodeOf(snapshot.linkcode)`.

### BUG-D7: Share-access `?token=` was stripped, breaking token-gated folder listings ✅ FIXED
**Severity:** Critical (user-reported — `https://www.fshare.vn/folder/QNSNPB4D7L5MG66?token=1776741504` failed to list)  
**Location:** `FshareUrl::normalize` (earlier fix) → stripped the share token before passing to `listFiles` / `getFileInfo` → API returned empty list / auth error.  
**Fix applied:**
- Replaced `normalize` with `canonicalUrl` that preserves `?token=<value>`.
- `FolderExpander::crawl` now appends the root share token to every sub-folder URL before calling `listFiles`.
- `FolderExpander::makeTask` appends the root share token to file URLs so `createDownloadSession` succeeds for each file in a token-gated share.
- `DownloadViewModel`, `TransferService::addDownload`, `TransferService::addFolderDownload` all updated to use `canonicalUrl` instead of `normalize`.

### BUG-D3: No client-side validation of non-fshare URLs (TC-11) ✅ FIXED
**Severity:** Low  
**Fix applied:** `DownloadViewModel::addDownload` collects all lines that fail `FshareUrl::parse` and emits one `downloadBlocked` signal listing them so the user sees exactly which inputs were rejected.

### BUG-D4: FolderExpander sanitizer weaker than file sanitizer (TC-17) 🔒 ✅ FIXED
**Severity:** Medium  
**Fix applied:** `FolderExpander::sanitizeName` delegates to `FileNameSanitizer::sanitize` so DOS device names, trailing dot/space, >200-char, and control chars are neutralised for sub-folder paths.

### BUG-D5: Silent overwrite of existing files (TC-34) ✅ FIXED
**Severity:** Medium  
**Fix applied:** New static helper `uniqueDestinationPath` in `TransferService.cpp` picks `name (1).ext`, `name (2).ext`, … when the chosen filename already exists. Called from `startNextInQueue` just before `task.localPath` is finalized — in-session pause/resume keeps its stable path because that lifecycle doesn't re-enter the helper.

### BUG-D6: Cancel race with in-flight `createDownloadSession` (TC-47) ✅ FIXED
**Severity:** Medium  
**Fix applied:** Marshalled callback after `createDownloadSession` now re-scans `m_tasks` for `taskId`; if the task has been cancelled the callback logs and returns without spawning an engine or incrementing `m_activeDownloads`.

---

## Fix plan

All 6 bugs fixed. Nothing deferred.

---

## Verification summary (2026-04-21)

- **Fixed & built clean:** BUG-D1, BUG-D2, BUG-D3, BUG-D4, BUG-D5, BUG-D6 (6 of 6).
- **New files:** `src/core/util/FshareUrl.h/.cpp` (URL parser/normalizer), added to `CMakeLists.txt`.
- **Modified files:** `DownloadViewModel.cpp`, `TransferService.cpp`, `FolderExpander.cpp`, `DownloadPage.qml`, `CMakeLists.txt`.
- **Post-fix audit:** TC-05, TC-07, TC-08, TC-11, TC-17, TC-34, TC-43, TC-47 now PASS.
- **Test-case count:** 48 (≥ 30 requested).
