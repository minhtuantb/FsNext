// SPDX-License-Identifier: Proprietary
// FsSwitch — On/off toggle (Aurora-native, 44×24 pill with sliding thumb)
//
// Usage:
//   FsSwitch { checked: true; onToggled: console.log(checked) }

import QtQuick
import FsAurora.Theme 1.0

Item {
    id: root

    property bool checked: false
    property bool enabled: true
    signal toggled(bool checked)

    implicitWidth: 44
    implicitHeight: 24
    opacity: enabled ? 1.0 : 0.45

    Accessible.role: Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: root.checked

    // ── Keyboard support (v6.0+) ────────────────────────────────────────────
    // Tab in, Space toggles — matches HTML <input type="checkbox"> + WAI-ARIA
    // "switch" role.  Return/Enter are NOT bound because the switch isn't a
    // form submitter; binding them would steal Enter from the dialog's
    // primary action when the switch happens to be focused.
    activeFocusOnTab: enabled
    Keys.onPressed: function(event) {
        if (!enabled) return;
        if (event.key === Qt.Key_Space) {
            checked = !checked;
            root.toggled(checked);
            event.accepted = true;
        }
    }

    // Focus ring — same accent halo as FsButton/FsTextField so the visual
    // language is consistent.  Sits one pixel out so the ring doesn't fight
    // the track color animation.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: (parent.height + 6) / 2
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                               AuroraTheme.accent.b, 0.55)
        visible: root.activeFocus
        z: -1
    }

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        // Track: accent gradient feel via accentTint when off (subtle vs flat grey),
        // solid accent when on. Aurora doesn't expose bg3-equivalent so we use
        // borderStrong for the off state — visually identical and palette-correct.
        color: root.checked ? AuroraTheme.accent : AuroraTheme.borderStrong
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

        // Thumb
        Rectangle {
            width: parent.height - 4
            height: parent.height - 4
            radius: width / 2
            color: AuroraTheme.panel
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - 2 : 2
            Behavior on x { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast; easing.type: Easing.OutCubic } }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (!root.enabled) return;
            root.checked = !root.checked;
            root.toggled(root.checked);
        }
    }
}
