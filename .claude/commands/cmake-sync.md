---
description: Kiểm src/**/*.cpp trên đĩa có khớp danh sách source trong CMakeLists.txt không — chặn lỗi quên thêm source mới.
allowed-tools: Bash, Glob, Grep, Read
---

Trong FsNext, C++ dùng **danh sách source tường minh** (`set(FSNEXT_SOURCES ...)` trong `CMakeLists.txt`; test trong `tests/CMakeLists.txt`) — chỉ QML mới GLOB. Quên thêm `.cpp` mới = lỗi link khó hiểu / hàm "không tồn tại". Command này phát hiện lệch.

1. **Liệt kê .cpp thật trên đĩa** dưới `src/` (bỏ qua file gitignored như `OAuthSecrets`):
   ```
   git ls-files 'src/**/*.cpp'
   ```
   (dùng `git ls-files` để chỉ lấy file được track; thêm untracked `.cpp` mới qua `git status --porcelain` nếu có.)

2. **Trích danh sách trong CMake**: đọc block `set(FSNEXT_SOURCES ...)` trong `CMakeLists.txt` và source list trong `tests/CMakeLists.txt`. Lấy mọi đường dẫn `.cpp`.

3. **So sánh 2 tập**, báo cáo:
   - ❌ **Trên đĩa nhưng THIẾU trong CMake** (nguy hiểm — sẽ không được build/link). Đây là phát hiện chính.
   - ⚠️ **Trong CMake nhưng KHÔNG còn trên đĩa** (rác — đã xóa/đổi tên, sẽ lỗi configure).
   - Lưu ý: header `.h` thường không cần liệt kê (trừ khi CMake có list header riêng cho AUTOMOC/IDE) — chỉ cảnh báo `.h` nếu repo đang liệt kê header tường minh.

4. Với mỗi file thiếu, đưa đúng dòng cần thêm và vị trí hợp lý trong block (theo nhóm thư mục layer). KHÔNG tự sửa CMakeLists trừ khi user yêu cầu — mặc định chỉ báo cáo.

Nếu khớp hoàn toàn: báo "✅ CMake source list đồng bộ với đĩa". Trả lời tiếng Việt.
