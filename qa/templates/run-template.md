# Run: YYYY-MM-DD-<module>-r<n>

- **Plan:** qa/plans/YYYY-MM-DD-<module>-r<n>.md
- **Build:** OK / FAIL — evidence/build.log
- **Môi trường:** Windows … · auto-lock TẮT · account test …
- **Crash events (Event Log, 10'):** không / CÓ → evidence/crash-events.txt

## Kết quả theo case
| Case ID | Loại | Kết quả | Bằng chứng | Bug |
|---|---|---|---|---|
| VLT-FN-001 | FN | PASS | evidence/ctest.log | — |
| VLT-UI-004 | UI | FAIL | evidence/ui-004.png | VLT-BUG-#### |

## Tổng kết
- Tổng: N · PASS: … · FAIL: … · SKIP/BLOCKED: …
- Bug mới mở: VLT-BUG-#### …
- Ghi chú môi trường (lock/flaky/native-dialog…): …
