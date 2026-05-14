// SPDX-License-Identifier: Proprietary
// FsBreadcrumb — Path-style navigation trail with clickable segments.
//
// Usage:
//   FsBreadcrumb {
//       segments: [
//           { label: "Home", linkcode: "0" },
//           { label: "Docs", linkcode: "abc123" }
//       ]
//       onSegmentClicked: (index, segment) => navigateTo(segment.linkcode)
//   }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    property var segments: []

    signal segmentClicked(int index, var segment)

    implicitWidth: row.implicitWidth
    implicitHeight: 28

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: root.segments
            delegate: Row {
                required property var modelData
                required property int index
                readonly property bool isLast: index === root.segments.length - 1
                spacing: 2

                Rectangle {
                    height: 26
                    width: segLabel.implicitWidth + AuroraTheme.sp3
                    radius: AuroraTheme.radiusSm
                    color: segMa.containsMouse && !parent.isLast ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        id: segLabel
                        anchors.centerIn: parent
                        text: (typeof modelData === "string") ? modelData : (modelData.label || "")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.body.pixelSize
                        font.weight: parent.parent.isLast ? Font.DemiBold : Font.Normal
                        color: parent.parent.isLast ? AuroraTheme.ink1 : AuroraTheme.ink2
                        elide: Text.ElideMiddle
                    }

                    MouseArea {
                        id: segMa
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: !parent.parent.isLast
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.segmentClicked(parent.parent.index, modelData)
                    }
                }

                FsIcon {
                    visible: !parent.isLast
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron-right"
                    sizePx: 12
                    color: AuroraTheme.ink3
                }
            }
        }
    }
}
