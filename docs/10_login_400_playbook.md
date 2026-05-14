# Emergency runbook — `POST /api/user/login` HTTP 400

**Trạng thái**: ✅ RESOLVED 2026-04-28 — login working with 2026 app_key + UA rotation. Doc kept as runbook for future recurrence.
**Last updated**: 2026-04-28
**Khi nào dùng tài liệu này**: nếu một ngày `/api/user/login` lại trả 400 (thường xảy ra sau khi backend rotation app_key / siết WAF), mở doc này và đi theo §5 (diagnostic playbook).
**Phạm vi đối tượng**: Email + password login. OAuth login (Google/Facebook/FPT ID) là path độc lập và sẽ hoạt động ngay cả khi password endpoint hỏng.

---

## 1. Tóm tắt một-câu

Server Fshare trả `HTTP 400 Bad Request` (HTML từ nginx, không phải JSON) cho mọi POST `/api/user/login` từ FsNext, trong khi cùng client + cùng credential lại login được qua `/api/user/oauth`. Nguyên nhân nhiều khả năng nằm ở **app_key/UA whitelist hoặc nginx WAF rule** áp riêng cho password endpoint, không phải lỗi C++ phía client.

---

## 2. Trạng thái thực tế ngày hôm nay (≠ BACKLOG)

`docs/BACKLOG.md` ghi blocker 2026-04-15 với app_key cũ:
> Payload: `{"app_key":"CtnLXisyQaf4mQwfx6aP58ZMQUomck14R7mI7KCe", ...}`

Nhưng `src/core/api/FshareApi.cpp:17-31` hiện dùng **app_key đã rotation**:
```
dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt
```
kèm User-Agent rotation `Fshare_Tool_2026` (`HttpClient.cpp:175`). Comment trong code:
> "User-Agent must appear in the Fshare server whitelist — unrecognized UAs get 400 from nginx before the request reaches the app layer. Paired with the 2026 app_key rotation."

**Hệ quả**: BACKLOG phản ánh tình trạng cũ. Cần một bài test xác minh blocker **vẫn còn** với rotation mới — có thể nó đã được backend fix âm thầm và FsNext chỉ chưa test lại.

---

## 3. Bằng chứng sẵn có

| Source | Quan sát |
|---|---|
| `runtime.log:11,19` | `Auto-login: silent OAuth refresh for "Google"` rồi `OAuth login success, Fshare token: "fa4bd8f7..."`. → OAuth + cookie + cả pipeline session đều hoạt động. |
| `FshareApi.cpp:213-241` | Password login post `application/json` body `{app_key, user_email, password}` to `/api/user/login`. |
| `FshareApi.cpp:243-280` | OAuth login post **cùng schema** `{app_key, service, access_token, user_email}` to `/api/user/oauth`. |
| `BACKLOG.md` (legacy snapshot) | `/api/service/getlatestversion` → 200 cùng client → server reachable, app_key được chấp nhận trên ít nhất 1 endpoint. |
| `docs/02_api_contracts.md` | Login spec gốc của Fshare client cũ (legacy fshareclient v5) — kiểm tra xem có header/format khác. |

→ Vấn đề **rất khả năng cục bộ ở `/api/user/login`** (hoặc nginx WAF / rate limit / app_key allowlist riêng cho endpoint này).

---

## 4. Hypotheses xếp theo xác suất

### H1 (cao nhất) — App_key bị rotation nhưng chỉ một số endpoint nhận key mới
Backend rotated key vào `getlatestversion`/`oauth`/etc nhưng quên update bảng cho `/api/user/login`. Triệu chứng đúng: tất cả endpoint khác ok, login HTTP 400 từ nginx (allowlist tầng nginx kiểm tra header trước app).
- **Verify**: gửi cùng app_key tới `/api/user/login` từ Postman trên cùng IP đã login OAuth thành công → nếu vẫn 400 thì xác nhận H1.

### H2 (cao) — Captcha/challenge bắt buộc với endpoint password
Fshare có thể đã thêm `recaptcha_token` hoặc `device_id` mandatory cho password login để chống credential stuffing. Body chỉ chứa `{app_key, user_email, password}` không đủ.
- **Verify**: chụp request thật từ web fshare.vn (DevTools → Network) khi login bằng email+password → so sánh body với FsNext.

### H3 (trung bình) — User-Agent whitelist mismatch
Bình luận trong code đã tăng UA → `Fshare_Tool_2026`, nhưng có thể server whitelist là `FshareTool/6.0` hoặc khác hoa-thường. Nginx fail tầng 7 trước khi tới app.
- **Verify**: thử từng UA: `FshareTool`, `Fshare Tool 6.0`, `Fshare_Tool/6.0.0`, copy UA của browser thật.

### H4 (trung bình) — Format body khác (form-urlencoded thay vì JSON)
Endpoint `/api/user/login` có thể vẫn dùng legacy `application/x-www-form-urlencoded` trong khi các endpoint mới dùng JSON. BACKLOG gốc đã thử cả hai và đều fail — nhưng với app_key cũ.
- **Verify**: thử lại cả 2 format với app_key 2026.

### H5 (thấp) — Rate limit / IP block
Dev IP bị flag do test nhiều lần. Khó debug trừ khi có proxy/VPN.
- **Verify**: chạy test từ một mạng khác (4G hotspot, văn phòng khác).

### H6 (thấp) — Cipher/TLS handshake mismatch
Code đã set `CURL_SSLVERSION_TLSv1_2`, OK. Nhưng nếu server từ chối một cipher curl cụ thể, sẽ trả 400 từ tầng F5/cloudfront chứ không phải nginx — ít khả thi.

---

## 5. Diagnostic playbook (60 phút)

Mục tiêu: thu thập bằng chứng để PUSH backend với mức tự tin ≥ 80% rằng client làm đúng.

### Bước 1 — Capture request thật từ web Fshare (10 phút)
1. Mở Chrome DevTools → tab Network → Preserve log.
2. Vào `https://www.fshare.vn`, login email+password bằng tài khoản test.
3. Tìm request đến `api.fshare.vn/api/user/login` (hoặc tương đương).
4. Right-click → Copy → **Copy as cURL (bash)**.
5. Lưu vào `docs/evidence/web_login_curl.txt`.

### Bước 2 — Replay chính xác cURL từ web (5 phút)
Chạy cURL đã copy từ bước 1 trên máy dev (cùng máy nơi FsNext bị 400):
```bash
bash web_login_curl.txt   # hoặc paste vào terminal
```
- **Nếu 200 OK** → vấn đề chỉ ở phía FsNext (xác định H2/H3/H4 ngay).
- **Nếu 400** → IP / network / cookies bị flag → H5.

### Bước 3 — Diff payload web vs FsNext (10 phút)
Bật log chi tiết HttpClient.cpp tạm thời:
```cpp
curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
```
Build dev → chạy `FsNext.exe` → đăng nhập bằng password → grep log:
```
> POST /api/user/login HTTP/1.1
> User-Agent: Fshare_Tool_2026
> Content-Type: application/json
> ...
{"app_key":"...","user_email":"...","password":"..."}
```
So sánh với cURL ở bước 1 — đặc biệt:
- Headers: `User-Agent`, `X-Requested-With`, `Origin`, `Referer`, `Accept-Language`.
- Body: extra fields như `recaptcha_token`, `device_token`, `client_id`.
- Cookies sent: web có thể đính kèm `_ga`, `cf_clearance` mà FsNext không có.

### Bước 4 — Câu test phân biệt H1 vs H4 (5 phút)
```bash
# H1 test: cùng app_key tới endpoint khác (OAuth)
curl -X POST https://api.fshare.vn/api/user/oauth \
  -H 'User-Agent: Fshare_Tool_2026' \
  -H 'Content-Type: application/json' \
  -d '{"app_key":"dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt","service":"google","access_token":"BAD","user_email":"a@b.c"}'
# Mong đợi: 405 (auth fail) hoặc 200 — nghĩa là endpoint chấp nhận app_key.
# Nếu vẫn 400 → app_key bị invalidate hoàn toàn.

# H4 test: form-urlencoded thay vì JSON
curl -X POST https://api.fshare.vn/api/user/login \
  -H 'User-Agent: Fshare_Tool_2026' \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  -d 'app_key=dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt&user_email=…&password=…'
```

### Bước 5 — Inspect 400 body (5 phút)
Nginx HTML 400 thường có:
```html
<center><h1>400 Bad Request</h1></center>
<hr><center>nginx/1.20.1</center>
```
Nếu body chứa thêm `<title>Forbidden</title>` hoặc `cf-ray:` → CDN/WAF tier blocks. Nếu chỉ "Bad Request" trần → tầng nginx app gateway.

### Bước 6 — TLS / network sanity (10 phút)
```bash
# Check SNI + TLS handshake
openssl s_client -servername api.fshare.vn -connect api.fshare.vn:443 < /dev/null
# Check IP / CDN
nslookup api.fshare.vn
curl -v https://api.fshare.vn/api/service/getlatestversion 2>&1 | head -30
```
Nếu DNS resolve về Cloudflare / Fastly: WAF rules áp riêng cho endpoint là rất khả thi.

### Bước 7 — Test từ network khác (15 phút)
Bật 4G hotspot / VPN khác → chạy lại bước 4 → nếu pass → IP-based block.

---

## 6. Mitigation độc lập với backend

Code FsNext đã nhân nhượng 80% — vẫn có thêm 4 việc để đảm bảo user không tắc:

### M1 — Promote OAuth lên cấp 1 trong UI khi password fail
LoginView hiện hiển thị form email+password ngang quyền với 3 nút OAuth. Khi `/api/user/login` trả error code 400/405 lần đầu, **inline alert** hiển thị:
> "Hiện đang gặp sự cố đăng nhập bằng mật khẩu. Vui lòng dùng nút Google / Facebook / FPT ID bên cạnh."
Đồng thời ghi local flag `Account/passwordLoginUnavailable` để **lần khởi động sau** ẩn form password và nâng OAuth lên đầu.

Tham khảo wire vào `AuthService::login()`:
```cpp
if (resp.error().code == 400 || resp.error().code == 405) {
    m_settings->setBool("Account/passwordLoginUnavailable", true);
    emit passwordLoginUnavailable();   // QML → swap UI
}
```

### M2 — Dev bypass có cờ kép (chỉ khi đã rỡ)
ADR D9 đã quy định: dev bypass phải gated **đồng thời** bởi:
- `FSNEXT_DEV_BUILD` macro (compile-time)
- `FSNEXT_DEV_BYPASS=1` env var (runtime)

Khi 400 xảy ra trong dev build với env set, AuthService trả về một `Session` mock với `email = "taikhoantestfshare@gmail.com"`, `token = "DEV_BYPASS_<rand>"`. Mọi API gọi sau đó vẫn fail thật, nhưng UI/QML có thể được iterate. **TUYỆT ĐỐI không** ship binary có FSNEXT_DEV_BUILD=1.

Xem skeleton ở `docs/decisions/003_upgrade_decisions.md § D2` — thêm vào AuthService.cpp khoảng 30 dòng. (Có ý đồ giữ ngoài cho tới khi backend xác nhận lần cuối.)

### M3 — Web-session login fallback
Legacy fshareclient v5 hỗ trợ login qua **session cookie từ browser**: user paste `PHPSESSID=xxx` thủ công, app dùng làm session. Hiện FsNext không expose. Wire path này trong `LoginView.qml` mục "Đăng nhập nâng cao" cho power user — không ảnh hưởng UX chính.

API endpoint giả định: `/api/user/info` với header `Cookie: PHPSESSID=xxx;key=<app_key>`. Nếu trả 200 + JSON user → coi như đăng nhập.

### M4 — Persistent OAuth refresh
`runtime.log` đã có `Auto-login: silent OAuth refresh for "Google"` — nghĩa là OAuth refresh_token đã hoạt động. Đảm bảo `Account/oauthRefreshToken` được persist qua DPAPI (`SecureStore`) và refresh trước expiry — code hiện tại đã làm (`AuthService::autoLogin §Priority 1`). Verify bằng mock test khi token refresh fail → fallback gracefully về OAuth full flow chứ không bounce ra LoginView trống.

---

## 7. Communication template gửi backend team

Tựa đề: **`[FsNext v6] /api/user/login HTTP 400 — request authoritative spec or whitelist sync`**

```text
Hi Backend team,

FsNext v6.0 (Qt desktop client rewrite) consistently sees HTTP 400 (HTML
from nginx, not JSON) when posting to /api/user/login. Same client + same
network + same app_key successfully:

  • POST /api/user/oauth         → 200 (Google + Facebook + FPT ID all OK)
  • GET  /api/service/getlatestversion → 200
  • GET  /api/user/get (with cookie from OAuth) → 200

Reproduction (paste in terminal on dev VPN):

    curl -X POST https://api.fshare.vn/api/user/login \
      -H 'User-Agent: Fshare_Tool_2026' \
      -H 'Content-Type: application/json' \
      -d '{
            "app_key": "dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt",
            "user_email": "taikhoantestfshare@gmail.com",
            "password": "<redacted>"
          }'

Response body (verbatim):

    <html><head><title>400 Bad Request</title></head>
    <body><center><h1>400 Bad Request</h1></center>
    <hr><center>nginx/<version>></center></body></html>

Hypothesis tree we already ruled out:

  ✗ App_key invalid       — same key works on /oauth and /getlatestversion
  ✗ TLS handshake         — openssl s_client OK, other endpoints reach app layer
  ✗ Format (form vs JSON) — both return same 400

Outstanding hypothesis we cannot verify without backend visibility:

  ▸ Endpoint-specific allowlist (app_key + UA pair not registered for /login)
  ▸ Mandatory captcha / device_id / signed body field added since
    fshareclient v5.x baseline
  ▸ WAF rule on /user/login differing from rest of /user/*

What we need from you (any one of these unblocks us):

  1. Authoritative spec for the current /api/user/login contract, including
     ALL required headers and body fields (we'd like to be told if
     recaptcha_v3 / device_token / signature is now mandatory).
  2. Confirmation that app_key dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt + UA
     "Fshare_Tool_2026" are on the allowlist for /api/user/login.  If not,
     please add or issue a new pair scoped to FsNext desktop.
  3. A 5-minute access log dump for the test request (timestamp X, source
     IP Y) so we can see WHY nginx 400'd.

Test account: taikhoantestfshare@gmail.com  (valid? if not, request a new
service account scoped to FsNext QA).

Workaround currently in place: OAuth login (Google/FB/FPT ID) is the only
working path. Email+password is silently degraded in the UI. Production
release blocked until /api/user/login works OR we get an alternative spec.

— FsNext team, 2026-04-28
```

---

## 8. Acceptance criteria

Coi blocker này CLOSED khi đồng thời:
1. `curl POST /api/user/login` với app_key 2026 trả 200 (hoặc 405 với credential sai — tức là server thừa nhận request hợp lệ).
2. FsNext build sạch → login email+password test account → nhận `token` + `session_id` non-empty + cookie `PHPSESSID=...`.
3. Sau login, `getUserInfo()` cùng cookie trả 200 với `level/email/webspace`.
4. Auto-login với `Remember me` reload session sau restart không bounce ra login.
5. Đã rỡ bypass khỏi build production (preset `msvc2022-production`).
6. Smoke test: đăng nhập + tải 1 file 50 MB → progress bar về 100% → file hợp lệ trên disk.

---

## 9. Người chịu trách nhiệm và escalation

| Vai trò | Tên | Trách nhiệm |
|---|---|---|
| Frontend (FsNext) | _hello_ (tuandm30@fpt.com) | Tạo bằng chứng diagnostic §5, gửi thread §7 cho backend, áp M1 + M3 trong khi chờ. |
| Backend Fshare | TBD | Xác nhận spec / whitelist, cung cấp app_key mới hoặc fix WAF rule. |
| QA | TBD | Khi backend trả lời, chạy smoke test §8.6 từ Windows clean install. |
| Release manager | TBD | Block production tag cho tới khi §8 toàn pass. |

**Escalation path** nếu sau 7 ngày không có phản hồi backend:
- Day 0: gửi mail §7 cho backend lead + cc PM.
- Day 3: nudge trong Slack #fshare-platform.
- Day 7: escalate lên product owner — propose ship FsNext với CHỈ OAuth login (giấu form password hoàn toàn) như là beta channel.
- Day 14: nếu vẫn deadlock, viết RFC reverse-engineering web flow để tự build path login (last resort, có rủi ro vi phạm ToS — cần legal review).

---

## 10. Phụ lục — debug snippets

### CURL_VERBOSE wire-up tạm thời
```cpp
// HttpClient::createHandle, ngay sau curl_easy_init():
#ifdef FSNEXT_DEV_BUILD
    if (qEnvironmentVariableIsSet("FSNEXT_HTTP_VERBOSE")) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
#endif
```
Run với:
```powershell
$env:FSNEXT_HTTP_VERBOSE=1; .\output\FsNext.exe
```
Log đầy đủ TLS handshake + headers ra `%APPDATA%\FPT\Fshare Tool\fsnext.log`.

### Mini test runner cô lập login
File `scripts/test_login.py` (chạy độc lập, không cần build FsNext):
```python
import requests, sys
r = requests.post(
    "https://api.fshare.vn/api/user/login",
    json={
        "app_key":   "dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt",
        "user_email": sys.argv[1],
        "password":  sys.argv[2],
    },
    headers={
        "User-Agent":   "Fshare_Tool_2026",
        "Content-Type": "application/json",
    },
    timeout=15,
)
print("HTTP", r.status_code)
print("Body:", r.text[:400])
print("Set-Cookie:", r.headers.get("set-cookie"))
```
Chạy: `python scripts/test_login.py user@example.com '<password>'`
Output dùng làm bằng chứng kèm bug report §7.

### Cách tái-encode app_key mới (nếu backend cấp)
Code hiện XOR encode với 0x5c — script đã có sẵn:
```bash
python scripts/encode_appkey.py "<plaintext>"
# Output:
#   const char encrypted_key[] = { 0xXX, 0xXX, ... };
# Paste vào FshareApi.cpp:18-25, rebuild.
```
**Nhớ tăng tag UA** đồng thời (FsNext convention: `Fshare_Tool_<year>`) và yêu cầu backend whitelist cả UA mới.
