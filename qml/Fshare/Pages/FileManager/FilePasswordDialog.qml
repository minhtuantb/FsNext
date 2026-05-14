// SPDX-License-Identifier: Proprietary
// FilePasswordDialog — extracted from FileManagerPage.qml under ADR 003 D13.
//
// Inputs (parent sets before open()):
//   targetLinkcodes — array of linkcodes to set / clear password on
//   hasExisting     — true when at least one file already has a password
//                     (toggles the "Remove existing password" checkbox row)
//
// Output:
//   Emits confirmed(linkcodes, password). Empty `password` means the user
//   chose to remove the existing password (when hasExisting was true).

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0

FsDialog {
    id: root

    property var    targetLinkcodes: []
    property bool   hasExisting: false
    property string newPassword: ""
    property bool   removeMode: false

    signal confirmed(var linkcodes, string password)

    title: qsTr("Set File Password")
    dialogWidth: 400

    onOpened: {
        newPassword = "";
        removeMode = false;
        Qt.callLater(() => pwdField.input.forceActiveFocus());
    }

    content: Item {
        width: 400
        height: col.implicitHeight + AuroraTheme.sp6 * 2

        ColumnLayout {
            id: col
            anchors {
                left: parent.left; right: parent.right; top: parent.top
                margins: AuroraTheme.sp6
            }
            spacing: AuroraTheme.sp3

            FsTextField {
                id: pwdField
                Layout.fillWidth: true
                label: qsTr("Password")
                placeholder: qsTr("Enter password for this file")
                echoMode: TextInput.Password
                readOnly: root.removeMode
                text: root.newPassword
                onTextChanged: root.newPassword = text
                onAccepted: if (confirmBtn.enabled) confirmBtn.clicked()
            }

            // "Remove password" toggle — only meaningful when at least one of
            // the targets already has a password set on the server side.
            // Hidden otherwise so first-time-set flows stay one-step.
            Item {
                id: removeToggle
                visible: root.hasExisting
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                // Keyboard support: opt the wrapper into the Tab chain and
                // bind Space (HTML checkbox convention) to toggle.  Without
                // this the inner Rectangle stayed mouse-only.
                activeFocusOnTab: true
                Accessible.role: Accessible.CheckBox
                Accessible.checkable: true
                Accessible.checked: root.removeMode
                Accessible.name: qsTr("Remove existing password")
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Space) {
                        root.removeMode = !root.removeMode;
                        if (root.removeMode) root.newPassword = "";
                        event.accepted = true;
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: AuroraTheme.radiusSm
                    color: removeMa.containsMouse ? AuroraTheme.bg : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    // Focus ring — same accent halo as the other components.
                    border.width: removeToggle.activeFocus ? 2 : 0
                    border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                                           AuroraTheme.accent.b, 0.55)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: AuroraTheme.sp2
                        anchors.rightMargin: AuroraTheme.sp2
                        spacing: AuroraTheme.sp3

                        Rectangle {
                            width: 18; height: 18; radius: AuroraTheme.radiusSm
                            color: root.removeMode ? AuroraTheme.accent : "transparent"
                            border.width: 1.5
                            border.color: root.removeMode ? AuroraTheme.accent : AuroraTheme.border
                            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                            Text {
                                anchors.centerIn: parent
                                visible: root.removeMode
                                text: "✓"; font.pixelSize: 12; color: "white"
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Remove existing password")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                            color: AuroraTheme.ink1
                        }
                    }
                    MouseArea {
                        id: removeMa; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.removeMode = !root.removeMode;
                            if (root.removeMode) root.newPassword = "";
                        }
                    }
                }
            }
        }
    }

    footer: Item {
        width: 400
        height: 64
        RowLayout {
            anchors { fill: parent; leftMargin: AuroraTheme.sp6; rightMargin: AuroraTheme.sp6 }
            spacing: AuroraTheme.sp2

            Item { Layout.fillWidth: true }

            FsButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: root.close() }
            FsButton {
                id: confirmBtn
                text: root.removeMode ? qsTr("Remove Password") : qsTr("Set Password")
                variant: "primary"
                enabled: root.removeMode || root.newPassword.trim().length > 0
                onClicked: {
                    const pwd = root.removeMode ? "" : root.newPassword.trim();
                    root.confirmed(root.targetLinkcodes, pwd);
                    root.close();
                }
            }
        }
    }
}
