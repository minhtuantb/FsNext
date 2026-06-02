// SPDX-License-Identifier: Proprietary
// FsCallout — tinted info/warn/danger/success/accent box with icon + text.
// Aurora-native. Used across the Vault module for guidance & warnings.
//
// Usage:
//   FsCallout { tone: "warn"; title: "Đọc kỹ"; body: "Mất passphrase = mất dữ liệu." }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Rectangle {
    id: root

    property string tone: "info"          // info | warn | danger | success | accent
    property string title: ""
    property string body: ""
    property string iconName: ""          // override the default tone icon
    property bool   compact: false

    readonly property color _fg:
        tone === "warn"    ? AuroraTheme.warn :
        tone === "danger"  ? AuroraTheme.danger :
        tone === "success" ? AuroraTheme.success :
        tone === "accent"  ? AuroraTheme.accent : AuroraTheme.info
    readonly property color _bg:
        tone === "warn"    ? AuroraTheme.warnSoft :
        tone === "danger"  ? AuroraTheme.dangerSoft :
        tone === "success" ? AuroraTheme.successSoft :
        tone === "accent"  ? AuroraTheme.accentSoft : AuroraTheme.infoSoft
    readonly property string _defIcon:
        (tone === "danger" || tone === "warn") ? "alert-triangle" :
        tone === "success" ? "shield-check" : "info"
    readonly property int _pad: compact ? 11 : 14

    color: _bg
    radius: AuroraTheme.radiusLg
    border.width: 1
    border.color: Qt.rgba(_fg.r, _fg.g, _fg.b, 0.18)
    implicitHeight: rowL.implicitHeight + _pad * 2
    implicitWidth: 320

    RowLayout {
        id: rowL
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: root._pad
        spacing: 11

        FsIcon {
            name: root.iconName !== "" ? root.iconName : root._defIcon
            sizePx: root.compact ? 16 : 18
            color: root._fg
            Layout.alignment: Qt.AlignTop
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                visible: root.title !== ""
                text: root.title
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: Font.Bold
                color: AuroraTheme.ink1
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Text {
                visible: root.body !== ""
                text: root.body
                font.family: AuroraTheme.fontSans
                font.pixelSize: root.compact ? 12 : 13
                color: AuroraTheme.ink2
                lineHeight: 1.4
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
