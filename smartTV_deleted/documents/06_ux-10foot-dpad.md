---
title: 06 — UX 10-foot & D-pad navigation
date: 2026-05-04
---

# 06. Trải nghiệm 10-foot & điều hướng D-pad

## 6.1 Triết lý 10-foot UI

User ngồi cách TV ~3 m, cầm remote 5–7 nút (D-pad + OK + Back + Home + Menu + đôi khi Voice). UI phải thiết kế khác hoàn toàn desktop:

- **Lớn**: text body tối thiểu 22 sp, title 32–48 sp.
- **Đơn giản**: mỗi screen 1 mục đích chính, tránh dày đặc thông tin.
- **Tương phản cao**: nền tối (#0F1218 đến #1A1F2A), text trắng/sáng, focus màu nổi (vàng/cam Fshare).
- **Khu vực an toàn (overscan)**: nội dung quan trọng cách mép màn hình ≥ 5%; padding ngoài ≥ 48 dp.
- **Focus rõ**: phải có vùng focus được vẽ rõ ràng (border 4 dp + scale 1.05 + shadow). Thiếu focus rõ ràng = user lạc.

## 6.2 Information architecture

```
TV App
├── Home (sau khi đăng nhập)
│   ├── "Tiếp tục xem" — hàng đầu (nếu có resume)
│   ├── "File gần đây" — file mới upload trong tài khoản
│   ├── "Thư mục yêu thích" — folder user đã star
│   └── "Tải về thiết bị" — file đã download xuống local
├── Browse (thư mục)
│   ├── Folder tree breadcrumb
│   └── Grid file (4 cột × N hàng)
├── Search (V2)
├── Settings
│   ├── Tài khoản (info, logout, switch user)
│   ├── Phát video (engine: Exo/VLC, audio passthrough on/off, subtitle font)
│   ├── Tải về (vị trí: USB / Internal, max threads)
│   ├── Mạng (Wi-Fi, proxy nếu cần)
│   ├── Cập nhật (channel: stable/beta, kiểm tra ngay)
│   └── Giới thiệu (version, gửi log lỗi)
└── Player (full-screen, immersive)
```

## 6.3 Wireframe text mô tả

### 6.3.1 Login Screen

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│                  ┌─ Fshare logo ─┐                      │
│                                                         │
│            Đăng nhập tài khoản Fshare                   │
│                                                         │
│       ┌───────────────────────────────────┐             │
│       │ [QR code 240×240]                 │             │
│       │  Quét bằng app Fshare mobile      │  ← Default  │
│       │                                   │     focus   │
│       └───────────────────────────────────┘             │
│                                                         │
│       ── hoặc ──                                        │
│                                                         │
│       [ Đăng nhập email + mật khẩu ]                    │
│       [ Đăng nhập Google ]   [ Đăng nhập FPT ID ]       │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 6.3.2 Home Screen

```
┌─────────────────────────────────────────────────────────┐
│  Fshare                          🔍   ⚙           [User]│ ← Top bar
├─────────────────────────────────────────────────────────┤
│  Tiếp tục xem                                           │
│  [▶ card 250×140] [▶ card] [▶ card] [▶ card] [▶ card] →│
│                                                         │
│  File gần đây                                           │
│  [card] [card] [card] [card] [card] [card] →            │
│                                                         │
│  Thư mục                                                │
│  [📁 Phim] [📁 Tài liệu] [📁 Nhạc] [📁 Backup] →        │
│                                                         │
│  Đã tải xuống thiết bị                                  │
│  [card] [card] [card] →                                 │
└─────────────────────────────────────────────────────────┘
```

Pattern: `TvLazyColumn` chứa nhiều `TvLazyRow` (immersive list) — D-pad up/down giữa các hàng, left/right trong hàng.

### 6.3.3 File Detail (preview trước khi play)

```
┌─────────────────────────────────────────────────────────┐
│  [Thumbnail 480×270]   The.Movie.2025.1080p.mkv         │
│                        12.4 GB · 2h 18m · MKV/HEVC      │
│                                                         │
│                        [ ▶ Phát ngay ]                  │ ← Default
│                        [ ⬇ Tải về ]                     │
│                        [ ⭐ Yêu thích ]                  │
│                                                         │
│                        Phụ đề: vi (mặc định), en        │
│                        Audio: AC3 5.1 (vi), AAC 2.0 (en)│
└─────────────────────────────────────────────────────────┘
```

### 6.3.4 Player Overlay

Khi đang phát, ẩn UI sau 3s. Bấm OK / D-pad → hiện overlay:

```
┌─────────────────────────────────────────────────────────┐
│  [↩ Thoát]                              The.Movie.mkv   │
│                                                         │
│                                                         │
│              [video full screen]                        │
│                                                         │
│                                                         │
│                                                         │
│                                                         │
│  ━━━━━━━━━━━━━━━━━━━━●─────────                         │
│  00:42:13                              02:18:00         │
│                                                         │
│  [⏮]  [⏯]  [⏭]      [Phụ đề ▾]  [Audio ▾]  [⚙ Cài đặt]│
└─────────────────────────────────────────────────────────┘
```

D-pad mapping:
- Left/Right (khi controls ẩn): tua ±10s
- Long press Left/Right: tua ±30s
- OK: play/pause
- Up: hiện overlay
- Down: hiện danh sách track
- Back: hỏi "Thoát phim?" → có / không (nhớ position trước khi thoát)

## 6.4 Focus model — Compose-TV

Compose-TV đã giải quyết phần lớn. Quy tắc khi viết:

```kotlin
// Focusable item phải dùng `Modifier.focusable()` HOẶC component có sẵn của tv-foundation
StandardCardLayout(
    imageCard = { interactionSource ->
        CardLayoutDefaults.ImageCard(
            onClick = onClick,
            interactionSource = interactionSource,
        ) {
            AsyncImage(model = thumbnail, contentDescription = null)
        }
    },
    title = { Text(file.name, maxLines = 2) },
    subtitle = { Text(file.sizeFormatted) }
)

// Cho phép focus quay lại item đầu tiên đã focus khi back vào row
val focusRestorer = remember { FocusRequester.Default }
TvLazyRow(modifier = Modifier.focusRestorer(focusRestorer)) {
    items(files) { ... }
}
```

Quan trọng:
- **Initial focus**: mỗi screen phải có 1 phần tử focus mặc định khi vào. Compose-TV `Modifier.focusRequester(...)` + `LaunchedEffect` để request.
- **Focus restoration**: khi back ra row rồi quay lại, focus phải về đúng card cũ — dùng `focusRestorer`.
- **Out-of-bounds**: D-pad up ở row đầu = không di chuyển; D-pad down ở row cuối = scroll up từ bottom; KHÔNG được loop vô tận.
- **Back button**: phải có hành vi rõ — Player → confirm thoát; Browse → lên thư mục cha; Home → confirm thoát app.

## 6.5 Phím nhập (text input) — kẻ thù của TV

Login bằng email/password trên TV qua remote D-pad là tệ nhất quả đất. Mitigation:

1. **Default = QR login** từ mobile — không phải gõ.
2. **Nếu vẫn cần gõ**: dùng `TextField` với on-screen keyboard, nhưng có shortcut:
   - "Sử dụng bàn phím Bluetooth của bạn" — gợi ý kết nối phím rời.
   - "Gửi link đăng nhập về email" — Fshare gửi magic link, user click trên phone, TV nhận token qua polling endpoint.
3. **Search bằng voice**: V2 — Google Assistant intent.

## 6.6 Performance targets

- **Cold start → Home hiển thị**: < 2 giây trên TV box giá rẻ (1 GB RAM, ARMv7).
- **Scroll TvLazyRow 500 items**: 60 fps không drop.
- **Mở Player → frame đầu tiên**: < 3 giây (network cho phép).
- **Memory peak**: ≤ 250 MB cho app + player.

Để đạt:
- Image load: Coil 2.7 với `crossfade(150ms)` + `memoryCachePolicy(ENABLED)`. Thumbnail size cố định 250×140 dp, không scale từ ảnh gốc.
- Tránh recompose toàn screen — split state nhỏ.
- ProGuard/R8 bật ở release.
- Baseline Profile (Macrobenchmark) cho cold start — giảm 20–30% startup.

## 6.7 Accessibility

- TalkBack support: mỗi card có `contentDescription` đầy đủ.
- Subtitle font size có thể tăng (Settings → Phụ đề → Lớn / Vừa / Nhỏ).
- High contrast mode: option trong Settings, đổi palette focus sang vàng đậm/đen.
- Hỗ trợ remote không có nút back — luôn có nút back trên-màn-hình ở mỗi screen quan trọng.

## 6.8 Localization

- Tiếng Việt là default. English là override.
- File `res/values/strings.xml` (vi) + `res/values-en/strings.xml`.
- Date/time format: `Locale("vi", "VN")`.
- Quan trọng: tránh string nối chuỗi — dùng `getString(R.string.xxx, arg1, arg2)` để dịch nhúng được.

## 6.9 Branding tokens — đề xuất palette TV

Dựa trên Aurora theme của desktop:

```kotlin
object FsTvColors {
    val Background = Color(0xFF0F1218)
    val Surface    = Color(0xFF1A1F2A)
    val SurfaceHi  = Color(0xFF252B38)
    val Primary    = Color(0xFFFFB12C)   // Fshare accent
    val OnPrimary  = Color(0xFF0F1218)
    val Focus      = Color(0xFFFFB12C)
    val FocusRing  = Color(0xFFFFFFFF)
    val Text       = Color(0xFFEEF1F6)
    val TextMuted  = Color(0xFFA9B1BD)
    val Error      = Color(0xFFE25062)
    val Success    = Color(0xFF4FCC8E)
}
```

Khoảng cách & typography:

| Token | Giá trị | Dùng cho |
|-------|---------|----------|
| `Spacing.xs` | 4 dp | Inner card padding |
| `Spacing.sm` | 8 dp | Khoảng giữa icon & text |
| `Spacing.md` | 16 dp | Giữa các card trong row |
| `Spacing.lg` | 32 dp | Giữa các row |
| `Spacing.xl` | 48 dp | Padding ngoài screen (overscan) |
| `Type.body` | 22 sp / 28 sp lh | Text thường |
| `Type.title` | 32 sp / 40 sp lh | Tiêu đề screen |
| `Type.display` | 48 sp / 56 sp lh | Tên phim trong detail |

## 6.10 Test với người dùng thật

Bắt buộc trước khi release:
- 5 user "non-tech" (bố mẹ, người lớn tuổi) thử login + browse + play 1 video.
- Đo: thời gian login, số lần bị "lạc focus", số lần bấm Back nhầm.
- Tiêu chí pass: 100% user mở được app và play 1 video trong 3 phút mà không cần hỗ trợ.

## 6.11 Kết luận chương 6

- 10-foot UI khác desktop về font, focus, density, input. Phải thiết kế lại từ đầu.
- Compose-TV xử lý focus engine — tận dụng. Đừng rebuild bằng View system cũ.
- Login QR là default; gõ keyboard remote là last resort.
- Performance phải đo trên TV box rẻ nhất, không emulator.

Tiếp theo: roadmap ([07](07_lo-trinh-trien-khai.md)).
