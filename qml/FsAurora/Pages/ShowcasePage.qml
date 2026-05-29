// SPDX-License-Identifier: Proprietary
// ShowcasePage (Aurora) — reference grid of every Aurora atom with its
// variants. Mirrors Fshare.Pages.ShowcasePage so a user can flip between
// the two variants and compare 1:1.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme
import FsAurora.Components
import Fshare.Components 1.0 as Fsh

ScrollView {
    id: root
    clip: true

    ColumnLayout {
        width: root.width - 48
        x: 24
        y: 24
        spacing: AuroraTheme.sp6

        // ── Header ──────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp2
            Text {
                text: "Aurora"
                font.family: AuroraTheme.fontSerif
                font.italic: true
                font.pixelSize: 32
                color: AuroraTheme.ink1
            }
            Text {
                text: "design system · showcase"
                font.family: AuroraTheme.fontSans
                font.pixelSize: 16
                color: AuroraTheme.ink3
            }
            Item { Layout.fillWidth: true }
        }

        // ── Colors ──────────────────────────────────
        SectionTitle { titleText: "Colors" }
        RowLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp2
            SwatchTile { label: "accent";  c: AuroraTheme.accent }
            SwatchTile { label: "accent2"; c: AuroraTheme.accent2 }
            SwatchTile { label: "accent3"; c: AuroraTheme.accent3 }
            SwatchTile { label: "success"; c: AuroraTheme.success }
            SwatchTile { label: "warn";    c: AuroraTheme.warn }
            SwatchTile { label: "danger";  c: AuroraTheme.danger }
            SwatchTile { label: "info";    c: AuroraTheme.info }
            Item { Layout.fillWidth: true }
        }

        // ── Gradient ────────────────────────────────
        SectionTitle { titleText: "Gradient" }
        Item {
            Layout.preferredWidth: 320
            Layout.preferredHeight: 120
            FsGradientRect {
                anchors.fill: parent
                radius: AuroraTheme.radiusLg
            }
            Text {
                anchors.centerIn: parent
                text: "linear-gradient(135°)"
                color: "#FFFFFF"
                font.family: AuroraTheme.fontMono
                font.pixelSize: 13
            }
        }

        // ── Buttons ─────────────────────────────────
        SectionTitle { titleText: "Buttons" }
        RowLayout {
            spacing: AuroraTheme.sp2
            FsButton { text: "Primary";   variant: "primary" }
            FsButton { text: "Secondary"; variant: "secondary" }
            FsButton { text: "Ghost";     variant: "ghost" }
            FsButton { text: "Danger";    variant: "danger" }
            FsButton { text: "Link";      variant: "link" }
        }
        RowLayout {
            spacing: AuroraTheme.sp2
            FsButton { text: "Small";   variant: "primary"; size: "sm" }
            FsButton { text: "Medium";  variant: "primary"; size: "md" }
            FsButton { text: "Large";   variant: "primary"; size: "lg" }
            FsButton { text: "Loading"; variant: "primary"; loading: true }
            FsButton { text: "Disabled"; variant: "primary"; enabled: false }
        }
        RowLayout {
            spacing: AuroraTheme.sp2
            FsButton { text: "Tải lên";   variant: "primary";   icon: "arrow-up" }
            FsButton { text: "Tải xuống"; variant: "secondary"; icon: "arrow-down" }
            FsButton { text: "Chia sẻ";   variant: "ghost";     icon: "link" }
        }

        // ── Inputs ──────────────────────────────────
        SectionTitle { titleText: "Inputs" }
        RowLayout {
            spacing: AuroraTheme.sp4
            Fsh.FsTextField {
                Layout.preferredWidth: 240
                label: "Email"
                placeholder: "ten@fshare.vn"
            }
            Fsh.FsTextField {
                Layout.preferredWidth: 240
                label: "Mật khẩu"
                placeholder: "••••••••"
                echoMode: TextInput.Password
            }
            Fsh.FsTextField {
                Layout.preferredWidth: 240
                label: "Có lỗi"
                placeholder: "user@…"
                text: "invalid-email"
                error: "Email không hợp lệ"
            }
            Item { Layout.fillWidth: true }
        }

        // ── Badges ──────────────────────────────────
        SectionTitle { titleText: "Badges" }
        RowLayout {
            spacing: AuroraTheme.sp2
            FsBadge { text: "Default" }
            FsBadge { text: "Accent";  variant: "accent"; dot: true }
            FsBadge { text: "Synced";  variant: "success"; dot: true }
            FsBadge { text: "Warn";    variant: "warn"; dot: true }
            FsBadge { text: "Error";   variant: "danger"; dot: true }
            FsBadge { text: "Info";    variant: "info"; dot: true }
            Item { Layout.fillWidth: true }
        }

        // ── Progress / Switch ───────────────────────
        SectionTitle { titleText: "Progress & Switch" }
        RowLayout {
            spacing: AuroraTheme.sp4
            FsProgressBar { Layout.preferredWidth: 220; value: 0.15 }
            FsProgressBar { Layout.preferredWidth: 220; value: 0.55 }
            FsProgressBar { Layout.preferredWidth: 220; value: 0.92 }
            Item { Layout.fillWidth: true }
        }
        RowLayout {
            spacing: AuroraTheme.sp4
            FsProgressBar { Layout.preferredWidth: 220; indeterminate: true }
            FsSwitch { label: "Bật đồng bộ"; checked: true }
            FsSwitch { label: "Xoá local sau khi tải"; checked: false }
            Item { Layout.fillWidth: true }
        }

        // ── Cards ───────────────────────────────────
        SectionTitle { titleText: "Cards" }
        RowLayout {
            spacing: AuroraTheme.sp3
            Fsh.FsCard {
                Layout.preferredWidth: 260
                Layout.preferredHeight: 130
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp4
                    spacing: AuroraTheme.sp1
                    Text {
                        text: "Project Zenith"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink1
                    }
                    Text {
                        text: "24 tệp · 12.4 GB · cập nhật 2 giờ trước"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 12
                        color: AuroraTheme.ink3
                    }
                    Item { Layout.fillHeight: true }
                    RowLayout {
                        spacing: AuroraTheme.sp1
                        FsBadge { text: "Đã đồng bộ"; variant: "success"; dot: true }
                        FsBadge { text: "Riêng tư" }
                    }
                }
            }
            Fsh.FsCard {
                accent: true
                Layout.preferredWidth: 260
                Layout.preferredHeight: 130
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp4
                    spacing: AuroraTheme.sp1
                    Text {
                        text: "VIP 1 năm"
                        font.family: AuroraTheme.fontSerif
                        font.italic: true
                        font.pixelSize: 22
                        color: AuroraTheme.ink1
                    }
                    Text {
                        text: "Còn 247 ngày · 820 GB / 1 TB"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 12
                        color: AuroraTheme.ink3
                    }
                    Item { Layout.fillHeight: true }
                    FsProgressBar { Layout.fillWidth: true; value: 0.82 }
                }
            }
            Item { Layout.fillWidth: true }
        }

        Item { Layout.preferredHeight: AuroraTheme.sp8 }
    }

    // ── Inline components ─────────────────────────────
    component SectionTitle: Text {
        property string titleText: ""
        text: titleText
        font.family: AuroraTheme.fontMono
        font.pixelSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 1.6
        font.capitalization: Font.AllUppercase
        color: AuroraTheme.ink4
        Layout.fillWidth: true
        Layout.topMargin: AuroraTheme.sp3
    }

    component SwatchTile: ColumnLayout {
        property string label: ""
        property color c: "transparent"
        spacing: AuroraTheme.sp1
        Rectangle {
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            color: c
            radius: AuroraTheme.radiusMd
            border.width: 1
            border.color: AuroraTheme.border
        }
        Text {
            text: label
            font.family: AuroraTheme.fontMono
            font.pixelSize: 10
            color: AuroraTheme.ink3
        }
    }
}
