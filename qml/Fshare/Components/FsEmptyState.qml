// SPDX-License-Identifier: Proprietary
// FsEmptyState — Empty state placeholder

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    property string icon: "○"
    property string title: "Nothing here yet"
    property string description: ""
    property string actionText: ""

    signal actionClicked()

    ColumnLayout {
        anchors.centerIn: parent
        spacing: AuroraTheme.sp4

        // Soft circle with icon
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 80; height: 80
            radius: 40
            color: AuroraTheme.accentSoft
            Text {
                anchors.centerIn: parent
                text: root.icon
                font.pixelSize: 40
                color: AuroraTheme.accent
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.title
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.h3.pixelSize
            font.weight: Font.DemiBold
            color: AuroraTheme.ink1
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: root.description.length > 0
            text: root.description
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.body.pixelSize
            color: AuroraTheme.ink2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.maximumWidth: 320
        }

        Item {
            Layout.preferredHeight: AuroraTheme.sp1
            visible: root.actionText.length > 0
        }

        // Inline button (avoid component circular dep)
        Rectangle {
            visible: root.actionText.length > 0
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: actionLabel.implicitWidth + AuroraTheme.sp6 * 2
            implicitHeight: 36
            radius: AuroraTheme.radiusPill
            color: actionMa.pressed ? AuroraTheme.accentPressed
                : (actionMa.containsMouse ? AuroraTheme.accentHover : AuroraTheme.accent)
            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

            Text {
                id: actionLabel
                anchors.centerIn: parent
                text: root.actionText
                color: AuroraTheme.textOnAccent
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                font.weight: Font.DemiBold
            }

            MouseArea {
                id: actionMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.actionClicked()
            }
        }
    }
}
