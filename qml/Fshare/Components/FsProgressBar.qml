// SPDX-License-Identifier: Proprietary
// FsProgressBar — progress track (design-system canonical, Fshare.Components).
//
// Merged from the two former implementations (design-system Stage 2, 2026-05-29):
//   • status == "default"  → 135° brand gradient fill (former Aurora bar), plus
//     an optional indeterminate bouncing pill.
//   • status != "default"  → semantic solid color (former Fshare bar):
//     downloading | uploading | complete | warning | error.
//
// Usage:
//   FsProgressBar { value: 0.6 }                       // gradient
//   FsProgressBar { value: 0.6; status: "uploading" }  // semantic solid
//   FsProgressBar { indeterminate: true }              // bouncing gradient

import QtQuick
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora   // FsGradientRect

Item {
    id: root

    property real value: 0.0                 // 0..1
    property string status: "default"        // default | downloading | uploading | complete | warning | error
    property bool indeterminate: false
    property int barHeight: 4                // canonical default
    property int trackHeight: -1             // back-compat alias; >=0 overrides barHeight

    readonly property int _h: trackHeight >= 0 ? trackHeight : barHeight
    readonly property bool _gradient: status === "default"

    implicitWidth: 200
    implicitHeight: _h

    property color _fillColor: {
        switch (status) {
        case "downloading": return AuroraTheme.accent
        case "uploading":   return AuroraTheme.accent
        case "complete":    return AuroraTheme.success
        case "warning":     return AuroraTheme.warn
        case "error":       return AuroraTheme.danger
        default:            return AuroraTheme.accent
        }
    }

    // Track
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: AuroraTheme.borderStrong
    }

    // Solid fill (semantic status)
    Rectangle {
        visible: !root._gradient && !root.indeterminate
        height: parent.height
        width: parent.width * Math.max(0, Math.min(1, root.value))
        radius: height / 2
        color: root._fillColor
        Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
    }

    // Gradient fill (default look)
    Aurora.FsGradientRect {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        radius: height / 2
        visible: root._gradient && !root.indeterminate
        width: Math.max(height, parent.width * Math.max(0, Math.min(1, root.value)))
        Behavior on width { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durBase; easing.type: AuroraTheme.easingStd } }
    }

    // Indeterminate — bounce a fixed-width gradient pill across the track
    Aurora.FsGradientRect {
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
