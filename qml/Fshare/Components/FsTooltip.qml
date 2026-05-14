// SPDX-License-Identifier: Proprietary
// FsTooltip — Hover tooltip anchored to a target.
//
// Usage:
//   Button {
//       id: myBtn
//       text: "Hi"
//       FsTooltip { text: qsTr("Details…"); target: myBtn }
//   }

import QtQuick
import FsAurora.Theme 1.0
Item {
    id: root

    property string text: ""
    property Item target: parent
    property int delay: 500
    property string position: "top"  // top | bottom

    visible: false
    z: 10000

    HoverHandler {
        id: hov
        target: root.target
    }

    Timer {
        id: showTimer
        interval: root.delay
        onTriggered: if (hov.hovered && root.text.length > 0) root.visible = true
    }

    Connections {
        target: hov
        function onHoveredChanged() {
            if (hov.hovered) showTimer.restart();
            else { showTimer.stop(); root.visible = false; }
        }
    }

    parent: root.target ? (root.target.Window.contentItem ?? root.target) : null

    Rectangle {
        id: box
        radius: AuroraTheme.radiusSm
        color: AuroraTheme.ink1
        opacity: root.visible ? 0.92 : 0
        Behavior on opacity { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }

        width: label.implicitWidth + AuroraTheme.sp4
        height: label.implicitHeight + AuroraTheme.sp2

        x: {
            if (!root.target) return 0;
            const pt = root.target.mapToItem(root.parent, root.target.width / 2, 0);
            return pt.x - width / 2;
        }
        y: {
            if (!root.target) return 0;
            if (root.position === "bottom") {
                const pt = root.target.mapToItem(root.parent, 0, root.target.height + 6);
                return pt.y;
            }
            const pt = root.target.mapToItem(root.parent, 0, 0);
            return pt.y - height - 6;
        }

        Text {
            id: label
            anchors.centerIn: parent
            text: root.text
            color: AuroraTheme.panel
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
        }
    }
}
