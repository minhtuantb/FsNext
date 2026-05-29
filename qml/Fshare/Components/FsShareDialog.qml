// SPDX-License-Identifier: Proprietary
// FsShareDialog — Share link dialog with copy + optional password.
//
// Usage:
//   FsShareDialog {
//       id: sd
//       shareUrl: "https://fshare.vn/file/ABC"
//       fileName: "doc.pdf"
//   }
//   sd.open()

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
FsDialog {
    id: root

    property string shareUrl: ""
    property string fileName: ""
    property bool hasPassword: false
    property string password: ""

    signal linkCopied()
    signal passwordChanged(string pwd)

    title: qsTr("Chia sẻ")
    dialogWidth: 480

    function copyLink() {
        urlField.input.selectAll();
        urlField.input.copy();
        root.linkCopied();
    }

    content: [
        Item {
            width: root.dialogWidth
            height: col.implicitHeight + AuroraTheme.sp8 + AuroraTheme.sp4

            ColumnLayout {
                id: col
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                anchors.topMargin: AuroraTheme.sp4
                spacing: AuroraTheme.sp3

                Text {
                    visible: root.fileName.length > 0
                    text: root.fileName
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.bodyStrong.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2

                    FsTextField {
                        id: urlField
                        Layout.fillWidth: true
                        label: qsTr("Liên kết chia sẻ")
                        text: root.shareUrl
                        readOnly: true
                    }
                    FsButton {
                        Layout.alignment: Qt.AlignBottom
                        text: qsTr("Sao chép")
                        variant: "secondary"
                        onClicked: root.copyLink()
                    }
                }

                FsInlineAlert {
                    visible: root.hasPassword
                    Layout.fillWidth: true
                    variant: "info"
                    title: qsTr("File được bảo vệ")
                    message: qsTr("Người nhận sẽ cần mật khẩu để tải file này.")
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
            height: 64

            FsButton {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Đóng"); variant: "ghost"
                onClicked: root.close()
            }
        }
    ]
}
