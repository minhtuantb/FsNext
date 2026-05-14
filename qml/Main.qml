// SPDX-License-Identifier: Proprietary
// Main.qml — Root window: login + main shell with sidebar nav (Aurora).

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import Fshare.Dialogs 1.0
import Fshare.Pages 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora
import FsAurora.Pages 1.0 as AuroraPages

ApplicationWindow {
    id: root

    width: 1100
    height: 720
    minimumWidth: 800
    minimumHeight: 560
    visible: true
    // Context-aware title: "FsNext — <page>" when logged in, plain "FsNext"
    // on the login screen.  Helps users with multiple FsNext-related tasks
    // identify the right window from Alt+Tab / taskbar peek.  Keep this in
    // lock-step with currentPage's mapping below — the indices must match.
    title: loggedIn
        ? ("FsNext — " + _pageTitles[currentPage])
        : "FsNext"
    color: AuroraTheme.bg

    property bool loggedIn: authViewModel ? authViewModel.isLoggedIn : false
    property int currentPage: 8  // 0=Tải xuống 1=Tải lên 2=Đồng bộ 3=File 4=Yêu thích 5=Tài khoản 6=Cài đặt 7=Showcase 8=Trang chủ

    // Page-name lookup driving the window title.  Index order MUST match the
    // currentPage comments above; an entry per page so out-of-range access
    // can't yield `undefined` in the title bar.
    readonly property var _pageTitles: [
        qsTr("Tải xuống"),   // 0
        qsTr("Tải lên"),     // 1
        qsTr("Đồng bộ"),     // 2
        qsTr("File"),        // 3
        qsTr("Yêu thích"),   // 4
        qsTr("Tài khoản"),   // 5
        qsTr("Cài đặt"),     // 6
        qsTr("Showcase"),    // 7
        qsTr("Trang chủ")    // 8
    ]

    // Load Aurora TTFs from qrc:/fonts/. Harmless when the font files are
    // missing from the build — FontLoader silently fails and the Aurora
    // design falls back to system fonts.
    Aurora.FsFontLoader { }

    // Emitted when something (drop / paste / future tray hook) brings Fshare
    // URLs into the app. DownloadPage listens and opens its add-dialog
    // pre-filled. Using a window-level signal keeps VMs free of UI concerns.
    signal openDownloadWithLinks(string links)

    // Emitted when local files are dropped for upload. UploadPage listens
    // and opens its upload dialog pre-seeded, so the user still chooses
    // target folder / password / privacy before the upload starts.
    signal openUploadWithFiles(var fileUrls)

    // Fired by the HomePage "Dán link & tải" quick-action. DownloadPage
    // listens and opens its add-dialog with an empty links field so the
    // user can paste manually.
    signal openDownloadDialog()

    // ── Global drag-drop routing ─────────────────────────────────────────
    // routeDrop(drop): inspects a DropEvent and dispatches it.
    //   • Local file URLs  → switch to Upload, open options dialog pre-seeded
    //   • Fshare page URLs → switch to Download, open add-dialog pre-filled
    //   • Plain text with Fshare URLs inside → Download
    // Called from the root DropArea AND from page-specific DropAreas that
    // want the same smart behavior (UploadPage keeps its one for the visual
    // drop zone, but delegates the routing here).
    function routeDrop(drop) {
        if (!loggedIn) return;

        const localFiles = [];
        const fshareUrls = [];

        if (drop.hasUrls) {
            for (let i = 0; i < drop.urls.length; ++i) {
                const u = drop.urls[i].toString();
                if (u.startsWith("file:")) {
                    localFiles.push(u);
                } else if (downloadViewModel
                           && downloadViewModel.extractFshareLinks(u).length > 0) {
                    fshareUrls.push(u);
                }
            }
        }

        // Accept drops that contain only text (link copied from a browser
        // address bar, a chat message, etc.).
        if (fshareUrls.length === 0 && drop.hasText && downloadViewModel) {
            const extracted = downloadViewModel.extractFshareLinks(drop.text);
            if (extracted.length > 0) {
                const lines = extracted.split('\n');
                for (let j = 0; j < lines.length; ++j)
                    if (lines[j].length > 0) fshareUrls.push(lines[j]);
            }
        }

        if (localFiles.length > 0 && uploadViewModel) {
            root.currentPage = 1; // Upload
            root.openUploadWithFiles(localFiles);
        }

        if (fshareUrls.length > 0) {
            root.currentPage = 0; // Download
            root.openDownloadWithLinks(fshareUrls.join('\n'));
        }
    }

    // routePastedText(text): clipboard-paste equivalent of routeDrop.
    // Only routes to Download — pasting a local filename from the clipboard
    // is not a standard OS flow, so Upload is drag-only.
    function routePastedText(text) {
        if (!loggedIn || !downloadViewModel) return;
        const extracted = downloadViewModel.extractFshareLinks(text);
        if (extracted.length === 0) return;
        root.currentPage = 0;
        root.openDownloadWithLinks(extracted);
    }

    // ── Dark mode wiring ─────────────────────────────
    // AuroraTheme.isDark is the single source of truth for visuals — the legacy
    // FshareTheme mirror was removed once every component/page was ported.
    Component.onCompleted: {
        if (settingsViewModel)
            AuroraTheme.isDark = settingsViewModel.darkMode;
        // OS Reduce Motion preference flows in through the `osPrefersReducedMotion`
        // context property (see Application::initAccessibility). We push it to the
        // theme here so every Behavior gated on AuroraTheme.reduceMotion respects it.
        if (typeof osPrefersReducedMotion !== "undefined")
            AuroraTheme.reduceMotion = osPrefersReducedMotion;
    }
    Connections {
        target: settingsViewModel
        function onDarkModeChanged() {
            AuroraTheme.isDark = settingsViewModel.darkMode;
        }
    }

    // Smooth color transitions on the root window
    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durBase } }

    // ── Minimize-to-tray on close ───────────────────────
    // When the user clicks the [X] button and Settings → Bảo mật → Thu nhỏ vào
    // khay is enabled, hide the window instead of letting it close.  The tray
    // (set up in main.cpp) keeps the app alive and the user restores via tray
    // double-click.  Only honoured if a tray actually exists — main.cpp gates
    // setQuitOnLastWindowClosed(false) on tray.setup() success, so closing the
    // window without a tray still quits cleanly even if the toggle is on.
    onClosing: (close) => {
        if (loggedIn && settingsViewModel && settingsViewModel.minimizeToTray) {
            close.accepted = false;
            root.hide();
        }
    }

    // Dev-only: Ctrl+Shift+D toggles dark mode for visual smoke-testing without
    // going through Settings page.
    Shortcut {
        sequence: "Ctrl+Shift+D"
        enabled: typeof isDevBuild !== "undefined" && isDevBuild
        onActivated: AuroraTheme.isDark = !AuroraTheme.isDark
    }

    // Ctrl+V anywhere in the shell: if the clipboard holds one or more
    // fshare.vn links, jump to Download and open the add-dialog pre-filled.
    // Silent no-op otherwise so it doesn't interfere with normal paste into
    // text fields (those consume the event before this fires).
    Shortcut {
        // StandardKey.Paste expands to multiple platform bindings (Ctrl+V on
        // Windows, ⌘+V on macOS, Shift+Insert on X11).  The singular
        // `sequence` property only attaches to the FIRST of those, leaving
        // the others wired to whatever default handler exists — which on
        // Qt 6 prints a "binding to one of multiple key bindings" warning.
        // `sequences` (plural) wraps the expansion so every variant routes
        // to this onActivated handler.
        sequences: [StandardKey.Paste]
        context: Qt.WindowShortcut
        enabled: loggedIn && downloadViewModel
        onActivated: {
            const txt = downloadViewModel.clipboardText();
            if (txt.length > 0) routePastedText(txt);
        }
    }

    // ── Command palette (⌘/Ctrl+K) ───────────────────────────────
    // Unified entry point for navigation + frequent actions.  Kept tiny on
    // purpose; we will grow the command list as features need surfacing.
    FsCommandPalette {
        id: cmdPalette
        anchors.centerIn: parent
        commands: [
            { id: "go.home",     label: qsTr("Trang chủ"),     hint: qsTr("Mở dashboard"),       icon: "⌂" },
            { id: "go.download", label: qsTr("Tải xuống"),     hint: qsTr("Mở trang tải xuống"), icon: "↓" },
            { id: "go.upload",   label: qsTr("Tải lên"),       hint: qsTr("Mở trang tải lên"),   icon: "↑" },
            { id: "go.sync",     label: qsTr("Đồng bộ"),       hint: qsTr("Mở trang đồng bộ"),   icon: "⟲" },
            { id: "go.files",    label: qsTr("File"),          hint: qsTr("Mở trình quản lý file"), icon: "▤" },
            { id: "go.fav",      label: qsTr("Yêu thích"),     hint: qsTr("Mở danh sách yêu thích"), icon: "★" },
            { id: "go.account",  label: qsTr("Tài khoản"),     hint: qsTr("Mở thông tin tài khoản"), icon: "◉" },
            { id: "go.settings", label: qsTr("Cài đặt"),       hint: qsTr("Mở trang cài đặt"),   icon: "⚙" },
            { id: "act.add-dl",  label: qsTr("Thêm tải xuống"), hint: qsTr("Dán link Fshare để bắt đầu"), icon: "+" },
            { id: "act.logout",  label: qsTr("Đăng xuất"),     hint: qsTr("Đăng xuất khỏi Fshare"), icon: "⏻" }
        ]
        onTriggered: (id) => {
            switch (id) {
            case "go.home":     root.currentPage = 8; break;
            case "go.download": root.currentPage = 0; break;
            case "go.upload":   root.currentPage = 1; break;
            case "go.sync":     root.currentPage = 2; break;
            case "go.files":    root.currentPage = 3; break;
            case "go.fav":      root.currentPage = 4; break;
            case "go.account":  root.currentPage = 5; break;
            case "go.settings": root.currentPage = 6; break;
            case "act.add-dl":  root.currentPage = 0; root.openDownloadDialog(); break;
            case "act.logout":  if (authViewModel) authViewModel.logout(); break;
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+K"
        context: Qt.WindowShortcut
        enabled: loggedIn
        onActivated: cmdPalette.open()
    }

    // ════════════════════════════════════════════
    //  LOGIN VIEW
    // ════════════════════════════════════════════
    Loader {
        anchors.fill: parent
        active: !loggedIn
        sourceComponent: auroraLoginComp
    }
    Component {
        id: auroraLoginComp
        AuroraPages.LoginView {
            authVm: authViewModel
        }
    }

    // ════════════════════════════════════════════
    //  MAIN SHELL: SIDEBAR + CONTENT
    // ════════════════════════════════════════════
    Item {
        anchors.fill: parent
        visible: loggedIn

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Aurora.FsSidebar {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                currentPage: root.currentPage
                devBuild: typeof isDevBuild !== "undefined" && isDevBuild
                userName: authViewModel ? authViewModel.userName : ""
                userEmail: authViewModel ? authViewModel.userEmail : ""

                // VIP + storage hero card. userInfoViewModel may be null
                // before /profile responds — fall back to empty strings so
                // the sidebar still renders.
                vipLabel: {
                    if (!userInfoViewModel) return "";
                    const lvl = userInfoViewModel.levelLabel || "";
                    const exp = userInfoViewModel.vipExpiry || "";
                    if (lvl.length === 0) return "";
                    return exp.length > 0 ? (lvl + " · " + exp) : lvl;
                }
                storageUsedText: userInfoViewModel
                    ? FsFormat.bytes(userInfoViewModel.webspaceUsed + userInfoViewModel.secureUsed) : ""
                storageTotalText: userInfoViewModel
                    ? FsFormat.bytes(userInfoViewModel.webspaceTotal + userInfoViewModel.secureTotal) : ""
                storagePct: {
                    if (!userInfoViewModel) return 0;
                    const t = userInfoViewModel.webspaceTotal + userInfoViewModel.secureTotal;
                    const u = userInfoViewModel.webspaceUsed + userInfoViewModel.secureUsed;
                    return t > 0 ? Math.min(1, u / t) : 0;
                }

                transferStatsVisible: transferBudgetViewModel
                    && (transferBudgetViewModel.activeTotal > 0
                        || (transferBudgetViewModel.pendingDownloads
                            + transferBudgetViewModel.pendingUploads) > 0)
                transferStatsText: transferBudgetViewModel
                    ? "DL " + transferBudgetViewModel.activeDownloads
                        + "/" + transferBudgetViewModel.maxDownloads
                        + " · UL " + transferBudgetViewModel.activeUploads
                        + "/" + transferBudgetViewModel.maxUploads
                        + ((transferBudgetViewModel.pendingDownloads
                            + transferBudgetViewModel.pendingUploads) > 0
                            ? " · Q " + (transferBudgetViewModel.pendingDownloads
                                + transferBudgetViewModel.pendingUploads)
                            : "")
                    : ""
                // Sync pending count: drives the small red pill next to the
                // "Đồng bộ" nav entry.  Falls back to 0 when the VM isn't
                // available yet (pre-login, AppContext mid-init) so the
                // binding never reads `undefined`.
                syncPendingCount: syncViewModel ? syncViewModel.pendingCount : 0
                onNavClicked: (idx) => root.currentPage = idx
                onLogoutClicked: if (authViewModel) authViewModel.logout()
                onUpgradeClicked: Qt.openUrlExternally("https://www.fshare.vn/upgrade")
            }

            // ── Content area ─────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: AuroraTheme.bg
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durBase } }

                // Page container with consistent padding
                Item {
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp6

                    // Use loaders so pages aren't all instantiated at once
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 0
                        sourceComponent: active ? downloadPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 1
                        sourceComponent: active ? uploadPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 2
                        sourceComponent: active ? syncPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 3
                        sourceComponent: active ? fileManagerPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 4
                        sourceComponent: active ? favoritesPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 5
                        sourceComponent: active ? userInfoPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 6
                        sourceComponent: active ? settingsPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 7
                        sourceComponent: active ? auroraShowcasePageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === 8
                        sourceComponent: active ? homePageComp : null
                    }
                }
            }
        }
    }

    // ════════════════════════════════════════════
    //  GLOBAL DRAG-DROP OVERLAY
    //  Declared AFTER all content so it hit-tests first: a drag anywhere in
    //  the window is caught here, even when the current page has no drop
    //  zone of its own. Pages that want richer per-page visual feedback
    //  (UploadPage's big dashed box) still declare their own DropArea — the
    //  inner one takes precedence; otherwise this catches.
    // ════════════════════════════════════════════
    DropArea {
        id: globalDropArea
        anchors.fill: parent
        enabled: loggedIn
        onDropped: (drop) => root.routeDrop(drop)

        // Full-window overlay only while a drag is hovering, so the normal
        // UI isn't dimmed. The overlay is decorative — events pass through
        // to the DropArea's own handler via the DropArea itself.
        Rectangle {
            anchors.fill: parent
            visible: globalDropArea.containsDrag
            color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.08)
            border.color: AuroraTheme.accent
            border.width: 3
            radius: AuroraTheme.radiusLg

            ColumnLayout {
                anchors.centerIn: parent
                spacing: AuroraTheme.sp2

                Aurora.FsIcon {
                    Layout.alignment: Qt.AlignHCenter
                    name: "sparkle"
                    sizePx: 48
                    color: AuroraTheme.accent
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Thả file để tải lên · Thả link Fshare để tải xuống")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.h3.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.accent
                }
            }
        }
    }

    // Page components (lazy-loaded)
    Component { id: downloadPageComp;       DownloadPage {} }
    Component { id: uploadPageComp;         UploadPage {} }
    Component { id: syncPageComp;           SyncPage {} }
    Component { id: fileManagerPageComp;    FileManagerPage {} }
    Component { id: favoritesPageComp;      FavoritesPage {} }
    Component { id: userInfoPageComp;       UserInfoPage {} }
    Component { id: settingsPageComp;       SettingsPage {} }
    Component { id: auroraShowcasePageComp; AuroraPages.ShowcasePage {} }
    Component {
        id: homePageComp
        AuroraPages.HomePage {
            onPageRequested: (idx) => root.currentPage = idx
            onAddDownloadRequested: (links) => {
                root.currentPage = 0;
                if (links && links.length > 0) root.openDownloadWithLinks(links);
                else                           root.openDownloadDialog();
            }
            // A file/folder share URL pasted into the homepage search opens
            // the matching surface. Both VMs talk to `remoteShareViewModel`
            // (context property) so the dialogs themselves stay stateless.
            onShowFileUrlRequested: (url) => {
                if (remoteShareViewModel) remoteShareViewModel.openFile(url);
                fileDetailSheet.open();
            }
            onShowFolderUrlRequested: (url) => {
                if (remoteShareViewModel) remoteShareViewModel.openFolder(url);
                folderBrowserDialog.open();
            }
            // Phase 3: clicking a row in the inline search overlay routes
            // through the same detail surface as a pasted file URL would —
            // synthesise the canonical URL from the linkcode (account-owned
            // files don't carry a share-access token).
            onShowFileFromLinkcode: (linkcode, _name) => {
                if (!remoteShareViewModel || linkcode.length === 0) return;
                remoteShareViewModel.openFile("https://www.fshare.vn/file/" + linkcode);
                fileDetailSheet.open();
            }
            // Legacy keyword route (→ FileManager) intentionally dropped.
            onSearchRequested: (_query) => { /* no-op */ }
            onUpgradeRequested: Qt.openUrlExternally("https://www.fshare.vn/upgrade")
        }
    }

    // ── Session-expired toast ────────────────────────────────────
    FsToast {
        id: sessionToast
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: AuroraTheme.sp4
        anchors.rightMargin: AuroraTheme.sp4
        visible: false
        variant: "warning"
        title: qsTr("Phiên đăng nhập hết hạn")
        desc: ""
        autoCloseMs: 6000
        onClosed: visible = false
    }

    Connections {
        target: authViewModel
        function onSessionExpiredNotice(message) {
            sessionToast.desc = message.length > 0
                ? message
                : qsTr("Vui lòng đăng nhập lại để tiếp tục.")
            sessionToast.visible = true;
        }
    }

    // ── Remote share routing surfaces ──────────────────────────
    // Single instances, lazy-shown — the dialogs themselves bind to
    // remoteShareViewModel and reset its state on close().
    FolderBrowserDialog {
        id: folderBrowserDialog
        // When the user clicks a file inside the folder browser, we want to
        // pop the folder context and switch to the file-detail surface for
        // that one file. The VM's openFile() resets folder state for us, so
        // we just hand it the synthesised file URL.
        onFileRowOpened: (linkcode, _row) => {
            if (!remoteShareViewModel) return;
            // Hand off to the file-detail surface while preserving the
            // share-access ?token= from the current folder context. The VM
            // does the token bookkeeping; QML just closes one dialog and
            // opens the other.
            folderBrowserDialog.visible = false;   // skip close() — keep VM state alive
            remoteShareViewModel.openFileFromCurrentFolder(linkcode);
            fileDetailSheet.open();
        }
    }

    FileDetailSheet {
        id: fileDetailSheet
    }
}
