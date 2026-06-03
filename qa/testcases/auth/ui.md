# Auth / Login — UI test cases

> Loại: **UI** (render màn login + gating + trạng thái + i18n). Màn: `LoginView.qml` ↔ `AuthViewModel`.
> Cờ ⚠️ = case rủi ro cao (security/fail-closed). Tổng: **11 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| AUTH-UI-001 | Render đầy đủ phần tử login | App khởi động, chưa đăng nhập | 1) Mở app 2) Chụp màn login | Hiện: ô email, ô password, nút "Đăng nhập ›", checkbox "Giữ đăng nhập", link "Quên mật khẩu?", "Đăng ký ngay →", 3 nút social (Google/Facebook/Zalo), footer "Kết nối an toàn · TLS 1.3 · SRP" | UI-driver | P1 | no | — |
| AUTH-UI-002 | Nút Đăng nhập disabled khi thiếu input | Màn login | 1) Để trống email hoặc password 2) Quan sát nút | Nút "Đăng nhập" **disabled** khi `email==""` hoặc `password==""`; chỉ enable khi đủ cả 2 | qmltest/UI-driver | P1 | no | — |
| AUTH-UI-003 | Trạng thái đang đăng nhập | Đã nhập email+pass hợp lệ | 1) Bấm Đăng nhập 2) Quan sát ngay | `isLoading=true` → nút đổi text "Đang đăng nhập…" + spinner, nút + 3 social bị disable; KHÔNG bấm lại được | UI-driver | P1 | no | — |
| AUTH-UI-004 ⚠️ | Sai mật khẩu → báo lỗi, fail-closed | Màn login | 1) Nhập sai mật khẩu 2) Đăng nhập | `errorMessage` hiển thị dưới ô password; **KHÔNG** chuyển màn chính; nút enable lại; `isLoading→false`; mật khẩu KHÔNG lộ ra log | UI-driver | P0 | no | — |
| AUTH-UI-005 | Password ẩn (echo) | Màn login | 1) Gõ vào ô mật khẩu | Ký tự hiển thị dạng `••••` (echoMode Password), không hiện plaintext | UI-driver | P2 | no | — |
| AUTH-UI-006 | Toggle "Giữ đăng nhập" | Màn login | 1) Click checkbox | `rememberMe` đổi true/false; checkbox hiện gradient+✓ khi bật, chỉ border khi tắt | qmltest/UI-driver | P2 | no | — |
| AUTH-UI-007 | Đăng nhập thành công → vào Home | Account test hợp lệ | 1) Nhập đúng 2) Đăng nhập | `isLoggedIn=true` → chuyển sang màn chính (sidebar + HomePage); không còn LoginView | UI-driver | P0 | no | — |
| AUTH-UI-008 | Link Quên mật khẩu / Đăng ký | Màn login | 1) Click "Quên mật khẩu?" 2) Click "Đăng ký ngay" | Mở trình duyệt đúng URL (`FsExternalLinks.forgotPassword` / `.signup`); app không crash, không rời màn login | UI-driver | P2 | no | — |
| AUTH-UI-009 | Social login mở loopback | Màn login | 1) Click Google/Facebook/Zalo | Gọi `loginWithGoogle/Facebook/FptId()`; mở browser flow loopback; quay lại app xử lý success/fail | tay | P2 | no | — |
| AUTH-UI-010 | i18n — chuỗi bọc qsTr | — | 1) Grep `LoginView.qml` | Mọi label/nút/placeholder/footer bọc `qsTr()`; không hardcode trần | tay | P1 | no | — |
| AUTH-UI-011 | Runtime switch sang EN | App màn login | 1) Đổi ngôn ngữ sang EN | Toàn bộ chuỗi login dịch EN, không sót VI; layout không vỡ | UI-driver | P2 | no | — |
