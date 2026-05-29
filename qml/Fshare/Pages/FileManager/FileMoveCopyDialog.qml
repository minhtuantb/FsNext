// SPDX-License-Identifier: Proprietary
// FileMoveCopyDialog — extracted from FileManagerPage.qml under ADR 003 D13.
//
// Inputs (parent sets before open()):
//   mode             — "move" or "copy"; toggles the dialog title and CTA
//   pendingLinkcodes — array of linkcodes that will be moved/copied
//   folderTreeModel  — QAbstractListModel feeding the destination tree (the
//                      caller passes fileManagerViewModel.folderTreeModel here
//                      so this component stays VM-agnostic)
//
// Output:
//   Emits confirmed(mode, linkcodes, destinationLinkcode) when the user
//   clicks the primary action.  Empty destinationLinkcode means root.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0

FsDialog {
    id: root

    property string mode: "move"          // "move" | "copy"
    property var    pendingLinkcodes: []
    property var    folderTreeModel: null

    // Internal selection — exposed so a parent can pre-set if needed.
    property string selectedDestId: ""    // linkcode of chosen destination ("" = root)
    property string selectedDestName: qsTr("My Files")

    signal confirmed(string mode, var linkcodes, string destinationLinkcode)

    title: mode === "move" ? qsTr("Move to...") : qsTr("Copy to...")
    dialogWidth: 440

    onOpened: {
        // Reset to root selection each time so a previous Move doesn't pre-
        // select a now-irrelevant folder when the user re-opens for a Copy.
        selectedDestId = "";
        selectedDestName = qsTr("My Files");
        // Hand focus to the folder list so the user can arrow-key down and
        // Enter to select without ever touching the mouse.
        Qt.callLater(() => destFolderList.forceActiveFocus());
    }

    content: Item {
        width: 440
        height: 360

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp4
            spacing: AuroraTheme.sp2

            // Selected destination chip
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: AuroraTheme.radiusSm
                color: AuroraTheme.accentTint10
                border.width: 1
                border.color: AuroraTheme.accent

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp3
                    anchors.rightMargin: AuroraTheme.sp3
                    spacing: AuroraTheme.sp2
                    FsIcon { name: "folder"; sizePx: 16; color: AuroraTheme.accent }
                    Text {
                        Layout.fillWidth: true
                        text: root.selectedDestName
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: AuroraTheme.accent
                        elide: Text.ElideMiddle
                    }
                }
            }

            // Folder list — tree shown flat with `depth` pixels of left padding
            // per level.  ListView (not TreeView) because the model is a flat
            // QAbstractListModel emitted by FileCacheService with depth
            // pre-computed; we get free virtualisation that way.
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: AuroraTheme.radiusMd
                color: AuroraTheme.bg
                border.width: 1
                border.color: AuroraTheme.border
                clip: true

                ListView {
                    id: destFolderList
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp1
                    clip: true
                    spacing: 0
                    model: root.folderTreeModel

                    // Root entry — always present so the user can move to "/".
                    header: Rectangle {
                        width: destFolderList.width
                        height: 36
                        radius: AuroraTheme.radiusSm
                        color: root.selectedDestId === ""
                            ? AuroraTheme.accentTint15
                            : (destRootMa.containsMouse ? AuroraTheme.panel : "transparent")
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: AuroraTheme.sp3
                            anchors.rightMargin: AuroraTheme.sp3
                            spacing: AuroraTheme.sp2
                            FsIcon {
                                name: "house"; sizePx: 15
                                color: root.selectedDestId === "" ? AuroraTheme.accent : AuroraTheme.ink2
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("My Files (root)")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: root.selectedDestId === "" ? Font.DemiBold : Font.Normal
                                color: root.selectedDestId === "" ? AuroraTheme.accent : AuroraTheme.ink1
                            }
                            FsIcon {
                                visible: root.selectedDestId === ""
                                name: "check"; sizePx: 14; color: AuroraTheme.accent
                            }
                        }
                        MouseArea {
                            id: destRootMa; anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.selectedDestId = "";
                                root.selectedDestName = qsTr("My Files");
                            }
                        }
                    }

                    delegate: Rectangle {
                        required property string linkcode
                        required property string name
                        required property int depth

                        width: destFolderList.width
                        height: 36
                        radius: AuroraTheme.radiusSm
                        color: root.selectedDestId === linkcode
                            ? AuroraTheme.accentTint15
                            : (destItemMa.containsMouse ? AuroraTheme.panel : "transparent")
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: AuroraTheme.sp3 + depth * AuroraTheme.sp4
                            anchors.rightMargin: AuroraTheme.sp3
                            spacing: AuroraTheme.sp2
                            FsIcon {
                                name: "folder"; sizePx: 15
                                color: root.selectedDestId === linkcode
                                    ? AuroraTheme.accent : AuroraTheme.ink2
                            }
                            Text {
                                Layout.fillWidth: true
                                text: name
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: root.selectedDestId === linkcode
                                    ? Font.DemiBold : Font.Normal
                                color: root.selectedDestId === linkcode
                                    ? AuroraTheme.accent : AuroraTheme.ink1
                                elide: Text.ElideRight
                            }
                            FsIcon {
                                visible: root.selectedDestId === linkcode
                                name: "check"; sizePx: 14; color: AuroraTheme.accent
                            }
                        }
                        MouseArea {
                            id: destItemMa; anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.selectedDestId = linkcode;
                                root.selectedDestName = name;
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
            }
        }
    }

    footer: Item {
        width: 440
        height: 64
        RowLayout {
            anchors { fill: parent; leftMargin: AuroraTheme.sp6; rightMargin: AuroraTheme.sp6 }
            spacing: AuroraTheme.sp2

            Text {
                text: qsTr("%1 item(s)").arg(root.pendingLinkcodes.length)
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                color: AuroraTheme.ink3
            }
            Item { Layout.fillWidth: true }

            FsButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: root.close() }
            FsButton {
                text: root.mode === "move" ? qsTr("Move Here") : qsTr("Copy Here")
                variant: "primary"
                enabled: root.pendingLinkcodes.length > 0
                onClicked: {
                    root.confirmed(root.mode, root.pendingLinkcodes, root.selectedDestId);
                    root.close();
                }
            }
        }
    }
}
