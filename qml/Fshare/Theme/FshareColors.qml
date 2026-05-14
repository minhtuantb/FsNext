// SPDX-License-Identifier: Proprietary
// FshareColors — DEPRECATED. Kept as a thin singleton so old `Fshare.Theme`
// imports still resolve. All values forward to AuroraColors via AuroraTheme.

pragma Singleton
import QtQuick
import FsAurora.Theme 1.0

QtObject {
    readonly property color red:         AuroraTheme.accent
    readonly property color redHover:    AuroraTheme.accentHover
    readonly property color redPressed:  AuroraTheme.accentPressed
    readonly property color redTint6:    AuroraTheme.accentSoft
    readonly property color redTint10:   AuroraTheme.accentTint10
    readonly property color redTint15:   AuroraTheme.accentTint15
    readonly property color redBorder:   AuroraTheme.accentTint15
    readonly property color redShadow:   AuroraTheme.accentShadow
    readonly property color redGlow:     AuroraTheme.accentGlow

    readonly property color green:       AuroraTheme.success
    readonly property color greenTint:   AuroraTheme.successSoft
    readonly property color amber:       AuroraTheme.warn
    readonly property color amberTint:   AuroraTheme.warnSoft
    readonly property color blue:        AuroraTheme.info
    readonly property color blueTint:    AuroraTheme.infoSoft
}
