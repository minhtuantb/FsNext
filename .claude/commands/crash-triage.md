---
description: Chẩn đoán crash FsNext — đọc fsnext.log + Windows Application event log, tương quan với code/async site gần đây.
allowed-tools: Bash, PowerShell, Glob, Grep, Read
argument-hint: [từ khóa/đặc tả hiện tượng, vd "crash khi cancel upload"]
---

Chẩn đoán crash/lỗi runtime của FsNext. Hiện tượng quan sát (nếu user mô tả): $ARGUMENTS

1. **Log ứng dụng** — đọc đuôi log tại `%APPDATA%\FPT\FsNext\fsnext.log`:
   ```powershell
   $log = "$env:APPDATA\FPT\FsNext\fsnext.log"
   if (Test-Path $log) { Get-Content $log -Tail 120 } else { "Không có log" }
   ```
   Lọc dấu hiệu nghiêm trọng: `crash|fatal|assert|abort|exception|qFatal|qCritical|terminate|SEH|backtrace|unresolved|undefined`.

2. **Windows Application event log** — crash native (access violation, 0xc0000005…) thường chỉ hiện ở đây, không vào fsnext.log:
   ```powershell
   Get-WinEvent -FilterHashtable @{LogName='Application'; ProviderName='Application Error'; StartTime=(Get-Date).AddHours(-24)} -ErrorAction SilentlyContinue |
     Where-Object { $_.Message -match 'FsNext' } | Select-Object -First 5 TimeCreated, Id, Message | Format-List
   ```
   Lấy faulting module + offset nếu có.

3. **Tương quan code**: từ dòng log/triệu chứng + thời điểm, khoanh vùng subsystem. So với `git log --oneline -10` và `git diff HEAD` xem code vừa đổi có dính không. Vì bug-class #1 của repo là **async-lambda thiếu QPointer guard** và race lúc shutdown, ưu tiên soi các site async trong vùng nghi ngờ. Nếu nghi race/lifetime/deadlock cả subsystem → gọi subagent **concurrency-auditor** cho module đó.

4. **Kết luận**: nguyên nhân khả dĩ nhất (kèm bằng chứng `file:line`), các giả thuyết thay thế, và bước tái hiện/chứng minh đề xuất. Không tự sửa — báo cáo để user quyết. Trả lời tiếng Việt.

Nếu cả 2 nguồn log đều trống: nói rõ "không có dấu vết crash trong 24h" và gợi ý bật log chi tiết / chạy lại để tái hiện.
