// SPDX-License-Identifier: Proprietary
// FsIcon — SVG icon renderer with runtime color tinting (Aurora-native).
//
// Icons live at qml/Fshare/Icons/<name>.svg as monochrome SVGs
// (stroke="#000" / fill="#000", 24x24 viewBox). Color tinting is applied
// at render time via QtQuick.Effects MultiEffect so any AuroraTheme color
// can be passed in without editing the SVG.
//
// Usage:
//   FsIcon { name: "arrow-down"; sizePx: 20; color: AuroraTheme.ink2 }
//
// To add a new icon:
//   1. Drop <name>.svg into qml/Fshare/Icons/ (24x24 viewBox, stroke="#000")
//   2. Rebuild — CMake globs qml/**/*.svg into qml_resources.

import QtQuick
import QtQuick.Effects
import FsAurora.Theme 1.0

Item {
    id: root

    property string name: ""
    property int sizePx: 20
    property color color: AuroraTheme.ink2
    // When rendered alone (without a parent FsButton), set `accessibleName` so
    // screen readers don't read out the icon's filename.  Empty by default —
    // most callers wrap the icon in a button that owns the a11y label.
    property string accessibleName: ""

    implicitWidth: sizePx
    implicitHeight: sizePx
    // Apply alpha at the Item level so MultiEffect receives a fully-opaque
    // colorizationColor — see comment below.
    opacity: root.color.a
    Accessible.role: accessibleName.length > 0 ? Accessible.Graphic : Accessible.NoRole
    Accessible.name: accessibleName
    Accessible.ignored: accessibleName.length === 0   // skip when purely decorative

    Image {
        id: iconSource
        anchors.fill: parent
        source: root.name.length > 0
            ? "qrc:/qml/Fshare/Icons/" + root.name + ".svg"
            : ""
        // Render at 2x for crisp scaling / HiDPI without blurring
        sourceSize.width: root.sizePx * 2
        sourceSize.height: root.sizePx * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        cache: true
        asynchronous: false

        // Source SVGs ship with hard-coded black fill/stroke. Qt 6.8's
        // MultiEffect.colorization does mix(source.rgb, colorizationColor.rgb,
        // colorization * colorizationColor.a) — black source + colour with
        // alpha < 1 produces a dark/muddy mix instead of the intended tint.
        // brightness=1.0 lifts every source pixel to white first, then a
        // colorization with a fully-opaque target colour replaces it
        // cleanly. The original alpha is re-applied via Item.opacity above.
        layer.enabled: true
        layer.effect: MultiEffect {
            brightness: 1.0
            colorization: 1.0
            colorizationColor: Qt.rgba(root.color.r, root.color.g, root.color.b, 1.0)
        }
    }
}
