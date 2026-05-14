// SPDX-License-Identifier: Proprietary
// FsSelect — Themed dropdown select built on QtQuick primitives.
//
// Usage:
//   FsSelect {
//       model: ["Option A", "Option B", "Option C"]
//       currentIndex: 0
//       onActivated: console.log(currentIndex, currentText)
//   }

import QtQuick
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

    // ── Keyboard support (v6.0+) ────────────────────────────────────────────
    // Same opt-in dance as FsButton — Item-based controls need explicit
    // tab-stop opt-in.  Space/Enter open the popup; arrow keys cycle when
    // the popup is closed (matches native ComboBox).  Up/Down inside the
    // open popup is handled by ListView's own key handler below.
    activeFocusOnTab: enabled
    Keys.onPressed: function(event) {
        if (!enabled) return;
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter) {
            popup.visible ? popup.close() : popup.open();
            event.accepted = true;
        } else if (!popup.visible && event.key === Qt.Key_Down) {
            if (currentIndex < (model ? model.length - 1 : 0)) {
                currentIndex += 1;
                root.activated(currentIndex);
            }
            event.accepted = true;
        } else if (!popup.visible && event.key === Qt.Key_Up) {
            if (currentIndex > 0) {
                currentIndex -= 1;
                root.activated(currentIndex);
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape && popup.visible) {
            popup.close();
            event.accepted = true;
        }
    }

    Rectangle {
        id: btn
        anchors.fill: parent
        radius: 8
        color: popup.visible ? AuroraTheme.divider
               : ma.containsMouse ? AuroraTheme.divider
               : AuroraTheme.panel
        border.width: 1
        border.color: popup.visible ? AuroraTheme.accent
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
                rotation: popup.visible ? 180 : 0
                Behavior on rotation { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }
            }
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
            onClicked: popup.visible ? popup.close() : popup.open()
        }
    }

    Rectangle {
        id: popup
        function open() { visible = true; }
        function close() { visible = false; }

        visible: false
        parent: root.Window.contentItem ?? root
        x: {
            const pt = root.mapToItem(parent, 0, root.height + 4);
            return pt.x;
        }
        y: {
            const pt = root.mapToItem(parent, 0, root.height + 4);
            return pt.y;
        }
        width: root.width
        height: Math.min(listView.contentHeight + AuroraTheme.sp2, 280)
        radius: AuroraTheme.radiusMd
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border
        z: 9999

        MouseArea {
            parent: popup.parent
            anchors.fill: parent
            enabled: popup.visible
            z: popup.z - 1
            onPressed: popup.close()
        }

        ListView {
            id: listView
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp1
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
