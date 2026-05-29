// SPDX-License-Identifier: Proprietary
// SettingsPage (Aurora) — editorial header + categorized setting cards.
//
// All VM wiring preserved:
//   settingsViewModel.autoLogin, stayOnTop, darkMode, autoDownload,
//     downloadFolder, effectiveDownloadFolder, proxyMode, proxyHost,
//     proxyPort, downloadThreads, downloadSegments, uploadThreads,
//     maxGlobalSlots;
//   languageViewModel.language.

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Fshare.Components 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Aurora.FsScrollPage {
    id: page
    contentSpacing: AuroraTheme.sp4

        // ═════════════════════════════════════════════════
        //  EDITORIAL HEADER
        // ═════════════════════════════════════════════════
        Aurora.FsPageHeader {
            framed: true
            kicker: qsTr("Tuỳ chọn & hệ thống")
            title: qsTr("Cài")
            accentWord: qsTr("đặt.")
            titlePixelSize: 56
            titleLetterSpacing: -1.8
            subtitle: qsTr("Tùy chỉnh tải xuống, tải lên, ngôn ngữ và kết nối")
        }

        // ── Chung ──────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Chung")
            kicker: qsTr("━ General")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Tự động đăng nhập khi khởi động")
                    desc: qsTr("Tự động đăng nhập bằng thông tin đã lưu")
                    checked: settingsViewModel ? settingsViewModel.autoLogin : false
                    onToggled: if (settingsViewModel) settingsViewModel.autoLogin = checked
                }

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Luôn hiển thị trên cùng")
                    desc: qsTr("Giữ cửa sổ ứng dụng luôn ở trên các cửa sổ khác")
                    checked: settingsViewModel ? settingsViewModel.stayOnTop : false
                    onToggled: if (settingsViewModel) settingsViewModel.stayOnTop = checked
                }

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Giao diện tối")
                    desc: qsTr("Sử dụng giao diện màu tối")
                    checked: settingsViewModel ? settingsViewModel.darkMode : false
                    onToggled: if (settingsViewModel) settingsViewModel.darkMode = checked
                }

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Thu nhỏ vào khay hệ thống khi đóng cửa sổ")
                    desc: qsTr("Cửa sổ ẩn xuống khay (tray) thay vì thoát hẳn ứng dụng. " +
                               "Có thể bật lại bằng cách bấm icon trên khay.")
                    checked: settingsViewModel ? settingsViewModel.minimizeToTray : false
                    onToggled: if (settingsViewModel) settingsViewModel.minimizeToTray = checked
                }

                // Re-enable the close confirmation dialog. The dialog itself
                // can tick "Don't ask again" which flips this off — this
                // toggle is the only way to turn it back ON without resetting
                // every setting.
                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Hỏi xác nhận khi đóng cửa sổ")
                    desc: qsTr("Hiện popup 'Thoát hẳn / Thu nhỏ vào khay' mỗi lần bạn nhấn nút X. " +
                               "Tắt nếu muốn cửa sổ ứng xử theo lựa chọn cuối.")
                    checked: settingsViewModel ? settingsViewModel.confirmOnClose : true
                    onToggled: if (settingsViewModel) settingsViewModel.confirmOnClose = checked
                }

                // ── HUD (Transfer mini window + tray balloons) ──────────
                // Two related toggles kept here next to minimizeToTray
                // because they're conceptually part of the same "what
                // happens after I send the window away" surface.
                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Hiện mini HUD khi thu nhỏ")
                    desc: qsTr("Khi bạn thu nhỏ cửa sổ (nút _) hoặc đóng vào khay mà đang có " +
                               "lượt chuyển file, một cửa sổ nhỏ luôn-trên-cùng sẽ hiện tốc độ + " +
                               "danh sách đang tải. Tự ẩn sau 30 giây khi mọi thứ hoàn tất.")
                    checked: settingsViewModel ? settingsViewModel.showOnHideToTray : true
                    onToggled: if (settingsViewModel) settingsViewModel.showOnHideToTray = checked
                }

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Hiện tiến độ trên thanh taskbar")
                    desc: qsTr("Vẽ thanh tiến độ ngay trên nút taskbar của Fshare khi đang " +
                               "chuyển file — xanh khi chạy, đỏ khi lỗi, vàng khi tạm dừng. " +
                               "Cách xem tiến độ gọn nhất, không cần mở cửa sổ. (Chỉ Windows.)")
                    checked: settingsViewModel ? settingsViewModel.showTaskbarProgress : true
                    onToggled: if (settingsViewModel) settingsViewModel.showTaskbarProgress = checked
                }

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Thông báo bóng nhỏ khi tải xong / lỗi")
                    desc: qsTr("Hiện balloon notification ở khay hệ thống mỗi khi một file " +
                               "tải xong hoặc tải lỗi. Bấm vào thông báo để mở thẳng task đó. " +
                               "Tắt sẽ chỉ đổi màu icon khay thầm lặng.")
                    checked: settingsViewModel ? settingsViewModel.notifyOnTransferDone : true
                    onToggled: if (settingsViewModel) settingsViewModel.notifyOnTransferDone = checked
                }
            }
        }

        // ── Khi tải file trùng tên ──────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Khi tên file bị trùng")
            kicker: qsTr("━ File conflict")

            content: ColumnLayout {
                spacing: AuroraTheme.sp3

                Repeater {
                    model: [
                        { v: 0, label: qsTr("Đổi tên \"(1)\""),
                          desc: qsTr("Mặc định, an toàn — không ghi đè dữ liệu cũ.") },
                        { v: 1, label: qsTr("Ghi đè"),
                          desc: qsTr("File cũ sẽ bị thay thế bởi bản tải mới.") },
                        { v: 2, label: qsTr("Bỏ qua"),
                          desc: qsTr("Tải xuống được hủy, file cũ giữ nguyên.") },
                        { v: 3, label: qsTr("Hỏi mỗi lần"),
                          desc: qsTr("Hiện hộp thoại lựa chọn cho mỗi lần trùng.") }
                    ]
                    delegate: FsRadio {
                        Layout.fillWidth: true
                        label: modelData.label
                        value: modelData.v
                        selectedValue: settingsViewModel ? settingsViewModel.fileConflictPolicy : 0
                        onSelected: (v) => {
                            if (settingsViewModel) settingsViewModel.fileConflictPolicy = v
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: AuroraTheme.sp2
                    text: {
                        const opts = [
                            qsTr("Đổi tên — đề xuất."),
                            qsTr("Ghi đè — cẩn trọng, có thể mất dữ liệu cũ."),
                            qsTr("Bỏ qua — file mới không được lưu."),
                            qsTr("Hỏi mỗi lần — thêm một bước cho mỗi tải xuống.")
                        ]
                        const idx = settingsViewModel ? settingsViewModel.fileConflictPolicy : 0
                        return opts[Math.max(0, Math.min(3, idx))]
                    }
                    color: AuroraTheme.ink3
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    wrapMode: Text.WordWrap
                }
            }
        }

        // ── Ngôn ngữ ───────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Ngôn ngữ")
            kicker: qsTr("━ Language")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Ngôn ngữ hiển thị")
                    desc: qsTr("Chọn ngôn ngữ giao diện ứng dụng")

                    control: FsSegmentedControl {
                        options: [
                            { value: "vi", label: "Tiếng Việt" },
                            { value: "en", label: "English" }
                        ]
                        selectedValue: languageViewModel ? languageViewModel.language : "vi"
                        onSelectionChanged: {
                            if (languageViewModel) languageViewModel.language = selectedValue
                        }
                    }
                }
            }
        }

        // ── Hiệu suất tải ──────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Hiệu suất tải")
            kicker: qsTr("━ Performance")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Số tải xuống đồng thời")
                    desc: qsTr("Số file tải xuống cùng lúc (1–16). "
                              + "Khuyến nghị: 8 cho tài khoản VIP, 4 cho tài khoản miễn phí.")

                    control: NumberSpinner {
                        value: settingsViewModel ? settingsViewModel.downloadThreads : 8
                        minValue: 1; maxValue: 16
                        onValueModified: if (settingsViewModel) settingsViewModel.downloadThreads = value
                    }
                }

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Luồng song song mỗi file")
                    desc: qsTr("Số luồng HTTP Range mỗi file (1–32). "
                              + "Khuyến nghị: 16 — chia file thành các phần tải song song. "
                              + "Chỉ áp dụng cho file ≥ 2 MB trên CDN hỗ trợ Range request.")

                    control: NumberSpinner {
                        value: settingsViewModel ? settingsViewModel.downloadSegments : 16
                        minValue: 1; maxValue: 32
                        onValueModified: if (settingsViewModel) settingsViewModel.downloadSegments = value
                    }
                }

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Số tải lên đồng thời")
                    desc: qsTr("Số file tải lên cùng lúc (1–8). "
                              + "Khuyến nghị: 4. Tải lên bị giới hạn bởi CPU/ổ đĩa; hơn 4 hiếm khi có thêm lợi ích.")

                    control: NumberSpinner {
                        value: settingsViewModel ? settingsViewModel.uploadThreads : 4
                        minValue: 1; maxValue: 8
                        onValueModified: if (settingsViewModel) settingsViewModel.uploadThreads = value
                    }
                }

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Tổng giới hạn đồng thời")
                    desc: qsTr("Trần tổng cho mọi loại (tải xuống + tải lên + duyệt thư mục), 1–32. "
                              + "Khuyến nghị: 10. Bảo vệ máy yếu khỏi việc cộng dồn các hạn mức riêng. "
                              + "Đặt 0 để tắt trần tổng.")

                    control: NumberSpinner {
                        value: settingsViewModel ? settingsViewModel.maxGlobalSlots : 10
                        minValue: 0; maxValue: 32
                        onValueModified: if (settingsViewModel) settingsViewModel.maxGlobalSlots = value
                    }
                }

                // Recommendation hint — Aurora accent-soft tip
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: AuroraTheme.sp2
                    height: hintRow.implicitHeight + AuroraTheme.sp4 * 2
                    radius: AuroraTheme.radiusMd
                    color: AuroraTheme.accentSoft
                    border.width: 1
                    border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.25)

                    RowLayout {
                        id: hintRow
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: AuroraTheme.sp4; anchors.rightMargin: AuroraTheme.sp4
                        spacing: AuroraTheme.sp3

                        FsIcon {
                            name: "sparkle"
                            sizePx: 16
                            color: AuroraTheme.accent
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Tối ưu cho tài khoản VIP: 8 file × 16 luồng = tối đa 128 kết nối HTTP/2 song song. "
                                      + "Tài khoản miễn phí: giảm xuống 4 file × 8 luồng để tránh bị giới hạn tốc độ.")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 12
                            color: AuroraTheme.ink2
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        // ── Tải xuống ──────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Tải xuống")
            kicker: qsTr("━ Downloads")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                SettingsToggle {
                    Layout.fillWidth: true
                    label: qsTr("Tự động bắt đầu tải")
                    desc: qsTr("Bắt đầu tải ngay khi thêm vào danh sách")
                    checked: settingsViewModel ? settingsViewModel.autoDownload : false
                    onToggled: if (settingsViewModel) settingsViewModel.autoDownload = checked
                }

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Thư mục tải xuống")
                    desc: qsTr("Vị trí mặc định lưu file đã tải")

                    control: FsFolderPicker {
                        Layout.preferredWidth: 360
                        placeholder: settingsViewModel ? settingsViewModel.effectiveDownloadFolder : ""
                        dialogTitle: qsTr("Chọn thư mục tải xuống mặc định")
                        folder: settingsViewModel ? settingsViewModel.downloadFolder : ""
                        onFolderChanged: if (settingsViewModel) settingsViewModel.downloadFolder = folder
                    }
                }
            }
        }

        // ── Kết nối ────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Kết nối")
            kicker: qsTr("━ Network")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                SettingsRow {
                    Layout.fillWidth: true
                    label: qsTr("Chế độ proxy")
                    desc: qsTr("Cách kết nối qua proxy")

                    control: FsSegmentedControl {
                        segmentWidth: 80
                        options: [
                            { value: 0, label: qsTr("Không dùng") },
                            { value: 1, label: qsTr("Hệ thống") },
                            { value: 2, label: qsTr("Thủ công") }
                        ]
                        selectedValue: settingsViewModel ? settingsViewModel.proxyMode : 0
                        onSelectionChanged: if (settingsViewModel) settingsViewModel.proxyMode = selectedValue
                    }
                }

                SettingsRow {
                    Layout.fillWidth: true
                    visible: settingsViewModel && settingsViewModel.proxyMode === 2
                    label: qsTr("Địa chỉ proxy")
                    desc: qsTr("Tên máy chủ hoặc địa chỉ IP proxy")

                    control: FsTextField {
                        Layout.preferredWidth: 280
                        placeholder: "proxy.example.com"
                        text: settingsViewModel ? settingsViewModel.proxyHost : ""
                        onTextChanged: if (settingsViewModel) settingsViewModel.proxyHost = text
                    }
                }

                SettingsRow {
                    Layout.fillWidth: true
                    visible: settingsViewModel && settingsViewModel.proxyMode === 2
                    label: qsTr("Cổng proxy")
                    desc: qsTr("Cổng máy chủ proxy (1–65535)")

                    control: NumberSpinner {
                        value: settingsViewModel ? settingsViewModel.proxyPort : 0
                        minValue: 0; maxValue: 65535
                        onValueModified: if (settingsViewModel) settingsViewModel.proxyPort = value
                    }
                }
            }
        }

        // ── Thông tin ──────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            title: qsTr("Thông tin")
            kicker: qsTr("━ About")

            content: ColumnLayout {
                spacing: AuroraTheme.sp2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp3

                    Item {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        FsGradientRect { anchors.fill: parent; radius: AuroraTheme.radiusMd }
                        Text {
                            anchors.centerIn: parent
                            text: "F"
                            color: "#FFFFFF"
                            font.family: AuroraTheme.fontSerif
                            font.pixelSize: 24
                            font.italic: true
                            font.weight: Font.Medium
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        RowLayout {
                            spacing: AuroraTheme.sp2
                            Text {
                                text: "FsNext"
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 18
                                font.weight: Font.Bold
                                color: AuroraTheme.ink1
                            }
                            Text {
                                text: "Aurora"
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.accent
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                        Text {
                            text: qsTr("Phiên bản %1 · © FPT Corporation").arg(Qt.application.version)
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            color: AuroraTheme.ink4
                        }
                    }
                }
            }
        }

        Item { Layout.preferredHeight: AuroraTheme.sp6 }

    // ═════════════════════════════════════════════════
    //  Helper components
    // ═════════════════════════════════════════════════
    component SettingsCard: Rectangle {
        property string title: ""
        property string kicker: ""
        property alias content: contentSlot.children
        Layout.fillWidth: true
        Layout.preferredHeight: header.height + contentSlot.childrenRect.height + AuroraTheme.sp6 * 2 + AuroraTheme.sp3
        radius: AuroraTheme.radiusLg
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border

        ColumnLayout {
            id: header
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp5
            height: headerKicker.implicitHeight + headerTitle.implicitHeight + AuroraTheme.sp2 + AuroraTheme.sp3 + 1
            spacing: 2

            Text {
                id: headerKicker
                visible: parent.parent.kicker.length > 0
                text: parent.parent.kicker
                color: AuroraTheme.ink4
                font.family: AuroraTheme.fontMono
                font.pixelSize: 10
                font.letterSpacing: 1.6
                font.capitalization: Font.AllUppercase
                font.weight: Font.DemiBold
            }
            Text {
                id: headerTitle
                text: parent.parent.title
                font.family: AuroraTheme.fontSerif
                font.pixelSize: 22
                color: AuroraTheme.ink1
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: AuroraTheme.sp2
                height: 1
                color: AuroraTheme.divider
            }
        }

        Item {
            id: contentSlot
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: header.bottom
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp4
            height: childrenRect.height
        }
    }

    component SettingsRow: RowLayout {
        property string label: ""
        property string desc: ""
        property alias control: ctrlSlot.children
        spacing: AuroraTheme.sp4

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: label
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
            }
            Text {
                visible: desc.length > 0
                text: desc
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                color: AuroraTheme.ink3
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        Item {
            id: ctrlSlot
            Layout.preferredWidth: childrenRect.width
            Layout.preferredHeight: childrenRect.height
        }
    }

    component SettingsToggle: RowLayout {
        property string label: ""
        property string desc: ""
        property bool checked: false
        signal toggled()
        spacing: AuroraTheme.sp4

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: label
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
            }
            Text {
                visible: desc.length > 0
                text: desc
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                color: AuroraTheme.ink3
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        FsSwitch {
            checked: parent.checked
            onToggled: function(c) {
                parent.checked = c;
                parent.toggled();
            }
        }
    }

    component NumberSpinner: Rectangle {
        property int value: 0
        property int minValue: 0
        property int maxValue: 100
        signal valueModified()

        implicitWidth: 108
        implicitHeight: 34
        radius: AuroraTheme.radiusMd
        color: AuroraTheme.bg
        border.width: 1
        border.color: AuroraTheme.border

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 30
                Layout.fillHeight: true
                radius: AuroraTheme.radiusSm
                color: minusMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
                Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 15
                    color: minusMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2 }
                MouseArea {
                    id: minusMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (value > minValue) { value--; valueModified(); }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: value.toString()
                horizontalAlignment: Text.AlignHCenter
                font.family: AuroraTheme.fontMono
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
            }

            Rectangle {
                Layout.preferredWidth: 30
                Layout.fillHeight: true
                radius: AuroraTheme.radiusSm
                color: plusMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
                Text { anchors.centerIn: parent; text: "+"; font.pixelSize: 15
                    color: plusMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2 }
                MouseArea {
                    id: plusMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (value < maxValue) { value++; valueModified(); }
                    }
                }
            }
        }
    }
}
