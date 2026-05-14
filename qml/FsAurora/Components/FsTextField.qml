// SPDX-License-Identifier: Proprietary
// FsTextField (Aurora) — label + input + 3px orange focus ring.

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme

Item {
    id: root

    property string label: ""
    property string placeholder: ""
    property string text: ""
    property string error: ""
    property string hint: ""
    property int echoMode: TextInput.Normal
    property bool readOnly: false
    property alias input: ti

    signal accepted()

    implicitWidth: 280
    implicitHeight: col.implicitHeight

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: AuroraTheme.sp1

        Text {
            visible: root.label !== ""
            text: root.label
            font.family: AuroraTheme.fontSans
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: AuroraTheme.ink2
            Layout.fillWidth: true
        }

        Rectangle {
            id: box
            Layout.fillWidth: true
            height: AuroraTheme.heightInput
            radius: AuroraTheme.radiusMd
            color: AuroraTheme.bg
            border.width: 1.5
            border.color: root.error !== "" ? AuroraTheme.danger
                        : ti.activeFocus   ? AuroraTheme.accent
                        : AuroraTheme.border
            Behavior on border.color { enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast } }

            // Focus / error glow — sits just outside the border
            Rectangle {
                visible: ti.activeFocus || root.error !== ""
                anchors.fill: parent
                anchors.margins: -3
                radius: parent.radius + 3
                color: "transparent"
                border.width: 3
                border.color: root.error !== "" ? Qt.rgba(0.835, 0.188, 0.188, 0.22)
                                                : AuroraTheme.accentGlow
                z: -1
            }

            TextInput {
                id: ti
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp3
                anchors.rightMargin: AuroraTheme.sp3
                verticalAlignment: TextInput.AlignVCenter
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                color: AuroraTheme.ink1
                selectionColor: AuroraTheme.accentTint15
                echoMode: root.echoMode
                readOnly: root.readOnly
                clip: true
                text: root.text
                onTextEdited: root.text = text
                onAccepted: root.accepted()

                Text {
                    visible: ti.text === "" && !ti.activeFocus
                    text: root.placeholder
                    font: ti.font
                    color: AuroraTheme.ink4
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        Text {
            visible: root.error !== "" || root.hint !== ""
            text: root.error !== "" ? ("⚠ " + root.error) : root.hint
            font.family: AuroraTheme.fontSans
            font.pixelSize: 11
            font.weight: root.error !== "" ? Font.DemiBold : Font.Normal
            color: root.error !== "" ? AuroraTheme.danger : AuroraTheme.ink3
            Layout.fillWidth: true
        }
    }
}
