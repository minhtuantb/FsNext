// SPDX-License-Identifier: Proprietary
// RemoveWatchDialog — Confirm removal of a watched folder

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
import Fshare.Components 1.0

FsDialog {
    id: root
    title: qsTr("Xóa thư mục đồng bộ")
    dialogWidth: 440

    property string watchId: ""
    property string localPath: ""
    property string remotePath: ""
    property int syncedCount: 0

    signal confirmed(string watchId)

    function open() {
        visible = true;
        // Park focus on Cancel so an accidental Enter doesn't destroy the
        // watch.  Mirrors the FileDeleteDialog safety pattern.
        Qt.callLater(() => cancelBtn.forceActiveFocus());
    }

    function close() {
        visible = false;
    }

    content: Item {
        width: 440
        height: confirmCol.implicitHeight + AuroraTheme.sp6 * 2

        ColumnLayout {
            id: confirmCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: AuroraTheme.sp6
            spacing: AuroraTheme.sp4

            Text {
                Layout.fillWidth: true
                text: qsTr("Bạn có chắc muốn ngừng theo dõi thư mục này?")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink1
                wrapMode: Text.WordWrap
            }

            // Folder info
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                FsIcon { name: "folder"; sizePx: 18; color: AuroraTheme.accent }
                Text {
                    Layout.fillWidth: true
                    text: root.localPath + "  \u2192  " + root.remotePath
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                    elide: Text.ElideMiddle
                    wrapMode: Text.NoWrap
                }
            }

            // Reassurance
            ColumnLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp1

                Text {
                    Layout.fillWidth: true
                    visible: root.syncedCount > 0
                    text: qsTr("%1 file đã đồng bộ sẽ không bị ảnh hưởng.").arg(root.syncedCount)
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    color: AuroraTheme.ink2
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("File trên Fshare và trên máy tính đều được giữ nguyên.")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    color: AuroraTheme.ink2
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    footer: Item {
        width: 440
        height: 64

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp3
            anchors.bottomMargin: AuroraTheme.sp3

            Item { Layout.fillWidth: true }

            FsButton {
                id: cancelBtn
                text: qsTr("Hủy")
                variant: "ghost"
                onClicked: root.close()
            }
            FsButton {
                text: qsTr("Xóa")
                variant: "danger"
                onClicked: {
                    root.confirmed(root.watchId);
                    root.close();
                }
            }
        }
    }
}
