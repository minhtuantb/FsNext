// SPDX-License-Identifier: Proprietary
// TransferHudPanel — shared content for the Tray Popup (P1) and the
// floating Mini Window (P2).  Pure content, no host chrome (no frameless
// Window, no drag handle, no auto-dismiss) — the parent decides those.
//
// Layout matches the mockup at mockup/transfer-hud-preview.html.
//
// Two display modes via `compact`:
//   compact=true   →  tray popup style: 44px rows, no sparkline, no
//                     pause-all/close buttons (right-click menu does that)
//   compact=false  →  mini window style: 52px rows, sparkline placeholder
//                     (real sparkline lands in P2), pause-all + close
//
// Data feed: transferHudViewModel context property (set by AppContext).
//
// Signals — host listens to convert these into window-level actions:
//   expandClicked() — user clicked ⛶ in the header
//   closeClicked()  — user clicked ✕ in the header
//   rowClicked(id)  — user clicked a row body (NOT a per-row button)

import QtQuick
import QtQuick.Controls       // ToolButton + ToolTip (header buttons, row actions)
import QtQuick.Layouts
import Fshare.Components 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: panel

    // ── Public API ────────────────────────────────────────────────────
    property bool compact: false
    property bool showHeader: true
    property bool showFooter: true
    property bool showCloseButton: !compact
    property bool showPauseButton: !compact
    // When false, the panel skips painting its own opaque surface + border.
    // The floating mini window is a translucent "glass" host that draws the
    // surface itself (with state-driven alpha) — a second opaque layer here
    // would defeat the see-through effect. Text/content still composite crisp
    // on top of the host's translucent fill. Standalone hosts (test harness)
    // keep drawSurface=true so the panel renders a complete card.
    property bool drawSurface: true

    // Aurora widget size tier — "collapsed" / "normal" / "expanded". Host
    // sets this; panel reacts by hiding the speedmeter section + file
    // list in collapsed mode (the inline speed in the header takes over).
    // Normal vs expanded share the same content density — width differs
    // at the host level. Separate from `compact` (which is the retired
    // tray-popup variant that hides chrome buttons entirely).
    property string widgetSize: "normal"
    readonly property bool _isCollapsed: widgetSize === "collapsed"
    // VM injection — default to context property `transferHudViewModel`
    // (registered by AppContext::registerQml).  Tests or alternate hosts
    // can override.
    property var vm: typeof transferHudViewModel !== "undefined"
                     ? transferHudViewModel : null

    signal expandClicked()
    signal closeClicked()
    signal rowClicked(string taskId)
    // Footer "+ Dán link" CTA — host wires this to the existing paste
    // flow (downloadViewModel.clipboardText → routePastedText in Main.qml)
    // so the widget can kick off a download without opening the main UI.
    signal pasteLinkClicked()
    // Header size-cycle button — emits a "cycle to next size tier" request
    // (normal → expanded → collapsed → normal). Host (MiniHudWindow) owns
    // the actual widgetSize state since it also drives the window width;
    // panel just emits the intent.
    signal cycleSizeRequested()

    // Sizing — host gives us a fixed width; height is derived from content
    // so popup / mini window can size to fit.
    implicitWidth: compact ? 296 : 328
    implicitHeight: layout.implicitHeight + AuroraTheme.sp4 * 2

    // Card background — host (popup/mini window) draws its own outer
    // frame, but we paint our own surface here so the panel can sit on
    // a plain Window too (testing harness).
    Rectangle {
        visible: panel.drawSurface
        anchors.fill: parent
        color: AuroraTheme.panel
        radius: AuroraTheme.radiusLg
        border.color: AuroraTheme.border
        border.width: 1
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: AuroraTheme.sp4
        spacing: AuroraTheme.sp2

        // ── Header ───────────────────────────────────────────────────
        RowLayout {
            visible: showHeader
            Layout.fillWidth: true
            spacing: AuroraTheme.sp2

            // ── Aurora brand mark ────────────────────────────────────
            // 22×22 rounded square (7px) with the signature gradient and
            // "Fs" set in Instrument Serif italic, white. Vertical
            // Rectangle gradient is close enough to the 135° spec at this
            // tiny size — the diagonal feel comes from the stop placement
            // (pink-top, orange-mid, mango-bottom) rather than the exact
            // angle. Soft accent shadow grounds it on the surface.
            Rectangle {
                Layout.preferredWidth: 22
                Layout.preferredHeight: 22
                radius: AuroraTheme.radiusChrome
                gradient: Gradient {
                    GradientStop { position: 0.0; color: AuroraTheme.accent3 }   // pink
                    GradientStop { position: 0.5; color: AuroraTheme.accent  }   // orange
                    GradientStop { position: 1.0; color: AuroraTheme.accent2 }   // mango
                }
                // Soft warm halo so the mark sits ON the surface, not in it.
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    z: -1
                    radius: parent.radius + 2
                    color: "transparent"
                    border.color: Qt.rgba(AuroraTheme.accent.r,
                                          AuroraTheme.accent.g,
                                          AuroraTheme.accent.b, 0.30)
                    border.width: 1
                }
                Text {
                    anchors.centerIn: parent
                    text: "Fs"
                    color: "#FFFFFF"
                    font.family: AuroraTheme.fontSerif
                    font.pixelSize: 14
                    font.italic: true
                    font.weight: Font.Normal
                }
            }

            // Wordmark "Fshare" — ink1 strong with tight letter-spacing
            // per Aurora spec; brand-accent text would clash with the Fs
            // mark right next to it.
            Text {
                text: "Fshare"
                color: AuroraTheme.ink1
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: Font.DemiBold
                font.letterSpacing: -0.1
            }

            // ── LIVE / IDLE / PAUSED pill ────────────────────────────
            // Pulsing green pill while transfers run; muted ink4 chip when
            // idle; warn-amber when explicitly paused. Always present so
            // the user has a constant connection-state read-out.
            Rectangle {
                id: livePill
                readonly property bool isRunning: panel.vm && panel.vm.runState === "running"
                readonly property bool isPaused:  panel.vm && panel.vm.runState === "paused"
                readonly property color stateColor:
                      isRunning ? AuroraTheme.success
                    : isPaused  ? AuroraTheme.warn
                    : AuroraTheme.ink4
                Layout.preferredHeight: 18
                Layout.preferredWidth: pillRow.implicitWidth + 12
                radius: AuroraTheme.radiusPill
                color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b, 0.10)
                Behavior on color {
                    enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast }
                }
                RowLayout {
                    id: pillRow
                    anchors.centerIn: parent
                    spacing: 5
                    Rectangle {
                        id: pillDot
                        Layout.preferredWidth: 5
                        Layout.preferredHeight: 5
                        radius: 999
                        color: livePill.stateColor
                        Behavior on color {
                            enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast }
                        }
                        // Aurora pulse — 1.6 s round-trip per spec. Only
                        // animates while genuinely live; idle/paused dot
                        // sits at full opacity.
                        SequentialAnimation on opacity {
                            running: livePill.isRunning && !AuroraTheme.reduceMotion
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.30; duration: 800; easing.type: Easing.InOutSine }
                            NumberAnimation { from: 0.30; to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                        }
                    }
                    Text {
                        text: livePill.isRunning ? "LIVE"
                            : livePill.isPaused  ? "PAUSED"
                            : "IDLE"
                        color: livePill.stateColor
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 10   // pixelSize is int; 9.5 was rejected at QML runtime
                        font.weight: Font.Bold
                        font.letterSpacing: 0.6
                    }
                }
            }

            Item { Layout.fillWidth: true; Layout.minimumWidth: AuroraTheme.sp2 }

            // Header inline speed — shown when the speedmeter section is
            // hidden (compact tray popup OR collapsed widget). Hidden in
            // normal/expanded where the speedmeter hero takes over.
            // Layout.fillWidth removed (it conflicted with the flex Item
            // sibling — both fillWidth in a RowLayout caused unstable
            // layout). Acceptable trade-off: long speed strings may run
            // tight against the chrome buttons at 320px width.
            Text {
                visible: (panel.compact || panel._isCollapsed) && text.length > 0
                Layout.alignment: Qt.AlignRight
                color: panel.vm && panel.vm.runState === "running"
                       ? AuroraTheme.accent : AuroraTheme.ink2
                Behavior on color {
                    enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast }
                }
                font.family: AuroraTheme.fontMono
                font.pixelSize: 12
                font.weight: Font.DemiBold
                text: {
                    if (!panel.vm) return "";
                    const dl = panel.vm.totalDownloadSpeedText;
                    const ul = panel.vm.totalUploadSpeedText;
                    if (dl.length === 0 && ul.length === 0)
                        return panel._runStateLabel();
                    const parts = [];
                    if (dl.length > 0) parts.push("↓ " + dl);
                    if (ul.length > 0) parts.push("↑ " + ul);
                    return parts.join("  ·  ");
                }
            }

            // Size-cycle button — toggles between collapsed / normal /
            // expanded widget sizes. Glyph reflects the NEXT action so
            // the user can predict the click: down-chevron when there's
            // a smaller size to drop to, up-chevron when collapsed
            // (next click goes back up). Hidden in compact (tray popup)
            // because that surface has no resize concept.
            ToolButton {
                id: sizeCycleBtn
                visible: !panel.compact
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: {
                    if (panel.widgetSize === "collapsed") return qsTr("Mở rộng widget");
                    if (panel.widgetSize === "expanded")  return qsTr("Thu nhỏ widget");
                    return qsTr("Đổi kích thước widget");
                }
                ToolTip.delay: 400
                onClicked: panel.cycleSizeRequested()
                contentItem: Text {
                    text: panel.widgetSize === "collapsed" ? "▾" : "▴"
                    color: AuroraTheme.ink3
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: AuroraTheme.radiusChrome
                    color: sizeCycleBtn.hovered ? AuroraTheme.accentTint10
                                                 : "transparent"
                    Behavior on color {
                        enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast }
                    }
                }
            }

            // Pause-all toggle. Hidden in compact mode (popup uses
            // right-click tray menu for the same action).
            ToolButton {
                id: pauseBtn
                visible: showPauseButton && vm && vm.runState !== "idle"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: vm && vm.runState === "running"
                              ? qsTr("Tạm dừng tất cả")
                              : qsTr("Tiếp tục tất cả")
                ToolTip.delay: 400
                onClicked: {
                    if (!vm) return;
                    if (vm.runState === "running") vm.pauseAll();
                    else                           vm.resumeAll();
                }
                contentItem: Text {
                    text: vm && vm.runState === "running" ? "⏸" : "▶"
                    color: vm && vm.runState === "running"
                           ? AuroraTheme.ink2 : AuroraTheme.success
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: pauseBtn.hovered ? AuroraTheme.accentTint10
                                            : "transparent"
                }
            }

            // Expand → open main window.  Always visible.
            ToolButton {
                id: expandBtn
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Mở cửa sổ chính")
                ToolTip.delay: 400
                onClicked: panel.expandClicked()
                contentItem: Text {
                    text: "⛶"
                    color: AuroraTheme.ink2
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: expandBtn.hovered ? AuroraTheme.accentTint10
                                             : "transparent"
                }
            }

            // Close — dismiss the host window (not quit app).  Mini window
            // hides; popup gets hidden too but P1 popup also closes on
            // focus-out so this button is largely redundant there. Kept
            // for symmetry & accessibility (Esc could also be wired here).
            ToolButton {
                id: closeBtn
                visible: showCloseButton
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Đóng")
                ToolTip.delay: 400
                onClicked: panel.closeClicked()
                contentItem: Text {
                    text: "✕"
                    color: closeBtn.hovered ? AuroraTheme.danger
                                            : AuroraTheme.ink3
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: closeBtn.hovered ? AuroraTheme.dangerSoft
                                            : "transparent"
                }
            }
        }

        // ── Speedmeter Hero (Aurora Glow) ────────────────────────────
        // Three-element cluster: section label + peak readout, the BIG
        // hero number with serif "MB/s" unit + ETA aggregate right, and
        // the live waveform below. Mirrors Aurora widget anatomy items
        // #5 and #6. Hidden in compact mode (collapsed tray popup) where
        // the speed appears inline in the header instead.
        ColumnLayout {
            id: speedmeter
            visible: !panel.compact && !panel._isCollapsed
            Layout.fillWidth: true
            Layout.topMargin: AuroraTheme.sp2
            spacing: 2

            // Top label row — "TỔNG TỐC ĐỘ ↓" + "đỉnh X MB/s" right.
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("TỔNG TỐC ĐỘ ↓")
                    color: AuroraTheme.ink3
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 1.6
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: panel._peakSpeedText.length > 0
                    text: qsTr("đỉnh ") + panel._peakSpeedText
                    color: AuroraTheme.ink4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                }
            }

            // Hero row — big number + serif unit + ETA.
            // Renders as SOLID AuroraTheme.accent (running) / ink2 (idle).
            // The "gradient text" appearance in the design spec was
            // implemented via MultiEffect mask in an earlier pass but
            // that hit GPU-driver edge cases on some user systems and was
            // reverted in favour of stability. The Aurora gradient
            // identity is still carried through the brand mark, waveform
            // stroke/fill, progress bars, and warm halo. Future revisit
            // tracked in project-transfer-hud-spec memory.
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 2
                spacing: 6

                Text {
                    text: panel._aggSpeedNum
                    color: panel.vm && panel.vm.runState === "running"
                           ? AuroraTheme.accent : AuroraTheme.ink2
                    Behavior on color {
                        enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durBase }
                    }
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 44
                    font.weight: Font.DemiBold
                    font.letterSpacing: -1.5
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    text: panel._aggSpeedUnit
                    color: AuroraTheme.ink3
                    font.family: AuroraTheme.fontSerif
                    font.italic: true
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 6
                }
                Item { Layout.fillWidth: true }
                ColumnLayout {
                    spacing: 1
                    Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                    Layout.bottomMargin: 4
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("ETA tổng")
                        color: AuroraTheme.ink4
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 10
                    }
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: panel._aggEtaText.length > 0
                              ? panel._aggEtaText : "—"
                        color: panel._aggEtaText.length > 0
                               ? AuroraTheme.ink1 : AuroraTheme.ink4
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                }
            }

            // Live waveform — Sparkline pulled UP against the hero so the
            // chart reads as the trail behind the current speed rather
            // than an independent strip. 36px height + 4px topMargin
            // tightens the visual coupling.
            Aurora.Sparkline {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                Layout.topMargin: 4
                data: panel.vm ? panel.vm.speedHistory : []
                lineColor: AuroraTheme.accent
                fillColor: AuroraTheme.accent
            }
        }

        Rectangle { // hairline divider — only when there are rows to separate
            visible: !panel.compact && !panel._isCollapsed && transferList.count > 0
            Layout.fillWidth: true
            Layout.topMargin: AuroraTheme.sp2
            Layout.preferredHeight: 1
            color: AuroraTheme.border
        }

        // ── Top-N transfer list ──────────────────────────────────────
        ListView {
            id: transferList
            visible: !panel._isCollapsed
            Layout.fillWidth: true
            Layout.preferredHeight: (!visible || count === 0) ? 0 : (count * rowHeight)
            interactive: false
            clip: true
            spacing: 0
            model: panel.vm ? panel.vm.topItems : null

            // Row height — bumped from 52 → 64 (mini) to fit the new
            // Aurora 2-stack content (icon + name + meta line) PLUS the
            // dedicated progress lane below. Compact stays 48 (popup
            // single-line layout). Without the bump the meta line "còn
            // 28:07" would clip into the progress bar.
            readonly property int rowHeight: panel.compact ? 48 : 64

            // _rowComponent is a sibling Component (declared later in this
            // file); QML id lookup spans the whole document so we reference
            // it directly. Using `panel._rowComponent` resolved to undefined
            // because Item has no such property — only the id binding works.
            delegate: _rowComponent
        }

        // ── Empty state ──────────────────────────────────────────────
        ColumnLayout {
            visible: !panel._isCollapsed && transferList.count === 0
            Layout.fillWidth: true
            Layout.topMargin: AuroraTheme.sp4
            Layout.bottomMargin: AuroraTheme.sp4
            spacing: AuroraTheme.sp1

            FsIcon {
                Layout.alignment: Qt.AlignHCenter
                name: "sparkle"
                sizePx: 32
                color: AuroraTheme.ink4
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Không có lượt chuyển nào")
                color: AuroraTheme.ink3
                font.family: AuroraTheme.fontSans
                font.pixelSize: 12
            }
        }

        // ── Overflow chip ────────────────────────────────────────────
        Rectangle {
            visible: !panel._isCollapsed && panel.vm && panel.vm.overflowCount > 0
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            radius: 8
            color: overflowMouse.containsMouse ? AuroraTheme.accentSoft
                                               : AuroraTheme.divider
            Behavior on color {
                enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp3
                anchors.rightMargin: AuroraTheme.sp3
                Text {
                    Layout.fillWidth: true
                    text: vm ? qsTr("+ %1 mục khác").arg(vm.overflowCount) : ""
                    color: overflowMouse.containsMouse
                            ? AuroraTheme.accent : AuroraTheme.ink3
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 12
                }
                Text {
                    text: "›"
                    color: overflowMouse.containsMouse
                            ? AuroraTheme.accent : AuroraTheme.ink4
                    font.pixelSize: 14
                    font.bold: true
                }
            }
            MouseArea {
                id: overflowMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: panel.expandClicked()
            }
        }

        // ── Footer band (Aurora Glow) ────────────────────────────────
        // Edge-to-edge warm strip with DL/UL stats on the left and the
        // "+ Dán link" outlined CTA on the right. Negative margins
        // counter the panel ColumnLayout's sp4 padding so the band
        // reaches the card's rounded corners (the host card clips the
        // overrun via its radius). Top hairline divider provides the
        // separator from the file list — no extra spacing token needed.
        Rectangle {
            id: footerBand
            visible: panel.showFooter
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.topMargin: AuroraTheme.sp2
            Layout.leftMargin: -AuroraTheme.sp4
            Layout.rightMargin: -AuroraTheme.sp4
            Layout.bottomMargin: -AuroraTheme.sp4
            color: AuroraTheme.bgWarm
            // Bottom corners follow the host card's outer radius so the
            // warm band meets the rounded card shape cleanly. Top corners
            // stay square — the top hairline divider sits flush against
            // the file list above. Qt 6.7+ per-corner radii.
            bottomLeftRadius:  AuroraTheme.radiusOuter
            bottomRightRadius: AuroraTheme.radiusOuter
            topLeftRadius: 0
            topRightRadius: 0

            // Top hairline — separates the warm band from the list.
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: AuroraTheme.border
            }

            // Reusable stat cluster — "icon · LABEL · value". Inline so
            // DL and UL share an exact visual contract. accent enables
            // the brand-orange icon tint when its count is non-zero.
            component FooterStat : RowLayout {
                property string glyph: ""
                property string label: ""
                property string value: ""
                property bool   accentEnabled: false
                spacing: 6
                Text {
                    text: glyph
                    color: accentEnabled ? AuroraTheme.accent : AuroraTheme.ink3
                    font.pixelSize: 11
                    font.bold: true
                    Behavior on color {
                        enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast }
                    }
                }
                Text {
                    text: label
                    color: AuroraTheme.ink3
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 0.6
                }
                Text {
                    text: value
                    color: AuroraTheme.ink1
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 12   // pixelSize is int; 11.5 was rejected at QML runtime
                    font.weight: Font.DemiBold
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp4
                anchors.rightMargin: AuroraTheme.sp4
                spacing: 14

                FooterStat {
                    glyph: "↓"
                    label: "DL"
                    value: {
                        if (typeof transferBudgetViewModel === "undefined" || !transferBudgetViewModel)
                            return "0 / 0";
                        return transferBudgetViewModel.activeDownloads
                             + " / " + transferBudgetViewModel.maxDownloads;
                    }
                    accentEnabled: typeof transferBudgetViewModel !== "undefined"
                                   && transferBudgetViewModel
                                   && transferBudgetViewModel.activeDownloads > 0
                }

                FooterStat {
                    glyph: "↑"
                    label: "UL"
                    value: {
                        if (typeof transferBudgetViewModel === "undefined" || !transferBudgetViewModel)
                            return "0 / 0";
                        return transferBudgetViewModel.activeUploads
                             + " / " + transferBudgetViewModel.maxUploads;
                    }
                    accentEnabled: typeof transferBudgetViewModel !== "undefined"
                                   && transferBudgetViewModel
                                   && transferBudgetViewModel.activeUploads > 0
                }

                // Sync indicator — slim chip slipped between the stats
                // and the CTA. Only present when there's pending sync work
                // (no permanent "0 đang chờ" parking spot).
                FooterStat {
                    visible: panel.vm && panel.vm.syncPending > 0
                    glyph: "⟲"
                    label: "SYNC"
                    value: panel.vm
                           ? qsTr("%1").arg(panel.vm.syncPending) : ""
                    accentEnabled: false   // sync sits in ink3 — neutral signal
                }

                Item { Layout.fillWidth: true }

                // ── "+ Dán link" CTA — outlined button per Aurora spec.
                // Emits pasteLinkClicked(); host wires it to the existing
                // clipboardText → routePastedText flow in Main.qml.
                Button {
                    id: pasteLinkBtn
                    Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignVCenter
                    hoverEnabled: true
                    onClicked: panel.pasteLinkClicked()
                    contentItem: RowLayout {
                        spacing: 5
                        Text {
                            text: "+"
                            color: pasteLinkBtn.hovered
                                   ? AuroraTheme.accent : AuroraTheme.ink2
                            font.pixelSize: 12
                            font.bold: true
                            Behavior on color {
                                enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast }
                            }
                        }
                        Text {
                            text: qsTr("Dán link")
                            color: pasteLinkBtn.hovered
                                   ? AuroraTheme.accent : AuroraTheme.ink2
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            Behavior on color {
                                enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast }
                            }
                        }
                    }
                    background: Rectangle {
                        radius: AuroraTheme.radiusChrome
                        color: pasteLinkBtn.hovered
                               ? AuroraTheme.bg : AuroraTheme.panel
                        border.color: pasteLinkBtn.hovered
                                      ? AuroraTheme.accent : AuroraTheme.border
                        border.width: 1
                        Behavior on border.color {
                            enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast }
                        }
                    }
                }
            }
        }
    }

    // ── Helpers ──────────────────────────────────────────────────────
    function _runStateLabel() {
        if (!vm) return "";
        if (vm.runState === "paused") return qsTr("⏸ Tạm dừng");
        if (vm.runState === "idle")   return qsTr("Sẵn sàng");
        return "";
    }

    // ── Speedmeter Hero helpers ──────────────────────────────────────
    // The VM exposes both directions as pre-formatted text ("10.9 MB/s").
    // Hero displays the number + unit separated, so we split on space.
    // Defaults to "0.0 MB/s" when idle so the hero never collapses to
    // empty layout — visual stability matters more than perfect accuracy
    // for an at-rest state.
    function _parseSpeed(s) {
        if (!s || s.length === 0) return { num: "0.0", unit: "MB/s" };
        const sp = s.indexOf(" ");
        if (sp <= 0) return { num: s, unit: "" };
        return { num: s.substring(0, sp), unit: s.substring(sp + 1) };
    }

    readonly property string _aggSpeedText: {
        if (!panel.vm) return "";
        const dl = panel.vm.totalDownloadSpeedText;
        const ul = panel.vm.totalUploadSpeedText;
        // Prefer DL — Fshare's primary direction. Show UL only when DL is
        // empty (pure upload session). When both are non-empty, the
        // sparkline already shows the aggregate; here we surface DL so
        // the hero number stays stable instead of flickering between
        // the two streams.
        if (dl.length > 0) return dl;
        if (ul.length > 0) return ul;
        return "";
    }
    readonly property string _aggSpeedNum:  _parseSpeed(_aggSpeedText).num
    readonly property string _aggSpeedUnit: _parseSpeed(_aggSpeedText).unit

    // Peak speed — max value seen in the rolling 60 s buffer. Bytes/sec
    // raw value formatted to a human string. Returns "" while the buffer
    // is empty/all-zero so the "đỉnh" label hides cleanly at rest.
    readonly property real _peakSpeedBps: {
        if (!panel.vm) return 0;
        const h = panel.vm.speedHistory;
        if (!h || h.length === 0) return 0;
        let m = 0;
        for (let i = 0; i < h.length; ++i) {
            const v = h[i] || 0;
            if (v > m) m = v;
        }
        return m;
    }
    readonly property string _peakSpeedText: _formatBps(_peakSpeedBps)

    function _formatBps(bps) {
        if (bps <= 0) return "";
        const KB = 1024, MB = KB * 1024, GB = MB * 1024;
        if (bps >= GB) return (bps / GB).toFixed(1) + " GB/s";
        if (bps >= MB) return (bps / MB).toFixed(1) + " MB/s";
        if (bps >= KB) return Math.round(bps / KB) + " KB/s";
        return Math.round(bps) + " B/s";
    }

    // Aggregate ETA — computed in C++ from Σ(remaining bytes) / Σ(speed)
    // of Active tasks and exposed as vm.aggEtaText. Empty string = idle
    // or no size info → QML hero falls back to "—".
    readonly property string _aggEtaText: panel.vm
                                          ? panel.vm.aggEtaText : ""

    // ── Row delegate ─────────────────────────────────────────────────
    // Pulled out so the ListView delegate definition stays readable.
    // Bindings reference model.* roles defined in TransferHudTopModel.
    Component {
        id: _rowComponent
        Rectangle {
            id: row
            width: ListView.view ? ListView.view.width : 0
            height: transferList.rowHeight
            // Aurora hover: row tints to the warm cream bgWarm token —
            // signals interactivity without competing with the brand
            // accent (which is reserved for the progress bar + active
            // icon ring). Failed rows keep dangerSoft as a persistent
            // attention tier.
            color: rowMouse.containsMouse ? AuroraTheme.bgWarm
                                          : (model.status === 4 /* Error */
                                              ? AuroraTheme.dangerSoft
                                              : "transparent")
            radius: 6
            Behavior on color {
                enabled: !AuroraTheme.reduceMotion
                ColorAnimation { duration: AuroraTheme.durFast }
            }

            // Highlight pulse hook — Main.qml stores the most recently
            // focused task id in a window-level property; we compare and
            // pulse for 600ms when it matches.  Property `_focusedTaskId`
            // is set on `panel` by the host (MiniHudWindow).
            property bool _isFocused: panel._focusedTaskId === model.taskId
                                       && model.taskId.length > 0
            border.width: _isFocused ? 2 : 0
            border.color: AuroraTheme.accent
            Behavior on border.width {
                enabled: !AuroraTheme.reduceMotion
                NumberAnimation { duration: AuroraTheme.durBase }
            }

            // Red border-left strip for failed rows.
            Rectangle {
                visible: model.status === 4
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 3
                color: AuroraTheme.danger
                radius: 6
            }

            // ── Two-row vertical layout: top content, progress lane ───
            // Per Aurora spec, the progress bar sits OUTSIDE the text
            // column (full row width, below name+meta+stats) so it reads
            // as a horizon line spanning the whole entry. This requires
            // a ColumnLayout body instead of the old "progress nested in
            // the text column" 3-row stack.
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: model.status === 4
                                    ? AuroraTheme.sp3 + 3 : AuroraTheme.sp3
                anchors.rightMargin: AuroraTheme.sp3
                anchors.topMargin: 9
                anchors.bottomMargin: 9
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    // ── Round Aurora icon ring ───────────────────────
                    // 22px circle (radius 999) instead of the previous
                    // rounded square. Two-tier palette mirrors the row
                    // delegate prior to this design pass:
                    //   ACTIVE / FAILED → solid semantic, white glyph
                    //   PAUSED / QUEUED / DONE → soft tint, colour glyph
                    // Active upload+download rows wear the BRAND gradient
                    // (cam→hồng→mango) instead of solid accent so the
                    // signature reads on every running line. Sync stays
                    // solid success; failed solid danger.
                    Rectangle {
                        id: rowIcon
                        Layout.preferredWidth: 22
                        Layout.preferredHeight: 22
                        Layout.alignment: Qt.AlignVCenter
                        radius: 999
                        readonly property bool _isActive: model.status === 1
                        readonly property bool _useGradient:
                            _isActive
                            && (model.direction === 0 || model.direction === 1)
                        color: {
                            if (rowIcon._useGradient) return "transparent";
                            if (model.status === 4) return AuroraTheme.danger;
                            if (rowIcon._isActive)   return AuroraTheme.success;
                            // Paused / queued / done — soft tint.
                            if (model.direction === 0) return AuroraTheme.infoSoft;
                            if (model.direction === 1) return AuroraTheme.accentTint10;
                            return AuroraTheme.successSoft;
                        }
                        Behavior on color {
                            enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast }
                        }
                        // Aurora gradient layer — only painted on active
                        // upload/download rows where it carries the brand.
                        Rectangle {
                            anchors.fill: parent
                            radius: 999
                            visible: rowIcon._useGradient
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: AuroraTheme.accent3 }
                                GradientStop { position: 0.5; color: AuroraTheme.accent  }
                                GradientStop { position: 1.0; color: AuroraTheme.accent2 }
                            }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: {
                                if (model.status === 4) return "⚠";
                                if (model.direction === 0) return "↓";
                                if (model.direction === 1) return "↑";
                                return "⟲";
                            }
                            color: {
                                // White on every saturated fill (gradient
                                // or solid). Coloured glyph only on the
                                // soft-tint waiting states.
                                if (model.status === 4) return "#FFFFFF";
                                if (rowIcon._isActive)  return "#FFFFFF";
                                if (model.direction === 0) return AuroraTheme.info;
                                if (model.direction === 1) return AuroraTheme.accent;
                                return AuroraTheme.success;
                            }
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    // ── Text column: filename + meta line ────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text {
                            Layout.fillWidth: true
                            text: model.fileName
                            elide: Text.ElideMiddle
                            color: AuroraTheme.ink1
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: panel.compact ? 12 : 12.5
                            font.weight: Font.Medium
                            font.letterSpacing: -0.05
                        }
                        // Meta line — Aurora spec: "384 MB / 6.4 GB · còn 28:07"
                        // when active. Falls back gracefully if bytes data
                        // is missing (folder scans pre-resolve, sync items
                        // without size). Non-active states show the status
                        // label or error message.
                        Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: {
                                if (model.status === 4)
                                    return model.errorMessage.length > 0
                                           ? model.errorMessage : qsTr("Lỗi");
                                if (model.status === 0) {
                                    return model.bytesText.length > 0
                                        ? model.bytesText + "  ·  " + qsTr("đang chờ")
                                        : qsTr("đang chờ");
                                }
                                if (model.status === 2) {
                                    return model.bytesText.length > 0
                                        ? model.bytesText + "  ·  " + qsTr("tạm dừng")
                                        : qsTr("tạm dừng");
                                }
                                if (model.status === 3) return qsTr("hoàn tất");
                                if (model.status === 1) {
                                    // Active — bytes + ETA, both if available.
                                    const parts = [];
                                    if (model.bytesText.length > 0) parts.push(model.bytesText);
                                    if (model.etaText.length > 0)   parts.push(qsTr("còn ") + model.etaText);
                                    return parts.join("  ·  ");
                                }
                                return "";
                            }
                            elide: Text.ElideRight
                            color: {
                                if (model.status === 4) return AuroraTheme.danger;
                                if (model.status === 3) return AuroraTheme.success;
                                return AuroraTheme.ink4;
                            }
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11   // pixelSize is int; 10.5 was rejected at QML runtime
                            font.weight: model.status === 3
                                         ? Font.DemiBold : Font.Normal
                        }
                    }

                    // ── Right-aligned stats: speed + pct ─────────────
                    ColumnLayout {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 1
                        Text {
                            Layout.alignment: Qt.AlignRight
                            visible: model.status === 1
                            text: model.speedText
                            color: AuroraTheme.ink1
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            font.letterSpacing: -0.1
                        }
                        Text {
                            Layout.alignment: Qt.AlignRight
                            text: Math.round(model.progress * 100) + "%"
                            color: AuroraTheme.accent
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                        }
                    }

                    // Per-row action button — appears only on hover for
                    // actionable states (active/paused/failed). Sits to
                    // the right of the stats column so it doesn't reshuffle
                    // the row layout when it appears/disappears (its
                    // 24×24 slot is always reserved via Layout properties;
                    // only the visible flag toggles).
                    ToolButton {
                        id: rowAction
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        Layout.alignment: Qt.AlignVCenter
                        visible: (rowMouse.containsMouse || panel.compact)
                                 && (model.status === 1 || model.status === 2
                                     || model.status === 4)
                        contentItem: Text {
                            text: model.status === 4 ? "⟲"
                                  : (model.status === 1 ? "⏸" : "▶")
                            color: AuroraTheme.ink2
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: AuroraTheme.radiusChrome
                            color: rowAction.hovered ? AuroraTheme.accentTint15
                                                     : "transparent"
                            Behavior on color {
                                enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast }
                            }
                        }
                        onClicked: {
                            if (!panel.vm) return;
                            if (model.status === 4)      panel.vm.resumeTask(model.taskId);
                            else if (model.status === 1) panel.vm.pauseTask(model.taskId);
                            else                         panel.vm.resumeTask(model.taskId);
                        }
                    }
                }

                // ── Progress lane (Aurora) ───────────────────────────
                // Thin horizontal bar SPANNING the row width with a
                // gradient fill + shimmer overlay sliding 1.4 s. Sits
                // BELOW the text content so the row reads top-to-bottom:
                // identity (icon + name) → state (meta) → progress (bar).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: panel.compact ? 3 : 4
                    radius: 999
                    color: "#F0EDE3"   // muted cream lane (warmer than border token)
                    clip: true

                    Rectangle {
                        id: barFill
                        width: parent.width * Math.max(0, Math.min(1, model.progress))
                        height: parent.height
                        radius: 999
                        readonly property color _barC0:
                            model.status === 4 ? AuroraTheme.danger
                            : (model.status === 2 || model.status === 0) ? AuroraTheme.ink4
                            : AuroraTheme.accent3
                        readonly property color _barC1:
                            model.status === 4 ? AuroraTheme.danger
                            : (model.status === 2 || model.status === 0) ? AuroraTheme.ink4
                            : AuroraTheme.accent2
                        readonly property color _barMid:
                            model.status === 4 ? AuroraTheme.danger
                            : (model.status === 2 || model.status === 0) ? AuroraTheme.ink4
                            : AuroraTheme.accent
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: barFill._barC0 }
                            GradientStop { position: 0.5; color: barFill._barC1 }
                            GradientStop { position: 1.0; color: barFill._barMid }
                        }
                        opacity: (model.status === 2 || model.status === 0) ? 0.5 : 1.0
                        Behavior on width {
                            enabled: !AuroraTheme.reduceMotion
                            NumberAnimation { duration: AuroraTheme.durBase }
                        }

                        // Aurora shimmer — translucent white band sliding
                        // L→R every 1.4 s. Only animates on ACTIVE rows
                        // (no shimmer on paused/queued/done — they're
                        // not "in motion"). Honours reduce-motion. Clipped
                        // by the parent lane so it never paints past the
                        // filled portion of the bar.
                        Rectangle {
                            id: shimmer
                            visible: model.status === 1 && !AuroraTheme.reduceMotion
                            width: 60
                            height: parent.height
                            y: 0
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: "transparent" }
                                GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.45) }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                            NumberAnimation on x {
                                running: shimmer.visible
                                from: -60
                                to: barFill.width
                                duration: 1400
                                loops: Animation.Infinite
                            }
                        }
                    }
                }

            }

            MouseArea {
                id: rowMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                // Bubble the click up so host can decide what to do
                // (typically: focusTask + close popup).
                onClicked: panel.rowClicked(model.taskId)
                // Don't block button hover/click on the right.
                propagateComposedEvents: true
            }
        }
    }

    // Window-scoped highlight tracker — host sets this when it dispatches
    // a focusTask navigation, the row delegate watches it for the pulse.
    property string _focusedTaskId: ""
    function pulseTask(taskId) {
        _focusedTaskId = taskId;
        focusTimer.restart();
    }
    Timer {
        id: focusTimer
        interval: 600
        repeat: false
        onTriggered: panel._focusedTaskId = ""
    }
}
