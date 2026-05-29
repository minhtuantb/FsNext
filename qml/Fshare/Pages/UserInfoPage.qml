// SPDX-License-Identifier: Proprietary
// UserInfoPage — Aurora account page: editorial hero + VIP card,
// benefits panel, personal info, storage breakdown.

import QtQuick
import QtQuick.Layouts
import Fshare.Components 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Aurora.FsScrollPage {
    id: page
    contentSpacing: AuroraTheme.sp6

    // ── Helpers ────────────────────────────────────────────────────────────
    function pct(used, total) {
        if (total <= 0) return 0;
        return Math.min(100, (used / total) * 100);
    }
    function barColor(p, variant) {
        if (p > 90) return AuroraTheme.danger;
        if (p > 75) return AuroraTheme.warn;
        return variant === "traffic" ? AuroraTheme.accent3 : AuroraTheme.success;
    }
    function fracPct(used, total) {
        if (total <= 0) return 0;
        return used / total;
    }

    readonly property var vm: userInfoViewModel

            // ═══════════════════════════════════════════════════════════════
            // EDITORIAL HEADER
            // ═══════════════════════════════════════════════════════════════
            Aurora.FsPageHeader {
                framed: false
                kicker: qsTr("Tài khoản")
                // The dynamic name + level pair is rendered as title + accentWord
                // so the second-token italic-accent treatment lines up with the
                // other editorial headers.
                title: {
                    const n = vm ? vm.userName : "";
                    return (n.length > 0 ? n : qsTr("Thành viên")) + ",";
                }
                accentWord: (vm && vm.levelLabel.length > 0 ? vm.levelLabel : "Member") + "."
                subtitle: qsTr("Thông tin tài khoản và dung lượng Fshare")
            }

            // ═══════════════════════════════════════════════════════════════
            // HERO ROW — VIP card (2fr) + Benefits (1fr)
            // ═══════════════════════════════════════════════════════════════
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp4

                // ── VIP card (dark sidebar bg + accent glow) ──────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 2
                    Layout.minimumHeight: vipCol.implicitHeight + AuroraTheme.sp6 * 2 + 40
                    radius: AuroraTheme.radiusXl
                    color: AuroraTheme.sidebar
                    clip: true

                    // Decorative gradient glow in the top-right corner. A
                    // Rectangle with radius = half its dimension + opacity gives
                    // a soft aurora-style halo on the dark card without MultiEffect
                    // blur (which is optional on some Qt builds).
                    Rectangle {
                        width: 360; height: 360
                        radius: width / 2
                        x: parent.width - 140
                        y: -120
                        color: AuroraTheme.accent3
                        opacity: 0.42
                    }
                    Rectangle {
                        width: 240; height: 240
                        radius: width / 2
                        x: parent.width - 80
                        y: -80
                        color: AuroraTheme.accent
                        opacity: 0.32
                    }

                    ColumnLayout {
                        id: vipCol
                        anchors {
                            left: parent.left; right: parent.right
                            top: parent.top
                            margins: AuroraTheme.sp6
                        }
                        spacing: AuroraTheme.sp3

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp2
                            FsIcon {
                                name: "sparkle"
                                sizePx: 12
                                color: AuroraTheme.accent2
                            }
                            Text {
                                text: (qsTr("Gói") + " " + (vm && vm.levelLabel.length > 0 ? vm.levelLabel : "Free")
                                      + " · " + (vm ? FsFormat.bytes(vm.storageTotalAll) : "—")).toUpperCase()
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 1.8
                                color: AuroraTheme.accent2
                            }
                            Item { Layout.fillWidth: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp2
                            Text {
                                text: vm ? FsFormat.bytes(vm.webspaceUsed + vm.secureUsed) : "0 B"
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 42
                                font.weight: Font.Normal
                                font.letterSpacing: -1.3
                                color: AuroraTheme.textOnSidebar
                            }
                            Text {
                                text: vm ? " / " + FsFormat.bytes(vm.webspaceTotal + vm.secureTotal) : ""
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 20
                                font.italic: true
                                color: Qt.rgba(1, 1, 1, 0.5)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 6
                            }
                            Text {
                                text: qsTr("đã dùng")
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 20
                                font.italic: true
                                color: Qt.rgba(1, 1, 1, 0.5)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 6
                                Layout.leftMargin: AuroraTheme.sp2
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Text {
                            text: {
                                const free = vm ? FsFormat.bytes(vm.webspaceFree + vm.secureFree) : "—";
                                const expText = (vm && vm.vipExpiry.length > 0)
                                    ? (" · " + qsTr("hết hạn") + " " + vm.vipExpiry)
                                    : "";
                                return qsTr("Còn") + " " + free + expText;
                            }
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 12
                            color: Qt.rgba(1, 1, 1, 0.65)
                        }

                        // Segmented storage bar (webspace / secure / free)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: AuroraTheme.sp3
                            Layout.preferredHeight: 10
                            radius: 5
                            color: Qt.rgba(1, 1, 1, 0.08)
                            clip: true

                            Row {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    width: parent.width * (vm ? page.fracPct(vm.webspaceUsed,
                                        vm.webspaceTotal + vm.secureTotal) : 0)
                                    height: parent.height
                                    color: AuroraTheme.accent3
                                    Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow } }
                                }
                                Rectangle {
                                    width: parent.width * (vm ? page.fracPct(vm.secureUsed,
                                        vm.webspaceTotal + vm.secureTotal) : 0)
                                    height: parent.height
                                    color: AuroraTheme.accent
                                    Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow } }
                                }
                            }
                        }

                        // Legend
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: AuroraTheme.sp2
                            spacing: AuroraTheme.sp5

                            Repeater {
                                model: [
                                    { c: AuroraTheme.accent3, l: qsTr("Không bảo đảm"),
                                      v: vm ? FsFormat.bytes(vm.webspaceUsed) : "—" },
                                    { c: AuroraTheme.accent,  l: qsTr("Bảo đảm"),
                                      v: vm ? FsFormat.bytes(vm.secureUsed) : "—" },
                                    { c: Qt.rgba(1, 1, 1, 0.3), l: qsTr("Còn trống"),
                                      v: vm ? FsFormat.bytes(vm.webspaceFree + vm.secureFree) : "—" }
                                ]
                                delegate: RowLayout {
                                    spacing: AuroraTheme.sp1
                                    Rectangle { width: 7; height: 7; radius: 3.5; color: modelData.c }
                                    Text {
                                        text: modelData.l + " · " + modelData.v
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 11
                                        color: Qt.rgba(1, 1, 1, 0.8)
                                    }
                                }
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }

                    // Refresh button — top-right corner
                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: AuroraTheme.sp5
                        width: 36; height: 36
                        radius: AuroraTheme.radiusMd
                        color: refreshMa.containsMouse
                            ? Qt.rgba(1, 1, 1, 0.14)
                            : Qt.rgba(1, 1, 1, 0.06)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.15)
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                        FsIcon {
                            anchors.centerIn: parent
                            name: "refresh"
                            sizePx: 15
                            color: AuroraTheme.textOnSidebar
                        }
                        MouseArea {
                            id: refreshMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (vm) vm.refresh()
                        }
                    }
                }

                // ── Benefits card (right) ─────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.minimumHeight: vipCol.implicitHeight + AuroraTheme.sp6 * 2 + 40
                    radius: AuroraTheme.radiusXl
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border

                    ColumnLayout {
                        anchors { fill: parent; margins: AuroraTheme.sp5 }
                        spacing: 0

                        Text {
                            text: qsTr("Quyền lợi")
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.4
                            font.capitalization: Font.AllUppercase
                            color: AuroraTheme.ink4
                            Layout.bottomMargin: AuroraTheme.sp3
                        }

                        Repeater {
                            model: [
                                { icon: "sparkle", label: qsTr("Loại thành viên"),
                                  value: vm ? vm.levelLabel : "—" },
                                { icon: "upload",  label: qsTr("Dung lượng"),
                                  value: vm ? FsFormat.bytes(vm.storageTotalAll) : "—" },
                                { icon: "download", label: qsTr("Lưu lượng"),
                                  value: vm ? FsFormat.bytes(vm.trafficTotal) : "—" },
                                { icon: "shield",  label: qsTr("Bảo mật"),
                                  value: "AES-256" },
                                { icon: "key",     label: qsTr("Điểm tích lũy"),
                                  value: vm ? (vm.totalPoints + " " + qsTr("điểm")) : "0",
                                  last: true }
                            ]
                            delegate: Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 42

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: AuroraTheme.sp3

                                    Rectangle {
                                        width: 28; height: 28
                                        radius: AuroraTheme.radiusSm
                                        color: AuroraTheme.accentTint10
                                        FsIcon {
                                            anchors.centerIn: parent
                                            name: modelData.icon
                                            sizePx: 14
                                            color: AuroraTheme.accent
                                        }
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.label
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 12
                                        color: AuroraTheme.ink2
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: modelData.value
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: AuroraTheme.ink1
                                        elide: Text.ElideRight
                                        Layout.maximumWidth: 140
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                Rectangle {
                                    visible: !modelData.last
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 1
                                    color: AuroraTheme.border
                                }
                            }
                        }
                    }
                }
            }

            // ═══════════════════════════════════════════════════════════════
            // PERSONAL INFO CARD
            // ═══════════════════════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: infoCol.implicitHeight + AuroraTheme.sp5 * 2
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border

                ColumnLayout {
                    id: infoCol
                    anchors { fill: parent; margins: AuroraTheme.sp5 }
                    spacing: 0

                    Text {
                        text: qsTr("Thông tin cá nhân")
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.4
                        font.capitalization: Font.AllUppercase
                        color: AuroraTheme.ink4
                        Layout.bottomMargin: AuroraTheme.sp3
                    }

                    // Avatar + name row
                    Item {
                        id: avatarRow
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64

                        readonly property string avatarSrc: vm ? vm.avatarUrl : ""
                        readonly property bool hasAvatar: avatarSrc.length > 0 && avatarImg.status === Image.Ready

                        RowLayout {
                            anchors.fill: parent
                            spacing: AuroraTheme.sp4

                            Item {
                                id: avatarSlot
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64

                                Aurora.FsGradientRect {
                                    anchors.fill: parent
                                    radius: 20
                                    visible: !avatarRow.hasAvatar
                                    Text {
                                        anchors.centerIn: parent
                                        text: {
                                            const n = vm ? vm.userName : "";
                                            return n.length > 0 ? n.charAt(0).toUpperCase() : "?";
                                        }
                                        font.family: AuroraTheme.fontSerif
                                        font.pixelSize: 28
                                        font.italic: true
                                        font.weight: Font.Bold
                                        color: AuroraTheme.textOnAccent
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 20
                                    clip: true
                                    color: "transparent"
                                    border.width: avatarRow.hasAvatar ? 1 : 0
                                    border.color: AuroraTheme.border
                                    visible: avatarRow.hasAvatar

                                    Image {
                                        id: avatarImg
                                        anchors.fill: parent
                                        source: avatarRow.avatarSrc
                                        sourceSize.width: 128
                                        sourceSize.height: 128
                                        fillMode: Image.PreserveAspectCrop
                                        smooth: true; mipmap: true; cache: true; asynchronous: true
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: vm ? vm.userName : ""
                                    font.family: AuroraTheme.fontSans
                                    font.pixelSize: 17
                                    font.weight: Font.Bold
                                    font.letterSpacing: -0.1
                                    color: AuroraTheme.ink1
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: {
                                        const id = vm && vm.userId.length > 0 ? ("#" + vm.userId) : "";
                                        const join = vm && vm.joinDate.length > 0 ? (" · " + qsTr("Tham gia") + " " + vm.joinDate) : "";
                                        return id + join;
                                    }
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 12
                                    color: AuroraTheme.ink3
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    // Divider
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: AuroraTheme.border
                        Layout.topMargin: AuroraTheme.sp4
                        Layout.bottomMargin: AuroraTheme.sp1
                    }

                    // Detail rows
                    Repeater {
                        model: [
                            { label: qsTr("Email"),             value: vm && vm.userEmail.length > 0 ? vm.userEmail : "—", verified: true },
                            { label: qsTr("Mã thành viên"),     value: vm && vm.userId.length > 0 ? vm.userId : "—",       mono: true },
                            { label: qsTr("Loại thành viên"),   value: vm ? vm.levelLabel : "—" },
                            { label: qsTr("Ngày đăng ký"),      value: vm && vm.joinDate.length > 0 ? vm.joinDate : "—" },
                            { label: qsTr("Thời hạn VIP"),      value: vm && vm.vipExpiry.length > 0 ? vm.vipExpiry : "—", last: true }
                        ]
                        delegate: Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 42

                            RowLayout {
                                anchors.fill: parent
                                spacing: AuroraTheme.sp3

                                Text {
                                    Layout.preferredWidth: 130
                                    text: modelData.label
                                    font.family: AuroraTheme.fontSans
                                    font.pixelSize: 12
                                    color: AuroraTheme.ink3
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.value
                                    font.family: modelData.mono ? AuroraTheme.fontMono : AuroraTheme.fontSans
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    color: AuroraTheme.ink1
                                    elide: Text.ElideRight
                                }
                                Rectangle {
                                    visible: modelData.verified === true
                                    Layout.preferredHeight: 20
                                    Layout.preferredWidth: verifiedText.implicitWidth + AuroraTheme.sp3
                                    radius: 4
                                    color: Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g,
                                                   AuroraTheme.success.b, 0.14)
                                    Text {
                                        id: verifiedText
                                        anchors.centerIn: parent
                                        text: "✓ " + qsTr("ĐÃ XÁC MINH")
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                        font.letterSpacing: 0.5
                                        color: AuroraTheme.success
                                    }
                                }
                            }

                            // Dashed-style divider via a Repeater of tiny rects
                            Row {
                                visible: !modelData.last
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                spacing: 3
                                clip: true
                                Repeater {
                                    model: Math.max(1, Math.floor(parent.width / 6))
                                    Rectangle {
                                        width: 3; height: 1
                                        color: AuroraTheme.border
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ═══════════════════════════════════════════════════════════════
            // STORAGE BREAKDOWN — 3 columns
            // ═══════════════════════════════════════════════════════════════
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp4

                // ── Non-secure (regular) storage ──────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: storageNSCol.implicitHeight + AuroraTheme.sp5 * 2
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1; border.color: AuroraTheme.border

                    ColumnLayout {
                        id: storageNSCol
                        anchors { fill: parent; margins: AuroraTheme.sp5 }
                        spacing: AuroraTheme.sp2

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("DL không bảo đảm")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.ink4
                            }
                            Text {
                                text: vm ? page.pct(vm.webspaceUsed, vm.webspaceTotal).toFixed(1) + "%" : ""
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink3
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp1
                            Text {
                                text: vm ? FsFormat.bytes(vm.webspaceUsed) : "0 B"
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 26
                                font.weight: Font.Normal
                                font.letterSpacing: -0.6
                                color: AuroraTheme.ink1
                            }
                            Text {
                                text: vm ? " / " + FsFormat.bytes(vm.webspaceTotal) : ""
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 14
                                font.italic: true
                                color: AuroraTheme.ink3
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 4
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6; radius: 3
                            color: AuroraTheme.border
                            Rectangle {
                                width: parent.width * (vm ? page.pct(vm.webspaceUsed, vm.webspaceTotal) / 100 : 0)
                                height: parent.height; radius: 3
                                color: page.barColor(vm ? page.pct(vm.webspaceUsed, vm.webspaceTotal) : 0, "storage")
                                Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow } }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: AuroraTheme.sp1
                            spacing: AuroraTheme.sp5
                            ColumnLayout {
                                spacing: 2
                                Text { text: qsTr("Đã dùng"); font.family: AuroraTheme.fontMono; font.pixelSize: 10; color: AuroraTheme.ink4 }
                                Text { text: vm ? FsFormat.bytes(vm.webspaceUsed) : "—"; font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.ink1 }
                            }
                            ColumnLayout {
                                spacing: 2
                                Text { text: qsTr("Còn lại"); font.family: AuroraTheme.fontMono; font.pixelSize: 10; color: AuroraTheme.ink4 }
                                Text { text: vm ? FsFormat.bytes(vm.webspaceFree) : "—"; font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.success; font.weight: Font.DemiBold }
                            }
                        }
                    }
                }

                // ── Secure storage ────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: storageSCol.implicitHeight + AuroraTheme.sp5 * 2
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1; border.color: AuroraTheme.border

                    ColumnLayout {
                        id: storageSCol
                        anchors { fill: parent; margins: AuroraTheme.sp5 }
                        spacing: AuroraTheme.sp2

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("DL bảo đảm")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.ink4
                            }
                            Text {
                                text: vm ? page.pct(vm.secureUsed, vm.secureTotal).toFixed(1) + "%" : ""
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink3
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp1
                            Text {
                                text: vm ? FsFormat.bytes(vm.secureUsed) : "0 B"
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 26
                                font.weight: Font.Normal
                                font.letterSpacing: -0.6
                                color: AuroraTheme.ink1
                            }
                            Text {
                                text: vm ? " / " + FsFormat.bytes(vm.secureTotal) : ""
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 14
                                font.italic: true
                                color: AuroraTheme.ink3
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 4
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6; radius: 3
                            color: AuroraTheme.border
                            Rectangle {
                                width: parent.width * (vm ? page.pct(vm.secureUsed, vm.secureTotal) / 100 : 0)
                                height: parent.height; radius: 3
                                color: page.barColor(vm ? page.pct(vm.secureUsed, vm.secureTotal) : 0, "storage")
                                Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow } }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: AuroraTheme.sp1
                            spacing: AuroraTheme.sp5
                            ColumnLayout {
                                spacing: 2
                                Text { text: qsTr("Đã dùng"); font.family: AuroraTheme.fontMono; font.pixelSize: 10; color: AuroraTheme.ink4 }
                                Text { text: vm ? FsFormat.bytes(vm.secureUsed) : "—"; font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.ink1 }
                            }
                            ColumnLayout {
                                spacing: 2
                                Text { text: qsTr("Còn lại"); font.family: AuroraTheme.fontMono; font.pixelSize: 10; color: AuroraTheme.ink4 }
                                Text { text: vm ? FsFormat.bytes(vm.secureFree) : "—"; font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.success; font.weight: Font.DemiBold }
                            }
                        }
                    }
                }

                // ── Traffic ───────────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: trafficCol.implicitHeight + AuroraTheme.sp5 * 2
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1; border.color: AuroraTheme.border

                    ColumnLayout {
                        id: trafficCol
                        anchors { fill: parent; margins: AuroraTheme.sp5 }
                        spacing: AuroraTheme.sp2

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Lưu lượng (Traffic)")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.ink4
                            }
                            Text {
                                text: vm ? page.pct(vm.trafficUsed, vm.trafficTotal).toFixed(1) + "%" : ""
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink3
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp1
                            Text {
                                text: vm ? FsFormat.bytes(vm.trafficUsed) : "0 B"
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 26
                                font.weight: Font.Normal
                                font.letterSpacing: -0.6
                                color: AuroraTheme.ink1
                            }
                            Text {
                                text: vm ? " / " + FsFormat.bytes(vm.trafficTotal) : ""
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 14
                                font.italic: true
                                color: AuroraTheme.ink3
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 4
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6; radius: 3
                            color: AuroraTheme.border
                            Rectangle {
                                width: parent.width * (vm ? page.pct(vm.trafficUsed, vm.trafficTotal) / 100 : 0)
                                height: parent.height; radius: 3
                                color: page.barColor(vm ? page.pct(vm.trafficUsed, vm.trafficTotal) : 0, "traffic")
                                Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow } }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: AuroraTheme.sp1
                            spacing: AuroraTheme.sp5
                            ColumnLayout {
                                spacing: 2
                                Text { text: qsTr("Đã dùng"); font.family: AuroraTheme.fontMono; font.pixelSize: 10; color: AuroraTheme.ink4 }
                                Text { text: vm ? FsFormat.bytes(vm.trafficUsed) : "—"; font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.ink1 }
                            }
                            ColumnLayout {
                                spacing: 2
                                Text { text: qsTr("Còn lại"); font.family: AuroraTheme.fontMono; font.pixelSize: 10; color: AuroraTheme.ink4 }
                                Text { text: vm ? FsFormat.bytes(vm.trafficFree) : "—"; font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.accent3; font.weight: Font.DemiBold }
                            }
                        }
                    }
                }
            }

            // ═══════════════════════════════════════════════════════════════
            // ACCOUNT MANAGEMENT — links out to fshare.vn
            // ═══════════════════════════════════════════════════════════════
            //
            // Every action below opens the system browser instead of being
            // re-implemented in the desktop client.  Rationale:
            //   • OTP / captcha / 2FA enrollment is easier in a regular
            //     browser session (cookies, password manager, autofill).
            //   • Account-management surface evolves on the web platform
            //     faster than we'd want to ship desktop updates for.
            //   • Single source of truth: FsExternalLinks holds every URL,
            //     so swapping a slug is a one-file change.
            //
            // The link grid is intentionally compact (3 cols × N rows) and
            // doesn't show a destructive "Xoá tài khoản" entry inline —
            // that lives one click deeper on fshare.vn for safety.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: acctCol.implicitHeight + AuroraTheme.sp5 * 2
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border

                ColumnLayout {
                    id: acctCol
                    anchors { fill: parent; margins: AuroraTheme.sp5 }
                    spacing: AuroraTheme.sp3

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Quản lý tài khoản")
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.4
                            font.capitalization: Font.AllUppercase
                            color: AuroraTheme.ink4
                        }
                        Text {
                            text: qsTr("Mở trên fshare.vn ↗")
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            color: AuroraTheme.ink4
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        // 2 columns now that activeSessions / linkedAccounts
                        // were dropped per product decision — keeps the 4
                        // remaining cards visually balanced (2×2) instead of
                        // leaving holes in a 3-column grid.
                        columns: 2
                        columnSpacing: AuroraTheme.sp3
                        rowSpacing: AuroraTheme.sp2

                        // ── Account action component ───────────────────────
                        // One row in the grid.  Renders an icon chip + label
                        // + helper, hover/focus accent, and triggers
                        // Qt.openUrlExternally(url) on Space/Enter/click.
                        // We don't use FsButton because we want the rich
                        // two-line layout; the underlying focus ring follows
                        // the same accent pattern though.
                        component AcctLink : Rectangle {
                            id: link
                            property string iconName: ""
                            property string label: ""
                            property string helper: ""
                            property url url: ""
                            property bool externalUrl: true  // shows the ↗ glyph
                            signal activated()

                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            radius: AuroraTheme.radiusMd
                            color: linkMa.containsMouse || link.activeFocus
                                ? AuroraTheme.bg
                                : AuroraTheme.panel
                            border.width: 1
                            border.color: link.activeFocus
                                ? AuroraTheme.accent
                                : AuroraTheme.border
                            Behavior on color { enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast } }
                            Behavior on border.color { enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast } }

                            activeFocusOnTab: true
                            Accessible.role: Accessible.Link
                            Accessible.name: link.label
                            Accessible.description: link.helper
                            Keys.onPressed: function(event) {
                                if (event.key === Qt.Key_Space
                                    || event.key === Qt.Key_Return
                                    || event.key === Qt.Key_Enter) {
                                    link.activated();
                                    if (link.url.toString().length > 0)
                                        Qt.openUrlExternally(link.url);
                                    event.accepted = true;
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: AuroraTheme.sp3
                                anchors.rightMargin: AuroraTheme.sp3
                                spacing: AuroraTheme.sp3

                                Rectangle {
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    radius: AuroraTheme.radiusSm
                                    color: AuroraTheme.accentTint10
                                    FsIcon {
                                        anchors.centerIn: parent
                                        name: link.iconName
                                        sizePx: 16
                                        color: AuroraTheme.accent
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        Layout.fillWidth: true
                                        text: link.label
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        color: AuroraTheme.ink1
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: link.helper
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 11
                                        color: AuroraTheme.ink3
                                        elide: Text.ElideRight
                                    }
                                }

                                Text {
                                    visible: link.externalUrl
                                    text: "↗"
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 12
                                    color: AuroraTheme.ink3
                                }
                            }

                            MouseArea {
                                id: linkMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    link.activated();
                                    if (link.url.toString().length > 0)
                                        Qt.openUrlExternally(link.url);
                                }
                            }
                        }

                        // All four cards point to the same /account/profile
                        // landing page (per product decision).  Labels stay
                        // distinct so the user clicks the row that matches
                        // their intent; fshare.vn surfaces the right section
                        // on the page itself.
                        AcctLink {
                            iconName: "user"
                            label: qsTr("Thông tin cá nhân")
                            helper: qsTr("Đổi email, số điện thoại, avatar")
                            url: FsExternalLinks.editProfile
                        }
                        AcctLink {
                            iconName: "key"
                            label: qsTr("Đổi mật khẩu")
                            helper: qsTr("Cần email/OTP xác nhận")
                            url: FsExternalLinks.changePassword
                        }
                        AcctLink {
                            iconName: "shield"
                            label: qsTr("Xác thực 2 lớp (2FA)")
                            helper: qsTr("Bảo vệ thêm bằng OTP")
                            url: FsExternalLinks.twoFactor
                        }
                        AcctLink {
                            iconName: "sparkle"
                            label: qsTr("Lịch sử VIP & thanh toán")
                            helper: qsTr("Gói đã mua, gia hạn, giao dịch")
                            url: FsExternalLinks.vipHistory
                        }
                    }

                    // Inline note — clarifies that opening these links jumps
                    // out of the desktop client.  Keeps the surprise factor
                    // low for users who expect everything to stay in-app.
                    Text {
                        Layout.fillWidth: true
                        Layout.topMargin: AuroraTheme.sp1
                        text: qsTr("Các thao tác bảo mật (đổi mật khẩu, 2FA, quản lý thiết bị) được thực hiện trên fshare.vn để đảm bảo OTP và xác minh hoạt động chính xác.")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        color: AuroraTheme.ink4
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // ═══════════════════════════════════════════════════════════════
            // SUMMARY ROW — Total storage + Total traffic
            // ═══════════════════════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.panel
                border.width: 1; border.color: AuroraTheme.border

                RowLayout {
                    anchors {
                        fill: parent
                        leftMargin: AuroraTheme.sp6
                        rightMargin: AuroraTheme.sp6
                    }
                    spacing: AuroraTheme.sp8

                    Text {
                        text: qsTr("TỔNG DUNG LƯỢNG")
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1.4
                        color: AuroraTheme.ink4
                    }
                    Text {
                        text: vm ? FsFormat.bytes(vm.storageTotalAll) : "—"
                        font.family: AuroraTheme.fontSerif
                        font.pixelSize: 28
                        font.weight: Font.Normal
                        font.letterSpacing: -0.4
                        color: AuroraTheme.ink1
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("TỔNG LƯU LƯỢNG")
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1.4
                        color: AuroraTheme.ink4
                    }
                    Text {
                        text: vm ? FsFormat.bytes(vm.trafficTotal) : "—"
                        font.family: AuroraTheme.fontSerif
                        font.pixelSize: 28
                        font.weight: Font.Normal
                        font.letterSpacing: -0.4
                        color: AuroraTheme.ink1
                    }
                }
            }

            // Bottom spacer
            Item { Layout.preferredHeight: AuroraTheme.sp4 }
}
