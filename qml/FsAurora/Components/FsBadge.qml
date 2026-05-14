// SPDX-License-Identifier: Proprietary
// FsBadge (Aurora) — pill text badge with semantic variants.

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme

Rectangle {
    id: root

    property string text: ""
    property string variant: "neutral"  // neutral | accent | success | warn | danger | info
    property bool dot: false             // show leading dot

    radius: AuroraTheme.radiusPill
    implicitHeight: 22
    implicitWidth: row.implicitWidth + AuroraTheme.sp3 * 2

    color: _bg
    border.width: 1
    border.color: _border

    readonly property color _bg: {
        switch (variant) {
        case "accent":  return AuroraTheme.accentSoft
        case "success": return AuroraTheme.successSoft
        case "warn":    return AuroraTheme.warnSoft
        case "danger":  return AuroraTheme.dangerSoft
        case "info":    return AuroraTheme.infoSoft
        default:        return Qt.rgba(0, 0, 0, 0.04)
        }
    }
    readonly property color _fg: {
        switch (variant) {
        case "accent":  return AuroraTheme.accent
        case "success": return AuroraTheme.success
        case "warn":    return AuroraTheme.warn
        case "danger":  return AuroraTheme.danger
        case "info":    return AuroraTheme.info
        default:        return AuroraTheme.ink2
        }
    }
    readonly property color _border: Qt.rgba(_fg.r, _fg.g, _fg.b, 0.25)

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: AuroraTheme.sp1

        Rectangle {
            visible: root.dot
            width: 6; height: 6; radius: 3
            color: root._fg
        }
        Text {
            text: root.text
            font.family: AuroraTheme.fontSans
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: root._fg
        }
    }
}
