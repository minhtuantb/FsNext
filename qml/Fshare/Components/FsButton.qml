// SPDX-License-Identifier: Proprietary
// FsButton — Pill-shaped button with variants (Aurora-native)
//
// Usage:
//   FsButton { text: "Tải lên"; icon: "⬆"; variant: "primary"; onClicked: { ... } }
//   FsButton { text: "Huỷ"; variant: "ghost" }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Item {
    id: root

    // -- Public API --
    property string text: "Button"
    property string icon: ""
    property string variant: "primary"  // primary | secondary | ghost | danger | success
    property string size: "default"     // sm | default | lg
    property bool enabled: true
    // Override `text` when the rendered label isn't enough (e.g. icon-only
    // toolbar buttons).  Falls back to `text` so the common case stays
    // zero-friction.  Screen readers (NVDA / JAWS / Narrator) read this aloud.
    property string accessibleName: ""

    signal clicked()

    Accessible.role: Accessible.Button
    Accessible.name: accessibleName.length > 0 ? accessibleName : text
    Accessible.description: text
    Accessible.focusable: true
    Accessible.onPressAction: if (enabled) root.clicked()

    // ── Keyboard support (v6.0+) ────────────────────────────────────────────
    // Opt into the Tab focus chain — Qt 6 leaves Item-based controls OUT of
    // it by default, which had left every FsButton in the app unreachable.
    // Disabled buttons drop out of the chain so a user holding Tab doesn't
    // land on something that won't react.
    activeFocusOnTab: enabled
    // Space / Return / Enter — match WAI-ARIA "button" expectations.  We
    // do NOT swallow other keys so KeyNavigation chains layered above keep
    // working (Tab/Shift+Tab continue to flow through the focus scope).
    Keys.onPressed: function(event) {
        if (!enabled) return;
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter) {
            root.clicked();
            event.accepted = true;
        }
    }

    // -- Sizing --
    property int _vPad: size === "sm" ? 6 : (size === "lg" ? 12 : 9)
    property int _hPad: size === "sm" ? 14 : (size === "lg" ? 24 : 18)
    property int _fontSize: size === "sm" ? 12 : (size === "lg" ? 15 : 13)
    property int _height: size === "sm" ? AuroraTheme.heightButtonSm
                       : (size === "lg" ? AuroraTheme.heightButtonLg
                                        : AuroraTheme.heightButtonMd)

    implicitWidth: row.implicitWidth + _hPad * 2
    implicitHeight: _height
    opacity: enabled ? 1.0 : 0.45

    // -- Colors by variant (Aurora palette) --
    // success uses Aurora success token + soft tints derived inline since
    // AuroraTheme exposes successSoft but not a "harder" hover shade.
    property color _bg: {
        switch (variant) {
        case "primary":   return AuroraTheme.accent
        case "secondary": return AuroraTheme.accentSoft
        case "ghost":     return "transparent"
        case "danger":    return AuroraTheme.danger
        case "success":   return AuroraTheme.successSoft
        default:          return AuroraTheme.accent
        }
    }
    property color _bgHover: {
        switch (variant) {
        case "primary":   return AuroraTheme.accentHover
        case "secondary": return AuroraTheme.accentTint15
        case "ghost":     return AuroraTheme.accentTint10
        case "danger":    return Qt.lighter(AuroraTheme.danger, 1.1)
        case "success":   return Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.18)
        default:          return AuroraTheme.accentHover
        }
    }
    property color _bgPress: {
        switch (variant) {
        case "primary":   return AuroraTheme.accentPressed
        case "secondary": return AuroraTheme.accentTint15
        case "ghost":     return AuroraTheme.accentTint15
        case "danger":    return Qt.darker(AuroraTheme.danger, 1.1)
        case "success":   return Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25)
        default:          return AuroraTheme.accentPressed
        }
    }
    property color _textColor: {
        switch (variant) {
        case "primary":   return AuroraTheme.textOnAccent
        case "secondary": return AuroraTheme.accent
        case "ghost":     return AuroraTheme.ink2
        case "danger":    return AuroraTheme.textOnAccent
        case "success":   return AuroraTheme.success
        default:          return AuroraTheme.textOnAccent
        }
    }
    property color _borderColor: {
        switch (variant) {
        case "secondary": return AuroraTheme.accentTint15
        case "ghost":     return AuroraTheme.border
        case "success":   return Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25)
        default:          return "transparent"
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: AuroraTheme.radiusPill
        color: mouse.pressed ? root._bgPress : (mouse.containsMouse ? root._bgHover : root._bg)
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
        border.width: root._borderColor === "transparent" ? 0 : 1.5
        border.color: root._borderColor
    }

    // Focus ring — matches FsTextField's accent-glow pattern so keyboard
    // users get the same visual cue across every primitive.  Sits OUTSIDE
    // the pill (margin -3) so it doesn't fight the bg color animation and
    // remains visible even on solid `primary` buttons where accent already
    // is the bg.  z = -1 keeps the ring under the bg fill but above any
    // backdrop the page has placed behind the button.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: AuroraTheme.radiusPill + 3
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                               AuroraTheme.accent.b, 0.55)
        visible: root.activeFocus
        z: -1
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: root.icon !== "" ? AuroraTheme.sp2 : 0

        Text {
            visible: root.icon !== ""
            text: root.icon
            font.pixelSize: root._fontSize
            color: root._textColor
        }

        Text {
            text: root.text
            font.family: AuroraTheme.fontSans
            font.pixelSize: root._fontSize
            font.weight: Font.DemiBold
            color: root._textColor
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled) root.clicked()
    }

    // Press scale effect — disabled under reduceMotion.
    transform: Scale {
        origin.x: root.width / 2
        origin.y: root.height / 2
        xScale: mouse.pressed ? 0.97 : 1.0
        yScale: mouse.pressed ? 0.97 : 1.0
        Behavior on xScale { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: 60 } }
        Behavior on yScale { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: 60 } }
    }
}
