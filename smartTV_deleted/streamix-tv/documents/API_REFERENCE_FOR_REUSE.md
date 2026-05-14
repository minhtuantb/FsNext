# Tài liệu API & Khóa dùng cho dự án StreamIX (tái sử dụng cho dự án khác)

> Tài liệu này tổng hợp đầy đủ tất cả API liên quan đến **Fshare**, **Timfshare** và **Google Sheet** đang được StreamIX sử dụng. Mục tiêu: copy/paste là dùng được ngay trong dự án Python / Node / Qt / mobile khác.
>
> Nguồn trích: `FshareClient.h/.cpp`, `SearchService.h/.cpp`, `MainWindow.cpp`, `infra/analytics/Ga4MeasurementClient.cpp`, `build-secret.local.env.bat.example`.

---

## 1. Tổng quan kiến trúc

StreamIX gọi 4 nhóm dịch vụ:

| Nhóm | Host | Mục đích |
|------|------|---------|
| Fshare Official API | `api.fshare.vn` | Login, lấy link stream/download trực tiếp, list thư mục, lấy info file, lấy info user |
| Fshare Public Web | `www.fshare.vn` | Parse trang Top public (HTML) |
| Timfshare (đối tác search) | `api.timfshare.com`, `timfshare.com` | Tìm kiếm video theo từ khóa & lấy danh sách video xu hướng |
| Google Sheet CSV export | `docs.google.com` | Tải bảng "Gợi ý" và "Xu hướng" do team biên tập thủ công |
| Google Analytics 4 (MP) | `www.google-analytics.com` | Telemetry sự kiện (tùy chọn) |

---

## 2. Các khóa & secret (đầy đủ)

```text
# ─── Fshare Official API ──────────────────────────────────────
APP_KEY            = "tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch"
USER_AGENT         = "FshareVideoDesktop_23052023"
SHARE_REFERRAL_ID  = "8805984"   # query ?share=8805984 — gắn vào URL khi getDirectUrl

# ─── Timfshare (đối tác cung cấp search) ──────────────────────
TIMFSHARE_BEARER   = "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJuYW1lIjoiZnNoYXJlIiwidXVpZCI6IjcxZjU1NjFkMTUiLCJ0eXBlIjoicGFydG5lciIsImV4cGlyZXMiOjAsImV4cGlyZSI6MH0.WBWRKbFf7nJ7gDn1rOgENh1_doPc07MNsKwiKCJg40U"
TIMFSHARE_UA       = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"
# JWT decode: { name: "fshare", uuid: "71f5561d15", type: "partner", expires: 0, expire: 0 }
# Token không hết hạn (expires=0). Vẫn nên coi là secret và không log header.

# ─── Google Sheet CSV (file_id + gid) ─────────────────────────
SHEET_GOIY_FILE_ID    = "1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0"
SHEET_GOIY_GID        = "1370676663"

SHEET_XUHUONG_FILE_ID = "1r2e9W81ostp9PmosITlajdzKK4oY275Q"
SHEET_XUHUONG_GID     = "936805767"

# ─── Google Analytics 4 (Measurement Protocol) ────────────────
GA4_MEASUREMENT_ID = "G-252LMTWNB9"
GA4_API_SECRET     = <từ env STREAMIX_GA4_API_SECRET — KHÔNG nhúng trong repo>
```

> ⚠️ **TIMFSHARE_BEARER** là JWT do partner Timfshare cấp riêng cho ứng dụng Fshare. Nếu Timfshare revoke, search/trending sẽ trả 401. Liên hệ partner để xin token mới.

---

## 3. Fshare Official API

Base: `https://api.fshare.vn`

Nguyên tắc chung:
* **Login first**. Sau login lưu `token` (string) và `session_id` (cookie). Mọi request sau gắn:
  * Header `User-Agent: FshareVideoDesktop_23052023` (bắt buộc, sai sẽ trả "Invalid User Agent!").
  * Header `Cookie: session_id=<session_id>`.
  * Body JSON có trường `"token": "<token>"`.
* HTTP status `201` từ bất kỳ endpoint nào ⇒ phiên hết hạn ⇒ phải login lại.
* HTTP status `200` không đảm bảo thành công — kiểm tra thêm trường `code` trong body JSON.

### 3.1 POST `/api/user/login`

Đăng nhập. Trả `token` + `session_id`.

```http
POST https://api.fshare.vn/api/user/login
User-Agent: FshareVideoDesktop_23052023
cache-control: no-cache
# (KHÔNG đặt Content-Type — addon Python gốc cũng không đặt)

{
  "app_key":    "tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch",
  "user_email": "you@example.com",
  "password":   "your_password"
}
```

Response thành công (200):

```json
{
  "code": 200,
  "msg": "Login successful!",
  "token": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
  "session_id": "yyyyyyyyyyyyyyyyyy"
}
```

Mã lỗi đáng chú ý (HTTP / `code` trong body):
| HTTP | code | Ý nghĩa |
|------|------|---------|
| 400  | 400  | Email/mật khẩu sai định dạng |
| 405  | 405 / "authenticate" | Sai email hoặc mật khẩu |
| 405  | "too many" | Chặn vì gõ sai quá nhiều lần |
| 406  | 406  | Tài khoản chưa kích hoạt |
| 408  | 407  | Bắt buộc đổi mật khẩu |
| 408  | 408  | Mật khẩu trùng số điện thoại |
| 409  | 409  | Tài khoản bị khóa |
| 410  | 410  | Domain email bị cấm |
| 424  | 424  | Bị tạm khóa do nhập sai nhiều lần (đợi 10 phút) |
| 403  | 33   | App chưa được cấp phép / dùng bản giả |
| 403  | 37/38| Thiết bị/môi trường không được hỗ trợ |
| 403  | 39   | App key không hợp lệ — cập nhật app |
| 403  | 403  | "Forbidden / Email invalid / Vip-only" |
| 502  | —    | App phiên bản chưa tương thích |

### 3.2 GET `/api/user/get`

Lấy thông tin user (level VIP, email, quota dung lượng).

```http
GET https://api.fshare.vn/api/user/get
User-Agent: FshareVideoDesktop_23052023
Cookie: session_id=<session_id>
```

Response (200): Trả flat object hoặc nested trong `data` / `user`. Các trường StreamIX đọc:

| Khoá đọc theo thứ tự ưu tiên | Ý nghĩa |
|---|---|
| `level` → `user_level` → `account_level` | Cấp độ VIP (>=1 hợp lệ) |
| `email` → `user_email` → `login` | Email |
| `user_id` → `userid` → `id` | ID user |
| `webspace_used` / `storage_used` / `used` / `used_bytes` / `traffic_used` | Đã dùng (bytes) |
| `webspace_remaining` / `storage_remaining` / `remaining` / `remain` / `remain_bytes` / `traffic_remaining` | Còn lại (bytes) |
| `webspace` / `storage_total` / `quota` / `quota_bytes` / `traffic` / `storage_limit` | Tổng (bytes) |

`code: 201` ⇒ session hết hạn.

### 3.3 POST `/api/session/download` — Lấy direct URL để stream/tải

```http
POST https://api.fshare.vn/api/session/download
Content-Type: application/json
User-Agent: FshareVideoDesktop_23052023
Cookie: session_id=<session_id>

{
  "zipflag": 0,
  "url":     "https://www.fshare.vn/file/ABC123?share=8805984",
  "password": "",
  "token":   "<token>"
}
```

Lưu ý:
* URL phải là `https://`, đúng host `fshare.vn`.
* StreamIX **luôn** chèn `?share=8805984` hoặc `&share=8805984` (referral) trước khi gọi.
* Trả về (200):
  ```json
  { "code": 200, "location": "https://download.fshare.vn/...stream-key..." }
  ```
* Mã lỗi:

| HTTP/code | Ý nghĩa |
|---|---|
| 200 + `code:123` | File yêu cầu mật khẩu |
| 201 | Session expired |
| 404 | Link không tồn tại / đã xóa |
| 471 | Quá nhiều phiên tải — vào fshare.vn → Bảo mật → Xóa phiên tải |

### 3.4 POST `/api/fileops/get` — Lấy info file

```http
POST https://api.fshare.vn/api/fileops/get
Content-Type: application/json
User-Agent: FshareVideoDesktop_23052023
Cookie: session_id=<session_id>

{
  "token": "<token>",
  "url":   "https://www.fshare.vn/file/ABC123"
}
```

Response thành công (200, code 200): Trả flat hoặc nested trong `data` / `file`. Đọc theo ưu tiên:
* Tên file: `name` → `filename` → `file_name`
* Kích thước: `size` → `file_size` → `filesize` → `human_size`

`code: 201` ⇒ session expired. Status 200 + code != 200 ⇒ lấy `msg` / `message` / `error` / `error_msg` để hiện lỗi.

### 3.5 POST `/api/fileops/getFolderList` — Liệt kê thư mục

```http
POST https://api.fshare.vn/api/fileops/getFolderList
Content-Type: application/json
User-Agent: FshareVideoDesktop_23052023
Cookie: session_id=<session_id>

{
  "token":     "<token>",
  "url":       "https://www.fshare.vn/folder/<linkcode>",
  "dirOnly":   0,
  "pageIndex": 0,
  "limit":     100
}
```

Yêu cầu **canonical URL** dạng `https://www.fshare.vn/folder/<linkcode>` (cắt query/fragment) — nếu không, API trả 404.

Response: có thể là array, hoặc object có `data`/`items`/`folders`/`files` (array) hoặc `data.items`/`data.folders`/`data.files`.

Mỗi item:
```json
{
  "name":     "Tập 01.mp4",
  "linkcode": "ABC123",
  "type":     "1",         // "0" = folder, "1" = file
  "size":     "534521234",
  "id":       1234567890   // dùng sort theo mới/cũ
}
```

URL StreamIX tự build:
* `type=="0"` → `https://www.fshare.vn/folder/<linkcode>`
* khác → `https://www.fshare.vn/file/<linkcode>`

`status==404` hoặc body `[]` ⇒ thư mục trống/không tồn tại. `status==201` ⇒ session expired.

### 3.6 GET `https://www.fshare.vn/top` — Public Top page (HTML)

Không cần đăng nhập. Parse HTML, trích các `<a href="/folder/<linkcode>" ...>Title</a>`.

Regex StreamIX dùng:
```regex
href=["']/folder/([A-Za-z0-9]+)["'][^>]*>([^<]+)</a>
```
Capture 1 = linkcode, capture 2 = tiêu đề. Build URL: `https://www.fshare.vn/folder/<linkcode>`.

---

## 4. Timfshare API (đối tác search)

### 4.1 POST `https://api.timfshare.com/v1/string-query-search?query=<keyword>`

```http
POST https://api.timfshare.com/v1/string-query-search?query=<percent-encoded keyword>
Content-Type: application/json
Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJuYW1lIjoiZnNoYXJlIiwidXVpZCI6IjcxZjU1NjFkMTUiLCJ0eXBlIjoicGFydG5lciIsImV4cGlyZXMiOjAsImV4cGlyZSI6MH0.WBWRKbFf7nJ7gDn1rOgENh1_doPc07MNsKwiKCJg40U
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) ... Chrome/58.0.3029.110 Safari/537.36

(empty body)
```

Lưu ý: keyword `.` được StreamIX thay bằng space trước khi encode. Giới hạn 256 ký tự, lọc control char + sequence nguy hiểm (`<script`, `javascript:`, …).

Response (200):
```json
{
  "data": [
    {
      "id": 1234,
      "name": "FilmName.mp4",
      "url":  "https://www.fshare.vn/file/ABC123?token=xxx",
      "file_type":   "1",            // "0" = không phát được, khác = playable
      "size":        534521234,      // hoặc string
      "size_display": "509.78 MB"    // tùy có/không
    }
  ]
}
```

Quy tắc xử lý của StreamIX:
* Cắt query string khỏi `url` (giữ phần trước `?`).
* Bỏ entry nếu `name` không kết thúc bằng `.mp4 .avi .mov .mkv .m4v .flv .mpeg .wav`.
* `isPlayable = (file_type != "0")`.

### 4.2 GET `https://timfshare.com/api/key/data-top` — Trending

Không cần `Authorization` (StreamIX **không** gắn Bearer cho endpoint này, chỉ gắn UA Chrome).

```http
GET https://timfshare.com/api/key/data-top
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) ... Chrome/58.0.3029.110 Safari/537.36
```

Response:
```json
{
  "dataFile": [
    {
      "id":            123,
      "name":          "Tập 01",
      "linkcode":      "ABC123",
      "file_extension":"mp4",
      "size":          534521234,
      "size_display":  "509.78 MB"
    }
  ]
}
```

Build URL: `https://www.fshare.vn/file/<linkcode>`. StreamIX lọc theo extension video giống mục 4.1.

---

## 5. Google Sheet — CSV export công khai

Hai bảng do team biên tập thủ công, share chế độ "Anyone with the link can view" rồi gọi endpoint export CSV của Google Docs.

| Tên | URL CSV | File ID | gid |
|---|---|---|---|
| Gợi ý | `https://docs.google.com/spreadsheets/d/1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0/export?format=csv&gid=1370676663` | `1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0` | `1370676663` |
| Xu hướng (Fvideo) — cũng dùng làm "gợi ý mặc định" cho module Tìm kiếm | `https://docs.google.com/spreadsheets/d/1r2e9W81ostp9PmosITlajdzKK4oY275Q/export?format=csv&gid=936805767` | `1r2e9W81ostp9PmosITlajdzKK4oY275Q` | `936805767` |

Header request: `User-Agent: Streamix/1.0` (chỉ vậy, không cần auth).

Định dạng CSV:
* Cột 0: tên hiển thị (label).
* Cột 1: URL Fshare (file hoặc folder).
* Header (dòng 1) bị bỏ qua nếu cell 0 chứa `"tên"` hoặc cell 1 chứa `"link"`.
* StreamIX hỗ trợ quoted-field `""` escape (CSV chuẩn).

Quy tắc lọc/parsing:
* `normalizeFshareUrl(url)` để chuẩn hoá http→https, bỏ tracking, thêm `www.`
* Bỏ row nếu URL không phải `fshare.vn/file/...` hoặc `fshare.vn/folder/...`
* `isPlayable = !isFolderUrl(url)`
* Tối đa `kSuggestMaxRows` rows được nạp.

TTL cache phía client: `kSuggestCacheTtlMs` (cấu hình ở `application/data_loading/RetryDelays.h`).

---

## 6. Google Analytics 4 — Measurement Protocol

Endpoint:
* Production: `https://www.google-analytics.com/mp/collect`
* Validation (debug, không ghi vào báo cáo): `https://www.google-analytics.com/debug/mp/collect`

Query string:
```
?measurement_id=G-252LMTWNB9&api_secret=<từ env STREAMIX_GA4_API_SECRET>
```

Body JSON (POST, `Content-Type: application/json`):
```json
{
  "client_id": "<UUID v4 lưu trong QSettings analytics/clientId>",
  "user_id":   "<sha256(email_lowercase)>",
  "non_personalized_ads": true,
  "events": [
    {
      "name": "streamix_app_open",
      "params": {
        "app_version": "1.0",
        "platform":    "windows",
        "session_id":  1234567890,
        "session_number": 7,
        "engagement_time_msec": 1
      }
    }
  ]
}
```

Thành công: HTTP `204 No Content` (production) hoặc `200 + JSON` (validation).

API secret **không** nằm trong source code — nạp qua biến môi trường `STREAMIX_GA4_API_SECRET` lúc build/run, hoặc nhúng compile-time qua macro `STREAMIX_GA4_API_SECRET_EMBEDDED` (CMake `-DSTREAMIX_GA4_API_SECRET=...`).

---

## 7. Sample code tái sử dụng

### 7.1 Python (requests) — login + lấy direct URL + list folder

```python
import requests, urllib.parse, re, csv, io

APP_KEY    = "tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch"
USER_AGENT = "FshareVideoDesktop_23052023"
SHARE_ID   = "8805984"
BASE       = "https://api.fshare.vn"

def login(email, password):
    body = ('{"app_key":"%s","user_email":"%s","password":"%s"}'
            % (APP_KEY, email, password)).encode("utf-8")
    r = requests.post(f"{BASE}/api/user/login", data=body,
        headers={"User-Agent": USER_AGENT, "cache-control": "no-cache"})
    r.raise_for_status()
    j = r.json()
    return j["token"], j["session_id"]

def get_direct_url(token, session_id, fshare_url):
    sep = "&" if "?" in fshare_url else "?"
    url = fshare_url + f"{sep}share={SHARE_ID}"
    r = requests.post(f"{BASE}/api/session/download",
        json={"zipflag": 0, "url": url, "password": "", "token": token},
        headers={"User-Agent": USER_AGENT,
                 "Cookie": f"session_id={session_id}"})
    j = r.json()
    if j.get("code") == 123:
        raise RuntimeError("File yêu cầu mật khẩu")
    return j.get("location")

def list_folder(token, session_id, folder_url, page=0, limit=100):
    m = re.search(r"fshare\.vn/folder/([^/?#&]+)", folder_url)
    if not m: raise ValueError("URL folder sai")
    canon = f"https://www.fshare.vn/folder/{m.group(1)}"
    r = requests.post(f"{BASE}/api/fileops/getFolderList",
        json={"token": token, "url": canon, "dirOnly": 0,
              "pageIndex": page, "limit": limit},
        headers={"User-Agent": USER_AGENT,
                 "Cookie": f"session_id={session_id}"})
    return r.json()  # array hoặc object có data/items/folders/files
```

### 7.2 Python — Search Timfshare + Trending

```python
TIMFSHARE_BEARER = "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJuYW1lIjoiZnNoYXJlIiwidXVpZCI6IjcxZjU1NjFkMTUiLCJ0eXBlIjoicGFydG5lciIsImV4cGlyZXMiOjAsImV4cGlyZSI6MH0.WBWRKbFf7nJ7gDn1rOgENh1_doPc07MNsKwiKCJg40U"
CHROME_UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
             "(KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36")

def search(keyword):
    q = urllib.parse.quote(keyword.replace(".", " "))
    r = requests.post(
        f"https://api.timfshare.com/v1/string-query-search?query={q}",
        headers={"Authorization": TIMFSHARE_BEARER,
                 "User-Agent": CHROME_UA,
                 "Content-Type": "application/json"})
    return r.json().get("data", [])

def trending():
    r = requests.get("https://timfshare.com/api/key/data-top",
        headers={"User-Agent": CHROME_UA})
    return r.json().get("dataFile", [])
```

### 7.3 Python — Đọc Google Sheet (gợi ý / xu hướng)

```python
GOIY_CSV    = "https://docs.google.com/spreadsheets/d/1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0/export?format=csv&gid=1370676663"
XUHUONG_CSV = "https://docs.google.com/spreadsheets/d/1r2e9W81ostp9PmosITlajdzKK4oY275Q/export?format=csv&gid=936805767"

def fetch_sheet(url):
    r = requests.get(url, headers={"User-Agent": "Streamix/1.0"})
    r.encoding = "utf-8"
    rows = list(csv.reader(io.StringIO(r.text)))
    items = []
    skipped_header = False
    for cols in rows:
        if len(cols) < 2: continue
        if not skipped_header:
            skipped_header = True
            if "tên" in cols[0].lower() or "link" in cols[1].lower():
                continue
        label, fshare_url = cols[0].strip(), cols[1].strip()
        if "fshare.vn/file/" in fshare_url or "fshare.vn/folder/" in fshare_url:
            items.append({"label": label,
                          "url": fshare_url,
                          "playable": "/file/" in fshare_url})
    return items
```

### 7.4 cURL nhanh

```bash
# Login
curl -X POST 'https://api.fshare.vn/api/user/login' \
  -H 'User-Agent: FshareVideoDesktop_23052023' \
  -H 'cache-control: no-cache' \
  --data '{"app_key":"tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch","user_email":"YOU","password":"PWD"}'

# Get direct URL
curl -X POST 'https://api.fshare.vn/api/session/download' \
  -H 'User-Agent: FshareVideoDesktop_23052023' \
  -H 'Content-Type: application/json' \
  -H 'Cookie: session_id=<SESSION>' \
  --data '{"zipflag":0,"url":"https://www.fshare.vn/file/ABC?share=8805984","password":"","token":"<TOKEN>"}'

# Search Timfshare
curl -X POST 'https://api.timfshare.com/v1/string-query-search?query=phim%20tinh%20cam' \
  -H 'Authorization: Bearer eyJ0eXAi...g40U' \
  -H 'Content-Type: application/json' \
  -H 'User-Agent: Mozilla/5.0 ... Chrome/58 ...'

# Sheet CSV
curl -L 'https://docs.google.com/spreadsheets/d/1wDBrW0D7KhrLJW8HU1aaIwDLzrFNkeB0/export?format=csv&gid=1370676663'
```

---

## 8. Best practices đã áp dụng trong StreamIX

* **Phiên hết hạn**: bất kỳ endpoint nào trả `status==201` hoặc `code==201` → xóa token+session, force re-login.
* **Cache**:
  * `fileops/get` (info file): TTL 3 ngày, hard-cap 10000 entry, prune mỗi 50 ghi.
  * `fileops/getFolderList`: TTL 15 phút, key gồm canonical URL + pageIndex + limit, đặt riêng theo email user (per-user bucket).
  * Sheet CSV: TTL `kSuggestCacheTtlMs` (RAM + đĩa).
* **Race condition**: dùng `requestSeq` (monotonically increasing) đính kèm `QNetworkReply::setProperty` → caller drop reply cũ khi user đổi context.
* **SSL**: load thêm `fshare_chain.pem` cạnh exe vào CA list khi khởi động (nếu hệ thống thiếu cert chain).
* **Sanitize input search**: cắt 256 ký tự, bỏ control + zero-width, chặn pattern `<script` / `javascript:` / `vbscript:` / `<iframe` / `data:text/html` / `on*=`.
* **Ẩn Bearer**: không log header `Authorization` của Timfshare ngay cả ở debug build.
* **Referral**: mọi request `session/download` đều gắn `share=8805984` để giữ tương thích chương trình partner.

---

## 9. Checklist khi chuyển sang dự án khác

1. [ ] Sao chép block "Các khóa & secret" (mục 2) vào `.env` của dự án mới.
2. [ ] Set header `User-Agent: FshareVideoDesktop_23052023` cho **mọi** request tới `api.fshare.vn` (sai sẽ bị từ chối).
3. [ ] Lưu `token` (body) + `session_id` (cookie) sau login; gắn lại ở mọi request file/download.
4. [ ] Canonicalize URL folder về dạng `https://www.fshare.vn/folder/<linkcode>` trước khi gọi `getFolderList`.
5. [ ] Gắn `?share=8805984` (hoặc `&share=...`) vào URL trước khi gọi `session/download`.
6. [ ] Coi `code==201` / `status==201` là phiên hết hạn và xử lý re-login.
7. [ ] Với Timfshare: gửi POST body rỗng + đầy đủ Bearer + UA Chrome.
8. [ ] Với Google Sheet: tải CSV bằng `?format=csv&gid=<gid>`, parse 2 cột (label, fshareUrl), bỏ header dòng 1 nếu trùng pattern.
9. [ ] Với GA4: lấy `STREAMIX_GA4_API_SECRET` từ env hoặc CMake — KHÔNG commit secret vào repo.
10. [ ] Ưu tiên cache theo email user (bucket riêng) nếu app multi-account; xoá bucket khi logout.

---

## 10. Phụ lục — Map source code

| Chức năng | File:Line |
|---|---|
| Khai báo URL/Key Fshare | `FshareClient.h:103-111` |
| Login | `FshareClient.cpp:319-352` |
| User info | `FshareClient.cpp:404-556` |
| Direct URL | `FshareClient.cpp:647-686, 958-1003` |
| Folder list | `FshareClient.cpp:1063-1296` |
| File info | `FshareClient.cpp:622-815` |
| Top page (HTML) | `FshareClient.cpp:1005-1061` |
| Timfshare Bearer + helpers | `SearchService.cpp:19, 40-49` |
| Search Timfshare | `SearchService.cpp:185-316` |
| Trending data-top | `SearchService.cpp:318-404` |
| Google Sheet CSV URLs | `MainWindow.cpp:318-329` |
| Sheet CSV fetch + parse | `MainWindow.cpp:3406-3619` |
| GA4 client | `infra/analytics/Ga4MeasurementClient.cpp` |
| Build secret env | `build-secret.local.env.bat.example` |
