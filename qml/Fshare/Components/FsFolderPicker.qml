// SPDX-License-Identifier: Proprietary
// FsFolderPicker — Text field + trailing "Browse" button + native folder dialog.
//
// Uses the OS-native folder picker (QtQuick.Dialogs.FolderDialog) so on Windows
// the user gets the familiar Shell folder browser; on macOS, the Finder picker.
//
// The `folder` property is the authoritative source of truth (plain string
// path). Two-way binding works: set from outside to pre-fill, bind externally
// to read.
//
// Usage:
//   FsFolderPicker {
//       label: qsTr("Save folder")
//       placeholder: downloadViewModel.defaultSaveFolder
//       folder: addDialog.folderText
//       onFolderChanged: addDialog.folderText = folder
//   }

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import FsAurora.Theme 1.0
Item {
    id: root

    property string label: ""
    property string placeholder: ""
    property string folder: ""                  // current path (plain string)
    property string dialogTitle: qsTr("Choose folder")
    property string hint: ""                    // optional helper text beneath the input

    // Initial directory shown when the dialog opens. If the current `folder`
    // is non-empty it wins; otherwise fall back to `placeholder` (which is
    // typically the effective default), then the user's home directory.
    readonly property string _initialDir: folder.length > 0
        ? folder
        : (placeholder.length > 0 ? placeholder : "")

    implicitWidth: 280
    implicitHeight: col.implicitHeight

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: AuroraTheme.sp1

        // Label
        Text {
            visible: root.label !== ""
            text: root.label
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.label.pixelSize
            font.weight: Font.DemiBold
            color: AuroraTheme.ink2
            Layout.fillWidth: true
        }

        // Input row: text field + browse button
        RowLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp2

            // Input container
            Rectangle {
                id: inputBg
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: AuroraTheme.radiusMd
                color: AuroraTheme.panel
                border.width: pathInput.activeFocus ? 2 : 1.5
                border.color: pathInput.activeFocus ? AuroraTheme.accent : AuroraTheme.border

                Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                Behavior on border.width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }

                TextInput {
                    id: pathInput
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp3
                    anchors.rightMargin: AuroraTheme.sp3
                    verticalAlignment: TextInput.AlignVCenter
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    color: AuroraTheme.ink1
                    selectionColor: AuroraTheme.accentTint15
                    clip: true
                    text: root.folder
                    onTextEdited: root.folder = text

                    // Placeholder (shows effective default when empty)
                    Text {
                        visible: pathInput.text === "" && !pathInput.activeFocus
                        text: root.placeholder
                        font: pathInput.font
                        color: AuroraTheme.ink4
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideMiddle
                        width: pathInput.width
                    }
                }
            }

            // Browse button — opens native folder dialog
            Rectangle {
                id: browseBtn
                Layout.preferredHeight: 40
                Layout.preferredWidth: browseText.implicitWidth + AuroraTheme.sp4 * 2
                radius: AuroraTheme.radiusMd
                color: browseMa.containsMouse ? AuroraTheme.divider : AuroraTheme.panel
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                border.width: 1.5
                border.color: AuroraTheme.border

                Text {
                    id: browseText
                    anchors.centerIn: parent
                    text: qsTr("Browse")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink2
                }

                MouseArea {
                    id: browseMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: folderDialog.open()
                }
            }
        }

        // Hint text
        Text {
            visible: root.hint !== ""
            text: root.hint
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
            color: AuroraTheme.ink3
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }

    // Native OS folder picker.
    //
    // FolderDialog.selectedFolder is a QUrl (e.g. file:///C:/Users/.../Downloads),
    // so we convert to a plain native path before exposing it.
    FolderDialog {
        id: folderDialog
        title: root.dialogTitle
        currentFolder: root._initialDir.length > 0
            ? Qt.resolvedUrl("file:///" + root._initialDir.replace(/\\/g, "/"))
            : ""

        onAccepted: {
            const raw = selectedFolder.toString();
            // Strip "file:///" (Windows) or "file://" (*nix) prefix.
            let path = raw;
            if (path.startsWith("file:///")) {
                path = path.substring(8);  // "file:///"
            } else if (path.startsWith("file://")) {
                path = path.substring(7);
            }
            // Decode URL-escapes (%20 → space, etc.)
            path = decodeURIComponent(path);
            // Normalize to native separators on Windows (optional — Qt accepts
            // both / and \ — we keep forward slashes which render cleanly).
            root.folder = path;
        }
    }
}
