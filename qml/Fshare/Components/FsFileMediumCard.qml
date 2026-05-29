// SPDX-License-Identifier: Proprietary
// FsFileMediumCard — Medium-size file card (160x170) for grid views.

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
import Fshare.Utils 1.0

Item {
    id: root

    property string fileName:      ""
    property string extension:     ""
    property string fileCategory:  ""
    property int    fileSize:      0
    property bool   isFolder:      false
    property string thumbUrl:      ""
    property bool   selected:      false
    property bool   isDownloaded:  false
    property string localPath:     ""

    signal clicked
    signal doubleClicked
    signal contextMenuRequested(real x, real y)
    signal openInExplorerClicked

    implicitWidth:  160
    implicitHeight: 170

    Rectangle {
        id: _card
        anchors.fill: parent
        radius:       AuroraTheme.radiusMd
        color:        root.selected ? AuroraTheme.accentSoft
                      : _hover.hovered ? AuroraTheme.divider
                      : AuroraTheme.panel
        border.color: root.selected ? AuroraTheme.accent
                      : _hover.hovered ? AuroraTheme.borderStrong
                      : AuroraTheme.divider
        border.width: root.selected ? 2 : 1

        Behavior on color        { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
        Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

        transform: Translate {
            y: _hover.hovered && !root.selected ? -2 : 0
            Behavior on y { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }
        }
    }

    HoverHandler { id: _hover }

    // Selection checkmark
    Rectangle {
        visible:  root.selected
        width:    20; height: 20
        radius:   10
        color:    AuroraTheme.accent
        border.color: "#fff"; border.width: 2
        anchors { top: parent.top; right: parent.right; margins: 8 }
        z: 2

        FsIcon {
            anchors.centerIn: parent
            name:   "check"
            sizePx: 12
            color:  "#fff"
        }
    }

    Column {
        anchors {
            top: parent.top; topMargin: 12
            left: parent.left; right: parent.right
            leftMargin: 10; rightMargin: 10
        }
        spacing: 8

        // Full-width row wrapping the 64x64 icon area. Column rejects
        // anchors on its direct children ("QML Column: Cannot specify
        // anchors for items inside Column"); previously _iconArea used
        // anchors.horizontalCenter on itself, which printed that warning
        // every time the page mounted. The wrapper takes Column's full
        // width and uses anchors *inside itself* to centre the icon —
        // legal because the inner Item is no longer a direct Column child.
        Item {
            width:  parent.width
            height: 64

            Item {
                id: _iconArea
                anchors.horizontalCenter: parent.horizontalCenter
                width:  64
                height: 64

                FsFileTypeIcon {
                    anchors.fill: parent
                    isFolder: root.isFolder
                    fileName: root.fileName.length > 0 ? root.fileName : ("." + root.extension)
                    sizePx:   64
                }

                // Downloaded badge — nested here so `parent` refers to _iconArea.
                // Previously declared as a sibling of Column and anchored to
                // `_iconArea`, which is not a parent/sibling; Qt printed
                // "Cannot anchor to an item that isn't a parent or sibling" for
                // every card and rendered a stray green disc at (0,0) of the
                // card — the mysterious empty box users saw flash on page load.
                Rectangle {
                    visible:  root.isDownloaded && !root.selected
                    width:    18; height: 18
                    radius:   9
                    color:    AuroraTheme.success
                    border.color: AuroraTheme.panel; border.width: 2
                    anchors { bottom: parent.bottom; right: parent.right; margins: -2 }
                    z: 2

                    FsIcon {
                        anchors.centerIn: parent
                        name:   "check"
                        sizePx: 10
                        color:  "#fff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openInExplorerClicked()
                    }
                }
            }
        }

        Text {
            width:          parent.width
            text:           root.fileName
            color:          AuroraTheme.ink1
            font.family:    AuroraTheme.fontSans
            font.pixelSize: 12
            font.weight:    Font.Medium
            horizontalAlignment: Text.AlignHCenter
            elide:          Text.ElideMiddle
            maximumLineCount: 2
            wrapMode:       Text.WrapAtWordBoundaryOrAnywhere
            lineHeight:     1.2
        }

        Text {
            width:          parent.width
            text:           root.isFolder ? "" : FsFormat.bytes(root.fileSize, true)
            color:          AuroraTheme.ink3
            font.family:    AuroraTheme.fontMono
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                const p = root.mapToGlobal(mouse.x, mouse.y);
                root.contextMenuRequested(p.x, p.y);
            } else {
                root.clicked();
            }
        }
        onDoubleClicked: root.doubleClicked()
    }
}
