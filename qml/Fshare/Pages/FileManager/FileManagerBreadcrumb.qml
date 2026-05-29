// SPDX-License-Identifier: Proprietary
// FileManagerBreadcrumb — extracted from FileManagerPage.qml under ADR 003 D13.
//
// Pure-breadcrumb navigation (no Back / Forward arrows): a Home chip plus
// chevron-separated segments fed by `fileManagerViewModel.breadcrumbs`.
//
// We deliberately don't call fileManagerViewModel.navigateTo() inline — the
// parent page also clears its selection state when navigating away, so the
// breadcrumb just emits a signal and lets the page own that side-effect:
//
//   onNavigateRequested: (folderId, folderName) => {
//     page.selectedFolder = folderId.length > 0 ? {linkcode: folderId, name: folderName} : null;
//     page.selectedFiles = [];
//     page.selectedFileData = null;
//     fileManagerViewModel.navigateTo(folderId, folderName);
//   }
//
// Empty `folderId` means "navigate to root".

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora
import Fshare.Components 1.0

RowLayout {
    id: root

    signal navigateRequested(string folderId, string folderName)

    spacing: AuroraTheme.sp1
    visible: fileManagerViewModel !== null

    // Scrollable breadcrumb path (clip when too deep)
    Flickable {
        Layout.fillWidth: true
        Layout.preferredHeight: 28
        contentWidth: bcPathRow.implicitWidth
        clip: true
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds

        // Auto-scroll to show the last (current) segment
        onContentWidthChanged: {
            if (contentWidth > width)
                contentX = contentWidth - width;
        }

        Row {
            id: bcPathRow
            spacing: AuroraTheme.sp1
            height: 28

            // Root: Home icon.  `atRoot` is derived from currentFolderId NOT
            // breadcrumbs.length, because the breadcrumb array can be
            // transiently empty even when we're deep inside a folder (parent
            // chain still syncing).  Using the actual folder id keeps the
            // Home button clickable whenever we're somewhere other than root.
            Rectangle {
                id: bcRootChip
                readonly property bool atRoot: !fileManagerViewModel
                    || fileManagerViewModel.currentFolderId.length === 0
                width: 28; height: 28; radius: AuroraTheme.radiusSm
                color: atRoot
                    ? AuroraTheme.accentTint15
                    : (bcRootMa.containsMouse || bcRootChip.activeFocus
                        ? AuroraTheme.panel : "transparent")
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                anchors.verticalCenter: parent.verticalCenter

                // Keyboard support — Home chip is the breadcrumb's tab stop.
                // Space / Enter navigates to root.  Walking individual
                // segments by arrow key is a known TODO — for now use
                // Backspace at the page level which already pops up one
                // folder (FileManagerPage.qml Keys.onPressed handler).
                activeFocusOnTab: !atRoot
                Accessible.role: Accessible.Link
                Accessible.name: qsTr("Về trang chủ")
                Keys.onPressed: function(event) {
                    if ((event.key === Qt.Key_Space || event.key === Qt.Key_Return
                         || event.key === Qt.Key_Enter) && !bcRootChip.atRoot) {
                        root.navigateRequested("", "");
                        event.accepted = true;
                    }
                }
                border.width: bcRootChip.activeFocus ? 2 : 0
                border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                                       AuroraTheme.accent.b, 0.55)

                FsIcon {
                    anchors.centerIn: parent
                    name: "house"
                    sizePx: 16
                    color: bcRootChip.atRoot
                        ? AuroraTheme.accent
                        : (bcRootMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2)
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                }

                MouseArea {
                    id: bcRootMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: bcRootChip.atRoot ? Qt.ArrowCursor : Qt.PointingHandCursor
                    onClicked: {
                        if (!bcRootChip.atRoot) root.navigateRequested("", "");
                    }
                }

                ToolTip.visible: bcRootMa.containsMouse && !bcRootChip.atRoot
                ToolTip.text: qsTr("Về trang chủ")
                ToolTip.delay: 500
            }

            // Breadcrumb path segments
            Repeater {
                model: fileManagerViewModel ? fileManagerViewModel.breadcrumbs : []

                Row {
                    spacing: AuroraTheme.sp1
                    anchors.verticalCenter: parent.verticalCenter

                    FsIcon {
                        name: "chevron-right"; sizePx: 14; color: AuroraTheme.ink3
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        property bool isLast: index === (fileManagerViewModel
                                                          ? fileManagerViewModel.breadcrumbs.length - 1
                                                          : 0)
                        text: modelData.name || ""
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: isLast ? Font.DemiBold : Font.Normal
                        color: isLast ? AuroraTheme.ink1
                                      : (bcSegMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2)
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        anchors.verticalCenter: parent.verticalCenter

                        MouseArea {
                            id: bcSegMa; anchors.fill: parent; hoverEnabled: true
                            cursorShape: parent.isLast ? Qt.ArrowCursor : Qt.PointingHandCursor
                            onClicked: {
                                if (!parent.isLast)
                                    root.navigateRequested(modelData.id, modelData.name);
                            }
                        }
                    }
                }
            }
        }
    }
}
