// SPDX-License-Identifier: Proprietary
// FsButton — the flagship button atom (design-system canonical, Fshare.Components).
//
// Merged from the two former implementations (design-system Stage 2, 2026-05-29):
//   • Visuals from the Aurora button — 135° brand gradient + orange glow + lift
//     on primary/danger, FsIcon-based leading icon, rounded-rect (radiusMd),
//     loading spinner, "link" variant.
//   • A11y from the Fshare button — Accessible.* roles, Tab focus, Space/Enter
//     keyboard activation, and an accent focus ring (matches FsTextField).
//
// Usage:
//   FsButton { text: "Tải lên"; variant: "primary"; icon: "arrow-up" }
//   FsButton { text: "Huỷ";     variant: "ghost" }
//   FsButton { text: "Xoá";     variant: "danger"; onClicked: { ... } }

import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import FsAurora.Theme 1.0

Item {
    id: root

    // ── Public API ───────────────────────────────────────
    property string text: "Button"
    property string icon: ""                    // FsIcon name; "" to hide
    property string variant: "primary"          // primary | secondary | ghost | danger | link | success
    property string size: "md"                  // sm | md | lg  ("default" accepted as md)
    property bool enabled: true
    property bool loading: false
    // Screen-reader label override (e.g. icon-only buttons). Falls back to text.
    property string accessibleName: ""

    signal clicked()

    // Accept the legacy Fshare "default" size as an alias for "md".
    readonly property string _size: size === "default" ? "md" : size

    // ── Accessibility (v6.0+) ────────────────────────────
    Accessible.role: Accessible.Button
    Accessible.name: accessibleName.length > 0 ? accessibleName : text
    Accessible.description: text
    Accessible.focusable: true
    Accessible.onPressAction: if (enabled && !loading) root.clicked()

    // Opt into the Tab focus chain (Qt 6 leaves Item-based controls out by
    // default). Disabled buttons drop out so Tab never lands on a dead control.
    activeFocusOnTab: enabled
    Keys.onPressed: function(event) {
        if (!enabled || loading) return;
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter) {
            root.clicked();
            event.accepted = true;
        }
    }

    // ── Sizing ───────────────────────────────────────────
    readonly property int _h:
        _size === "sm" ? AuroraTheme.heightButtonSm
      : _size === "lg" ? AuroraTheme.heightButtonLg
      : AuroraTheme.heightButtonMd
    readonly property int _hPad: _size === "sm" ? 14 : (_size === "lg" ? 22 : 18)
    readonly property int _fontSize: _size === "sm" ? 12 : (_size === "lg" ? 14 : 13)
    readonly property int _radius: AuroraTheme.radiusMd

    readonly property bool _gradient: variant === "primary" || variant === "danger"

    implicitHeight: _h
    implicitWidth: row.implicitWidth + _hPad * 2
    opacity: enabled ? 1.0 : 0.45

    // ── Gradient primary / danger ────────────────────────
    FsGradientRect {
        id: gradBg
        anchors.fill: parent
        radius: root._radius
        visible: root._gradient
        stop0: root.variant === "danger" ? "#D53030" : AuroraTheme.accent
        stop1: root.variant === "danger" ? "#FF3D3D" : AuroraTheme.accent3
        stop2: root.variant === "danger" ? "#FF7A1A" : AuroraTheme.accent2
        z: 0

        // Glow shadow — only on hover (not pressed) so it reads as "lifting".
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

    // ── Solid variants (secondary / ghost / link / success) ──
    Rectangle {
        id: solidBg
        anchors.fill: parent
        radius: root._radius
        visible: !root._gradient
        color: {
            if (root.variant === "link") return "transparent"
            if (root.variant === "ghost") {
                return mouse.pressed ? AuroraTheme.accentTint15
                     : mouse.containsMouse ? AuroraTheme.accentSoft
                     : "transparent"
            }
            if (root.variant === "success") {
                return mouse.pressed ? Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25)
                     : mouse.containsMouse ? Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.18)
                     : AuroraTheme.successSoft
            }
            // secondary
            return mouse.pressed ? AuroraTheme.accentTint10
                 : mouse.containsMouse ? AuroraTheme.accentSoft
                 : AuroraTheme.panel
        }
        border.width: root.variant === "secondary" ? 1.5
                    : root.variant === "ghost" ? 1
                    : root.variant === "success" ? 1
                    : 0
        border.color: root.variant === "secondary" ? AuroraTheme.borderStrong
                    : root.variant === "ghost" ? AuroraTheme.border
                    : root.variant === "success" ? Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25)
                    : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }
    }

    // Hover/press tint over the gradient — darkens slightly on press.
    Rectangle {
        anchors.fill: parent
        radius: root._radius
        visible: root._gradient
        color: mouse.pressed ? Qt.rgba(0, 0, 0, 0.14)
             : mouse.containsMouse ? Qt.rgba(1, 1, 1, 0.08)
             : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }
        z: 1
    }

    // Focus ring — accent ring just outside the button (matches FsTextField),
    // visible only on keyboard focus.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: root._radius + 3
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.55)
        visible: root.activeFocus
        z: 4
    }

    // ── Content ──────────────────────────────────────────
    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: root.icon !== "" || root.loading ? AuroraTheme.sp2 : 0
        z: 2

        FsIcon {   // same-module sibling (Fshare.Components)
            visible: root.icon !== "" && !root.loading
            name: root.icon
            sizePx: root._size === "lg" ? AuroraTheme.iconMd : AuroraTheme.iconSm
            color: root._textColor
        }

        // Rotating ring while loading.
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
        case "success":  return AuroraTheme.success
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

    // Lift 1px on hover for primary (Aurora spec).
    transform: Translate {
        y: root.variant === "primary" && mouse.containsMouse && !mouse.pressed ? -1 : 0
        Behavior on y { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }
    }
}
