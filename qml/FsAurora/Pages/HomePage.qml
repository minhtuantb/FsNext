// SPDX-License-Identifier: Proprietary
// HomePage (Aurora) — dashboard landing page.
//
// Composition:
//   0. Top bar       — search + settings + upload CTA (FIXED, does not scroll)
//   1. Greeting hero — time-of-day kicker + editorial serif welcome
//   2. Quick actions — 4 entry-points (paste links / upload / sync / share)
//   3. "Tiếp tục đang tải" — active download + upload transfers (max 3)
//   4. "File gần đây" — most recent completed downloads, filterable
//
// Context properties expected from Main.qml:
//   authViewModel, downloadViewModel, uploadViewModel, homeSearchViewModel
//
// Navigation is signalled through `pageRequested(int)` so Main.qml owns the
// routing table (the same pattern the sidebar uses).
//
// Search routing — the homepage owns its own search input but delegates
// classification to `homeSearchViewModel`. The VM emits routeFileUrl /
// routeFolderUrl / routeMultipleUrls / routeKeyword which Main.qml turns
// into actual UI actions (open dialog, etc.). Bad-word / length rejection
// stays inline on this page.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Utils 1.0
import Fshare.Components 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: page

    signal pageRequested(int index)
    // Parent opens DownloadPage add-dialog. `links` empty → blank dialog;
    // non-empty → pre-filled with the given URLs (newline-separated).
    signal addDownloadRequested(string links)
    signal searchRequested(string query)            // legacy, no-op route — kept for API compat
    signal showFileUrlRequested(string url)         // file/<code> → detail sheet
    signal showFolderUrlRequested(string url)       // folder/<code> → browser
    // Phase 3 — inline overlay row clicked. The host opens FileDetailSheet
    // for the synthesised file URL of an in-account search result.
    signal showFileFromLinkcode(string linkcode, string name)
    signal upgradeRequested()

    // Mirrors HomeSearchViewModel::State — keep in sync with the C++ enum.
    readonly property int _stIdle:        0
    readonly property int _stTooShort:    1
    readonly property int _stUrlFile:     2
    readonly property int _stUrlFolder:   3
    readonly property int _stUrlMultiple: 4
    readonly property int _stKeyword:     5
    readonly property int _stBlocked:     6

    // Live classification state — bound to the VM. Falls back to Idle when
    // the VM is missing (defensive — context property may be absent in
    // standalone QML previews / tests).
    readonly property int _searchState: homeSearchViewModel ? homeSearchViewModel.state : 0
    readonly property string _searchHint: homeSearchViewModel ? homeSearchViewModel.hint : ""

    property int _filter: 0   // 0=Tất cả, 1=Video, 2=Tài liệu, 3=Ảnh

    // ── Recent files — merged download + upload history ──────────────
    //
    // TransferListModel role enum (src/viewmodels/TransferListModel.h). We
    // hard-code the integer offsets here because TransferListModel isn't
    // QML-registered (no QML_ELEMENT macro), so the enum constants aren't
    // exposed by name. Keep these in lock-step with the C++ enum — they
    // will compile against the same struct fields, so a header reorder
    // would break the binding loudly at runtime (wrong-typed data).
    readonly property int _roleFileName    : 257   // Qt.UserRole(256) + 1
    readonly property int _roleFileSize    : 258
    readonly property int _roleLinkCode    : 263   // + 7  (fshare file/<code>)
    readonly property int _roleLocalPath   : 267   // + 11
    readonly property int _roleCompletedAt : 270   // + 14

    // How many rows we scan per model before merging. The history models are
    // newest-first (prependTask inserts at index 0; scroll-loaded older rows
    // append to the tail), so the most-recent entries — the only ones this
    // glanceable surface shows — live at the low indices. Scanning a fixed
    // window keeps this binding O(1) in history size: without it, every
    // progress tick or scroll-load on the Download/Upload pages (which grow
    // these models unbounded) would walk thousands of rows here. The window has
    // headroom over the 50-row display cap so an active filter still has
    // candidates to draw from.
    readonly property int _recentScanCap: 80

    // Merged history array, newest-first by completedAt, already filtered by the
    // active category tab. Re-evaluated by QML's dependency tracker whenever
    // either model's `count` ticks (QAbstractItemModel exposes count via
    // rowsInserted/rowsRemoved) or `_filter` changes — _matchesFilter() reads
    // page._filter, so the binding re-runs on tab switches too.
    readonly property var _recentHistory: {
        const dh = downloadViewModel ? downloadViewModel.historyModel : null;
        const uh = uploadViewModel   ? uploadViewModel.historyModel   : null;
        const dhCount = dh ? dh.count : 0;
        const uhCount = uh ? uh.count : 0;
        const out = [];
        function _push(m, n, isUp) {
            const lim = Math.min(n, page._recentScanCap);
            for (let i = 0; i < lim; ++i) {
                const idx = m.index(i, 0);
                const name = m.data(idx, page._roleFileName) || "";
                // Filter BEFORE the 50-row cap so selecting e.g. "Video"
                // surfaces older videos instead of being starved by more-recent
                // files of other types that would otherwise fill the cap first.
                if (!page._matchesFilter(name)) continue;
                out.push({
                    fileName:    name,
                    fileSize:    m.data(idx, page._roleFileSize)    || 0,
                    linkCode:    m.data(idx, page._roleLinkCode)    || "",
                    localPath:   m.data(idx, page._roleLocalPath)   || "",
                    completedAt: m.data(idx, page._roleCompletedAt) || 0,
                    isUpload:    isUp
                });
            }
        }
        if (dh) _push(dh, dhCount, false);
        if (uh) _push(uh, uhCount, true);
        // Newest first. completedAt is ms-since-epoch; descending sort
        // puts the just-finished transfer at the top regardless of which
        // queue produced it.
        out.sort((a, b) => b.completedAt - a.completedAt);
        // Cap at the 50 most-recent matches — the homepage "recent" surface is a
        // glanceable summary, not a full log (the dedicated Download / Upload
        // pages own the complete history with infinite scroll).
        return out.slice(0, 50);
    }
    readonly property bool _recentEmpty: _recentHistory.length === 0

    // ── Live time / relative-time clock ──────────────────────────────
    property real nowMs: Date.now()
    Timer {
        interval: 60 * 1000
        running: true
        repeat: true
        onTriggered: page.nowMs = Date.now()
    }

    readonly property string _firstName: {
        if (!authViewModel) return "bạn";
        const n = (authViewModel.userName || "").trim();
        if (n.length === 0) return "bạn";
        const parts = n.split(/\s+/);
        return parts[parts.length - 1];   // last token — Vietnamese given name
    }

    readonly property string _greetingKicker: {
        const d = new Date(page.nowMs);
        const h = d.getHours();
        const days = ["CHỦ NHẬT","THỨ HAI","THỨ BA","THỨ TƯ","THỨ NĂM","THỨ SÁU","THỨ BẢY"];
        const dow = days[d.getDay()];
        let part;
        if      (h < 5)  part = "RẠNG SÁNG";
        else if (h < 11) part = "SÁNG";
        else if (h < 13) part = "TRƯA";
        else if (h < 18) part = "CHIỀU";
        else             part = "TỐI";
        const hh = ("0" + h).slice(-2);
        const mm = ("0" + d.getMinutes()).slice(-2);
        return part + " " + dow + " · " + hh + ":" + mm;
    }

    function _relTime(tsMs) {
        if (!tsMs || tsMs <= 0) return "";
        const diffS = Math.max(0, (page.nowMs - tsMs) / 1000);
        if (diffS < 60)        return "vừa xong";
        if (diffS < 3600)      return Math.floor(diffS / 60) + " phút trước";
        if (diffS < 86400)     return Math.floor(diffS / 3600) + " giờ trước";
        if (diffS < 86400 * 2) return "hôm qua";
        if (diffS < 86400 * 7) return Math.floor(diffS / 86400) + " ngày trước";
        return Qt.formatDate(new Date(tsMs), "dd/MM/yyyy");
    }

    function _extOf(name) {
        const s = String(name || "");
        const dot = s.lastIndexOf(".");
        return dot < 0 ? "" : s.substring(dot + 1).toLowerCase();
    }

    // ── Recent-file row actions ──────────────────────────────────────
    // Route through DownloadViewModel's C++ helpers rather than QML
    // Qt.openUrlExternally. The C++ side uses PlatformUtils (explorer
    // /select on Windows, QDesktopServices::openUrl) which handles spaces /
    // Unicode / native separators robustly — the hand-built file:// URL
    // approach silently failed on real Windows paths. openShareUrl also
    // detects bare-linkcode vs full-URL (downloads store the full fshare
    // URL in linkcode → prefixing it produced a double-domain link).
    function _openLocalFile(localPath) {
        if (downloadViewModel && localPath && localPath.length > 0)
            downloadViewModel.openLocalFile(localPath);
    }
    function _openLocalFolder(localPath) {
        if (downloadViewModel && localPath && localPath.length > 0)
            downloadViewModel.revealInFolder(localPath);
    }
    function _openFshareLink(linkCode) {
        if (downloadViewModel && linkCode && linkCode.length > 0)
            downloadViewModel.openShareUrl(linkCode);
    }
    function _copyFshareLink(linkCode) {
        // VM copies to clipboard + emits shareLinkCopied → Main.qml toasts.
        if (downloadViewModel && linkCode && linkCode.length > 0)
            downloadViewModel.copyShareLink(linkCode);
    }

    function _categoryOf(name) {
        const ext = _extOf(name);
        if (["mp4","mkv","avi","mov","wmv","flv","webm","m4v","3gp"].indexOf(ext) >= 0) return "video";
        if (["jpg","jpeg","png","gif","bmp","svg","webp","tif","tiff","heic"].indexOf(ext) >= 0) return "image";
        if (["pdf","doc","docx","xls","xlsx","ppt","pptx","txt","rtf"].indexOf(ext) >= 0) return "document";
        return "other";
    }

    function _matchesFilter(name) {
        if (page._filter === 0) return true;
        const c = _categoryOf(name);
        if (page._filter === 1) return c === "video";
        if (page._filter === 2) return c === "document";
        if (page._filter === 3) return c === "image";
        return true;
    }

    // ── Aggregate live transfer stats ────────────────────────────────
    readonly property var _dlModel: downloadViewModel ? downloadViewModel.model : null
    readonly property var _ulModel: uploadViewModel   ? uploadViewModel.model   : null
    readonly property int _dlCount: _dlModel ? _dlModel.count : 0
    readonly property int _ulCount: _ulModel ? _ulModel.count : 0

    readonly property string _dlSpeed: downloadViewModel ? downloadViewModel.totalSpeed : ""
    readonly property string _ulSpeed: uploadViewModel   ? uploadViewModel.totalSpeed   : ""

    // ═════════════════════════════════════════════════
    //  0. TOP BAR — fixed header; stays visible while content scrolls
    // ═════════════════════════════════════════════════
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: AuroraTheme.heightInput + 2 * AuroraTheme.sp4
        color: AuroraTheme.bg
        z: 2

        // Subtle divider
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: AuroraTheme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp4
            anchors.bottomMargin: AuroraTheme.sp4
            spacing: AuroraTheme.sp3

            // Search pill
            Rectangle {
                id: searchPill
                Layout.fillWidth: true
                Layout.preferredHeight: AuroraTheme.heightInput
                radius: AuroraTheme.radiusPill
                color: AuroraTheme.panel
                border.width: 1
                // Border color reflects classification state, falling back to
                // hover/focus chrome when there's nothing semantically
                // interesting (Idle/Keyword on hover etc.).
                border.color: {
                    if (page._searchState === page._stBlocked) return AuroraTheme.danger;
                    if (page._searchState === page._stTooShort) return AuroraTheme.warn;
                    if (page._searchState === page._stUrlFile
                        || page._searchState === page._stUrlFolder
                        || page._searchState === page._stUrlMultiple) return AuroraTheme.accent;
                    if (searchInput.activeFocus) return AuroraTheme.accent;
                    if (searchMa.containsMouse) return AuroraTheme.borderStrong;
                    return AuroraTheme.border;
                }
                Behavior on border.color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp4
                    anchors.rightMargin: AuroraTheme.sp3
                    spacing: AuroraTheme.sp2

                    FsIcon {
                        name: "search"
                        sizePx: 15
                        color: searchInput.activeFocus ? AuroraTheme.accent : AuroraTheme.ink3
                    }

                    TextField {
                        id: searchInput
                        Layout.fillWidth: true
                        placeholderText: qsTr("Tìm file, folder, hoặc dán link Fshare…")
                        placeholderTextColor: AuroraTheme.ink4
                        color: AuroraTheme.ink1
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        selectByMouse: true
                        background: Item {}   // strip default box — our pill is the chrome
                        verticalAlignment: TextInput.AlignVCenter
                        // Re-classify live so the border / helper-text reflect
                        // the current input. Cheap — just a regex + bad-word
                        // dictionary lookup.
                        onTextChanged: if (homeSearchViewModel) homeSearchViewModel.classify(text)

                        // Keyboard nav for the inline overlay. The overlay
                        // doesn't take focus (it sits in a different parent
                        // tree and would interfere with typing) so we hand
                        // its highlight cursor explicit moves from here.
                        Keys.onUpPressed:    function(e) {
                            if (searchOverlay.active) { searchOverlay.moveHighlight(-1); e.accepted = true; }
                        }
                        Keys.onDownPressed:  function(e) {
                            if (searchOverlay.active) { searchOverlay.moveHighlight(1); e.accepted = true; }
                        }
                        // Esc clears the field — classify() then transitions
                        // to Idle and the overlay hides itself.
                        Keys.onEscapePressed: function(e) {
                            if (searchInput.text.length > 0) {
                                searchInput.text = "";
                                e.accepted = true;
                            }
                        }

                        onAccepted: {
                            if (!homeSearchViewModel) return;
                            // If the overlay has a row highlighted via ↑/↓,
                            // Enter activates that row instead of re-submitting
                            // the keyword (matches Spotlight / Linear).
                            if (searchOverlay.active && searchOverlay.highlightedIndex >= 0) {
                                searchOverlay.activateHighlighted();
                                searchInput.text = "";
                                return;
                            }
                            // The VM emits exactly one routing signal (or
                            // rejected*). The handlers in Main.qml /
                            // Connections below decide what UI to open.
                            const st = homeSearchViewModel.submit(text);
                            // Only clear the input on a successful route so the
                            // user can correct rejected input without retyping.
                            if (st === page._stUrlFile
                                || st === page._stUrlFolder
                                || st === page._stUrlMultiple
                                || st === page._stKeyword) {
                                text = "";
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredHeight: 22
                        Layout.preferredWidth: kbdHint.implicitWidth + 14
                        radius: AuroraTheme.radiusSm
                        color: Qt.rgba(0, 0, 0, 0.04)
                        border.width: 1
                        border.color: AuroraTheme.border
                        visible: !searchInput.activeFocus && searchInput.text.length === 0
                        Text {
                            id: kbdHint
                            anchors.centerIn: parent
                            text: "⌘K"
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink3
                        }
                    }
                }

                MouseArea {
                    id: searchMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.IBeamCursor
                    onClicked: searchInput.forceActiveFocus()
                }
            }

            TopIconButton {
                icon: "gear"
                tooltip: qsTr("Cài đặt")
                onActivated: page.pageRequested(Pages.settings)
            }

            // Upload CTA (gradient pill)
            FsGradientRect {
                Layout.preferredWidth: uploadLbl.implicitWidth + 42
                Layout.preferredHeight: AuroraTheme.heightInput
                radius: AuroraTheme.radiusPill

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    FsIcon {
                        name: "upload"
                        sizePx: 14
                        color: "#FFFFFF"
                    }
                    Text {
                        id: uploadLbl
                        text: qsTr("Tải lên")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: "#FFFFFF"
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: page.pageRequested(Pages.upload)
                }
            }
        }
    }

    // ── Search helper chip — floats just below the top bar, aligned
    // with the search pill. Shows the VM's current hint and a coloured
    // status dot. Hidden in Idle / Keyword-ready states (the latter
    // just nudges the user to press Enter, which is obvious enough).
    Rectangle {
        id: searchHintChip
        anchors.top: topBar.bottom
        anchors.left: topBar.left
        anchors.leftMargin: AuroraTheme.sp6
        anchors.topMargin: 6
        height: 22
        width: hintRow.implicitWidth + AuroraTheme.sp3 * 2
        radius: AuroraTheme.radiusPill
        z: 3
        visible: page._searchState !== page._stIdle && page._searchHint.length > 0
        color: {
            if (page._searchState === page._stBlocked)  return AuroraTheme.dangerSoft;
            if (page._searchState === page._stTooShort) return AuroraTheme.warnSoft;
            return AuroraTheme.accentTint10;
        }
        border.width: 1
        border.color: {
            if (page._searchState === page._stBlocked)  return AuroraTheme.danger;
            if (page._searchState === page._stTooShort) return AuroraTheme.warn;
            return AuroraTheme.accent;
        }

        RowLayout {
            id: hintRow
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp3
            anchors.rightMargin: AuroraTheme.sp3
            spacing: 6
            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: 3
                color: searchHintChip.border.color
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: page._searchHint
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                font.weight: Font.DemiBold
                color: searchHintChip.border.color
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    // ── Routing — VM emits exactly one of these when the user submits.
    // Folder / file URLs go to the parent (Main.qml) to open the right
    // surface; multiple URLs reuse the existing bulk-download path.
    // Keyword routing now feeds the inline overlay (HomeSearchOverlay
    // below) — the VM already kicked off a debounced API search during
    // classify(), so onRouteKeyword just becomes a no-op when the user
    // hits Enter (results, if any, are already on-screen).
    Connections {
        target: homeSearchViewModel
        function onRouteFileUrl(url)         { page.showFileUrlRequested(url); }
        function onRouteFolderUrl(url)       { page.showFolderUrlRequested(url); }
        function onRouteMultipleUrls(joined) { page.addDownloadRequested(joined); }
        function onRouteKeyword(_q)          { /* overlay already shows results */ }
        function onRejectedBadWord(_hit)     { /* border + chip already convey the rejection */ }
        function onRejectedTooShort()        { /* same */ }
    }

    // ── Click-outside catcher for the overlay ─────────────────
    // Transparent rectangle below the topBar that intercepts mouse clicks
    // while the overlay is open and routes them to "dismiss". The overlay
    // itself sits ABOVE this (z=5) so clicks INSIDE it still reach its
    // ListView. Anchoring to topBar.bottom means clicks on the search pill
    // / settings / upload buttons pass through (they're inside topBar at
    // z=2, behind this).
    MouseArea {
        id: overlayDismissCatcher
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        z: 4
        visible: searchOverlay.visible
        // propagateComposedEvents=false (default) — the click is consumed
        // here, not forwarded to the underlying ScrollView.
        onClicked: {
            // Clear the search field. classify() then transitions to Idle
            // and both the overlay and hint chip auto-hide.
            searchInput.text = "";
        }
    }

    // ── Inline keyword-search overlay ──────────────────────────
    // Floats below the search hint chip; only visible while the VM is in
    // Keyword state AND has something to show (loading / results / empty).
    // We mount as a child of `page` (not inside the topBar) so the panel
    // can extend past the bar's height without clipping. z above the
    // scrollable content but below modal dialogs.
    Aurora.HomeSearchOverlay {
        id: searchOverlay
        // Track the search pill's position dynamically so the overlay
        // stays aligned even as the user resizes the window (the pill
        // is fillWidth inside the topBar RowLayout). mapToItem is
        // evaluated each render via the binding on searchPill.width /
        // page.width — the dummy reads keep Qt from caching the result.
        readonly property point _origin: searchPill.mapToItem(page, 0, searchPill.height)
        readonly property real  _hintGap: searchHintChip.visible ? (6 + searchHintChip.height) : 0
        // Force re-evaluation when the pill resizes / window resizes.
        readonly property int   _w: searchPill.width
        readonly property int   _pw: page.width
        x: _origin.x
        y: _origin.y + _hintGap + 6
        width: _w
        z: 5
        visible: page._searchState === page._stKeyword && active
        onFileActivated: (lc, name) => page.showFileFromLinkcode(lc, name)
    }

    // ═════════════════════════════════════════════════
    //  Scrollable content area (below the top bar)
    // ═════════════════════════════════════════════════
    ScrollView {
        id: scroll
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        ColumnLayout {
            width: scroll.availableWidth
            spacing: AuroraTheme.sp8

            // ═════════════════════════════════════════════════
            //  1. GREETING HERO
            // ═════════════════════════════════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AuroraTheme.sp6
                Layout.rightMargin: AuroraTheme.sp6
                Layout.topMargin: AuroraTheme.sp6
                spacing: 8

                Text {
                    text: page._greetingKicker
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.letterSpacing: 2.0
                    font.capitalization: Font.AllUppercase
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink4
                }

                RowLayout {
                    id: greetingRow
                    Layout.fillWidth: true
                    spacing: 0

                    // Match FsPageHeader's narrow-window breakpoint (720px) so
                    // editorial type scales consistently across the shell.
                    readonly property int _heroSize: page.width < 720 ? 28 : 40

                    Text {
                        text: qsTr("Chào ") + page._firstName + ", "
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: greetingRow._heroSize
                        font.weight: Font.DemiBold
                        font.letterSpacing: -1.1
                        color: AuroraTheme.ink1
                    }

                    Text {
                        text: qsTr("tiếp tục?")
                        font.family: AuroraTheme.fontSerif
                        font.italic: true
                        font.pixelSize: greetingRow._heroSize
                        font.letterSpacing: -1.1
                        color: AuroraTheme.accent
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            // ═════════════════════════════════════════════════
            //  2. QUICK ACTIONS
            // ═════════════════════════════════════════════════
            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AuroraTheme.sp6
                Layout.rightMargin: AuroraTheme.sp6
                columns: 4
                columnSpacing: AuroraTheme.sp4
                rowSpacing: AuroraTheme.sp4

                QuickAction {
                    Layout.fillWidth: true
                    icon: "link"
                    title: qsTr("Dán link & tải")
                    subtitle: qsTr("Paste link Fshare")
                    onActivated: page.addDownloadRequested("")
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "arrow-up"
                    title: qsTr("Tải file lên")
                    subtitle: qsTr("Kéo-thả hoặc chọn")
                    onActivated: page.pageRequested(Pages.upload)
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "sync"
                    title: qsTr("Thêm folder sync")
                    subtitle: qsTr("2-way đồng bộ")
                    onActivated: page.pageRequested(Pages.sync)
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "external-link"
                    title: qsTr("Tạo link chia sẻ")
                    subtitle: qsTr("Password + hết hạn")
                    onActivated: page.pageRequested(Pages.files)
                }
            }

            // ═════════════════════════════════════════════════
            //  3. TIẾP TỤC ĐANG TẢI
            // ═════════════════════════════════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AuroraTheme.sp6
                Layout.rightMargin: AuroraTheme.sp6
                spacing: AuroraTheme.sp3
                visible: page._dlCount > 0 || page._ulCount > 0

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2

                    Text {
                        text: qsTr("Tiếp tục đang tải")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        font.letterSpacing: -0.2
                        color: AuroraTheme.ink1
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        // Split DL / UL counts so the title section reads as
                        // "↓ 2 tải xuống · ↑ 1 tải lên · 47 MB/s tổng" and the
                        // user knows what's running without scanning the cards.
                        // Drop the part(s) that are zero so the chip stays tight
                        // when only one direction is active.
                        text: {
                            const parts = [];
                            if (page._dlCount > 0) parts.push("↓ " + page._dlCount + " " + qsTr("tải xuống"));
                            if (page._ulCount > 0) parts.push("↑ " + page._ulCount + " " + qsTr("tải lên"));
                            const sp = page._dlSpeed || page._ulSpeed || "";
                            if (sp.length > 0) parts.push(sp + " " + qsTr("tổng"));
                            return parts.join(" · ");
                        }
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        color: AuroraTheme.ink4
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp4

                    Repeater {
                        model: page._dlModel
                        delegate: TransferMiniCard {
                            required property int index
                            required property string taskId
                            required property string fileName
                            required property real fileSize
                            required property real progress
                            required property real speed
                            required property string eta
                            required property int status

                            Layout.fillWidth: visible
                            Layout.preferredWidth: visible ? -1 : 0
                            visible: index < 3
                            kind: "download"
                            tName:     fileName
                            tSize:     fileSize
                            tProgress: progress
                            tSpeed:    speed
                            tEta:      eta
                            tStatus:   status
                            onPauseClicked:  if (downloadViewModel) downloadViewModel.pauseTask(taskId)
                            onResumeClicked: if (downloadViewModel) downloadViewModel.resumeTask(taskId)
                            onOpenClicked: page.pageRequested(Pages.download)
                        }
                    }

                    Repeater {
                        model: page._ulModel
                        delegate: TransferMiniCard {
                            required property int index
                            required property string taskId
                            required property string fileName
                            required property real fileSize
                            required property real progress
                            required property real speed
                            required property string eta
                            required property int status

                            Layout.fillWidth: visible
                            Layout.preferredWidth: visible ? -1 : 0
                            visible: index < Math.max(0, 3 - page._dlCount)
                            kind: "upload"
                            tName:     fileName
                            tSize:     fileSize
                            tProgress: progress
                            tSpeed:    speed
                            tEta:      eta
                            tStatus:   status
                            onPauseClicked:  if (uploadViewModel) uploadViewModel.pauseTask(taskId)
                            onResumeClicked: if (uploadViewModel) uploadViewModel.resumeTask(taskId)
                            onOpenClicked: page.pageRequested(Pages.upload)
                        }
                    }
                }
            }

            // ═════════════════════════════════════════════════
            //  4. FILE GẦN ĐÂY (recent downloads)
            // ═════════════════════════════════════════════════
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AuroraTheme.sp6
                Layout.rightMargin: AuroraTheme.sp6
                spacing: AuroraTheme.sp4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2

                    Text {
                        text: qsTr("File gần đây")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        font.letterSpacing: -0.2
                        color: AuroraTheme.ink1
                    }
                    Item { Layout.fillWidth: true }

                    Row {
                        spacing: AuroraTheme.sp4
                        FilterTab { label: qsTr("Tất cả"); active: page._filter === 0; onActivated: page._filter = 0 }
                        FilterTab { label: qsTr("Video");  active: page._filter === 1; onActivated: page._filter = 1 }
                        FilterTab { label: qsTr("Tài liệu"); active: page._filter === 2; onActivated: page._filter = 2 }
                        FilterTab { label: qsTr("Ảnh");    active: page._filter === 3; onActivated: page._filter = 3 }
                    }
                }

                // Recent file rows — sourced from the download history
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(220, recentList.contentHeight + 4)
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border

                    ListView {
                        id: recentList
                        anchors.fill: parent
                        anchors.margins: AuroraTheme.sp3
                        clip: true
                        interactive: false
                        spacing: 4
                        // JS array driven by page._recentHistory (merged DL+UL
                        // sorted newest-first). Switching from a QAbstractItem
                        // Model to an array also means delegates access fields
                        // via `modelData`, not directly bound role names.
                        model: page._recentHistory

                        // Category filtering happens upstream in _recentHistory,
                        // so every row here is already a match — render it
                        // directly without a per-row Loader/visibility gate.
                        delegate: RecentRow {
                            required property var modelData
                            width: recentList.width
                            height: 60
                            fileName:    modelData.fileName
                            fileSize:    modelData.fileSize
                            linkCode:    modelData.linkCode
                            localPath:   modelData.localPath
                            completedAt: modelData.completedAt
                            isUpload:    modelData.isUpload
                            nowMs:       page.nowMs
                            // Double-click row → open the local file.
                            onOpenClicked:  page._openLocalFile(modelData.localPath)
                            // Single-click row → copy the fshare link.
                            onCopyClicked:  page._copyFshareLink(modelData.linkCode)
                            // Folder icon → open the containing folder.
                            onFolderClicked: page._openLocalFolder(modelData.localPath)
                            // Fshare icon → open the share page in browser.
                            onShareClicked: page._openFshareLink(modelData.linkCode)
                        }

                        // Empty state
                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: page._recentEmpty
                            spacing: AuroraTheme.sp2

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                width: 56; height: 56
                                radius: 28
                                color: AuroraTheme.accentTint10
                                FsIcon {
                                    anchors.centerIn: parent
                                    name: "folder-open"
                                    sizePx: 24
                                    color: AuroraTheme.accent
                                }
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("Chưa có file nào gần đây")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink2
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("Các file tải xuống sẽ hiển thị ở đây.")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 12
                                color: AuroraTheme.ink4
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: AuroraTheme.sp6 }
        }
    }

    // ── Top-bar icon button (transparent ghost) ────────────────────
    component TopIconButton: Rectangle {
        id: tib
        property string icon: ""
        property string tooltip: ""
        signal activated()

        Layout.preferredWidth: AuroraTheme.heightInput
        Layout.preferredHeight: AuroraTheme.heightInput
        radius: AuroraTheme.radiusPill
        color: tibMa.containsMouse ? AuroraTheme.accentTint10 : AuroraTheme.panel
        border.width: 1
        border.color: tibMa.containsMouse ? AuroraTheme.accent : AuroraTheme.border
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }
        Behavior on border.color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        FsIcon {
            anchors.centerIn: parent
            name: tib.icon
            sizePx: 16
            color: tibMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2
        }

        MouseArea {
            id: tibMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: tib.activated()
            ToolTip.visible: containsMouse && tib.tooltip.length > 0
            ToolTip.text: tib.tooltip
            ToolTip.delay: 400
        }
    }

    // ── Quick-action card ──────────────────────────────────────────
    component QuickAction: Rectangle {
        id: card
        property string icon: ""
        property string title: ""
        property string subtitle: ""
        signal activated()

        // Height bumped 108 → 124. Earlier value forced the inner layout
        // into negative overflow (RowLayout 38 + Title 17 + Subtitle 14 +
        // 3×spacing 10 = 99 > 76 content area), so Qt squashed spacings
        // and the fill-height spacer per card unpredictably. Result: rows
        // of cards looked subtly mis-aligned because the title/subtitle
        // y-positions weren't deterministic. 124 leaves ~23px of real
        // breathing room and the column layout fits cleanly.
        Layout.preferredHeight: 124
        // Force equal-width columns in the parent GridLayout. Without
        // this, each card's implicit width is derived from its own
        // title / subtitle Text content — so "Dán link & tải" + "Paste
        // link Fshare" ends up narrower than "Thêm folder sync" +
        // "2-way đồng bộ". Setting preferredWidth=0 + the existing
        // fillWidth means every card grows from the same zero base
        // and shares the available width equally.
        Layout.preferredWidth: 0
        // Same insurance for implicitWidth — Qt 6 sometimes prefers
        // Item's implicitWidth over Layout.preferredWidth when the
        // latter is 0. Explicit 0 settles the tie unambiguously.
        implicitWidth: 0
        radius: AuroraTheme.radiusLg
        color: AuroraTheme.panel
        border.width: 1
        border.color: qaMa.containsMouse ? AuroraTheme.accent : AuroraTheme.border
        Behavior on border.color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp4
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                spacing: 0

                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    // Pin the box explicitly to vertical-center so a future
                    // RowLayout default change can't drift it off-axis.
                    Layout.alignment: Qt.AlignVCenter
                    radius: AuroraTheme.radiusMd
                    color: AuroraTheme.accentTint10
                    FsIcon {
                        anchors.centerIn: parent
                        name: card.icon
                        // Bumped 18 → 20 so the icon fills more of the 40px
                        // box. Smaller sizes amplified the per-SVG viewBox
                        // padding differences (link/external-link have
                        // off-centre content) and made the icons look
                        // mis-aligned card-to-card.
                        sizePx: 20
                        color: AuroraTheme.accent
                    }
                }
                Item { Layout.fillWidth: true }
                FsIcon {
                    name: "chevron-right"
                    sizePx: 14
                    // Explicit vertical-centre. FsIcon's recent
                    // `opacity: root.color.a` binding led to inconsistent
                    // default alignment in a few Qt 6.8 builds; this kills
                    // the ambiguity.
                    Layout.alignment: Qt.AlignVCenter
                    color: qaMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink4
                    Behavior on color { enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast } }
                }
            }

            Item { Layout.fillHeight: true }

            Text {
                text: card.title
                font.family: AuroraTheme.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: card.subtitle
                font.family: AuroraTheme.fontSans
                font.pixelSize: 12
                color: AuroraTheme.ink3
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: qaMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.activated()
        }
    }

    // ── Filter tab (used in "File gần đây" header) ─────────────────
    component FilterTab: Item {
        id: tab
        property string label: ""
        property bool active: false
        signal activated()

        width: tabText.implicitWidth + 4
        height: 22

        Text {
            id: tabText
            anchors.centerIn: parent
            text: tab.label
            font.family: AuroraTheme.fontSans
            font.pixelSize: 13
            font.weight: tab.active ? Font.DemiBold : Font.Normal
            color: tab.active ? AuroraTheme.ink1 : AuroraTheme.ink3
        }

        Rectangle {
            visible: tab.active
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: tabText.implicitWidth
            height: 2
            color: AuroraTheme.accent
            radius: 1
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: tab.activated()
        }
    }

    // ── Compact live transfer card ─────────────────────────────────
    // Flat properties (tName/tSize/...) because inline components used
    // inside a Repeater can't also declare `required property ...` without
    // clashing with the delegate's own role-bound properties.
    component TransferMiniCard: Rectangle {
        id: tCard
        property string kind: "download"   // or "upload"
        property string tName: ""
        property real   tSize: 0
        property real   tProgress: 0
        property real   tSpeed: 0
        property string tEta: ""
        property int    tStatus: 0

        signal pauseClicked()
        signal resumeClicked()
        signal openClicked()

        Layout.preferredHeight: 96
        radius: AuroraTheme.radiusLg
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border

        // Status enum (mirrors TransferState): 0 Queued, 1 Active, 2 Paused,
        // 3 Complete, 4 Error, 5 Cancelled
        readonly property bool _paused: tStatus === 2
        readonly property bool _queued: tStatus === 0
        readonly property bool _error:  tStatus === 4
        readonly property bool _active: tStatus === 1
        readonly property bool _isUp:   kind === "upload"
        // Human-readable state label appended after the size — keeps the
        // user oriented even when speed=0 (queued / paused) so the card
        // isn't ambiguous.
        readonly property string _stateLabel: {
            if (_error)  return qsTr("Lỗi");
            if (_paused) return qsTr("Tạm dừng");
            if (_queued) return qsTr("Trong hàng đợi");
            return _isUp ? qsTr("Đang tải lên") : qsTr("Đang tải xuống");
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp3
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp3

                // File-type icon with a small direction badge in the
                // bottom-right corner. Single 36×36 footprint preserved —
                // the badge is overlaid, not a separate column, so the
                // card height doesn't grow on small windows. Color picks:
                //   download (↓) → accent  (orange — matches DL surfaces)
                //   upload   (↑) → success (green — matches UL Complete state)
                Item {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    FsFileTypeIcon {
                        anchors.fill: parent
                        fileName: tCard.tName
                        isFolder: false
                        sizePx: 36
                    }
                    Rectangle {
                        width: 14; height: 14; radius: 7
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: -2
                        anchors.bottomMargin: -2
                        color: tCard._isUp ? AuroraTheme.success : AuroraTheme.accent
                        border.width: 2
                        border.color: AuroraTheme.panel
                        Text {
                            anchors.centerIn: parent
                            text: tCard._isUp ? "↑" : "↓"
                            color: "#FFFFFF"
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        Layout.fillWidth: true
                        text: tCard.tName
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink1
                        elide: Text.ElideMiddle
                    }
                    // Status line — direction-aware. Always leads with the
                    // state label so a card without any speed/ETA (queued,
                    // paused, error) still tells the user what's going on.
                    // For active cards we append the ETA chip when available
                    // so the line is "Đang tải xuống · 2.4 GB · còn 3 phút".
                    Text {
                        text: {
                            const sz = FsFormat.bytes(tCard.tSize);
                            const parts = [tCard._stateLabel];
                            if (sz.length > 0) parts.push(sz);
                            if (tCard._active && tCard.tEta.length > 0)
                                parts.push(qsTr("còn ") + tCard.tEta);
                            return parts.join(" · ");
                        }
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        color: tCard._error
                                ? AuroraTheme.danger
                                : (tCard._paused ? AuroraTheme.warn : AuroraTheme.ink3)
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    radius: AuroraTheme.radiusSm
                    color: pauseMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                    border.width: 1
                    border.color: AuroraTheme.border
                    FsIcon {
                        anchors.centerIn: parent
                        name: tCard._paused ? "play" : "pause"
                        sizePx: 12
                        color: AuroraTheme.ink2
                    }
                    MouseArea {
                        id: pauseMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (tCard._paused) tCard.resumeClicked();
                            else               tCard.pauseClicked();
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp3

                Text {
                    text: tCard._paused ? qsTr("Tạm dừng") : FsFormat.speed(tCard.tSpeed)
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: AuroraTheme.accent
                }

                FsProgressBar {
                    Layout.fillWidth: true
                    value: Math.max(0, Math.min(1, tCard.tProgress / 100.0))
                    trackHeight: 4
                }

                Text {
                    text: Math.floor(tCard.tProgress) + "%"
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink2
                    Layout.preferredWidth: 36
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }

    // ── Recent-file row ────────────────────────────────────────────
    component RecentRow: Rectangle {
        id: rRow
        property string fileName: ""
        property real   fileSize: 0
        property string linkCode: ""
        property string localPath: ""
        property real   completedAt: 0
        property real   nowMs: 0
        // Direction badge: false → ↓ accent (download), true → ↑ success
        // (upload). Default false preserves the visual treatment for any
        // legacy caller that doesn't set the property.
        property bool   isUpload: false
        signal openClicked()
        signal folderClicked()
        signal shareClicked()
        signal copyClicked()

        readonly property bool _hasLocal: localPath.length > 0
        readonly property bool _hasLink:  linkCode.length > 0

        height: 56
        radius: AuroraTheme.radiusMd
        color: rowHover.hovered ? AuroraTheme.accentTint10 : "transparent"
        border.width: 0
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        // Pointer handlers (NOT an anchors.fill MouseArea) for row-level
        // hover + double-click. A full-cover MouseArea declared after the
        // RowLayout would sit on top in z-order and SWALLOW the per-action
        // IconAction clicks — which is exactly why "mở thư mục" / "mở
        // fshare" appeared dead. HoverHandler/TapHandler cooperate with the
        // child MouseAreas instead of stealing their events.
        HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
        TapHandler {
            acceptedButtons: Qt.LeftButton
            // Single tap → copy the fshare share link (quick, common action).
            // Double tap → open the local file. TapHandler disambiguates the
            // two by waiting one double-tap interval before emitting
            // singleTapped, so a double-click won't also fire copy.
            onSingleTapped: rRow.copyClicked()
            onDoubleTapped: rRow.openClicked()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp4
            anchors.rightMargin: AuroraTheme.sp4
            spacing: AuroraTheme.sp3

            // 36x36 icon + a small overlaid direction badge bottom-right —
            // same visual idiom as the transfer cards above the list, so
            // the user can scan direction at a glance without parsing the
            // status text on every row.
            Item {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                FsFileTypeIcon {
                    anchors.fill: parent
                    fileName: rRow.fileName
                    isFolder: false
                    sizePx: 36
                }
                Rectangle {
                    width: 14; height: 14; radius: 7
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: -2
                    anchors.bottomMargin: -2
                    color: rRow.isUpload ? AuroraTheme.success : AuroraTheme.accent
                    border.width: 2
                    border.color: AuroraTheme.panel
                    Text {
                        anchors.centerIn: parent
                        text: rRow.isUpload ? "↑" : "↓"
                        color: "#FFFFFF"
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 9
                        font.bold: true
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    Layout.fillWidth: true
                    text: rRow.fileName
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: AuroraTheme.ink1
                    elide: Text.ElideMiddle
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2

                    // Direction pill — "Đã tải lên" / "Đã tải về" gives the
                    // row an at-a-glance verb instead of relying only on the
                    // tiny corner badge. Uses semantic soft tokens.
                    Rectangle {
                        Layout.preferredHeight: 16
                        Layout.preferredWidth: dirLabel.implicitWidth + AuroraTheme.sp3
                        radius: AuroraTheme.radiusPill
                        color: rRow.isUpload ? AuroraTheme.successSoft : AuroraTheme.accentSoft
                        Text {
                            id: dirLabel
                            anchors.centerIn: parent
                            text: rRow.isUpload ? qsTr("Đã tải lên") : qsTr("Đã tải về")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: rRow.isUpload ? AuroraTheme.success : AuroraTheme.accent
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: FsFormat.bytes(rRow.fileSize) + " · " + page._relTime(rRow.completedAt)
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        color: AuroraTheme.ink3
                        elide: Text.ElideRight
                    }
                }
            }

            // Actions reveal on row hover for a calmer idle state; the row is
            // still openable via double-click when actions are hidden.
            //   • folder       → open the containing folder on disk
            //   • external-link → open the file's page on fshare.vn
            // Each hides when its underlying data is missing (a download with
            // no retained local copy still has a linkcode; an upload always
            // has both).
            IconAction {
                icon: "folder"
                tooltip: qsTr("Mở thư mục chứa")
                visible: rRow._hasLocal && rowHover.hovered
                onActivated: rRow.folderClicked()
            }
            IconAction {
                icon: "external-link"
                tooltip: qsTr("Mở trên Fshare")
                visible: rRow._hasLink && rowHover.hovered
                onActivated: rRow.shareClicked()
            }
        }
    }

    component IconAction: Rectangle {
        id: ia
        property string icon: ""
        property string tooltip: ""
        signal activated()

        Layout.preferredWidth: 28
        Layout.preferredHeight: 28
        width: 28; height: 28
        radius: AuroraTheme.radiusSm
        color: iaMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
        FsIcon {
            anchors.centerIn: parent
            name: ia.icon
            sizePx: 14
            color: iaMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
        }
        MouseArea {
            id: iaMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: ia.activated()
            ToolTip.visible: containsMouse && ia.tooltip.length > 0
            ToolTip.text: ia.tooltip
            ToolTip.delay: 400
        }
    }
}
