// SPDX-License-Identifier: Proprietary
// FsBadge — Status badge/chip (Aurora-native)
//
// Usage:
//   FsBadge { text: "VIP"; variant: "solid-accent" }
//   FsBadge { text: "Hoàn tất"; variant: "success"; dot: true }
//
// Variant aliases:
//   red          → accent       (orange Aurora — kept for back-compat)
//   solid-red    → solid-accent
//   green        → success
//   amber        → warn
//   blue         → info

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Rectangle {
    id: root

    property string text: ""
    property string variant: "neutral"
    property bool dot: false

    implicitWidth: row.implicitWidth + AuroraTheme.sp3 * 2
    implicitHeight: 22
    radius: AuroraTheme.radiusPill

    color: {
        switch (variant) {
        case "red":            // alias
        case "accent":         return AuroraTheme.accentTint10
        case "green":          // alias
        case "success":        return AuroraTheme.successSoft
        case "amber":          // alias
        case "warn":           return AuroraTheme.warnSoft
        case "blue":           // alias
        case "info":           return AuroraTheme.infoSoft
        case "solid-red":      // alias
        case "solid-accent":   return AuroraTheme.accent
        case "solid-amber":    // alias
        case "solid-warn":     return AuroraTheme.warn
        default:               return AuroraTheme.divider
        }
    }

    border.width: (variant === "solid-red" || variant === "solid-amber"
                    || variant === "solid-accent" || variant === "solid-warn") ? 0 : 1
    border.color: {
        switch (variant) {
        case "red":
        case "accent":   return AuroraTheme.accentTint15
        case "green":
        case "success":  return Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25)
        case "amber":
        case "warn":     return Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25)
        case "blue":
        case "info":     return Qt.rgba(AuroraTheme.info.r, AuroraTheme.info.g, AuroraTheme.info.b, 0.20)
        default:         return AuroraTheme.border
        }
    }

    property color _textColor: {
        switch (variant) {
        case "red":
        case "accent":         return AuroraTheme.accent
        case "green":
        case "success":        return AuroraTheme.success
        case "amber":
        case "warn":           return AuroraTheme.warn
        case "blue":
        case "info":           return AuroraTheme.info
        case "solid-red":
        case "solid-accent":   return AuroraTheme.textOnAccent
        case "solid-amber":
        case "solid-warn":     return AuroraTheme.textOnAccent
        default:               return AuroraTheme.ink2
        }
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: AuroraTheme.sp1

        Rectangle {
            visible: root.dot
            width: 6; height: 6
            radius: 3
            color: root._textColor
        }

        Text {
            text: root.text
            font.family: AuroraTheme.fontSans
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: root._textColor
        }
    }
}
