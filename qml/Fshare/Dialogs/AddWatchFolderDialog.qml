// SPDX-License-Identifier: Proprietary
// AddWatchFolderDialog — 2-step wizard for adding a watched folder

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora
import Fshare.Components 1.0

FsDialog {
    id: root
    title: qsTr("Thêm thư mục đồng bộ")
    dialogWidth: 520

    property int step: 1  // 1 = choose folders, 2 = options

    // When true, Step 1 surfaces the Fshare remote-folder tree so the user can
    // pick the destination explicitly.  When false (current SyncService
    // contract, where the remote folder always shadows the local leaf name),
    // Step 1 collapses to just the local folder picker and shows a small
    // hint where the tree would otherwise render.  Toggle this to true once
    // SyncService grows a custom remote-destination API.
    property bool remoteFolderEditable: false

    // Step 1 state
    property string localPath: ""
    property string remoteFolderId: "0"
    property string remoteFolderPath: "/"

    // Step 2 state
    property bool watchSubfolders: true
    property bool deleteAfterUpload: false
    property string ignorePatterns: "*.tmp, *.part, Thumbs.db, desktop.ini, .DS_Store"

    // Preview
    property int previewFileCount: 0
    property string previewTotalSize: ""
    property string previewEstTime: ""
    property bool previewQuotaOk: true
    property bool previewLoading: false

    // Remote tree (populated by ViewModel)
    property var remoteFolderTree: []
    property bool remoteFolderLoading: false

    signal accepted(string localPath, string remoteFolderId, string remoteFolderPath,
                    bool watchSubfolders, bool deleteAfterUpload, string ignorePatterns)
    signal previewRequested(string localPath, bool watchSubfolders, string ignorePatterns)
    signal loadRemoteFolders()
    signal createRemoteFolder(string parentId)

    function open() {
        step = 1;
        localPath = "";
        remoteFolderId = "0";
        remoteFolderPath = "/";
        watchSubfolders = true;
        deleteAfterUpload = false;
        ignorePatterns = "*.tmp, *.part, Thumbs.db, desktop.ini, .DS_Store";
        previewFileCount = 0;
        previewLoading = false;
        visible = true;
        loadRemoteFolders();
    }

    function close() {
        visible = false;
    }

    // ── Content ──────────────────────────────────────
    content: Item {
        width: 520
        height: step === 1 ? step1Col.implicitHeight + sp24 * 2
                           : step2Col.implicitHeight + sp24 * 2

        readonly property real sp24: AuroraTheme.sp6

        Behavior on height {
            enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durBase; easing.type: Easing.OutCubic }
        }

        // ═══════════════ STEP 1 ═══════════════
        ColumnLayout {
            id: step1Col
            visible: root.step === 1
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: AuroraTheme.sp6
            spacing: AuroraTheme.sp4

            // Local folder picker
            FsFolderPicker {
                Layout.fillWidth: true
                label: qsTr("Thư mục trên máy tính")
                dialogTitle: qsTr("Chọn thư mục để đồng bộ tự động")
                folder: root.localPath
                onFolderChanged: root.localPath = folder
            }

            // Remote folder tree (editable mode) — selectable destination
            // under the user's Fshare account.  Kept opt-in via
            // remoteFolderEditable so the page can hide it when the
            // SyncService contract doesn't yet support custom destinations.
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.remoteFolderEditable
                spacing: AuroraTheme.sp1

                Text {
                    text: qsTr("Thư mục đích trên Fshare")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                }

                FsRemoteFolderTree {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    selectedFolderId: root.remoteFolderId
                    selectedFolderPath: root.remoteFolderPath
                    treeModel: root.remoteFolderTree
                    loading: root.remoteFolderLoading

                    onFolderSelected: function(folderId, folderPath) {
                        root.remoteFolderId = folderId;
                        root.remoteFolderPath = folderPath;
                    }
                    onExpandRequested: function(folderId) {
                        // ViewModel handles expansion
                    }
                    onCreateFolderRequested: function(parentId) {
                        root.createRemoteFolder(parentId);
                    }
                }
            }

            // Auto-destination hint — visible when remote-folder editing is
            // suppressed.  Uses accentTint to match the rest of the Aurora
            // info-banner family (preview hint, drop overlay) so the user
            // reads it as "info" rather than "warning".
            Rectangle {
                Layout.fillWidth: true
                visible: !root.remoteFolderEditable
                Layout.preferredHeight: autoHintRow.implicitHeight + AuroraTheme.sp3 * 2
                radius: 8
                color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                               AuroraTheme.accent.b, 0.06)
                border.width: 1
                border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                                       AuroraTheme.accent.b, 0.18)

                RowLayout {
                    id: autoHintRow
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp3
                    spacing: AuroraTheme.sp2

                    FsIcon { name: "folder"; sizePx: 16; color: AuroraTheme.accent }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Thư mục trên Fshare sẽ được tạo tự động theo tên thư mục đã chọn.")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.caption.pixelSize
                        color: AuroraTheme.ink2
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // ═══════════════ STEP 2 ═══════════════
        ColumnLayout {
            id: step2Col
            visible: root.step === 2
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: AuroraTheme.sp6
            spacing: AuroraTheme.sp4

            // Summary of selected paths
            Rectangle {
                Layout.fillWidth: true
                height: pathSummary.implicitHeight + AuroraTheme.sp3 * 2
                radius: 8
                color: AuroraTheme.divider

                Text {
                    id: pathSummary
                    anchors.centerIn: parent
                    width: parent.width - AuroraTheme.sp6
                    text: root.localPath + "  \u2192  Fshare: " + root.remoteFolderPath
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideMiddle
                }
            }

            // Options card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: optionsCol.implicitHeight + AuroraTheme.sp4 * 2
                radius: AuroraTheme.radiusMd
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border

                ColumnLayout {
                    id: optionsCol
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp4
                    spacing: 0

                    // Watch subfolders
                    OptionToggle {
                        Layout.fillWidth: true
                        label: qsTr("Theo dõi thư mục con")
                        desc: qsTr("Bao gồm file trong các thư mục con")
                        checked: root.watchSubfolders
                        onToggled: root.watchSubfolders = checked
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.divider }

                    // Delete after upload
                    OptionToggle {
                        Layout.fillWidth: true
                        label: qsTr("Xóa file local sau khi tải lên")
                        desc: qsTr("File sẽ bị xóa khỏi máy tính sau khi upload thành công và được xác nhận trên Fshare")
                        checked: root.deleteAfterUpload
                        onToggled: root.deleteAfterUpload = checked
                    }

                    // Delete warning
                    Rectangle {
                        Layout.fillWidth: true
                        visible: root.deleteAfterUpload
                        Layout.topMargin: AuroraTheme.sp2
                        height: warningText.implicitHeight + 10 * 2
                        radius: 8
                        color: AuroraTheme.warnSoft
                        border.width: 1
                        border.color: Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: AuroraTheme.sp2

                            Text {
                                text: "\u26A0"
                                font.pixelSize: 14
                                color: AuroraTheme.warn
                            }
                            Text {
                                id: warningText
                                Layout.fillWidth: true
                                text: qsTr("File sẽ bị xóa vĩnh viễn khỏi máy tính sau khi upload thành công. "
                                          + "Hành động này không thể hoàn tác. Đảm bảo bạn có bản sao lưu nếu cần.")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: AuroraTheme.caption.pixelSize
                                color: AuroraTheme.warn
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.divider;
                                Layout.topMargin: root.deleteAfterUpload ? AuroraTheme.sp2 : 0 }

                    // Ignore patterns
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: AuroraTheme.sp3
                        spacing: AuroraTheme.sp1

                        Text {
                            text: qsTr("Bỏ qua file theo mẫu")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: AuroraTheme.body.pixelSize
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink1
                        }
                        FsTextField {
                            Layout.fillWidth: true
                            text: root.ignorePatterns
                            hint: qsTr("Mỗi mẫu cách nhau bởi dấu phẩy")
                            onTextChanged: root.ignorePatterns = text
                            // Enter on this last Step-2 input commits the
                            // dialog — primary action is "Thêm" in Step 2.
                            // Step 1 doesn't use this field, so no branch
                            // needed.
                            onAccepted: if (primaryBtn.enabled) primaryBtn.clicked()
                        }
                    }
                }
            }

            // Preview hint box
            Rectangle {
                Layout.fillWidth: true
                visible: root.previewFileCount > 0 || root.previewLoading
                height: previewContent.implicitHeight + AuroraTheme.sp3 * 2
                radius: 8
                color: root.previewQuotaOk
                    ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.06)
                    : AuroraTheme.warnSoft
                border.width: 1
                border.color: root.previewQuotaOk
                    ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.18)
                    : Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25)

                RowLayout {
                    id: previewContent
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp3
                    spacing: AuroraTheme.sp2

                    Text {
                        text: root.previewQuotaOk ? "\uD83D\uDCA1" : "\u26A0"
                        font.pixelSize: 14
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.previewLoading
                            ? qsTr("Đang quét thư mục...")
                            : qsTr("Sẽ quét %1 file (%2) trong thư mục này.\n%3")
                                .arg(root.previewFileCount)
                                .arg(root.previewTotalSize)
                                .arg(qsTr("Quá trình tải lên sẽ bắt đầu sau khi thêm."))
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.caption.pixelSize
                        color: root.previewQuotaOk ? AuroraTheme.ink2 : AuroraTheme.warn
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    // ── Footer ───────────────────────────────────────
    footer: Item {
        width: 520
        height: 64

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp3
            anchors.bottomMargin: AuroraTheme.sp3

            Item { Layout.fillWidth: true }

            Aurora.FsButton {
                text: root.step === 1 ? qsTr("Hủy") : qsTr("Quay lại")
                variant: "ghost"
                onClicked: {
                    if (root.step === 2) { root.step = 1; }
                    else { root.close(); }
                }
            }

            Aurora.FsButton {
                id: primaryBtn
                text: root.step === 1 ? qsTr("Tiếp tục") : qsTr("Thêm")
                variant: "primary"
                enabled: root.step === 1
                    ? root.localPath.length > 0
                    : !root.previewLoading
                onClicked: {
                    if (root.step === 1) {
                        root.step = 2;
                        root.previewRequested(root.localPath, root.watchSubfolders, root.ignorePatterns);
                    } else {
                        root.accepted(root.localPath, root.remoteFolderId, root.remoteFolderPath,
                                      root.watchSubfolders, root.deleteAfterUpload, root.ignorePatterns);
                        root.close();
                    }
                }
            }
        }
    }

    // ── Inline helper ────────────────────────────────
    component OptionToggle: RowLayout {
        property string label: ""
        property string desc: ""
        property bool checked: false
        signal toggled()
        Layout.topMargin: AuroraTheme.sp3
        Layout.bottomMargin: AuroraTheme.sp3
        spacing: AuroraTheme.sp4

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: label
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
            }
            Text {
                visible: desc.length > 0
                text: desc
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink3
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        FsSwitch {
            checked: parent.checked
            onToggled: function(c) {
                parent.checked = c;
                parent.toggled();
            }
        }
    }
}
