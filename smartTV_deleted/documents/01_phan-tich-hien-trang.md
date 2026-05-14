---
title: 01 — Phân tích hiện trạng & quyết định cắt/giữ tính năng
date: 2026-05-04
---

# 01. Phân tích hiện trạng FsNext desktop và quyết định tính năng cho bản TV

## 1.1 Hồ sơ codebase hiện tại

| Mục | Giá trị |
|-----|---------|
| Tên dự án | FsNext (Fshare Tool v6.0) |
| Stack | Qt 6.8, QML, C++17, CMake, vcpkg, MSVC 2022 |
| Pattern | MVVM + Clean Architecture (5 layer: View → ViewModel → Service → Repository → Data) |
| Engine HTTP | libcurl với connection pooling (`curl_share_init`, DNS/TLS/cookie share) |
| Engine transfer | Multi-segment download (4 segment mặc định), chunked upload (20MB) |
| Lines of code | ~30k C++ + ~10k QML (ước lượng từ cây thư mục) |
| Build target | Windows desktop x64 |
| Bản mới nhất | Upgrade pass 2026-04-28 (ADR 003 — single design system Aurora) |

## 1.2 Mục module hiện tại (10 module)

Trích từ `docs/01_features.md`:

1. **Authentication** — email/password, OAuth (Google/Facebook/FPT ID), session, user info
2. **Download** — multi-segment, queue, history, video preview qua app ngoài
3. **Upload** — chunked, FTP, history, queue
4. **File Management** — folder tree, file ops (rename/delete/move/copy), security, share
5. **User Info & Statistics** — profile, VIP, storage pie chart
6. **Settings** — language, proxy, threads, folders
7. **RSS Auto-Download** — feed → episode detect → auto queue
8. **Subtitle Search** — online search, save .srt
9. **System Integration** — Chrome extension, single-instance, system tray, drag&drop, update checker
10. **Analytics & Logging** — Google Analytics, Fshare logger

## 1.3 Đặc điểm môi trường Smart TV (khác desktop)

| Khía cạnh | Desktop | Smart TV (Android TV) |
|-----------|---------|----------------------|
| Đầu vào | Chuột + bàn phím | Remote D-pad (lên/xuống/trái/phải/OK/back/home), đôi khi voice |
| Khoảng cách nhìn | ~50 cm | ~3 m ("10-foot UI") |
| Resolution | 1920×1080 / 4K, 100% scale | 1920×1080 hoặc 4K, overscan vùng 5% an toàn |
| CPU/GPU | x64, mạnh | ARMv8, RAM 1.5–4 GB, GPU yếu hơn 5–10× |
| Storage | HDD/SSD GB–TB | eMMC 8–32 GB, thường còn trống <4 GB |
| Network | Ethernet/Wi-Fi ổn định | Wi-Fi nhà, có thể chập chờn |
| Multitasking | Có | Hầu như không, app chiếm full screen |
| Filesystem | NTFS đầy đủ | Sandbox `/data/data/<pkg>/` + USB OTG (read/write hạn chế) |
| Phân phối | EXE installer | APK qua Play Store hoặc sideload |
| Vòng đời session | Người dùng tự đóng | OS có thể kill khi switch app |

## 1.4 Quyết định tính năng cho bản TV

Nguyên tắc: TV là thiết bị **tiêu thụ media**, không phải workstation. Loại bỏ tính năng tạo nội dung (upload, file ops nặng), giữ tính năng tiêu thụ (xem, tải về, browse).

### 1.4.1 GIỮ và NÂNG CẤP

| Module | Hiện trạng desktop | Trên TV |
|--------|-------------------|---------|
| Auth — email/password | Form input | Form + bàn phím trên màn hình + nhớ session lâu (30 ngày) |
| Auth — OAuth | QWebEngineView popup | Qua **device-flow OAuth** (mã hiển thị trên TV, user mở phone vào URL nhập mã) |
| Auth — QR login | Không có | **MỚI**: Hiển thị QR, user scan bằng app Fshare mobile để login (UX TV phổ biến) |
| File browse | Folder tree desktop | List grid 16:9 thumbnail, focus rõ ràng, lazy load |
| Download (về thiết bị) | Vào folder bất kỳ | Vào USB OTG hoặc internal storage; cần cảnh báo dung lượng |
| **Stream/Play media** | Mở app ngoài | **NHÚNG ExoPlayer** ngay trong app — yêu cầu cốt lõi |
| Settings | Đầy đủ | Rút gọn: tài khoản, ngôn ngữ, chất lượng stream mặc định, vị trí lưu |
| Update | Check + download installer | **In-app update**: tải APK + tự cài qua `PackageInstaller` |
| User info | Pie chart, VIP | Card đơn giản, nhấn mạnh trạng thái VIP và còn bao nhiêu traffic |

### 1.4.2 CẮT BỎ khỏi bản TV

| Module | Lý do |
|--------|-------|
| Upload (3.1–3.4) | TV không có nguồn file để upload; người dùng không upload từ remote |
| Chrome extension (9.1) | Không có browser desktop trên TV |
| Drag & drop | Không tồn tại trên TV |
| RSS Auto-Download (7) | Cần config phức tạp, không phù hợp 10-foot UI; có thể đưa lên mobile companion sau |
| Subtitle search bằng keyword (8.1) | Nhập keyword bằng remote rất kém UX; thay bằng auto-match phụ đề kèm video, hoặc cho user tải subtitle từ file đã có sẵn |
| File ops nặng (rename/move/copy) | Không có nhu cầu trên TV; nếu giữ, chỉ giữ delete đơn giản |
| Single-instance enforcement | Android quản lý vòng đời app, không cần |
| System tray | Khái niệm này không tồn tại trên Android TV |
| Proxy manual config | Hầu như không cần; nếu có, để Android system proxy lo |

### 1.4.3 MỚI HOÀN TOÀN trên TV

| Tính năng | Mô tả |
|-----------|-------|
| **Embedded Player** | Phát video/audio trực tiếp trong app (ExoPlayer + libVLC fallback) |
| **Resume playback** | Lưu vị trí xem mỗi file (mỗi 5 s), khi mở lại hỏi "Tiếp tục từ 23:14?" |
| **QR login** | Đăng nhập nhanh bằng quét QR từ app mobile |
| **Continue Watching row** | Hàng đầu trang chủ: các video đang xem dở |
| **Recommendations** (V2) | Tích hợp Android TV "Channels" (Recommendations row trên launcher) |
| **Voice search** (V2) | Tích hợp Google Assistant "Search in Fshare" |
| **Cast nhận** (V2) | Nhận lệnh cast từ app Fshare mobile |

## 1.5 Tài sản code có thể tái sử dụng

| Layer | Reusable? | Cách tái sử dụng |
|-------|-----------|------------------|
| Models (`User`, `FileItem`, `TransferTask`, `AppSettings`) | **Có** (logic), KHÔNG (mã) | Định nghĩa lại bằng Kotlin `data class` — chuyển 1-1, ~1 ngày |
| `FshareApi.h/.cpp` (REST endpoints) | **Có** (hợp đồng API) | Viết lại bằng Kotlin + Retrofit/OkHttp; giữ nguyên endpoint + signing logic (`app_key` XOR-encoded) |
| `OAuthProvider.h/.cpp` | **Có** (config), KHÔNG (mã) | Dùng AppAuth-Android, chuyển config 1-1 |
| `HttpClient.h/.cpp` | **KHÔNG dùng được trực tiếp**; có thể gọi qua NDK nếu muốn | Khuyến nghị viết lại bằng OkHttp để đơn giản hoá |
| `DownloadEngine.h/.cpp` (multi-segment + libcurl) | **Tái sử dụng được qua NDK** | Build libcurl-Android + giữ nguyên `DownloadEngine`; expose qua JNI. Hoặc viết lại bằng coroutines + OkHttp range request |
| `UploadEngine.h/.cpp` | Bỏ hẳn (không upload trên TV) | — |
| `SpeedMeter.h/.cpp` | Có | Chuyển 1-1 sang Kotlin (~50 dòng) |
| `FileNameSanitizer` | Có | Chuyển 1-1 |
| `BudgetManager` (slot caps) | Có | Chuyển 1-1 |
| Theme/Colors (Aurora) | **Có** (token), KHÔNG (file) | Đổi sang Material 3 + Compose token; giữ palette gốc để brand consistency |
| QML pages (LoginPage, FileManagerPage, …) | **KHÔNG** | Viết lại Compose-TV từ đầu, theo UX 10-foot |

**Ước lượng**: 60–70% logic backend tái sử dụng được (sau khi chuyển ngôn ngữ); UI viết lại 100%.

## 1.6 Kết luận chương 1

- Bản TV là **một sản phẩm con tinh gọn**, không phải port nguyên trạng. ~50% module bị cắt, ~30% module được biến đổi đáng kể, ~20% module mới.
- Trọng tâm sản phẩm: **xem media trực tiếp trên TV**. APK + update là yêu cầu phân phối; embedded player là yêu cầu trải nghiệm cốt lõi.
- Quyết định kiến trúc và stack triển khai ở [02](02_khuyen-nghi-kien-truc.md) và [03](03_lua-chon-cong-nghe.md).
