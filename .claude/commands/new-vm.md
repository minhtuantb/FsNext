---
description: Scaffold một ViewModel mới cho FsNext và wire đầy đủ qua AppContext (header + init + registerQml + CMake).
argument-hint: <TênViewModel> [mô tả ngắn vai trò]
---

Tạo ViewModel mới tên `$1` (nếu chưa có đuôi `ViewModel` thì thêm vào) cho FsNext, theo đúng MVVM của repo. Vai trò: $2

Thực hiện đầy đủ, KHÔNG bỏ bước nào:

1. **Tạo cặp `.h/.cpp`** trong `src/viewmodels/`:
   - Class kế thừa `QObject`, namespace `fsnext`, `Q_OBJECT`.
   - Expose state qua `Q_PROPERTY` (kèm getter + NOTIFY signal), hành động qua `Q_INVOKABLE`.
   - Business logic KHÔNG nằm ở đây — chỉ gọi xuống Service (nhận con trỏ non-owning qua constructor). Nếu vai trò cần service chưa tồn tại, hỏi user trước.
   - Mọi chuỗi hiển thị bọc `tr()`.
   - Mọi lambda async phải capture `QPointer` + `if (!guard) return;` (bug-class #1 của repo).

2. **Wire trong `src/app/AppContext.h`**: thêm member `std::unique_ptr<$1> m_<camel>VM;` cạnh các VM khác.

3. **Wire trong `src/app/AppContext.cpp`**:
   - `#include "viewmodels/$1.h"`.
   - Khởi tạo trong `init()` (đặt sau service phụ thuộc, đúng thứ tự lifetime): `m_<camel>VM = std::make_unique<$1>(...);`
   - Đăng ký trong `registerQml()`: `ctx->setContextProperty(QStringLiteral("<camel>ViewModel"), m_<camel>VM.get());`

4. **Cập nhật `CMakeLists.txt`**: thêm `src/viewmodels/$1.cpp` (và `.h` nếu danh sách header có liệt kê) vào danh sách source.

5. Sau khi xong, gọi subagent **mvvm-wiring-checker** để xác nhận 3 điểm wiring khớp, rồi tóm tắt cho user tên context property QML sẽ dùng (`<camel>ViewModel`).

Đừng build trong command này — để user tự chạy skill `fsnext-run`. Nếu thiếu thông tin (service phụ thuộc, property cần expose), hỏi trước khi tạo file.
