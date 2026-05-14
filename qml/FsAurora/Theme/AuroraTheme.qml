// SPDX-License-Identifier: Proprietary
// FsAurora · theme singleton — parallel to Fshare.Theme.FshareTheme.
//
// Usage:
//   import FsAurora.Theme
//   color: AuroraTheme.panel
//   font.family: AuroraTheme.fontSans
//
// Dark-mode sync is pushed from Main.qml (same source-of-truth as
// FshareTheme.isDark) so the two systems stay visually aligned when the
// user toggles theme while comparing variants.

pragma Singleton
import QtQuick
import FsAurora.Theme

QtObject {
    id: theme

    // ── Mode ─────────────────────────────────────────────
    property bool isDark: false
    property var _p: isDark ? AuroraColors.dark : AuroraColors.light

    // ── Brand ────────────────────────────────────────────
    readonly property color accent:        AuroraColors.accent
    readonly property color accent2:       AuroraColors.accent2
    readonly property color accent3:       AuroraColors.accent3
    readonly property color accentHover:   AuroraColors.accentHover
    readonly property color accentPressed: AuroraColors.accentPressed
    readonly property color accentGlow:    AuroraColors.accentGlow
    readonly property color accentShadow:  AuroraColors.accentShadow
    readonly property color accentSoft:    AuroraColors.accentSoft
    readonly property color accentTint10:  AuroraColors.accentTint10
    readonly property color accentTint15:  AuroraColors.accentTint15
    readonly property var   gradientStops: AuroraColors.gradientStops

    // ── Semantic ─────────────────────────────────────────
    readonly property color success:     AuroraColors.success
    readonly property color successSoft: AuroraColors.successSoft
    readonly property color warn:        AuroraColors.warn
    readonly property color warnSoft:    AuroraColors.warnSoft
    readonly property color danger:      AuroraColors.danger
    readonly property color dangerSoft:  AuroraColors.dangerSoft
    readonly property color info:        AuroraColors.info
    readonly property color infoSoft:    AuroraColors.infoSoft

    // ── Surface / ink (adaptive) ─────────────────────────
    readonly property color bg:           _p.bg
    readonly property color panel:        _p.panel
    readonly property color sidebar:      _p.sidebar          // always dark
    readonly property color border:       _p.border
    readonly property color borderStrong: _p.borderStrong
    readonly property color divider:      _p.divider
    readonly property color ink1:         _p.ink1
    readonly property color ink2:         _p.ink2
    readonly property color ink3:         _p.ink3
    readonly property color ink4:         _p.ink4
    readonly property color textOnAccent:     _p.textOnAccent
    readonly property color textOnSidebar:    _p.textOnSidebar

    // ── Typography ───────────────────────────────────────
    // Aurora calls for Geist + Be Vietnam Pro (sans) / Geist Mono (mono) /
    // Instrument Serif (editorial hero). Fall back to system fonts if the
    // font files weren't shipped with the build — user visible result is
    // identical glyph shapes but not pixel-identical to the handoff file.
    readonly property string fontSans:
        "'Geist', 'Be Vietnam Pro', -apple-system, 'Segoe UI Variable', 'Segoe UI', system-ui, sans-serif"
    readonly property string fontMono:
        "'Geist Mono', ui-monospace, Consolas, 'Cascadia Code', monospace"
    readonly property string fontSerif:
        "'Instrument Serif', Georgia, serif"

    // ── Type scale ───────────────────────────────────────
    // Pixel sizes chosen to track the handoff 1:1. Qt renders at device
    // pixels so this is direct.
    readonly property var hero:       ({ pixelSize: 48, weight: Font.Normal,    letterSpacing: -1.4, serif: true  })
    readonly property var heroSmall:  ({ pixelSize: 36, weight: Font.Normal,    letterSpacing: -1.1, serif: true  })
    readonly property var h1:         ({ pixelSize: 32, weight: Font.Bold,      letterSpacing: -0.6, serif: false })
    readonly property var h2:         ({ pixelSize: 22, weight: Font.Bold,      letterSpacing: -0.2, serif: false })
    readonly property var h3:         ({ pixelSize: 17, weight: Font.DemiBold,  letterSpacing: -0.1, serif: false })
    readonly property var body:       ({ pixelSize: 13, weight: Font.Normal,    letterSpacing:  0.0, serif: false })
    readonly property var bodyStrong: ({ pixelSize: 13, weight: Font.DemiBold,  letterSpacing:  0.0, serif: false })
    readonly property var small:      ({ pixelSize: 12, weight: Font.Normal,    letterSpacing:  0.0, serif: false })
    readonly property var caption:    ({ pixelSize: 11, weight: Font.Medium,    letterSpacing:  0.0, serif: false })
    readonly property var label:      ({ pixelSize: 11, weight: Font.Bold,      letterSpacing:  1.5, serif: false, mono: true })  // uppercase tracking
    readonly property var mono:       ({ pixelSize: 12, weight: Font.Medium,    letterSpacing:  0.0, serif: false, mono: true })

    // ── Spacing (4px grid) ───────────────────────────────
    readonly property int sp1:  4
    readonly property int sp2:  8
    readonly property int sp3:  12
    readonly property int sp4:  16
    readonly property int sp5:  20
    readonly property int sp6:  24
    readonly property int sp8:  32
    readonly property int sp10: 40
    readonly property int sp12: 48
    readonly property int sp16: 64

    // ── Radius ───────────────────────────────────────────
    readonly property int radiusSm:    6
    readonly property int radiusMd:   10
    readonly property int radiusLg:   14
    readonly property int radiusXl:   20
    readonly property int radiusPill: 999

    // ── Motion ───────────────────────────────────────────
    // Aurora spec: cubic-bezier(.2, 0, 0, 1). Qt maps to OutCubic-ish; for a
    // closer match we expose BezierSpline control points too, so Behaviors
    // that need the exact curve can opt in.
    readonly property int durFast: 140
    readonly property int durBase: 220
    readonly property int durSlow: 360
    readonly property int easingStd: Easing.OutCubic     // close enough
    readonly property var bezierStd: [0.2, 0.0, 0.0, 1.0]
    property bool reduceMotion: false

    // ── Icon sizes ───────────────────────────────────────
    readonly property int iconSm: 16
    readonly property int iconMd: 20
    readonly property int iconLg: 24

    // ── Interactive heights ──────────────────────────────
    readonly property int heightInput:    40
    readonly property int heightButtonSm: 32
    readonly property int heightButtonMd: 40
    readonly property int heightButtonLg: 48
    readonly property int heightListRow:  44
    readonly property int heightTitlebar: 40
    readonly property int heightToolbar:  56
}
