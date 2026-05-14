---
title: 11 — Audit Design Handoff v1.0 (StreamIX TV package)
date: 2026-05-04
reviewer: Tech Lead Android
status: 🟡 ACCEPT WITH CONDITIONS (cập nhật 2026-05-04 sau quyết định ở 12)
package: Fshare.vn.zip → streamix-handoff/ v1.0 (designer release 2026-05-04)
revision: 2 — verdict updated sau decision của Project Owner
---

# 11. Audit gói Design Handoff v1.0

> **⚠️ CẬP NHẬT 2026-05-04**: Project Owner đã ra quyết định về 3 vấn đề nền tảng — xem `12_scope-decisions-v1.md`. Phần "Verdict" và "Action items" dưới đây đã được điều chỉnh tương ứng. Nội dung audit kỹ thuật giữ nguyên làm record.

## 0. Verdict (revised)

**✅ ACCEPT** (cập nhật cuối 2026-05-04 sau D-3.5 + D-4) — đã accept toàn bộ scope decisions. Chỉ còn lại các technical fix designer làm trong v1.1.

Tất cả vấn đề nền tảng đã được Project Owner quyết định 2026-05-04:

1. **Brand "StreamIX TV"**: ✅ APPROVED.
2. **Cắt 4 module P0** (QR login, OAuth login, Downloads, In-app update): ✅ APPROVED.
3. **Internal APK distribution**: ✅ APPROVED.
4. **Update workflow = Phương án A** (manual notify + manual install): ✅ APPROVED.
5. **Package name = `vn.streamix.tv`**: ✅ APPROVED.

Còn lại trước Phase 2 UI shell — chỉ technical fix:
- Designer fix các issue kỹ thuật ở §4 dưới (typography bump về spec, tokens 3 lớp, Figma/HTML interactive, missing icon `ic_notification` — vẫn có thể cần cho stream/download done toast trong app).
- Designer giao logo + app icon + splash + banner với brand StreamIX TV.

Engineering **bắt đầu Phase 0 + Foundation NGAY** với gói v1.0 hiện có.

Chi tiết audit kỹ thuật giữ nguyên ở các mục dưới — vẫn áp dụng cho v1.1.

---

## 1. Tóm tắt package nhận được

```
Fshare.vn.zip  → 51 KB
└── streamix-handoff/
    ├── README.md                       (6.5 KB)
    ├── design-tokens.json              (14.3 KB)  — Tokens Studio format
    ├── tokens.style-dictionary.json    (3.8 KB)   — Style Dictionary backup
    ├── motion-specs.md                 (7.8 KB)
    ├── localization.csv                (15.7 KB)  — 208 keys
    ├── streamix-handoff-hub.html       (74.7 KB)  — interactive dashboard
    └── icons/                          (45 SVG files)
```

Designer cũng tham chiếu một file `../StreamIX TV.html` ở cấp ngoài cho screen mockup — **file này KHÔNG có trong zip** (xem §4 dưới).

---

## 2. Đối chiếu với 14 nhóm deliverable (theo `10_design-handoff-requirements.md`)

| # | Nhóm | Yêu cầu | Trạng thái | Ghi chú |
|---|------|---------|-----------|---------|
| D1 | File Figma master | Figma cloud + Dev Mode | ❌ **Thiếu** | Không có Figma URL, designer thay bằng HTML mockup riêng (file ngoài zip) — không tương đương |
| D2 | Design tokens JSON | Tokens Studio + 2 lớp primitive/semantic | ⚠️ **Đạt một phần** | Có cả Tokens Studio + Style Dictionary; nhưng **chỉ 1 lớp primitive**, không có lớp semantic/component như spec yêu cầu |
| D3 | Icon assets | ≥ 45 SVG, viewBox 24, currentColor, ≤ 4 KB | ✅ **Đạt** | 45/46 (thiếu `ic_notification`); ic_oauth_google có nested SVG cần fix |
| D4 | Logo & brand | 6 file SVG | ❌ **Thiếu hoàn toàn** | Designer ghi rõ "pending brand approval" |
| D5 | App icon launcher | Banner 320×180 + adaptive + notification | ❌ **Thiếu hoàn toàn** | Cùng lý do |
| D6 | Splash screen | Logo 288×288 + bg color | ❌ **Thiếu hoàn toàn** | Cùng lý do |
| D7 | Illustration | 3 SVG empty/error states + placeholder thumbnail | ❌ **Thiếu** | Designer claim "icon-only EmptyState" — nhưng vẫn cần placeholder thumbnail |
| D8 | Component specs (Dev Mode) | 24 component, đầy đủ states | ⚠️ **Một phần** | HTML mockup chỉ có **14 components visualised**, không có Dev Mode để inspect dimension/token reference |
| D9 | Screen specs mọi state | 27 screens × tất cả states | ⚠️ **Một phần** | HTML có **18 screens**; thiếu screen flows + login QR/OAuth + Downloads + Update + Settings Network/Update/About |
| D10 | Prototype 6 user flows | Click-through Figma | ❌ **Thiếu** | HTML có thể là static; không có rõ ràng prototype interactive cho 6 flow |
| D11 | Motion specs | Bảng + 6 animation chính | ✅ **Đạt vượt** | 10 animations specified, có reduce-motion handling, có Compose easing names |
| D12 | Localization CSV | ~200 key vi+en+context+screen | ✅ **Đạt** | 208 keys, đúng format |
| D13 | Sample content | VN realistic, không Lorem | ✅ **Đạt** (theo HTML preview) | Tên file/folder VN realistic |
| D14 | Accessibility audit | Optional | ⏭️ **Skip** | Hợp lý vì optional |

**Kết quả tổng**: 4/14 đạt đầy đủ, 4/14 đạt một phần, 5/14 thiếu/không có, 1/14 hợp lý skip.

---

## 3. 🚨 3 vấn đề nền tảng phải resolve trước

### 3.1 Đổi thương hiệu sang "StreamIX TV"

**Bằng chứng**:
- File README mở đầu: "StreamIX TV — Design Handoff Package v1.0"
- Folder zip: `streamix-handoff/`
- Localization: `app_name = StreamIX TV`, `app_tagline = Phát mọi định dạng từ Fshare`, `login_hub_title = Đăng nhập StreamIX`
- Tokens metadata: `"name": "StreamIX TV"`
- HTML hub: `<title>StreamIX TV — Handoff Hub v1.0</title>`

**Mâu thuẫn với**:
- Project instructions: "Phát triển ứng dụng desktop client cho fshare.vn".
- Tài liệu `09_design-spec.md` §15 Open Questions: tên app "Fshare TV hay FsNext" — quyết định brand cần chốt **trước Splash design**, không phải tự đổi.
- Tài liệu `01_phan-tich-hien-trang.md`: tên dự án "FsNext (Fshare Tool v6.0)".

**Tác động**:
- Brand không được Marketing/Legal/Lãnh đạo Fshare approve.
- Toàn bộ string `app_name`, `login_hub_title`, `app_tagline`… phải sửa.
- Logo, app icon, splash chưa làm được cho đến khi chốt brand → kéo dài critical path.
- User cuối nhận APK với tên lạ "StreamIX TV" sẽ bối rối — không liên hệ với Fshare.

**Action item AI-1** (P0):
- PM họp với Marketing/Brand team → chốt tên chính thức (đề xuất: **"Fshare TV"** hoặc **"FsNext"** theo project instructions).
- Designer rename trong: tokens metadata, README, folder name, mọi key localization có "StreamIX", HTML hub title.
- Re-issue handoff v1.1 với brand đã chốt.

### 3.2 Cắt 4 module P0 khỏi V1

Designer tự cắt mà không có CR (change request). README liệt kê:

| Module bị cắt | Lý do designer đưa ra | Đánh giá |
|---------------|----------------------|----------|
| **Login QR (S1a)** | Không thấy lý do trong README | ❌ Không chấp nhận. QR là **default login method** theo `09_design-spec.md` §9.3 và nguyên tắc N7 ("an toàn cho remote, không gõ keyboard dài"). Bỏ QR = bắt user gõ email/password trên D-pad — UX tệ |
| **Login OAuth (S1c)** | Không có lý do | ❌ Không chấp nhận. Google/FPT ID là yêu cầu của Persona 1; nhiều user Fshare chỉ login qua OAuth |
| **In-app update (S8 + S7e)** | "Play Store handles updates" | ❌ Không chấp nhận. Tài liệu `04_apk-va-update.md` định nghĩa rõ **internal APK distribution**, KHÔNG qua Play Store. Bỏ in-app update = không thể release |
| **Downloads (S6 + S7c)** | "no offline storage in V1" | ❌ Không chấp nhận. `09_design-spec.md` §4 và `01_phan-tich-hien-trang.md` §1.4.1 đều liệt kê download là **GIỮ và NÂNG CẤP** trong V1 |

**Tác động**:
- Nếu accept như vậy, V1 sẽ phát hành **không có cách update** ngoài cài đè APK thủ công — toàn bộ kế hoạch CI/CD ở §4.10 vô nghĩa.
- User mới không có cách login dễ dàng → tỷ lệ activation thấp.
- App không có offline functionality → bị mạng yếu là tê liệt hoàn toàn.

**Action item AI-2** (P0):
- Tech Lead + PM phản hồi designer: 4 module trên BẮT BUỘC trong V1.
- Designer thêm vào handoff v1.1: screens S1a (QR), S1c (OAuth), S6 (Downloads), S7c, S7e (Update settings), S8 (Update prompts).

### 3.3 Giả định Play Store

**Bằng chứng**:
- README §"Quickstart cho engineering" không nhắc gì đến `tv-update.fshare.local` hay sideload.
- Bỏ S8 với lý do "Play Store handles updates".

**Mâu thuẫn**:
- Tài liệu `04_apk-va-update.md` rev.2 §0 mô tả rõ ngữ cảnh **internal deployment**: "phân phối nội bộ qua APK", "không phụ thuộc Play Store", "self-update qua PackageInstaller".
- Nếu Play Store, không cần custom update flow, không cần fingerprint hard-code, không cần manifest — toàn bộ §4.5–§4.9 của tài liệu 04 trở nên dư thừa.

**Action item AI-3** (P0):
- PM gửi designer link tài liệu 04 + giải thích: bản này là **internal-only**, không Play Store.
- Designer cần thiết kế lại S7e Settings → Update + 4 variant của S8 Update prompt theo §9.21–9.23 của spec 09.

---

## 4. Chi tiết technical issues cần fix

### 4.1 D1 — Không có Figma file

**Vấn đề**: Designer thay Figma bằng HTML mockup riêng. Hai vấn đề:
1. File HTML `StreamIX TV.html` ở cấp ngoài zip — KHÔNG được delivered. Engineering không có cái để mở.
2. HTML không có **Dev Mode**: không inspect được dimension, không copy CSS properties, không thấy token reference của từng element. Engineering phải đoán.

**Action item AI-4** (P0): Một trong hai:
- Cung cấp Figma URL chính thức với Dev Mode.
- HOẶC nếu giữ HTML, đính kèm file mockup trong zip + thêm "specs sidebar" hiển thị tokens dùng cho mỗi element.

### 4.2 D2 — Tokens chỉ có 1 lớp

**Yêu cầu** (`10_design-handoff-requirements.md` §3.4):
> KHÔNG hard-code hex trong tokens "semantic" — phải reference về tokens "primitive". Nguyên tắc 2 lớp:
> - Primitive: `color/orange/500 = #FFB12C`
> - Semantic: `color/accent/primary = {color.orange.500}`

**Thực tế** trong `tokens.style-dictionary.json`:
```json
"accent": {
  "primary":      { "value": "#FFB12C" },   ← hard-code, không phải reference
  "primary-hi":   { "value": "#FFCB6B" }
}
```

**Tác động**: Khi brand refresh đổi accent từ amber sang xanh, phải đổi `#FFB12C` ở **mọi nơi semantic** thay vì 1 chỗ primitive. Mất đi mục đích chính của tokens.

**Action item AI-5** (P1): Designer thêm lớp `primitive` đầu tiên (palette amber/blue/green/red với tonal scale 50–900), lớp `semantic` reference qua `{primitive.amber.400}`.

### 4.3 D2 — Typography sizes nhỏ hơn spec

| Token | Spec `09_design-spec.md` §6.2 | Delivery |
|-------|------------------------------|----------|
| body/lg | 24 sp | 20 sp ⬇ |
| body/md | 22 sp | 18 sp ⬇ |
| label/lg | 22 sp | (thiếu) |
| caption | 18 sp | 16 sp ⬇ |
| headline (36 sp) | có | (thiếu) |

**Tác động**: TV xa 3 m, font 18 sp khó đọc. Nguyên tắc N6 yêu cầu body min 22 sp.

**Action item AI-6** (P0): Designer bump lại body-md = 22, body-lg = 24, restore headline 36, label-lg 22, caption 18. Tokens updated reflect cả Figma.

Vùng cần test: skim qua mọi screen mockup xem còn fit không khi font tăng — có thể phải reduce padding hoặc tăng card width.

### 4.4 D3 — Icon `ic_notification` thiếu, `ic_oauth_google` invalid SVG

**ic_notification**: thiếu hoàn toàn. Cần để hiển thị badge update, download done, error.

**ic_oauth_google**: structure SVG invalid:
```svg
<svg viewBox="0 0 24 24" stroke="currentColor" stroke-width="2" ... fill="none">
  <svg viewBox="0 0 24 24" ...>          ← nested SVG, invalid
    <path d="..." fill="#4285F4"/>
    ...
  </svg>
</svg>
```

Chuyển sang VectorDrawable Android sẽ lỗi hoặc render rỗng. Phải flatten về single root SVG với multi-color paths (giữ brand colors hard-coded vì là logo bên thứ 3).

**Action item AI-7** (P1):
- Add `ic_notification.svg` (monochrome, viewBox 24, currentColor).
- Re-export `ic_oauth_google.svg` flatten 1 root SVG.
- Verify `ic_oauth_fpt.svg` cũng đã flatten đúng.

### 4.5 D8 — Chỉ 14/24 components

Spec `09_design-spec.md` §8 yêu cầu 24 components. HTML mockup có 14. Thiếu 10:

Cần xác minh thiếu cụ thể nào — không xem được Figma. Dự đoán dựa trên scope cuts:
- C9 BottomSheet (vì cắt S5b track selection)
- C16 TextField (vì cắt login email cũng có thể? Cần verify)
- C17 Switch
- C18 RadioGroup
- C21 KeyboardOnScreen
- C22 QRCard (vì cắt QR login)
- C23 PlayerOverlay (chuyên biệt — chắc có nhưng không count?)

**Action item AI-8** (P0): Designer xác nhận danh sách 14 component đã có, làm tiếp 10 còn lại theo spec §8.

### 4.6 D9 — 18 screens thay 27

Screens hiển thị trong HTML hub: S0, S1, S2, S2b, S3, S3b, S3c, S4, S5a, S5b, S5c, S5d, S5e, S7, S7a, S7b, S7c, S8, S9, S10, S11.

So với spec `09_design-spec.md` §9 (27 screens):

| Screen | Trong package? | Ghi chú |
|--------|----------------|---------|
| S0 Splash | ✅ | |
| S1 Login Hub | ✅ | nhưng simplified |
| S1a Login QR | ❌ | bị cắt |
| S1b Login Email | ⚠️ | hợp nhất với S1? |
| S1c Login OAuth | ❌ | bị cắt |
| S2 Home | ✅ | layout đổi nhiều |
| S2b | ✅ | mới — cần biết là gì |
| S3 Browse | ✅ | |
| S3b, S3c | ✅ | mới — cần biết là gì |
| S4 File Detail | ✅ | |
| S5 Player full-screen | ⚠️ | có S5a-e nhưng không có S5 base? |
| S5a Player Overlay | ✅ | |
| S5b Track selection | ✅ trong list nhưng README nói cắt — **mâu thuẫn nội bộ** |
| S5c Error | ✅ | |
| S5d Resume Prompt | ✅ | |
| S5e Post-play | ✅ trong list nhưng README nói cắt — **mâu thuẫn nội bộ** |
| S6 Downloads | ❌ | bị cắt |
| S7 Settings Hub | ✅ | |
| S7a Account | ✅ | |
| S7b Playback | ✅ | |
| S7c Download | ❌ | bị cắt |
| S7d Network | ❌ | thiếu |
| S7e Update | ❌ | bị cắt |
| S7f About | ❌ | thiếu |
| S8 Update prompts | ✅ trong list nhưng README nói cắt — **mâu thuẫn nội bộ** |
| S9 Onboarding | ✅ | |
| S10 Confirm | ✅ | |
| S11 Global Error | ✅ | |

Nội bộ designer **không nhất quán** (README cắt S5b/S5e/S8 nhưng HTML lại có).

**Action item AI-9** (P0): Designer làm rõ với từng screen còn thiếu/mâu thuẫn:
- S1a, S1c, S6, S7c, S7d, S7e, S7f cần được thiết kế.
- S5b, S5e, S8 phải xác nhận: có trong V1 hay không? Đồng bộ README với HTML.

### 4.7 D10 — Không rõ có prototype interactive không

HTML mockup có thể là static (không click-through) hoặc có. Cần verify.

**Action item AI-10** (P1): Designer cung cấp:
- Link prototype Figma click-through cho 6 user flows, HOẶC
- Bản HTML version có click-navigation giữa các screen để QA dùng.

### 4.8 Tokens.json thiếu component layer

Spec §3.4 yêu cầu 3 lớp: primitive → semantic → component. Designer chỉ có 1 lớp.

Component tokens cần thiết:
```json
"button": {
  "primary": {
    "bg":   { "value": "{color.accent.primary}" },
    "text": { "value": "{color.text.inverse}" },
    "padding-x": { "value": "{spacing.5}" }
  }
}
```

**Action item AI-11** (P2): Sau khi có lớp primitive (AI-5), designer thêm lớp component cho các element trọng tâm: Button, Card, ListItem, TextField. Có thể defer V1.1 nếu schedule chặt.

---

## 5. Những điểm tích cực — KEEP

Đừng để feedback negative làm mất phần giỏi của handoff. Những phần dưới đây **đã đạt chất lượng cao**, giữ nguyên cho v1.1:

✅ **Motion specs (D11)** — file `motion-specs.md` rất tốt: 10 animations, có Compose easing names, có reduce-motion handling, có scope per animation. Vượt yêu cầu.

✅ **Localization (D12)** — 208 keys, format đúng (`key, vi, en, context, screen`), placeholder `{name}` đúng convention, sample data realistic VN.

✅ **Color tokens** — palette dark-first đúng, thêm `text/tertiary` và `text/disabled` mà spec không có (cải tiến hợp lý).

✅ **Spacing & radius tokens** — đầy đủ, kebab-case, bội số 4. Giữ nguyên.

✅ **Icon style consistency** — 45 icon đều stroke-style, viewBox 24, currentColor, file size dưới 1 KB mỗi cái — vượt budget 4 KB.

✅ **README + HTML hub** — chỉn chu, có self-checklist, có versioning protocol, có Q&A protocol. Designer thể hiện tinh thần làm việc nghiêm túc.

✅ **Sample content** — tên file/folder Vietnamese realistic ở localization, đúng instruction §13.

---

## 6. Action items tổng hợp + ưu tiên

| ID | Vấn đề | Ưu tiên | Owner | Ước lượng |
|----|--------|---------|-------|-----------|
| AI-1 | Chốt brand chính thức + rename | P0 | PM + Brand | 1 tuần |
| AI-2 | Khôi phục 4 module P0 (QR / OAuth / Update / Downloads) | P0 | PM + Designer | bao gồm trong AI-9 |
| AI-3 | Sửa giả định Play Store → internal APK distribution | P0 | PM | 0.5 ngày (nói chuyện) |
| AI-4 | Cung cấp Figma URL hoặc HTML mockup interactive trong package | P0 | Designer | 0.5 ngày |
| AI-5 | Tokens 3 lớp primitive/semantic/component | P1 | Designer | 1 ngày |
| AI-6 | Bump typography sizes về theo spec | P0 | Designer | 0.5 ngày |
| AI-7 | Thêm `ic_notification`, fix `ic_oauth_google` SVG | P1 | Designer | 0.5 ngày |
| AI-8 | Bổ sung 10 components còn thiếu | P0 | Designer | 3 ngày |
| AI-9 | Bổ sung 7 screens còn thiếu (S1a/c, S6, S7c/d/e/f) + đồng bộ S5b/e + S8 | P0 | Designer | 5 ngày |
| AI-10 | Prototype interactive 6 user flows | P1 | Designer | 2 ngày |
| AI-11 | Tokens layer component | P2 | Designer | có thể defer V1.1 |

**Path khẩn cấp**:
1. Ngày 0 (hôm nay): họp 60 phút PM + Tech Lead + Designer + Brand — chốt AI-1, AI-2, AI-3 ngay.
2. Ngày 1–2: Designer fix AI-3, AI-4, AI-6, AI-7 (toàn bộ < 2 ngày).
3. Ngày 3–10: Designer làm AI-8, AI-9 (component + screen còn thiếu).
4. Ngày 11: handoff v1.1 review meeting → expect Accept.

→ Tổng delay: **~2 tuần** so với kế hoạch ban đầu của Phase 2 (`07_lo-trinh-trien-khai.md` Phase 2 = 3 tuần). Có thể compensate bằng cách Engineering bắt đầu Foundation ngay với gói hiện tại trong khi designer fix.

---

## 7. Engineering có thể bắt đầu gì NGAY

Trong khi chờ designer fix, **Engineering không phải đợi không**. Có thể bắt đầu:

### 7.1 Tuần này

- [ ] Setup repo `fsnext-tv/` (Phase 0 spike — chưa làm).
- [ ] Build APK Hello World, cài thử Mi Box S + FPT Box.
- [ ] Convert tokens hiện tại sang Compose theme initial (sẽ phải refactor 1 lần khi tokens mới về).
- [ ] Convert 45 icon SVG sang VectorDrawable, kiểm tra render ở 24/32/40 dp.
- [ ] Wire Hilt + Navigation + base Activity.
- [ ] Setup Room DB schema (đã định nghĩa ở `02_khuyen-nghi-kien-truc.md` §2.3).
- [ ] Setup Retrofit + FshareApiService theo `02` §2.3.
- [ ] Bắt đầu auth module (logic, không UI — chờ design).

### 7.2 Tuần sau

- [ ] Bắt đầu module update theo `04_apk-va-update.md` (logic-first, UI follow design v1.1).
- [ ] Module download (logic-first).
- [ ] Setup CI/CD theo §4.10 của 04.

→ Phase 0 + một phần Phase 1 hoàn toàn unblock.

---

## 8. Format reply cho designer (template)

Để PM gửi designer, có thể paste nguyên block dưới (bằng tiếng Việt):

```
Chào team Design,

Cảm ơn package handoff v1.0. Chất lượng kỹ thuật của những phần đã giao
(tokens, icons, motion, localization) đạt yêu cầu — KEEP nguyên cho v1.1.

Tuy nhiên có 3 vấn đề nền tảng phải resolve trước khi accept:

1. BRAND: tên app trong package là "StreamIX TV" — chưa được Brand/PM
   sign-off. Tên dự án là Fshare TV (hoặc FsNext TV). Cần họp brand
   để chốt tên trước khi tiếp tục.

2. SCOPE: 4 module P0 bị cắt khỏi V1 mà không có change request:
   - Login QR (S1a)
   - Login OAuth (S1c)
   - Downloads (S6 + S7c)
   - In-app update (S7e + S8 4 variant)
   Cả 4 đều BẮT BUỘC trong V1. Tham khảo lại spec 09 §4 và 10 §4.

3. PHÂN PHỐI: package giả định Play Store update. Dự án này là
   internal-only, phân phối qua APK sideload. Tham khảo tài liệu 04
   "APK & In-App Update — Internal Deployment Playbook".

Chi tiết action items trong tài liệu 11_review-handoff-v1.md
(có 11 AI sắp xếp theo ưu tiên).

Path đề xuất: họp 60 phút hôm nay để chốt AI-1/2/3, sau đó designer
fix trong ~10 ngày → handoff v1.1 review → Accept.

Engineering bắt đầu Foundation ngay với assets hiện tại — sẽ refactor
khi v1.1 về.

Trân trọng,
Tech Lead
```

---

## 9. Kết luận

Gói v1.0 thể hiện **chuyên môn cao** ở phần đã làm — tokens, motion, localization, icon set là production-quality. Nhưng deviation về **scope** và **brand** quá lớn để accept ở vai trò product handoff.

**Verdict**: BLOCK. Re-issue v1.1 sau ~10 ngày, expect Accept.

Engineering **không idle** trong thời gian chờ — Phase 0 + một phần Phase 1 hoàn toàn unblock.

— Hết audit handoff v1.0 —
