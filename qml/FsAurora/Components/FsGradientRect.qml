// SPDX-License-Identifier: Proprietary
// FsGradientRect — rounded-rectangle fill with a diagonal gradient.
//
// Qt Quick's built-in Rectangle.gradient is vertical-only, so the Aurora
// 135° brand gradient is built on QtQuick.Shapes. Exposed API mirrors
// Rectangle where it makes sense:
//
//   FsGradientRect {
//       width: 120; height: 40
//       radius: AuroraTheme.radiusMd
//   }
//
// The gradient itself is the Aurora brand gradient, hardcoded here as the
// 3 canonical stops (orange → pink → yellow) from design-tokens.json.

import QtQuick
import QtQuick.Shapes
import FsAurora.Theme

Item {
    id: root

    // Radius of the rounded corners. Set to a very large value for a pill.
    property real radius: 10
    // Gradient direction in degrees. 135° matches the Aurora token.
    property real angle: 135
    // Optional 1px border stroke. `borderWidth: 0` hides the stroke.
    property color borderColor: "transparent"
    property real  borderWidth: 0
    // Override colors if needed; default is the Aurora brand gradient.
    property color stop0: "#FF5B2E"
    property real  stop0Pos: 0.0
    property color stop1: "#FF3D7F"
    property real  stop1Pos: 0.6
    property color stop2: "#FFAF1D"
    property real  stop2Pos: 1.0

    implicitWidth: 100
    implicitHeight: 32

    Shape {
        id: shape
        anchors.fill: parent
        antialiasing: true
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            id: spath

            property real _r: Math.min(root.radius, Math.min(shape.width, shape.height) / 2)

            strokeColor: root.borderColor
            strokeWidth: root.borderWidth
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap

            fillGradient: LinearGradient {
                // Map angle → endpoints across the bounding box.
                property real _rad: root.angle * Math.PI / 180
                property real _cx: shape.width / 2
                property real _cy: shape.height / 2
                property real _dx: Math.cos(_rad) * Math.max(shape.width, shape.height)
                property real _dy: Math.sin(_rad) * Math.max(shape.width, shape.height)

                x1: _cx - _dx / 2
                y1: _cy - _dy / 2
                x2: _cx + _dx / 2
                y2: _cy + _dy / 2

                GradientStop { position: root.stop0Pos; color: root.stop0 }
                GradientStop { position: root.stop1Pos; color: root.stop1 }
                GradientStop { position: root.stop2Pos; color: root.stop2 }
            }

            startX: spath._r; startY: 0
            PathLine { x: shape.width - spath._r; y: 0 }
            PathArc  { x: shape.width; y: spath._r; radiusX: spath._r; radiusY: spath._r }
            PathLine { x: shape.width; y: shape.height - spath._r }
            PathArc  { x: shape.width - spath._r; y: shape.height; radiusX: spath._r; radiusY: spath._r }
            PathLine { x: spath._r; y: shape.height }
            PathArc  { x: 0; y: shape.height - spath._r; radiusX: spath._r; radiusY: spath._r }
            PathLine { x: 0; y: spath._r }
            PathArc  { x: spath._r; y: 0; radiusX: spath._r; radiusY: spath._r }
        }
    }
}
