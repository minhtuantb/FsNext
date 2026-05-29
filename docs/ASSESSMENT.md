# FsNext — Đánh giá kiến trúc & Khuyến nghị

> Đánh giá độc lập của một kiến trúc sư C++/desktop, dựa trên code thực tế ngày **2026-05-29** (v6.x).
> Thay thế `docs/08_assessment_and_roadmap.md` (tháng 4, đã xóa).

## Tóm tắt điều hành

**Sức khỏe codebase: TỐT.** Đây là một ứng dụng desktop trưởng thành, kiến trúc sạch (MVVM + clean layering),
DI tập trung qua `AppContext`, không còn global state. Các subsystem khó (transfer scheduling, OAuth + silent
refresh, cache-first file manager) được thiết kế chỉn chu và đã chạy production. Phần lớn rủi ro còn lại là **nợ kỹ
thuật tích tụ qua nhiều đợt cập nhật nhanh** (2 design-system, audit chưa đóng, test chưa phủ phần lõi) chứ không
phải lỗi thiết kế nền tảng.

## Điểm mạnh

1. **Tách tầng & DI rõ ràng.** `AppContext::init()` là một bản đồ phụ thuộc tường minh, thứ tự khởi tạo có chủ đích
   (comment giải thích vì sao coordinator tạo sớm, vì sao HUD VM tạo cuối). Dễ đọc, dễ test từng tầng.
2. **Transfer subsystem đáng nể.** `TransferOrchestrator` (own thread) + `BudgetManager` (non-QObject, lock-free) +
   3 `PriorityScheduler` + engine libcurl resume/multi-segment. Việc cho metadata-prefetch và sync-upload **dùng chung
   một budget** là một quyết định tốt: chống starvation mà không cần điều phối thủ công.
3. **Auth chắc chắn.** OAuth2 loopback RFC 8252 (PKCE + state), silent refresh single-flight với 3 outcome rõ ràng
   (Success/HardFail/SoftFail) và ownership token tập trung ở coordinator — tránh được lớp lỗi "refresh đua nhau".
4. **Cache-first UX.** SQLite + FTS5 + write-through optimistic làm file manager phản hồi tức thì kể cả mạng chậm.
5. **Khả năng chịu lỗi.** Resume download/upload qua sidecar, history persistence, session-expired flow có toast,
   single-instance, fail-open cho bad-word filter.
6. **Test có nền móng.** 8 unit test phủ cache DB/service, budget manager, speed meter, fshare URL, filename
   sanitizer/resolver, proxy resolve — nhiều hơn mức thường thấy ở app desktop nội bộ.

## Rủi ro & Khuyến nghị (theo ưu tiên)

### P1 — Nên xử lý sớm

**1. Hai design-system QML song song (`FsAurora` + `Fshare`).**
`FsAurora.Theme` là theme đang chạy, nhưng UI import lẫn lộn `Fshare.Components/Pages/Dialogs/Utils` và
`FsAurora.Components/Pages/Windows` (alias `Aurora*`). `qml/Fshare/Theme/*` đã bị xóa nhưng các component Fshare khác
vẫn dùng. Đây là nguồn nhầm lẫn (sửa nhầm bộ) và trùng lặp component.
→ *Khuyến nghị*: Chốt **một** bộ component là chuẩn (đề xuất: hợp nhất về `Fshare.*` cho component/page, giữ
`FsAurora.Theme` làm token). Lập bảng ánh xạ component trùng, migrate dần `FsAurora.Pages` (HomePage, LoginView) sang
Fshare. Tài liệu hóa ranh giới tạm thời trong CLAUDE.md (đã làm) để tránh hồi quy. Dọn artifact thiết kế
(`qml/FsAurora/*.html`, `design-canvas.jsx`, `handoff/`, `uploads/`) ra khỏi cây source runtime.

**2. Hai crash-audit chưa đóng.**
[docs/CRASH_AUDIT.md](CRASH_AUDIT.md) (47 findings) và [docs/FILE_MANAGER_CRASH_AUDIT.md](FILE_MANAGER_CRASH_AUDIT.md)
mô tả nhiều rủi ro nghiêm trọng (QML gọi method không tồn tại, race khi click nhanh, `FolderTreeModel` đệ quy không
cap depth) nhưng chưa được đánh dấu fixed/verified với code hiện tại.
→ *Khuyến nghị*: Một vòng review chuyên dụng (có thể dùng `/code-review` hoặc workflow verify) đối chiếu từng finding
với code 2026-05; đánh dấu Fixed / Won't-fix / Still-open ngay trong file; mở issue cho phần còn lại. Ưu tiên các mục
"race khi click nhanh" và "đệ quy không giới hạn độ sâu" vì chúng gây crash thực tế cho người dùng.

### P2 — Nên xử lý trong vài sprint tới

**3. Thread-safety HttpClient/cookie là bản vá tạm.**
Cookie/session trong `HttpClient` được bảo vệ bằng mutex theo kiểu band-aid. Hoạt động, nhưng nếu sau này refactor
session/cookie (vd. multi-account, đổi cách lưu cookie) thì mô hình hiện tại dễ vỡ.
→ *Khuyến nghị*: Khi chạm tới session/cookie, thiết kế lại sở hữu rõ ràng (một owner, truy cập qua snapshot
bất biến giống cách `RefreshTokenCoordinator` làm với token) thay vì khóa rải rác.

**4. Test chưa phủ phần lõi rủi ro cao.**
Đã có test cho cache/budget/util, nhưng **chưa có** cho: `TransferOrchestrator` dispatch + budget interplay dưới tải,
`RefreshTokenCoordinator` (single-flight, 3 outcome, cold-start 7-day), và write-through correctness của
`FileCacheService` khi API fail. Đây đúng là những chỗ bug sẽ đắt nhất.
→ *Khuyến nghị*: Thêm test cho orchestrator (giả lập producer + assert thứ tự dispatch/đếm slot), coordinator (giả lập
401 đồng thời → 1 refresh), và cache rollback. Ưu tiên coordinator vì lỗi ở đó = mất phiên đăng nhập của người dùng.

**5. i18n mới một phần.**
Nguồn tiếng Việt, mới có English; chưa rà chuỗi hardcode trong QML.
→ *Khuyến nghị*: Chạy `lupdate` + audit chuỗi literal trong `qml/`; quyết định có hỗ trợ thêm ngôn ngữ không trước
khi UI phình to.

### P3 — Cải thiện dài hạn

**6. `SecureStore` non-Windows trả plaintext.** Chỉ ảnh hưởng khi port macOS/Linux. Khi đó cần Keychain/libsecret.

**7. Tài liệu dễ drift.** Nguyên nhân gốc của đợt dọn này: tài liệu trạng thái/kế hoạch sinh ra theo sprint rồi không
ai cập nhật. → *Khuyến nghị*: Quy ước "tài liệu sống" (ARCHITECTURE/ASSESSMENT/BACKLOG) vs "snapshot dùng-một-lần"
(verification report, PR description) — snapshot không commit vào `docs/`, hoặc bỏ ngay sau khi merge. CLAUDE.md +
docs/README.md là điểm vào duy nhất.

## Việc đã làm trong đợt review này

- Gỡ `smartTV_deleted/` (221MB, 242 file) khỏi working tree (giữ trong git history).
- Xóa ~18 tài liệu lỗi thời (legacy v5.3.0, thiết kế migration tháng 4, report một-lần) + ~39 file rác ở root;
  bổ sung `.gitignore` chặn tái phát.
- Tạo tài liệu chuẩn: `CLAUDE.md`, `docs/ARCHITECTURE.md`, `docs/ASSESSMENT.md` (file này), `docs/README.md`.
- Tạo project skill `.claude/skills/fsnext-run` (build & run & verify).

Các mục P1–P3 ở trên **chưa** thực hiện (ngoài phạm vi đợt dọn tài liệu); đề xuất đưa vào `docs/BACKLOG.md`.
