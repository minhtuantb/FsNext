// SPDX-License-Identifier: Proprietary
// FsNavigation — Sidebar navigation with brand, nav items, user card
//
// Usage:
//   FsNavigation {
//       model: [ { icon: "⬇", label: "Tải xuống", page: "download" }, ... ]
//       currentPage: "download"
//       onNavigated: (page) => stackView.push(page)
//   }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Rectangle {
    id: root

    property var model: []
    property string currentPage: ""
    property string userName: ""
    property string userEmail: ""
    property string userInitial: ""
    property bool isVip: false

    signal navigated(string page)

    width: 220
    color: AuroraTheme.panel
    border.width: 0

    // Right border
    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: AuroraTheme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: AuroraTheme.sp3
        spacing: 2

        // ── Brand ──
        RowLayout {
            spacing: 10
            Layout.bottomMargin: AuroraTheme.sp3

            Rectangle {
                width: 32; height: 32
                radius: 8
                color: AuroraTheme.accent

                Text {
                    anchors.centerIn: parent
                    text: "☁"
                    font.pixelSize: 16
                    color: "white"
                }
            }

            Text {
                text: "<b><font color='" + AuroraTheme.accent + "'>F</font>share</b>"
                font.family: AuroraTheme.fontSans
                font.pixelSize: 17
                font.weight: Font.Bold
                color: AuroraTheme.ink1
                textFormat: Text.RichText
            }
        }

        // ── Nav items ──
        Repeater {
            model: root.model

            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 8
                color: modelData.page === root.currentPage
                    ? AuroraTheme.accentSoft
                    : (itemMouse.containsMouse ? AuroraTheme.divider : "transparent")

                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                // Active indicator bar
                Rectangle {
                    visible: modelData.page === root.currentPage
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: 18
                    radius: AuroraTheme.radiusPill
                    color: AuroraTheme.accent
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Text {
                        text: modelData.icon
                        font.pixelSize: 16
                        Layout.preferredWidth: 20
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: modelData.label
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.body.pixelSize
                        font.weight: modelData.page === root.currentPage ? Font.DemiBold : Font.Normal
                        color: modelData.page === root.currentPage ? AuroraTheme.accent : AuroraTheme.ink2
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                        Layout.fillWidth: true
                    }

                    // Count badge
                    Rectangle {
                        visible: !!modelData.count
                        width: 22; height: 18
                        radius: AuroraTheme.radiusPill
                        color: AuroraTheme.accentTint10
                        Text {
                            anchors.centerIn: parent
                            text: modelData.count || ""
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: AuroraTheme.accent
                        }
                    }
                }

                MouseArea {
                    id: itemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.navigated(modelData.page)
                }
            }
        }

        Item { Layout.fillHeight: true }

        // ── User card ──
        Rectangle {
            visible: root.userName !== ""
            Layout.fillWidth: true
            height: 56
            radius: AuroraTheme.radiusMd
            color: AuroraTheme.divider

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Rectangle {
                    width: 32; height: 32
                    radius: 16
                    color: AuroraTheme.accent

                    Text {
                        anchors.centerIn: parent
                        text: root.userInitial
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: "white"
                    }
                }

                Column {
                    Layout.fillWidth: true
                    Text {
                        text: root.userName
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink1
                    }
                    Text {
                        text: root.userEmail
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        color: AuroraTheme.ink3
                    }
                }

                Rectangle {
                    visible: root.isVip
                    width: 32; height: 18
                    radius: AuroraTheme.radiusPill
                    color: AuroraTheme.accent
                    Text {
                        anchors.centerIn: parent
                        text: "VIP"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: "white"
                    }
                }
            }
        }
    }
}
