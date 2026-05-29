// SPDX-License-Identifier: Proprietary
// DownloadPage (Aurora) — editorial header + transfers list + history.
//
// Matches handoff `variant-aurora.jsx → AuroraDownloads`:
//   • Top panel: serif mega-number for aggregate speed, mono kicker, mini
//     stats (done/remaining/ETA/threads), pause-all + "Thêm URL" CTA.
//   • Body: segmented Active/History tabs, folder-scan banner, transfer
//     rows (FsTransferItem — renders in the rebranded palette).
//
// All VM signal/method wiring from the legacy page is preserved: addDownload,
// pauseAll/resumeAll, pauseTask/resumeTask/cancelTask/dismissCompleted,
// infinite-scroll history (loadMoreHistory/hasMoreHistory), scan banner with
// cancelFolderScan, system-folder-block toast.

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import Fshare.Components 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: page

    // Listen for the root window's "open add-download dialog" signals.
    // Drag-drop / paste / Chrome native-host route URLs via the
    // `pendingDownloadLinks` property on root (set BEFORE the page is
    // navigated to, so we drain it both on first Component.onCompleted
    // and on subsequent change events — mirrors the upload-page pattern,
    // dodges the same Loader-instantiation race).
    function _consumePendingDownloads() {
        const w = Window.window;
        if (!w) return;
        const links = w.pendingDownloadLinks;
        if (!links || links.length === 0) return;
        addDialog.linksText = links;
        addDialog.open();
        w.pendingDownloadLinks = "";   // consumed
    }
    Component.onCompleted: _consumePendingDownloads()
    Connections {
        target: Window.window
        ignoreUnknownSignals: true
        function onPendingDownloadLinksChanged() { _consumePendingDownloads(); }
        function onOpenDownloadDialog() {
            addDialog.linksText = "";
            addDialog.open();
        }
    }

    property bool showHistory: false
    readonly property int activeCount:  downloadViewModel ? downloadViewModel.model.count : 0
    readonly property int historyCount: downloadViewModel ? downloadViewModel.historyModel.count : 0

    // Highlighted task id — set by focusTask() and auto-cleared 1500ms later.
    // FsTransferItem doesn't currently watch this, so visual feedback is
    // limited to the ListView scrolling the row into view.  P3 polish can
    // add a border pulse by wiring this through the delegate.
    property string focusedTaskId: ""
    Timer {
        id: _focusClearTimer
        interval: 1500
        onTriggered: page.focusedTaskId = ""
    }

    // Called by Main.qml's HUD VM signal router when the user clicks a
    // row in the tray popup / mini window.  Scrolls the active list to
    // bring `taskId` into view; if the task isn't active (history-only)
    // we silently no-op since scrolling a history list to a random row
    // is rarely what the user wanted from a tray-popup click.
    function focusTask(taskId) {
        if (!downloadViewModel || !downloadViewModel.model) return;
        const idx = downloadViewModel.model.rowOfTask(taskId);
        if (idx < 0) return;          // not in active list — skip
        page.showHistory = false;
        downloadActiveList.positionViewAtIndex(idx, ListView.Center);
        downloadActiveList.currentIndex = idx;
        page.focusedTaskId = taskId;
        _focusClearTimer.restart();
    }

    // Page-level clock driving relative-time labels on just-completed rows.
    property real nowMs: Date.now()
    Timer {
        interval: 30 * 1000
        running: true
        repeat: true
        onTriggered: page.nowMs = Date.now()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: AuroraTheme.sp4

        // ═════════════════════════════════════════════════
        //  EDITORIAL HEADER PANEL
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: headerCol.implicitHeight + AuroraTheme.sp6 * 2
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            ColumnLayout {
                id: headerCol
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp6
                spacing: AuroraTheme.sp4

                // ── Top row: kicker+serif number · stats · actions ──
                // Spacing trimmed sp6 (24) → sp4 (16) — earlier value sat right
                // at the edge of available width on a 1100-window with the
                // 240-px sidebar, so the trailing "Thêm URL" button could
                // overflow past the panel border by a couple of pixels.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp4

                    // Left: kicker + big speed
                    ColumnLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            text: page.showHistory
                                ? "━━ Lịch sử · " + page.historyCount + " file"
                                : "━━ Đang tải · " + page.activeCount + " file"
                            color: AuroraTheme.ink4
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            font.letterSpacing: 2.0
                            font.capitalization: Font.AllUppercase
                        }

                        RowLayout {
                            spacing: 8
                            Layout.bottomMargin: 2

                            Text {
                                id: bigNumber
                                property string _speed: downloadViewModel ? downloadViewModel.totalSpeed : ""
                                // Parse "47.3 MB/s" → value "47.3", unit "MB/s"
                                property string _value: {
                                    if (_speed.length === 0) return page.showHistory ? String(page.historyCount) : "—";
                                    const m = _speed.match(/^\s*([\d.,]+)\s*(.*)$/);
                                    return m ? m[1] : _speed;
                                }
                                text: _value
                                color: AuroraTheme.ink1
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 56
                                font.letterSpacing: -1.8
                                lineHeight: 1.0
                            }

                            Text {
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 10
                                property string _unit: {
                                    if (page.showHistory) return "file";
                                    const s = bigNumber._speed;
                                    if (s.length === 0) return "";
                                    const m = s.match(/^\s*[\d.,]+\s*(.*)$/);
                                    return m ? m[1] : "";
                                }
                                text: _unit
                                visible: text.length > 0
                                color: AuroraTheme.ink3
                                font.family: AuroraTheme.fontSerif
                                font.italic: true
                                font.pixelSize: 22
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Right: mini stats (only on Active tab). Internal
                    // spacing trimmed sp6 → sp4 for the same overflow reason
                    // as the outer row above.
                    RowLayout {
                        visible: !page.showHistory
                        spacing: AuroraTheme.sp4
                        Layout.alignment: Qt.AlignVCenter

                        Repeater {
                            model: [
                                {
                                    label: "Đang chạy",
                                    value: (transferBudgetViewModel
                                        ? transferBudgetViewModel.activeDownloads + "/" + transferBudgetViewModel.maxDownloads
                                        : "—")
                                },
                                {
                                    label: "Hàng đợi",
                                    value: (transferBudgetViewModel
                                        ? String(transferBudgetViewModel.pendingDownloads)
                                        : "—")
                                },
                                {
                                    // "Tổng tác vụ" was correct domain-wise
                                    // (a transfer task), but users read it
                                    // as ambiguous CS jargon. "Tổng file"
                                    // matches the kicker line above and is
                                    // also visually narrower → buys a few
                                    // more px of room for the action buttons.
                                    label: "Tổng file",
                                    value: String(page.activeCount)
                                }
                            ]
                            delegate: ColumnLayout {
                                spacing: 2
                                Text {
                                    text: modelData.label
                                    color: AuroraTheme.ink4
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 10
                                    font.letterSpacing: 1.4
                                    font.capitalization: Font.AllUppercase
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: modelData.value
                                    color: AuroraTheme.ink1
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 16
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }

                    // Actions
                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: AuroraTheme.sp2

                        // Pause/Resume all — only when there's active work
                        Aurora.FsButton {
                            readonly property string _run: downloadViewModel ? downloadViewModel.runState : "idle"
                            visible: !page.showHistory && _run !== "idle"
                            text: _run === "running" ? qsTr("Tạm dừng tất cả") : qsTr("Tiếp tục tất cả")
                            icon: _run === "running" ? "pause" : "play"
                            variant: "secondary"
                            size: "md"
                            onClicked: {
                                if (!downloadViewModel) return;
                                if (_run === "running") downloadViewModel.pauseAll();
                                else                    downloadViewModel.resumeAll();
                            }
                        }

                        // Clear history — only on History tab with rows
                        Aurora.FsButton {
                            visible: page.showHistory && page.historyCount > 0
                            text: qsTr("Xoá lịch sử")
                            variant: "ghost"
                            size: "md"
                            onClicked: if (downloadViewModel) downloadViewModel.clearHistory()
                        }

                        Aurora.FsButton {
                            visible: !page.showHistory
                            text: qsTr("Thêm URL")
                            icon: "plus"
                            variant: "primary"
                            size: "md"
                            onClicked: addDialog.open()
                        }
                    }
                }

                // ── Segmented Active/History tabs ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: AuroraTheme.sp2
                    spacing: AuroraTheme.sp2

                    FsSegmentedControl {
                        options: [
                            { value: false, label: qsTr("Đang tải") },
                            { value: true,  label: qsTr("Lịch sử") }
                        ]
                        selectedValue: page.showHistory
                        onSelectionChanged: page.showHistory = selectedValue
                    }

                    Item { Layout.fillWidth: true }

                    // Hint about drag/paste
                    Text {
                        visible: !page.showHistory && page.activeCount === 0
                        text: qsTr("Mẹo: kéo thả link Fshare vào cửa sổ hoặc Ctrl+V")
                        color: AuroraTheme.ink4
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  FOLDER SCAN BANNER
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: scanRow.implicitHeight + AuroraTheme.sp4 * 2
            radius: AuroraTheme.radiusMd
            color: AuroraTheme.accentSoft
            border.width: 1
            border.color: AuroraTheme.accentTint15
            visible: downloadViewModel ? downloadViewModel.isScanning : false

            RowLayout {
                id: scanRow
                anchors {
                    left: parent.left; right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: AuroraTheme.sp4; rightMargin: AuroraTheme.sp4
                }
                spacing: AuroraTheme.sp3

                FsIcon {
                    name: "search"
                    sizePx: 18
                    color: AuroraTheme.accent
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: downloadViewModel
                            ? qsTr("Đang quét thư mục \"%1\"…").arg(downloadViewModel.scanFolderName)
                            : ""
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink1
                    }
                    Text {
                        text: downloadViewModel
                            ? qsTr("Đã tìm thấy %1 file").arg(downloadViewModel.scanFoundFiles)
                            : ""
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        color: AuroraTheme.ink3
                    }
                }

                Aurora.FsButton {
                    text: qsTr("Huỷ")
                    variant: "ghost"
                    size: "sm"
                    onClicked: {
                        if (downloadViewModel && downloadViewModel.scanGroupId.length > 0)
                            downloadViewModel.cancelFolderScan(downloadViewModel.scanGroupId)
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  CONTENT AREA
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            // Empty — active
            FsEmptyState {
                anchors.fill: parent
                visible: !page.showHistory && page.activeCount === 0
                icon: "↓"
                title: qsTr("Chưa có tác vụ tải xuống")
                description: qsTr("Dán link Fshare để bắt đầu tải.")
                actionText: qsTr("Thêm URL")
                onActionClicked: addDialog.open()
            }

            // Empty — history
            FsEmptyState {
                anchors.fill: parent
                visible: page.showHistory && page.historyCount === 0
                icon: "✓"
                title: qsTr("Chưa có lịch sử tải xuống")
                description: qsTr("Các file đã tải xong sẽ hiển thị ở đây.")
            }

            // Active list
            ListView {
                id: downloadActiveList
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp2
                clip: true
                visible: !page.showHistory && page.activeCount > 0
                model: downloadViewModel ? downloadViewModel.model : null
                spacing: 0

                // P3 polish — focused row highlight.  ListView paints this
                // around currentIndex; opacity follows page.focusedTaskId
                // so the pulse only shows briefly after a focusTask() call
                // (1500 ms via _focusClearTimer), not on every keyboard nav.
                highlight: Rectangle {
                    color: "transparent"
                    border.color: AuroraTheme.accent
                    border.width: 2
                    radius: AuroraTheme.radiusSm
                    opacity: page.focusedTaskId.length > 0 ? 1.0 : 0.0
                    Behavior on opacity {
                        enabled: !AuroraTheme.reduceMotion
                        NumberAnimation { duration: AuroraTheme.durBase }
                    }
                }
                highlightFollowsCurrentItem: true
                highlightMoveDuration: 200

                delegate: FsTransferItem {
                    width: ListView.view ? ListView.view.width : 0
                    transferId:       model.taskId           || ""
                    fileName:         model.fileName         || ""
                    folderPath:       model.folderPath       || ""
                    fileSize:         model.fileSize         || 0
                    bytesTransferred: model.bytesTransferred || 0
                    progress:         model.progress         || 0
                    speed:            model.speed            || 0
                    eta:              model.eta              || ""
                    status:           model.status           || 0
                    errorMessage:     model.errorMessage     || ""
                    completedAt:      model.completedAt      || 0
                    nowMs:            page.nowMs
                    // The download task carries the original share URL the user
                    // pasted (file or folder) — pass it through so the copy
                    // button is visible and copies the right thing.
                    linkcode:         model.linkCode || ""

                    onPauseClicked:    if (downloadViewModel) downloadViewModel.pauseTask(transferId)
                    onResumeClicked:   if (downloadViewModel) downloadViewModel.resumeTask(transferId)
                    onCancelClicked:   if (downloadViewModel) downloadViewModel.cancelTask(transferId)
                    onDismissClicked:  if (downloadViewModel) downloadViewModel.dismissCompleted(transferId)
                    onCopyLinkClicked: if (downloadViewModel) downloadViewModel.copyShareLink(linkcode)
                    onOpenFolderClicked: {
                        const p = (model.localPath || "").replace(/\\/g, "/")
                        const dir = p.lastIndexOf("/") > 0 ? p.substring(0, p.lastIndexOf("/")) : p
                        Qt.openUrlExternally("file:///" + dir)
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }

            // History list
            ListView {
                id: downloadHistoryList
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp2
                clip: true
                visible: page.showHistory && page.historyCount > 0
                model: downloadViewModel ? downloadViewModel.historyModel : null
                spacing: 0

                delegate: FsTransferItem {
                    width: ListView.view ? ListView.view.width : 0
                    transferId:       model.taskId    || ""
                    fileName:         model.fileName  || ""
                    folderPath:       model.folderPath || ""
                    fileSize:         model.fileSize  || 0
                    bytesTransferred: model.fileSize  || 0
                    progress:         100
                    status:           3   // Complete
                    showActions:      true
                    completedAt:      model.completedAt || 0
                    nowMs:            page.nowMs
                    linkcode:         model.linkCode || ""

                    onCopyLinkClicked: if (downloadViewModel) downloadViewModel.copyShareLink(linkcode)
                    onOpenFolderClicked: {
                        const p = (model.localPath || "").replace(/\\/g, "/")
                        const dir = p.lastIndexOf("/") > 0 ? p.substring(0, p.lastIndexOf("/")) : p
                        Qt.openUrlExternally("file:///" + dir)
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // Infinite-scroll: trigger the VM's pager on-bottom. The VM
                // calls TransferService → HistoryRepository → SQLite LIMIT/OFFSET
                // query; each page is bounded (kHistoryPageSize) so this stays
                // well under a frame even cold.
                footer: Item {
                    width: downloadHistoryList.width
                    height: (downloadViewModel && downloadViewModel.hasMoreHistory)
                              ? AuroraTheme.sp10 : 0
                    visible: downloadViewModel && downloadViewModel.hasMoreHistory
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Đang tải thêm…")
                        color: AuroraTheme.ink4
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                    }
                }
                onAtYEndChanged: {
                    if (atYEnd && downloadViewModel && downloadViewModel.hasMoreHistory)
                        downloadViewModel.loadMoreHistory()
                }
            }
        }
    }

    // ═════════════════════════════════════════════════════
    //  System-folder block toast
    // ═════════════════════════════════════════════════════
    FsToast {
        id: blockToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: AuroraTheme.sp4
        z: 1001
        variant: "error"
        autoCloseMs: 5000
        visible: false
        onClosed: visible = false
    }

    Connections {
        target: downloadViewModel
        function onDownloadBlocked(reason) {
            blockToast.title = qsTr("Không thể tải vào thư mục hệ thống")
            blockToast.desc  = reason
            blockToast.visible = true
            blockToast.show()
        }
    }

    // ═════════════════════════════════════════════════════
    //  Add Download Dialog
    // ═════════════════════════════════════════════════════
    FsDialog {
        id: addDialog
        title: qsTr("Thêm tải xuống")
        dialogWidth: 520

        property string linksText: ""
        property string folderText: ""
        property string passwordText: ""

        onOpened: {
            if (folderText.length === 0 && downloadViewModel)
                folderText = downloadViewModel.defaultSaveFolder;
            // Park focus on the link area so a quick Ctrl+V (or a paste
            // landing from the global Ctrl+V hook in Main.qml) goes
            // straight in.  Qt.callLater defers until after FsDialog's own
            // open() focus grab so we don't fight the trap initialisation.
            Qt.callLater(() => linkArea.forceActiveFocus());
        }

        content: Item {
            width: 520
            height: addCol.implicitHeight + AuroraTheme.sp6 * 2

            ColumnLayout {
                id: addCol
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp6
                spacing: AuroraTheme.sp4

                // ── Multi-URL input ──────────────────────
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2

                    Text {
                        text: qsTr("LINK FSHARE")
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        font.letterSpacing: 1.4
                        font.capitalization: Font.AllUppercase
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink4
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 96
                        radius: AuroraTheme.radiusMd
                        color: AuroraTheme.panel
                        border.width: 1
                        border.color: linkArea.activeFocus ? AuroraTheme.accent : AuroraTheme.border
                        Behavior on border.color {
                            enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast }
                        }

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: AuroraTheme.sp3
                            clip: true

                            TextArea {
                                id: linkArea
                                wrapMode: TextEdit.Wrap
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 12
                                color: AuroraTheme.ink1
                                background: Item {}
                                placeholderText: qsTr("Dán link vào đây — mỗi link một dòng\nhttps://www.fshare.vn/file/…")
                                placeholderTextColor: AuroraTheme.ink4
                                text: addDialog.linksText
                                onTextChanged: addDialog.linksText = text
                            }
                        }
                    }

                    // URL analysis: show file/folder counts inline
                    Row {
                        spacing: AuroraTheme.sp2
                        visible: addDialog.linksText.trim().length > 0

                        property var _analysis: {
                            // Case-insensitive detection so FILE/ or Folder/ pastes are classified correctly.
                            const lines = addDialog.linksText.split('\n').filter(function(l) { return l.trim().length > 0; });
                            let files = 0, folders = 0, invalid = 0;
                            for (let i = 0; i < lines.length; i++) {
                                const l = lines[i].trim().toLowerCase();
                                if (l.indexOf("fshare.vn/folder/") >= 0) folders++;
                                else if (l.indexOf("fshare.vn/file/") >= 0) files++;
                                else invalid++;
                            }
                            return { files: files, folders: folders, invalid: invalid, total: lines.length };
                        }

                        Rectangle {
                            visible: parent._analysis.files > 0
                            width: fileLinkLabel.implicitWidth + AuroraTheme.sp3 * 2
                            height: 22; radius: 11
                            color: AuroraTheme.accentSoft
                            border.width: 1; border.color: AuroraTheme.accentTint15

                            Text {
                                id: fileLinkLabel; anchors.centerIn: parent
                                text: parent.parent._analysis.files + " file"
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: AuroraTheme.accent
                            }
                        }

                        Rectangle {
                            visible: parent._analysis.folders > 0
                            width: folderLinkLabel.implicitWidth + AuroraTheme.sp3 * 2
                            height: 22; radius: 11
                            color: AuroraTheme.accentSoft
                            border.width: 1; border.color: AuroraTheme.accentTint15

                            Text {
                                id: folderLinkLabel; anchors.centerIn: parent
                                text: parent.parent._analysis.folders + " " + qsTr("thư mục")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: AuroraTheme.accent
                            }
                        }

                        Text {
                            visible: parent._analysis.invalid > 0
                            text: qsTr("⚠ %1 link không hợp lệ").arg(parent._analysis.invalid)
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 11
                            color: AuroraTheme.warn
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // Folder link note
                    Rectangle {
                        Layout.fillWidth: true
                        visible: {
                            const t = addDialog.linksText.toLowerCase();
                            return t.indexOf("fshare.vn/folder/") >= 0;
                        }
                        implicitHeight: folderNoteCol.implicitHeight + AuroraTheme.sp2 * 2
                        radius: AuroraTheme.radiusSm
                        color: AuroraTheme.panel
                        border.width: 1; border.color: AuroraTheme.border

                        ColumnLayout {
                            id: folderNoteCol
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.top: parent.top; anchors.margins: AuroraTheme.sp2
                            spacing: 2

                            Text {
                                text: qsTr("Link thư mục sẽ được quét tự động")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Tất cả file trong thư mục (bao gồm thư mục con) sẽ được thêm vào hàng đợi tải xuống.")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 11
                                color: AuroraTheme.ink3
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                FsFolderPicker {
                    Layout.fillWidth: true
                    label: qsTr("Thư mục lưu")
                    placeholder: downloadViewModel ? downloadViewModel.defaultSaveFolder : ""
                    hint: qsTr("Để trống sẽ dùng thư mục mặc định.")
                    dialogTitle: qsTr("Chọn thư mục tải xuống")
                    folder: addDialog.folderText
                    onFolderChanged: addDialog.folderText = folder
                }

                FsTextField {
                    Layout.fillWidth: true
                    label: qsTr("Mật khẩu (nếu có)")
                    placeholder: qsTr("Nhập mật khẩu nếu file được bảo vệ")
                    echoMode: TextInput.Password
                    text: addDialog.passwordText
                    onTextChanged: addDialog.passwordText = text
                    // Enter on the last field submits — saves a tab from
                    // the password to the primary button.  The button's
                    // own enabled-state mirrors the validation so we
                    // delegate instead of re-checking here.
                    onAccepted: if (downloadBtn.enabled) downloadBtn.clicked()
                }
            }
        }

        footer: Item {
            width: 520
            height: 64

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                spacing: AuroraTheme.sp2

                Item { Layout.fillWidth: true }

                Aurora.FsButton {
                    text: qsTr("Huỷ")
                    variant: "ghost"
                    onClicked: addDialog.close()
                }
                Aurora.FsButton {
                    id: downloadBtn
                    text: qsTr("Tải xuống")
                    variant: "primary"
                    enabled: addDialog.linksText.trim().length > 0
                    onClicked: {
                        if (downloadViewModel) {
                            downloadViewModel.addDownload(
                                addDialog.linksText.trim(),
                                addDialog.folderText.trim(),
                                addDialog.passwordText
                            )
                        }
                        addDialog.linksText = ""
                        addDialog.passwordText = ""
                        addDialog.close()
                    }
                }
            }
        }
    }
}
