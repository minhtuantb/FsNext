// SPDX-License-Identifier: Proprietary
// WatchFolderSettingsDialog — Edit settings for an existing watched folder

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FsAurora.Theme 1.0
import Fshare.Components 1.0

FsDialog {
    id: root
    title: qsTr("Cài đặt thư mục đồng bộ")
    dialogWidth: 480

    property string watchId: ""
    property string localPath: ""
    property string remotePath: ""
    property bool watchSubfolders: true
    property bool deleteAfterUpload: false
    property string ignorePatterns: ""
    property int speedLimitMode: 0        // 0 = unlimited, 1 = custom
    property int speedLimitKBps: 5120     // 5 MB/s default

    signal saved(string watchId, bool watchSubfolders, bool deleteAfterUpload,
                 string ignorePatterns, int speedLimitKBps)

    function open() {
        visible = true;
        // Land on the ignore-patterns text field — it's the only free-form
        // text input in the form, so typing is the likely next action.
        // Tab from there flows through the radios / spinner / footer
        // buttons in source order.
        Qt.callLater(() => ignoreField.forceActiveFocus());
    }

    function close() {
        visible = false;
    }

    content: Item {
        width: 480
        height: settingsCol.implicitHeight + AuroraTheme.sp6 * 2

        ColumnLayout {
            id: settingsCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: AuroraTheme.sp6
            spacing: AuroraTheme.sp4

            // Path summary
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                FsIcon { name: "folder"; sizePx: 18; color: AuroraTheme.accent }
                Text {
                    Layout.fillWidth: true
                    text: root.localPath + "  \u2192  " + root.remotePath
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                    elide: Text.ElideMiddle
                }
            }

            // Settings card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: optsCol.implicitHeight + AuroraTheme.sp4 * 2
                radius: AuroraTheme.radiusMd
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border

                ColumnLayout {
                    id: optsCol
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: AuroraTheme.sp4
                    spacing: 0

                    // Watch subfolders
                    SettingToggle {
                        Layout.fillWidth: true
                        label: qsTr("Theo dõi thư mục con")
                        desc: qsTr("Bao gồm file trong các thư mục con")
                        checked: root.watchSubfolders
                        onToggled: root.watchSubfolders = checked
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.divider }

                    // Delete after upload
                    SettingToggle {
                        Layout.fillWidth: true
                        label: qsTr("Xóa file local sau khi tải lên")
                        desc: qsTr("File sẽ bị xóa khỏi máy tính sau upload thành công")
                        checked: root.deleteAfterUpload
                        onToggled: root.deleteAfterUpload = checked
                    }

                    // Delete warning
                    Rectangle {
                        Layout.fillWidth: true
                        visible: root.deleteAfterUpload
                        Layout.topMargin: AuroraTheme.sp1
                        Layout.bottomMargin: AuroraTheme.sp1
                        height: delWarning.implicitHeight + AuroraTheme.sp2 * 2
                        radius: 8
                        color: AuroraTheme.warnSoft
                        border.width: 1
                        border.color: Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25)

                        Text {
                            id: delWarning
                            anchors.centerIn: parent
                            width: parent.width - AuroraTheme.sp4
                            text: "\u26A0 " + qsTr("File sẽ bị xóa vĩnh viễn khỏi máy tính sau khi upload thành công.")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: AuroraTheme.caption.pixelSize
                            color: AuroraTheme.warn
                            wrapMode: Text.WordWrap
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.divider }

                    // Ignore patterns
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: AuroraTheme.sp3
                        Layout.bottomMargin: AuroraTheme.sp3
                        spacing: AuroraTheme.sp1

                        Text {
                            text: qsTr("Bỏ qua file theo mẫu")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: AuroraTheme.body.pixelSize
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink1
                        }
                        FsTextField {
                            id: ignoreField
                            Layout.fillWidth: true
                            text: root.ignorePatterns
                            hint: qsTr("Mỗi mẫu cách nhau bởi dấu phẩy")
                            onTextChanged: root.ignorePatterns = text
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.divider }

                    // Speed limit
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: AuroraTheme.sp3
                        spacing: AuroraTheme.sp2

                        Text {
                            text: qsTr("Giới hạn tốc độ tải lên")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: AuroraTheme.body.pixelSize
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink1
                        }

                        FsRadio {
                            label: qsTr("Không giới hạn")
                            value: 0
                            selectedValue: root.speedLimitMode
                            onSelected: root.speedLimitMode = 0
                        }

                        RowLayout {
                            spacing: AuroraTheme.sp2

                            FsRadio {
                                label: qsTr("Tùy chỉnh")
                                value: 1
                                selectedValue: root.speedLimitMode
                                onSelected: root.speedLimitMode = 1
                            }

                            SpeedSpinner {
                                visible: root.speedLimitMode === 1
                                value: root.speedLimitKBps
                                onValueModified: root.speedLimitKBps = value
                            }

                            Text {
                                visible: root.speedLimitMode === 1
                                text: "KB/s"
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: AuroraTheme.caption.pixelSize
                                color: AuroraTheme.ink2
                            }
                        }
                    }
                }
            }
        }
    }

    footer: Item {
        width: 480
        height: 64

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp3
            anchors.bottomMargin: AuroraTheme.sp3

            Item { Layout.fillWidth: true }

            FsButton {
                text: qsTr("Hủy")
                variant: "ghost"
                onClicked: root.close()
            }
            FsButton {
                text: qsTr("Lưu")
                variant: "primary"
                onClicked: {
                    var limit = root.speedLimitMode === 0 ? 0 : root.speedLimitKBps;
                    root.saved(root.watchId, root.watchSubfolders,
                               root.deleteAfterUpload, root.ignorePatterns, limit);
                    root.close();
                }
            }
        }
    }

    // ── Inline helpers ───────────────────────────────
    component SettingToggle: RowLayout {
        property string label: ""
        property string desc: ""
        property bool checked: false
        signal toggled()
        Layout.topMargin: AuroraTheme.sp3
        Layout.bottomMargin: AuroraTheme.sp3
        spacing: AuroraTheme.sp4

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: label
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
            }
            Text {
                visible: desc.length > 0
                text: desc
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink3
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        FsSwitch {
            checked: parent.checked
            onToggled: function(c) { parent.checked = c; parent.toggled(); }
        }
    }

    component SpeedSpinner: Rectangle {
        property int value: 5120
        signal valueModified()

        implicitWidth: 100; implicitHeight: 32
        radius: 8
        color: AuroraTheme.panel
        border.width: 1; border.color: AuroraTheme.border

        RowLayout {
            anchors.fill: parent; anchors.margins: 1; spacing: 0

            Rectangle {
                Layout.preferredWidth: 28; Layout.fillHeight: true
                radius: AuroraTheme.radiusSm
                color: minMa.containsMouse ? AuroraTheme.divider : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                Text { anchors.centerIn: parent; text: "\u2212"; font.pixelSize: 14; color: AuroraTheme.ink2 }
                MouseArea {
                    id: minMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (value > 512) { value -= 512; valueModified(); } }
                }
            }
            Text {
                Layout.fillWidth: true
                text: value.toString()
                horizontalAlignment: Text.AlignHCenter
                font.family: AuroraTheme.fontMono
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink1
            }
            Rectangle {
                Layout.preferredWidth: 28; Layout.fillHeight: true
                radius: AuroraTheme.radiusSm
                color: plusMa.containsMouse ? AuroraTheme.divider : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                Text { anchors.centerIn: parent; text: "+"; font.pixelSize: 14; color: AuroraTheme.ink2 }
                MouseArea {
                    id: plusMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (value < 102400) { value += 512; valueModified(); } }
                }
            }
        }
    }
}
