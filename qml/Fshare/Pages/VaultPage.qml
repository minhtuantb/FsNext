// SPDX-License-Identifier: Proprietary
// VaultPage — encryption vault surface. Routes the 3 home states (no-vault /
// locked / unlocked) plus the Intro page, and hosts the Unlock dialog and a
// compact Create dialog (the full 7-step wizard replaces the latter in U4).
//
// Binds to the `vaultViewModel` context property (see AppContext).

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import Fshare.Components 1.0
import FsAurora.Theme 1.0

Item {
    id: page

    property var vm: (typeof vaultViewModel !== "undefined") ? vaultViewModel : null
    readonly property int ls: vm ? vm.lockState : 0          // 0 none · 1 locked · 2 unlocked
    readonly property bool busy: vm ? vm.busy : false
    readonly property bool fileBusy: vm ? vm.fileBusy : false
    property bool showIntro: false
    property bool showKeys: false
    property bool showSettings: false
    property string errorText: ""
    property bool menuOpen: false
    property var largeFiles: []      // pending DLG-LARGEFILE entries (from VM)
    signal notify(string title, bool isError)

    readonly property string mode: showIntro ? "intro"
        : (showKeys ? "keys"
        : (showSettings ? "vsettings"
        : (ls === 0 ? "empty" : (ls === 1 ? "locked" : "unlocked"))))

    function _keyTypeLabel(ty) {
        return ty === "vault" ? qsTr("Vault") : (ty === "recovery" ? qsTr("Khôi phục") : qsTr("Tệp khóa"));
    }
    function _keyTypeTone(ty) {
        return ty === "vault" ? "accent" : (ty === "recovery" ? "success" : "info");
    }

    // ── auto-lock countdown (cosmetic; the VM owns the real timer) ──
    property int _countdown: 0
    function _resetCountdown() { _countdown = (vm ? vm.autoLockMinutes : 15) * 60 }
    onLsChanged: { if (ls === 2) _resetCountdown(); if (ls !== 2) { showKeys = false; showSettings = false; } }
    Component.onCompleted: _resetCountdown()
    Timer {
        running: page.mode === "unlocked"
        interval: 1000; repeat: true
        onTriggered: if (page._countdown > 0) page._countdown--
    }
    function _mmss(s) {
        var m = Math.floor(s / 60), ss = s % 60;
        return (m < 10 ? "0" + m : m) + ":" + (ss < 10 ? "0" + ss : ss);
    }

    function _genericError(code) {
        if (code === "WrongKey" || code === "KdfFailed")
            return qsTr("Mở khóa thất bại. Hãy kiểm tra lại passphrase.");
        if (code === "AlreadyExists")
            return qsTr("Vault đã tồn tại tại vị trí này.");
        return qsTr("Đã xảy ra lỗi. Vui lòng thử lại.");
    }

    Connections {
        target: page.vm
        function onOperationFinished(op, ok, error) {
            if (op === "changePassphrase") {
                if (ok) { chgPassDialog.close(); page.notify(qsTr("Đã đổi passphrase"), false); }
                else if (error !== "Busy")
                    chgPassDialog.err = (error === "WrongKey" || error === "KdfFailed")
                        ? qsTr("Passphrase hiện tại không đúng.") : page._genericError(error);
                return;
            }
            if (op === "export") {
                if (ok) { exportDialog.done = true; page.notify(qsTr("Đã lưu tệp khóa khôi phục"), false); }
                else if (error !== "Busy") exportDialog.err = page._genericError(error);
                return;
            }
            if (op === "delete") {
                if (ok) page.notify(qsTr("Đã xóa cấu hình Vault khỏi máy này."), false);
                return;
            }
            if (op === "importKey") {
                page.notify(ok ? qsTr("Đã nhập khóa") : qsTr("Tệp khóa không hợp lệ"), !ok);
                return;
            }
            // unlock / unlockKeyfile
            if (ok) {
                page.errorText = "";
                if (op === "unlock" || op === "unlockKeyfile") unlockDialog.close();
            } else if (error !== "Busy") {
                page.errorText = page._genericError(error);
            }
        }
        function onLargeFilesPending(files) {
            page.largeFiles = files;
            largeFileDialog.open();
        }
        function onTempCleaned(count) {
            page.notify(qsTr("Đã xóa bản giải mã tạm an toàn."), false);
        }
        function onFileUploadQueued(name) {
            page.notify(qsTr("Đang tải lên Fshare: ") + name, false);
        }
        function onBatchProgress(done, total, currentName) {
            batchDialog.doneCount = done;
            batchDialog.totalCount = total;
            batchDialog.currentName = currentName;
        }
        function onFileOpFinished(op, ok, name, error) {
            if (op === "encrypt") {
                if (batchDialog.visible && batchDialog.step === "progress") {
                    var parts = name.split("/");
                    batchDialog.okCount = parseInt(parts[0]) || 0;
                    batchDialog.totalShown = parseInt(parts[1]) || 0;
                    batchDialog.step = "done";
                }
                page.notify(ok ? qsTr("Đã mã hóa & thêm vào Vault")
                               : qsTr("Một số file mã hóa thất bại"), !ok);
            } else if (op === "decrypt") {
                if (ok) { decryptDialog.close(); page.notify(qsTr("Đã mở: ") + name, false); }
                else decryptDialog.err = (error === "Tampered")
                    ? qsTr("File có thể đã hỏng hoặc bị can thiệp.")
                    : qsTr("Giải mã thất bại.");
            }
        }
    }

    // ════════════════════════════════════════════
    //  HEADER (adaptive)
    // ════════════════════════════════════════════
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp2
            anchors.rightMargin: AuroraTheme.sp2
            spacing: AuroraTheme.sp3

            // Back (intro / keys / settings)
            FsButton {
                visible: page.mode === "intro" || page.mode === "keys" || page.mode === "vsettings"
                text: qsTr("Quay lại")
                variant: "ghost"
                icon: "arrow-left"
                onClicked: { page.showIntro = false; page.showKeys = false; page.showSettings = false; }
            }

            // Icon chip
            Rectangle {
                visible: page.mode !== "intro" && page.mode !== "keys" && page.mode !== "vsettings"
                Layout.preferredWidth: 38
                Layout.preferredHeight: 38
                radius: AuroraTheme.radiusMd
                color: page.mode === "unlocked" ? AuroraTheme.accentSoft : AuroraTheme.sunk
                FsIcon {
                    anchors.centerIn: parent
                    name: page.mode === "unlocked" ? "lock-open" : (page.mode === "locked" ? "lock" : "shield-key")
                    sizePx: 19
                    color: page.mode === "unlocked" ? AuroraTheme.accent : AuroraTheme.ink3
                }
            }

            ColumnLayout {
                spacing: 1
                RowLayout {
                    spacing: AuroraTheme.sp2
                    Text {
                        text: page.mode === "intro" ? qsTr("Giới thiệu Vault")
                            : page.mode === "keys" ? qsTr("Quản lý khóa")
                            : page.mode === "vsettings" ? qsTr("Cài đặt Vault")
                            : (page.ls === 0 ? qsTr("Vault") : (page.vm ? page.vm.vaultName : "Vault"))
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.h2.pixelSize
                        font.weight: Font.Bold
                        color: AuroraTheme.ink1
                    }
                    FsBadge {
                        visible: page.mode === "locked"
                        text: qsTr("Đang khóa"); variant: "warn"
                    }
                    FsBadge {
                        visible: page.mode === "unlocked"
                        text: qsTr("Đã mở"); variant: "success"
                    }
                }
                Text {
                    visible: page.mode === "unlocked"
                    text: qsTr("Tự khóa sau ") + page._mmss(page._countdown)
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    color: AuroraTheme.ink4
                }
            }

            Item { Layout.fillWidth: true }

            // Actions (unlocked)
            FsButton {
                visible: page.mode === "unlocked"
                text: qsTr("Khóa ngay"); variant: "ghost"; icon: "lock"
                onClicked: if (page.vm) page.vm.lock()
            }
            FsButton {
                visible: page.mode === "unlocked"
                text: page.fileBusy ? qsTr("Đang mã hóa…") : qsTr("Thêm file")
                variant: "primary"; icon: "plus"
                enabled: !page.fileBusy
                onClicked: filesPickDialog.open()
            }
            FsButton {
                visible: page.mode === "unlocked"
                text: qsTr("Thêm thư mục"); variant: "ghost"; icon: "folder-lock"
                enabled: !page.fileBusy
                onClicked: addFolderDialog.open()
            }
            // Help (locked + unlocked)
            FsButton {
                visible: page.mode === "locked" || page.mode === "unlocked"
                text: qsTr("Trợ giúp"); variant: "ghost"; icon: "info"
                onClicked: page.showIntro = true
            }
            // Overflow menu (unlocked)
            FsButton {
                visible: page.mode === "unlocked"
                text: ""; variant: "ghost"; icon: "menu"
                accessibleName: qsTr("Thêm")
                onClicked: page.menuOpen = !page.menuOpen
            }
        }

        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
    }

    // ════════════════════════════════════════════
    //  CONTENT
    // ════════════════════════════════════════════
    Item {
        id: body
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // ── Intro ──
        Loader {
            anchors.fill: parent
            active: page.mode === "intro"
            visible: active
            sourceComponent: VaultIntroPage {
                hasVault: page.ls !== 0
                onCreateRequested: { page.showIntro = false; wizard.open(); }
                onGoHomeRequested: page.showIntro = false
            }
        }

        // ── Empty (no vault) ──
        ColumnLayout {
            visible: page.mode === "empty"
            anchors.centerIn: parent
            width: Math.min(440, parent.width - AuroraTheme.sp8)
            spacing: AuroraTheme.sp4

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 72; Layout.preferredHeight: 72; radius: 20
                color: AuroraTheme.accentSoft
                FsIcon { anchors.centerIn: parent; name: "shield-key"; sizePx: 38; color: AuroraTheme.accent }
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Bảo vệ tài liệu nhạy cảm của bạn")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.h3.pixelSize
                font.weight: Font.Bold
                color: AuroraTheme.ink1
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Mã hóa ngay trên máy · Server không đọc được · Bạn giữ chìa khóa.")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                color: AuroraTheme.ink3
                wrapMode: Text.WordWrap
            }
            FsButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: AuroraTheme.sp2
                text: qsTr("Tạo Vault"); variant: "primary"; icon: "plus"
                onClicked: wizard.open()
            }
            FsButton {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Tìm hiểu về mã hóa"); variant: "link"
                onClicked: page.showIntro = true
            }
        }

        // ── Locked ──
        ColumnLayout {
            visible: page.mode === "locked"
            anchors.centerIn: parent
            spacing: AuroraTheme.sp4
            FsIcon { Layout.alignment: Qt.AlignHCenter; name: "lock"; sizePx: 40; color: AuroraTheme.ink4 }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Vault đang khóa")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 15; font.weight: Font.DemiBold
                color: AuroraTheme.ink2
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Mở khóa để xem và quản lý file đã mã hóa.")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13; color: AuroraTheme.ink3
            }
            FsButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: AuroraTheme.sp1
                text: qsTr("Mở khóa"); variant: "primary"; icon: "lock-open"
                onClicked: { page.errorText = ""; unlockDialog.open(); }
            }
        }

        // ── Unlocked: empty state ──
        ColumnLayout {
            visible: page.mode === "unlocked" && (!page.vm || page.vm.vaultFiles.length === 0)
            anchors.centerIn: parent
            spacing: AuroraTheme.sp3
            FsIcon { Layout.alignment: Qt.AlignHCenter; name: "folder-lock"; sizePx: 40; color: AuroraTheme.ink4 }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Vault trống")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 15; font.weight: Font.DemiBold
                color: AuroraTheme.ink2
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Kéo file vào đây hoặc bấm “Thêm file” để mã hóa.")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13; color: AuroraTheme.ink3
            }
        }

        // ── Unlocked: file list ──
        ListView {
            id: fileList
            visible: page.mode === "unlocked" && page.vm && page.vm.vaultFiles.length > 0
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp4
            clip: true
            spacing: AuroraTheme.sp2
            model: page.vm ? page.vm.vaultFiles : []
            delegate: Rectangle {
                id: fileRow
                required property var modelData
                width: ListView.view ? ListView.view.width : 0
                height: 60
                radius: AuroraTheme.radiusMd
                color: rowMa.containsMouse ? AuroraTheme.bgWarm : AuroraTheme.panel
                border.width: 1; border.color: AuroraTheme.border
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: AuroraTheme.sp3; anchors.rightMargin: AuroraTheme.sp3
                    spacing: AuroraTheme.sp3
                    FsFileTypeIcon { fileName: fileRow.modelData.name; sizePx: 40; locked: true }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 2
                        Text { Layout.fillWidth: true; text: fileRow.modelData.name; elide: Text.ElideMiddle
                            font.family: AuroraTheme.fontSans; font.pixelSize: 13; font.weight: Font.DemiBold; color: AuroraTheme.ink1 }
                        RowLayout {
                            spacing: AuroraTheme.sp2
                            FsBadge { text: qsTr("Đã mã hóa"); variant: "success" }
                            Text { text: fileRow.modelData.sizeText; font.family: AuroraTheme.fontMono
                                font.pixelSize: 11; color: AuroraTheme.ink4 }
                        }
                    }
                    FsButton {
                        text: qsTr("Mở"); variant: "ghost"; icon: "lock-open"
                        enabled: !page.fileBusy
                        onClicked: { decryptDialog.err = ""; decryptDialog.open(); page.vm.decryptAndOpen(fileRow.modelData.path); }
                    }
                }
                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton   // hover only; actions via the button
                }
            }
        }

        // ── Keys (Key Manager) ──
        ColumnLayout {
            visible: page.mode === "keys"
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp4
            spacing: AuroraTheme.sp3

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Các khóa đã và đang dùng trên máy này.")
                    font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink3
                }
                FsButton { text: qsTr("Nhập khóa…"); variant: "ghost"; icon: "key-file"; onClicked: importKeyDialog.open() }
            }
            FsCallout {
                Layout.fillWidth: true; tone: "warn"
                body: qsTr("Hãy giữ thêm một bản sao lưu khóa ở nơi an toàn, tách biệt với máy này (USB, két, trình quản lý mật khẩu). Mất khóa đồng nghĩa không mở được file đã mã hóa bằng khóa đó.")
            }
            Text {
                visible: !page.vm || page.vm.keys.length === 0
                Layout.fillWidth: true; Layout.topMargin: AuroraTheme.sp6
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Chưa có khóa nào.")
                font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink4
            }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true; spacing: AuroraTheme.sp2
                model: page.vm ? page.vm.keys : []
                delegate: Rectangle {
                    id: keyRow
                    required property var modelData
                    width: ListView.view ? ListView.view.width : 0
                    height: 66
                    radius: AuroraTheme.radiusMd
                    color: AuroraTheme.panel
                    border.width: 1; border.color: AuroraTheme.border
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: AuroraTheme.sp3; anchors.rightMargin: AuroraTheme.sp3
                        spacing: AuroraTheme.sp3
                        Rectangle {
                            Layout.preferredWidth: 40; Layout.preferredHeight: 40; radius: 11
                            color: AuroraTheme.accentSoft
                            FsIcon { anchors.centerIn: parent; name: "key"; sizePx: 18; color: AuroraTheme.accent }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 3
                            RowLayout {
                                spacing: AuroraTheme.sp2
                                Text { text: keyRow.modelData.label; font.family: AuroraTheme.fontSans
                                    font.pixelSize: 14; font.weight: Font.DemiBold; color: AuroraTheme.ink1 }
                                FsBadge { text: page._keyTypeLabel(keyRow.modelData.type); variant: page._keyTypeTone(keyRow.modelData.type) }
                                FsBadge { visible: keyRow.modelData.backedUp; text: qsTr("Đã sao lưu"); variant: "success" }
                                FsBadge { visible: !keyRow.modelData.backedUp; text: qsTr("Chưa sao lưu"); variant: "warn" }
                            }
                            Text { Layout.fillWidth: true; text: keyRow.modelData.fingerprint; elide: Text.ElideRight
                                font.family: AuroraTheme.fontMono; font.pixelSize: 11; color: AuroraTheme.ink3 }
                        }
                        FsButton {
                            text: qsTr("Sao chép vân tay"); variant: "ghost"; icon: "copy"
                            onClicked: { page.vm.copyFingerprint(keyRow.modelData.fingerprint); page.notify(qsTr("Đã sao chép vân tay"), false); }
                        }
                        FsButton {
                            visible: keyRow.modelData.type === "keyfile"
                            text: ""; variant: "ghost"; icon: "trash"; accessibleName: qsTr("Xóa khóa")
                            onClicked: page.vm.removeKey(keyRow.modelData.id)
                        }
                    }
                }
            }
        }

        // ── Vault settings (SET-VAULT) ──
        Flickable {
            id: vsFlick
            visible: page.mode === "vsettings"
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp4
            contentHeight: vsCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: vsCol
                width: vsFlick.width
                spacing: AuroraTheme.sp3

                // Status
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 2
                        Text { text: page.vm ? page.vm.vaultName : ""; font.family: AuroraTheme.fontSans
                            font.pixelSize: 15; font.weight: Font.Bold; color: AuroraTheme.ink1 }
                        Text { Layout.fillWidth: true; text: page.vm ? page.vm.vaultDir : ""; elide: Text.ElideMiddle
                            font.family: AuroraTheme.fontMono; font.pixelSize: 11; color: AuroraTheme.ink3 }
                    }
                    FsBadge { text: qsTr("Đã mở"); variant: "success" }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AuroraTheme.divider; Layout.topMargin: AuroraTheme.sp1 }

                Text { text: qsTr("BẢO MẬT"); font.family: AuroraTheme.fontMono; font.pixelSize: 11
                    font.weight: Font.Bold; color: AuroraTheme.ink4; Layout.topMargin: AuroraTheme.sp1 }
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Tự khóa sau"); font.family: AuroraTheme.fontSans
                        font.pixelSize: 13; color: AuroraTheme.ink1 }
                    FsSegmentedControl {
                        options: [ { value: 5, label: "5m" }, { value: 10, label: "10m" },
                                   { value: 15, label: "15m" }, { value: 30, label: "30m" }, { value: 60, label: "60m" } ]
                        selectedValue: page.vm ? page.vm.autoLockMinutes : 15
                        onSelectionChanged: (value) => { if (page.vm) page.vm.autoLockMinutes = value }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Cấp lưu khóa"); font.family: AuroraTheme.fontSans
                        font.pixelSize: 13; color: AuroraTheme.ink1 }
                    Text {
                        text: !page.vm ? "" : (page.vm.storageLevel === 1 ? qsTr("Cao")
                              : (page.vm.storageLevel === 3 ? qsTr("Tiện lợi") : qsTr("Vừa")))
                        font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink3
                    }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AuroraTheme.divider; Layout.topMargin: AuroraTheme.sp1 }

                Text { text: qsTr("QUẢN LÝ KHÓA"); font.family: AuroraTheme.fontMono; font.pixelSize: 11
                    font.weight: Font.Bold; color: AuroraTheme.ink4; Layout.topMargin: AuroraTheme.sp1 }
                SetActionRow { label: qsTr("Quản lý khóa"); btnText: qsTr("Mở"); onTriggered: { page.showSettings = false; page.showKeys = true; } }
                SetActionRow { label: qsTr("Đổi passphrase"); btnText: qsTr("Đổi…"); onTriggered: chgPassDialog.open() }
                SetActionRow { label: qsTr("Xuất khóa khôi phục"); btnText: qsTr("Xuất…"); onTriggered: exportDialog.open() }
                SetActionRow { label: qsTr("Nhập khóa khôi phục"); btnText: qsTr("Chọn file…"); onTriggered: importKeyDialog.open() }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AuroraTheme.divider; Layout.topMargin: AuroraTheme.sp1 }

                Text { text: qsTr("MÃ HÓA"); font.family: AuroraTheme.fontMono; font.pixelSize: 11
                    font.weight: Font.Bold; color: AuroraTheme.ink4; Layout.topMargin: AuroraTheme.sp1 }

                // Auto-encrypt size cap
                ColumnLayout {
                    Layout.fillWidth: true; spacing: AuroraTheme.sp1
                    Text { text: qsTr("Tự động mã hóa file đến"); font.family: AuroraTheme.fontSans
                        font.pixelSize: 13; color: AuroraTheme.ink1 }
                    FsSegmentedControl {
                        options: [ { value: 0, label: qsTr("Không giới hạn") },
                                   { value: 1024, label: qsTr("1 GB") },
                                   { value: 5120, label: qsTr("5 GB") } ]
                        selectedValue: page.vm ? page.vm.maxEncryptMb : 0
                        onSelectionChanged: (value) => { if (page.vm) page.vm.maxEncryptMb = value }
                    }
                }

                // Over-limit behaviour
                ColumnLayout {
                    Layout.fillWidth: true; spacing: AuroraTheme.sp1
                    Text { text: qsTr("Khi file vượt ngưỡng"); font.family: AuroraTheme.fontSans
                        font.pixelSize: 13; color: AuroraTheme.ink1 }
                    FsSegmentedControl {
                        options: [ { value: 0, label: qsTr("Hỏi mỗi lần") },
                                   { value: 1, label: qsTr("Bỏ qua") },
                                   { value: 2, label: qsTr("Thư mục thường") } ]
                        selectedValue: page.vm ? page.vm.overLimitBehavior : 0
                        onSelectionChanged: (value) => { if (page.vm) page.vm.overLimitBehavior = value }
                    }
                }

                // Auto-upload toggle
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 1
                        Text { text: qsTr("Tự tải lên Fshare sau khi mã hóa"); font.family: AuroraTheme.fontSans
                            font.pixelSize: 13; color: AuroraTheme.ink1 }
                        Text { text: qsTr("File .fshenc sẽ được tải lên thư mục đích bên dưới."); font.family: AuroraTheme.fontSans
                            font.pixelSize: 11; color: AuroraTheme.ink4 }
                    }
                    FsSwitch {
                        checked: page.vm ? page.vm.autoUploadDefault : false
                        onToggled: (checked) => { if (page.vm) page.vm.autoUploadDefault = checked }
                    }
                }
                FsTextField {
                    Layout.fillWidth: true
                    visible: page.vm && page.vm.autoUploadDefault
                    label: qsTr("Thư mục đích trên Fshare")
                    text: page.vm ? page.vm.uploadFolder : "/"
                    leadingIcon: "cloud-up"
                    mono: true
                    hint: qsTr("Nhấn Enter để lưu (vd: / hoặc /Vault)")
                    onAccepted: if (page.vm) page.vm.uploadFolder = text
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AuroraTheme.divider; Layout.topMargin: AuroraTheme.sp1 }

                Text { text: qsTr("NGUY HIỂM"); font.family: AuroraTheme.fontMono; font.pixelSize: 11
                    font.weight: Font.Bold; color: AuroraTheme.danger; Layout.topMargin: AuroraTheme.sp1 }
                SetActionRow { label: qsTr("Xóa Vault"); btnText: qsTr("Xóa Vault"); dangerBtn: true; onTriggered: delVaultDialog.open() }
            }
        }

        // Drag-drop: encrypt dropped files into the vault.
        DropArea {
            anchors.fill: parent
            enabled: page.mode === "unlocked" && !page.fileBusy
            onDropped: (drop) => {
                if (page.vm && drop.hasUrls)
                    page.vm.addFiles(drop.urls.map(u => u.toString()));
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp4
                visible: parent.containsDrag
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.accentSoft
                border.color: AuroraTheme.accent
                border.width: 2
                Text {
                    anchors.centerIn: parent
                    text: qsTr("Thả vào đây để mã hóa & thêm vào Vault")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 14; font.weight: Font.DemiBold
                    color: AuroraTheme.accent
                }
            }
        }
    }

    // ════════════════════════════════════════════
    //  UNLOCK DIALOG
    // ════════════════════════════════════════════
    FsDialog {
        id: unlockDialog
        title: qsTr("Mở khóa Vault")
        dialogWidth: 440
        closeOnOverlayClick: !page.busy
        onClosed: { unlockPass.text = ""; page.errorText = ""; }

        content: [
            Item {
                width: unlockDialog.dialogWidth
                implicitHeight: ucol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: ucol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3
                    FsTextField {
                        id: unlockPass
                        Layout.fillWidth: true
                        label: qsTr("Passphrase")
                        echoMode: TextInput.Password
                        reveal: true
                        leadingIcon: "key"
                        error: page.errorText
                        onAccepted: if (!page.busy && text.length > 0 && page.vm) page.vm.unlock(text)
                    }
                    RowLayout {
                        visible: page.busy
                        spacing: AuroraTheme.sp2
                        FsSpinner { sizePx: 16 }
                        Text {
                            text: qsTr("Đang xác thực…")
                            font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink3
                        }
                    }
                    Text {
                        text: qsTr("Quên passphrase? Vault không thể khôi phục nếu không có khóa khôi phục.")
                        font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink4
                        wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4
                spacing: AuroraTheme.sp2
                FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Huỷ"); variant: "ghost"
                    enabled: !page.busy
                    onClicked: unlockDialog.close()
                }
                FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Mở khóa"); variant: "primary"; icon: "lock-open"
                    enabled: !page.busy && unlockPass.text.length > 0
                    onClicked: if (page.vm) page.vm.unlock(unlockPass.text)
                }
            }
        ]
    }

    // ── Overflow menu (unlocked) ──
    MouseArea {
        anchors.fill: parent
        visible: page.menuOpen
        z: 40
        onClicked: page.menuOpen = false
    }
    Rectangle {
        id: headerMenu
        visible: page.menuOpen && page.mode === "unlocked"
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.rightMargin: AuroraTheme.sp2
        anchors.topMargin: 2
        width: 230
        height: menuCol.implicitHeight + AuroraTheme.sp2 * 2
        radius: AuroraTheme.radiusMd
        color: AuroraTheme.panel
        border.width: 1; border.color: AuroraTheme.border
        z: 50
        ColumnLayout {
            id: menuCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.margins: AuroraTheme.sp1
            spacing: 0
            VaultMenuRow { iconName: "shield-key"; label: qsTr("Quản lý khóa"); onClicked: { page.menuOpen = false; page.showKeys = true; } }
            VaultMenuRow { iconName: "key"; label: qsTr("Đổi passphrase"); onClicked: { page.menuOpen = false; chgPassDialog.open(); } }
            VaultMenuRow { iconName: "save"; label: qsTr("Xuất khóa khôi phục"); onClicked: { page.menuOpen = false; exportDialog.open(); } }
            VaultMenuRow { iconName: "menu"; label: qsTr("Cài đặt Vault"); onClicked: { page.menuOpen = false; page.showSettings = true; } }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.topMargin: 4; Layout.bottomMargin: 4; color: AuroraTheme.divider }
            VaultMenuRow { iconName: "trash"; label: qsTr("Xóa Vault"); danger: true; onClicked: { page.menuOpen = false; delVaultDialog.open(); } }
        }
    }

    FileDialog {
        id: exportFileDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Tệp khóa (*.key)"), qsTr("Tất cả (*)")]
        onAccepted: if (page.vm) page.vm.exportRecoveryKeyfile(selectedFile.toString())
    }

    FileDialog {
        id: filesPickDialog
        fileMode: FileDialog.OpenFiles
        onAccepted: if (page.vm) page.vm.addFiles(selectedFiles.map(u => u.toString()))
    }

    FileDialog {
        id: importKeyDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Tệp khóa (*.key)"), qsTr("Tất cả (*)")]
        onAccepted: if (page.vm) page.vm.importKeyfile(selectedFile.toString())
    }

    FolderDialog {
        id: addFolderDialog
        onAccepted: {
            batchDialog.srcFolder   = selectedFolder.toString();
            batchDialog.autoUpload  = page.vm ? page.vm.autoUploadDefault : false;
            batchDialog.cloudFolder = page.vm ? page.vm.uploadFolder : "/";
            batchDialog.step = "config";
            batchDialog.open();
        }
    }

    // ════════════════════════════════════════════
    //  DLG-DECRYPT
    // ════════════════════════════════════════════
    FsDialog {
        id: decryptDialog
        title: qsTr("Mở file")
        dialogWidth: 420
        closeOnOverlayClick: !page.fileBusy
        property string err: ""
        onClosed: decryptDialog.err = ""
        content: [
            Item {
                width: decryptDialog.dialogWidth
                implicitHeight: dccol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: dccol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3
                    RowLayout {
                        visible: page.fileBusy && decryptDialog.err === ""
                        spacing: AuroraTheme.sp2
                        FsSpinner { sizePx: 18 }
                        Text { text: qsTr("Đang giải mã & mở file…"); font.family: AuroraTheme.fontSans
                            font.pixelSize: 13; color: AuroraTheme.ink2 }
                    }
                    FsCallout {
                        visible: !page.fileBusy && decryptDialog.err === ""
                        Layout.fillWidth: true; tone: "info"; compact: true
                        body: qsTr("Bản giải mã tạm sẽ được mở bằng ứng dụng mặc định.")
                    }
                    FsCallout {
                        visible: decryptDialog.err !== ""
                        Layout.fillWidth: true; tone: "danger"
                        title: qsTr("Không thể mở an toàn")
                        body: decryptDialog.err
                    }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp2
                FsButton { anchors.verticalCenter: parent.verticalCenter
                    text: page.fileBusy ? qsTr("Hủy") : qsTr("Đóng"); variant: "ghost"
                    onClicked: decryptDialog.close() }
            }
        ]
    }

    // ════════════════════════════════════════════
    //  DLG-CHGPASS
    // ════════════════════════════════════════════
    FsDialog {
        id: chgPassDialog
        title: qsTr("Đổi passphrase")
        dialogWidth: 460
        closeOnOverlayClick: !page.busy
        property string err: ""
        onClosed: { curPass.text = ""; newPass.text = ""; newPass2.text = ""; chgPassDialog.err = ""; }
        content: [
            Item {
                width: chgPassDialog.dialogWidth
                implicitHeight: cgcol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: cgcol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3
                    FsTextField { id: curPass; Layout.fillWidth: true; label: qsTr("Passphrase hiện tại")
                        echoMode: TextInput.Password; reveal: true; leadingIcon: "key"; mono: true
                        error: chgPassDialog.err; onTextChanged: chgPassDialog.err = "" }
                    FsTextField { id: newPass; Layout.fillWidth: true; label: qsTr("Passphrase mới")
                        echoMode: TextInput.Password; reveal: true; leadingIcon: "key"; mono: true
                        hint: qsTr("Tối thiểu 12 ký tự") }
                    FsStrengthMeter { Layout.fillWidth: true; password: newPass.text }
                    FsTextField { id: newPass2; Layout.fillWidth: true; label: qsTr("Nhập lại passphrase mới")
                        echoMode: TextInput.Password; reveal: true; leadingIcon: "key"; mono: true
                        valid: newPass2.text.length > 0 && newPass2.text === newPass.text
                        error: (newPass2.text.length > 0 && newPass2.text !== newPass.text) ? qsTr("Passphrase chưa khớp") : "" }
                    FsCallout { Layout.fillWidth: true; tone: "info"; compact: true
                        body: qsTr("File đã mã hóa vẫn dùng được bình thường, không cần mã hóa lại.") }
                    RowLayout { visible: page.busy; spacing: AuroraTheme.sp2
                        FsSpinner { sizePx: 16 }
                        Text { text: qsTr("Đang đổi…"); font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink3 } }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp2
                FsButton { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Hủy"); variant: "ghost"
                    enabled: !page.busy; onClicked: chgPassDialog.close() }
                FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Đổi passphrase"); variant: "primary"
                    enabled: !page.busy && curPass.text.length > 0 && newPass.text.length >= 12
                             && newPass.text === newPass2.text
                    onClicked: if (page.vm) page.vm.changePassphrase(curPass.text, newPass.text, 0)
                }
            }
        ]
    }

    // ════════════════════════════════════════════
    //  DLG-EXPORT
    // ════════════════════════════════════════════
    FsDialog {
        id: exportDialog
        title: qsTr("Xuất khóa khôi phục")
        dialogWidth: 460
        property bool done: false
        property string err: ""
        onClosed: { exportDialog.done = false; exportDialog.err = ""; }
        content: [
            Item {
                width: exportDialog.dialogWidth
                implicitHeight: excol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: excol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3
                    Text { visible: !exportDialog.done; Layout.fillWidth: true; wrapMode: Text.WordWrap
                        text: qsTr("Tạo một tệp .key giúp bạn mở Vault nếu quên passphrase.")
                        font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink2 }
                    FsButton { visible: !exportDialog.done; text: qsTr("Chọn nơi lưu & xuất (.key)")
                        variant: "primary"; icon: "save"; onClicked: exportFileDialog.open() }
                    Rectangle {
                        visible: exportDialog.done
                        Layout.fillWidth: true; radius: AuroraTheme.radiusMd
                        color: AuroraTheme.successSoft
                        border.width: 1; border.color: Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.2)
                        implicitHeight: 48
                        RowLayout { anchors.fill: parent; anchors.margins: AuroraTheme.sp3; spacing: AuroraTheme.sp3
                            FsIcon { name: "shield-check"; sizePx: 20; color: AuroraTheme.success }
                            Text { Layout.fillWidth: true; text: qsTr("Đã lưu tệp khóa khôi phục")
                                font.family: AuroraTheme.fontSans; font.pixelSize: 13; font.weight: Font.DemiBold; color: AuroraTheme.ink1 } }
                    }
                    FsCallout { Layout.fillWidth: true; tone: "warn"; compact: true
                        body: qsTr("Bất kỳ ai có tệp này đều mở được Vault. Hãy bảo quản như chìa khóa nhà.") }
                    Text { visible: exportDialog.err !== ""; text: exportDialog.err; color: AuroraTheme.danger
                        font.family: AuroraTheme.fontSans; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp2
                FsButton { anchors.verticalCenter: parent.verticalCenter
                    text: exportDialog.done ? qsTr("Đóng") : qsTr("Hủy"); variant: "ghost"
                    onClicked: exportDialog.close() }
            }
        ]
    }

    // ════════════════════════════════════════════
    //  DLG-DELVAULT
    // ════════════════════════════════════════════
    FsDialog {
        id: delVaultDialog
        title: qsTr("Xóa Vault")
        dialogWidth: 480
        onClosed: delName.text = ""
        content: [
            Item {
                width: delVaultDialog.dialogWidth
                implicitHeight: dvcol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: dvcol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3
                    FsCallout { Layout.fillWidth: true; tone: "danger"; compact: true
                        body: qsTr("Thao tác này xóa cấu hình Vault khỏi máy này. File đã tải lên cloud vẫn còn, nhưng sẽ không mở được nếu bạn không còn giữ khóa.") }
                    Text { Layout.fillWidth: true; wrapMode: Text.WordWrap
                        text: qsTr("Gõ tên Vault để xác nhận: ") + (page.vm ? page.vm.vaultName : "")
                        font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink2 }
                    FsTextField { id: delName; Layout.fillWidth: true; placeholder: page.vm ? page.vm.vaultName : "" }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp2
                FsButton { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Hủy"); variant: "ghost"
                    onClicked: delVaultDialog.close() }
                FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Xóa Vault"); variant: "danger"; icon: "trash"
                    enabled: delName.text.length > 0 && delName.text === (page.vm ? page.vm.vaultName : "")
                    onClicked: { if (page.vm) page.vm.deleteVault(); delVaultDialog.close(); }
                }
            }
        ]
    }

    // ════════════════════════════════════════════
    //  DLG-LARGEFILE
    // ════════════════════════════════════════════
    FsDialog {
        id: largeFileDialog
        title: qsTr("Mã hóa file lớn?")
        dialogWidth: 480
        closeOnOverlayClick: false
        content: [
            Item {
                width: largeFileDialog.dialogWidth
                implicitHeight: lfcol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: lfcol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3
                    FsCallout {
                        Layout.fillWidth: true; tone: "info"; compact: true
                        body: qsTr("%n file lớn vượt ngưỡng mã hóa — bạn muốn xử lý thế nào? Lựa chọn áp dụng cho tất cả file lớn trong lượt này.", "", page.largeFiles.length)
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: AuroraTheme.sp1
                        Repeater {
                            model: page.largeFiles
                            delegate: RowLayout {
                                id: lfRow
                                required property var modelData
                                Layout.fillWidth: true; spacing: AuroraTheme.sp2
                                FsIcon { name: "alert-triangle"; sizePx: 14; color: AuroraTheme.warn }
                                Text { Layout.fillWidth: true; text: lfRow.modelData.name; elide: Text.ElideMiddle
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink2 }
                                Text { text: lfRow.modelData.sizeText; font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11; color: AuroraTheme.ink4 }
                            }
                        }
                    }
                    FsCallout {
                        Layout.fillWidth: true; tone: "warn"; compact: true
                        body: qsTr("File lớn phải được giải mã toàn bộ trước khi xem (không xem trực tiếp) và cần thêm dung lượng đĩa tạm thời.")
                    }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp2
                FsButton { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Hủy"); variant: "ghost"
                    onClicked: { if (page.vm) page.vm.resolveLargeFiles("skip"); largeFileDialog.close(); } }
                FsButton { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Thêm không mã hóa"); variant: "secondary"
                    onClicked: { if (page.vm) page.vm.resolveLargeFiles("plain"); largeFileDialog.close(); } }
                FsButton { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Mã hóa & thêm"); variant: "primary"; icon: "shield-key"
                    onClicked: { if (page.vm) page.vm.resolveLargeFiles("encrypt"); largeFileDialog.close(); } }
            }
        ]
    }

    // ════════════════════════════════════════════
    //  DLG-BATCH (folder: config → progress → done)
    // ════════════════════════════════════════════
    FsDialog {
        id: batchDialog
        title: qsTr("Mã hóa thư mục")
        dialogWidth: 480
        closeOnOverlayClick: batchDialog.step !== "progress"
        property string step: "config"      // config | progress | done
        property string srcFolder: ""
        property bool   autoUpload: false
        property string cloudFolder: "/"
        property int    doneCount: 0
        property int    totalCount: 0
        property int    okCount: 0
        property int    totalShown: 0
        property string currentName: ""
        onClosed: { batchDialog.step = "config"; batchDialog.doneCount = 0; batchDialog.totalCount = 0;
                    batchDialog.currentName = ""; }

        content: [
            Item {
                width: batchDialog.dialogWidth
                implicitHeight: bdcol.implicitHeight + AuroraTheme.sp6 * 2
                ColumnLayout {
                    id: bdcol
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp6
                    spacing: AuroraTheme.sp3

                    // ── CONFIG ──
                    ColumnLayout {
                        visible: batchDialog.step === "config"
                        Layout.fillWidth: true; spacing: AuroraTheme.sp3
                        RowLayout {
                            Layout.fillWidth: true; spacing: AuroraTheme.sp2
                            FsIcon { name: "folder-lock"; sizePx: 18; color: AuroraTheme.accent }
                            Text { Layout.fillWidth: true; elide: Text.ElideMiddle
                                text: batchDialog.srcFolder.replace("file:///", "")
                                font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.ink2 }
                        }
                        Text { Layout.fillWidth: true; wrapMode: Text.WordWrap
                            text: qsTr("Tất cả file trong thư mục (kể cả thư mục con) sẽ được mã hóa & thêm vào Vault.")
                            font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink3 }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.fillWidth: true; text: qsTr("Tự tải lên Fshare sau khi mã hóa")
                                font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink1 }
                            FsSwitch { checked: batchDialog.autoUpload; onToggled: (c) => batchDialog.autoUpload = c }
                        }
                        FsTextField {
                            id: bdCloud
                            Layout.fillWidth: true; visible: batchDialog.autoUpload
                            label: qsTr("Thư mục đích trên Fshare")
                            text: batchDialog.cloudFolder; leadingIcon: "cloud-up"; mono: true
                        }
                    }

                    // ── PROGRESS ──
                    ColumnLayout {
                        visible: batchDialog.step === "progress"
                        Layout.fillWidth: true; spacing: AuroraTheme.sp3
                        FsProgressBar {
                            Layout.fillWidth: true; status: "uploading"
                            value: batchDialog.totalCount > 0 ? batchDialog.doneCount / batchDialog.totalCount : 0
                            indeterminate: batchDialog.totalCount === 0
                        }
                        Text {
                            Layout.fillWidth: true; elide: Text.ElideMiddle
                            text: qsTr("Đang xử lý ") + batchDialog.doneCount + "/" + batchDialog.totalCount
                                  + (batchDialog.currentName ? " · " + batchDialog.currentName : "")
                            font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink3
                        }
                    }

                    // ── DONE ──
                    ColumnLayout {
                        visible: batchDialog.step === "done"
                        Layout.fillWidth: true; spacing: AuroraTheme.sp3
                        FsCallout {
                            Layout.fillWidth: true
                            tone: batchDialog.okCount === batchDialog.totalShown ? "success" : "warn"
                            body: qsTr("Hoàn tất: ") + batchDialog.okCount + "/" + batchDialog.totalShown
                                  + qsTr(" file thành công.")
                        }
                    }
                }
            }
        ]
        footer: [
            Row {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp2

                FsButton { visible: batchDialog.step === "config"; anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Hủy"); variant: "ghost"; onClicked: batchDialog.close() }
                FsButton {
                    visible: batchDialog.step === "config"; anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Bắt đầu"); variant: "primary"; icon: "shield-key"
                    onClicked: {
                        batchDialog.cloudFolder = bdCloud.text;
                        batchDialog.step = "progress"; batchDialog.doneCount = 0; batchDialog.totalCount = 0;
                        if (page.vm) page.vm.addFolderUpload(batchDialog.srcFolder,
                                                             batchDialog.autoUpload, batchDialog.cloudFolder);
                    }
                }

                FsButton { visible: batchDialog.step === "progress"; anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Hủy"); variant: "danger"; onClicked: if (page.vm) page.vm.cancelBatch() }

                FsButton {
                    visible: batchDialog.step === "done" && batchDialog.okCount < batchDialog.totalShown
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Thử lại file lỗi"); variant: "secondary"
                    onClicked: {
                        batchDialog.step = "progress"; batchDialog.doneCount = 0; batchDialog.totalCount = 0;
                        if (page.vm) page.vm.retryFailed();
                    }
                }
                FsButton { visible: batchDialog.step === "done"; anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Đóng"); variant: "primary"; onClicked: batchDialog.close() }
            }
        ]
    }

    // ── overflow-menu row ──
    component VaultMenuRow: Rectangle {
        id: mrow
        property string iconName: ""
        property string label: ""
        property bool danger: false
        signal clicked()
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        radius: AuroraTheme.radiusSm
        color: mrowMa.containsMouse ? AuroraTheme.bgWarm : "transparent"
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: AuroraTheme.sp3; anchors.rightMargin: AuroraTheme.sp3
            spacing: AuroraTheme.sp3
            FsIcon { name: mrow.iconName; sizePx: 16; color: mrow.danger ? AuroraTheme.danger : AuroraTheme.ink3 }
            Text { Layout.fillWidth: true; text: mrow.label; font.family: AuroraTheme.fontSans; font.pixelSize: 13
                color: mrow.danger ? AuroraTheme.danger : AuroraTheme.ink2 }
        }
        MouseArea { id: mrowMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: mrow.clicked() }
    }

    // ── settings action row (label + button) ──
    component SetActionRow: RowLayout {
        id: sar
        property string label: ""
        property string btnText: qsTr("Mở")
        property bool dangerBtn: false
        signal triggered()
        Layout.fillWidth: true
        spacing: AuroraTheme.sp3
        Text { Layout.fillWidth: true; text: sar.label; font.family: AuroraTheme.fontSans
            font.pixelSize: 13; color: sar.dangerBtn ? AuroraTheme.danger : AuroraTheme.ink1 }
        FsButton { text: sar.btnText; variant: sar.dangerBtn ? "danger" : "ghost"
            onClicked: sar.triggered() }
    }

    // ════════════════════════════════════════════
    //  CREATE WIZARD (WIZ-1..7)
    // ════════════════════════════════════════════
    VaultWizard {
        id: wizard
    }
}
