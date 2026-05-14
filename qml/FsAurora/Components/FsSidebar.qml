// SPDX-License-Identifier: Proprietary
// FsSidebar (Aurora) — dark sidebar with VIP profile card, sectioned nav,
// and promo card at the bottom.
//
// Parent binds `currentPage`, user info, VIP/storage strings, and handles
// navClicked(index) + logoutClicked + upgradeClicked signals. All display
// strings default to reasonable fallbacks so the sidebar works even when
// userInfoViewModel has not loaded yet.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme

Rectangle {
    id: root

    // Two-way sync — parent sets currentPage; child emits navClicked
    property int currentPage: 0
    property bool devBuild: false

    // Account info
    property string userName: ""
    property string userEmail: ""
    property string avatarUrl: ""

    // VIP + storage strip shown inside the hero card. When empty, uses
    // graceful fallbacks ("Miễn phí" / 0 GB / 0 GB).
    property string vipLabel: ""       // e.g. "VIP · 243 ngày" or "Miễn phí"
    property string storageUsedText: "" // e.g. "287 GB"
    property string storageTotalText: "" // e.g. "500 GB"
    property real storagePct: 0.0       // 0..1

    // Legacy transfer stats (kept for backwards compatibility — rendered as
    // a subtle line below the nav when non-empty).
    property string transferStatsText: ""
    property bool transferStatsVisible: false

    // Sync module pending count — drives the small red pill next to the
    // "Đồng bộ" nav item.  0 hides it; the value is rendered as "99+" when
    // greater than 99 so the pill never grows wider than the nav row.
    property int syncPendingCount: 0

    signal navClicked(int index)
    signal logoutClicked()
    signal upgradeClicked()

    color: AuroraTheme.sidebar
    width: 240

    // Right-edge 1px highlight
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Qt.rgba(1, 1, 1, 0.06)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 18
        anchors.bottomMargin: 14
        spacing: 0

        // ══════════════════════════════════════════════
        //  VIP HERO CARD
        // ══════════════════════════════════════════════
        FsGradientRect {
            Layout.fillWidth: true
            Layout.preferredHeight: heroCol.implicitHeight + 24
            radius: 12

            ColumnLayout {
                id: heroCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 10
                        color: Qt.rgba(1, 1, 1, 0.22)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.32)
                        Text {
                            anchors.centerIn: parent
                            text: root.userName.length > 0
                                ? root.userName.charAt(0).toUpperCase() : "U"
                            color: "#FFFFFF"
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            Layout.fillWidth: true
                            text: root.userName.length > 0 ? root.userName : qsTr("Người dùng")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: "#FFFFFF"
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            FsIcon {
                                name: "sparkle"
                                sizePx: 10
                                color: "#FFFFFF"
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.vipLabel.length > 0 ? root.vipLabel : qsTr("Miễn phí")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.letterSpacing: 0.4
                                color: Qt.rgba(1, 1, 1, 0.85)
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                // Storage line
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        Layout.fillWidth: true
                        text: root.storageUsedText.length > 0 ? root.storageUsedText : "0 GB"
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 10
                        color: Qt.rgba(1, 1, 1, 0.9)
                    }
                    Text {
                        text: "/ " + (root.storageTotalText.length > 0 ? root.storageTotalText : "0 GB")
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 10
                        color: Qt.rgba(1, 1, 1, 0.9)
                        horizontalAlignment: Text.AlignRight
                    }
                }

                // Progress bar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 4
                    radius: 2
                    color: Qt.rgba(0, 0, 0, 0.22)
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: Math.max(0, Math.min(1, root.storagePct)) * parent.width
                        radius: 2
                        color: Qt.rgba(1, 1, 1, 0.92)
                        Behavior on width { enabled: !AuroraTheme.reduceMotion
                            NumberAnimation { duration: AuroraTheme.durBase } }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════
        //  NAV
        // ══════════════════════════════════════════════
        Item { Layout.preferredHeight: 20 }

        NavLabel { text: qsTr("KHÔNG GIAN") }

        Repeater {
            model: [
                { label: qsTr("Trang chủ"), icon: "house",       index: 8, badge: "" },
                { label: qsTr("My Files"),  icon: "folder",      index: 3, badge: "" },
                { label: qsTr("Yêu thích"), icon: "heart",       index: 4, badge: "" },
                { label: qsTr("Tài khoản"), icon: "user-circle", index: 5, badge: "" },
                { label: qsTr("Cài đặt"),   icon: "gear",        index: 6, badge: "" }
            ]
            delegate: NavItem {}
        }

        Item { Layout.preferredHeight: 12 }

        NavLabel { text: qsTr("HOẠT ĐỘNG") }

        Repeater {
            // The Sync entry's badge is data-bound to syncPendingCount so the
            // pill grows/shrinks live as upload state changes — Repeater
            // re-renders the row whenever the model array reference is reset.
            // Computed string: "" when count == 0 (NavItem hides the pill),
            // "99+" when > 99 (keeps pill width stable inside a 240px sidebar).
            model: [
                { label: qsTr("Tải xuống"), icon: "arrow-down", index: 0, badge: "" },
                { label: qsTr("Tải lên"),   icon: "arrow-up",   index: 1, badge: "" },
                { label: qsTr("Đồng bộ"),   icon: "sync",       index: 2,
                  badge: root.syncPendingCount <= 0
                      ? ""
                      : (root.syncPendingCount > 99 ? "99+"
                                                    : String(root.syncPendingCount)) }
            ]
            delegate: NavItem {}
        }

        Repeater {
            model: root.devBuild
                ? [{ label: qsTr("Showcase"), icon: "sparkle", index: 7, badge: "DEV" }] : []
            delegate: NavItem {}
        }

        // Transfer stats strip (legacy) — appears below nav when enabled
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            Layout.topMargin: 8
            radius: AuroraTheme.radiusSm
            color: Qt.rgba(1, 1, 1, 0.04)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.08)
            visible: root.transferStatsVisible

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6
                FsIcon { name: "sync"; sizePx: 11; color: Qt.rgba(1, 1, 1, 0.5) }
                Text {
                    Layout.fillWidth: true
                    text: root.transferStatsText
                    elide: Text.ElideRight
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    color: Qt.rgba(1, 1, 1, 0.65)
                }
            }
        }

        Item { Layout.fillHeight: true }

        // ══════════════════════════════════════════════
        //  UPGRADE CARD (promo)
        // ══════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: upgradeCol.implicitHeight + 28
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.04)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.08)

            ColumnLayout {
                id: upgradeCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 2

                Text {
                    text: qsTr("GIẢM 40%")
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.0
                    color: AuroraTheme.accent2
                }
                Text {
                    text: qsTr("Nâng cấp VIP Pro")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: "#FFFFFF"
                    Layout.topMargin: 2
                }
                Text {
                    text: qsTr("1TB · tốc độ không giới hạn")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 11
                    color: Qt.rgba(1, 1, 1, 0.55)
                    Layout.topMargin: 1
                }

                FsGradientRect {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    Layout.topMargin: 10
                    radius: 8

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Xem ưu đãi")
                        color: "#FFFFFF"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.upgradeClicked()
                    }
                }
            }
        }

        // ══════════════════════════════════════════════
        //  LOGOUT ROW (icon-only, ghost style)
        //  Transparent by default; a soft tinted hover avoids the heavy dark
        //  square that the solid-fill variant produced on the dark sidebar.
        // ══════════════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 10
            spacing: 6

            Item { Layout.fillWidth: true }

            Rectangle {
                id: logoutBtn
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: AuroraTheme.radiusSm
                color: logoutMa.containsMouse ? Qt.rgba(1, 1, 1, 0.10) : "transparent"
                border.width: 0
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
                FsIcon {
                    anchors.centerIn: parent
                    name: "power"
                    sizePx: 14
                    color: logoutMa.containsMouse ? "#FFFFFF" : Qt.rgba(1, 1, 1, 0.55)
                    Behavior on color { enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast } }
                }
                MouseArea {
                    id: logoutMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.logoutClicked()
                    ToolTip.visible: containsMouse
                    ToolTip.text: qsTr("Đăng xuất")
                    ToolTip.delay: 400
                }
            }
        }
    }

    // ── Section label component ──
    component NavLabel: Text {
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.bottomMargin: 6
        font.family: AuroraTheme.fontMono
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 1.8
        color: Qt.rgba(1, 1, 1, 0.4)
    }

    // ── Nav item component ──
    component NavItem: Rectangle {
        id: navItem
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        radius: AuroraTheme.radiusSm
        readonly property bool active: root.currentPage === modelData.index
        // hovered OR keyboard-focused — both states share the same subtle
        // highlight so users on either input mode get the same affordance.
        readonly property bool _hot: navMa.containsMouse || navItem.activeFocus
        color: navItem.active ? Qt.rgba(1, 1, 1, 0.08)
            : (_hot ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        // Keyboard support — Tab into the item, Space/Enter to activate,
        // Up/Down to walk to siblings inside the same Repeater (Qt's
        // KeyNavigation.tab default rolls forward in declaration order so
        // up/down would otherwise both jump to the next outer focus item).
        activeFocusOnTab: true
        Accessible.role: Accessible.MenuItem
        Accessible.name: modelData.label
        Accessible.onPressAction: root.navClicked(modelData.index)
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
                || event.key === Qt.Key_Enter) {
                root.navClicked(modelData.index);
                event.accepted = true;
            } else if (event.key === Qt.Key_Down || event.key === Qt.Key_Up) {
                // Walk the sidebar's NavItem siblings.  Repeater inserts
                // each delegate as a sibling of the Repeater itself, so
                // its parent's children list is the natural arrow path.
                const sibs = parent.children;
                const idx = sibs.indexOf(navItem);
                const delta = (event.key === Qt.Key_Down) ? 1 : -1;
                for (let i = idx + delta; i >= 0 && i < sibs.length; i += delta) {
                    if (sibs[i] && sibs[i].activeFocusOnTab) {
                        sibs[i].forceActiveFocus();
                        event.accepted = true;
                        break;
                    }
                }
            }
        }

        // Focus ring — subtle 2px ring inside the sidebar's dark surface
        // so it reads as "focused" without competing with the active
        // gradient bar on the left.
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: AuroraTheme.radiusSm
            color: "transparent"
            border.width: navItem.activeFocus ? 2 : 0
            border.color: Qt.rgba(1, 1, 1, 0.45)
            visible: navItem.activeFocus
        }

        // Active indicator — 3px gradient bar on the left
        FsGradientRect {
            visible: navItem.active
            width: 3; height: 20; radius: 2
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 10

            FsIcon {
                name: modelData.icon
                sizePx: 16
                color: navItem.active ? "#FFFFFF" : Qt.rgba(1, 1, 1, 0.72)
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
            }
            Text {
                Layout.fillWidth: true
                text: modelData.label
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: navItem.active ? Font.DemiBold : Font.Medium
                color: navItem.active ? "#FFFFFF" : Qt.rgba(1, 1, 1, 0.82)
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
            }

            Rectangle {
                visible: modelData.badge.length > 0
                Layout.preferredHeight: 16
                Layout.preferredWidth: badgeText.implicitWidth + 12
                radius: 8
                color: AuroraTheme.accent
                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: modelData.badge
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: "#FFFFFF"
                }
            }
        }

        MouseArea {
            id: navMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.navClicked(modelData.index)
        }
    }
}
