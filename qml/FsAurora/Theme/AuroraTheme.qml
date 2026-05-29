// SPDX-License-Identifier: Proprietary
// FsAurora · theme singleton — the single source of truth for design tokens.
//
// Usage:
//   import FsAurora.Theme
//   color: AuroraTheme.panel
//   font.family: AuroraTheme.fontSans
//
// Dark-mode toggle is pushed from Main.qml (driven by SettingsViewModel).

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
    readonly property color danger:       AuroraColors.danger
    readonly property color dangerSoft:   AuroraColors.dangerSoft
    readonly property color dangerTint15: AuroraColors.dangerTint15
    readonly property color dangerGlow:   AuroraColors.dangerGlow
    readonly property color info:        AuroraColors.info
    readonly property color infoSoft:    AuroraColors.infoSoft

    // ── Surface / ink (adaptive) ─────────────────────────
    readonly property color bg:           _p.bg
    readonly property color panel:        _p.panel
    // Aurora-tier warm tint — used for row hover and the floating widget
    // footer per the Aurora Glow spec. Mode-adaptive (cream in light,
    // tinted-dark in dark).
    readonly property color bgWarm:       _p.bgWarm
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

    // ── Sidebar (Dark Aurora v2) ─────────────────────────
    // Always-dark palette for the left rail. Not mode-adaptive — see
    // AuroraColors.qml for the rationale (sidebar is intentionally near-
    // black so the orange brand gradient reads as the shell's signature).
    readonly property color sidebarBg:         AuroraColors.sidebarBg
    readonly property color sidebarBgElev:     AuroraColors.sidebarBgElev
    readonly property color sidebarBgElev2:    AuroraColors.sidebarBgElev2
    readonly property color sidebarBgHover:    AuroraColors.sidebarBgHover
    readonly property color sidebarLine:       AuroraColors.sidebarLine
    readonly property color sidebarLineStrong: AuroraColors.sidebarLineStrong
    readonly property color sidebarInk:        AuroraColors.sidebarInk
    readonly property color sidebarInk2:       AuroraColors.sidebarInk2
    readonly property color sidebarInk3:       AuroraColors.sidebarInk3
    readonly property color sidebarInk4:       AuroraColors.sidebarInk4
    readonly property color auroraSuccess:     AuroraColors.auroraSuccess
    readonly property color auroraDanger:      AuroraColors.auroraDanger

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
    // Aurora widget outer corner — 18px per the Aurora Glow spec. Sits
    // between radiusLg (14, generic cards) and radiusXl (20, hero CTAs).
    // Used specifically by the floating overlay widget surface.
    readonly property int radiusOuter:18
    readonly property int radiusXl:   20
    // Chrome button radius — 7px per Aurora spec. Snug for the 24x24
    // header buttons (Pause/Expand/Close) so they read as compact glyph
    // tiles rather than full pills.
    readonly property int radiusChrome: 7
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

    // ── Aurora widget — overlay HUD spec ─────────────────
    // Three canonical widths for the floating widget (collapsed peek →
    // normal default → expanded power-user). Heights are derived from
    // content (header + optional speedmeter + N rows + footer); only
    // widths are tokenised since they drive snap targets and layout.
    readonly property int widgetWidthCollapsed: 320
    readonly property int widgetWidthNormal:    364
    readonly property int widgetWidthExpanded:  420
    // Aurora signature shadow tier values — opacities are exposed so
    // the host (MiniHudWindow) can compose the multi-layer shadow with
    // Rectangle border rings without hardcoding magic numbers.
    readonly property real auroraGlowOpacity:   0.18  // warm halo
    readonly property real auroraStrokeOpacity: 0.05  // inner stroke
    readonly property real auroraDropOpacity:   0.06  // ambient drop
}
