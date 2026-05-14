<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<!--
    fshare_en.ts — English translations for Fshare Tool (FsNext)

    Source language : vi_VN  (Vietnamese — source strings in qsTr() are Vietnamese)
    Target language : en_US  (English)

    HOW TO UPDATE:
      1. After adding/changing qsTr() strings in QML or tr() in C++, run:
             lupdate qml/ src/ -ts src/i18n/fshare_en.ts
         This adds new <message> entries with empty <translation> tags.
      2. Fill in the English translation for each new entry.
      3. Recompile:
             lrelease src/i18n/fshare_en.ts -qm output/translations/fshare_en.qm
         Or rebuild the project (CMake hooks lrelease into POST_BUILD).

    GUIDELINES:
      - Keep messages short and customer-friendly.
      - Do NOT add technical error codes or stack traces.
      - Brand names (Fshare, VIP, FPT ID, Google, Facebook) are never translated.
      - Units (MB/s, GB, KB, %) are never translated.
      - "Email" stays in English in both languages.
      - Mark a translation as <translation type="unfinished"> when not yet reviewed.
-->
<TS version="2.1" language="en_US" sourcelanguage="vi_VN">

<!-- ═══════════════════════════════════════════════════════════════════
     Main.qml — Root window: login form + sidebar shell
     ═══════════════════════════════════════════════════════════════════ -->
<context>
    <name>Main</name>

    <!-- Login form -->
    <message>
        <source>Đăng nhập vào tài khoản của bạn</source>
        <translation>Sign in to your account</translation>
    </message>
    <message>
        <source>Mật khẩu</source>
        <translation>Password</translation>
    </message>
    <message>
        <source>Ghi nhớ đăng nhập</source>
        <translation>Remember me</translation>
    </message>
    <message>
        <source>Đang đăng nhập…</source>
        <translation>Signing in…</translation>
    </message>
    <message>
        <source>Đăng nhập</source>
        <translation>Sign In</translation>
    </message>
    <message>
        <source>hoặc tiếp tục với</source>
        <translation>or continue with</translation>
    </message>

    <!-- Sidebar navigation -->
    <message>
        <source>Tải xuống</source>
        <translation>Downloads</translation>
    </message>
    <message>
        <source>Tải lên</source>
        <translation>Uploads</translation>
    </message>
    <message>
        <source>File</source>
        <translation>Files</translation>
    </message>
    <message>
        <source>Tài khoản</source>
        <translation>Account</translation>
    </message>
    <message>
        <source>Cài đặt</source>
        <translation>Settings</translation>
    </message>

    <!-- User card fallback -->
    <message>
        <source>Người dùng</source>
        <translation>User</translation>
    </message>

    <!-- Session toast -->
    <message>
        <source>Phiên đăng nhập hết hạn</source>
        <translation>Session expired</translation>
    </message>
    <message>
        <source>Vui lòng đăng nhập lại để tiếp tục.</source>
        <translation>Please sign in again to continue.</translation>
    </message>
</context>

<!-- ═══════════════════════════════════════════════════════════════════
     DownloadPage.qml
     ═══════════════════════════════════════════════════════════════════ -->
<context>
    <name>DownloadPage</name>

    <!-- Header -->
    <message>
        <source>Tải xuống</source>
        <translation>Downloads</translation>
    </message>
    <message>
        <source>Đã tải xong (%1)</source>
        <translation>Completed (%1)</translation>
    </message>
    <message>
        <source>Đang tải (%1)</source>
        <translation>Downloading (%1)</translation>
    </message>
    <message>
        <source>Đang tải</source>
        <translation>Active</translation>
    </message>
    <message>
        <source>Lịch sử</source>
        <translation>History</translation>
    </message>
    <message>
        <source>Thêm tải xuống</source>
        <translation>Add Download</translation>
    </message>

    <!-- Folder scan banner -->
    <message>
        <source>Đang quét thư mục "%1"…</source>
        <translation>Scanning folder "%1"…</translation>
    </message>
    <message>
        <source>Đã tìm thấy %1 file</source>
        <translation>Found %1 file(s)</translation>
    </message>
    <message>
        <source>Hủy</source>
        <translation>Cancel</translation>
    </message>

    <!-- Toolbar -->
    <message>
        <source>Tạm dừng tất cả</source>
        <translation>Pause All</translation>
    </message>
    <message>
        <source>Tiếp tục tất cả</source>
        <translation>Resume All</translation>
    </message>
    <message>
        <source>Xóa lịch sử</source>
        <translation>Clear History</translation>
    </message>

    <!-- Empty states -->
    <message>
        <source>Chưa có tác vụ tải xuống</source>
        <translation>No active downloads</translation>
    </message>
    <message>
        <source>Dán link Fshare để bắt đầu tải.</source>
        <translation>Paste a Fshare link to start downloading.</translation>
    </message>
    <message>
        <source>Chưa có lịch sử tải xuống</source>
        <translation>No download history</translation>
    </message>
    <message>
        <source>Các file đã tải xong sẽ hiển thị ở đây.</source>
        <translation>Completed downloads will appear here.</translation>
    </message>

    <!-- System-folder block toast -->
    <message>
        <source>Không thể tải vào thư mục hệ thống</source>
        <translation>Cannot download to a system folder</translation>
    </message>

    <!-- Add Download dialog -->
    <message>
        <source>Link Fshare</source>
        <translation>Fshare Links</translation>
    </message>
    <message>
        <source>Dán link vào đây — mỗi link một dòng
https://www.fshare.vn/file/…</source>
        <translation>Paste links here — one per line
https://www.fshare.vn/file/…</translation>
    </message>
    <message>
        <source>%1 link</source>
        <translation>%1 links</translation>
    </message>
    <message>
        <source>Thư mục lưu</source>
        <translation>Save folder</translation>
    </message>
    <message>
        <source>Để trống sẽ dùng thư mục mặc định.</source>
        <translation>Leave blank to use the default folder.</translation>
    </message>
    <message>
        <source>Chọn thư mục tải xuống</source>
        <translation>Choose download folder</translation>
    </message>
    <message>
        <source>Mật khẩu (nếu có)</source>
        <translation>Password (optional)</translation>
    </message>
    <message>
        <source>Nhập mật khẩu nếu file được bảo vệ</source>
        <translation>Enter file password if protected</translation>
    </message>
    <message>
        <source>Tải xuống</source>
        <translation>Download</translation>
    </message>
</context>

<!-- ═══════════════════════════════════════════════════════════════════
     UploadPage.qml
     ═══════════════════════════════════════════════════════════════════ -->
<context>
    <name>UploadPage</name>

    <message>
        <source>Tải lên</source>
        <translation>Uploads</translation>
    </message>
    <message>
        <source>Đã tải lên (%1)</source>
        <translation>Completed (%1)</translation>
    </message>
    <message>
        <source>Đang tải lên (%1)</source>
        <translation>Uploading (%1)</translation>
    </message>
    <message>
        <source>Đang tải</source>
        <translation>Active</translation>
    </message>
    <message>
        <source>Lịch sử</source>
        <translation>History</translation>
    </message>
    <message>
        <source>Chọn file</source>
        <translation>Upload Files</translation>
    </message>
    <message>
        <source>Tạm dừng tất cả</source>
        <translation>Pause All</translation>
    </message>
    <message>
        <source>Tiếp tục tất cả</source>
        <translation>Resume All</translation>
    </message>
    <message>
        <source>Xóa lịch sử</source>
        <translation>Clear History</translation>
    </message>
    <message>
        <source>Kéo file vào đây để tải lên</source>
        <translation>Drop files here to upload</translation>
    </message>
    <message>
        <source>Chưa có tác vụ tải lên</source>
        <translation>No active uploads</translation>
    </message>
    <message>
        <source>Nhấn &apos;Chọn file&apos; hoặc kéo file vào đây.</source>
        <translation>Click &apos;Upload Files&apos; or drag files here.</translation>
    </message>
    <message>
        <source>Chưa có lịch sử tải lên</source>
        <translation>No upload history</translation>
    </message>
    <message>
        <source>Các file đã tải lên sẽ hiển thị ở đây.</source>
        <translation>Completed uploads will appear here.</translation>
    </message>
</context>

<!-- ═══════════════════════════════════════════════════════════════════
     SettingsPage.qml
     ═══════════════════════════════════════════════════════════════════ -->
<context>
    <name>SettingsPage</name>

    <message>
        <source>Cài đặt</source>
        <translation>Settings</translation>
    </message>
    <message>
        <source>Tùy chỉnh tải xuống, tải lên và kết nối</source>
        <translation>Configure downloads, uploads, and connection</translation>
    </message>

    <!-- General -->
    <message>
        <source>Chung</source>
        <translation>General</translation>
    </message>
    <message>
        <source>Tự động đăng nhập khi khởi động</source>
        <translation>Auto-login at startup</translation>
    </message>
    <message>
        <source>Tự động đăng nhập bằng thông tin đã lưu</source>
        <translation>Automatically sign in with saved credentials</translation>
    </message>
    <message>
        <source>Luôn hiển thị trên cùng</source>
        <translation>Stay on top</translation>
    </message>
    <message>
        <source>Giữ cửa sổ ứng dụng luôn ở trên các cửa sổ khác</source>
        <translation>Keep the application window above others</translation>
    </message>
    <message>
        <source>Giao diện tối</source>
        <translation>Dark mode</translation>
    </message>
    <message>
        <source>Sử dụng giao diện màu tối</source>
        <translation>Use dark theme</translation>
    </message>

    <!-- Language -->
    <message>
        <source>Ngôn ngữ</source>
        <translation>Language</translation>
    </message>
    <message>
        <source>Ngôn ngữ hiển thị</source>
        <translation>Display language</translation>
    </message>
    <message>
        <source>Chọn ngôn ngữ giao diện ứng dụng</source>
        <translation>Choose the interface language</translation>
    </message>

    <!-- Transfer performance -->
    <message>
        <source>Hiệu suất tải</source>
        <translation>Transfer Performance</translation>
    </message>
    <message>
        <source>Số tải xuống đồng thời</source>
        <translation>Concurrent downloads</translation>
    </message>
    <message>
        <source>Số file tải xuống cùng lúc (1–16). Khuyến nghị: 8 cho tài khoản VIP, 4 cho tài khoản miễn phí.</source>
        <translation>Files downloading in parallel (1–16). Recommended: 8 for VIP, 4 for free accounts.</translation>
    </message>
    <message>
        <source>Luồng song song mỗi file</source>
        <translation>Segments per file</translation>
    </message>
    <message>
        <source>Số luồng HTTP Range mỗi file (1–32). Khuyến nghị: 16 — chia file thành các phần tải song song. Chỉ áp dụng cho file ≥ 2 MB trên CDN hỗ trợ Range request.</source>
        <translation>HTTP Range streams per file (1–32). Recommended: 16. Only applies to files ≥ 2 MB on CDN servers that support Range requests.</translation>
    </message>
    <message>
        <source>Số tải lên đồng thời</source>
        <translation>Concurrent uploads</translation>
    </message>
    <message>
        <source>Số file tải lên cùng lúc (1–8). Khuyến nghị: 4. Tải lên bị giới hạn bởi CPU/ổ đĩa; hơn 4 hiếm khi có thêm lợi ích.</source>
        <translation>Files uploading in parallel (1–8). Recommended: 4. Upload is CPU/disk-bound; more than 4 rarely helps.</translation>
    </message>
    <message>
        <source>Tối ưu cho tài khoản VIP: 8 file × 16 luồng = tối đa 128 kết nối HTTP/2 song song. Tài khoản miễn phí: giảm xuống 4 file × 8 luồng để tránh bị giới hạn tốc độ.</source>
        <translation>Optimal for VIP: 8 files × 16 segments = up to 128 parallel HTTP/2 streams. Free accounts: reduce to 4 × 8 to avoid rate limiting.</translation>
    </message>

    <!-- Download -->
    <message>
        <source>Tự động bắt đầu tải</source>
        <translation>Auto-start downloads</translation>
    </message>
    <message>
        <source>Bắt đầu tải ngay khi thêm vào danh sách</source>
        <translation>Start download immediately when added</translation>
    </message>
    <message>
        <source>Thư mục tải xuống</source>
        <translation>Download folder</translation>
    </message>
    <message>
        <source>Vị trí mặc định lưu file đã tải</source>
        <translation>Default location for downloaded files</translation>
    </message>
    <message>
        <source>Chọn thư mục tải xuống mặc định</source>
        <translation>Choose default download folder</translation>
    </message>

    <!-- Connection -->
    <message>
        <source>Kết nối</source>
        <translation>Connection</translation>
    </message>
    <message>
        <source>Chế độ proxy</source>
        <translation>Proxy mode</translation>
    </message>
    <message>
        <source>Cách kết nối qua proxy</source>
        <translation>How to connect through proxies</translation>
    </message>
    <message>
        <source>Không dùng</source>
        <translation>None</translation>
    </message>
    <message>
        <source>Hệ thống</source>
        <translation>System</translation>
    </message>
    <message>
        <source>Thủ công</source>
        <translation>Manual</translation>
    </message>
    <message>
        <source>Địa chỉ proxy</source>
        <translation>Proxy host</translation>
    </message>
    <message>
        <source>Tên máy chủ hoặc địa chỉ IP proxy</source>
        <translation>Proxy server hostname or IP address</translation>
    </message>
    <message>
        <source>Cổng proxy</source>
        <translation>Proxy port</translation>
    </message>
    <message>
        <source>Cổng máy chủ proxy (1–65535)</source>
        <translation>Proxy server port (1–65535)</translation>
    </message>

    <!-- About -->
    <message>
        <source>Thông tin</source>
        <translation>About</translation>
    </message>
    <message>
        <source>Phiên bản %1</source>
        <translation>Version %1</translation>
    </message>
</context>

<!-- ═══════════════════════════════════════════════════════════════════
     UserInfoPage.qml
     ═══════════════════════════════════════════════════════════════════ -->
<context>
    <name>UserInfoPage</name>

    <message>
        <source>Tài khoản</source>
        <translation>Account</translation>
    </message>
    <message>
        <source>Thông tin tài khoản và dung lượng Fshare</source>
        <translation>Your Fshare account information and quota</translation>
    </message>
    <message>
        <source>Dung lượng</source>
        <translation>Storage</translation>
    </message>
    <message>
        <source>Lưu lượng</source>
        <translation>Traffic</translation>
    </message>
    <message>
        <source>HẾT HẠN VIP</source>
        <translation>VIP EXPIRY</translation>
    </message>
    <message>
        <source>ĐIỂM TÍCH LŨY</source>
        <translation>POINTS</translation>
    </message>
</context>

<!-- ═══════════════════════════════════════════════════════════════════
     FileManagerPage.qml  (placeholder — run lupdate to populate)
     ═══════════════════════════════════════════════════════════════════ -->
<context>
    <name>FileManagerPage</name>
</context>

</TS>
