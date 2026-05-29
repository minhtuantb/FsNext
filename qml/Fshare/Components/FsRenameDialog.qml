// SPDX-License-Identifier: Proprietary
// FsRenameDialog — Single-input rename dialog.
//
// Usage:
//   FsRenameDialog {
//       id: rn
//       onAccepted: name => applyRename(name)
//   }
//   rn.currentName = "old.txt"; rn.open()

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
FsDialog {
    id: root

    property string currentName: ""
    property string label: qsTr("Tên mới")
    property string placeholder: qsTr("Nhập tên…")

    signal accepted(string newName)

    title: qsTr("Đổi tên")
    dialogWidth: 440

    onOpened: {
        input.text = root.currentName;
        input.input.selectAll();
        input.input.forceActiveFocus();
    }

    function applyRename() {
        const val = input.text.trim();
        if (val.length === 0 || val === root.currentName) { root.close(); return; }
        root.accepted(val);
        root.close();
    }

    content: [
        Item {
            width: root.dialogWidth
            height: input.implicitHeight + AuroraTheme.sp8 + AuroraTheme.sp4

            FsTextField {
                id: input
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: AuroraTheme.sp6
                label: root.label
                placeholder: root.placeholder
                onAccepted: root.applyRename()
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
                text: qsTr("Hủy"); variant: "ghost"
                onClicked: root.close()
            }
            FsButton {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Đổi tên"); variant: "primary"
                onClicked: root.applyRename()
            }
        }
    ]
}
