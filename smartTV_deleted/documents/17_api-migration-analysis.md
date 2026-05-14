---
title: 17 — Phân tích migration API: Open API Gateway → Legacy + Timfshare + Sheets
date: 2026-05-05
status: 📊 Analysis — chờ Project Owner quyết định migrate hay giữ
inputs: streamix-tv/documents/API_REFERENCE_FOR_REUSE.md (legacy API thực tế từ StreamIX desktop)
        13_implementation-spec-v1.md §3 (current Open API Gateway spec)
---

# 17. API Migration Analysis

## 0. TL;DR — Verdict

**🟢 KHUYẾN NGHỊ: MIGRATE** sang stack API mới — phù hợp hơn với V1 StreamIX TV vì:

1. ✅ **Solve B1 blocker** — Stream URL endpoint `/api/session/download` đã chạy production (không phải chờ backend ship `v1/session.md`)
2. ✅ **Solve search natively** — Timfshare partner API có sẵn cho search + trending
3. ✅ **Featured content** — Google Sheets cho row "Gợi ý" / "Xu hướng" trên Home (cải thiện UX user mới)
4. ✅ **Battle-tested** — đang chạy production trên StreamIX desktop, không phải đoán shape
5. ✅ **Solve C2** — `/api/user/get` có trường traffic (gateway thiếu)

**Trade-off chấp nhận được**:
- App key `tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch` đã embed trong source desktop StreamIX → không thể keep secret cho TV
- Refactor toàn bộ network layer (~1.5 tuần engineering)

---

## 1. So sánh 2 stack

### 1.1 Bảng đối chiếu

| Aspect | Open API Gateway (đã spec V1.1) | **Legacy + Partner (mới)** |
|--------|--------------------------------|---------------------------|
| **Base URL** | `https://oapi.fshare.vn/api/v1/...` | `https://api.fshare.vn/api/...` (legacy) + 3 host khác |
| **Hosts dùng** | 1 (oapi.fshare.vn) | 4 (api.fshare.vn / www.fshare.vn / api.timfshare.com / docs.google.com) |
| **Auth model** | Bearer JWT + `X-App-Key` header | `session_id` cookie + `token` trong body + `app_key` trong body |
| **Token lifetime** | JWT 1h + refresh_token 7d (rotate) | Single session token, không expire timeout cố định |
| **Refresh** | Authenticator + Mutex single-flight | KHÔNG — phải re-login khi `code: 201` |
| **Response envelope** | `{data, meta?, error?, request_id}` | Flat: `{code, msg, ...payload}` hoặc array |
| **Error codes** | String (`AUTH_INVALID_CREDENTIALS`) | Numeric (200, 405, 471, …) |
| **User-Agent** | `Fshare/androidtv/<ver>` | **Cố định** `FshareVideoDesktop_23052023` (sai → bị từ chối) |
| **App key location** | Header `X-App-Key` | Body field `app_key` (chỉ login) |
| **Stream URL endpoint** | ⏳ **CHƯA có** trong gateway (B1 blocker) | ✅ `POST /api/session/download` production |
| **Search** | `GET /api/v1/files/search` | ✅ `POST api.timfshare.com/v1/string-query-search` (partner) |
| **Trending** | Không có | ✅ `GET timfshare.com/api/key/data-top` |
| **Top public** | Không có | ✅ HTML scraping `https://www.fshare.vn/top` |
| **Featured rows** | Không có | ✅ Google Sheets CSV (Gợi ý + Xu hướng) |
| **Production status** | Pre-production (gateway đang rolling out) | ✅ Production (StreamIX desktop dùng hằng ngày) |
| **Documentation** | OpenAPI 3.0 spec | Reverse-engineered từ desktop code |

### 1.2 Multi-source architecture (mới)

```
Login + Browse + Stream    ──► api.fshare.vn        (legacy Fshare)
Search (full-text)          ──► api.timfshare.com    (partner JWT)
Trending                    ──► timfshare.com        (partner public)
Top/Public                  ──► www.fshare.vn/top    (HTML scraping)
Featured rows               ──► docs.google.com/...  (CSV public)
Telemetry                   ──► google-analytics.com (GA4 MP — optional)
```

→ App phải maintain **5 OkHttpClient** với cấu hình khác nhau, hoặc 1 client + multiple interceptor sets.

---

## 2. Tác động đến codebase hiện tại

### 2.1 Files cần sửa

| Layer | File | Thay đổi |
|-------|------|---------|
| Network | `AppKeyInterceptor.kt` | **DELETE** — không có header X-App-Key. App key chỉ dùng ở body login |
| Network | `AuthInterceptor.kt` | **REWRITE** — chèn `Cookie: session_id=...` + thay JWT bằng session_id |
| Network | `TokenRefreshAuthenticator.kt` | **DELETE** — không có refresh token model |
| Network | `Envelope.kt` (ApiEnvelope) | **REWRITE** — flat envelope thay `{data, meta, error}` |
| Network | `ApiCall.kt` | **REWRITE** — error mapping numeric thay string |
| Network | `AuthApi.kt` | **REWRITE** — login body khác, response shape khác |
| Network | `MeApi.kt` | **REWRITE** — `/api/user/get` flat response với `level`, `email`, etc. |
| Network | `FilesApi.kt` | **REWRITE** — `getFolderList` + `fileops/get` thay `/files` |
| Network | `SessionApi.kt` | ✅ **READY** — `POST /api/session/download` đã đúng spec |
| Network | + new | **ADD** `TimfshareApi.kt` — Search + Trending |
| Network | + new | **ADD** `SheetsApi.kt` — CSV fetch |
| Domain | `User.kt` | **UPDATE** trường: `level/email/webspace_used` thay `id/fullName/storageUsed` |
| Domain | `FileItem.kt` | **UPDATE** type "0"/"1" string mapping |
| Domain | `Session.kt` | **REWRITE** — `(token, sessionId, email)` thay `(accessToken, refreshToken, …)` |
| Data | `AuthStore.kt` | **REWRITE** — lưu sessionId thay refresh token |
| Data | `AuthRepositoryImpl.kt` | **REWRITE** — login flow khác |
| Data | `UserRepositoryImpl.kt` | **REWRITE** — parse flat response với key flexibility |
| Data | `FilesRepositoryImpl.kt` | **REWRITE** — folder list response shape variants |
| Data | `PlayerRepositoryImpl.kt` | **MOSTLY KEEP** — endpoint giống, body khác |
| Data | + new | **ADD** `TimfshareRepository.kt` (search/trending) |
| Data | + new | **ADD** `FeaturedRepository.kt` (Google Sheets CSV) |
| Domain | + new | **ADD** `SearchSource` enum (FshareNative vs Timfshare) |
| Feature | `home/HomeViewModel.kt` | **EXTEND** — thêm row "Gợi ý" + "Xu hướng" |
| Feature | `search/SearchViewModel.kt` | **REWRITE** — gọi Timfshare API thay Fshare /search |
| App | `BuildConfig` | **ADD** `TIMFSHARE_BEARER` build-time inject |
| App | Manifest | **ADD** `usesCleartextTraffic` cho timfshare.com nếu HTTP (check) |

**Tổng**: ~15 files rewrite + ~6 files mới + Manifest update.

### 2.2 Estimate effort

| Task | Days |
|------|------|
| Network layer refactor (interceptors + envelope + ApiCall) | 1.5 |
| AuthApi + AuthRepository + AuthStore migration | 1 |
| MeApi + UserRepository + flat parsing | 0.5 |
| FilesApi + FilesRepository (folder list response variants) | 1 |
| SessionApi (giữ — chỉ đổi base URL + headers) | 0.25 |
| TimfshareApi + TimfshareRepository (search + trending) | 1 |
| SheetsApi + FeaturedRepository (CSV parser) | 0.5 |
| HomeScreen UI thêm 2 row mới (Gợi ý + Xu hướng) | 0.5 |
| Search screen rewire | 0.25 |
| Tests update | 1 |
| Verification + bug fix | 1.5 |
| **Tổng** | **~9 ngày** (~2 tuần) |

---

## 3. Risks khi migrate

### 3.1 Risk mới

| ID | Risk | Severity | Mitigation |
|----|------|----------|------------|
| R-A1 | App key `tdf5ATEa...` exposed → public abuse | Medium | Đã exposed trong desktop source. TV không tệ hơn. Mitigation: rotate khi cần |
| R-A2 | Timfshare JWT `eyJ0eXAi...g40U` partner | Medium | Token vĩnh viễn (`expires=0`). Nếu Timfshare revoke → search/trending fail. Cần partner contact backup |
| R-A3 | Google Sheets fail nếu owner đổi sharing setting | Low | Sheets do team biên tập. Có thể fallback bỏ row |
| R-A4 | HTML scraping `/top` brittle nếu Fshare đổi HTML | Medium | Chấp nhận; có fallback show empty |
| R-A5 | Timfshare DDoS protection từ chối user-agent custom | Low | Đã giải quyết: dùng Chrome UA |
| R-A6 | Cleartext HTTP cho timfshare.com (chưa rõ HTTPS không) | Low | Check thực tế; nếu HTTP → khai `usesCleartextTraffic` cho domain riêng |
| R-A7 | Cache invalidation — sessionId KHÔNG có refresh model → user phải login lại sau N ngày (TTL ko biết) | Medium | Detect HTTP 201 / code 201 → force re-login. UX: hiển thị D7 SessionExpired dialog |

### 3.2 Risk cũ giải quyết / bỏ

- ❌ **B1 Stream URL endpoint chưa có** — GIẢI QUYẾT (endpoint đã production)
- ❌ **C2 /me thiếu traffic field** — GIẢI QUYẾT (`/api/user/get` có `traffic_used`/`traffic_remaining`/`traffic`)
- ❌ **C3 Backend ship refresh + logout** — KHÔNG CÒN ÁP DỤNG (legacy không có refresh)
- ⚠️ **R-S2 Update adoption thấp (Phương án A)** — KHÔNG ẢNH HƯỞNG (vẫn vậy)

---

## 4. Compatibility với Spec đã chốt

### 4.1 Vẫn giữ

- ✅ Brand StreamIX TV + package `vn.streamix.tv` (D-1, D-4)
- ✅ Cắt Login QR/OAuth, Downloads, In-app update (D-2, D-3.5)
- ✅ Phương án A manual distribution (D-3.5)
- ✅ minSdk 21, target 34, Compose-TV
- ✅ Hilt DI + MVVM + Clean Architecture
- ✅ Toàn bộ UI (Splash/Login/Home/Browse/Detail/Player/Search/Settings)
- ✅ Toàn bộ design tokens, components, dialogs, overlays

### 4.2 Đổi

- 🔄 §3 API contracts trong `13` — REWRITE hoàn toàn
- 🔄 §4 Common patterns (interceptor, envelope, error mapping) — REWRITE
- 🔄 Login flow (S1) — vẫn email/password nhưng response shape khác
- 🔄 Home (S2) — **THÊM** row "Gợi ý" + row "Xu hướng" từ Sheets/Timfshare
- 🔄 Search (S12) — endpoint khác (Timfshare), filter logic khác
- ⚠️ Player (S5) — endpoint URL stream giờ thực sự dùng được

### 4.3 Đề xuất scope V1.0 với API mới

Home screen: **5 row** (thay vì 3 row đã chốt sau cuts):

1. Tiếp tục xem (Continue Watching — Room local)
2. **Gợi ý** (Google Sheet `1wDBrW0D...`)
3. **Xu hướng** (Timfshare data-top + Google Sheet `1r2e9W81...`)
4. File gần đây (Fshare API listing)
5. Thư mục (folder root)

→ V1 trở nên RICH content hơn — phù hợp scope "streaming TV experience", không chỉ file browser.

---

## 5. Quy trình migrate

Nếu Owner approve, đề xuất 4 wave migration:

### Wave M1 — Network layer (2 ngày)
- Rewrite `AppKeyInterceptor` → `LegacyHeaderInterceptor` (UA + cache-control)
- Rewrite `AuthInterceptor` → cookie session_id
- Delete `TokenRefreshAuthenticator`
- Rewrite `ApiEnvelope<T>` → flat decode helpers
- Rewrite `apiCall()` với error mapping numeric

### Wave M2 — Fshare APIs + repositories (2 ngày)
- Rewrite AuthApi/MeApi/FilesApi/SessionApi với shape mới
- Update Repository impls
- Rewrite AuthStore với sessionId model
- Update Hilt modules

### Wave M3 — Partner & Sheets integrations (2 ngày)
- New `TimfshareApi` + `TimfshareRepository`
- New `SheetsApi` + CSV parser + `FeaturedRepository`
- Multi-OkHttpClient configuration (different UAs/auths)
- Add Timfshare Bearer + Sheet IDs vào BuildConfig

### Wave M4 — Feature integration + tests (3 ngày)
- HomeScreen: thêm row Gợi ý + Xu hướng
- SearchScreen: rewire qua TimfshareApi
- LoginScreen: update error mapping
- Tests update
- Build + smoke test

**Tổng**: ~9 ngày (2 tuần). Engineering có thể start NGAY sau khi Owner approve.

---

## 6. Câu hỏi cần Owner trả lời trước khi migrate

| # | Câu hỏi | Tại sao quan trọng |
|---|---------|-------------------|
| Q1 | App key `tdf5ATEa3VUuGeSyw98D1YLSKvFr8pch` đã exposed qua StreamIX desktop. TV được phép dùng cùng key? | Quyết định: rotate hay reuse |
| Q2 | Timfshare partner OK với TV app? Có cần ký lại agreement? | Pháp lý + tránh token revoke |
| Q3 | Google Sheets sharing có giữ "Anyone with link" mãi không? | Featured rows phụ thuộc 100% vào điều kiện này |
| Q4 | Có cần GA4 telemetry cho TV không? | Nếu CÓ — cần `STREAMIX_GA4_API_SECRET` env |
| Q5 | UA `FshareVideoDesktop_23052023` có ổn cho TV (đăng ký TV, không phải desktop) không? | Backend Fshare có thể flag UA mismatch |
| Q6 | Refresh token bỏ → user phải re-login mỗi N ngày. Có chấp nhận UX này không? | TTL session legacy không biết, có thể là 30d, 90d, hoặc không expire |

---

## 7. Khuyến nghị cuối

### Nếu Owner approve migration

1. Set up appointment với Timfshare partner để confirm Q2
2. Verify với DevOps/Backend Q1 + Q5
3. Approve effort 2 tuần engineering (~9 ngày)
4. Engineering chạy 4 wave M1→M4
5. **Bonus**: 5 row Home thay 3 row → V1 product RICH hơn, đáng ship hơn

### Nếu Owner KHÔNG migrate

1. Giữ Open API Gateway plan
2. PM phải push backend ship `v1/session.md` (Stream URL) **trước Tuần 6** (B1 blocker)
3. Bỏ row Featured + chấp nhận Search V2
4. /me thiếu traffic field → bỏ ô traffic ở S7a

### Khuyến nghị cá nhân: **MIGRATE**

Lý do:
- Backend Open API Gateway chưa sẵn sàng (B1 chưa ship sau nhiều tuần)
- Legacy + Timfshare + Sheets là combo đã tested ở production StreamIX desktop
- Content rich hơn (5 row Home) → V1 đáng ship hơn
- Effort 2 tuần là chấp nhận được, vẫn trong lộ trình 11.5 tuần

Khuyến nghị Owner:
1. Reply `OK` → engineering bắt đầu Wave M1
2. Hoặc: trả lời 6 câu hỏi Q1-Q6 trước, sau đó decide
3. Hoặc: keep Open API Gateway và forge ahead

— Hết phân tích —
