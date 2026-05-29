// SPDX-License-Identifier: Proprietary
// FsSidebar — Dark Aurora v2.
//
// Layout (240px wide, near-black surface):
//   • Brand mark (28×28 gradient square + "Fshare" wordmark)
//   • 6 nav rows — Workspace (Trang chủ · My Files · Yêu thích)
//     separated by a 1px line from Activity (Tải xuống · Tải lên · Đồng bộ).
//     Showcase appears only on dev builds.
//   • Swipeable Mini HUD card (4 variants, persisted via SettingsVM).
//   • Avatar pivot at the bottom replaces the old power button — clicking
//     it opens a 3-item popover above (Thông tin tài khoản · Cài đặt ·
//     Đăng xuất danger). Tài khoản + Cài đặt now live in the popover so
//     they don't dilute the workspace/activity nav.
//
// Compatibility — keeps every property + signal the previous sidebar
// exposed, so Main.qml's bindings continue to work untouched:
//   • currentPage, devBuild, userName, userEmail, avatarUrl
//   • vipLabel, storageUsedText, storageTotalText, storagePct, isVip
//   • collapsed, toggleCollapseRequested()
//   • transferStatsText, transferStatsVisible, syncPendingCount
//   • navClicked(int), logoutClicked(), upgradeClicked()
//
// Notes on collapse mode:
//   The 64px rail is preserved for users who still prefer the slim variant
//   (SettingsViewModel.sidebarCollapsed). In rail mode the HUD and avatar
//   pivot hide entirely; only the brand mark + icon-only nav rows remain.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme

Rectangle {
    id: root

    // ── Public API (preserved from v1) ──────────────────────────────────
    property int currentPage: 0
    property bool devBuild: false

    property string userName: ""
    property string userEmail: ""
    property string avatarUrl: ""

    property string vipLabel: ""
    property string storageUsedText: ""
    property string storageTotalText: ""
    property real   storagePct: 0.0
    property bool   isVip: true

    property bool collapsed: false
    readonly property int _expandedWidth: 240
    readonly property int _collapsedWidth: 64

    signal toggleCollapseRequested()

    property string transferStatsText: ""
    property bool   transferStatsVisible: false

    property int syncPendingCount: 0

    signal navClicked(int index)
    signal logoutClicked()
    signal upgradeClicked()

    // ── Layout ─────────────────────────────────────────────────────────
    color: AuroraTheme.sidebarBg
    width: collapsed ? _collapsedWidth : _expandedWidth
    Behavior on width { enabled: !AuroraTheme.reduceMotion
        NumberAnimation { duration: AuroraTheme.durBase; easing.type: AuroraTheme.easingStd } }

    // Right-edge 1px divider — kept very subtle on the near-black bg so
    // the rail bleeds into the content area as a tonal step rather than
    // a hard line.
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: AuroraTheme.sidebarLine
    }

    ColumnLayout {
        id: shell
        anchors.fill: parent
        anchors.leftMargin: root.collapsed ? 8 : 14
        anchors.rightMargin: root.collapsed ? 8 : 14
        anchors.topMargin: 18
        anchors.bottomMargin: 14
        spacing: 0

        // ══════════════════════════════════════════════
        //  BRAND MARK
        // ══════════════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 18
            Layout.leftMargin: root.collapsed ? 0 : 4
            spacing: 10

            // 28×28 gradient square with italic "Fs" — the only place in
            // the shell where the brand serif appears.
            FsGradientRect {
                id: brandMark
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Layout.alignment: root.collapsed ? Qt.AlignHCenter : Qt.AlignVCenter
                radius: 8
                stop0: AuroraTheme.accent3
                stop1: AuroraTheme.accent
                stop2: AuroraTheme.accent2
                Text {
                    anchors.centerIn: parent
                    text: "Fs"
                    color: "#FFFFFF"
                    font.family: AuroraTheme.fontSerif
                    font.pixelSize: 17
                    font.italic: true
                    font.weight: Font.Medium
                }
            }

            Text {
                visible: !root.collapsed
                Layout.fillWidth: true
                text: "Fshare"
                color: AuroraTheme.sidebarInk
                font.family: AuroraTheme.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
                font.letterSpacing: -0.14
            }

            // Collapse toggle — preserved from v1. Lives on the right edge
            // of the brand row when expanded; auto-hidden in rail mode
            // (the brand mark itself acts as the click target via tooltip).
            Rectangle {
                id: collapseBtn
                visible: !root.collapsed
                Layout.preferredWidth: 22
                Layout.preferredHeight: 22
                radius: 6
                color: collapseMa.containsMouse ? AuroraTheme.sidebarBgElev2 : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
                Text {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -1
                    text: "‹"
                    color: AuroraTheme.sidebarInk3
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                MouseArea {
                    id: collapseMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleCollapseRequested()
                    ToolTip.visible: containsMouse
                    ToolTip.delay: 500
                    ToolTip.text: qsTr("Thu gọn sidebar")
                }
            }
        }

        // Rail-mode expand button — sits below the brand mark so it doesn't
        // crowd the 28×28 brand square in a 64px column.
        Rectangle {
            visible: root.collapsed
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            Layout.bottomMargin: 12
            radius: 6
            color: railExpandMa.containsMouse ? AuroraTheme.sidebarBgElev2 : "transparent"
            Behavior on color { enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast } }
            Text {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -1
                text: "›"
                color: AuroraTheme.sidebarInk3
                font.family: AuroraTheme.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            MouseArea {
                id: railExpandMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.toggleCollapseRequested()
                ToolTip.visible: containsMouse
                ToolTip.delay: 500
                ToolTip.text: qsTr("Mở rộng sidebar")
            }
        }

        // ══════════════════════════════════════════════
        //  NAV — Workspace
        // ══════════════════════════════════════════════
        Repeater {
            model: [
                { label: qsTr("Trang chủ"), icon: "house",  index: Pages.home,      sub: "" },
                { label: qsTr("My Files"),  icon: "folder", index: Pages.files,     sub: "" },
                { label: qsTr("Yêu thích"), icon: "heart",  index: Pages.favorites, sub: "" }
            ]
            delegate: NavRow {}
        }

        // Thin separator — 1px × 90% opacity .6. The marker between
        // workspace and activity, replaces the old uppercase group label.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            color: AuroraTheme.sidebarLine
            opacity: 0.6
        }

        // ══════════════════════════════════════════════
        //  NAV — Activity (Tải xuống · Tải lên · Đồng bộ)
        //  Sync row shows pending count + a small animated dot when sync is
        //  in progress.
        // ══════════════════════════════════════════════
        Repeater {
            model: [
                { label: qsTr("Tải xuống"), icon: "arrow-down", index: Pages.download, sub: "" },
                { label: qsTr("Tải lên"),   icon: "arrow-up",   index: Pages.upload,   sub: "" },
                { label: qsTr("Đồng bộ"),   icon: "sync",       index: Pages.sync,
                  sub: root.syncPendingCount <= 0
                      ? "" : (root.syncPendingCount > 99 ? "99+" : String(root.syncPendingCount)),
                  pulse: root.syncPendingCount > 0 }
            ]
            delegate: NavRow {}
        }

        // (Showcase route removed — it's a design-system demo page, not a
        // user destination. Devs can still reach it via the command palette
        // /go.showcase if they wire it up; sidebar real-estate is better
        // spent on the HUD card below.)

        // ── Filler — pushes HUD + avatar pivot to the bottom ────────
        Item { Layout.fillHeight: true }

        // ══════════════════════════════════════════════
        //  MINI HUD — visible only when a transfer is moving / queued.
        //  Hidden entirely in rail mode (the 64px column is too narrow).
        // ══════════════════════════════════════════════
        FsMiniHud {
            id: hud
            visible: !root.collapsed && root.transferStatsVisible
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            Layout.bottomMargin: visible ? 10 : 0
        }

        // ══════════════════════════════════════════════
        //  UPGRADE CARD (free tier · expanded mode only)
        //  Preserved for compatibility — the design's avatar popover lists
        //  the user's plan, but the "Nâng cấp" promo card still belongs at
        //  the bottom of the rail because it's an active CTA, not a meta
        //  surface. We only show it when:
        //    • isVip = false (paid users already see no value-prop)
        //    • collapsed = false (rail mode can't fit the body copy)
        //    • HUD is not visible (avoid stacking two cards above the avatar)
        // ══════════════════════════════════════════════
        Rectangle {
            id: upgradeCard
            visible: !root.isVip && !root.collapsed && !hud.visible
            Layout.fillWidth: true
            Layout.bottomMargin: visible ? 10 : 0
            Layout.preferredHeight: visible ? upgradeCol.implicitHeight + 22 : 0
            radius: 12
            color: AuroraTheme.sidebarBgElev
            border.width: 1
            border.color: AuroraTheme.sidebarLine

            ColumnLayout {
                id: upgradeCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 12
                anchors.rightMargin: 12
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
                    color: AuroraTheme.sidebarInk
                    Layout.topMargin: 2
                }
                Text {
                    text: qsTr("1TB · tốc độ không giới hạn")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 11
                    color: AuroraTheme.sidebarInk3
                    Layout.topMargin: 1
                }

                FsGradientRect {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    Layout.topMargin: 10
                    radius: 8
                    stop0: AuroraTheme.accent3
                    stop1: AuroraTheme.accent
                    stop2: AuroraTheme.accent2
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
        //  AVATAR PIVOT
        //  Click → toggle popover above (Tài khoản · Cài đặt · Đăng xuất)
        //  In rail mode: just the initials circle, click logs out via the
        //  legacy logoutClicked() signal (keeps muscle memory for power-rail
        //  users).
        // ══════════════════════════════════════════════
        AvatarPivot {
            visible: !root.collapsed
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 42 : 0
        }

        // Rail-mode avatar — initials circle centred. Single click → emit
        // navClicked(account) so the user lands on their account page.
        FsGradientRect {
            visible: root.collapsed
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 36
            Layout.preferredHeight: visible ? 36 : 0
            radius: 18
            stop0: AuroraTheme.accent3
            stop1: AuroraTheme.accent
            stop2: AuroraTheme.accent2
            Text {
                anchors.centerIn: parent
                text: root._initials(root.userName)
                color: "#FFFFFF"
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: Font.Bold
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.navClicked(Pages.account)
                ToolTip.visible: containsMouse
                ToolTip.delay: 400
                ToolTip.text: root.userName.length > 0 ? root.userName : qsTr("Tài khoản")
            }
        }
    }

    // ── Helpers ─────────────────────────────────────────────────────────
    function _initials(name) {
        if (!name || name.length === 0) return "U";
        const parts = name.trim().split(/\s+/);
        if (parts.length === 1) return parts[0].charAt(0).toUpperCase();
        return (parts[0].charAt(0) + parts[parts.length - 1].charAt(0)).toUpperCase();
    }

    // ── NavRow component ────────────────────────────────────────────────
    component NavRow: Rectangle {
        id: navRow
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        radius: 8

        readonly property bool active: root.currentPage === modelData.index
        readonly property bool _hot: navMa.containsMouse || navRow.activeFocus
        // Optional model field — "pulse" is true on the Sync row when there
        // is pending work; surfaces a tiny green dot on the icon.
        readonly property bool _pulse: ("pulse" in modelData) ? modelData.pulse : false

        color: active ? AuroraTheme.sidebarBgElev2
                      : (_hot ? AuroraTheme.sidebarBgElev : "transparent")
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        // ── Accessibility / keyboard ────────────────────────────────
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
                const sibs = parent.children;
                const idx = sibs.indexOf(navRow);
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

        // Focus ring — subtle outline so keyboard users see where they are.
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 7
            color: "transparent"
            border.width: navRow.activeFocus ? 2 : 0
            border.color: AuroraTheme.sidebarInk3
            visible: navRow.activeFocus
        }

        // Active marker — 3×16 vermilion pill at the very left edge. The
        // pill sits BEFORE the row's left padding so the row text doesn't
        // need to shift when active/inactive. Glow via a soft halo behind.
        Rectangle {
            visible: navRow.active
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: -14
            width: 3
            height: 16
            radius: 2
            color: AuroraTheme.accent
            // Halo
            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: 4
                color: AuroraTheme.accent
                opacity: 0.35
                z: -1
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 10

            // Icon + optional sync-pulse dot
            Item {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                Layout.alignment: root.collapsed ? Qt.AlignHCenter : Qt.AlignVCenter
                FsIcon {
                    anchors.fill: parent
                    name: modelData.icon
                    sizePx: 16
                    color: navRow.active ? AuroraTheme.accent
                                         : (navRow._hot ? AuroraTheme.sidebarInk2
                                                        : AuroraTheme.sidebarInk3)
                    Behavior on color { enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast } }
                }
                Rectangle {
                    visible: navRow._pulse
                    width: 6; height: 6; radius: 3
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: -2
                    anchors.bottomMargin: -2
                    color: AuroraTheme.auroraSuccess
                    property real _ph: 1
                    opacity: _ph
                    Timer {
                        running: navRow._pulse && !AuroraTheme.reduceMotion
                        repeat: true
                        interval: 700
                        triggeredOnStart: true
                        onTriggered: parent._ph = (parent._ph < 1 ? 1 : 0.4)
                    }
                    Behavior on opacity { enabled: !AuroraTheme.reduceMotion
                        NumberAnimation { duration: 700 } }
                }
            }

            Text {
                visible: !root.collapsed
                Layout.fillWidth: true
                text: modelData.label
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: navRow.active ? Font.DemiBold : Font.Normal
                color: navRow.active ? AuroraTheme.sidebarInk
                                     : (navRow._hot ? AuroraTheme.sidebarInk2
                                                    : AuroraTheme.sidebarInk2)
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
            }

            Text {
                visible: !root.collapsed && modelData.sub && modelData.sub.length > 0
                text: modelData.sub || ""
                font.family: AuroraTheme.fontMono
                font.pixelSize: 11
                font.weight: navRow.active ? Font.DemiBold : Font.Normal
                color: navRow.active ? AuroraTheme.sidebarInk : AuroraTheme.sidebarInk4
                Behavior on color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }
            }
        }

        MouseArea {
            id: navMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.navClicked(modelData.index)
            ToolTip.visible: root.collapsed && containsMouse
            ToolTip.delay: 400
            ToolTip.text: modelData.label
        }
    }

    // ── Avatar pivot ────────────────────────────────────────────────────
    // Sits at the bottom of the sidebar with the user's name + plan label.
    // Click toggles a popover above with 3 items (Tài khoản · Cài đặt ·
    // Đăng xuất). Clicking outside the popover closes it.
    component AvatarPivot: Item {
        id: pivot
        property bool open: false

        // Avatar button (always visible)
        Rectangle {
            id: pivotBtn
            anchors.fill: parent
            radius: 10
            color: AuroraTheme.sidebarBgElev
            border.width: 1
            border.color: pivot.open ? AuroraTheme.sidebarLineStrong : AuroraTheme.sidebarLine
            Behavior on border.color { enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast } }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 10
                spacing: 10

                // Initials avatar
                FsGradientRect {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    radius: 13
                    stop0: AuroraTheme.accent3
                    stop1: AuroraTheme.accent
                    stop2: AuroraTheme.accent2
                    Text {
                        anchors.centerIn: parent
                        text: root._initials(root.userName)
                        color: "#FFFFFF"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        font.weight: Font.Bold
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        Layout.fillWidth: true
                        text: root.userName.length > 0 ? root.userName : qsTr("Người dùng")
                        color: AuroraTheme.sidebarInk
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        // Combine VIP label + storage strip so the user gets
                        // both their tier and "287 / 1024 GB" in a single
                        // mono line. Falls back gracefully when fields are
                        // empty (pre-/profile state).
                        text: {
                            const tier = root.vipLabel.length > 0 ? root.vipLabel : qsTr("Miễn phí");
                            const used = root.storageUsedText;
                            const total = root.storageTotalText;
                            if (used.length === 0 && total.length === 0) return tier;
                            return tier + " · " + used + (total.length > 0 ? " / " + total : "");
                        }
                        color: AuroraTheme.sidebarInk4
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }

                Text {
                    text: "⌄"
                    color: AuroraTheme.sidebarInk3
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignVCenter
                    rotation: pivot.open ? 180 : 0
                    Behavior on rotation { enabled: !AuroraTheme.reduceMotion
                        NumberAnimation { duration: AuroraTheme.durFast } }
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: pivot.open = !pivot.open
            }
        }

        // Popover — opens UPWARD so it never spills off the bottom of the
        // sidebar.  Width matches the pivot; offset 8px above.  Animates in
        // with a slide-up + fade so the open/close transition feels deliberate
        // rather than abrupt.  Header (avatar + name + tier) is intentionally
        // omitted — that information is already on the avatar pivot directly
        // below; duplicating it doubles the visual weight for no new content.
        Rectangle {
            id: popover
            // `visible` is driven by the opacity animation: keep the item
            // around while fading so the slide-out actually plays before
            // disappearing.  When opacity hits 0, `visible: false` releases
            // the hit-test area.
            visible: opacity > 0.01
            opacity: pivot.open ? 1 : 0
            // 8px slide travel — small enough to feel like a polish rather
            // than a transition, fast enough not to delay interaction.
            transform: Translate { y: pivot.open ? 0 : 8 }
            Behavior on opacity { enabled: !AuroraTheme.reduceMotion
                NumberAnimation { duration: AuroraTheme.durFast; easing.type: AuroraTheme.easingStd } }

            anchors.left: pivot.left
            anchors.right: pivot.right
            anchors.bottom: pivot.top
            anchors.bottomMargin: 8
            radius: 12
            color: AuroraTheme.sidebarBgElev
            border.color: AuroraTheme.sidebarLineStrong
            border.width: 1
            height: popCol.implicitHeight + 16
            // Lift above the avatar pivot itself + brand mark.
            z: 100

            ColumnLayout {
                id: popCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                // Per-row spacing widens breathing room between menu entries
                // so each action reads as a distinct affordance.
                spacing: 2

                PopItem {
                    icon: "user-circle"
                    label: qsTr("Thông tin tài khoản")
                    onActivated: { pivot.open = false; root.navClicked(Pages.account); }
                }
                PopItem {
                    icon: "gear"
                    label: qsTr("Cài đặt")
                    hint: "Ctrl ,"
                    onActivated: { pivot.open = false; root.navClicked(Pages.settings); }
                }

                // Free-tier popover gets an extra "Nâng cấp" item — gives
                // users a 2nd entry point to the upgrade flow even when the
                // promo card is hidden by an active HUD.
                PopItem {
                    visible: !root.isVip
                    icon: "sparkle"
                    label: qsTr("Nâng cấp tài khoản")
                    hint: qsTr("VIP Pro")
                    onActivated: { pivot.open = false; root.upgradeClicked(); }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    color: AuroraTheme.sidebarLine
                }

                PopItem {
                    icon: "power"
                    label: qsTr("Đăng xuất")
                    danger: true
                    onActivated: { pivot.open = false; root.logoutClicked(); }
                }
            }
        }

        // Auto-close — popover closes when currentPage changes (a nav
        // action came in from anywhere). An overlay MouseArea to catch
        // outside clicks sounds tempting but it tends to steal clicks
        // meant for the popover items themselves (z-order across nested
        // subtrees is brittle). Toggling via the avatar button is good
        // enough UX for this surface.
        Connections {
            target: root
            function onCurrentPageChanged() { pivot.open = false }
        }
    }

    // ── Popover item component ──────────────────────────────────────────
    // 40px row · 14px side padding gives the popover a less-cramped feel
    // than the old 32×10 layout — actions read as distinct destinations
    // rather than a stacked menu.
    component PopItem: Rectangle {
        id: popItem
        property string icon: ""
        property string label: ""
        property string hint: ""
        property bool   danger: false
        signal activated()

        Layout.fillWidth: true
        Layout.preferredHeight: visible ? 40 : 0
        radius: 8
        color: popItemMa.containsMouse ? AuroraTheme.sidebarBgHover : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 12
            FsIcon {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                name: popItem.icon
                sizePx: 16
                color: popItem.danger ? AuroraTheme.auroraDanger
                                      : (popItemMa.containsMouse ? AuroraTheme.sidebarInk2
                                                                 : AuroraTheme.sidebarInk3)
            }
            Text {
                Layout.fillWidth: true
                text: popItem.label
                color: popItem.danger ? AuroraTheme.auroraDanger
                                      : (popItemMa.containsMouse ? AuroraTheme.sidebarInk
                                                                 : AuroraTheme.sidebarInk2)
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: popItemMa.containsMouse ? Font.DemiBold : Font.Normal
            }
            Text {
                visible: popItem.hint.length > 0
                text: popItem.hint
                color: AuroraTheme.sidebarInk4
                font.family: AuroraTheme.fontMono
                font.pixelSize: 10
            }
        }

        MouseArea {
            id: popItemMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: popItem.activated()
        }
    }
}
