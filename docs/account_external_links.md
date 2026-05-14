# Account External Links — Redirect to fshare.vn

**Status:** Implementation in code (`qml/Fshare/Utils/FsExternalLinks.qml`).
**Decision date:** 2026-05-14
**Decision owner:** Product team (URL spec confirmed).

---

## 1. Rationale

Các tính năng nhạy cảm về bảo mật và quản lý tài khoản Fshare **được thực hiện trên fshare.vn website** thay vì re-implement trong desktop client. Lý do:

| Lý do | Giải thích |
|---|---|
| **OTP / Captcha** | Browser quản lý OTP via SMS, captcha images, autofill từ password manager — đều khó implement đúng trong Qt desktop. |
| **Cookie session** | User đã có cookie fshare.vn từ trước — bypass thêm 1 lần đăng nhập trong webview. |
| **Update cadence** | Web platform update form/UI nhanh hơn desktop. Desktop chỉ cần update link slug khi web đổi route. |
| **Audit / compliance** | Hành động security-critical leave audit trail bên server side. |
| **Maintenance** | Single source of truth — 1 file QML chứa mọi URL. |

---

## 2. Implementation

### Single source of truth

File `qml/Fshare/Utils/FsExternalLinks.qml` (singleton) chứa tất cả URL.

```qml
import Fshare.Utils 1.0

MouseArea {
    onClicked: Qt.openUrlExternally(FsExternalLinks.changePassword)
}
```

### Design decision: Collapsed to single landing page

Product confirmed (2026-05-14): các sub-feature dưới đây đều **redirect về cùng URL** `https://www.fshare.vn/account/profile`. Web sẽ surface section tương ứng trên page đó. Lý do:

- Không cần backend team đảm bảo từng deep link route tồn tại
- Web Profile page evolve không break desktop links
- UX: user thấy label đúng intent → click → web page có section phù hợp

Các feature collapse vào `/account/profile`:
- Đổi mật khẩu
- Đổi email / số điện thoại / avatar
- Xác thực 2 lớp (2FA TOTP/SMS)
- Lịch sử VIP / payment

### Features ĐÃ LOẠI BỎ (per product decision)

3 feature dưới đây không expose trong desktop client (do Fshare web hiện không có hoặc chưa cần):

| Feature | Lý do bỏ |
|---|---|
| **Active sessions** (danh sách thiết bị đăng nhập) | Fshare web chưa hỗ trợ — bỏ nút trong UserInfoPage |
| **Linked accounts** (unlink OAuth bindings) | Fshare web chưa hỗ trợ — bỏ nút |
| **Delete account** | Safety — không expose trong UI; user muốn xoá thì liên hệ support |

Đã xoá khỏi:
- `FsExternalLinks.qml` (3 property)
- `UserInfoPage.qml` (3 AcctLink)
- Bảng URL trong tài liệu này

---

## 3. Final URL table

| Property | URL | Trạng thái |
|---|---|:---:|
| `home` | `https://www.fshare.vn/` | ✅ confirmed |
| `login` | `https://www.fshare.vn/login` | ✅ confirmed |
| `upgrade` | `https://www.fshare.vn/upgrade` | ✅ confirmed |
| `profile` | `https://www.fshare.vn/account/profile` | ✅ confirmed |
| `changePassword` | `https://www.fshare.vn/account/profile` | ✅ confirmed (collapsed) |
| `changeEmail` | `https://www.fshare.vn/account/profile` | ✅ confirmed (collapsed) |
| `editProfile` | `https://www.fshare.vn/account/profile` | ✅ confirmed (collapsed) |
| `twoFactor` | `https://www.fshare.vn/account/profile` | ✅ confirmed (collapsed) |
| `vipHistory` | `https://www.fshare.vn/account/profile` | ✅ confirmed (collapsed) |
| `paymentHistory` | `https://www.fshare.vn/account/profile` | ✅ confirmed (collapsed) |
| `support` | `https://www.fshare.vn/` | ✅ confirmed (homepage redirect) |
| `termsOfService` | `https://www.fshare.vn/` | ✅ confirmed (homepage redirect) |
| `privacyPolicy` | `https://www.fshare.vn/` | ✅ confirmed (homepage redirect) |
| `signup` | `https://www.fshare.vn/` | ✅ confirmed (homepage redirect) |
| `forgotPassword` | `https://www.fshare.vn/` | ✅ confirmed (homepage redirect) |

→ Tất cả URL đã được product team xác nhận. **Không còn placeholder.**

---

## 4. UI inventory

### LoginView

| Phần tử | URL property | File |
|---|---|---|
| "Đăng ký ngay →" link | `signup` | `qml/FsAurora/Pages/LoginView.qml` |
| "Quên mật khẩu?" link | `forgotPassword` | Same file |

### UserInfoPage — Section "Quản lý tài khoản"

Grid 2 cột × 2 hàng. Tất cả 4 nút redirect về `/account/profile`:

| # | Icon | Label | Helper | URL property |
|---|---|---|---|---|
| 1 | `user` | Thông tin cá nhân | Đổi email, số điện thoại, avatar | `editProfile` |
| 2 | `key` | Đổi mật khẩu | Cần email/OTP xác nhận | `changePassword` |
| 3 | `shield` | Xác thực 2 lớp (2FA) | Bảo vệ thêm bằng OTP | `twoFactor` |
| 4 | `sparkle` | Lịch sử VIP & thanh toán | Gói đã mua, gia hạn, giao dịch | `vipHistory` |

Footer note: *"Các thao tác bảo mật (đổi mật khẩu, 2FA, quản lý thiết bị) được thực hiện trên fshare.vn để đảm bảo OTP và xác minh hoạt động chính xác."*

### Sidebar (đã có sẵn từ trước session này)

| Phần tử | URL hiện tại | Đề xuất refactor |
|---|---|---|
| Hero card "Xem ưu đãi" | `"https://www.fshare.vn/upgrade"` (inline) | Đổi sang `FsExternalLinks.upgrade` |
| HomePage "Nâng cấp" CTA | `"https://www.fshare.vn/upgrade"` (inline) | Đổi sang `FsExternalLinks.upgrade` |

---

## 5. Câu hỏi UX còn lại

### Q1. Khi user đổi mật khẩu trên web → desktop session?

Hiện tại desktop session vẫn dùng token cũ → tiếp tục hoạt động đến lần API call kế tiếp, lúc đó Fshare trả 201 (session expired) → desktop tự logout + show toast.

**Hành vi này OK?** Hay cần aggressive logout ngay sau khi user nhấn "Đổi mật khẩu" trên desktop?

Khuyến nghị: giữ nguyên (lazy invalidation). Nếu user đổi mật khẩu xong còn muốn dùng desktop session cũ (vd. chưa logout ý định), force logout là quá invasive.

### Q2. Universal/deep link callback (P3, long-term)

Khi user xong việc trên web fshare.vn, web có thể redirect về `fsnext://refresh` custom protocol để app tự refresh user info. Cần:
- Backend (web team) thêm support cho custom redirect
- Installer (`installer.iss`) register protocol handler

Defer cho v6.1+.

---

## 6. Test plan

| # | Action | Expected |
|---|---|---|
| 1 | LoginView → click "Đăng ký ngay" | Browser mở `signup` URL |
| 2 | LoginView → click "Quên mật khẩu?" | Browser mở `forgotPassword` URL |
| 3 | UserInfoPage → click 4 nút trong "Quản lý tài khoản" | Browser mở `/account/profile` (cùng URL cho cả 4) |
| 4 | Sidebar → click "Xem ưu đãi" | Browser mở `/upgrade` |
| 5 | Test với user chưa login fshare.vn trên browser | Web redirect về `/login` → đăng nhập → quay lại target page |
| 6 | Test với user đã login fshare.vn | Direct mở target page, không bounce |
| 7 | Test trên Windows / macOS / Linux | `Qt.openUrlExternally` dùng OS default browser — verify all 3 OS |
| 8 | Keyboard nav: Tab → focus card → Space/Enter | Mở browser giống click |
| 9 | Screen reader (NVDA): focus button → đọc label + helper | Accessible.role: Link + name + description đã set |

---

## 7. Maintenance procedure

Khi Fshare đổi URL slug:

1. Mở `qml/Fshare/Utils/FsExternalLinks.qml`
2. Update property tương ứng
3. Đổi marker `⚠ UNVERIFIED` thành `✅ confirmed` + ghi date
4. Update bảng URL trong tài liệu này
5. Rebuild — không cần đụng vào page QML nào

**Quy ước:** Tránh hard-code URL inline trong page QML — luôn thông qua singleton. Hiện vẫn còn 2 chỗ inline (`Main.qml:310` và `Main.qml:456`) — refactor ở next pass.

---

## 8. Files đã thay đổi (session 2026-05-14)

| File | Loại |
|---|---|
| `qml/Fshare/Utils/FsExternalLinks.qml` | 🆕 Singleton (14 URL properties + 2 function builders) |
| `qml/Fshare/Utils/qmldir` | ✏ +1 dòng singleton registration |
| `qml/FsAurora/Pages/LoginView.qml` | ✏ Wire signup + forgotPassword + import Fshare.Utils |
| `qml/Fshare/Pages/UserInfoPage.qml` | ✏ Thêm section "Quản lý tài khoản" — 4 nút redirect /account/profile |
| `docs/account_external_links.md` | 🆕 Tài liệu này |
