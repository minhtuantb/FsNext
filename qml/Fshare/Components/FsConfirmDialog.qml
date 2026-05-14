// SPDX-License-Identifier: Proprietary
// FsConfirmDialog — Preset confirm/cancel dialog built on FsDialog.
//
// Usage:
//   FsConfirmDialog {
//       id: confirm
//       title: qsTr("Xóa file?")
//       message: qsTr("Hành động này không thể hoàn tác.")
//       primaryLabel: qsTr("Xóa")
//       cancelLabel: qsTr("Hủy")
//       dangerAction: true
//       onConfirmed: doDelete()
//   }
//   confirm.open()

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
FsDialog {
    id: root

    property string message: ""
    property string primaryLabel: qsTr("Xác nhận")
    property string cancelLabel: qsTr("Hủy")
    property bool dangerAction: false
    property bool primaryEnabled: true

    signal confirmed()
    signal cancelled()

    dialogWidth: 420

    // Destructive confirms (dangerAction=true) get focus parked on Cancel
    // for the same reason FileDeleteDialog does — an over-eager Enter must
    // not auto-fire destruction.  Non-danger confirms (e.g. "Scan now")
    // route focus to the primary button so Enter is a fast "yes".
    onOpened: Qt.callLater(() => {
        if (dangerAction) cancelConfirmBtn.forceActiveFocus();
        else              primaryConfirmBtn.forceActiveFocus();
    })

    content: [
        Item {
            width: root.dialogWidth
            height: msgText.implicitHeight + AuroraTheme.sp8 + AuroraTheme.sp4

            Text {
                id: msgText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: AuroraTheme.sp6
                wrapMode: Text.WordWrap
                text: root.message
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink2
                lineHeight: 1.4
            }
        }
    ]

    footer: [
        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: AuroraTheme.sp4
            spacing: AuroraTheme.sp2
            height: 64

            FsButton {
                id: cancelConfirmBtn
                anchors.verticalCenter: parent.verticalCenter
                text: root.cancelLabel
                variant: "ghost"
                onClicked: { root.cancelled(); root.close(); }
            }
            FsButton {
                id: primaryConfirmBtn
                anchors.verticalCenter: parent.verticalCenter
                text: root.primaryLabel
                variant: root.dangerAction ? "danger" : "primary"
                enabled: root.primaryEnabled
                onClicked: { root.confirmed(); root.close(); }
            }
        }
    ]
}
