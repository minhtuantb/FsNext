# Manual Test Checklist — các lớp KHÔNG tự động hoá được

Danh sách các lớp engine / network / threaded-worker mà unit test (CTest) không
phủ tự động được, kèm lý do và cách kiểm thủ công. Cập nhật theo Lô2 (2026-05-29).

> Lý do chung: các lớp này cần I/O mạng thật (libcurl), OAuth loopback redirect,
> hoặc chạy trên worker-thread riêng với event loop — không deterministic trong
> môi trường CI nên dễ flaky. Thay vì mock cả tầng dưới, ta kiểm thủ công.

| Lớp | Vì sao không tự động hoá | Cách kiểm thủ công |
|-----|--------------------------|--------------------|
| `DownloadEngine` | libcurl I/O thật, multi-segment + resume, ghi file lên đĩa; tốc độ/segment phụ thuộc mạng | Tải 1 file lớn (>100MB) từ Fshare; tạm dừng giữa chừng rồi resume; kiểm tra file checksum khớp và HUD hiển thị tiến độ/segment đúng |
| `UploadEngine` | libcurl chunked upload + resume, phụ thuộc server Fshare và token VIP | Upload 1 file >50MB; ngắt mạng giữa chừng rồi nối lại; xác nhận resume tiếp tục, không upload lại từ đầu, link trả về mở được |
| `FileService` | Gọi FshareApi thật (list/create/delete/move/rename) qua HTTP | Đăng nhập, duyệt thư mục, tạo/xoá/đổi tên/di chuyển; xác nhận cache-first hiển thị ngay rồi đồng bộ với server |
| `OAuthService` | OAuth loopback redirect (LoopbackServer mở cổng localhost), mở trình duyệt thật | Bấm Login → trình duyệt mở trang Fshare → đăng nhập → redirect về app → xác nhận user info hiển thị |
| `SyncService` | Quét folder + so khớp + đẩy TransferService, chạy theo timer/FS-watch trên worker | Bật auto-sync 1 folder; thêm/sửa file cục bộ; xác nhận file mới được phát hiện và upload, log activity cập nhật |
| `FileSyncWorker` | Chạy trên thread riêng, đồng bộ cache SQLite với API theo nền | Duyệt 1 thư mục lớn (>500 file); xác nhận danh sách hiển thị từ cache trước, sau đó refresh nền không treo UI |
| `LoopbackServer` | Lắng nghe cổng localhost cho OAuth redirect; phụ thuộc cổng trống + trình duyệt | Gián tiếp qua luồng Login ở trên; nếu cổng bận, xác nhận app báo lỗi/đổi cổng gọn gàng |

## Đã tự động hoá ở Lô2 (tham khảo)

Pure-logic / header-only / repo (SQLite + QSettings) đã có unit test ổn định
(25/25 stress, không flaky): FormatUtil, FileTypeHelper, Pkce, SecureStore (DPAPI
round-trip trên Windows), AppError, ApiResponse, AppSettings, FileItem,
TransferTask, User, SyncFolder, HistoryRepository, SyncRepository,
PriorityScheduler, LanguageViewModel, UserInfoViewModel (sync getter/NOTIFY).
