---
description: Cổng chất lượng trước khi commit FsNext — review diff, lint QML thay đổi, build, chạy test, soát i18n.
---

Chạy quy trình kiểm tra trước commit cho thay đổi hiện tại. Báo cáo gọn theo từng bước, dừng và hỏi user nếu một bước fail nghiêm trọng.

1. **Tóm tắt diff**: `git status` + `git diff HEAD --stat`. Liệt kê ngắn những gì đã đổi.

2. **Review code**: gọi subagent **cpp-qt-reviewer** trên diff hiện tại. Nếu diff có thêm ViewModel, gọi thêm **mvvm-wiring-checker**. Tổng hợp phát hiện; nếu có P0/P1 → khuyến cáo sửa trước khi commit.

3. **i18n**: nếu diff có thêm/sửa chuỗi hiển thị (QML `qsTr` / C++ `tr`), nhắc user chạy skill `fsnext-i18n` để lupdate + dịch EN. Soát nhanh xem có chuỗi nào QUÊN bọc `tr()/qsTr()` không.

4. **Lint QML**: nếu có `.qml` thay đổi, chạy nội dung command `/lint-qml-changed`.

5. **Build**: dùng skill `fsnext-run` để build Release. Nếu fail → dừng, báo lỗi build (đây là blocker).

6. **Test**: `ctest --test-dir build --output-on-failure`. Báo PASS/FAIL từng test. Lưu ý test auth async hay flaky (xem memory) — nếu fail đúng test đó, nêu khả năng race chứ đừng kết luận hỏng vội.

Cuối cùng đưa **verdict**: ✅ sẵn sàng commit / ⚠️ commit được nhưng có lưu ý / ❌ chưa nên commit (lý do). KHÔNG tự commit — để user quyết.
