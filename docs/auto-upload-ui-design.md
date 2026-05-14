# Auto Upload — UI Design Specification

**Feature**: Tự động tải lên (Auto Upload / Folder Watch & Sync)
**Version**: 1.0
**Design System**: Fshare Clean Warm Light

---

## 1. Navigation Placement

### New Sidebar Item: "Đồng bộ" (Sync)

Position: **index 2** — ngay sau "Tải lên", trước "File"

```
┌──────────────────────┐
│  [F] Fshare Tool     │
│                      │
│  v  Tải xuống        │  index 0
│  ^  Tải lên          │  index 1
│  @  Đồng bộ       <──│  index 2  * NEW
│  [] File              │  index 3
│  () Tài khoản         │  index 4
│  {} Cài đặt           │  index 5
│                      │
│  ┌────────────────┐  │
│  │ [U] User Name  │  │
│  │ user@email.com │  │
│  └────────────────┘  │
└──────────────────────┘
```

- **Icon**: `sync` (new SVG — circular arrows, Phosphor Icons style)
- **Label**: `qsTr("Đồng bộ")`
- **Badge**: Pending file count (if > 0), small red pill on nav item

### Lý do chọn page riêng thay vì tab trong UploadPage:
- Feature phức tạp: quản lý folders + activity log + settings
- UploadPage đã có toolbar/list/history riêng cho manual upload
- Folder sync là mental model khác — "set and forget" vs "pick and upload"

---

## 2. Page Layout Overview — AutoUploadPage.qml

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│  Đồng bộ tự động                              [* Bật]              │
│  Tự động tải file từ thư mục lên Fshare                            │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │ THU MUC THEO DOI (2/5)                      [+ Thêm]       │    │
│  │─────────────────────────────────────────────────────────────│    │
│  │                                                             │    │
│  │  ┌─────────────────────────────────────────────────────┐   │    │
│  │  │ [] D:\Photos\2026                                   │   │    │
│  │  │    -> Fshare: /Photos/2026                          │   │    │
│  │  │    v 245 file đã đồng bộ | 12 đang chờ             │   │    │
│  │  │    Xóa local sau upload: Có                         │   │    │
│  │  │                           [Tạm dừng] [Cài đặt] [x] │   │    │
│  │  └─────────────────────────────────────────────────────┘   │    │
│  │                                                             │    │
│  │  ┌─────────────────────────────────────────────────────┐   │    │
│  │  │ [] D:\Work\Reports                                  │   │    │
│  │  │    -> Fshare: /Backup/Reports                       │   │    │
│  │  │    * Đang tải (3 file) | 1.2 GB còn lại            │   │    │
│  │  │    ==================-------  67%    2.4 MB/s       │   │    │
│  │  │    Xóa local sau upload: Không                      │   │    │
│  │  │                           [Tạm dừng] [Cài đặt] [x] │   │    │
│  │  └─────────────────────────────────────────────────────┘   │    │
│  │                                                             │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │ HOAT DONG GAN DAY                          [Xóa lịch sử]  │    │
│  │─────────────────────────────────────────────────────────────│    │
│  │                                                             │    │
│  │  v  report-q1.xlsx          -> /Backup/Reports   vừa xong  │    │
│  │  v  photo_001.jpg           -> /Photos/2026      2 phút    │    │
│  │  v  photo_002.jpg           -> /Photos/2026      2 phút    │    │
│  │  !  large_video.mp4         Lỗi: hết quota       5 phút    │    │
│  │  ^  presentation.pptx       Đang tải... 45%      ngay bây  │    │
│  │                                                             │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### QML Layout Structure
```
ScrollView
  ColumnLayout (spacing: sp16)
    RowLayout — Header (title + subtitle | master FsSwitch)
    SettingsCard — Folder Watch Section
      Section header ("THU MUC THEO DOI (n/5)" + "Thêm" FsButton)
      ColumnLayout of FsWatchCard items (max 5)
    SettingsCard — Activity Section
      Section header ("HOAT DONG GAN DAY" + "Xóa lịch sử" FsButton)
      ListView of FsActivityRow items
```

---

## 3. Component: FsWatchCard

### 3.1 State: Idle (đã đồng bộ xong)

```
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  []  D:\Photos\2026                                     * Hoạt  │
│      -> Fshare: /Photos/2026                             động   │
│                                                                  │
│  v 245 file đã đồng bộ | 3.2 GB           Theo dõi thư mục con │
│  Cập nhật lần cuối: 14:30 hôm nay         Xóa local: Không     │
│                                                                  │
│                                    [Quét lại]  [Cài đặt]  [x]  │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 State: Active (đang upload)

```
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  []  D:\Work\Reports                                    * Đang  │
│      -> Fshare: /Backup/Reports                          tải    │
│                                                                  │
│  ===================-----------  67%                             │
│  Đang tải: report-q1.xlsx (3/15 file)                           │
│  1.2 GB còn lại | 2.4 MB/s | ~8 phút                           │
│                                                                  │
│                                  [Tạm dừng]  [Cài đặt]  [x]   │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 3.3 State: Paused (tạm dừng)

```
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  []  D:\Photos\2026                                    * Tạm    │
│      -> Fshare: /Photos/2026                            dừng    │
│                                                                  │
│  ===============-----------------  45%  (tạm dừng)              │
│  12 file đang chờ | 800 MB còn lại                              │
│                                                                  │
│                                [Tiếp tục]  [Cài đặt]  [x]     │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 3.4 State: Error

```
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  []  D:\Videos\Raw                                     ! Lỗi   │
│      -> Fshare: /Videos                                         │
│                                                                  │
│  ! Hết dung lượng lưu trữ Fshare.                              │
│    Nâng cấp tài khoản hoặc xóa bớt file trên Fshare.           │
│                                                                  │
│  2 file lỗi | 245 file đã đồng bộ                              │
│                                                                  │
│                                  [Thử lại]  [Cài đặt]  [x]    │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 3.5 FsWatchCard — QML API

```qml
// FsWatchCard.qml
FsCard {
    // Data
    property string watchId: ""
    property string localPath: ""            // "D:\Photos\2026"
    property string remotePath: ""           // "/Photos/2026"
    property int status: 0                   // WatchStatus enum

    // Progress (status = Active)
    property real progress: 0.0              // 0.0-1.0
    property string currentFile: ""
    property int filesCompleted: 0
    property int filesTotal: 0
    property string remainingSize: ""
    property string speed: ""
    property string eta: ""

    // Stats (status = Idle)
    property int syncedCount: 0
    property string totalSize: ""
    property string lastSyncTime: ""
    property int pendingCount: 0

    // Config display
    property bool deleteAfterUpload: false
    property bool watchSubfolders: true

    // Error (status = Error)
    property string errorMessage: ""
    property int errorCount: 0

    // Signals
    signal pauseClicked()
    signal resumeClicked()
    signal rescanClicked()
    signal retryClicked()
    signal settingsClicked()
    signal removeClicked()
}
```

### 3.6 WatchStatus Enum

| Value | Name | Description |
|---|---|---|
| 0 | Idle | Da dong bo xong, dang theo doi |
| 1 | Scanning | Dang quet thu muc |
| 2 | Active | Dang upload file |
| 3 | Paused | Nguoi dung tam dung |
| 4 | Error | Co loi (quota, network, v.v.) |
| 5 | Disabled | Thu muc bi tat (folder khong ton tai) |

### 3.7 Visual Tokens

| Element | Token | Value |
|---|---|---|
| Card bg (default) | `FshareTheme.surface` | White / Dark surface |
| Card bg (error) | Subtle amber | `rgba(amber, 0.04)` |
| Card border (idle) | `FshareTheme.border` | Default |
| Card border (active) | `FshareTheme.redBorder` | Red accent |
| Card border (error) | `FshareTheme.amberBorder` | Amber accent |
| Card radius | `FshareTheme.r14` | 14px |
| Card padding | `FshareTheme.sp20` | 20px |
| Folder icon | `FsIcon "folder"` | 20px, `FshareTheme.red` |
| Local path | `body` weight `DemiBold` | `text1` |
| Remote path | `caption` | `text3`, prefix "-> Fshare:" |
| Stats text | `caption` | `text2` |
| Progress bar | `FsProgressBar` | 4px, status `"uploading"` |
| Current file | `caption` weight `DemiBold` | `text2` |
| Speed/ETA | `captionMono` | `text2`, monospace |
| Error message | `caption` | `FshareTheme.amberText` |
| Action buttons | `FsButton` size `"sm"` | variant `"ghost"` |
| Remove (x) | Icon button | `text3` -> `red` on hover |

### 3.8 Status Badge Mapping

| Status | FsBadge variant | Label | dot |
|---|---|---|---|
| Idle | `"green"` | `qsTr("Hoạt động")` | false |
| Scanning | `"blue"` | `qsTr("Đang quét")` | true |
| Active | `"red"` | `qsTr("Đang tải")` | true |
| Paused | `"amber"` | `qsTr("Tạm dừng")` | false |
| Error | `"amber"` | `qsTr("Lỗi")` | false |
| Disabled | `"neutral"` | `qsTr("Đã tắt")` | false |

### 3.9 Action Buttons by Status

| Status | Button 1 | Button 2 | Button 3 |
|---|---|---|---|
| Idle | Quét lại (ghost) | Cài đặt (ghost) | x Remove |
| Scanning | -- | Cài đặt (ghost) | x Remove |
| Active | Tạm dừng (ghost) | Cài đặt (ghost) | x Remove |
| Paused | Tiếp tục (secondary) | Cài đặt (ghost) | x Remove |
| Error | Thử lại (secondary) | Cài đặt (ghost) | x Remove |
| Disabled | Bật lại (secondary) | Cài đặt (ghost) | x Remove |

---

## 4. Page States

### State 1: Empty (chưa có folder nào)

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                  │
│  Đồng bộ tự động                                                │
│  Tự động tải file từ thư mục lên Fshare                        │
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                                                           │  │
│  │                        @                                  │  │
│  │              Chưa có thư mục nào                          │  │
│  │                                                           │  │
│  │     Thêm thư mục để tự động tải file lên Fshare.         │  │
│  │     Hỗ trợ tối đa 5 thư mục theo dõi.                    │  │
│  │                                                           │  │
│  │              [ + Thêm thư mục ]                           │  │
│  │                                                           │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

- Component: `FsEmptyState`
- `icon`: sync symbol
- `title`: `qsTr("Chưa có thư mục nào")`
- `description`: `qsTr("Thêm thư mục để tự động tải file lên Fshare. Hỗ trợ tối đa 5 thư mục theo dõi.")`
- `actionText`: `qsTr("Thêm thư mục")`
- Master toggle: **hidden** when empty

### State 2: Has Folders, All Idle

- Header shows master toggle `[* Bật]`
- Each folder shows Idle card
- Activity section shows recent sync history

### State 3: Has Folders, Syncing

- Active cards show progress bar + speed + ETA
- Sidebar nav badge shows pending file count
- Header subtitle: `qsTr("Đang đồng bộ %1 file...").arg(totalPending)`

### State 4: Has Folders, All Paused

- All cards show Paused state
- Master toggle still ON, subtitle: `qsTr("Đã tạm dừng")`

### State 5: Has Folders, Error

- Error card shows amber border + error message
- Toast notification: `FsToast` variant `"warning"`
- Other cards continue normally

### State 6: Master Toggle OFF

- All cards dim: `opacity: 0.5`
- No progress, no new activity
- Subtitle: `qsTr("Đã tắt đồng bộ tự động")`
- Each card badge: `"neutral"` "Đã tắt"

---

## 5. Dialog: Thêm thư mục — AddWatchFolderDialog

### Step 1: Chọn thư mục

```
┌──────────────────────────────────────────────────────────────┐
│  Thêm thư mục đồng bộ                                   x  │
│──────────────────────────────────────────────────────────────│
│                                                              │
│  Thư mục trên máy tính                                      │
│  ┌──────────────────────────────────────────────┐ [Chọn]    │
│  │ D:\Photos\2026                               │            │
│  └──────────────────────────────────────────────┘            │
│                                                              │
│  Thư mục đích trên Fshare                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  [] / (Gốc)                                          │   │
│  │  |-- [] Photos                                        │   │
│  │  |   |-- [] 2025                                      │   │
│  │  |   '-- [] 2026  <-- đã chọn                         │   │
│  │  |-- [] Backup                                        │   │
│  │  '-- [] Videos                                        │   │
│  │                                                       │   │
│  │  [+ Tạo thư mục mới]                                 │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│──────────────────────────────────────────────────────────────│
│                                       [Hủy]  [Tiếp tục ->] │
└──────────────────────────────────────────────────────────────┘
```

### Step 2: Tùy chọn

```
┌──────────────────────────────────────────────────────────────┐
│  Thêm thư mục đồng bộ                                   x  │
│──────────────────────────────────────────────────────────────│
│                                                              │
│  D:\Photos\2026  ->  Fshare: /Photos/2026                   │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Theo dõi thư mục con                          [*]   │   │
│  │ Bao gồm file trong các thư mục con                   │   │
│  │──────────────────────────────────────────────────────│   │
│  │ Xóa file local sau khi tải lên               [ ]    │   │
│  │ File sẽ bị xóa khỏi máy tính sau khi upload          │   │
│  │ thành công và được xác nhận trên Fshare               │   │
│  │──────────────────────────────────────────────────────│   │
│  │ Bỏ qua file theo mẫu                                 │   │
│  │ ┌──────────────────────────────────────────────┐     │   │
│  │ │ *.tmp, *.part, Thumbs.db, desktop.ini        │     │   │
│  │ └──────────────────────────────────────────────┘     │   │
│  │ Mỗi mẫu cách nhau bởi dấu phẩy                      │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  ! Sẽ quét 1,247 file (2.8 GB) trong thư mục này.   │   │
│  │    Quá trình tải lên sẽ bắt đầu sau khi thêm.       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│──────────────────────────────────────────────────────────────│
│                                  [<- Quay lại]  [Thêm]     │
└──────────────────────────────────────────────────────────────┘
```

### Dialog Specs

| Element | Value |
|---|---|
| Component | `FsDialog` |
| `dialogWidth` | 520px |
| Title | `qsTr("Thêm thư mục đồng bộ")` |
| Local folder | `FsFolderPicker` |
| Remote folder | `FsRemoteFolderTree` (new component) |
| Toggle rows | Reuse SettingsToggle pattern from SettingsPage |
| Ignore patterns | `FsTextField` with hint text |
| Preview box | Red tint hint box (same style as SettingsPage) |
| Footer | Ghost "Hủy"/"Quay lại" + Primary "Tiếp tục"/"Thêm" |

### "Delete after upload" Warning

When user enables "Xóa file local sau khi tải lên", show inline:

```
┌──────────────────────────────────────────────────────────┐
│ ! File sẽ bị xóa vĩnh viễn khỏi máy tính sau khi       │
│   upload thành công. Hành động này không thể hoàn tác.   │
│   Đảm bảo bạn có bản sao lưu nếu cần.                  │
└──────────────────────────────────────────────────────────┘
```

- Background: `FshareTheme.amberTint`
- Border: `FshareTheme.amberBorder`
- Text color: `FshareTheme.amberText`
- Font: `caption`

### FsRemoteFolderTree (new component)

```qml
Rectangle {
    property string selectedFolderId: "0"    // "0" = root
    property string selectedFolderPath: "/"

    signal folderSelected(string folderId, string folderPath)
    signal createFolderRequested(string parentId)
}
```

- Surface bg, border, `r10`, max-height 240px with ScrollBar
- Each row: 32px height, folder icon + name, indent 20px per level
- Selected row: `redTint10` bg + `red` text
- Hover: `bg2`
- Expand/collapse: arrow icon, `durFast` rotation animation
- "Tạo thư mục mới" button: ghost, bottom of tree

---

## 6. Dialog: Cài đặt thư mục — WatchFolderSettingsDialog

```
┌──────────────────────────────────────────────────────────────┐
│  Cài đặt thư mục đồng bộ                                x  │
│──────────────────────────────────────────────────────────────│
│                                                              │
│  [] D:\Photos\2026  ->  /Photos/2026                        │
│                                                              │
│  Theo dõi thư mục con                               [*]    │
│  Bao gồm file trong các thư mục con                         │
│  ────────────────────────────────────────────────────────    │
│  Xóa file local sau khi tải lên                     [ ]    │
│  File sẽ bị xóa khỏi máy tính sau upload thành công         │
│  ────────────────────────────────────────────────────────    │
│  Bỏ qua file theo mẫu                                       │
│  ┌──────────────────────────────────────────────────┐       │
│  │ *.tmp, *.part, Thumbs.db, desktop.ini            │       │
│  └──────────────────────────────────────────────────┘       │
│  ────────────────────────────────────────────────────────    │
│  Giới hạn tốc độ tải lên                                    │
│  o Không giới hạn                                            │
│  o Tùy chỉnh: [____5___] MB/s                               │
│                                                              │
│──────────────────────────────────────────────────────────────│
│                                          [Hủy]  [Lưu]      │
└──────────────────────────────────────────────────────────────┘
```

| Element | Component |
|---|---|
| Dialog | `FsDialog`, `dialogWidth: 480` |
| Toggle rows | SettingsToggle pattern |
| Ignore input | `FsTextField` |
| Speed limit | `FsRadio` group + `NumberSpinner` |
| Footer | Ghost "Hủy" + Primary "Lưu" |

---

## 7. Dialog: Xác nhận xóa — RemoveWatchDialog

```
┌──────────────────────────────────────────────────────────────┐
│  Xóa thư mục đồng bộ                                    x  │
│──────────────────────────────────────────────────────────────│
│                                                              │
│  Bạn có chắc muốn ngừng theo dõi thư mục này?              │
│                                                              │
│  [] D:\Photos\2026  ->  /Photos/2026                        │
│                                                              │
│  245 file đã đồng bộ sẽ không bị ảnh hưởng.                │
│  File trên Fshare và trên máy tính đều được giữ nguyên.    │
│                                                              │
│──────────────────────────────────────────────────────────────│
│                                          [Hủy]  [Xóa]      │
└──────────────────────────────────────────────────────────────┘
```

- "Xóa" button: `FsButton` variant `"danger"`
- `dialogWidth`: 440px

---

## 8. Activity Feed

### Row Layout (36px height)

```
[StatusIcon 16px] [FileName fill] [Destination/Status 180px] [Size 60px] [Time 70px]
```

| Column | Font | Color |
|---|---|---|
| Status icon | -- | Semantic color |
| File name | `body` | `text1` |
| Destination | `caption` | `text2` |
| File size | `captionMono` | `text3` |
| Time ago | `caption` | `text3` |

### Status Icons

| Status | Symbol | Color |
|---|---|---|
| Completed | v | `FshareTheme.green` |
| Uploading | ^ | `FshareTheme.red` |
| Error | ! | `FshareTheme.amber` |
| Deleted local | v+ | `FshareTheme.green` |
| Skipped | -> | `FshareTheme.text3` |

### Activity Row Hover
- Background: `bg1`, transition `durFast`

### Empty Activity
- Inline text: `qsTr("Chưa có hoạt động nào.")`
- `body`, `text3`, centered

### "Xem thêm" (Load More)
- Visible when > 20 items
- `FsButton` variant `"ghost"` size `"sm"`, centered
- Loads 20 more items per click

---

## 9. Sidebar Badge

When pending files > 0, show count badge on "Đồng bộ" nav item:

```
│  @  Đồng bộ  [12]  │
```

| Element | Value |
|---|---|
| Position | Right-aligned, verticalCenter in nav row |
| Size | auto width (min 18px) x 16px |
| Radius | 8px (pill) |
| Background | `FshareTheme.red` |
| Text | White, 10px, `Font.Bold` |
| Content | Count ("99+" if > 99) |
| Visibility | Only when `pendingCount > 0` |
| Animation | Scale 0->1, `durFast`, `Easing.OutBack` |

---

## 10. Toast Notifications

### Batch Complete

```
┌───────────────────────────────────────────────┐
│  v  Đã đồng bộ 15 file                       │
│     từ thư mục Photos/2026 | 120 MB          │
└───────────────────────────────────────────────┘
```
- `FsToast` variant `"success"`, `autoCloseMs: 5000`
- Grouped: 1 toast per 5-10 minutes, not per file

### Error

```
┌───────────────────────────────────────────────┐
│  !  Đồng bộ tạm dừng                         │
│     Hết dung lượng Fshare                     │
└───────────────────────────────────────────────┘
```
- `FsToast` variant `"warning"`, `autoCloseMs: 8000`
- Show once per error type (no spam)

### Folder Missing

```
┌───────────────────────────────────────────────┐
│  x  Thư mục không tìm thấy                   │
│     D:\Photos\2026 đã bị xóa hoặc di chuyển  │
└───────────────────────────────────────────────┘
```
- `FsToast` variant `"error"`, `autoCloseMs: 10000`

---

## 11. Dry Run Preview

In Step 2 of AddWatchFolderDialog, show scan preview before committing:

```
┌──────────────────────────────────────────────────────────┐
│  ! Sẽ quét 1,247 file (2.8 GB) trong thư mục này.      │
│    Quá trình tải lên sẽ bắt đầu sau khi thêm.          │
│                                                          │
│    Ước tính thời gian: ~45 phút (tùy tốc độ mạng)      │
│    Dung lượng Fshare còn trống: 48.2 GB                 │
└──────────────────────────────────────────────────────────┘
```

- Style: redTint6 bg, redBorder 18% (same as SettingsPage hint)
- If files > quota: switch to amber warning style
- Shows during preview loading: `FsLoadingState` with "Đang quét thư mục..."

---

## 12. Responsive Behavior

| Window Width | Adaptation |
|---|---|
| >= 1100px | Full layout, 2-column stats in FsWatchCard |
| 800-1100px | Stats stack vertically, activity hides size column |
| < 800px (min) | Cards full-width, activity shows filename + status only |

### FsWatchCard Responsive

**Wide** (card width >= 500px): Side by side
```
v 245 file da dong bo | 3.2 GB      Theo doi thu muc con
Cap nhat: 14:30 hom nay             Xoa local: Khong
```

**Narrow** (card width < 500px): Stacked
```
v 245 file da dong bo | 3.2 GB
Cap nhat: 14:30 hom nay
Theo doi thu muc con | Xoa local: Khong
```

---

## 13. Animations

| Event | Animation | Duration |
|---|---|---|
| Card added | Fade in + slide down | `durNormal` `OutCubic` |
| Card removed | Fade out + slide up | `durFast` `OutCubic` |
| Progress bar | Width transition | 300ms `OutCubic` |
| Badge count | Scale bounce | `durFast` `OutBack` |
| Status badge | Color cross-fade | `durFast` |
| Master OFF | All cards opacity 1->0.5 | `durNormal` |
| Dialog open | Scale + opacity | `durDialog` `OutBack` |
| Activity row | Fade in from top | `durFast` |

All gated on `!FshareTheme.reduceMotion`.

---

## 14. Keyboard Accessibility

| Action | Shortcut |
|---|---|
| Add folder | `Ctrl+Shift+A` (on Sync page) |
| Toggle master | `Ctrl+Shift+S` |
| Navigate cards | `Tab` / `Shift+Tab` |
| Card settings | `Enter` when focused |
| Remove card | `Delete` (with confirm dialog) |
| Pause/Resume | `Space` when card focused |

Tab order: Master toggle -> Add button -> Card 1 -> Card 2 -> ... -> Activity

---

## 15. ViewModel Interface

```cpp
class AutoUploadViewModel : public QObject {
    Q_OBJECT

    // Master control
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int totalPending READ totalPending NOTIFY totalPendingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

    // Models
    Q_PROPERTY(QAbstractListModel* folders READ folders CONSTANT)
    Q_PROPERTY(QAbstractListModel* activity READ activity CONSTANT)

public:
    // Folder management
    Q_INVOKABLE void addFolder(const QString &localPath,
                                const QString &remoteFolderId,
                                bool watchSubfolders,
                                bool deleteAfterUpload,
                                const QString &ignorePatterns);
    Q_INVOKABLE void removeFolder(const QString &watchId);
    Q_INVOKABLE void updateFolderSettings(const QString &watchId,
                                           bool watchSubfolders,
                                           bool deleteAfterUpload,
                                           const QString &ignorePatterns,
                                           int speedLimitKBps);

    // Control
    Q_INVOKABLE void pauseFolder(const QString &watchId);
    Q_INVOKABLE void resumeFolder(const QString &watchId);
    Q_INVOKABLE void rescanFolder(const QString &watchId);
    Q_INVOKABLE void retryFolder(const QString &watchId);
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();

    // Activity
    Q_INVOKABLE void clearActivity();

    // Dry-run preview
    Q_INVOKABLE void previewFolder(const QString &localPath,
                                    bool watchSubfolders,
                                    const QString &ignorePatterns);

signals:
    void enabledChanged();
    void totalPendingChanged();
    void statusTextChanged();
    void previewReady(int fileCount, const QString &totalSize,
                      const QString &estimatedTime, bool quotaOk);
    void batchCompleted(const QString &folderName, int fileCount,
                        const QString &totalSize);
    void syncError(const QString &folderName, const QString &message);
    void folderMissing(const QString &localPath);
};
```

### WatchFolderListModel Roles

| Role | Type | Description |
|---|---|---|
| watchId | string | UUID |
| localPath | string | Local folder path |
| remotePath | string | Fshare folder path |
| status | int | WatchStatus enum |
| progress | real | 0.0-1.0 |
| currentFile | string | Current uploading file |
| filesCompleted | int | Done count |
| filesTotal | int | Total count |
| remainingSize | string | "1.2 GB" |
| speed | string | "2.4 MB/s" |
| eta | string | "~8 phut" |
| syncedCount | int | Total synced files |
| totalSize | string | "3.2 GB" |
| lastSyncTime | string | "14:30 hom nay" |
| pendingCount | int | Files waiting |
| deleteAfterUpload | bool | Config flag |
| watchSubfolders | bool | Config flag |
| errorMessage | string | Error text |
| errorCount | int | Failed files |

### ActivityListModel Roles

| Role | Type | Description |
|---|---|---|
| fileName | string | File name |
| remotePath | string | Fshare destination |
| fileSize | string | "2.4 MB" |
| status | string | completed/uploading/error/skipped |
| timeAgo | string | "vua xong", "2 phut" |
| errorMessage | string | Error detail |
| progress | real | Only for uploading status |
| watchFolderId | string | Parent watch folder |

---

## 16. QML File Structure

### New Files

```
FsNext/qml/Fshare/Pages/AutoUploadPage.qml
FsNext/qml/Fshare/Components/FsWatchCard.qml
FsNext/qml/Fshare/Components/FsRemoteFolderTree.qml
FsNext/qml/Fshare/Components/FsActivityRow.qml
FsNext/qml/Fshare/Dialogs/AddWatchFolderDialog.qml
FsNext/qml/Fshare/Dialogs/WatchFolderSettingsDialog.qml
FsNext/qml/Fshare/Dialogs/RemoveWatchDialog.qml
FsNext/qml/Fshare/Icons/sync.svg
```

### Modified Files

```
FsNext/qml/Main.qml                      — Add nav item (index 2) + Loader
FsNext/qml/Fshare/Components/qmldir      — Register new components
FsNext/qml/Fshare/Pages/qmldir           — Register AutoUploadPage
FsNext/qml/Fshare/Dialogs/qmldir         — Register new dialogs
```

---

## 17. i18n Strings

All use `qsTr()` with Vietnamese source, context `"AutoUploadPage"`.

### Page
```
qsTr("Đồng bộ tự động")
qsTr("Tự động tải file từ thư mục lên Fshare")
qsTr("Đang đồng bộ %1 file...").arg(n)
qsTr("Đã tắt đồng bộ tự động")
qsTr("Đã tạm dừng")
```

### Sections
```
qsTr("Thư mục theo dõi (%1/%2)").arg(count).arg(max)
qsTr("Hoạt động gần đây")
```

### Empty State
```
qsTr("Chưa có thư mục nào")
qsTr("Thêm thư mục để tự động tải file lên Fshare. Hỗ trợ tối đa 5 thư mục theo dõi.")
qsTr("Thêm thư mục")
```

### FsWatchCard
```
qsTr("Hoạt động")
qsTr("Đang quét")
qsTr("Đang tải")
qsTr("Tạm dừng")
qsTr("Lỗi")
qsTr("Đã tắt")
qsTr("%1 file đã đồng bộ").arg(n)
qsTr("%1 đang chờ").arg(n)
qsTr("Cập nhật lần cuối: %1").arg(time)
qsTr("Đang tải: %1 (%2/%3 file)").arg(name).arg(done).arg(total)
qsTr("%1 còn lại").arg(size)
qsTr("Theo dõi thư mục con")
qsTr("Xóa local: Có")
qsTr("Xóa local: Không")
qsTr("Quét lại")
qsTr("Cài đặt")
qsTr("Tạm dừng")
qsTr("Tiếp tục")
qsTr("Thử lại")
qsTr("Bật lại")
```

### Dialogs
```
qsTr("Thêm thư mục đồng bộ")
qsTr("Thư mục trên máy tính")
qsTr("Thư mục đích trên Fshare")
qsTr("Tạo thư mục mới")
qsTr("Theo dõi thư mục con")
qsTr("Bao gồm file trong các thư mục con")
qsTr("Xóa file local sau khi tải lên")
qsTr("File sẽ bị xóa khỏi máy tính sau khi upload thành công và được xác nhận trên Fshare")
qsTr("Bỏ qua file theo mẫu")
qsTr("Mỗi mẫu cách nhau bởi dấu phẩy")
qsTr("Tiếp tục")
qsTr("Quay lại")
qsTr("Thêm")
qsTr("Hủy")
qsTr("Lưu")
qsTr("Xóa thư mục đồng bộ")
qsTr("Bạn có chắc muốn ngừng theo dõi thư mục này?")
qsTr("%1 file đã đồng bộ sẽ không bị ảnh hưởng.").arg(n)
qsTr("File trên Fshare và trên máy tính đều được giữ nguyên.")
qsTr("Xóa")
qsTr("Cài đặt thư mục đồng bộ")
qsTr("Giới hạn tốc độ tải lên")
qsTr("Không giới hạn")
qsTr("Tùy chỉnh")
```

### Warnings
```
qsTr("File sẽ bị xóa vĩnh viễn khỏi máy tính sau khi upload thành công. Hành động này không thể hoàn tác. Đảm bảo bạn có bản sao lưu nếu cần.")
```

### Toasts
```
qsTr("Đã đồng bộ %1 file").arg(n)
qsTr("từ thư mục %1 | %2").arg(folder).arg(size)
qsTr("Đồng bộ tạm dừng")
qsTr("Hết dung lượng Fshare. Nâng cấp tài khoản hoặc xóa bớt file.")
qsTr("Thư mục không tìm thấy")
qsTr("%1 đã bị xóa hoặc di chuyển").arg(path)
```

### Preview
```
qsTr("Sẽ quét %1 file (%2) trong thư mục này.").arg(n).arg(size)
qsTr("Quá trình tải lên sẽ bắt đầu sau khi thêm.")
qsTr("Ước tính thời gian: %1 (tùy tốc độ mạng)").arg(time)
qsTr("Dung lượng Fshare còn trống: %1").arg(space)
```

### Activity
```
qsTr("Xóa lịch sử")
qsTr("Chưa có hoạt động nào.")
qsTr("Các file được đồng bộ sẽ hiển thị ở đây.")
qsTr("Xem thêm")
qsTr("vừa xong")
```

---

## 18. Implementation Priority

| # | Component | Complexity | Depends on |
|---|---|---|---|
| 1 | AutoUploadPage.qml (layout + empty state) | Low | -- |
| 2 | FsWatchCard.qml (all 6 states) | Medium | -- |
| 3 | AddWatchFolderDialog.qml (2-step) | Medium | FsRemoteFolderTree |
| 4 | Main.qml nav update | Low | AutoUploadPage |
| 5 | FsActivityRow + activity ListView | Low | -- |
| 6 | WatchFolderSettingsDialog.qml | Low | -- |
| 7 | RemoveWatchDialog.qml | Low | -- |
| 8 | FsRemoteFolderTree.qml | High | API integration |
| 9 | Toast notification wiring | Low | -- |
| 10 | Sidebar badge | Low | -- |
| 11 | sync.svg icon | Low | -- |
