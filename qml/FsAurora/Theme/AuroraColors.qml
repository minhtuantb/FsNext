// SPDX-License-Identifier: Proprietary
// FsAurora · color tokens — mirrors qml/FsAurora/handoff/design-tokens.json.
// Kept parallel to Fshare.Theme.FshareColors so the two systems can coexist
// at runtime; switched via the top-level UI variant toggle.

pragma Singleton
import QtQuick

QtObject {
    id: colors

    // ── Brand ─────────────────────────────────────────────
    // Core accent trio + gradient stops. Gradient is rendered by
    // FsGradientRect (Shape + ShapePath fillGradient) since Rectangle's
    // built-in gradient is vertical-only.
    readonly property color accent:        "#FF5B2E"  // orange
    readonly property color accent2:       "#FFAF1D"  // yellow
    readonly property color accent3:       "#FF3D7F"  // pink
    readonly property color accentHover:   "#FF7550"
    readonly property color accentPressed: "#E54A20"

    readonly property var gradientStops: [
        { position: 0.00, color: "#FF5B2E" },
        { position: 0.60, color: "#FF3D7F" },
        { position: 1.00, color: "#FFAF1D" }
    ]

    readonly property color accentGlow:    Qt.rgba(1.0, 0.357, 0.180, 0.22)   // focus ring
    readonly property color accentShadow:  Qt.rgba(1.0, 0.357, 0.180, 0.30)   // shadow-md
    readonly property color accentSoft:    Qt.rgba(1.0, 0.357, 0.180, 0.08)   // tinted bg
    readonly property color accentTint10:  Qt.rgba(1.0, 0.357, 0.180, 0.10)
    readonly property color accentTint15:  Qt.rgba(1.0, 0.357, 0.180, 0.15)

    // ── Semantic ─────────────────────────────────────────
    readonly property color success:       "#0A8A5C"
    readonly property color successSoft:   Qt.rgba(0.039, 0.541, 0.361, 0.10)
    readonly property color warn:          "#C96A00"
    readonly property color warnSoft:      Qt.rgba(0.788, 0.416, 0.000, 0.10)
    readonly property color danger:        "#D53030"
    readonly property color dangerSoft:    Qt.rgba(0.835, 0.188, 0.188, 0.10)
    readonly property color info:          "#2566E5"
    readonly property color infoSoft:      Qt.rgba(0.145, 0.400, 0.898, 0.10)

    // ── Light palette ────────────────────────────────────
    readonly property var light: QtObject {
        readonly property color bg:            "#F5F4F1"   // canvas (cream)
        readonly property color panel:         "#FFFFFF"   // card / modal
        readonly property color sidebar:       "#0E0E12"   // dark in both modes (intentional)
        readonly property color border:        "#E6E3DC"
        readonly property color borderStrong:  "#C8C3B8"
        readonly property color divider:       Qt.rgba(0, 0, 0, 0.05)
        readonly property color ink1:          "#0E0E12"   // body
        readonly property color ink2:          "#2A2A32"   // secondary
        readonly property color ink3:          "#5C5C66"   // tertiary
        readonly property color ink4:          "#8A8A94"   // placeholder / caption
        readonly property color textOnAccent:      "#FFFFFF"
        readonly property color textOnSidebar:     "#F5F4F1"
    }

    // ── Dark palette ─────────────────────────────────────
    readonly property var dark: QtObject {
        readonly property color bg:            "#0E0E12"
        readonly property color panel:         "#17171E"
        readonly property color sidebar:       "#0A0A0E"
        readonly property color border:        "#25252E"
        readonly property color borderStrong:  "#3A3A45"
        readonly property color divider:       Qt.rgba(1, 1, 1, 0.05)
        readonly property color ink1:          "#F5F4F1"
        readonly property color ink2:          "#D8D6D0"
        readonly property color ink3:          "#A0A0AC"
        readonly property color ink4:          "#6F6F7A"
        readonly property color textOnAccent:      "#FFFFFF"
        readonly property color textOnSidebar:     "#F5F4F1"
    }
}
