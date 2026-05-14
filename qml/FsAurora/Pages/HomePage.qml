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

                    Aurora.FsIcon {
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

                        onAccepted: {
                            if (!homeSearchViewModel) return;
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
                onActivated: page.pageRequested(6)
            }

            // Upload CTA (gradient pill)
            Aurora.FsGradientRect {
                Layout.preferredWidth: uploadLbl.implicitWidth + 42
                Layout.preferredHeight: AuroraTheme.heightInput
                radius: AuroraTheme.radiusPill

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Aurora.FsIcon {
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
                    onClicked: page.pageRequested(1)
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
                    Layout.fillWidth: true
                    spacing: 0

                    Text {
                        text: qsTr("Chào ") + page._firstName + ", "
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 40
                        font.weight: Font.DemiBold
                        font.letterSpacing: -1.1
                        color: AuroraTheme.ink1
                    }

                    Text {
                        text: qsTr("tiếp tục?")
                        font.family: AuroraTheme.fontSerif
                        font.italic: true
                        font.pixelSize: 40
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
                    onActivated: page.pageRequested(1)
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "sync"
                    title: qsTr("Thêm folder sync")
                    subtitle: qsTr("2-way đồng bộ")
                    onActivated: page.pageRequested(2)
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "external-link"
                    title: qsTr("Tạo link chia sẻ")
                    subtitle: qsTr("Password + hết hạn")
                    onActivated: page.pageRequested(3)
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
                        text: {
                            const total = page._dlCount + page._ulCount;
                            let s = total + qsTr(" đang hoạt động");
                            const sp = page._dlSpeed || page._ulSpeed || "";
                            if (sp.length > 0) s += " · " + sp + qsTr(" tổng");
                            return s;
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
                            onOpenClicked: page.pageRequested(0)
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
                            onOpenClicked: page.pageRequested(1)
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
                        model: downloadViewModel ? downloadViewModel.historyModel : null

                        delegate: Loader {
                            width: recentList.width
                            active: page._matchesFilter(model.fileName || "")
                            sourceComponent: active ? recentRowComp : null
                            visible: active
                            height: active ? 60 : 0

                            property var _row: ({
                                fileName:  model.fileName  || "",
                                fileSize:  model.fileSize  || 0,
                                localPath: model.localPath || "",
                                completedAt: model.completedAt || 0
                            })

                            Component {
                                id: recentRowComp
                                RecentRow {
                                    width: parent ? parent.width : 0
                                    fileName:    _row.fileName
                                    fileSize:    _row.fileSize
                                    localPath:   _row.localPath
                                    completedAt: _row.completedAt
                                    nowMs:       page.nowMs
                                    onOpenClicked:  Qt.openUrlExternally("file:///" + localPath)
                                    onFolderClicked: {
                                        const p = (localPath || "").replace(/\\/g, "/");
                                        const dir = p.lastIndexOf("/") > 0 ? p.substring(0, p.lastIndexOf("/")) : p;
                                        Qt.openUrlExternally("file:///" + dir);
                                    }
                                }
                            }
                        }

                        // Empty state
                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: (!downloadViewModel
                                      || !downloadViewModel.historyModel
                                      || downloadViewModel.historyModel.count === 0)
                            spacing: AuroraTheme.sp2

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                width: 56; height: 56
                                radius: 28
                                color: AuroraTheme.accentTint10
                                Aurora.FsIcon {
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

        Aurora.FsIcon {
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

        Layout.preferredHeight: 108
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
                spacing: 0

                Rectangle {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    radius: AuroraTheme.radiusMd
                    color: AuroraTheme.accentTint10
                    Aurora.FsIcon {
                        anchors.centerIn: parent
                        name: card.icon
                        sizePx: 18
                        color: AuroraTheme.accent
                    }
                }
                Item { Layout.fillWidth: true }
                Aurora.FsIcon {
                    name: "chevron-right"
                    sizePx: 14
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

        Layout.preferredHeight: 86
        radius: AuroraTheme.radiusLg
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border

        // Status enum (mirrors TransferState): 0 Queued, 1 Active, 2 Paused,
        // 3 Complete, 4 Error, 5 Cancelled
        readonly property bool _paused: tStatus === 2

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp3
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp3

                FsFileTypeIcon {
                    fileName: tCard.tName
                    isFolder: false
                    sizePx: 36
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
                    Text {
                        text: {
                            const sz = FsFormat.bytes(tCard.tSize);
                            if (tCard._paused) return sz + " · " + qsTr("tạm dừng");
                            if (tCard.tEta.length > 0) return sz + " · " + qsTr("còn ") + tCard.tEta;
                            return sz;
                        }
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                        color: AuroraTheme.ink3
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
                    Aurora.FsIcon {
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

                Aurora.FsProgressBar {
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
        property string localPath: ""
        property real   completedAt: 0
        property real   nowMs: 0
        signal openClicked()
        signal folderClicked()

        height: 56
        radius: AuroraTheme.radiusMd
        color: rowMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
        border.width: 0
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: AuroraTheme.sp4
            anchors.rightMargin: AuroraTheme.sp4
            spacing: AuroraTheme.sp3

            FsFileTypeIcon {
                fileName: rRow.fileName
                isFolder: false
                sizePx: 36
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: rRow.fileName
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: AuroraTheme.ink1
                    elide: Text.ElideMiddle
                }
                Text {
                    text: FsFormat.bytes(rRow.fileSize) + " · " + page._relTime(rRow.completedAt)
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    color: AuroraTheme.ink3
                }
            }

            IconAction { icon: "folder";        tooltip: qsTr("Mở thư mục chứa"); onActivated: rRow.folderClicked() }
            IconAction { icon: "external-link"; tooltip: qsTr("Mở file");         onActivated: rRow.openClicked() }
        }

        MouseArea {
            id: rowMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onDoubleClicked: rRow.openClicked()
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
        Aurora.FsIcon {
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
