---
description: Chạy qmllint lên mọi file .qml đang thay đổi trong working tree (git diff), báo cáo lỗi/cảnh báo.
allowed-tools: Bash(git status:*), Bash(git diff:*), Bash(*qmllint*), Glob, Read
---

Lint các file QML đang sửa, không cần build.

1. Liệt kê file `.qml` thay đổi: `git status --porcelain | findstr ".qml"` (gồm cả untracked `??` và modified `M`).
2. Với mỗi file, chạy qmllint từ bản Qt mingw, luôn truyền `-I qml`:
   ```
   "C:\Qt\6.8.3\mingw_64\bin\qmllint.exe" -I qml <đường-dẫn-file.qml>
   ```
   Nếu báo thiếu module Qt, thêm `-I "C:\Qt\6.8.3\mingw_64\qml"`.
3. Tổng hợp: liệt kê từng file → PASS / có WARNING / có ERROR, kèm dòng lỗi. Bỏ qua cảnh báo nhiễu không liên quan (vd chưa resolve được type do context property runtime) nhưng vẫn nêu để user biết.

Nếu không có file QML nào thay đổi, báo "Không có QML thay đổi" và dừng. Trả lời tiếng Việt.
