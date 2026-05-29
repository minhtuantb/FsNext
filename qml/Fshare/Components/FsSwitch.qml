// SPDX-License-Identifier: Proprietary
// FsSwitch — on/off toggle (design-system canonical, Fshare.Components).
//
// Merged (design-system Stage 2, 2026-05-29): Fshare a11y/keyboard/focus-ring +
// solid accent track, plus the former Aurora `label` (optional text to the right).
//
// Usage:
//   FsSwitch { checked: true; onToggled: console.log(checked) }
//   FsSwitch { label: "Bật đồng bộ"; checked: true }

import QtQuick
import FsAurora.Theme 1.0

Item {
    id: root

    property bool checked: false
    property bool enabled: true
    property string label: ""           // optional trailing label
    signal toggled(bool checked)

    implicitWidth: sw.width + (label !== "" ? lbl.implicitWidth + AuroraTheme.sp2 : 0)
    implicitHeight: 24
    opacity: enabled ? 1.0 : 0.45

    Accessible.role: Accessible.CheckBox
    Accessible.name: label
    Accessible.checkable: true
    Accessible.checked: root.checked

    // Tab in, Space toggles — WAI-ARIA "switch". Return/Enter intentionally
    // unbound so they don't steal a dialog's primary action when focused.
    activeFocusOnTab: enabled
    Keys.onPressed: function(event) {
        if (!enabled) return;
        if (event.key === Qt.Key_Space) {
            checked = !checked;
            root.toggled(checked);
            event.accepted = true;
        }
    }

    // ── Switch graphic (fixed 44×24) ─────────────────────
    Item {
        id: sw
        width: 44; height: 24
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter

        // Focus ring — accent halo matching FsButton/FsTextField.
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
            color: root.checked ? AuroraTheme.accent : AuroraTheme.borderStrong
            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

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
    }

    Text {
        id: lbl
        visible: root.label !== ""
        anchors.left: sw.right
        anchors.leftMargin: AuroraTheme.sp2
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        font.family: AuroraTheme.fontSans
        font.pixelSize: 13
        color: AuroraTheme.ink1
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
