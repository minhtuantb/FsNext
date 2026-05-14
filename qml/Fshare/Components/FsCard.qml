// SPDX-License-Identifier: Proprietary
// FsCard — Container with hover lift and accent variant (Aurora-native)

import QtQuick
import FsAurora.Theme 1.0

Rectangle {
    id: root

    property bool accent: false
    property bool hoverable: true

    color: AuroraTheme.panel
    radius: AuroraTheme.radiusLg
    border.width: 1
    border.color: mouse.containsMouse
        ? (accent ? AuroraTheme.accent : AuroraTheme.borderStrong)
        : (accent ? AuroraTheme.accentTint15 : AuroraTheme.border)

    Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

    // Subtle lift on hover
    y: mouse.containsMouse ? -1 : 0
    Behavior on y { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast; easing.type: Easing.OutCubic } }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: root.hoverable
        propagateComposedEvents: true
        onPressed: (event) => event.accepted = false
        onReleased: (event) => event.accepted = false
        onClicked: (event) => event.accepted = false
    }
}
