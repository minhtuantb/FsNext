// SPDX-License-Identifier: Proprietary
// FsCard (Aurora) — panel surface with subtle border and optional accent edge.

import QtQuick
import FsAurora.Theme

Rectangle {
    id: root

    property bool accent: false
    property bool hoverable: false

    color: AuroraTheme.panel
    radius: AuroraTheme.radiusLg
    border.width: 1
    border.color: mouse.containsMouse && hoverable
        ? (accent ? AuroraTheme.accent : AuroraTheme.borderStrong)
        : (accent ? AuroraTheme.accent : AuroraTheme.border)

    Behavior on border.color { enabled: !AuroraTheme.reduceMotion
        ColorAnimation { duration: AuroraTheme.durFast } }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: root.hoverable
        acceptedButtons: Qt.NoButton
        propagateComposedEvents: true
    }
}
