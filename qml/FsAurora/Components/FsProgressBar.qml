// SPDX-License-Identifier: Proprietary
// FsProgressBar (Aurora) — gradient-filled track.

import QtQuick
import FsAurora.Theme

Item {
    id: root

    property real value: 0.0   // 0..1
    property bool indeterminate: false
    property int trackHeight: 6

    implicitHeight: trackHeight
    implicitWidth: 200

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: AuroraTheme.isDark
            ? Qt.rgba(1, 1, 1, 0.06)
            : Qt.rgba(0, 0, 0, 0.06)
    }

    FsGradientRect {
        id: fill
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        radius: height / 2
        visible: !root.indeterminate
        width: Math.max(height, parent.width * Math.max(0, Math.min(1, root.value)))
        Behavior on width { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durBase; easing.type: AuroraTheme.easingStd } }
    }

    // Indeterminate — bounces a fixed-width gradient pill across the track
    FsGradientRect {
        id: ind
        height: parent.height
        radius: height / 2
        width: parent.width * 0.35
        visible: root.indeterminate
        x: 0
        SequentialAnimation on x {
            running: root.indeterminate
            loops: Animation.Infinite
            NumberAnimation { from: -ind.width; to: root.width; duration: 1200; easing.type: Easing.InOutQuad }
        }
    }
}
