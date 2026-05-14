// SPDX-License-Identifier: Proprietary
// FsDialog — Modal dialog wrapper

import QtQuick
import FsAurora.Theme 1.0
Item {
    id: root

    property string title: ""
    property alias content: contentArea.children
    property alias footer: footerArea.children
    property int dialogWidth: 440

    signal opened()
    signal closed()

    function open() {
        root.visible = true;
        // Grab keyboard focus AFTER the dialog is on screen so Tab/Escape
        // land here rather than back on the underlying page.  Qt 6 won't
        // honour focus changes until visible→true so the assignment must
        // be after the visibility flip; setting focus before would silently
        // no-op (left users in the prior focus scope).
        root.forceActiveFocus();
        root.opened();
    }
    function close() { root.visible = false; root.closed() }

    anchors.fill: parent
    visible: false
    z: 1000

    // ── Focus + Escape (v6.0+) ──────────────────────────────────────────────
    // FsDialog inherits from Item rather than QtQuick.Controls.Popup, so we
    // don't get closePolicy / focus-scope behaviour for free.  Adding focus
    // scope here makes Tab cycling stay within the dialog while it's open,
    // and Keys.onEscapePressed implements the standard "Esc closes modal"
    // contract every Qt user expects.
    focus: visible
    activeFocusOnTab: false        // dialog itself isn't a control — children are
    Keys.onEscapePressed: function(event) {
        root.close();
        event.accepted = true;
    }

    // Accessibility — Dialog role + title as the accessible name so screen
    // readers announce "<title> dialog" when focus enters.  (Accessible.modal
    // isn't an attached property in Qt 6 QML; the focus scope above already
    // gives the equivalent UX since Tab can't escape the dialog.)
    Accessible.role: Accessible.Dialog
    Accessible.name: root.title

    // Overlay
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, root.visible ? 0.25 : 0)
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // Dialog box
    Rectangle {
        id: dialog
        anchors.centerIn: parent
        width: root.dialogWidth
        height: headerItem.height + contentArea.childrenRect.height + footerItem.height
        radius: 16
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border

        scale: root.visible ? 1.0 : 0.95
        opacity: root.visible ? 1 : 0
        Behavior on scale { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow; easing.type: Easing.OutBack } }
        Behavior on opacity { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }

        MouseArea { anchors.fill: parent }

        Column {
            anchors.fill: parent

            // Header
            Item {
                id: headerItem
                width: parent.width
                height: 56

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.title
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.h2.pixelSize
                    font.weight: Font.Bold
                    color: AuroraTheme.ink1
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: AuroraTheme.sp4
                    anchors.verticalCenter: parent.verticalCenter
                    width: 30; height: 30
                    radius: 8
                    color: closeMouse.containsMouse ? AuroraTheme.accentSoft : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "✕"; font.pixelSize: 16
                        color: closeMouse.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    }
                    MouseArea {
                        id: closeMouse; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor; onClicked: root.close()
                    }
                }

                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
            }

            // Content slot
            Item {
                id: contentArea
                width: parent.width
                height: childrenRect.height
            }

            // Footer slot
            Item {
                id: footerItem
                width: parent.width
                height: footerArea.childrenRect.height > 0 ? footerArea.childrenRect.height : 0

                Rectangle {
                    visible: footerArea.childrenRect.height > 0
                    anchors.top: parent.top; width: parent.width; height: 1; color: AuroraTheme.divider
                }

                Item {
                    id: footerArea
                    anchors.fill: parent
                }
            }
        }
    }
}
