// SPDX-License-Identifier: Proprietary
// FsButton (Aurora) — the flagship atom of the Aurora design.
// Primary variant uses the brand 135° gradient with an orange glow shadow;
// secondary uses a strong 1.5px ink border; ghost uses a subtle border;
// danger mirrors primary but with a red gradient.
//
// Usage:
//   FsButton { text: "Tải lên"; variant: "primary" }
//   FsButton { text: "Huỷ";    variant: "secondary" }
//   FsButton { text: "Xoá";    variant: "danger" }

import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import FsAurora.Theme

Item {
    id: root

    // ── Public API ───────────────────────────────────────
    property string text: "Button"
    property string icon: ""                          // FsIcon name; "" to hide
    property string variant: "primary"                 // primary | secondary | ghost | danger | link
    property string size: "md"                         // sm | md | lg
    property bool enabled: true
    property bool loading: false

    signal clicked()

    // ── Sizing ───────────────────────────────────────────
    readonly property int _h:
        size === "sm" ? AuroraTheme.heightButtonSm
      : size === "lg" ? AuroraTheme.heightButtonLg
      : AuroraTheme.heightButtonMd
    readonly property int _hPad:
        size === "sm" ? 14 : (size === "lg" ? 22 : 18)
    readonly property int _fontSize:
        size === "sm" ? 12 : (size === "lg" ? 14 : 13)
    readonly property int _radius: AuroraTheme.radiusMd

    implicitHeight: _h
    implicitWidth: row.implicitWidth + _hPad * 2
    opacity: enabled ? 1.0 : 0.45

    // ── Gradient primary / danger ────────────────────────
    FsGradientRect {
        id: gradBg
        anchors.fill: parent
        radius: root._radius
        visible: root.variant === "primary" || root.variant === "danger"
        stop0: root.variant === "danger" ? "#D53030" : AuroraTheme.accent
        stop1: root.variant === "danger" ? "#FF3D3D" : AuroraTheme.accent3
        stop2: root.variant === "danger" ? "#FF7A1A" : AuroraTheme.accent2
        z: 0

        // Glow shadow — only when hovered, not when pressed (so it reads
        // as "lifting off the page" during hover, snaps back on click).
        layer.enabled: root.variant === "primary" && mouse.containsMouse && !mouse.pressed
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 1.0
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 4
            shadowColor: AuroraTheme.accentShadow
            shadowScale: 1.02
        }
    }

    // ── Solid variants (secondary / ghost / link) ───────
    Rectangle {
        id: solidBg
        anchors.fill: parent
        radius: root._radius
        visible: !gradBg.visible
        color: {
            if (root.variant === "link") return "transparent"
            if (root.variant === "ghost") {
                return mouse.pressed ? AuroraTheme.accentTint15
                     : mouse.containsMouse ? AuroraTheme.accentSoft
                     : "transparent"
            }
            // secondary
            return mouse.pressed ? AuroraTheme.accentTint10
                 : mouse.containsMouse ? AuroraTheme.accentSoft
                 : AuroraTheme.panel
        }
        border.width: root.variant === "secondary" ? 1.5
                    : root.variant === "ghost" ? 1
                    : 0
        border.color: root.variant === "secondary" ? AuroraTheme.borderStrong
                    : root.variant === "ghost" ? AuroraTheme.border
                    : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }
    }

    // Hover/press tint over the gradient — darkens slightly on press
    Rectangle {
        anchors.fill: parent
        radius: root._radius
        visible: gradBg.visible
        color: mouse.pressed ? Qt.rgba(0, 0, 0, 0.14)
             : mouse.containsMouse ? Qt.rgba(1, 1, 1, 0.08)
             : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }
        z: 1
    }

    // ── Content ──────────────────────────────────────────
    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: root.icon !== "" || root.loading ? AuroraTheme.sp2 : 0
        z: 2

        FsIcon {
            visible: root.icon !== "" && !root.loading
            name: root.icon
            sizePx: root.size === "lg" ? AuroraTheme.iconMd : AuroraTheme.iconSm
            color: root._textColor
        }

        // Simple rotating ring while loading
        Rectangle {
            visible: root.loading
            width: 14; height: 14; radius: 7
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(root._textColor.r, root._textColor.g, root._textColor.b, 0.35)
            Rectangle {
                width: 14; height: 14; radius: 7
                color: "transparent"
                border.width: 2
                border.color: root._textColor
                // Only top-left quarter is visible via rotation trick
                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    width: parent.width / 2
                    height: parent.height / 2
                    color: Qt.rgba(root._textColor.r, root._textColor.g, root._textColor.b, 0.35)
                    visible: false  // actually don't need — the outer border does the job
                }
                RotationAnimator on rotation {
                    from: 0; to: 360
                    duration: 900; loops: Animation.Infinite
                    running: root.loading
                }
            }
        }

        Text {
            text: root.text
            font.family: AuroraTheme.fontSans
            font.pixelSize: root._fontSize
            font.weight: Font.DemiBold
            font.letterSpacing: root.variant === "link" ? 0 : -0.1
            color: root._textColor
            Behavior on color { enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast } }
        }
    }

    // ── Text color per variant ───────────────────────────
    readonly property color _textColor: {
        switch (variant) {
        case "primary":
        case "danger":   return AuroraTheme.textOnAccent
        case "secondary":return AuroraTheme.ink1
        case "ghost":    return AuroraTheme.ink2
        case "link":     return AuroraTheme.accent
        default:         return AuroraTheme.textOnAccent
        }
    }

    // ── Interaction ──────────────────────────────────────
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled && !root.loading) root.clicked()
        z: 3
    }

    // Translate-up 1px on hover (Aurora spec: "lift")
    transform: Translate {
        y: root.variant === "primary" && mouse.containsMouse && !mouse.pressed ? -1 : 0
        Behavior on y { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }
    }
}
