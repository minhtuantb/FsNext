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

    // Drag-drop → options dialog. The global router in Main.qml forwards
    // local file URLs here instead of kicking off an upload directly, so
    // the user still picks target folder / password / privacy before the
    // transfer starts.
    Connections {
        target: Window.window
        ignoreUnknownSignals: true
        function onOpenUploadWithFiles(fileUrls) {
            if (!fileUrls || fileUrls.length === 0) return;
            uploadDialog.pendingFiles = [];
            uploadDialog._addFiles(fileUrls);
            uploadDialog.open();
        }
    }

    // Navigate to MyFiles with the destination folder loaded. folderId is
    // the upload destination (Fshare path / linkcode — empty for root).
    function _openInMyFiles(folderId, linkcode) {
        const rootWin = Window.window;
        if (!rootWin) return;
        rootWin.currentPage = 3;
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

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp6

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

                    RowLayout {
                        visible: !page.showHistory
                        spacing: AuroraTheme.sp6
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
                                    label: "Tổng tác vụ",
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

                        Aurora.FsButton {
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

                        Aurora.FsButton {
                            visible: page.showHistory && page.historyCount > 0
                            text: qsTr("Xoá lịch sử")
                            variant: "ghost"
                            size: "md"
                            onClicked: if (uploadViewModel) uploadViewModel.clearHistory()
                        }

                        Aurora.FsButton {
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
                onDropped: (drop) => {
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
                    Aurora.FsIcon {
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
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp2
                clip: true
                visible: !page.showHistory && page.activeCount > 0
                model: uploadViewModel ? uploadViewModel.model : null

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

        // Preferred path: hand the C++ FolderPickerModel directly so the
        // dropdown reads precomputed label/id arrays without QML needing
        // to iterate the folder tree. This is what made the Upload tab
        // freeze for power users with thousands of folders.
        folderPickerModel: fileManagerViewModel
                           ? fileManagerViewModel.folderPickerModel : null

        onUploadStarted: (files, folder) => {
            if (!uploadViewModel) return;
            const paths = files.map(f => f.path);
            uploadViewModel.addUpload(paths, folder,
                                      "",
                                      uploadDialog.password ?? "",
                                      uploadDialog.secured ?? false,
                                      false);
        }
    }
}
