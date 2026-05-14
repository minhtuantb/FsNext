// SPDX-License-Identifier: Proprietary
// FsSegmentedControl — Pill-style group of mutually-exclusive options
//
// Usage:
//   FsSegmentedControl {
//       options: [ {value: 0, label: "Active"}, {value: 1, label: "History"} ]
//       selectedValue: 0
//       onSelectionChanged: console.log(selectedValue)
//   }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
Item {
    id: root

    // Each option: { value: <int|string>, label: <string> }
    property var options: []
    property var selectedValue: undefined
    property int segmentWidth: 90
    property int segmentHeight: 32

    signal selectionChanged(var value)

    implicitWidth: row.implicitWidth
    implicitHeight: segmentHeight

    // The segmented control behaves as a radio group from an AT
    // perspective — one tab stop, arrow keys to switch the active option.
    Accessible.role: Accessible.PageTabList

    // ── Keyboard support (v6.0+) ────────────────────────────────────────────
    // The whole segmented control is one tab-stop (matches HTML radio
    // groups), then Left/Right (and Home/End for first/last) cycle the
    // selection.  Wraps so users can't get stuck at either end.
    activeFocusOnTab: true
    function _selectByIndex(idx) {
        if (idx < 0 || idx >= options.length) return;
        const v = options[idx].value;
        if (v === selectedValue) return;
        selectedValue = v;
        root.selectionChanged(v);
    }
    function _currentIndex() {
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === selectedValue) return i;
        return -1;
    }
    Keys.onPressed: function(event) {
        if (options.length === 0) return;
        const cur = _currentIndex();
        if (event.key === Qt.Key_Left) {
            _selectByIndex(cur <= 0 ? options.length - 1 : cur - 1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            _selectByIndex(cur < 0 || cur >= options.length - 1 ? 0 : cur + 1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Home) {
            _selectByIndex(0);
            event.accepted = true;
        } else if (event.key === Qt.Key_End) {
            _selectByIndex(options.length - 1);
            event.accepted = true;
        }
    }

    // Focus ring around the whole group — matches the FsButton pattern.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: 11
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                               AuroraTheme.accent.b, 0.55)
        visible: root.activeFocus
        z: -1
    }

    RowLayout {
        id: row
        spacing: AuroraTheme.sp1

        Repeater {
            model: root.options
            delegate: Rectangle {
                Layout.preferredWidth: root.segmentWidth
                Layout.preferredHeight: root.segmentHeight
                radius: 8

                readonly property bool selected: modelData.value === root.selectedValue

                color: selected ? AuroraTheme.accentTint10 : "transparent"
                border.width: 1
                border.color: selected ? AuroraTheme.accentTint15 : AuroraTheme.border
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                Text {
                    anchors.centerIn: parent
                    text: modelData.label
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: parent.selected ? Font.DemiBold : Font.Normal
                    color: parent.selected ? AuroraTheme.accent : AuroraTheme.ink2
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.selectedValue === modelData.value) return;
                        root.selectedValue = modelData.value;
                        root.selectionChanged(modelData.value);
                    }
                }
            }
        }
    }
}
