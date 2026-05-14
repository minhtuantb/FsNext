// SPDX-License-Identifier: Proprietary
// FileRenameDialog — extracted from FileManagerPage.qml to keep that file
// under 800 LOC (ADR 003 D13).
//
// Inputs (set by parent before calling open()):
//   targetLinkcode  — Fshare linkcode of the file/folder to rename
//   originalName    — current display name; pre-fills the input
//   isFolder        — toggles dialog title between "Rename File" / "Folder"
//
// Output:
//   Emits confirmed(newName) when the user clicks Rename. Parent is
//   responsible for the actual API call (fileManagerViewModel.renameFile).
//
// Validation matches the server contract: blocks Windows-reserved chars and
// Fshare's extra blocked set (comma, exclamation, etc), enforces 100-char
// limit, no-op when unchanged.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0

FsDialog {
    id: root

    property string targetLinkcode: ""
    property string originalName: ""
    property bool   isFolder: false

    // Internal state — exposed as readonly so a parent can disable its own
    // "OK" button if it lifts validation. Most callers don't need them.
    property string newName: ""
    readonly property bool _hasInvalidChars:
        /[\/\\:*?"<>|,!'`—#$%^&~=+@]/.test(newName)
    readonly property bool _tooLong:   newName.length > 100
    readonly property bool _isEmpty:   newName.trim().length === 0
    readonly property bool _unchanged: newName.trim() === originalName

    signal confirmed(string newName)

    title: isFolder ? qsTr("Rename Folder") : qsTr("Rename File")
    dialogWidth: 420

    onOpened: {
        newName = originalName;
        // Select-all so the user can immediately type a replacement name —
        // matches Explorer / Finder rename UX.  Defer until after focus
        // lands or selectAll() runs on the unfocused TextInput and the
        // selection visually doesn't render until first interaction.
        Qt.callLater(() => {
            nameField.input.forceActiveFocus();
            nameField.input.selectAll();
        });
    }

    content: Item {
        width: 420
        height: col.implicitHeight + AuroraTheme.sp6 * 2

        ColumnLayout {
            id: col
            anchors {
                left: parent.left; right: parent.right; top: parent.top
                margins: AuroraTheme.sp6
            }
            spacing: AuroraTheme.sp2

            FsTextField {
                id: nameField
                Layout.fillWidth: true
                label: root.isFolder ? qsTr("Folder name") : qsTr("File name")
                placeholder: root.originalName
                text: root.newName
                onTextChanged: root.newName = text
                error: root._hasInvalidChars
                    ? qsTr("Name cannot contain: / \\ : * ? \" < > | , ! ' ` — # $ % ^ & ~ = + @")
                    : root._tooLong
                    ? qsTr("Name must be 100 characters or fewer")
                    : ""
                onAccepted: if (saveBtn.enabled) saveBtn.clicked()
            }

            // Character counter — only visible near the limit so it doesn't
            // distract during normal short renames.
            Text {
                visible: root.newName.length >= 80
                text: qsTr("%1 / 100").arg(root.newName.length)
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                color: root._tooLong              ? AuroraTheme.accent
                     : root.newName.length > 90   ? AuroraTheme.warn
                     :                               AuroraTheme.ink3
            }
        }
    }

    footer: Item {
        width: 420
        height: 64
        RowLayout {
            anchors {
                fill: parent
                leftMargin: AuroraTheme.sp6; rightMargin: AuroraTheme.sp6
            }
            spacing: AuroraTheme.sp2

            Item { Layout.fillWidth: true }

            FsButton {
                text: qsTr("Cancel")
                variant: "ghost"
                onClicked: root.close()
            }
            FsButton {
                id: saveBtn
                text: qsTr("Rename")
                variant: "primary"
                enabled: !root._hasInvalidChars && !root._tooLong &&
                         !root._isEmpty && !root._unchanged
                onClicked: {
                    root.confirmed(root.newName.trim());
                    root.close();
                }
            }
        }
    }
}
