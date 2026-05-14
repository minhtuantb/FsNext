// SPDX-License-Identifier: Proprietary
// FsCommandPalette — ⌘/Ctrl+K command picker, Aurora-themed.
//
// Usage (in Main.qml):
//   FsCommandPalette {
//       id: cmdPalette
//       commands: [
//           { id: "go.download",  label: "Tải xuống",  hint: "Mở trang tải xuống", icon: "↓" },
//           { id: "go.upload",    label: "Tải lên",    hint: "Mở trang tải lên",   icon: "↑" },
//       ]
//       onTriggered: (id) => router.dispatch(id)
//   }
//   Shortcut { sequence: "Ctrl+K"; onActivated: cmdPalette.open() }
//
// Filtering is a simple substring match against label + hint, ranked label-
// matches before hint-matches.  No fuzzy match — substring matches the
// "what every command palette does badly" expectation users carry from
// Slack / VS Code, and skips the relevance-tuning rabbit hole.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme 1.0

Popup {
    id: root

    // Public API ─────────────────────────────────────────────
    // commands: [{ id, label, hint, icon }]
    property var commands: []
    signal triggered(string id)

    // Layout ────────────────────────────────────────────────
    width: Math.min(parent ? parent.width * 0.72 : 640, 640)
    height: 420
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
        color: AuroraTheme.panel
        radius: AuroraTheme.radiusLg
        border.color: AuroraTheme.border
        border.width: 1
    }
    // `Overlay.modal` is an attached property on Popup (capital O) — used to
    // override the dimmer that appears behind a modal popup. Lower-case
    // `overlay.modal` would error at parse time.
    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, AuroraTheme.isDark ? 0.45 : 0.25)
    }

    onOpened: {
        searchInput.text = "";
        searchInput.forceActiveFocus();
        list.currentIndex = 0;
    }

    // Filtering ─────────────────────────────────────────────
    function _filtered() {
        const q = searchInput.text.trim().toLowerCase();
        if (q === "") return commands;
        const labelMatches = [];
        const hintMatches  = [];
        for (let i = 0; i < commands.length; ++i) {
            const c = commands[i];
            const lbl = (c.label || "").toLowerCase();
            const hnt = (c.hint  || "").toLowerCase();
            if (lbl.indexOf(q) >= 0)      labelMatches.push(c);
            else if (hnt.indexOf(q) >= 0) hintMatches.push(c);
        }
        return labelMatches.concat(hintMatches);
    }
    property var _result: _filtered()

    contentItem: ColumnLayout {
        spacing: 0

        // Search row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: AuroraTheme.heightInput + AuroraTheme.sp4 * 2
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp4
                anchors.rightMargin: AuroraTheme.sp4
                anchors.topMargin: AuroraTheme.sp3
                anchors.bottomMargin: AuroraTheme.sp3
                spacing: AuroraTheme.sp2

                Text {
                    text: "⌘"
                    color: AuroraTheme.ink4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 16
                }

                TextInput {
                    id: searchInput
                    Layout.fillWidth: true
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 16
                    color: AuroraTheme.ink1
                    selectionColor: AuroraTheme.accentTint15
                    onTextChanged: {
                        root._result = root._filtered();
                        list.currentIndex = 0;
                    }
                    Keys.onUpPressed:    list.decrementCurrentIndex()
                    Keys.onDownPressed:  list.incrementCurrentIndex()
                    Keys.onReturnPressed: root._activateCurrent()
                    Keys.onEnterPressed:  root._activateCurrent()

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: searchInput.text === ""
                        text: qsTr("Gõ để tìm lệnh…")
                        color: AuroraTheme.ink4
                        font: searchInput.font
                    }
                }

                Text {
                    text: "Esc"
                    color: AuroraTheme.ink4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                }
            }

            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: AuroraTheme.divider
            }
        }

        // Result list
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            keyNavigationWraps: true
            model: root._result

            delegate: Rectangle {
                width: ListView.view ? ListView.view.width : 0
                height: 44
                color: ListView.isCurrentItem ? AuroraTheme.accentTint10 : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: list.currentIndex = index
                    onClicked: { list.currentIndex = index; root._activateCurrent(); }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp4
                    anchors.rightMargin: AuroraTheme.sp4
                    spacing: AuroraTheme.sp3

                    Text {
                        text: modelData.icon || "▸"
                        color: AuroraTheme.accent
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: modelData.label || ""
                            color: AuroraTheme.ink1
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            visible: !!modelData.hint
                            text: modelData.hint || ""
                            color: AuroraTheme.ink3
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("Không có lệnh phù hợp")
                color: AuroraTheme.ink3
                font.family: AuroraTheme.fontSans
                font.pixelSize: 12
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }

    function _activateCurrent() {
        if (list.count === 0) return;
        const item = root._result[list.currentIndex];
        if (item && item.id) {
            root.triggered(item.id);
            root.close();
        }
    }
}
