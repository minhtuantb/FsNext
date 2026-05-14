// SPDX-License-Identifier: Proprietary
// FsTextField — Input with label, focus glow, error state (Aurora-native)
//
// Usage:
//   FsTextField { label: "Email"; placeholder: "email@fshare.vn" }
//   FsTextField { label: "Mật khẩu"; echoMode: TextInput.Password; error: "Sai mật khẩu" }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Item {
    id: root

    property string label: ""
    property string placeholder: ""
    property string text: ""
    property string error: ""
    property string hint: ""
    property int echoMode: TextInput.Normal
    property bool readOnly: false
    property alias input: textInput

    signal accepted()

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

        // Input container — Aurora 40px height
        Rectangle {
            id: inputBg
            Layout.fillWidth: true
            height: AuroraTheme.heightInput
            radius: AuroraTheme.radiusMd
            color: AuroraTheme.panel
            border.width: textInput.activeFocus ? 2 : 1
            border.color: root.error !== "" ? AuroraTheme.danger
                        : textInput.activeFocus ? AuroraTheme.accent
                        : AuroraTheme.border

            Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
            Behavior on border.width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durFast } }

            // Focus glow — accent ring at 3px offset (handoff a11y spec)
            Rectangle {
                visible: textInput.activeFocus && root.error === ""
                anchors.fill: parent
                anchors.margins: -3
                radius: parent.radius + 3
                color: "transparent"
                border.width: 3
                border.color: AuroraTheme.accentGlow
            }

            // Error glow
            Rectangle {
                visible: root.error !== ""
                anchors.fill: parent
                anchors.margins: -3
                radius: parent.radius + 3
                color: "transparent"
                border.width: 3
                border.color: Qt.rgba(AuroraTheme.danger.r, AuroraTheme.danger.g, AuroraTheme.danger.b, 0.22)
            }

            TextInput {
                id: textInput
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp3
                anchors.rightMargin: AuroraTheme.sp3
                verticalAlignment: TextInput.AlignVCenter
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink1
                selectionColor: AuroraTheme.accentTint15
                echoMode: root.echoMode
                readOnly: root.readOnly
                clip: true
                text: root.text
                onTextEdited: root.text = text
                onAccepted: root.accepted()

                // Placeholder
                Text {
                    visible: textInput.text === "" && !textInput.activeFocus
                    text: root.placeholder
                    font: textInput.font
                    color: AuroraTheme.ink4
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Error / Hint text
        Text {
            visible: root.error !== "" || root.hint !== ""
            text: root.error !== "" ? "⚠ " + root.error : root.hint
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
            font.weight: root.error !== "" ? Font.DemiBold : Font.Normal
            color: root.error !== "" ? AuroraTheme.danger : AuroraTheme.ink3
            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
            Layout.fillWidth: true
        }
    }

    // Shake animation on error — gated by reduceMotion.
    SequentialAnimation {
        id: shakeAnim
        NumberAnimation { target: root; property: "x"; to: root.x - 4; duration: 50 }
        NumberAnimation { target: root; property: "x"; to: root.x + 4; duration: 50 }
        NumberAnimation { target: root; property: "x"; to: root.x - 4; duration: 50 }
        NumberAnimation { target: root; property: "x"; to: root.x;     duration: 50 }
    }

    onErrorChanged: if (error !== "" && !AuroraTheme.reduceMotion) shakeAnim.start()
}
