---
title: StreamIX TV — Design System Gap Analysis
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# Gap Analysis — Mockups vs Current Implementation

Sau khi user cung cấp mockups chính thức, đối chiếu thấy implementation hiện tại lệch khá xa khỏi design system. Tài liệu này phân tích lại requirement đầy đủ, list gap và plan rebuild.

## Mockups đã được cung cấp

7 màn hình chính:

1. **S0 Splash** — Logo "S" amber square + "StreamIX TV" wordmark + golden underline + version.
2. **S1 Login** — Split layout: hero text (trái) + form đăng nhập (phải).
3. **S2 Home** — Search bar trên cùng + **3 hero cards** (Gợi Ý / Xu Hướng / Top) + "Tiếp tục xem" row.
4. **S12 Search** — Filter chips (Tất cả / Phim / Series / Folder) + on-screen keyboard + "Gợi ý cho bạn" + result grid.
5. **S2a Gợi Ý detail** — Grid 4 cột folder + video, sort filter (SẮP XẾP MỚI NHẤT, TẤT CẢ).
6. **S2b Xu Hướng detail** — Time period tabs (HÔM NAY / 7 NGÀY / 30 NGÀY) + grid với badges (HOT, #1, OSCAR).
7. **S2c Top detail** — Vertical list với rank numbers (01, 02, 03...) + filter (IMDB / RT / FSHARE).

## So sánh chi tiết

### S0 Splash
| Element | Mockup | Hiện tại | Gap |
|---|---|---|---|
| Logo | Rounded amber square với "S" in dark navy | "S" trên amber square 192dp | OK (placeholder hợp lý) |
| Wordmark | "StreamIX TV" sans-serif lớn | Có | OK |
| Golden underline | Có thanh ngang dưới logo | KHÔNG | ⚠️ Cần thêm |
| Version | Dưới cùng "v1.0.x" | Có nhưng chưa hiển thị rõ | ⚠️ Cần verify position |

**Gap nhỏ**: thêm horizontal accent line giữa logo và wordmark.

### S1 Login
| Element | Mockup | Hiện tại | Gap |
|---|---|---|---|
| Layout | Split 50/50: hero trái, form phải | Form căn giữa duy nhất | ❌ Sai layout |
| Hero text | "Cloud của bạn, trên màn hình lớn." | KHÔNG có | ❌ Thiếu |
| Subtitle | "Đăng nhập bằng tài khoản Fshare..." | KHÔNG | ❌ Thiếu |
| Form header | "ĐĂNG NHẬP" small caps | Có "Đăng nhập" thường | ⚠️ Style khác |
| Form title | "Tài khoản Fshare" | KHÔNG | ❌ Thiếu |
| Email field | Yellow border khi focus | OK | OK |
| Password show/hide | "Hiện" text button bên phải | Icon eye | ⚠️ Cần đổi |
| Remember me | Checkbox "Nhớ tôi 30 ngày" | KHÔNG | ❌ Thiếu |
| Submit button | Yellow full-width "Đăng nhập" | OK | OK |
| Forgot password | Helper text với link fshare.vn | KHÔNG | ❌ Thiếu |
| Footer | "FSHARE.VN - ACCOUNT" small | KHÔNG | ⚠️ Optional |

**Gap lớn**: Cần redesign Login screen theo split layout hoàn toàn.

### S2 Home
| Element | Mockup | Hiện tại | Gap |
|---|---|---|---|
| Top bar | Logo trái + Settings + Avatar | OK | OK |
| Search bar | Lớn, full-width, placeholder "Tìm phim, tên file..." + "OK · TÌM KIẾM" hint | KHÔNG (search chỉ có ở Top bar) | ❌ Thiếu prominent search bar |
| Hero card 1 | **Gợi Ý** — yellow bg, star icon, "42 mục · cá nhân hoá" | KHÔNG (chỉ có row nhỏ) | ❌ Sai layout |
| Hero card 2 | **Xu Hướng** — orange-red bg, flame icon, "55 mục · 7 ngày qua" | KHÔNG (row nhỏ) | ❌ Sai layout |
| Hero card 3 | **Top** — dark blue bg, trophy icon, "100 phim hàng đầu" | KHÔNG TỒN TẠI | ❌ Thiếu hoàn toàn |
| Continue watching | Horizontal row với progress bars | OK | OK |

**Gap lớn**: Home phải redesign với 3 hero cards (mỗi card click → detail screen) thay vì 4 rows nhỏ.

### S12 Search
| Element | Mockup | Hiện tại | Gap |
|---|---|---|---|
| Title | "Tìm kiếm" | KHÔNG | ⚠️ Thiếu |
| Search input | Yellow border + result count display | TextField đơn giản | ⚠️ Cần redesign |
| Filter chips | TẤT CẢ · 14, PHIM · 9, SERIES · 2, FOLDER · 3 | KHÔNG | ❌ Thiếu |
| On-screen keyboard | QWERTY 4 rows + numbers/shift/enter | KHÔNG | ❌ Thiếu |
| "Gợi ý cho bạn" | 4 quick suggestions với "+" prefix | KHÔNG | ❌ Thiếu |
| Result grid | 4 cột × 2 rows với badges (HD, NEW) | OK (4 cột grid) | ⚠️ Thiếu badges |

**Gap lớn**: Search cần on-screen keyboard + filter chips + suggestions.

### S2a Gợi Ý detail (chưa có)
- Grid 4 cột mixing folder + video.
- Folder cards có "(N mục · M thư mục con)".
- Video cards có badges (HD, 4K).
- Sort filter (SẮP XẾP / TẤT CẢ).
- Header "ĐỀ XUẤT CHO BẠN".

**Gap lớn**: Chưa có screen này.

### S2b Xu Hướng detail (chưa có)
- Tab "HÔM NAY / 7 NGÀY / 30 NGÀY".
- Grid mixing folder + video.
- Badges (HOT, #1, OSCAR).
- Header "BẢNG XẾP HẠNG NHANH".

**Gap lớn**: Chưa có screen này.

### S2c Top detail (chưa có)
- Vertical list với rank number 01-100.
- Mỗi row: rank + thumbnail + title + year + size + chevron.
- Filter chips (IMDB / RT / FSHARE).
- Header "BẢNG XẾP HẠNG".

**Gap lớn**: Chưa có screen này — mà đây là feature chính của app TV theo design.

## Vietnamese language audit

Mockups dùng các thuật ngữ:
- "Gợi Ý" (capital ý)
- "Xu Hướng" (capital H)
- "Top"
- "Tiếp tục xem"
- "Đề xuất cho bạn"
- "Bảng xếp hạng"
- "Sắp xếp · Mới nhất"
- "Tất cả"
- "Tài khoản Fshare"
- "Cloud của bạn, trên màn hình lớn"
- "Đăng nhập bằng tài khoản Fshare để xem ngay video lưu trữ trên cloud — không copy file, không cài thêm."
- "Quên mật khẩu? Truy cập fshare.vn trên điện thoại để khôi phục."
- "Nhớ tôi 30 ngày"
- "Hiện" / "Ẩn" cho password toggle
- "Tìm phim, tên file, hoặc bộ phim..."
- "Bàn phím"
- "Gợi ý cho bạn"
- "Phim hành động"
- "Anime"
- "TV Series"
- "Phim tài liệu"

→ Strings hiện tại cơ bản OK nhưng cần audit + bổ sung mới.

## Plan rebuild

Phân chia thành 6 phase, mỗi phase độc lập compile được:

### Phase R1: Top feature (CRITICAL — chưa có)
- Domain: `TopMovie` model, `TopRepository` interface
- Data: `TopRepositoryImpl` đọc từ Sheet thứ 3 hoặc hardcode (V1)
- Network: `SheetsApi` đã có, thêm sheet ID cho Top
- UI: `TopScreen` — vertical list rank 01-100
- Nav: `TopRoute`

### Phase R2: Home redesign (3 hero cards)
- Component: `FsHeroCard` với icon + title + subtitle + bg gradient
- HomeScreen: search bar + 3 hero cards row + ContinueWatching row
- Navigate: click card → respective detail screen

### Phase R3: Detail screens
- `SuggestedScreen` (Gợi Ý detail) — grid 4 cột mixing folder/video
- `TrendingScreen` (Xu Hướng detail) — tabs + grid với badges
- `TopScreen` đã có ở R1
- Routes mới + navigation

### Phase R4: Search redesign
- Component: `FsKeyboard` (QWERTY on-screen 4 rows)
- Component: `FsFilterChip` (chip với count badge)
- Component: `FsSuggestionPill` (+ prefix)
- SearchScreen: title + input + chips + keyboard + suggestions + result grid
- ViewModel: thêm filter state (All/Movie/Series/Folder)

### Phase R5: Login split layout
- LoginScreen: Row split 50/50
- Left: hero text + subtitle + footer
- Right: form với header "ĐĂNG NHẬP" + title + fields + remember + submit + forgot
- Component: `FsCheckbox`

### Phase R6: Vietnamese strings audit + Splash polish
- Audit strings.xml mọi screens, đảm bảo proper Vietnamese
- Splash: thêm golden underline accent
- Test build + visual verify

## Estimate

| Phase | LOC est | Files mới | Files sửa |
|---|---|---|---|
| R1 Top | ~400 | 5 | 3 |
| R2 Home redesign | ~250 | 1 | 2 |
| R3 Detail screens | ~600 | 3 | 2 |
| R4 Search | ~500 | 3 | 2 |
| R5 Login | ~300 | 1 | 2 |
| R6 Polish | ~100 | 0 | 5 |
| **TỔNG** | **~2150** | **13** | **16** |

## Đề xuất thứ tự thực thi

**Ưu tiên 1 (user complaints):**
1. R1 Top — addresses "chưa có trang Top"
2. R4 Search — addresses "search không hoạt động"
3. R2 Home — needed for navigation to detail screens

**Ưu tiên 2 (visual fidelity):**
4. R3 Detail screens
5. R5 Login

**Ưu tiên 3 (polish):**
6. R6 Vietnamese + Splash

Implement liên tục, mỗi phase commit + build để verify. Không dồn cuối session.
