// SPDX-License-Identifier: Proprietary
// FsLoadingState — Spinning loading indicator with optional message

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    property string message: ""

    ColumnLayout {
        anchors.centerIn: parent
        spacing: AuroraTheme.sp4

        // Spinner: thin red ring with rotating gap
        Rectangle {
            id: ring
            Layout.alignment: Qt.AlignHCenter
            width: 32; height: 32; radius: 16
            color: "transparent"
            border.width: 3
            border.color: AuroraTheme.accentTint15

            Rectangle {
                width: 8; height: 3
                color: AuroraTheme.accent
                radius: 1.5
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: -1.5
            }

            RotationAnimation on rotation {
                from: 0; to: 360
                duration: 900
                loops: Animation.Infinite
                running: root.visible
            }
        }

        Text {
            visible: root.message.length > 0
            Layout.alignment: Qt.AlignHCenter
            text: root.message
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.body.pixelSize
            color: AuroraTheme.ink2
        }
    }
}
