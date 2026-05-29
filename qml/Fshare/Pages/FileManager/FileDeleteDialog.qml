// SPDX-License-Identifier: Proprietary
// FileDeleteDialog — extracted from FileManagerPage.qml under ADR 003 D13.
//
// Inputs (set by parent before calling open()):
//   pendingLinkcodes — array of linkcodes to delete
//   hasFolder        — true when the selection includes any folder; toggles
//                      a warning banner about cascading deletion
//
// Output:
//   Emits confirmed(linkcodes) when the user clicks the Delete button.
//   Parent is responsible for the actual API call (fileManagerViewModel.deleteFiles)
//   and for clearing selection state on its side.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0

FsDialog {
    id: root

    property var  pendingLinkcodes: []
    property bool hasFolder: false

    signal confirmed(var linkcodes)

    title: qsTr("Xác nhận xoá")
    dialogWidth: 420

    // Safety: park keyboard focus on Cancel when the dialog opens.  Without
    // this an over-eager Enter (e.g. user just confirmed a previous dialog)
    // would land on the danger button and silently nuke files.  Qt.callLater
    // defers until after the dialog's own focus grab in FsDialog.open(), so
    // we don't fight the trap setup.
    onOpened: Qt.callLater(() => cancelBtn.forceActiveFocus())

    content: Item {
        width: 420
        height: col.implicitHeight + AuroraTheme.sp6 * 2

        ColumnLayout {
            id: col
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp6
            spacing: AuroraTheme.sp3

            Text {
                Layout.fillWidth: true
                text: root.pendingLinkcodes.length === 1
                    ? qsTr("Bạn có chắc muốn xoá mục này?")
                    : qsTr("Bạn có chắc muốn xoá %1 mục?").arg(root.pendingLinkcodes.length)
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                color: AuroraTheme.ink1
                wrapMode: Text.WordWrap
            }

            // Cascading-delete warning — only shown when a folder is in scope,
            // because a "delete file" is intuitive but a "delete folder
            // including children" needs an explicit nudge before the click.
            Rectangle {
                Layout.fillWidth: true
                visible: root.hasFolder
                implicitHeight: warnRow.implicitHeight + AuroraTheme.sp2 * 2
                radius: AuroraTheme.radiusSm
                color: AuroraTheme.warnSoft
                border.width: 1; border.color: AuroraTheme.warn

                RowLayout {
                    id: warnRow
                    anchors.fill: parent; anchors.margins: AuroraTheme.sp2
                    spacing: AuroraTheme.sp2
                    Text { text: "⚠"; font.pixelSize: 16 }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Thư mục sẽ bị xoá cùng toàn bộ nội dung bên trong.")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        color: AuroraTheme.ink1
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Hành động này không thể hoàn tác.")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                color: AuroraTheme.ink3
            }
        }
    }

    footer: Item {
        width: 420
        height: 64
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            spacing: AuroraTheme.sp2

            Item { Layout.fillWidth: true }

            FsButton {
                id: cancelBtn
                text: qsTr("Huỷ")
                variant: "ghost"
                onClicked: root.close()
            }
            FsButton {
                text: qsTr("Xoá")
                variant: "danger"
                onClicked: {
                    root.confirmed(root.pendingLinkcodes);
                    root.close();
                }
            }
        }
    }
}
