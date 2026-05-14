// SPDX-License-Identifier: Proprietary
// FsSwitch (Aurora) — track + thumb toggle. On = brand gradient.

import QtQuick
import FsAurora.Theme

Item {
    id: root

    property bool checked: false
    property bool enabled: true
    property string label: ""

    signal toggled(bool checked)

    implicitWidth: track.width + (root.label !== "" ? labelText.width + AuroraTheme.sp2 : 0)
    implicitHeight: 22
    opacity: enabled ? 1.0 : 0.5

    Rectangle {
        id: track
        width: 38; height: 22; radius: 11
        color: root.checked ? "transparent" : (AuroraTheme.isDark ? "#3A3A45" : "#C8C3B8")
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        FsGradientRect {
            anchors.fill: parent
            radius: 11
            visible: root.checked
        }

        Rectangle {
            id: thumb
            width: 18; height: 18; radius: 9
            color: "#FFFFFF"
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - 2 : 2
            Behavior on x { enabled: !AuroraTheme.reduceMotion
                NumberAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }
        }
    }

    Text {
        id: labelText
        visible: root.label !== ""
        anchors.left: track.right
        anchors.leftMargin: AuroraTheme.sp2
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        font.family: AuroraTheme.fontSans
        font.pixelSize: 13
        color: AuroraTheme.ink1
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (!root.enabled) return;
            root.checked = !root.checked;
            root.toggled(root.checked);
        }
    }
}
