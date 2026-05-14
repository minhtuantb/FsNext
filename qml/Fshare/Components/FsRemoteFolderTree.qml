// SPDX-License-Identifier: Proprietary
// FsRemoteFolderTree — Tree view for selecting a remote Fshare folder

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FsAurora.Theme 1.0
import Fshare.Components 1.0

Rectangle {
    id: root

    property string selectedFolderId: "0"
    property string selectedFolderPath: "/"

    // The tree model is a JS array of objects:
    // [{ id, name, parentId, path, hasChildren, expanded, level, loading }]
    property var treeModel: []
    property bool loading: false

    signal folderSelected(string folderId, string folderPath)
    signal expandRequested(string folderId)
    signal createFolderRequested(string parentId)

    implicitHeight: Math.min(280, contentCol.implicitHeight + AuroraTheme.sp2 * 2)
    radius: AuroraTheme.radiusMd
    color: AuroraTheme.panel
    border.width: 1
    border.color: AuroraTheme.borderStrong
    clip: true

    ScrollView {
        anchors.fill: parent
        anchors.margins: AuroraTheme.sp1
        clip: true

        ColumnLayout {
            id: contentCol
            width: parent.width
            spacing: 0

            // Loading state
            FsLoadingState {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                visible: root.loading && root.treeModel.length === 0
                message: qsTr("Đang tải thư mục...")
            }

            // Tree items
            Repeater {
                model: root.treeModel

                delegate: Rectangle {
                    id: treeItem
                    Layout.fillWidth: true
                    height: 32
                    radius: AuroraTheme.radiusSm
                    color: {
                        if (modelData.id === root.selectedFolderId) return AuroraTheme.accentTint10;
                        if (itemMa.containsMouse) return AuroraTheme.divider;
                        return "transparent";
                    }
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: AuroraTheme.sp2 + (modelData.level || 0) * 20
                        anchors.rightMargin: AuroraTheme.sp2
                        spacing: 6

                        // Expand / collapse arrow
                        FsIcon {
                            visible: modelData.hasChildren || false
                            name: (modelData.expanded || false) ? "chevron-down" : "chevron-right"
                            sizePx: 14
                            color: AuroraTheme.ink3
                            Layout.preferredWidth: 14

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.expandRequested(modelData.id)
                            }
                        }
                        // Spacer when no children arrow
                        Item {
                            visible: !(modelData.hasChildren || false)
                            Layout.preferredWidth: 14
                        }

                        FsIcon {
                            name: (modelData.expanded || false) ? "folder-open" : "folder"
                            sizePx: 16
                            color: modelData.id === root.selectedFolderId
                                ? AuroraTheme.accent : AuroraTheme.ink2
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.name || ""
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: AuroraTheme.body.pixelSize
                            font.weight: modelData.id === root.selectedFolderId ? Font.DemiBold : Font.Normal
                            color: modelData.id === root.selectedFolderId
                                ? AuroraTheme.accent : AuroraTheme.ink1
                            elide: Text.ElideRight
                        }

                        // Inline spinner when loading children
                        Rectangle {
                            visible: modelData.loading || false
                            width: 12; height: 12; radius: 6
                            color: "transparent"
                            border.width: 1.5
                            border.color: AuroraTheme.accentTint15
                            Rectangle {
                                width: 4; height: 1.5; radius: 1
                                color: AuroraTheme.accent
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top; anchors.topMargin: 1
                            }
                            RotationAnimator on rotation {
                                from: 0; to: 360; duration: 900; loops: Animation.Infinite
                                running: (modelData.loading || false) && !AuroraTheme.reduceMotion
                            }
                        }
                    }

                    MouseArea {
                        id: itemMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.selectedFolderId = modelData.id;
                            root.selectedFolderPath = modelData.path || "/";
                            root.folderSelected(modelData.id, root.selectedFolderPath);
                        }
                    }
                }
            }

            // Create folder button
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                Layout.topMargin: AuroraTheme.sp1
                visible: !root.loading

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp2
                    spacing: 6

                    FsIcon {
                        name: "plus"
                        sizePx: 14
                        color: createMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    }
                    Text {
                        text: qsTr("Tạo thư mục mới")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.caption.pixelSize
                        font.weight: Font.DemiBold
                        color: createMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    }
                }
                MouseArea {
                    id: createMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.createFolderRequested(root.selectedFolderId)
                }
            }
        }
    }
}
