// SPDX-License-Identifier: Proprietary
// FsSpinner — Indeterminate circular loading spinner (Aurora-native).
//
// Usage:
//   FsSpinner { sizePx: 20; color: AuroraTheme.accent }

import QtQuick
import FsAurora.Theme 1.0

Item {
    id: root

    property int sizePx: 20
    property color color: AuroraTheme.accent
    property int thickness: 2
    property bool running: true

    implicitWidth: sizePx
    implicitHeight: sizePx

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        property real angle: 0

        onAngleChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const cx = width / 2;
            const cy = height / 2;
            const r  = Math.min(width, height) / 2 - root.thickness;
            ctx.lineWidth = root.thickness;
            ctx.lineCap = "round";

            // Track
            ctx.beginPath();
            ctx.strokeStyle = Qt.rgba(root.color.r, root.color.g, root.color.b, 0.15);
            ctx.arc(cx, cy, r, 0, Math.PI * 2);
            ctx.stroke();

            // Arc
            ctx.beginPath();
            ctx.strokeStyle = root.color;
            const start = angle;
            ctx.arc(cx, cy, r, start, start + Math.PI * 1.2);
            ctx.stroke();
        }
    }

    NumberAnimation on rotation {
        from: 0; to: 360
        duration: 900
        loops: Animation.Infinite
        running: root.running && root.visible && !AuroraTheme.reduceMotion
    }
}
