// SPDX-License-Identifier: Proprietary
// FshareTheme — DEPRECATED back-compat alias for FsAurora.Theme.AuroraTheme.
//
// Every in-tree component and page has been migrated to import FsAurora.Theme
// directly (see ADR 003). This file remains only so external code that still
// imports `Fshare.Theme` does not break at runtime; it forwards every property
// to AuroraTheme. Remove `qml/Fshare/Theme/` entirely once we are sure no
// extension / plugin / vendored snippet still references it.

pragma Singleton
import QtQuick
import FsAurora.Theme 1.0

QtObject {
    // Brand
    readonly property color red:         AuroraTheme.accent
    readonly property color redHover:    AuroraTheme.accentHover
    readonly property color redPressed:  AuroraTheme.accentPressed
    readonly property color redTint6:    AuroraTheme.accentSoft
    readonly property color redTint10:   AuroraTheme.accentTint10
    readonly property color redTint15:   AuroraTheme.accentTint15
    readonly property color redBorder:   AuroraTheme.accentTint15
    readonly property color redShadow:   AuroraTheme.accentShadow
    readonly property color redGlow:     AuroraTheme.accentGlow

    // Semantic
    readonly property color green:       AuroraTheme.success
    readonly property color greenTint:   AuroraTheme.successSoft
    readonly property color greenBorder: Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25)
    readonly property color greenText:   AuroraTheme.success
    readonly property color amber:       AuroraTheme.warn
    readonly property color amberTint:   AuroraTheme.warnSoft
    readonly property color amberBorder: Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25)
    readonly property color amberText:   AuroraTheme.warn
    readonly property color blue:        AuroraTheme.info
    readonly property color blueTint:    AuroraTheme.infoSoft
    readonly property color blueBorder:  Qt.rgba(AuroraTheme.info.r, AuroraTheme.info.g, AuroraTheme.info.b, 0.20)
    readonly property color blueText:    AuroraTheme.info

    // Adaptive surface
    readonly property color bg0:          AuroraTheme.bg
    readonly property color bg1:          AuroraTheme.panel
    readonly property color bg2:          AuroraTheme.divider
    readonly property color bg3:          AuroraTheme.borderStrong
    readonly property color surface:      AuroraTheme.panel
    readonly property color border:       AuroraTheme.border
    readonly property color borderStrong: AuroraTheme.borderStrong
    readonly property color divider:      AuroraTheme.divider
    readonly property color text1:        AuroraTheme.ink1
    readonly property color text2:        AuroraTheme.ink2
    readonly property color text3:        AuroraTheme.ink3
    readonly property color text4:        AuroraTheme.ink4
    readonly property color textOnRed:    AuroraTheme.textOnAccent

    // Mode mirror — writes propagate to Aurora; reads just read it back.
    property bool isDark: AuroraTheme.isDark
    onIsDarkChanged: AuroraTheme.isDark = isDark
    readonly property string platform: Qt.platform.os === "osx" ? "macos" : "windows"

    // Typography
    readonly property string fontFamily: AuroraTheme.fontSans
    readonly property string monoFamily: AuroraTheme.fontMono

    // Type tokens
    readonly property var display:     AuroraTheme.hero
    readonly property var heading1:    AuroraTheme.h2
    readonly property var heading2:    AuroraTheme.h3
    readonly property var body:        AuroraTheme.body
    readonly property var bodyStrong:  AuroraTheme.bodyStrong
    readonly property var caption:     AuroraTheme.caption
    readonly property var captionMono: AuroraTheme.caption
    readonly property var label:       AuroraTheme.label
    readonly property var button:      AuroraTheme.bodyStrong

    // Spacing — value-equivalent
    readonly property int sp2:  2
    readonly property int sp4:  AuroraTheme.sp1
    readonly property int sp6:  6
    readonly property int sp8:  AuroraTheme.sp2
    readonly property int sp10: 10
    readonly property int sp12: AuroraTheme.sp3
    readonly property int sp14: 14
    readonly property int sp16: AuroraTheme.sp4
    readonly property int sp20: AuroraTheme.sp5
    readonly property int sp24: AuroraTheme.sp6
    readonly property int sp32: AuroraTheme.sp8
    readonly property int sp40: AuroraTheme.sp10
    readonly property int sp48: AuroraTheme.sp12

    // Radii
    readonly property int r4:    4
    readonly property int r6:    AuroraTheme.radiusSm
    readonly property int r8:    8
    readonly property int r10:   AuroraTheme.radiusMd
    readonly property int r12:   12
    readonly property int r14:   AuroraTheme.radiusLg
    readonly property int r16:   16
    readonly property int rPill: AuroraTheme.radiusPill

    // Animation
    readonly property int durFast:   AuroraTheme.durFast
    readonly property int durNormal: AuroraTheme.durBase
    readonly property int durSlow:   AuroraTheme.durSlow
    readonly property int durDialog: AuroraTheme.durSlow
    property bool reduceMotion: AuroraTheme.reduceMotion
    onReduceMotionChanged: AuroraTheme.reduceMotion = reduceMotion

    readonly property int easingFast:   AuroraTheme.easingStd
    readonly property int easingNormal: AuroraTheme.easingStd
    readonly property int easingSlow:   AuroraTheme.easingStd
    readonly property int easingDialog: Easing.OutBack

    // Icon sizes
    readonly property int iconSm: AuroraTheme.iconSm
    readonly property int iconMd: AuroraTheme.iconMd
    readonly property int iconLg: AuroraTheme.iconLg

    // Heights
    readonly property int heightInput:    AuroraTheme.heightInput
    readonly property int heightButtonSm: AuroraTheme.heightButtonSm
    readonly property int heightButtonMd: AuroraTheme.heightButtonMd
    readonly property int heightButtonLg: AuroraTheme.heightButtonLg
    readonly property int heightListRow:  AuroraTheme.heightListRow
    readonly property int heightTitlebar: AuroraTheme.heightTitlebar
    readonly property int heightToolbar:  AuroraTheme.heightToolbar
    readonly property int heightFooter:   42

    readonly property real letterSpacingOverline: 1.4
    readonly property real letterSpacingHeading:  -0.01
}
