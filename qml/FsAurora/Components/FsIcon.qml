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

        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: root.color
        }
    }
}
