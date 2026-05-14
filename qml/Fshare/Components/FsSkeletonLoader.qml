// SPDX-License-Identifier: Proprietary
// FsSkeletonLoader — Shimmering placeholder block for loading states.
//
// Usage:
//   FsSkeletonLoader { width: 200; height: 16; radius: 4 }

import QtQuick
import FsAurora.Theme 1.0
Rectangle {
    id: root

    implicitWidth: 120
    implicitHeight: 14

    radius: AuroraTheme.radiusSm
    color: AuroraTheme.divider
    clip: true

    Rectangle {
        id: shimmer
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height * 2
        width: parent.width * 0.4
        rotation: 15
        opacity: 0.55
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.5; color: AuroraTheme.borderStrong }
            GradientStop { position: 1.0; color: "transparent" }
        }

        NumberAnimation on x {
            from: -root.width * 0.5
            to: root.width
            duration: 1200
            loops: Animation.Infinite
            running: root.visible && !AuroraTheme.reduceMotion
        }
    }
}
