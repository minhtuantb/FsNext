// SPDX-License-Identifier: Proprietary
// FsIcon — SVG icon renderer with runtime color tinting.
//
// Reuses the same SVG files under qml/Fshare/Icons/; this variant just
// defaults to Aurora ink colors. MultiEffect (Qt 6.5+) handles the recolor
// without forking the SVGs.

import QtQuick
import QtQuick.Effects
import FsAurora.Theme

Item {
    id: root

    property string name: ""
    property int sizePx: 20
    property color color: AuroraTheme.ink2

    implicitWidth: sizePx
    implicitHeight: sizePx
    // Apply the colour's alpha at the Item level so we can pass a fully-
    // opaque colour into MultiEffect (see comment below).
    opacity: root.color.a

    Image {
        id: src
        anchors.fill: parent
        source: root.name.length > 0
            ? "qrc:/qml/Fshare/Icons/" + root.name + ".svg"
            : ""
        sourceSize.width: root.sizePx * 2
        sourceSize.height: root.sizePx * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        cache: true
        asynchronous: false

        // Source SVGs ship with hard-coded black fill/stroke. Qt 6.8's
        // MultiEffect.colorization does `mix(source.rgb, colorizationColor.rgb,
        // colorization * colorizationColor.a)` — when the source pixel is pure
        // black AND the colour the caller passes carries alpha < 1.0, the mix
        // factor drops below 1.0 and the result skews dark / muddy. The user
        // hit exactly this: sidebar icons rendered ≈ black instead of bright
        // grey on the lifted indigo bg.
        //
        // Fix: brightness=1.0 first lifts every source pixel to white, then
        // colorize at 1.0 with the *fully-opaque* target colour replaces it
        // cleanly. The original alpha is re-applied via Item.opacity above.
        layer.enabled: true
        layer.effect: MultiEffect {
            brightness: 1.0
            colorization: 1.0
            colorizationColor: Qt.rgba(root.color.r, root.color.g, root.color.b, 1.0)
        }
    }
}
