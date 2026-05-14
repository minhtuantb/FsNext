// SPDX-License-Identifier: Proprietary
// FsActivityRow — Single row in the auto-upload activity feed

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
import Fshare.Components 1.0

Rectangle {
    id: root

    property string fileName: ""
    property string remotePath: ""
    property string fileSize: ""
    property string status: "completed"  // completed | uploading | error | skipped
    property string timeAgo: ""
    property string errorMessage: ""
    property real progress: 0.0

    implicitHeight: AuroraTheme.heightListRow  // 36px
    color: rowMa.containsMouse ? AuroraTheme.panel : "transparent"
    radius: AuroraTheme.radiusSm

    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

    MouseArea {
        id: rowMa
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: AuroraTheme.sp2
        anchors.rightMargin: AuroraTheme.sp2
        spacing: 10

        // Status icon
        Text {
            Layout.preferredWidth: 16
            horizontalAlignment: Text.AlignHCenter
            text: {
                switch (root.status) {
                case "completed": return "\u2713";
                case "uploading": return "\u2191";
                case "error":     return "\u26A0";
                case "skipped":   return "\u2192";
                default:          return "\u2022";
                }
            }
            font.family: AuroraTheme.fontSans
            font.pixelSize: 12
            font.weight: Font.Bold
            color: {
                switch (root.status) {
                case "completed": return AuroraTheme.success;
                case "uploading": return AuroraTheme.accent;
                case "error":     return AuroraTheme.warn;
                case "skipped":   return AuroraTheme.ink3;
                default:          return AuroraTheme.ink3;
                }
            }
        }

        // File name
        Text {
            Layout.fillWidth: true
            text: root.fileName
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.body.pixelSize
            color: AuroraTheme.ink1
            elide: Text.ElideMiddle
        }

        // Destination / status text
        Text {
            Layout.preferredWidth: 180
            text: {
                if (root.status === "error") return root.errorMessage;
                if (root.status === "uploading")
                    return qsTr("Đang tải\u2026 %1%").arg(Math.round(root.progress * 100));
                return root.remotePath;
            }
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
            color: {
                if (root.status === "error") return AuroraTheme.warn;
                if (root.status === "uploading") return AuroraTheme.accent;
                return AuroraTheme.ink2;
            }
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
        }

        // File size
        Text {
            Layout.preferredWidth: 60
            text: root.fileSize
            font.family: AuroraTheme.fontMono
            font.pixelSize: AuroraTheme.caption.pixelSize
            color: AuroraTheme.ink3
            horizontalAlignment: Text.AlignRight
        }

        // Time ago
        Text {
            Layout.preferredWidth: 70
            text: root.timeAgo
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
            color: AuroraTheme.ink3
            horizontalAlignment: Text.AlignRight
        }
    }
}
