// SPDX-License-Identifier: Proprietary
// FsStrengthMeter — 4-bar passphrase strength indicator (Aurora-native).
// Mirrors the C++/JS scorePass rule (len>=12, letters+digits, symbol, len>=16).
//
// Usage:
//   FsStrengthMeter { password: pwField.text }

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Item {
    id: root

    property string password: ""
    property string strengthPrefix: qsTr("Độ mạnh")
    // Labels indexed by score 0..4 (0/1 share "Yếu").
    property var labels: [qsTr("Yếu"), qsTr("Yếu"), qsTr("Trung bình"), qsTr("Khá"), qsTr("Mạnh")]

    readonly property int score: _score(password)

    function _score(p) {
        p = p || "";
        var len = p.length >= 12;
        var alpha = /[a-zA-Z]/.test(p);
        var num = /[0-9]/.test(p);
        var sym = /[^a-zA-Z0-9]/.test(p);
        var s = 0;
        if (len) s++;
        if (alpha && num) s++;
        if (sym) s++;
        if (p.length >= 16 && alpha && num && sym) s++;
        return Math.min(s, 4);
    }

    function _color(s) {
        return s <= 1 ? AuroraTheme.danger
             : s === 2 ? AuroraTheme.warn
             : s === 3 ? AuroraTheme.info
             : AuroraTheme.success;
    }

    readonly property int _active: password === "" ? 0 : score

    implicitWidth: 200
    implicitHeight: col.implicitHeight

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 6

        RowLayout {
            spacing: 5
            Layout.fillWidth: true
            Repeater {
                model: 4
                delegate: Rectangle {
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredHeight: 5
                    radius: AuroraTheme.radiusPill
                    color: index < root._active ? root._color(root.score) : AuroraTheme.border
                    Behavior on color {
                        enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast }
                    }
                }
            }
        }

        Text {
            visible: root.password !== ""
            text: root.strengthPrefix + ": " + root.labels[root._active]
            font.family: AuroraTheme.fontSans
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: root._color(root._active)
        }
    }
}
