// SPDX-License-Identifier: Proprietary
// LoginView (Aurora) — faithful port of handoff/src/aurora-auth.jsx.
//
// Layout:
//   ┌──────────────────── 560 ────────────┬── flex ──────────────┐
//   │  Dark brand panel (#0E0E12)         │  Light form panel    │
//   │  · radial gradient orbs (pink/      │  · signup link top-r │
//   │    orange top-right, yellow bot-l)  │  · "━━ Đăng nhập"    │
//   │  · 48px grid overlay                │  · Chào <name>.      │
//   │  · "━━ Chào bạn trở lại"            │  · Google/Facebook/  │
//   │  · "Tiếp tục nơi bạn dừng." (88px   │    Zalo row          │
//   │    serif, italic accent line)       │  · HOẶC divider      │
//   │  · stats row at bottom              │  · email / password  │
//   │                                     │  · remember / forgot │
//   │                                     │  · gradient button   │
//   │                                     │  · TLS 1.3 footer    │
//   └─────────────────────────────────────┴──────────────────────┘

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme
import FsAurora.Components
import Fshare.Components 1.0 as Fsh
import Fshare.Utils 1.0

Item {
    id: root

    property var authVm: null                   // expected: authViewModel

    // Derived display name — first word of userName, defaults to "bạn".
    readonly property string _displayName: {
        if (!authVm || !authVm.userName) return "bạn";
        const parts = authVm.userName.trim().split(/\s+/);
        return parts[parts.length - 1] || "bạn";
    }

    // ═════════════════════════════════════════════════════════
    //  LEFT · Brand panel (dark editorial hero)
    // ═════════════════════════════════════════════════════════
    Rectangle {
        id: brand
        width: Math.min(560, parent.width * 0.5)
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: "#0E0E12"
        clip: true

        // ── Radial gradient orbs ────────────────────────────
        // Qt doesn't have RadialGradient in QtQuick 6 core; approximate with
        // a RadialGradient Shape fallback is heavy. Instead, use Qt6 Effects'
        // RadialGradient via a Canvas draw — small (two orbs, drawn once).
        Canvas {
            id: orbs
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();

                // Orb 1 — pink/orange, top-right
                var grad1 = ctx.createRadialGradient(
                    width - 160 + 260, -120 + 260, 0,
                    width - 160 + 260, -120 + 260, 260);
                grad1.addColorStop(0.0,  Qt.rgba(1.00, 0.24, 0.50, 0.55));  // #FF3D7F
                grad1.addColorStop(0.4,  Qt.rgba(1.00, 0.36, 0.18, 0.45));  // #FF5B2E
                grad1.addColorStop(0.7,  Qt.rgba(1.00, 0.36, 0.18, 0.00));
                grad1.addColorStop(1.0,  Qt.rgba(1.00, 0.36, 0.18, 0.00));
                ctx.fillStyle = grad1;
                ctx.fillRect(0, 0, width, height);

                // Orb 2 — yellow, bottom-left
                var grad2 = ctx.createRadialGradient(
                    -120 + 210, height + 120 - 210, 0,
                    -120 + 210, height + 120 - 210, 210);
                grad2.addColorStop(0.0, Qt.rgba(1.00, 0.69, 0.11, 0.45));
                grad2.addColorStop(0.7, Qt.rgba(1.00, 0.69, 0.11, 0.00));
                grad2.addColorStop(1.0, Qt.rgba(1.00, 0.69, 0.11, 0.00));
                ctx.fillStyle = grad2;
                ctx.fillRect(0, 0, width, height);
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }

        // ── 48px grid overlay ───────────────────────────────
        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.03);
                ctx.lineWidth = 1;
                var step = 48;
                for (var x = 0; x < width; x += step) {
                    ctx.beginPath();
                    ctx.moveTo(x + 0.5, 0);
                    ctx.lineTo(x + 0.5, height);
                    ctx.stroke();
                }
                for (var y = 0; y < height; y += step) {
                    ctx.beginPath();
                    ctx.moveTo(0, y + 0.5);
                    ctx.lineTo(width, y + 0.5);
                    ctx.stroke();
                }
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 56
            anchors.bottomMargin: 40
            anchors.leftMargin: 52
            anchors.rightMargin: 52
            spacing: 0

            // ── Brand mark ──────────────────────────────────
            RowLayout {
                spacing: 12
                Item {
                    width: 36; height: 36
                    Fsh.FsGradientRect { anchors.fill: parent; radius: 10 }
                    Text {
                        anchors.centerIn: parent
                        text: "F"
                        color: "#FFFFFF"
                        font.family: AuroraTheme.fontSerif
                        font.pixelSize: 22
                        font.italic: true
                    }
                }
                Text {
                    text: "Fshare Desktop"
                    color: "#FFFFFF"
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    font.letterSpacing: -0.36
                }
            }

            Item { Layout.fillHeight: true }

            // ── Editorial hero ──────────────────────────────
            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true

                Text {
                    text: "━━ Chào bạn trở lại"
                    color: "#FFAF1D"
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.letterSpacing: 2.0
                    font.capitalization: Font.AllUppercase
                    Layout.bottomMargin: 18
                }

                // Line 1 — "Tiếp tục" serif upright
                Text {
                    text: "Tiếp tục"
                    color: "#FFFFFF"
                    font.family: AuroraTheme.fontSerif
                    font.pixelSize: 72
                    font.letterSpacing: -2.8
                    lineHeight: 0.96
                }

                // Line 2 — "nơi bạn dừng." serif italic.
                // Qt doesn't support CSS text-gradient; use accent3 pink as a
                // single-tone stand-in that still reads as "the accent line".
                Text {
                    text: "nơi bạn dừng."
                    color: AuroraTheme.accent3
                    font.family: AuroraTheme.fontSerif
                    font.italic: true
                    font.pixelSize: 72
                    font.letterSpacing: -2.8
                    lineHeight: 0.96
                }

                Text {
                    Layout.topMargin: 22
                    Layout.preferredWidth: 380
                    text: "287 GB trong kho, 7 file đang chờ tải về, 14 link đang sống — mọi thứ của bạn, cách một lần đăng nhập."
                    color: Qt.rgba(1, 1, 1, 0.70)
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 14
                    font.weight: Font.Light
                    wrapMode: Text.WordWrap
                    lineHeight: 1.55
                }
            }

            Item { Layout.preferredHeight: 48 }

            // ── Stats divider + row ─────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Qt.rgba(1, 1, 1, 0.10)
            }
            RowLayout {
                Layout.topMargin: 24
                spacing: 32

                Repeater {
                    model: [
                        { label: "File đang lưu",  value: "287 GB" },
                        { label: "Tốc độ VIP",     value: "Không giới hạn" },
                        { label: "Hết hạn",        value: "243 ngày" }
                    ]
                    delegate: ColumnLayout {
                        spacing: 3
                        Text {
                            text: modelData.label
                            color: Qt.rgba(1, 1, 1, 0.50)
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            font.letterSpacing: 1.4
                            font.capitalization: Font.AllUppercase
                        }
                        Text {
                            text: modelData.value
                            color: "#FFFFFF"
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }
        }
    }

    // ═════════════════════════════════════════════════════════
    //  RIGHT · Form panel
    // ═════════════════════════════════════════════════════════
    Rectangle {
        id: form
        anchors.left: brand.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: AuroraTheme.bg

        // Top-right signup link
        RowLayout {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.rightMargin: 32
            spacing: 6

            Text {
                text: "Chưa có tài khoản?"
                font.family: AuroraTheme.fontSans
                font.pixelSize: 12
                color: AuroraTheme.ink3
            }
            Text {
                text: qsTr("Đăng ký ngay →")
                font.family: AuroraTheme.fontSans
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: AuroraTheme.accent
                // Registration intentionally lives on fshare.vn (phone OTP,
                // captcha, terms acceptance) — opening the system browser
                // is the supported flow, not in-app duplication.
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally(FsExternalLinks.signup)
                }
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(400, parent.width - 64)
            spacing: 0

            // ── Kicker + greeting ───────────────────────────
            Text {
                text: "━━ Đăng nhập"
                font.family: AuroraTheme.fontMono
                font.pixelSize: 11
                font.letterSpacing: 2.0
                font.capitalization: Font.AllUppercase
                color: AuroraTheme.ink4
                Layout.bottomMargin: 10
            }

            RowLayout {
                spacing: 0
                Text {
                    text: "Chào "
                    font.family: AuroraTheme.fontSerif
                    font.pixelSize: 48
                    color: AuroraTheme.ink1
                    font.letterSpacing: -1.4
                }
                Text {
                    text: root._displayName + "."
                    font.family: AuroraTheme.fontSerif
                    font.italic: true
                    font.pixelSize: 48
                    color: AuroraTheme.accent
                    font.letterSpacing: -1.4
                }
            }

            Text {
                Layout.topMargin: 10
                Layout.fillWidth: true
                text: "Đăng nhập để tiếp tục đồng bộ 3 folder và quản lý 14 link đang hoạt động."
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                color: AuroraTheme.ink3
                wrapMode: Text.WordWrap
            }

            // ── Social row ──────────────────────────────────
            RowLayout {
                Layout.topMargin: 28
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: [
                        { brand: "google",   label: "Google",   dot: "#4285F4" },
                        { brand: "facebook", label: "Facebook", dot: "#1877F2" },
                        { brand: "zalo",     label: "Zalo",     dot: "#0068FF" }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: AuroraTheme.radiusMd
                        color: socialMa.containsMouse
                            ? Qt.rgba(0, 0, 0, 0.03) : AuroraTheme.panel
                        border.width: 1
                        border.color: AuroraTheme.border

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 7
                            Rectangle {
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 14
                                radius: 7
                                color: modelData.dot
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label.charAt(0)
                                    color: "#FFFFFF"
                                    font.family: AuroraTheme.fontSans
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                }
                            }
                            Text {
                                text: modelData.label
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                            }
                        }

                        MouseArea {
                            id: socialMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: !(root.authVm && root.authVm.isLoading)
                            onClicked: {
                                if (!root.authVm) return;
                                if (modelData.brand === "google")        root.authVm.loginWithGoogle();
                                else if (modelData.brand === "facebook") root.authVm.loginWithFacebook();
                                else if (modelData.brand === "zalo")     root.authVm.loginWithFptId();
                            }
                        }
                    }
                }
            }

            // ── HOẶC divider ────────────────────────────────
            RowLayout {
                Layout.topMargin: 24
                Layout.bottomMargin: 22
                Layout.fillWidth: true
                spacing: 12

                Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.border }
                Text {
                    text: "HOẶC"
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.letterSpacing: 1.5
                    color: AuroraTheme.ink4
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: AuroraTheme.border }
            }

            // ── Fields ──────────────────────────────────────
            Fsh.FsTextField {
                id: emailField
                Layout.fillWidth: true
                label: "EMAIL HOẶC TÊN ĐĂNG NHẬP"
                placeholder: "your@email.com"
                text: root.authVm ? root.authVm.email : ""
                onTextChanged:
                    if (root.authVm && root.authVm.email !== text) root.authVm.email = text
            }
            Item { Layout.preferredHeight: 14 }
            Fsh.FsTextField {
                id: passwordField
                Layout.fillWidth: true
                label: "MẬT KHẨU"
                placeholder: "••••••••"
                echoMode: TextInput.Password
                text: root.authVm ? root.authVm.password : ""
                onTextChanged:
                    if (root.authVm && root.authVm.password !== text) root.authVm.password = text
                error: root.authVm ? root.authVm.errorMessage : ""
                onAccepted: if (root.authVm) root.authVm.login()
            }

            // ── Remember + forgot row ───────────────────────
            RowLayout {
                Layout.topMargin: 14
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    spacing: 8
                    Rectangle {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        radius: 4
                        color: "transparent"
                        Fsh.FsGradientRect {
                            anchors.fill: parent
                            radius: 4
                            visible: root.authVm ? root.authVm.rememberMe : false
                        }
                        Rectangle {
                            anchors.fill: parent
                            radius: 4
                            color: "transparent"
                            border.width: 1.5
                            border.color: AuroraTheme.border
                            visible: !(root.authVm ? root.authVm.rememberMe : false)
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            visible: root.authVm ? root.authVm.rememberMe : false
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.authVm) root.authVm.rememberMe = !root.authVm.rememberMe
                        }
                    }
                    Text {
                        text: "Giữ đăng nhập"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 12
                        color: AuroraTheme.ink2
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.authVm) root.authVm.rememberMe = !root.authVm.rememberMe
                        }
                    }
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: qsTr("Quên mật khẩu?")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: AuroraTheme.accent
                    // Reset flow (OTP, email link) is web-only — desktop just
                    // hands off to the browser via FsExternalLinks so the
                    // password slug lives in one file.
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally(FsExternalLinks.forgotPassword)
                    }
                }
            }

            // ── Login button ────────────────────────────────
            Fsh.FsButton {
                Layout.topMargin: 22
                Layout.fillWidth: true
                text: (root.authVm && root.authVm.isLoading) ? "Đang đăng nhập…" : "Đăng nhập  ›"
                variant: "primary"
                size: "lg"
                loading: root.authVm && root.authVm.isLoading
                enabled: emailField.text.length > 0 && passwordField.text.length > 0
                    && !(root.authVm && root.authVm.isLoading)
                onClicked: if (root.authVm) root.authVm.login()
            }

            // ── Security footer ─────────────────────────────
            Rectangle {
                Layout.topMargin: 22
                Layout.fillWidth: true
                implicitHeight: 36
                radius: AuroraTheme.radiusMd
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        radius: 3
                        color: AuroraTheme.success
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#FFFFFF"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Kết nối an toàn · TLS 1.3 · SRP authentication"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        color: AuroraTheme.ink3
                    }
                }
            }
        }
    }
}
