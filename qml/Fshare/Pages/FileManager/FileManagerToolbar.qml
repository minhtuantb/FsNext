// SPDX-License-Identifier: Proprietary
// FileManagerToolbar — extracted from FileManagerPage.qml under ADR 003 D13.
//
// A search-input + actions row.  Uses `fileManagerViewModel` from context
// (same way the inline block did), so the only state we have to wire through
// is the page-level `searchQuery` text — exposed as a two-way alias.
//
// Outputs:
//   newFolderRequested() — wired to the page-owned newFolderDialog.

import QtQuick
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

RowLayout {
    id: root

    property string searchQuery: ""
    signal newFolderRequested()

    spacing: AuroraTheme.sp4

    // Exposed so the parent page can route Ctrl+F here without having to
    // reach into a nested id.  forceActiveFocus + selectAll mirror what
    // browsers / Explorer / VS Code do — Ctrl+F either focuses the box or,
    // when it already has focus, re-selects the existing query for replace.
    function focusSearch() {
        searchInput.forceActiveFocus();
        searchInput.selectAll();
    }

    // Search — takes the width freed up by removing the title
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        radius: AuroraTheme.radiusMd
        color: AuroraTheme.panel
        border.width: 1.5
        border.color: searchInput.activeFocus ? AuroraTheme.accent : AuroraTheme.border
        Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp3
            anchors.rightMargin: AuroraTheme.sp3
            spacing: AuroraTheme.sp2

            FsIcon {
                name: "search"
                sizePx: 16
                color: AuroraTheme.ink3
            }
            TextInput {
                id: searchInput
                Layout.fillWidth: true
                verticalAlignment: TextInput.AlignVCenter
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                color: AuroraTheme.ink1
                text: root.searchQuery
                onTextChanged: root.searchQuery = text
                onAccepted: {
                    // Empty submit → clearSearch — saves the user a separate
                    // "x" affordance (still works via clearSearch wired on the VM).
                    if (text.length > 0 && fileManagerViewModel)
                        fileManagerViewModel.searchFiles(text);
                    else if (fileManagerViewModel)
                        fileManagerViewModel.clearSearch();
                }

                Text {
                    visible: parent.text.length === 0
                    text: qsTr("Search files...")
                    color: AuroraTheme.ink4
                    font: parent.font
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    FsButton {
        text: qsTr("New Folder")
        icon: "+"
        variant: "secondary"
        size: "default"
        onClicked: root.newFolderRequested()
    }

    FsButton {
        text: qsTr("Refresh")
        variant: "ghost"
        size: "default"
        onClicked: if (fileManagerViewModel) fileManagerViewModel.refreshCurrentFolder()
    }

    // View mode toggle — list / medium grid.  Routes directly through the VM
    // since `viewMode` is a Q_PROPERTY with persistence wired on the C++ side.
    Row {
        spacing: 2
        Rectangle {
            width: 32; height: 32; radius: AuroraTheme.radiusSm
            color: fileManagerViewModel && fileManagerViewModel.viewMode === "list"
                ? AuroraTheme.accentTint15
                : (listModeMa.containsMouse ? AuroraTheme.bg : "transparent")
            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
            FsIcon { anchors.centerIn: parent; name: "list"; sizePx: 16
                color: fileManagerViewModel && fileManagerViewModel.viewMode === "list"
                    ? AuroraTheme.accent : AuroraTheme.ink2 }
            MouseArea {
                id: listModeMa; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (fileManagerViewModel) fileManagerViewModel.viewMode = "list"
            }
        }
        Rectangle {
            width: 32; height: 32; radius: AuroraTheme.radiusSm
            color: fileManagerViewModel && fileManagerViewModel.viewMode === "medium"
                ? AuroraTheme.accentTint15
                : (medModeMa.containsMouse ? AuroraTheme.bg : "transparent")
            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
            FsIcon { anchors.centerIn: parent; name: "grid"; sizePx: 16
                color: fileManagerViewModel && fileManagerViewModel.viewMode === "medium"
                    ? AuroraTheme.accent : AuroraTheme.ink2 }
            MouseArea {
                id: medModeMa; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (fileManagerViewModel) fileManagerViewModel.viewMode = "medium"
            }
        }
    }
}
