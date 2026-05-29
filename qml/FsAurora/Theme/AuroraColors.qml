// SPDX-License-Identifier: Proprietary
// FsAurora · color tokens — mirrors qml/FsAurora/handoff/design-tokens.json.

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
    // 15% tier used where the error indicator needs to read at a glance
    // (e.g. HUD row icon backgrounds) — mirrors the accentTint10/15 pair.
    readonly property color dangerTint15:  Qt.rgba(0.835, 0.188, 0.188, 0.15)
    // Focus-ring tier for error state — mirrors accentGlow at 0.22.
    readonly property color dangerGlow:    Qt.rgba(0.835, 0.188, 0.188, 0.22)
    readonly property color info:          "#2566E5"
    readonly property color infoSoft:      Qt.rgba(0.145, 0.400, 0.898, 0.10)

    // ── Sidebar (Dark Aurora v2) ─────────────────────────
    // Independent palette for the sidebar surface. The rail is intentionally
    // always near-black (#0E0E12) so the orange brand gradient pops as the
    // signature accent of the entire shell. These tokens live outside the
    // light/dark adaptive palette because the sidebar never switches mode.
    readonly property color sidebarBg:           "#0E0E12"
    readonly property color sidebarBgElev:       "#1A1A22"   // avatar pivot · HUD card · popover
    readonly property color sidebarBgElev2:      "#252530"   // active nav row · row hover
    readonly property color sidebarBgHover:      Qt.rgba(1, 1, 1, 0.04)  // popover item hover
    readonly property color sidebarLine:         "#25252D"   // 1px separator · card border
    readonly property color sidebarLineStrong:   "#35353F"   // active border · inactive dot
    readonly property color sidebarInk:          "#EDEDEF"   // active label · hero number
    readonly property color sidebarInk2:         "#C9C9D4"   // inactive label · stat text
    readonly property color sidebarInk3:         "#8E8E9E"   // inactive icon · ETA value
    readonly property color sidebarInk4:         "#5A5A6E"   // separator label · sub-count

    // Status accents for the sidebar HUD — mint pulse + hot pink danger.
    // Separate from the semantic `success`/`danger` (which are tuned for
    // light-bg buttons & badges) so the sidebar's near-black surface gets
    // saturated, glowy versions that read at a glance.
    readonly property color auroraSuccess:       "#3DE0B5"
    readonly property color auroraDanger:        "#FF5470"

    // ── Light palette ────────────────────────────────────
    readonly property var light: QtObject {
        readonly property color bg:            "#F5F4F1"   // canvas (cream)
        readonly property color panel:         "#FFFFFF"   // card / modal
        // bgWarm — Aurora-tier warm tint used for row hover and the
        // floating widget's footer band. Cream with a barely-there
        // orange undertone so the Aurora gradient brand reads as the
        // same family of warmth (vs. a neutral grey hover which fights
        // the cam-hồng-mango identity). #FBF8F1 per Aurora widget spec.
        readonly property color bgWarm:        "#FBF8F1"
        // Sidebar is always dark (decision). #1E1F2C is a slate-indigo —
        // a visibly tinted dark rather than near-black, so the rail reads as
        // an intentional surface (not "off"). Earlier #0A0A0E / #13141E had
        // too little luma headroom for the white overlay system (hover /
        // active / card chrome / borders) — every tint sat at <2:1 and the
        // sidebar looked dim/muddy. The lift to L≈0.012 lets α=0.10-0.25
        // overlays read as clearly tinted layers without breaking the
        // dark-mode aesthetic. Text contrast stays excellent (10+:1 for the
        // 92% white nav labels).
        readonly property color sidebar:       "#1E1F2C"
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
        // Dark-mode bgWarm — a faintly warmer / brighter tier above panel
        // so hover/footer surfaces lift visibly without losing the dark
        // aesthetic. Picked to keep ≥3.5:1 separation from #17171E.
        readonly property color bgWarm:        "#1F1E22"
        // Same midnight indigo as light — keeps the sidebar identical across
        // modes (intentional brand decision: nav rail is always dark).
        readonly property color sidebar:       "#1E1F2C"
        readonly property color border:        "#25252E"
        readonly property color borderStrong:  "#3A3A45"
        readonly property color divider:       Qt.rgba(1, 1, 1, 0.05)
        readonly property color ink1:          "#F5F4F1"
        readonly property color ink2:          "#D8D6D0"
        readonly property color ink3:          "#A0A0AC"
        // ink4: bumped from #6F6F7A → #8A8A98 (Sprint 2 item O).
        // Previously ≈5.4:1 on bg #0E0E12 — passed WCAG AA for normal text but
        // was borderline for the 11px caption / placeholder uses that consume
        // this token across the app. New value ≈7:1 → pass AAA for small text.
        readonly property color ink4:          "#8A8A98"
        readonly property color textOnAccent:      "#FFFFFF"
        readonly property color textOnSidebar:     "#F5F4F1"
    }
}
