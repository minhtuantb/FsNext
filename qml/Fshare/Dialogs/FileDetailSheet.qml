// SPDX-License-Identifier: Proprietary
// FileDetailSheet — Single-file detail view for a Fshare share URL.
//
// Opens when the user pastes a https://www.fshare.vn/file/<code> URL into
// the homepage search, or clicks a file row inside FolderBrowserDialog.
//
// Show: icon + name + size + uploaded-at + downloads + description.
// Actions: Play (video/audio only) · Download · Copy link · Open on fshare.vn.
// Password input appears inline when the file is password-protected; the
// same value is forwarded to both Play and Download so the user enters it
// once per session.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: root

    property var vm: remoteShareViewModel

    signal closed()

    function open()  { root.visible = true; root.forceActiveFocus(); }
    function close() {
        root.visible = false;
        if (vm) vm.close();
        passwordInput.text = "";
        root.closed();
    }

    anchors.fill: parent
    visible: false
    z: 1000

    focus: visible
    activeFocusOnTab: false
    Keys.onEscapePressed: function(event) { root.close(); event.accepted = true; }

    Accessible.role: Accessible.Dialog
    Accessible.name: qsTr("Chi tiết file chia sẻ")

    // ── Convenience accessors ─────────────────────────────────
    readonly property var _info: vm ? vm.currentFile : ({})
    readonly property string _name:     (_info && _info.name) || ""
    readonly property var    _size:     (_info && _info.size) || 0
    readonly property bool   _hasPwd:   !!(_info && _info.hasPassword)
    readonly property string _category: (_info && _info.fileCategory) || ""
    readonly property bool   _isMedia:  _category === "video" || _category === "audio"
    readonly property string _created:  (_info && _info.created) || ""
    readonly property int    _dlCount:  (_info && _info.downloadCount) || 0
    readonly property string _description: (_info && _info.description) || ""

    // ── Backdrop ───────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, root.visible ? 0.35 : 0)
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }
        MouseArea { anchors.fill: parent; onClicked: root.close() }
    }

    // ── Card ───────────────────────────────────────────────────
    Rectangle {
        id: card
        width: Math.min(520, parent.width - 64)
        anchors.centerIn: parent
        height: contentCol.implicitHeight + AuroraTheme.sp6 * 2
        radius: 16
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border
        scale:   root.visible ? 1.0 : 0.95
        opacity: root.visible ? 1.0 : 0.0
        Behavior on scale   { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durSlow; easing.type: Easing.OutBack } }
        Behavior on opacity { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durFast } }
        MouseArea { anchors.fill: parent }     // swallow

        // Close (top-right)
        Rectangle {
            anchors.right: parent.right
            anchors.top:   parent.top
            anchors.margins: AuroraTheme.sp3
            width: 30; height: 30
            radius: 8
            color: closeMa.containsMouse ? AuroraTheme.accentSoft : "transparent"
            z: 2
            Text {
                anchors.centerIn: parent
                text: "✕"; font.pixelSize: 16
                color: closeMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
            }
            MouseArea {
                id: closeMa; anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: root.close()
            }
        }

        ColumnLayout {
            id: contentCol
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp6
            spacing: AuroraTheme.sp4

            // ── Loading state (initial fetch) ──────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                visible: vm && vm.isLoading && root._name.length === 0
                spacing: AuroraTheme.sp3
                FsSpinner { Layout.alignment: Qt.AlignHCenter; running: parent.visible }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Đang lấy thông tin file…")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    color: AuroraTheme.ink3
                }
            }

            // ── Error state ────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                visible: vm && (vm.errorText || "").length > 0 && root._name.length === 0
                spacing: AuroraTheme.sp2
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 56; height: 56; radius: 28
                    color: AuroraTheme.dangerSoft
                    Text { anchors.centerIn: parent; text: "⚠"; font.pixelSize: 22; color: AuroraTheme.danger }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: vm ? vm.errorText : ""
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    color: AuroraTheme.ink2
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.maximumWidth: 360
                }
            }

            // ── Loaded state ───────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                visible: root._name.length > 0
                spacing: AuroraTheme.sp4

                // Big icon + title
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: AuroraTheme.sp2

                    FsFileTypeIcon {
                        Layout.alignment: Qt.AlignHCenter
                        fileName: root._name
                        isFolder: false
                        sizePx: 56
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.maximumWidth: card.width - AuroraTheme.sp6 * 4
                        text: root._name
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink1
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: {
                            let parts = [];
                            if (root._size > 0)     parts.push(FsFormat.bytes(root._size));
                            if (root._dlCount > 0)  parts.push(root._dlCount + " " + qsTr("lượt tải"));
                            if (root._created.length > 0) parts.push(root._created);
                            return parts.join(" · ");
                        }
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        color: AuroraTheme.ink3
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        visible: root._hasPwd
                        width: pwdHintLabel.implicitWidth + 16
                        height: 22
                        radius: 11
                        color: AuroraTheme.warnSoft
                        Text {
                            id: pwdHintLabel
                            anchors.centerIn: parent
                            text: "🔒 " + qsTr("File có mật khẩu")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: AuroraTheme.warn
                        }
                    }
                }

                // Description (collapsible if needed; for now show as-is)
                Text {
                    Layout.fillWidth: true
                    visible: root._description.length > 0
                    text: root._description
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 12
                    color: AuroraTheme.ink3
                    wrapMode: Text.WordWrap
                }

                // Password input — appears only when the file requires one.
                // Driven by FsTextField for consistency with the rest of the app.
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root._hasPwd
                    spacing: 4
                    Text {
                        text: qsTr("Mật khẩu")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink3
                    }
                    FsTextField {
                        id: passwordInput
                        Layout.fillWidth: true
                        placeholder: qsTr("Nhập mật khẩu để mở file")
                        echoMode: TextInput.Password
                    }
                }

                // Action buttons
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2

                    FsButton {
                        Layout.fillWidth: true
                        visible: root._isMedia
                        variant: "primary"
                        text: qsTr("▶  Xem trực tiếp")
                        enabled: !root._hasPwd || passwordInput.text.length > 0
                        onClicked: if (vm) vm.playCurrentFile(passwordInput.text)
                    }
                    FsButton {
                        Layout.fillWidth: true
                        variant: root._isMedia ? "secondary" : "primary"
                        text: qsTr("⬇  Tải về máy")
                        enabled: !root._hasPwd || passwordInput.text.length > 0
                        onClicked: if (vm) vm.downloadCurrentFile(passwordInput.text)
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AuroraTheme.sp2
                        FsButton {
                            Layout.fillWidth: true
                            variant: "ghost"
                            text: qsTr("⎘  Sao chép link")
                            onClicked: if (vm) vm.copyCurrentFileLink()
                        }
                        FsButton {
                            Layout.fillWidth: true
                            variant: "ghost"
                            text: qsTr("↗  Mở trên fshare.vn")
                            onClicked: if (vm) vm.openCurrentFileOnFshare()
                        }
                    }
                }
            }
        }
    }
}
