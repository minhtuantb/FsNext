// SPDX-License-Identifier: Proprietary
// VaultIntroPage — "About Vault" / quick guide + liability disclaimer.
// Embedded by VaultPage when the user taps Help (or on first run).

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Fshare.Components 1.0
import FsAurora.Theme 1.0

Item {
    id: intro

    property bool hasVault: false
    signal createRequested()
    signal goHomeRequested()

    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + AuroraTheme.sp10
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: col
            width: Math.min(720, intro.width - AuroraTheme.sp8)
            x: Math.max(AuroraTheme.sp4, (intro.width - width) / 2)
            y: AuroraTheme.sp6
            spacing: AuroraTheme.sp5

            // ── Hero ──
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp4
                Rectangle {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.accentSoft
                    FsIcon { anchors.centerIn: parent; name: "shield-key"; sizePx: 30; color: AuroraTheme.accent }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Vault — mã hóa đầu-cuối")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.h2.pixelSize
                        font.weight: Font.Bold
                        color: AuroraTheme.ink1
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("File của bạn được mã hóa ngay trên máy trước khi rời khỏi thiết bị.")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        color: AuroraTheme.ink3
                    }
                }
            }

            // ── How it works (4 steps) ──
            Text {
                text: qsTr("Cách hoạt động")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.h3.pixelSize
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
                Layout.topMargin: AuroraTheme.sp2
            }
            Repeater {
                model: [
                    { icon: "key",       t: qsTr("1. Đặt passphrase"),     d: qsTr("Tạo Vault và đặt passphrase của riêng bạn.") },
                    { icon: "shield",    t: qsTr("2. Mã hóa trên máy"),    d: qsTr("Thêm file → FsNext mã hóa ngay trên máy của bạn.") },
                    { icon: "cloud-up",  t: qsTr("3. Tải lên an toàn"),    d: qsTr("File mã hóa được tải lên Fshare — server không đọc được nội dung.") },
                    { icon: "lock-open", t: qsTr("4. Mở khi cần"),         d: qsTr("Mở khóa bằng passphrase để xem lại bất cứ lúc nào.") }
                ]
                delegate: Rectangle {
                    id: stepCard
                    required property var modelData
                    Layout.fillWidth: true
                    radius: AuroraTheme.radiusMd
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border
                    implicitHeight: stepRow.implicitHeight + AuroraTheme.sp4 * 2
                    RowLayout {
                        id: stepRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: AuroraTheme.sp4
                        spacing: AuroraTheme.sp4
                        Rectangle {
                            Layout.preferredWidth: 38
                            Layout.preferredHeight: 38
                            Layout.alignment: Qt.AlignTop
                            radius: AuroraTheme.radiusMd
                            color: AuroraTheme.accentSoft
                            FsIcon { anchors.centerIn: parent; name: stepCard.modelData.icon; sizePx: 18; color: AuroraTheme.accent }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: stepCard.modelData.t
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: stepCard.modelData.d
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 12
                                color: AuroraTheme.ink3
                            }
                        }
                    }
                }
            }

            // ── Disclaimer ──
            FsCallout {
                Layout.fillWidth: true
                Layout.topMargin: AuroraTheme.sp2
                tone: "info"
                title: qsTr("Lưu ý quan trọng")
                body: qsTr("Toàn bộ quá trình mã hóa diễn ra ngay trên máy của bạn. FsNext không lưu giữ passphrase hay khóa, nên không thể truy cập hay khôi phục nội dung trong Vault. Mã hóa giúp bảo vệ quyền riêng tư, nhưng không thay thế cho việc sao lưu — hãy luôn giữ thêm một bản sao lưu các file quan trọng ở nơi an toàn khác.")
            }

            // ── CTA ──
            FsButton {
                Layout.topMargin: AuroraTheme.sp2
                text: intro.hasVault ? qsTr("Đến trang Vault") : qsTr("Tạo Vault")
                variant: "primary"
                icon: intro.hasVault ? "vault" : "plus"
                onClicked: intro.hasVault ? intro.goHomeRequested() : intro.createRequested()
            }
        }
    }
}
