// SPDX-License-Identifier: Proprietary
// FsContextMenu — Floating menu with action items
// Position via popup(x, y) or anchored to a target item.
//
// Usage:
//   FsContextMenu {
//       id: menu
//       actions: [
//           { label: "Rename",   icon: "✎", onTriggered: () => doRename() },
//           { label: "Delete",   icon: "✕", danger: true, onTriggered: () => doDelete() },
//           { separator: true },
//           { label: "Properties", icon: "ⓘ", onTriggered: () => showInfo() }
//       ]
//   }
//   ...
//   onClicked: menu.popup(mouseX, mouseY)

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    // Each action: { label, icon (optional), danger (bool, optional),
    //                separator (bool, optional), onTriggered (function) }
    property var actions: []
    property int menuWidth: 200

    visible: false
    z: 9999

    function popup(globalX, globalY) {
        menu.x = globalX;
        menu.y = globalY;
        // Clamp to parent bounds if needed
        if (parent) {
            if (menu.x + menu.width > parent.width) menu.x = parent.width - menu.width - 4;
            if (menu.y + menu.height > parent.height) menu.y = parent.height - menu.height - 4;
            if (menu.x < 0) menu.x = 4;
            if (menu.y < 0) menu.y = 4;
        }
        root.visible = true;
    }

    function close() { root.visible = false; }

    // Click-outside to dismiss
    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
        onPressed: root.close()
    }

    Rectangle {
        id: menu
        width: root.menuWidth
        height: contentCol.implicitHeight + AuroraTheme.sp2
        radius: AuroraTheme.radiusMd
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border

        // Soft shadow approximation
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            z: -1
            border.width: 4
            border.color: AuroraTheme.divider  // ~5% subtle outline (theme-aware)
        }

        ColumnLayout {
            id: contentCol
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp1
            spacing: 0

            Repeater {
                model: root.actions
                delegate: Loader {
                    Layout.fillWidth: true
                    sourceComponent: modelData.separator ? sepComp : itemComp

                    Component {
                        id: sepComp
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: 9
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width - AuroraTheme.sp2
                                height: 1
                                color: AuroraTheme.divider
                            }
                        }
                    }

                    Component {
                        id: itemComp
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 32
                            radius: AuroraTheme.radiusSm
                            readonly property bool isDanger: modelData.danger === true
                            color: itemMa.containsMouse
                                ? (isDanger ? AuroraTheme.accentTint10 : AuroraTheme.divider)
                                : "transparent"
                            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: AuroraTheme.sp2
                                anchors.rightMargin: AuroraTheme.sp2
                                spacing: AuroraTheme.sp2

                                Text {
                                    visible: modelData.icon !== undefined
                                    text: modelData.icon || ""
                                    font.pixelSize: 13
                                    color: parent.parent.isDanger
                                        ? AuroraTheme.accent : AuroraTheme.ink2
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.label || ""
                                    font.family: AuroraTheme.fontSans
                                    font.pixelSize: AuroraTheme.body.pixelSize
                                    color: parent.parent.isDanger
                                        ? AuroraTheme.accent : AuroraTheme.ink1
                                }
                                // Optional shortcut hint
                                Text {
                                    visible: modelData.shortcut !== undefined
                                    text: modelData.shortcut || ""
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: AuroraTheme.caption.pixelSize
                                    color: AuroraTheme.ink3
                                }
                            }

                            MouseArea {
                                id: itemMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.close();
                                    if (modelData.onTriggered) modelData.onTriggered();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
