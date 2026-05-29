// SPDX-License-Identifier: Proprietary
// Main.qml — Root window: login + main shell with sidebar nav (Aurora).

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Fshare.Components 1.0
import Fshare.Dialogs 1.0
import Fshare.Pages 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora
import FsAurora.Pages 1.0 as AuroraPages
import FsAurora.Windows 1.0 as AuroraWindows

ApplicationWindow {
    id: root

    width: 1100
    height: 720
    minimumWidth: 800
    minimumHeight: 560
    visible: true
    // Context-aware title: "Fshare — <page>" when logged in, plain "Fshare"
    // on the login screen.  Brand string shown in Alt+Tab / taskbar peek
    // matches the public product name (users know "Fshare", not "FsNext").
    // Dev builds keep the "FsNext (dev)" prefix so we can tell them apart
    // from a shipped client side-by-side. Keep in lock-step with
    // currentPage's mapping below — the indices must match.
    readonly property string _brand: (typeof isDevBuild !== "undefined" && isDevBuild)
        ? "FsNext (dev)" : "Fshare"
    title: loggedIn
        ? (_brand + " — " + Pages.titles[currentPage])
        : _brand
    color: AuroraTheme.bg

    property bool loggedIn: authViewModel ? authViewModel.isLoggedIn : false
    // Symbolic page indices live in FsAurora.Theme.Pages so adding/reordering
    // pages updates one file. Default landing: Trang chủ.
    property int currentPage: Pages.home

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

    // Fired by the Ctrl+U shortcut. UploadPage listens and opens the OS
    // file picker directly — user picks files, then the upload options
    // dialog appears with the pending files (same flow as drag-drop).
    signal openUploadDialog()

    // ── Drag-drop handoff via property (not signal) ──────────────────────
    // When a drag-drop arrives BEFORE the destination page is loaded for
    // the first time, the page's `Connections { target: Window.window }`
    // block may not yet be set up when the signal fires, so the handler
    // never gets called and the files vanish. Properties don't have this
    // problem: a freshly-loaded page can read the current value AND
    // listen for future changes from the same Connections block.
    //
    // routeDrop() now writes localFiles/fshareUrls here and flips the
    // page; the destination page consumes (and clears) the property in
    // its Component.onCompleted plus a Connections handler for repeat
    // drops while it's already in view.
    property var pendingUploadFiles: []
    property string pendingDownloadLinks: ""

    // Companion sequence counter — bumped every time pendingUploadFiles is
    // written from routeDrop / onOpenUploadWithFiles. UploadPage listens to
    // THIS instead of pendingUploadFiles directly because Qt 6.8 has been
    // observed deduping `property var` change notifications in some build
    // configurations, swallowing rapid re-drops. An int counter never gets
    // deduped (always-different value), so the listener fires reliably.
    // pendingUploadFiles remains for the page's Component.onCompleted to
    // read on first load.
    property int pendingUploadFilesSeq: 0

    // The legacy signals are still declared above; they're the entry
    // point C++ / Chrome-native-host / single-instance use to push URLs
    // in. We catch them here at the root and convert into the property
    // form so every code path takes the same robust handoff.
    onOpenDownloadWithLinks: (links) => {
        if (!links || links.length === 0) return;
        root.pendingDownloadLinks = links;
        root.currentPage = Pages.download;
    }
    onOpenUploadWithFiles: (fileUrls) => {
        if (!fileUrls || fileUrls.length === 0) return;
        // Staging VM (AppContext-owned, survives page-Loader teardown) is the
        // single source of truth for "files queued but not yet uploading". The
        // old pendingUploadFiles + seq dance died with QML 6.8's Connections
        // quirk and was lost on every page switch. See UploadStagingViewModel.h.
        if (uploadStagingViewModel) uploadStagingViewModel.addFiles(fileUrls);
        root.currentPage = Pages.upload;
    }

    // ── Global drag-drop routing ─────────────────────────────────────────
    // routeDrop(drop): inspects a DropEvent and dispatches it.
    //   • Local file URLs  → switch to Upload, open options dialog pre-seeded
    //   • Fshare page URLs → switch to Download, open add-dialog pre-filled
    //   • Plain text with Fshare URLs inside → Download
    // Called from the root DropArea AND from page-specific DropAreas that
    // want the same smart behavior (UploadPage keeps its one for the visual
    // drop zone, but delegates the routing here).
    function routeDrop(drop) {
        if (!loggedIn) { console.info("[routeDrop] skip — not logged in"); return; }

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

        console.info("[routeDrop] localFiles=", localFiles.length,
                     " fshareUrls=", fshareUrls.length,
                     " currentPage=", root.currentPage);

        if (localFiles.length > 0 && uploadStagingViewModel) {
            // Add to the AppContext-owned staging VM. It survives page-Loader
            // teardown so navigating Home → drop file → Upload preserves any
            // batch the user had assembled. The VM raises showRequested → the
            // dialog auto-opens (UploadPage handler). See ADR — replaces the
            // pendingUploadFiles + pendingUploadFilesSeq dance that broke after
            // the first drop due to a QML 6.8 Connections-target quirk.
            uploadStagingViewModel.addFiles(localFiles);
            root.currentPage = Pages.upload;
        }

        if (fshareUrls.length > 0) {
            root.pendingDownloadLinks = fshareUrls.join('\n');
            root.currentPage = Pages.download;
        }
    }

    // routePastedText(text): clipboard-paste equivalent of routeDrop.
    // Only routes to Download — pasting a local filename from the clipboard
    // is not a standard OS flow, so Upload is drag-only.
    function routePastedText(text) {
        if (!loggedIn || !downloadViewModel) return;
        const extracted = downloadViewModel.extractFshareLinks(text);
        if (extracted.length === 0) return;
        root.pendingDownloadLinks = extracted;
        root.currentPage = Pages.download;
    }

    // ── Dark mode wiring ─────────────────────────────
    // AuroraTheme.isDark is the single source of truth for visuals.
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

    // ── Close-button behaviour ───────────────────────────
    // Three states, gated on `settingsViewModel.confirmOnClose`:
    //   1) confirmOnClose = true (default)        → popup asks user
    //   2) confirmOnClose = false + minimizeToTray = true  → silent minimize
    //   3) confirmOnClose = false + minimizeToTray = false → silent full quit
    //
    // The popup itself (CloseConfirmDialog below) writes both flags when the
    // user ticks "Don't ask again", so subsequent X presses honour the
    // chosen action without re-asking.
    //
    // We always set close.accepted=false initially: the dialog (or the
    // explicit-quit path) decides whether to actually hide/quit. Letting
    // QML accept the close while the app stays alive in tray (the bug the
    // user reported) is exactly the "vanishes silently" surprise we want
    // to avoid.
    onClosing: (close) => {
        if (!loggedIn || !settingsViewModel) {
            // Not logged in (e.g. on the login screen) — let X behave like
            // a normal quit. Tray isn't useful in that state.
            Qt.quit();
            return;
        }
        // ── Data-safety guard ───────────────────────────────────────────
        // If any transfer is in flight (active OR queued/pending), ALWAYS
        // show the confirm dialog — even when confirmOnClose=false. Silently
        // quitting here would kill the user's downloads/uploads, which is
        // exactly the "vanished my upload" surprise we must never cause.
        // The dialog's "Thu nhỏ vào khay" keeps them running; "Thoát hẳn"
        // pauses + persists first. See transfer-hud-ux-spec §2.4.
        const hasInFlight = transferHudViewModel
            && (transferHudViewModel.activeTotal
                + transferHudViewModel.pendingTotal) > 0;
        if (settingsViewModel.confirmOnClose || hasInFlight) {
            close.accepted = false;
            closeConfirmDialog.open();
            return;
        }
        if (settingsViewModel.minimizeToTray) {
            close.accepted = false;
            root.hide();
            // P2: when there's active transfer activity, surface the
            // floating mini HUD so progress stays visible.  Gated by
            // showOnHideToTray so users who find it noisy can opt out.
            // acknowledgeMini() clears any prior "user dismissed" flag —
            // closing the main window is an implicit "I want passive
            // feedback again".
            if (settingsViewModel.showOnHideToTray
                && transferHudViewModel
                && transferHudViewModel.shouldShowMini) {
                transferHudViewModel.acknowledgeMini();
                miniHudWindow.bindToVisibility(true);
            }
        } else {
            // User opted out of the dialog AND opted out of tray → fully
            // quit. Qt.quit() ignores setQuitOnLastWindowClosed(false) and
            // tears the QApplication event loop down, which also drops the
            // tray icon (SystemTray dtor fires).
            Qt.quit();
        }
    }

    // ── Close-confirmation dialog ──────────────────────────────────
    // Rebuilt on FsDialog + FsButton + FsCheckbox so the visual language
    // matches every other modal in the app (Aurora panel, accent gradient
    // primary button, ghost cancel, themed checkbox). Three actions:
    //   • Huỷ           — dismiss, do nothing
    //   • Thu nhỏ       — hide window, keep app alive in tray
    //   • Thoát hẳn     — full Qt.quit() (drops tray too)
    //
    // The "Không hỏi lại" checkbox writes BOTH `confirmOnClose=false` AND
    // the matching `minimizeToTray` flag when the user makes a choice, so
    // subsequent X presses honour that choice silently. Resetting the
    // checkbox on every open prevents sticky-checkbox confusion.
    FsDialog {
        id: closeConfirmDialog
        title: qsTr("Thoát Fshare?")
        dialogWidth: 460

        property bool _dontAskAgain: false
        // Snapshot of in-flight count when the dialog opens — drives the
        // data-loss warning banner + whether "Không hỏi lại" is allowed.
        property int _inFlight: transferHudViewModel
            ? (transferHudViewModel.activeTotal + transferHudViewModel.pendingTotal)
            : 0

        onOpened: {
            _dontAskAgain = false;
            _inFlight = transferHudViewModel
                ? (transferHudViewModel.activeTotal + transferHudViewModel.pendingTotal)
                : 0;
        }

        content: [
            Item {
                width: closeConfirmDialog.dialogWidth
                height: closeBody.implicitHeight + AuroraTheme.sp6 * 2

                ColumnLayout {
                    id: closeBody
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.rightMargin: AuroraTheme.sp6
                    anchors.topMargin: AuroraTheme.sp4
                    spacing: AuroraTheme.sp3

                    // Icon + body text row — mirrors the Home quick-action
                    // card pattern (40px tinted square + content beside).
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AuroraTheme.sp4

                        Rectangle {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            Layout.alignment: Qt.AlignTop
                            radius: AuroraTheme.radiusMd
                            color: AuroraTheme.accentTint10
                            Aurora.FsIcon {
                                anchors.centerIn: parent
                                name: "power"
                                sizePx: 22
                                color: AuroraTheme.accent
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp1

                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Bạn muốn đóng cửa sổ chính?")
                                color: AuroraTheme.ink1
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Chọn <b>Thu nhỏ vào khay</b> để giữ app "
                                         + "chạy nền (các tải xuống đang chạy sẽ tiếp tục). "
                                         + "Chọn <b>Thoát hẳn</b> để đóng hoàn toàn.")
                                textFormat: Text.RichText
                                color: AuroraTheme.ink3
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                lineHeight: 1.4
                            }
                        }
                    }

                    // Data-loss warning — only when transfers are in flight.
                    // Amber banner so the user can't miss that "Thoát hẳn"
                    // will stop their downloads/uploads.
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: AuroraTheme.sp1
                        visible: closeConfirmDialog._inFlight > 0
                        radius: AuroraTheme.radiusSm
                        color: AuroraTheme.warnSoft
                        implicitHeight: warnRow.implicitHeight + AuroraTheme.sp3 * 2
                        RowLayout {
                            id: warnRow
                            anchors.fill: parent
                            anchors.margins: AuroraTheme.sp3
                            spacing: AuroraTheme.sp2
                            Aurora.FsIcon {
                                Layout.alignment: Qt.AlignTop
                                name: "info"
                                sizePx: 16
                                color: AuroraTheme.warn
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Đang có %1 lượt chuyển chưa xong. "
                                         + "Thoát hẳn sẽ tạm dừng tất cả.")
                                         .arg(closeConfirmDialog._inFlight)
                                color: AuroraTheme.warn
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 12
                                lineHeight: 1.35
                            }
                        }
                    }

                    // "Don't ask again" — themed Aurora checkbox. Disabled
                    // while transfers run: the data-safety guard always
                    // re-prompts on active transfer, so persisting "don't ask"
                    // here would be misleading.
                    FsCheckbox {
                        Layout.fillWidth: true
                        Layout.topMargin: AuroraTheme.sp2
                        Layout.leftMargin: 44 + AuroraTheme.sp4 // align under body text
                        enabled: closeConfirmDialog._inFlight === 0
                        opacity: enabled ? 1.0 : 0.45
                        label: closeConfirmDialog._inFlight === 0
                               ? qsTr("Không hỏi lại lần sau")
                               : qsTr("Không hỏi lại (chỉ khi không có lượt chuyển)")
                        checked: closeConfirmDialog._dontAskAgain
                        onToggled: closeConfirmDialog._dontAskAgain = checked
                    }
                }
            }
        ]

        footer: [
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: AuroraTheme.sp4
                spacing: AuroraTheme.sp2
                height: 64

                // Aurora.FsButton — flagship atom of the design system,
                // hand-tuned with the brand 135° gradient + orange glow
                // shadow on primary. The legacy Fshare.Components.FsButton
                // existed mostly for back-compat; this dialog should use
                // the canonical Aurora variant like every other modern
                // surface in the shell.
                Aurora.FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Huỷ")
                    variant: "ghost"
                    onClicked: closeConfirmDialog.close()
                }
                Aurora.FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Thu nhỏ vào khay")
                    icon: "arrow-down"
                    variant: "secondary"
                    onClicked: {
                        if (closeConfirmDialog._dontAskAgain && settingsViewModel) {
                            settingsViewModel.confirmOnClose = false;
                            settingsViewModel.minimizeToTray = true;
                        }
                        closeConfirmDialog.close();
                        root.hide();
                        // P2: trigger the floating mini HUD on this code
                        // path too — same logic as the silent-minimize branch
                        // in root.onClosing so the dialog and the no-dialog
                        // paths converge on identical post-hide behaviour.
                        if (settingsViewModel
                            && settingsViewModel.showOnHideToTray
                            && transferHudViewModel
                            && transferHudViewModel.shouldShowMini) {
                            transferHudViewModel.acknowledgeMini();
                            miniHudWindow.bindToVisibility(true);
                        }
                    }
                }
                Aurora.FsButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Thoát hẳn")
                    icon: "power"
                    variant: "primary"
                    onClicked: {
                        // Only persist the "don't ask" choice when nothing is
                        // in flight — otherwise the data-safety guard would
                        // contradict it next time anyway.
                        if (closeConfirmDialog._dontAskAgain
                            && closeConfirmDialog._inFlight === 0
                            && settingsViewModel) {
                            settingsViewModel.confirmOnClose = false;
                            settingsViewModel.minimizeToTray = false;
                        }
                        closeConfirmDialog.close();
                        // Data safety: pause everything so the engines stop
                        // writing and TransferService's state-change snapshot
                        // (ADR D12) persists progress before we tear down.
                        if (transferHudViewModel) transferHudViewModel.pauseAll();
                        Qt.quit();
                    }
                }
            }
        ]
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
            case "go.home":     root.currentPage = Pages.home;      break;
            case "go.download": root.currentPage = Pages.download;  break;
            case "go.upload":   root.currentPage = Pages.upload;    break;
            case "go.sync":     root.currentPage = Pages.sync;      break;
            case "go.files":    root.currentPage = Pages.files;     break;
            case "go.fav":      root.currentPage = Pages.favorites; break;
            case "go.account":  root.currentPage = Pages.account;   break;
            case "go.settings": root.currentPage = Pages.settings;  break;
            case "act.add-dl":  root.currentPage = Pages.download;  root.openDownloadDialog(); break;
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

    // ── Ctrl+U: jump to Upload + open OS file picker ────────────
    // Power-user shortcut for the most common entry into the upload flow.
    // Order matters: navigate first so the lazy Loader for UploadPage
    // instantiates the page (and its Connections handler) before we emit.
    Shortcut {
        sequence: "Ctrl+U"
        context: Qt.WindowShortcut
        enabled: loggedIn
        onActivated: {
            root.currentPage = Pages.upload;
            root.openUploadDialog();
        }
    }

    // ── Ctrl+, : jump to Settings (matches macOS Preferences convention) ──
    Shortcut {
        sequence: "Ctrl+,"
        context: Qt.WindowShortcut
        enabled: loggedIn
        onActivated: root.currentPage = Pages.settings
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
                id: sidebar
                // Track our own animated `width` so the surrounding RowLayout
                // resizes alongside the collapse animation. Pinning a fixed
                // Layout.preferredWidth here would fight the Behavior on width.
                Layout.preferredWidth: sidebar.width
                Layout.fillHeight: true
                currentPage: root.currentPage
                devBuild: typeof isDevBuild !== "undefined" && isDevBuild
                userName: authViewModel ? authViewModel.userName : ""
                userEmail: authViewModel ? authViewModel.userEmail : ""

                // Collapsed state persists in QSettings via SettingsService.
                // Default false (expanded) on first launch.
                collapsed: settingsViewModel ? settingsViewModel.sidebarCollapsed : false
                onToggleCollapseRequested: {
                    if (settingsViewModel)
                        settingsViewModel.sidebarCollapsed = !settingsViewModel.sidebarCollapsed;
                }

                // Account tier — picks gradient hero (VIP) vs quiet hero
                // (free). userInfoViewModel may be null pre-/profile;
                // default to false so a fresh login sees the free variant
                // until real data arrives (subtler than flashing the
                // orange gradient and then dropping it).
                isVip: userInfoViewModel ? userInfoViewModel.isVip : false

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

                // Pause-aware: transferHudViewModel.activeTotal already includes
                // Paused tasks with progress (see VM comment), so the HUD stays
                // visible while the user has work-in-flight that they merely
                // paused.  Falls back to budgetVM only if hudVM isn't bound yet.
                transferStatsVisible: transferHudViewModel
                    ? (transferHudViewModel.activeTotal + transferHudViewModel.pendingTotal) > 0
                    : (transferBudgetViewModel
                       && (transferBudgetViewModel.activeTotal > 0
                           || (transferBudgetViewModel.pendingDownloads
                               + transferBudgetViewModel.pendingUploads) > 0))
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

                    // Use loaders so pages aren't all instantiated at once.
                    // Download / Upload loaders carry an id so the HUD's
                    // taskFocusRequested router (root._dispatchFocusTask)
                    // can reach into their items and call focusTask(id).
                    Loader {
                        id: downloadLoader
                        anchors.fill: parent
                        active: currentPage === Pages.download
                        sourceComponent: active ? downloadPageComp : null
                    }
                    Loader {
                        id: uploadLoader
                        anchors.fill: parent
                        active: currentPage === Pages.upload
                        sourceComponent: active ? uploadPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.sync
                        sourceComponent: active ? syncPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.files
                        sourceComponent: active ? fileManagerPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.favorites
                        sourceComponent: active ? favoritesPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.account
                        sourceComponent: active ? userInfoPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.settings
                        sourceComponent: active ? settingsPageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.showcase
                        sourceComponent: active ? auroraShowcasePageComp : null
                    }
                    Loader {
                        anchors.fill: parent
                        active: currentPage === Pages.home
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
        // Diagnostic for task #14 — remove after the drag-drop bug is
        // confirmed fixed in user testing. Tells us at runtime whether
        // the OS-level drag event even reaches the QML scene, and which
        // DropArea (root vs page-inner) actually catches it.
        onEntered: (drag) => console.info("[Drop] entered hasUrls=", drag.hasUrls,
                                          " hasText=", drag.hasText,
                                          " currentPage=", root.currentPage)
        onDropped: (drop) => {
            console.info("[Drop] dropped urls=", drop.urls,
                         " currentPage=", root.currentPage);
            root.routeDrop(drop);
        }

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
                if (links && links.length > 0) root.pendingDownloadLinks = links;
                root.currentPage = Pages.download;
                if (!links || links.length === 0) root.openDownloadDialog();
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

    // ── Toast queue manager ──────────────────────────────────────
    // One FsToastHost handles every toast surface in the shell: session-
    // expired warnings, share-action confirmations, copy-link notices etc.
    // Up to 3 visible at once stacked top-right; overflow is queued FIFO.
    Aurora.FsToastHost {
        id: toastHost
    }

    Connections {
        target: authViewModel
        function onSessionExpiredNotice(message) {
            toastHost.show({
                title:       qsTr("Phiên đăng nhập hết hạn"),
                desc:        message.length > 0
                                ? message
                                : qsTr("Vui lòng đăng nhập lại để tiếp tục."),
                variant:     "warning",
                autoCloseMs: 6000
            });
        }
    }

    // HomePage "recent files" single-click → copy fshare link. The VM puts
    // the URL on the clipboard and emits this; we confirm with a toast.
    Connections {
        target: downloadViewModel
        function onShareLinkCopied(url) {
            toastHost.show({
                title:   qsTr("Đã sao chép link Fshare"),
                desc:    url,
                variant: "success"
            });
        }
    }

    // ── Remote share routing surfaces ──────────────────────────
    // Single instances, lazy-shown — the dialogs themselves bind to
    // remoteShareViewModel and reset its state on close().
    FolderBrowserDialog {
        id: folderBrowserDialog
        onFileRowOpened: (linkcode, _row) => {
            if (!remoteShareViewModel) return;
            // openFileFromCurrentFolder snapshots the folder state, so
            // closing the detail sheet later can pop us back here.
            folderBrowserDialog.visible = false;   // skip close()/vm.close() — snapshot still live
            remoteShareViewModel.openFileFromCurrentFolder(linkcode);
            fileDetailSheet.open();
        }
        onClosed: {
            // User dismissed the folder browser explicitly (X / Esc /
            // backdrop). Tear down the VM so a future search URL doesn't
            // reopen stale data.
            if (remoteShareViewModel) remoteShareViewModel.close();
        }
    }

    FileDetailSheet {
        id: fileDetailSheet
        onClosed: {
            if (!remoteShareViewModel) return;
            // If the user drilled in from a folder, pop back to that
            // folder instead of dismissing everything.
            if (remoteShareViewModel.hasFolderContext
                && remoteShareViewModel.restoreFolderContext()) {
                folderBrowserDialog.open();
            } else {
                remoteShareViewModel.close();
            }
        }
    }

    // ── Remote-share actions — routed through toastHost so spam (5x copy
    //  + 3x download in a row) stacks/queues cleanly instead of overwriting.
    Connections {
        target: remoteShareViewModel
        // Generic status — also fired for non-error info. The VM passes
        // `isError=true` for failures so we can colour-code the toast.
        function onOperationMessage(msg, isError) {
            if (!msg || msg.length === 0) return;
            toastHost.show({
                title:   isError ? qsTr("Lỗi") : msg,
                desc:    isError ? msg : "",
                variant: isError ? "error" : "info"
            });
        }
        function onLinkCopied() {
            toastHost.show({
                title:   qsTr("Đã sao chép link"),
                variant: "success"
            });
        }
        function onDownloadQueued(fileName) {
            toastHost.show({
                title:   qsTr("Đã thêm vào danh sách tải"),
                desc:    fileName || "",
                variant: "success"
            });
        }
    }

    // ════════════════════════════════════════════
    //  TRANSFER HUD — Mini window (P2)
    //  Floating, draggable, snap-to-edges; restored to its saved screen
    //  position on each show().  Driven by hudVM.shouldShowMini and the
    //  onClosing handler above.
    // ════════════════════════════════════════════
    AuroraWindows.MiniHudWindow {
        id: miniHudWindow
        onExpandRequested: {
            // User wants the full app — bring main window back and dismiss
            // the mini so the two don't overlap.
            root.show(); root.raise(); root.requestActivate();
            miniHudWindow.dismissWithFade();
        }
        onCloseRequested: {
            // Explicit ✕ click → suppress the mini until a NEW transfer
            // starts.  Without this flag, hudVM.shouldShowMini would flip
            // true again on the next progress tick.
            if (transferHudViewModel) transferHudViewModel.dismissMini();
            miniHudWindow.dismissWithFade();
        }
        onRowFocused: (taskId) => {
            // Same route as the tray popup: show main + ask hudVM to emit
            // taskFocusRequested.  The Connections block below routes it
            // to the matching Page's focusTask().
            root.show(); root.raise(); root.requestActivate();
            if (transferHudViewModel) transferHudViewModel.focusTask(taskId);
        }
        // ── Footer "+ Dán link" CTA from the Aurora widget ──────────
        // Reads the clipboard and routes it through routePastedText,
        // exactly the same path the Ctrl+V global shortcut takes. The
        // widget can kick off a download without opening the main UI.
        // routePastedText itself validates that the text contains a
        // Fshare URL and surfaces a toast on garbage clipboard contents,
        // so we don't have to defend that here.
        onPasteLinkRequested: {
            if (!loggedIn || !downloadViewModel) return;
            const txt = downloadViewModel.clipboardText();
            if (txt && txt.length > 0) routePastedText(txt);
        }
    }

    // React to hudVM.shouldShowMini transitions.  Trigger the mini's
    // bindToVisibility() helper which encapsulates the 30 s idle-grace
    // window and re-show-on-activity logic.
    Connections {
        target: transferHudViewModel
        function onShouldShowMiniChanged() {
            if (!settingsViewModel || !settingsViewModel.showOnHideToTray) return;
            // Only sync the mini if the main window is currently hidden;
            // we never want the mini popping over the main window.
            if (root.visible) return;
            miniHudWindow.bindToVisibility(transferHudViewModel.shouldShowMini);
        }
    }

    // When the main window comes back forward (tray double-click, expand
    // button, single-instance __show__ message), the mini becomes
    // redundant — dismiss it immediately to avoid two HUD surfaces.
    onVisibleChanged: {
        if (root.visible && miniHudWindow.visible) {
            miniHudWindow.dismissWithFade();
        }
    }

    // ── Minimize (_) → Mini HUD (spec v3) ───────────────────────────────
    // The taskbar minimize button sets visibility=Minimized while visible
    // stays true (so onVisibleChanged above doesn't fire). Catch it here:
    //   • Minimized + có transfer + setting → pop the Mini HUD so progress
    //     stays watchable while the user works in another app.
    //   • Restored (Windowed/Maximized/FullScreen) → dismiss the mini so the
    //     two surfaces never overlap.
    // Note: hide-to-tray (X → "Thu nhỏ vào khay") goes through onClosing +
    // root.hide() instead — that path already pops the mini. This handler
    // covers ONLY the standard minimize button.
    onVisibilityChanged: {
        if (!loggedIn || !settingsViewModel) return;
        if (visibility === Window.Minimized) {
            if (settingsViewModel.showOnHideToTray
                && transferHudViewModel
                && transferHudViewModel.shouldShowMini) {
                transferHudViewModel.acknowledgeMini();
                miniHudWindow.bindToVisibility(true);
            }
        } else if (visibility === Window.Windowed
                   || visibility === Window.Maximized
                   || visibility === Window.FullScreen) {
            // Restored from minimize → close mini.
            if (miniHudWindow.visible) miniHudWindow.dismissWithFade();
        }
    }

    // NOTE: The earlier transient Tray Popup (auto-dismiss on focus-out) was
    // retired — the user wants the tray icon to summon a PERSISTENT floating
    // widget that stays over Word/Excel until explicitly dismissed (Idea A).
    // Both the tray-summon (pinned) and the auto-on-minimize (transient) flows
    // now share the single MiniHudWindow above.

    // main.cpp invokes this via QMetaObject::invokeMethod when the user
    // picks tray menu → "Hiện mini HUD".  Pins the widget (stays put).
    function showMiniHud() {
        if (!loggedIn) {
            // Not logged in — widget is useless. Surface the main window
            // instead so the user can complete login.
            root.show(); root.raise(); root.requestActivate();
            return;
        }
        if (transferHudViewModel) transferHudViewModel.acknowledgeMini();
        miniHudWindow.showPinned();
    }

    // main.cpp invokes this on tray menu → "Cài đặt thông báo".  Switch
    // to the Settings page; main window show is done in the C++ wiring
    // before this fires.
    function navigateToSettings() {
        root.currentPage = Pages.settings;
    }

    // main.cpp invokes this via QMetaObject::invokeMethod on tray single-
    // click.  Toggles the PERSISTENT floating widget (Idea A): show it
    // pinned (stays over other apps) if hidden, or hide it if showing.
    // `trayRect` is unused now (the widget anchors to its saved/default
    // top-right position, not to the tray icon) but kept for signature
    // compatibility with the C++ invokeMethod call.
    function toggleHudWidget(trayRect) {
        if (!loggedIn) {
            root.show(); root.raise(); root.requestActivate();
            return;
        }
        if (transferHudViewModel) transferHudViewModel.acknowledgeMini();
        miniHudWindow.togglePinned();
    }

    // ── HUD VM signal routing ────────────────────────────────────────
    // The HUD VM emits taskFocusRequested(page, taskId) when the user
    // clicks a row in the popup (or, in P2, the mini window).  We switch
    // currentPage and ask the destination Page to scroll its ListView
    // to the row.  The destination Page exposes a focusTask(id) method.
    Connections {
        target: transferHudViewModel
        function onTaskFocusRequested(page, taskId) {
            root.currentPage = page;
            // The Loader for the destination page may not be instantiated
            // yet (lazy load).  Defer one tick so the Loader has a frame
            // to materialise its sourceComponent, then dispatch.
            Qt.callLater(function() {
                _dispatchFocusTask(page, taskId);
            });
        }
    }
    // Internal helper — walks the page Loaders, finds the one matching
    // `page`, and calls focusTask(taskId) on its item if available.
    function _dispatchFocusTask(page, taskId) {
        // Map page enum → corresponding loader item.  Each Page that
        // wants to be focusable exposes focusTask(id); pages without
        // the method are silently skipped.
        const target = _pageItemForIndex(page);
        if (target && typeof target.focusTask === "function") {
            target.focusTask(taskId);
        }
    }
    function _pageItemForIndex(page) {
        // currentPage uses the Pages enum (see qml/FsAurora/Theme/Pages.qml).
        // Only Download + Upload pages currently expose focusTask(id); other
        // pages return null and the dispatch silently no-ops.
        if (page === Pages.download) return downloadLoader.item;
        if (page === Pages.upload)   return uploadLoader.item;
        return null;
    }
}
