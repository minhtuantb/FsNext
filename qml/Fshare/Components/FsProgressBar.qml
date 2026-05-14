// SPDX-License-Identifier: Proprietary
// FsProgressBar — Semantic colored progress bar
//
// Usage:
//   FsProgressBar { value: 0.68; status: "downloading" }
//   FsProgressBar { value: 1.0; status: "complete" }

import QtQuick
import FsAurora.Theme 1.0
Item {
    id: root

    property real value: 0.0       // 0.0 – 1.0
    property string status: "default"  // default | downloading | uploading | complete | warning | error
    property int barHeight: 4

    implicitWidth: 200
    implicitHeight: barHeight

    property color _fillColor: {
        switch (status) {
        case "downloading": return AuroraTheme.accent
        case "uploading":   return AuroraTheme.accent
        case "complete":    return AuroraTheme.success
        case "warning":     return AuroraTheme.warn
        case "error":       return Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.4)
        default:            return AuroraTheme.accent
        }
    }

    // Track
    Rectangle {
        anchors.fill: parent
        radius: AuroraTheme.radiusPill
        color: AuroraTheme.borderStrong
    }

    // Fill
    Rectangle {
        height: parent.height
        width: parent.width * Math.max(0, Math.min(1, root.value))
        radius: AuroraTheme.radiusPill
        color: root._fillColor

        Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
    }
}
