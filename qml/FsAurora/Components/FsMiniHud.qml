// SPDX-License-Identifier: Proprietary
// FsMiniHud — Swipeable mini-HUD card for the Aurora sidebar v2.
//
// Four canonical variants (chosen via pointer drag, ‹ ›, or dot click):
//   0 · Speed Pulse   — hero MB/s + live 24-bar sparkline + DL/UL/ETA chips
//   1 · Dual Rings    — DL ring (gradient) + UL ring (mint) bracketing the hero
//   2 · File Marquee  — current file name + 2px shimmer bar + speed/ETA
//   3 · Stat Grid     — 2×2 (DL, UL, ETA, today's GB)
//
// Animations are gated on `vm.activeTotal > 0` so an idle sidebar costs zero
// frame-tick budget — no shimmer loops, no pulse loops, no sparkline repaint
// when nothing is moving.
//
// Persistence: variant index lives in SettingsViewModel.sidebarHudVariant so
// the user's chosen card sticks across launches. Component is read-only when
// vm/settings are unavailable (e.g. design previews) — falls back to local
// `_localIdx` state.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme 1.0

Item {
    id: hud

    // ── Public API ──────────────────────────────────────────────────────
    // VM injection — defaults to context properties registered by AppContext.
    property var hudVm: typeof transferHudViewModel !== "undefined"
                       ? transferHudViewModel : null
    property var budgetVm: typeof transferBudgetViewModel !== "undefined"
                          ? transferBudgetViewModel : null
    property var settingsVm: typeof settingsViewModel !== "undefined"
                            ? settingsViewModel : null

    // True iff any transfer is moving or queued — drives animation gates so
    // the card costs nothing when idle.
    readonly property bool _hasActivity: hudVm
        && (hudVm.activeTotal + hudVm.pendingTotal) > 0

    // Card dimensions — bumped from the 86px handoff to 102px after field
    // testing showed data lines crowding together. Extra height gives the
    // hero · sparkline · footer-chip bands each their own breathing room
    // and lets the typography scale up one tier (10/11/22 → 11/13/24) so
    // the card reads from the corner of the eye.
    implicitWidth: 212
    implicitHeight: cardWrap.height + 22   // card + 6 (gap) + 16 (controls row)

    // Variant state — persisted via SettingsVM if available, else runtime
    // only.  Clamped to 0..3 so a stale persisted value can't OOB our array.
    property int _localIdx: 0
    readonly property int currentIdx: settingsVm
        ? Math.max(0, Math.min(3, settingsVm.sidebarHudVariant))
        : _localIdx
    function _setIdx(i) {
        const n = ((i % 4) + 4) % 4;
        if (settingsVm) settingsVm.sidebarHudVariant = n;
        else            _localIdx = n;
        // Reset the auto-rotate dwell window: a deliberate switch by the
        // user should always get the full 5s before the carousel moves on
        // on its own.  Toggling `running` is the canonical way to restart
        // a QML Timer.
        if (autoSwipeTimer.running) {
            autoSwipeTimer.running = false;
            autoSwipeTimer.running = true;
        }
    }
    function _next() { _setIdx(currentIdx + 1); }
    function _prev() { _setIdx(currentIdx - 1); }

    // ── Card surface + swipe area ───────────────────────────────────────
    Rectangle {
        id: cardWrap
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 102
        radius: 12
        color: AuroraTheme.sidebarBgElev
        border.color: AuroraTheme.sidebarLine
        border.width: 1
        clip: true

        // Top "accent line" — subtle horizontal gradient stripe at the very
        // top of the card; helps the card read as "alive" without animating.
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: AuroraTheme.accent }
                GradientStop { position: 1.0; color: "transparent" }
            }
            opacity: hud._hasActivity ? 0.5 : 0.2
            Behavior on opacity { enabled: !AuroraTheme.reduceMotion
                NumberAnimation { duration: AuroraTheme.durBase } }
        }

        // Variant carousel — only the active variant is painted (Loader),
        // so swapping is cheap and the inactive cards cost nothing.
        Loader {
            anchors.fill: parent
            anchors.margins: 1
            asynchronous: false
            sourceComponent: hud.currentIdx === 0 ? cmpSpeedPulse
                          : hud.currentIdx === 1 ? cmpDualRings
                          : hud.currentIdx === 2 ? cmpFileMarquee
                                                 : cmpStatGrid
        }

        // Pointer drag for swipe — threshold ±28px → next/prev variant.
        // hoverEnabled drives the auto-swipe Timer below: when the cursor
        // is anywhere on the card the rotation pauses, resuming on leave.
        MouseArea {
            id: swipeMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.OpenHandCursor
            property real _startX: -1
            preventStealing: false
            onPressed: (mouse) => { _startX = mouse.x; cursorShape = Qt.ClosedHandCursor; }
            onReleased: (mouse) => {
                cursorShape = Qt.OpenHandCursor;
                if (_startX < 0) return;
                const dx = mouse.x - _startX;
                if (Math.abs(dx) > 28) (dx < 0 ? hud._next() : hud._prev());
                _startX = -1;
            }
            onCanceled: { _startX = -1; cursorShape = Qt.OpenHandCursor; }
            ToolTip.visible: containsPress
            ToolTip.delay: 600
            ToolTip.text: qsTr("Kéo trái/phải hoặc click chấm để đổi dạng HUD")
        }
    }

    // ── Auto-swipe carousel ─────────────────────────────────────────────
    // Cycle to the next variant every 5 seconds so the four HUD layouts get
    // surfaced even when the user never interacts.  Three independent pause
    // conditions:
    //   • hover anywhere on the card (don't yank info the user is reading)
    //   • reduce-motion preference is on
    //   • no transfer activity (an idle card pulsing through variants is
    //     just noise — the values would all read "0/0")
    // Manual swipe / dot click / arrow click reset the elapsed-time bucket
    // by toggling running, so a deliberate switch always gets the full 5s
    // dwell before auto-rotation resumes.
    Timer {
        id: autoSwipeTimer
        interval: 5000
        repeat: true
        running: hud._hasActivity
                 && !swipeMa.containsMouse
                 && !AuroraTheme.reduceMotion
        onTriggered: hud._next()
    }

    // ── Controls row (‹ · dots · ›) ─────────────────────────────────────
    RowLayout {
        anchors.top: cardWrap.bottom
        anchors.topMargin: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        spacing: 6
        height: 16

        HudNavBtn { iconText: "‹"; onActivated: hud._prev() }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 14
            Row {
                anchors.centerIn: parent
                spacing: 4
                Repeater {
                    model: 4
                    delegate: Rectangle {
                        property bool _active: index === hud.currentIdx
                        width: _active ? 14 : 5
                        height: 5
                        radius: 3
                        color: _active ? AuroraTheme.accent : AuroraTheme.sidebarLineStrong
                        anchors.verticalCenter: parent.verticalCenter
                        Behavior on width { enabled: !AuroraTheme.reduceMotion
                            NumberAnimation { duration: AuroraTheme.durBase } }
                        Behavior on color { enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durBase } }
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -3   // generous hit-target
                            cursorShape: Qt.PointingHandCursor
                            onClicked: hud._setIdx(index)
                        }
                    }
                }
            }
        }

        HudNavBtn { iconText: "›"; onActivated: hud._next() }
    }

    // ── Tiny chevron button ─────────────────────────────────────────────
    component HudNavBtn: Rectangle {
        id: nb
        property string iconText: "‹"
        signal activated()
        Layout.preferredWidth: 18
        Layout.preferredHeight: 16
        radius: 5
        color: nbMa.containsMouse ? AuroraTheme.sidebarBgElev2 : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -1
            text: nb.iconText
            color: nbMa.containsMouse ? AuroraTheme.sidebarInk2 : AuroraTheme.sidebarInk4
            font.family: AuroraTheme.fontSans
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
        MouseArea {
            id: nbMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: nb.activated()
        }
    }

    // ── Helpers — bind-once formatted strings ───────────────────────────
    // The HUD shows aggregate speed; pull DL + UL from hudVm where present.
    // Speed is summed from speedHistory's last sample; FsFormat.bytes()
    // turns it into "10.9 MB/s" style strings.
    function _heroSpeedText() {
        if (!hudVm) return "0";
        const t = hudVm.totalDownloadSpeedText || hudVm.totalUploadSpeedText || "";
        // Strip the "/s" unit so the hero can render unit separately.
        // Both strings come from FsFormat.bytes(...)+"/s" — split on first space.
        if (t.length === 0) return "0";
        const sp = t.indexOf(' ');
        return sp > 0 ? t.substring(0, sp) : t;
    }
    function _heroSpeedUnit() {
        if (!hudVm) return "MB/s";
        const t = hudVm.totalDownloadSpeedText || hudVm.totalUploadSpeedText || "";
        const sp = t.indexOf(' ');
        return sp > 0 ? t.substring(sp + 1) : "MB/s";
    }
    function _etaText() {
        return hudVm && hudVm.aggEtaText && hudVm.aggEtaText.length > 0
            ? hudVm.aggEtaText : "—";
    }
    // Peek the first row of the HUD top-items model (current "headline"
    // transfer). Done via an invisible Repeater so we read role properties
    // through delegate bindings instead of the QAbstractItemModel C++ API
    // — clean, reactive, and respects role-name conventions.
    property string _topFile: ""
    property real   _topProgress: 0
    Repeater {
        id: topPeek
        visible: false
        model: hud.hudVm ? hud.hudVm.topItems : null
        delegate: Item {
            required property int index
            required property string fileName
            required property real progress
            visible: false
            onFileNameChanged: if (index === 0) hud._topFile = fileName
            onProgressChanged: if (index === 0) hud._topProgress = progress
            Component.onCompleted: if (index === 0) {
                hud._topFile = fileName;
                hud._topProgress = progress;
            }
        }
        onCountChanged: if (count === 0) { hud._topFile = ""; hud._topProgress = 0; }
    }
    // Counter formula — "đang chạy / tổng file trong hàng đợi" per direction.
    //   DL active = downloads currently moving bytes
    //   DL total  = active + queued downloads
    //   UL active = uploads currently moving bytes  (sync runs through the
    //               upload pipeline so syncActive is already inside
    //               activeUploads when a sync task holds an upload slot)
    //   UL total  = activeUploads + pendingUploads + syncPending
    //               (syncPending is the count of files the watcher has
    //                queued for sync but haven't been promoted to upload
    //                slots yet — counted with upload because sync IS upload)
    //
    // Previous formula divided by `maxDownloads/maxUploads` (the budget cap,
    // e.g. "/8") which conflated "how many slots can run in parallel" with
    // "how many files left to process". The latter is what users want to
    // see in the HUD.
    function _budget(which) {
        if (!budgetVm) return 0;
        const aDL = budgetVm.activeDownloads;
        const pDL = budgetVm.pendingDownloads;
        const aUL = budgetVm.activeUploads;
        const pUL = budgetVm.pendingUploads;
        const sP  = hudVm ? hudVm.syncPending : 0;
        switch (which) {
        case "actDL": return aDL;
        case "totDL": return aDL + pDL;
        case "actUL": return aUL;
        case "totUL": return aUL + pUL + sP;
        }
        return 0;
    }

    // ══════════════════════════════════════════════════════════════════
    //  VARIANT 0 · Speed Pulse — hero + live sparkline + DL/UL/ETA chips
    // ══════════════════════════════════════════════════════════════════
    Component {
        id: cmpSpeedPulse
        Item {
            // Header strip — "TỐC ĐỘ" + LIVE pulse dot
            RowLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 11
                anchors.leftMargin: 13
                anchors.rightMargin: 13
                spacing: 6
                Text {
                    text: qsTr("TỐC ĐỘ")
                    color: AuroraTheme.sidebarInk4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.letterSpacing: 1.4
                }
                Item { Layout.fillWidth: true }
                LiveDot { active: hud._hasActivity }
                Text {
                    text: hud._hasActivity ? qsTr("LIVE") : qsTr("IDLE")
                    color: hud._hasActivity ? AuroraTheme.auroraSuccess : AuroraTheme.sidebarInk4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.letterSpacing: 0.8
                }
            }

            // Hero number row — bumped one tier (22 → 24) now that there's
            // 14px more card height to play with.
            RowLayout {
                anchors.top: parent.top
                anchors.topMargin: 26
                anchors.left: parent.left
                anchors.leftMargin: 13
                spacing: 5
                Text {
                    text: hud._heroSpeedText()
                    color: AuroraTheme.sidebarInk
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.8
                }
                Text {
                    text: hud._heroSpeedUnit()
                    color: AuroraTheme.accent
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 4
                }
            }

            // Live 24-bar sparkline — re-uses hudVm.speedHistory.  Last bar
            // gets the accent color + glow.  Bumped to 22px tall (was 18)
            // for the new card height.
            Sparkbars {
                id: bars
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 13
                anchors.rightMargin: 13
                anchors.top: parent.top
                anchors.topMargin: 58
                height: 22
                samples: hud.hudVm ? hud.hudVm.speedHistory : []
            }

            // Footer chips — ↓ active/total  · ↑ active/total  · ETA …
            //
            // Plain-text rendering (not RichText) — RichText's implicitWidth
            // measurement under-reports the actual painted width, causing
            // adjacent chips to crash into each other at the queue-size
            // densities a real user sees (e.g. "↓ 8/749" + "↑ 0/0" + ETA).
            // Plain Text with single-color labels measures correctly, so
            // RowLayout's spacing actually separates them on screen.
            //
            // Long-queue safety: each chip is elided + the ETA chip pushed
            // to the right via a flex spacer.  When a 4-digit total would
            // still squeeze the row, the ETA elides first — preserving the
            // more time-critical "DL/UL" numbers.
            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 13
                anchors.rightMargin: 13
                anchors.bottomMargin: 11
                spacing: 10

                Text {
                    text: "↓ " + hud._budget("actDL") + "/" + hud._budget("totDL")
                    color: AuroraTheme.sidebarInk2
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }
                Text {
                    text: "↑ " + hud._budget("actUL") + "/" + hud._budget("totUL")
                    color: AuroraTheme.sidebarInk2
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }
                Item { Layout.fillWidth: true; Layout.minimumWidth: 4 }
                Text {
                    text: "ETA " + hud._etaText()
                    color: AuroraTheme.sidebarInk4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    Layout.maximumWidth: 70
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    //  VARIANT 1 · Dual Rings — DL outer (gradient) + UL inner (mint)
    // ══════════════════════════════════════════════════════════════════
    Component {
        id: cmpDualRings
        Item {
            // Twin ring — left side; pure paint, no animation.
            Canvas {
                id: rings
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 56; height: 56
                renderTarget:   Canvas.Image
                renderStrategy: Canvas.Cooperative
                property real dlPct: {
                    const t = hud._budget("totDL");
                    return t > 0 ? Math.min(1, hud._budget("actDL") / t) : 0;
                }
                property real ulPct: {
                    const t = hud._budget("totUL");
                    return t > 0 ? Math.min(1, hud._budget("actUL") / t) : 0;
                }
                onDlPctChanged: requestPaint()
                onUlPctChanged: requestPaint()
                onAvailableChanged: if (available) requestPaint()
                Component.onCompleted: requestPaint()
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    const cx = width / 2, cy = height / 2;
                    const r1 = 24, r2 = 17;
                    // Tracks
                    ctx.lineWidth = 3;
                    ctx.strokeStyle = AuroraTheme.sidebarLine;
                    ctx.beginPath(); ctx.arc(cx, cy, r1, 0, 2*Math.PI); ctx.stroke();
                    ctx.beginPath(); ctx.arc(cx, cy, r2, 0, 2*Math.PI); ctx.stroke();
                    // DL ring — gradient pink → mango
                    if (dlPct > 0) {
                        const g = ctx.createLinearGradient(0, 0, width, height);
                        g.addColorStop(0.0, AuroraTheme.accent3);
                        g.addColorStop(1.0, AuroraTheme.accent2);
                        ctx.strokeStyle = g; ctx.lineCap = "round";
                        ctx.beginPath();
                        ctx.arc(cx, cy, r1, -Math.PI/2, -Math.PI/2 + dlPct*2*Math.PI);
                        ctx.stroke();
                    }
                    if (ulPct > 0) {
                        ctx.strokeStyle = AuroraTheme.auroraSuccess; ctx.lineCap = "round";
                        ctx.beginPath();
                        ctx.arc(cx, cy, r2, -Math.PI/2, -Math.PI/2 + ulPct*2*Math.PI);
                        ctx.stroke();
                    }
                }
            }

            Text {
                anchors.horizontalCenter: rings.horizontalCenter
                anchors.verticalCenter: rings.verticalCenter
                anchors.verticalCenterOffset: -4
                text: hud._heroSpeedText()
                color: AuroraTheme.sidebarInk
                font.family: AuroraTheme.fontMono
                font.pixelSize: 14
                font.weight: Font.Bold
                font.letterSpacing: -0.4
            }
            Text {
                anchors.horizontalCenter: rings.horizontalCenter
                anchors.top: rings.verticalCenter
                anchors.topMargin: 6
                text: hud._heroSpeedUnit()
                color: AuroraTheme.sidebarInk4
                font.family: AuroraTheme.fontMono
                font.pixelSize: 7
            }

            // Right-side stat lines
            Column {
                anchors.left: rings.right
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                // Plain text — RichText under-reports implicitWidth and
                // crowds chips together at real-world queue densities.
                Row {
                    spacing: 6
                    width: parent.width
                    Rectangle { width: 6; height: 6; radius: 2; color: AuroraTheme.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "↓ " + hud._budget("actDL") + " / " + hud._budget("totDL")
                        color: AuroraTheme.sidebarInk
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: 6
                    width: parent.width
                    Rectangle { width: 6; height: 6; radius: 2; color: AuroraTheme.auroraSuccess; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "↑ " + hud._budget("actUL") + " / " + hud._budget("totUL")
                        color: AuroraTheme.sidebarInk
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Text {
                    text: "ETA " + hud._etaText()
                    color: AuroraTheme.sidebarInk4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    elide: Text.ElideRight
                    width: parent.width
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    //  VARIANT 2 · File Marquee — top file + 2px shimmer bar
    // ══════════════════════════════════════════════════════════════════
    Component {
        id: cmpFileMarquee
        Item {
            // Header row — pulse + label + n/total
            RowLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 10
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6
                LiveDot { active: hud._hasActivity }
                Text {
                    text: hud._budget("actUL") > 0
                        ? qsTr("ĐANG TẢI LÊN")
                        : (hud._budget("actDL") > 0 ? qsTr("ĐANG TẢI XUỐNG") : qsTr("CHỜ"))
                    color: AuroraTheme.sidebarInk4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 9
                    font.letterSpacing: 1.4
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: hud._budget("actDL") + hud._budget("actUL") + " / "
                        + (hud._budget("totDL") + hud._budget("totUL"))
                    color: AuroraTheme.sidebarInk2
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }
            }

            Text {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 28
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                text: hud._topFile.length > 0 ? hud._topFile : qsTr("Không có tệp đang chạy")
                color: AuroraTheme.sidebarInk
                font.family: AuroraTheme.fontMono
                font.pixelSize: 11
                font.weight: Font.Medium
                elide: Text.ElideRight
                wrapMode: Text.NoWrap
            }

            // Progress bar — 2px tall, gradient fill clipped to pct.
            // Shimmer overlay only runs when activity present.
            Item {
                id: bar
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 50
                height: 2
                Rectangle {
                    anchors.fill: parent
                    color: AuroraTheme.sidebarLine
                    radius: 1
                }
                Rectangle {
                    id: barFill
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: Math.max(0, Math.min(1, hud._topProgress)) * parent.width
                    radius: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: AuroraTheme.accent3 }
                        GradientStop { position: 0.5; color: AuroraTheme.accent }
                        GradientStop { position: 1.0; color: AuroraTheme.accent2 }
                    }
                    Behavior on width { enabled: !AuroraTheme.reduceMotion
                        NumberAnimation { duration: AuroraTheme.durBase } }
                    clip: true
                    Rectangle {
                        id: shimmer
                        width: 40
                        height: parent.height
                        opacity: 0.4
                        visible: hud._hasActivity && !AuroraTheme.reduceMotion
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 0.5; color: "#FFFFFF" }
                            GradientStop { position: 1.0; color: "transparent" }
                        }
                        NumberAnimation on x {
                            running: shimmer.visible
                            loops: Animation.Infinite
                            from: -40
                            to: bar.width
                            duration: 1400
                        }
                    }
                }
            }

            // Footer — hero MB/s + ETA
            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.bottomMargin: 8
                spacing: 6
                Text {
                    text: hud._heroSpeedText()
                    color: AuroraTheme.sidebarInk
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    font.letterSpacing: -0.4
                }
                Text {
                    text: hud._heroSpeedUnit()
                    color: AuroraTheme.accent
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 2
                }
                Item { Layout.fillWidth: true; Layout.minimumWidth: 4 }
                Text {
                    text: "ETA " + hud._etaText()
                    color: AuroraTheme.sidebarInk2
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.maximumWidth: 80
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    //  VARIANT 3 · Stat Grid — 2×2 cells
    // ══════════════════════════════════════════════════════════════════
    Component {
        id: cmpStatGrid
        Item {
            // Header — hero + LIVE
            RowLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 8
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 5
                // Hero number with brand gradient text.
                Text {
                    id: heroNum
                    text: hud._heroSpeedText()
                    color: AuroraTheme.accent
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    font.letterSpacing: -0.6
                }
                Text {
                    text: hud._heroSpeedUnit()
                    color: AuroraTheme.sidebarInk3
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 2
                }
                Item { Layout.fillWidth: true }
                LiveDot { active: hud._hasActivity }
                Text {
                    text: hud._hasActivity ? qsTr("LIVE") : qsTr("IDLE")
                    color: hud._hasActivity ? AuroraTheme.auroraSuccess : AuroraTheme.sidebarInk4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 9
                    font.letterSpacing: 0.8
                }
            }

            // 2×2 grid — DL · UL // ETA · Today
            GridLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                columns: 2
                rowSpacing: 0
                columnSpacing: 0
                StatCell { lbl: qsTr("DL"); val: String(hud._budget("actDL")); sub: " / " + hud._budget("totDL"); col: AuroraTheme.accent }
                StatCell { lbl: qsTr("UL"); val: String(hud._budget("actUL")); sub: " / " + hud._budget("totUL"); col: AuroraTheme.auroraSuccess; leftBorder: true }
                StatCell { lbl: qsTr("ETA"); val: hud._etaText(); sub: ""; col: AuroraTheme.sidebarInk; topBorder: true }
                StatCell {
                    lbl: qsTr("HÔM NAY"); val: "—"; sub: " GB";
                    col: AuroraTheme.sidebarInk; topBorder: true; leftBorder: true
                }
            }

            component StatCell: Rectangle {
                id: cell
                property string lbl: ""
                property string val: ""
                property string sub: ""
                property color col: AuroraTheme.sidebarInk
                property bool leftBorder: false
                property bool topBorder: false
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                color: "transparent"
                Rectangle {
                    visible: cell.leftBorder
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: AuroraTheme.sidebarLine
                }
                Rectangle {
                    visible: cell.topBorder
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: AuroraTheme.sidebarLine
                }
                Column {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 10
                    spacing: 1
                    Text {
                        text: cell.lbl
                        color: AuroraTheme.sidebarInk4
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 9
                        font.letterSpacing: 1.0
                    }
                    Row {
                        spacing: 2
                        Text {
                            id: cellVal
                            text: cell.val
                            color: cell.col
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            font.letterSpacing: -0.3
                        }
                        Text {
                            text: cell.sub
                            color: AuroraTheme.sidebarInk4
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 9
                            anchors.bottom: cellVal.bottom
                            anchors.bottomMargin: 1
                        }
                    }
                }
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════
    //  Internal — tiny live-pulse dot. Animation skipped when reduce-motion
    //  is on or the HUD has no activity (no spinning frame budget for an
    //  idle indicator).
    // ══════════════════════════════════════════════════════════════════
    component LiveDot: Rectangle {
        id: dot
        property bool active: false
        width: 5; height: 5; radius: 3
        color: active ? AuroraTheme.auroraSuccess : AuroraTheme.sidebarInk4
        // Manual opacity pulse via Timer so we can cleanly disable when idle.
        property real _phase: 1
        opacity: _phase
        Timer {
            running: dot.active && !AuroraTheme.reduceMotion
            repeat: true
            interval: 700
            triggeredOnStart: true
            onTriggered: dot._phase = (dot._phase < 1 ? 1 : 0.4)
        }
        Behavior on opacity { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: 700 } }
    }

    // ══════════════════════════════════════════════════════════════════
    //  Internal — 24 little bars for the Speed-Pulse sparkline.
    //  Re-renders on speedHistory change.  Plain Rectangles (no Canvas)
    //  keep this lightweight even with hot updates.
    // ══════════════════════════════════════════════════════════════════
    component Sparkbars: Item {
        id: spark
        property var samples: []
        readonly property int bins: 24
        // Compute once per samples change — cheaper than 24 individual binds.
        readonly property var _normalized: {
            const a = (samples && samples.length) ? samples : [];
            const out = new Array(24).fill(0.15);
            let mx = 0;
            const N = a.length;
            const k = Math.min(24, N);
            for (let i = 0; i < k; ++i) {
                const v = Number(a[N - k + i]) || 0;
                if (v > mx) mx = v;
            }
            if (mx <= 0) return out;   // flat baseline
            for (let i = 0; i < k; ++i) {
                const v = Number(a[N - k + i]) || 0;
                out[24 - k + i] = Math.max(0.15, Math.min(1, v / mx));
            }
            return out;
        }
        Row {
            id: row
            anchors.fill: parent
            spacing: 1.5
            Repeater {
                model: 24
                delegate: Rectangle {
                    property real _v: spark._normalized[index] || 0
                    width: (row.width - 23 * row.spacing) / 24
                    height: Math.max(2, _v * spark.height)
                    anchors.bottom: parent.bottom
                    radius: 1
                    color: index === 23
                        ? AuroraTheme.accent
                        : Qt.rgba(1, 0.357, 0.180, 0.25 + _v * 0.5)
                }
            }
        }
    }
}
