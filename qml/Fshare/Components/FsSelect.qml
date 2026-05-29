// SPDX-License-Identifier: Proprietary
// FsSelect — Themed dropdown select built on Qt Quick Controls' Popup.
//
// Usage:
//   FsSelect {
//       model: ["Option A", "Option B", "Option C"]
//       currentIndex: 0
//       onActivated: console.log(currentIndex, currentText)
//   }
//
// Implementation note:
//   Earlier revisions hand-rolled the dropdown as a Rectangle reparented to
//   `root.Window.contentItem` and positioned via `mapToItem`. That works for
//   a select sitting directly on a Page, but it breaks when the select is
//   inside a modal (FsDialog) — the Window-rooted popup ends up either
//   below the dialog overlay or at stale coordinates because the binding
//   doesn't re-evaluate when ancestors move during the dialog's open
//   animation. Switching to Controls.Popup hands all of this — overlay
//   parenting, modal stacking, close-on-press-outside, focus capture — to
//   Qt's own popup machinery, which is correct in both contexts.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    property var model: []
    property int currentIndex: 0
    property string placeholder: qsTr("Chọn...")
    property bool enabled: true
    readonly property string currentText: {
        if (currentIndex < 0 || currentIndex >= (model ? model.length : 0)) return placeholder;
        const it = model[currentIndex];
        return (typeof it === "string") ? it : (it.label || it.text || "");
    }

    signal activated(int index)

    implicitWidth: 200
    implicitHeight: 36
    opacity: enabled ? 1.0 : 0.45

    Accessible.role: Accessible.ComboBox
    Accessible.name: root.currentText
    Accessible.editable: false

    // ── Keyboard support ──────────────────────────────────────────────────
    // Space/Enter open the popup; arrow keys cycle when the popup is closed
    // (matches native ComboBox).  Up/Down inside the open popup is handled
    // by the ListView below.
    activeFocusOnTab: enabled
    Keys.onPressed: function(event) {
        if (!enabled) return;
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter) {
            popup.opened ? popup.close() : popup.open();
            event.accepted = true;
        } else if (!popup.opened && event.key === Qt.Key_Down) {
            if (currentIndex < (model ? model.length - 1 : 0)) {
                currentIndex += 1;
                root.activated(currentIndex);
            }
            event.accepted = true;
        } else if (!popup.opened && event.key === Qt.Key_Up) {
            if (currentIndex > 0) {
                currentIndex -= 1;
                root.activated(currentIndex);
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape && popup.opened) {
            popup.close();
            event.accepted = true;
        }
    }

    Rectangle {
        id: btn
        anchors.fill: parent
        radius: 8
        color: popup.opened ? AuroraTheme.divider
               : ma.containsMouse ? AuroraTheme.divider
               : AuroraTheme.panel
        border.width: 1
        border.color: popup.opened ? AuroraTheme.accent
                      : ma.containsMouse ? AuroraTheme.borderStrong
                      : AuroraTheme.border
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
        Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp3
            anchors.rightMargin: AuroraTheme.sp2
            spacing: AuroraTheme.sp2

            Text {
                Layout.fillWidth: true
                text: root.currentText
                elide: Text.ElideRight
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: (root.currentIndex < 0) ? AuroraTheme.ink3 : AuroraTheme.ink1
            }
            FsIcon {
                name: "chevron-down"
                sizePx: 14
                color: AuroraTheme.ink3
                rotation: popup.opened ? 180 : 0
                Behavior on rotation { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }
            }
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
            onClicked: popup.opened ? popup.close() : popup.open()
        }
    }

    // Drop-down popup — Controls.Popup auto-handles Overlay parenting so it
    // shows correctly whether the FsSelect is on a plain page or inside a
    // modal FsDialog. Coordinates are local to the popup's parent (which by
    // default is this Item) — so y=root.height+4 places the dropdown
    // directly under the button without any mapToItem gymnastics.
    Popup {
        id: popup
        x: 0
        y: root.height + 4
        width: root.width
        height: Math.min(listView.contentHeight + AuroraTheme.sp2, 280)
        padding: 0
        // Keep the dropdown above any sibling Item (including modal dialogs
        // that aren't using Overlay.overlay themselves — FsDialog is plain
        // Item-based so z is what stacks the popup over its content).
        z: 9999
        modal: false
        focus: true
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        background: Rectangle {
            radius: AuroraTheme.radiusMd
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border
        }

        contentItem: ListView {
            id: listView
            implicitHeight: contentHeight
            model: root.model
            clip: true
            interactive: contentHeight > height

            delegate: Rectangle {
                required property var modelData
                required property int index
                width: ListView.view ? ListView.view.width : 0
                height: 32
                radius: AuroraTheme.radiusSm
                readonly property bool _hover: itemMa.containsMouse
                readonly property bool _selected: index === root.currentIndex
                color: _hover ? AuroraTheme.divider
                       : _selected ? AuroraTheme.accentSoft
                       : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: AuroraTheme.sp3
                    anchors.right: parent.right
                    anchors.rightMargin: AuroraTheme.sp3
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: (typeof modelData === "string") ? modelData : (modelData.label || modelData.text || "")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    color: parent._selected ? AuroraTheme.accent : AuroraTheme.ink1
                }

                MouseArea {
                    id: itemMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentIndex = index;
                        root.activated(index);
                        popup.close();
                    }
                }
            }
        }
    }
}
