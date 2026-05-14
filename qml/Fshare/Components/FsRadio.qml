// SPDX-License-Identifier: Proprietary
// FsRadio — Radio button (Aurora-native, use in group with shared `selectedValue`)
//
// Usage:
//   FsRadio { label: "Option A"; value: "a"; selectedValue: groupSel
//             onSelected: groupSel = value }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Item {
    id: root

    property string label: ""
    property var value: ""
    property var selectedValue: undefined
    property bool enabled: true

    readonly property bool checked: value !== undefined && value === selectedValue

    signal selected(var value)

    implicitWidth: row.implicitWidth
    implicitHeight: 20
    opacity: enabled ? 1.0 : 0.45

    RowLayout {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: AuroraTheme.sp2

        // Outer ring
        Rectangle {
            width: 18; height: 18
            radius: 9
            color: AuroraTheme.panel
            border.width: root.checked ? 2 : 1.5
            border.color: root.checked ? AuroraTheme.accent : AuroraTheme.borderStrong
            Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

            // Inner dot
            Rectangle {
                anchors.centerIn: parent
                width: root.checked ? 8 : 0
                height: width
                radius: width / 2
                color: AuroraTheme.accent
                Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast; easing.type: Easing.OutCubic } }
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
            if (!root.enabled || root.checked) return;
            root.selected(root.value);
        }
    }
}
