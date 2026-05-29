// SPDX-License-Identifier: Proprietary
// UploadPage (Aurora) — editorial header + big drop zone + queue + history.
//
// Matches handoff `aurora-screens.jsx → AuroraUpload`:
//   • Top panel: serif mega-number aggregate speed + mini stats + actions.
//   • Drop zone card: dashed accent border, gradient soft bg, serif italic
//     call-to-action; clicking "chọn từ máy" opens the upload dialog.
//   • Body: segmented Active/History tabs; rows use FsTransferItem.
//
// All VM wiring from the legacy page is preserved:
//   addUpload, pauseAll/resumeAll, pauseTask/resumeTask/cancelTask,
//   dismissCompleted, copyLinkToClipboard, revealLocalFile,
//   openShareLinkInBrowser, moveTaskUp/Down, deleteLocalFile, clearHistory,
//   loadMoreHistory/hasMoreHistory; signals onUploadError,
//   onLocalFileDeleted, onLocalFileDeleteFailed.

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import Fshare.Components 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: page

    property bool showHistory: false
    readonly property int activeCount: uploadViewModel ? uploadViewModel.model.count : 0
    readonly property int historyCount: uploadViewModel ? uploadViewModel.historyModel.count : 0

    // Highlighted task id — set by focusTask(), auto-cleared after 1500ms.
    // Mirrors DownloadPage's pattern; visual pulse can be wired in P3 by
    // having FsTransferItem watch this and animate a border colour.
    property string focusedTaskId: ""
    Timer {
        id: _focusClearTimer
        interval: 1500
        onTriggered: page.focusedTaskId = ""
    }

    // Called by Main.qml when the HUD VM's taskFocusRequested signal
    // routes to this page.  Switches to active tab, scrolls the active
    // ListView to the requested row, and sets the highlight property.
    // No-op when the task isn't in the active list (history-only rows
    // are skipped to avoid surprise scroll-to in a long history).
    function focusTask(taskId) {
        if (!uploadViewModel || !uploadViewModel.model) return;
        const idx = uploadViewModel.model.rowOfTask(taskId);
        if (idx < 0) return;
        page.showHistory = false;
        uploadActiveList.positionViewAtIndex(idx, ListView.Center);
        uploadActiveList.currentIndex = idx;
        page.focusedTaskId = taskId;
        _focusClearTimer.restart();
    }

    // Page-level clock for relative-time labels ("vừa xong" / "5 phút trước").
    // A single ticker beats per-row timers when the list grows long.
    property real nowMs: Date.now()
    Timer {
        interval: 30 * 1000
        running: true
        repeat: true
        onTriggered: page.nowMs = Date.now()
    }

    // ── Toasts ─────────────────────────────────────────────
    Connections {
        target: uploadViewModel
        function onUploadError(message) {
            errorToast.message = message;
            errorToast.visible = true;
            errorToastTimer.restart();
        }
        function onLocalFileDeleted(taskId) {
            errorToast.message = qsTr("Đã xoá file cục bộ.");
            errorToast.visible = true;
            errorToastTimer.restart();
        }
        function onLocalFileDeleteFailed(taskId, reason) {
            errorToast.message = reason;
            errorToast.visible = true;
            errorToastTimer.restart();
        }
    }

    // Drag-drop → options dialog, routed via uploadStagingViewModel (C++).
    // Staging state lives on AppContext, so it survives page-Loader teardown
    // (the previous QML-only design lost the batch every time the user
    // navigated away from Upload, and a Qt.callLater closure capturing the
    // destroyed dialog used to crash the app).
    //
    // We open the dialog only when the VM raises showRequested — i.e. the user
    // just dropped or picked a file. A passive sidebar navigation back to
    // Upload while staging is leftover does NOT auto-pop the dialog (that
    // would feel like the app pushing a modal on the user). The "Thêm file"
    // / drop / Ctrl+U surfaces are still available to bring it back.
    function _maybeOpenStaging() {
        if (!uploadStagingViewModel) return;
        if (uploadStagingViewModel.restoredFromDisk) return;   // banner path
        if (!uploadStagingViewModel.showRequested)   return;
        if (uploadDialog.visible) {
            uploadStagingViewModel.acknowledgeShow();
            return;
        }
        // Defer one tick — when this fires from globalDropArea.onDropped, the
        // OS drag-drop (OLE on Windows) event is still being delivered.
        // Opening a modal / forceActiveFocus inside that callback corrupts the
        // drag session and silently kills future drops. Qt.callLater puts us
        // on the next event-loop tick, after the drop event has unwound.
        Qt.callLater(() => {
            if (!uploadStagingViewModel) return;
            if (!uploadStagingViewModel.showRequested) return;
            if (!uploadDialog.visible) uploadDialog.open();
            uploadStagingViewModel.acknowledgeShow();
        });
    }
    Component.onCompleted: {
        console.info("[UploadPage] onCompleted hasStaged=",
                     uploadStagingViewModel ? uploadStagingViewModel.hasStaged : "no-vm",
                     " showReq=",
                     uploadStagingViewModel ? uploadStagingViewModel.showRequested : "?",
                     " restored=",
                     uploadStagingViewModel ? uploadStagingViewModel.restoredFromDisk : "?");
        _maybeOpenStaging();
    }
    Connections {
        target: uploadStagingViewModel
        ignoreUnknownSignals: true
        // VM raises this whenever addFiles() actually adds at least one file —
        // covers drop / Ctrl+U / "chọn từ máy" picker / programmatic
        // openUploadWithFiles. Sidebar nav alone does NOT raise it.
        function onShowRequestedChanged() { _maybeOpenStaging(); }
    }
    Connections {
        target: Window.window
        ignoreUnknownSignals: true
        // Ctrl+U shortcut path: open the OS file picker directly. Picked files
        // flow into the staging VM same as drops, which auto-opens the dialog.
        function onOpenUploadDialog() {
            ctrlUFilePicker.open();
        }
    }

    FileDialog {
        id: ctrlUFilePicker
        title:    qsTr("Chọn file để tải lên")
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            const urls = [];
            for (let i = 0; i < selectedFiles.length; ++i)
                urls.push(selectedFiles[i].toString());
            if (urls.length === 0) return;
            // Stage via the VM (dedupe, persistence, showRequested). The VM
            // raises showRequested → _maybeOpenStaging() opens the dialog —
            // same path as drag-drop, so behaviour is uniform across entries.
            if (uploadStagingViewModel) uploadStagingViewModel.addFiles(urls);
        }
    }

    // Navigate to MyFiles with the destination folder loaded. folderId is
    // the upload destination (Fshare path / linkcode — empty for root).
    function _openInMyFiles(folderId, linkcode) {
        const rootWin = Window.window;
        if (!rootWin) return;
        rootWin.currentPage = Pages.files;
        if (fileManagerViewModel)
            fileManagerViewModel.loadFolder(folderId === "/" ? "" : folderId);
    }

    function _askDeleteLocal(taskId, fileName) {
        deleteLocalDialog._pendingTaskId = taskId;
        deleteLocalDialog.message = qsTr("Xoá bản sao cục bộ của \"%1\"? Hành động này không thể hoàn tác.")
                                      .arg(fileName);
        deleteLocalDialog.open();
    }

    FsConfirmDialog {
        id: deleteLocalDialog
        property string _pendingTaskId: ""
        title: qsTr("Xoá file cục bộ?")
        primaryLabel: qsTr("Xoá")
        cancelLabel: qsTr("Huỷ")
        dangerAction: true
        onConfirmed: {
            if (_pendingTaskId.length > 0 && uploadViewModel)
                uploadViewModel.deleteLocalFile(_pendingTaskId);
            _pendingTaskId = "";
        }
        onCancelled: _pendingTaskId = ""
    }

    // Compact toast strip
    Rectangle {
        id: errorToast
        property string message: ""
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: AuroraTheme.sp4
        z: 100
        visible: false
        width: Math.min(parent.width - AuroraTheme.sp8, toastText.implicitWidth + AuroraTheme.sp6)
        height: toastText.implicitHeight + AuroraTheme.sp4
        radius: AuroraTheme.radiusMd
        color: AuroraTheme.dangerSoft
        border.width: 1
        border.color: Qt.rgba(AuroraTheme.danger.r, AuroraTheme.danger.g, AuroraTheme.danger.b, 0.30)
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: AuroraTheme.durFast } }

        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - AuroraTheme.sp6
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: errorToast.message
            font.family: AuroraTheme.fontSans
            font.pixelSize: 12
            color: AuroraTheme.danger
        }

        Timer {
            id: errorToastTimer
            interval: 4000
            onTriggered: errorToast.visible = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: AuroraTheme.sp4

        // ═════════════════════════════════════════════════
        //  EDITORIAL HEADER PANEL
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: uploadHeaderCol.implicitHeight + AuroraTheme.sp6 * 2
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            ColumnLayout {
                id: uploadHeaderCol
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp6
                spacing: AuroraTheme.sp4

                // Spacing trimmed sp6 → sp4 — matches DownloadPage. Earlier
                // value let the trailing action button overflow the panel
                // border on 1100-wide windows with the sidebar expanded.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp4

                    ColumnLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            text: page.showHistory
                                ? "━━ Lịch sử · " + page.historyCount + " file"
                                : "━━ Tải lên · " + page.activeCount + " file"
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
                                property string _speed: uploadViewModel ? uploadViewModel.totalSpeed : ""
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

                    // Stats internal spacing also sp6 → sp4 (mirror of
                    // DownloadPage). "Tổng tác vụ" relabelled to "Tổng file"
                    // — clearer for end-users + visually narrower.
                    RowLayout {
                        visible: !page.showHistory
                        spacing: AuroraTheme.sp4
                        Layout.alignment: Qt.AlignVCenter

                        Repeater {
                            model: [
                                {
                                    label: "Đang chạy",
                                    value: (transferBudgetViewModel
                                        ? transferBudgetViewModel.activeUploads + "/" + transferBudgetViewModel.maxUploads
                                        : "—")
                                },
                                {
                                    label: "Hàng đợi",
                                    value: (transferBudgetViewModel
                                        ? String(transferBudgetViewModel.pendingUploads)
                                        : "—")
                                },
                                {
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

                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: AuroraTheme.sp2

                        FsButton {
                            readonly property string _run: uploadViewModel ? uploadViewModel.runState : "idle"
                            visible: !page.showHistory && _run !== "idle"
                            text: _run === "running" ? qsTr("Tạm dừng tất cả") : qsTr("Tiếp tục tất cả")
                            icon: _run === "running" ? "pause" : "play"
                            variant: "secondary"
                            size: "md"
                            onClicked: {
                                if (!uploadViewModel) return;
                                if (_run === "running") uploadViewModel.pauseAll();
                                else                    uploadViewModel.resumeAll();
                            }
                        }

                        FsButton {
                            visible: page.showHistory && page.historyCount > 0
                            text: qsTr("Xoá lịch sử")
                            variant: "ghost"
                            size: "md"
                            onClicked: if (uploadViewModel) uploadViewModel.clearHistory()
                        }

                        FsButton {
                            text: qsTr("Thêm file")
                            icon: "plus"
                            variant: "primary"
                            size: "md"
                            onClicked: uploadDialog.open()
                        }
                    }
                }

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
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  RESTORE FROM PREVIOUS SESSION BANNER
        //  Shown only when the staging VM has hydrated files from disk on app
        //  launch. We intentionally DON'T auto-pop the upload dialog here — a
        //  modal appearing seconds after launch feels intrusive. Instead the
        //  banner asks the user to opt back in (or discard).
        // ═════════════════════════════════════════════════
        Rectangle {
            id: restoreBanner
            Layout.fillWidth: true
            Layout.preferredHeight: restoreBannerRow.implicitHeight + AuroraTheme.sp4 * 2
            visible: uploadStagingViewModel
                  && uploadStagingViewModel.restoredFromDisk
                  && uploadStagingViewModel.hasStaged
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.accentSoft
            border.width: 1
            border.color: AuroraTheme.accent

            RowLayout {
                id: restoreBannerRow
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp4
                spacing: AuroraTheme.sp3

                FsIcon {
                    Layout.preferredWidth:  24
                    Layout.preferredHeight: 24
                    name: "history"
                    sizePx: 24
                    color: AuroraTheme.accent
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: qsTr("Còn %1 file từ phiên trước chưa tải lên")
                                .arg(uploadStagingViewModel
                                     ? uploadStagingViewModel.stagedFiles.length : 0)
                        color: AuroraTheme.ink1
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: qsTr("Tiếp tục để mở lại bảng tải lên, hoặc bỏ để xoá khỏi danh sách.")
                        color: AuroraTheme.ink3
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                    }
                }

                FsButton {
                    text:    qsTr("Tiếp tục")
                    variant: "primary"
                    size:    "sm"
                    onClicked: {
                        if (!uploadStagingViewModel) return;
                        uploadStagingViewModel.acknowledgeRestored();
                        uploadDialog.open();
                    }
                }
                FsButton {
                    text:    qsTr("Bỏ")
                    variant: "ghost"
                    size:    "sm"
                    onClicked: if (uploadStagingViewModel) uploadStagingViewModel.clear()
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  DROP ZONE
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: dropRow.implicitHeight + AuroraTheme.sp6 * 2
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.accentSoft
            visible: !page.showHistory

            // Dashed accent border (Qt QML doesn't expose a native dashed
            // border — we fake it with a transparent Rectangle on top that
            // uses a solid accent border, then overlay the gradient mark).
            border.width: 2
            border.color: dropArea.containsDrag ? AuroraTheme.accent
                       : Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.45)
            Behavior on border.color { enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast } }

            DropArea {
                id: dropArea
                anchors.fill: parent
                // Visual feedback (`containsDrag`) is per-DropArea, so the
                // dashed border still highlights even though Main.qml's
                // globalDropArea is the one that actually handles the drop
                // (it sits above this in z-order). Keeping the onDropped
                // here as a fallback for the edge case where globalDropArea
                // is disabled (logged-out state, etc.).
                onEntered: (drag) => console.info("[UploadPage:innerDrop] entered hasUrls=", drag.hasUrls)
                onDropped: (drop) => {
                    console.info("[UploadPage:innerDrop] dropped urls=", drop.urls);
                    if (Window.window && Window.window.routeDrop)
                        Window.window.routeDrop(drop);
                }
            }

            RowLayout {
                id: dropRow
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp6
                spacing: AuroraTheme.sp5

                // Gradient upload mark
                Item {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 64
                    Aurora.FsGradientRect { anchors.fill: parent; radius: AuroraTheme.radiusLg }
                    FsIcon {
                        anchors.centerIn: parent
                        name: "upload"
                        sizePx: 28
                        color: "#FFFFFF"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: dropArea.containsDrag
                            ? qsTr("Thả để tải lên.")
                            : qsTr("Kéo thả bất kỳ vào đây.")
                        color: AuroraTheme.ink1
                        font.family: AuroraTheme.fontSerif
                        font.italic: true
                        font.pixelSize: 26
                        font.letterSpacing: -0.5
                    }

                    RowLayout {
                        spacing: 4
                        Text {
                            text: qsTr("Hoặc")
                            color: AuroraTheme.ink3
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                        }
                        Text {
                            text: qsTr("chọn từ máy")
                            color: AuroraTheme.accent
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: uploadDialog.open()
                            }
                        }
                        Text {
                            text: qsTr("— tối đa 50 GB mỗi file · tự động resume")
                            color: AuroraTheme.ink3
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 6

                    Repeater {
                        model: [qsTr("Ưu tiên · VIP"), qsTr("Mã hoá TLS 1.3")]
                        delegate: Rectangle {
                            Layout.alignment: Qt.AlignRight
                            implicitWidth: chipTxt.implicitWidth + 16
                            implicitHeight: 22
                            radius: 11
                            color: AuroraTheme.panel
                            border.width: 1
                            border.color: AuroraTheme.border
                            Text {
                                id: chipTxt
                                anchors.centerIn: parent
                                text: modelData
                                color: AuroraTheme.ink3
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.letterSpacing: 0.4
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  LIST AREA
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            FsEmptyState {
                anchors.fill: parent
                visible: !page.showHistory && page.activeCount === 0 && !dropArea.containsDrag
                icon: "↑"
                title: qsTr("Chưa có tác vụ tải lên")
                description: qsTr("Kéo thả file vào vùng phía trên, hoặc nhấn “Thêm file”.")
                actionText: qsTr("Thêm file")
                onActionClicked: uploadDialog.open()
            }

            FsEmptyState {
                anchors.fill: parent
                visible: page.showHistory && page.historyCount === 0
                icon: "✓"
                title: qsTr("Chưa có lịch sử tải lên")
                description: qsTr("Các file đã tải lên sẽ hiển thị ở đây.")
            }

            ListView {
                id: uploadActiveList
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp2
                clip: true
                visible: !page.showHistory && page.activeCount > 0
                model: uploadViewModel ? uploadViewModel.model : null

                // P3 polish — focused row highlight ring.  Mirrors the
                // DownloadPage pattern so HUD-driven focusTask() lands
                // with consistent visual feedback across both surfaces.
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
                    // Use taskId for pause/resume/cancel (UUID), linkCode only
                    // for the copy-link action on completed items.
                    transferId: model.taskId || ""
                    fileName: model.fileName || ""
                    fileSize: model.fileSize || 0
                    bytesTransferred: model.fileSize > 0 ? model.fileSize * (model.progress || 0) / 100 : 0
                    progress: model.progress || 0
                    speed: model.speed || 0
                    eta: model.eta || ""
                    status: model.status || 0
                    errorMessage: model.errorMessage || ""
                    completedAt: model.completedAt || 0
                    nowMs: page.nowMs
                    showReorder: (model.status || 0) === 0
                    showOpenInBrowser: true
                    showInfoButton:    true
                    showDeleteLocal:   true
                    // The Fshare URL is only known after the upload completes;
                    // FsTransferItem's copy-link button keys off this being
                    // non-empty, so the button auto-appears at that moment.
                    linkcode:          model.linkCode || ""

                    onPauseClicked:        if (uploadViewModel) uploadViewModel.pauseTask(transferId)
                    onResumeClicked:       if (uploadViewModel) uploadViewModel.resumeTask(transferId)
                    onCancelClicked:       if (uploadViewModel) uploadViewModel.cancelTask(transferId)
                    onDismissClicked:      if (uploadViewModel) uploadViewModel.dismissCompleted(transferId)
                    onCopyLinkClicked:     if (uploadViewModel) uploadViewModel.copyLinkToClipboard(transferId)
                    onOpenFolderClicked:   if (uploadViewModel) uploadViewModel.revealLocalFile(transferId)
                    onOpenInBrowserClicked:if (uploadViewModel) uploadViewModel.openShareLinkInBrowser(transferId)
                    onShowInfoClicked:     page._openInMyFiles(model.folderId || "", model.linkCode || "")
                    onDeleteLocalClicked:  page._askDeleteLocal(transferId, fileName)
                    onMoveUpClicked:       if (uploadViewModel) uploadViewModel.moveTaskUp(transferId)
                    onMoveDownClicked:     if (uploadViewModel) uploadViewModel.moveTaskDown(transferId)
                }
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }

            ListView {
                id: historyList
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp2
                clip: true
                visible: page.showHistory && page.historyCount > 0
                model: uploadViewModel ? uploadViewModel.historyModel : null

                delegate: FsTransferItem {
                    width: ListView.view ? ListView.view.width : 0
                    transferId: model.taskId || ""
                    fileName: model.fileName || ""
                    fileSize: model.fileSize || 0
                    bytesTransferred: model.fileSize || 0
                    progress: 100
                    status: 3
                    completedAt: model.completedAt || 0
                    nowMs: page.nowMs
                    showOpenInBrowser: true
                    showInfoButton:    true
                    showDeleteLocal:   true
                    linkcode:          model.linkCode || ""

                    onCopyLinkClicked:     if (uploadViewModel) uploadViewModel.copyLinkToClipboard(transferId)
                    onOpenFolderClicked:   if (uploadViewModel) uploadViewModel.revealLocalFile(transferId)
                    onOpenInBrowserClicked:if (uploadViewModel) uploadViewModel.openShareLinkInBrowser(transferId)
                    onShowInfoClicked:     page._openInMyFiles(model.folderId || "", model.linkCode || "")
                    onDeleteLocalClicked:  page._askDeleteLocal(transferId, fileName)
                }
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // Infinite-scroll: as the user reaches the bottom of the list,
                // fetch the next page of older history rows from the DB.
                footer: Item {
                    width: historyList.width
                    height: (uploadViewModel && uploadViewModel.hasMoreHistory)
                              ? AuroraTheme.sp10 : 0
                    visible: uploadViewModel && uploadViewModel.hasMoreHistory
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Đang tải thêm…")
                        color: AuroraTheme.ink4
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                    }
                }
                onAtYEndChanged: {
                    if (atYEnd && uploadViewModel && uploadViewModel.hasMoreHistory)
                        uploadViewModel.loadMoreHistory()
                }
            }
        }
    }

    FsUploadDialog {
        id: uploadDialog

        // Bind the staging model so the dialog's file list, folder choice and
        // privacy flags read/write the AppContext-owned VM. The dialog itself
        // is short-lived (dies when UploadPage is unloaded by the Loader); the
        // VM is the persistent home for the user's in-progress batch.
        stagingModel: uploadStagingViewModel ?? null

        // Preferred path: hand the C++ FolderPickerModel directly so the
        // dropdown reads precomputed label/id arrays without QML needing
        // to iterate the folder tree. This is what made the Upload tab
        // freeze for power users with thousands of folders.
        folderPickerModel: fileManagerViewModel
                           ? fileManagerViewModel.folderPickerModel : null
    }
}
