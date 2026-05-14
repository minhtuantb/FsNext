// SPDX-License-Identifier: Proprietary
// FsInlineAlert — In-flow banner for info/success/warning/error states.
//
// Usage:
//   FsInlineAlert { variant: "warning"; title: "Chú ý"; message: "..." }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    property string variant: "info"  // info | success | warning | error
    property string title: ""
    property string message: ""
    property bool closable: false

    signal closed()

    implicitWidth: 400
    implicitHeight: row.implicitHeight + AuroraTheme.sp4 * 2

    readonly property var _palette: {
        switch (variant) {
        case "success": return { bg: AuroraTheme.successSoft, border: Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25), fg: AuroraTheme.success, icon: "check" }
        case "warning": return { bg: AuroraTheme.warnSoft, border: Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25), fg: AuroraTheme.warn, icon: "info" }
        case "error":   return { bg: AuroraTheme.accentTint10, border: AuroraTheme.accentTint15,   fg: AuroraTheme.accent,       icon: "info" }
        default:        return { bg: AuroraTheme.infoSoft,  border: Qt.rgba(AuroraTheme.info.r, AuroraTheme.info.g, AuroraTheme.info.b, 0.20),  fg: AuroraTheme.info,  icon: "info" }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: AuroraTheme.radiusMd
        color: root._palette.bg
        border.color: root._palette.border
        border.width: 1
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: AuroraTheme.sp4
        spacing: AuroraTheme.sp3

        FsIcon {
            Layout.alignment: Qt.AlignTop
            name: root._palette.icon
            sizePx: 18
            color: root._palette.fg
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp1

            Text {
                visible: root.title.length > 0
                text: root.title
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.bodyStrong.pixelSize
                font.weight: Font.DemiBold
                color: root._palette.fg
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            Text {
                visible: root.message.length > 0
                text: root.message
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink2
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }

        Item {
            visible: root.closable
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            Layout.alignment: Qt.AlignTop
            FsIcon {
                anchors.centerIn: parent
                name: "x"
                sizePx: 12
                color: AuroraTheme.ink3
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.closed()
            }
        }
    }
}
