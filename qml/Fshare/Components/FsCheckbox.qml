// SPDX-License-Identifier: Proprietary
// FsCheckbox — Square checkbox (Aurora-native, accent fill when checked)
//
// Usage:
//   FsCheckbox { label: "Remember me"; checked: true; onToggled: { ... } }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Item {
    id: root

    property string label: ""
    property bool checked: false
    property bool enabled: true
    signal toggled(bool checked)

    implicitWidth: row.implicitWidth
    implicitHeight: 20
    opacity: enabled ? 1.0 : 0.45

    RowLayout {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: AuroraTheme.sp2

        // Box
        Rectangle {
            id: box
            width: 18; height: 18
            radius: AuroraTheme.radiusSm - 2  // tighter than input — visual weight
            color: root.checked ? AuroraTheme.accent : AuroraTheme.panel
            border.width: 1.5
            border.color: root.checked ? AuroraTheme.accent : AuroraTheme.borderStrong
            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
            Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

            // Checkmark
            Text {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -1
                text: "✓"
                font.pixelSize: 13
                font.bold: true
                color: AuroraTheme.textOnAccent
                opacity: root.checked ? 1.0 : 0.0
                Behavior on opacity { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }
            }
        }

        Text {
            visible: root.label.length > 0
            text: root.label
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.body.pixelSize
            color: AuroraTheme.ink1
        }
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
