// SPDX-License-Identifier: Proprietary
// FileManagerPage — Aurora editorial header + folder tree (left) + file list (right) + toolbar

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Fshare.Components 1.0
import Fshare.Pages.FileManager 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0

Item {
    id: page

    property var selectedFolder: null
    property var selectedFiles: []
    property string searchQuery: ""

    // Context-menu state — the file that was right-clicked
    property var    _ctxFile:       null
    // Rename dialog state
    property string _renameTarget:  ""     // linkcode
    property string _renameOrigName: ""    // current name shown in dialog
    property bool   _renameIsFolder: false

    // Currently selected file's full data (for detail panel, null when 0 or 2+ selected)
    property var selectedFileData: null

    // ── Direct-play state ──────────────────────────────────────────
    // When the user invokes "Xem trực tiếp" on a remote media file we request
    // the stream URL via getStreamLink and remember which linkcode we want to
    // play. The shared streamLinkReady signal is also used for plain
    // "copy-link" invocations, so this pending state lets the handler know
    // whether to launch the media player or just toast the copy.
    property string _pendingStreamLinkcode: ""
    property string _pendingStreamName: ""

    // Media file check — matches videos + audio (everything the media
    // player handles). Folders never qualify.
    function _isMediaFile(file) {
        if (!file || file.isFolder) return false;
        return file.fileCategory === "video" || file.fileCategory === "audio";
    }

    // Launch a media file in the default player:
    //   • already downloaded → open the local file directly
    //   • remote             → fetch the stream URL, then hand it to the
    //                          OS via a temp .m3u8 playlist
    function _playMediaFile(file) {
        if (!file || !fileManagerViewModel) return;
        if (file.isDownloaded && file.localPath) {
            fileManagerViewModel.openLocalFile(file.localPath);
            return;
        }
        page._pendingStreamLinkcode = file.linkcode;
        page._pendingStreamName     = file.name || "";
        fileManagerViewModel.getStreamLink(file.linkcode);
        page.showToast(qsTr("Đang lấy link xem trực tiếp..."), file.name || "", "info");
    }

    // Show a toast notification. Silently swallows empty-title calls —
    // earlier we'd still render the toast card with just the variant icon
    // and close button (an "empty box" flashing on screen) when a signal
    // like operationMessage forwarded an error with no message payload.
    function showToast(title, desc, variant) {
        const t = (title || "").trim();
        const d = (desc  || "").trim();
        if (t.length === 0 && d.length === 0) return;
        pageToast.title   = t;
        pageToast.desc    = d;
        pageToast.variant = variant || "info";
        pageToast.visible = true;
        pageToast.show();
    }

    // Build context-menu actions dynamically based on the clicked file/folder.
    // Call this right before fileMenu.popup() so labels reflect current state.
    function _buildContextActions(file) {
        if (!file) return [];
        return [
            {
                label: qsTr("Rename"),
                icon: "✎",
                onTriggered: function() {
                    page._renameTarget   = file.linkcode;
                    page._renameOrigName = file.name;
                    page._renameIsFolder = file.isFolder || false;
                    renameDialog.open();
                }
            },
            {
                label: qsTr("Copy Link"),
                icon: "⎘",
                onTriggered: function() {
                    if (fileManagerViewModel)
                        fileManagerViewModel.copyLinks([file.linkcode]);
                }
            },
            // "Open containing folder" — only for downloaded/uploaded files
            file.isDownloaded ? {
                label: qsTr("Mở thư mục chứa"),
                icon: "📂",
                onTriggered: function() {
                    if (fileManagerViewModel)
                        fileManagerViewModel.openInExplorer(file.localPath);
                }
            } : null,
            file.isDownloaded ? {
                label: qsTr("Mở file"),
                icon: "▶",
                onTriggered: function() {
                    if (fileManagerViewModel)
                        fileManagerViewModel.openLocalFile(file.localPath);
                }
            } : null,
            { separator: true },
            {
                label: qsTr("Move to..."),
                icon: "↗",
                onTriggered: function() {
                    moveCopyDialog.mode             = "move";
                    moveCopyDialog.pendingLinkcodes = [file.linkcode];
                    moveCopyDialog.open();
                }
            },
            {
                label: qsTr("Copy to..."),
                icon: "⎘",
                onTriggered: function() {
                    moveCopyDialog.mode             = "copy";
                    moveCopyDialog.pendingLinkcodes = [file.linkcode];
                    moveCopyDialog.open();
                }
            },
            { separator: true },
            {
                label: file.secure ? qsTr("Remove Secure") : qsTr("Secure File"),
                icon: file.secure ? "🔓" : "🔒",
                onTriggered: function() {
                    if (fileManagerViewModel)
                        fileManagerViewModel.changeSecure([file.linkcode], !file.secure);
                }
            },
            {
                label: qsTr("Set Password..."),
                icon: "🔑",
                onTriggered: function() {
                    passwordDialog.targetLinkcodes  = [file.linkcode];
                    passwordDialog.hasExisting      = file.hasPassword || false;
                    passwordDialog.open();
                }
            },
            {
                label: file.directlink ? qsTr("Disable Direct Link") : qsTr("Enable Direct Link"),
                icon: "🔗",
                onTriggered: function() {
                    if (fileManagerViewModel)
                        fileManagerViewModel.setDirectLink([file.linkcode], !file.directlink);
                }
            },
            // Folder: open / navigate into
            file.isFolder ? {
                label: qsTr("Mở thư mục"),
                icon: "📂",
                onTriggered: function() {
                    page.selectedFolder = { linkcode: file.linkcode, name: file.name };
                    page.selectedFiles = [];
                    page.selectedFileData = null;
                    if (fileManagerViewModel) fileManagerViewModel.navigateTo(file.linkcode, file.name);
                }
            } : null,
            // Media (video / audio): play in default player.
            // For remote files we fetch the stream URL and hand a temp .m3u8
            // playlist to the OS; for already-downloaded files we open the
            // local file directly. Both paths are handled inside _playMediaFile.
            page._isMediaFile(file) ? {
                label: qsTr("Xem trực tiếp"),
                icon: "▶",
                onTriggered: function() {
                    page._playMediaFile(file);
                }
            } : null,
            // Media-only: also expose a plain "copy stream link" so power users
            // can paste the URL into another player / downloader by hand.
            (page._isMediaFile(file) && !file.isDownloaded) ? {
                label: qsTr("Sao chép link xem trực tiếp"),
                icon: "⎘",
                onTriggered: function() {
                    page._pendingStreamLinkcode = "";
                    page._pendingStreamName     = "";
                    if (fileManagerViewModel)
                        fileManagerViewModel.getStreamLink(file.linkcode);
                }
            } : null,
            { separator: true },
            {
                label: qsTr("Delete"),
                icon: "✕",
                danger: true,
                onTriggered: function() {
                    page._confirmDelete([file.linkcode]);
                }
            }
        ].filter(function(a) { return a !== null; });
    }

    Component.onCompleted: {
        if (fileManagerViewModel) fileManagerViewModel.navigateTo("", "");
    }

    // ── Keyboard shortcuts ───────────────────────────────
    focus: true

    // Sync ListView.currentIndex (keyboard cursor) → page-level selection
    // by reading from the materialised delegate at that index.  positionView
    // first so off-screen rows have a delegate before currentItem reads.
    function _selectByListIndex(idx) {
        if (!fileListView || idx < 0) return;
        const count = fileListView.count;
        if (idx >= count) return;
        fileListView.positionViewAtIndex(idx, ListView.Contain);
        fileListView.currentIndex = idx;
        const it = fileListView.currentItem;
        if (!it) return;
        page.selectedFiles = [it.linkcode];
        page.selectedFolder = it.isFolder
            ? { linkcode: it.linkcode, name: it.name } : null;
        page.selectedFileData = {
            linkcode: it.linkcode, name: it.name, isFolder: it.isFolder,
            size: it.size, extension: it.extension,
            fileCategory: it.fileCategory, secure: it.secure,
            hasPassword: it.hasPassword, directlink: it.directlink,
            isDownloaded: it.isDownloaded, localPath: it.localPath
        };
        if (fileManagerViewModel)
            fileManagerViewModel.setSelected([it.linkcode]);
    }

    Keys.onPressed: function(event) {
        // Arrow keys → move keyboard cursor in the file list.  Wired at
        // page scope so the existing Ctrl+C / Delete / Backspace shortcuts
        // remain in one place (ListView's own keyNavigation would also work
        // but only when ListView itself has focus — page-level keeps the
        // shortcut surface unified).
        if (event.key === Qt.Key_Down) {
            const idx = fileListView.currentIndex < 0 ? 0
                       : Math.min(fileListView.count - 1, fileListView.currentIndex + 1);
            page._selectByListIndex(idx);
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_Up) {
            const idx = Math.max(0, fileListView.currentIndex - 1);
            page._selectByListIndex(idx);
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_Home) {
            page._selectByListIndex(0);
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_End) {
            page._selectByListIndex(fileListView.count - 1);
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_PageDown) {
            page._selectByListIndex(Math.min(fileListView.count - 1,
                fileListView.currentIndex + 10));
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_PageUp) {
            page._selectByListIndex(Math.max(0,
                fileListView.currentIndex - 10));
            event.accepted = true;
            return;
        }
        // Ctrl+C → copy link(s) of selected items
        if (event.key === Qt.Key_C && (event.modifiers & Qt.ControlModifier)) {
            if (page.selectedFiles.length > 0 && fileManagerViewModel) {
                fileManagerViewModel.copyLinks(page.selectedFiles);
                event.accepted = true;
            }
            return;
        }
        // Delete → delete selected items (with confirmation)
        if (event.key === Qt.Key_Delete) {
            if (page.selectedFiles.length > 0) {
                page._confirmDelete(page.selectedFiles.slice());
                event.accepted = true;
            }
            return;
        }
        // Enter → open folder (if single folder selected)
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (page.selectedFileData && page.selectedFileData.isFolder && fileManagerViewModel) {
                fileManagerViewModel.navigateTo(page.selectedFileData.linkcode, page.selectedFileData.name);
                page.selectedFiles = [];
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
        // Backspace → navigate back
        if (event.key === Qt.Key_Backspace) {
            if (fileManagerViewModel && fileManagerViewModel.canGoBack) {
                page.selectedFiles = [];
                page.selectedFileData = null;
                fileManagerViewModel.navigateBack();
                event.accepted = true;
            }
            return;
        }
        // Ctrl+A → select all
        if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
            if (fileManagerViewModel) {
                fileManagerViewModel.selectAll();
                // Sync local selectedFiles with ViewModel
                page.selectedFiles = fileManagerViewModel.selectedLinkcodes;
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
        // Escape → clear selection
        if (event.key === Qt.Key_Escape) {
            if (page.selectedFiles.length > 0) {
                page.selectedFiles = [];
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
        // Menu key / Shift+F10 → open context menu on currently selected
        // file or folder.  Standard Windows / GNOME / KDE keyboard pattern
        // for the right-click equivalent.  Anchor the popup near the
        // selected row so the menu feels positioned, not floating in the
        // void.
        if (event.key === Qt.Key_Menu
            || (event.key === Qt.Key_F10 && (event.modifiers & Qt.ShiftModifier))) {
            if (page.selectedFileData) {
                page._ctxFile = page.selectedFileData;
                fileMenu.actions = page._buildContextActions(page.selectedFileData);
                const it = fileListView.currentItem;
                const anchor = it
                    ? it.mapToItem(page, it.width * 0.5, it.height)
                    : { x: page.width * 0.5, y: page.height * 0.5 };
                fileMenu.popup(anchor.x, anchor.y);
                event.accepted = true;
            }
            return;
        }
    }


    ColumnLayout {
        anchors.fill: parent
        spacing: AuroraTheme.sp4

        // ═══════════════════════════════════════════════════════════════
        // EDITORIAL HEADER — mono kicker + serif wordmark + mini stats
        // ═══════════════════════════════════════════════════════════════
        FileManagerHeader {
            Layout.fillWidth: true
        }

        // ── Toolbar row — search + actions ───────────────
        FileManagerToolbar {
            id: toolbar
            Layout.fillWidth: true
            Layout.topMargin: AuroraTheme.sp3
            searchQuery: page.searchQuery
            onSearchQueryChanged: page.searchQuery = searchQuery
            onNewFolderRequested: newFolderDialog.open()
        }

        // Ctrl+F focuses + selects the search input — matches Explorer /
        // browser / VS Code conventions.  Scoped to the page so the
        // shortcut only fires while File Manager is the active page (Main.qml
        // page router ensures the page Item is hidden when not active).
        Shortcut {
            // Use `sequences` so every platform binding StandardKey.Find
            // expands to (Ctrl+F on Windows / ⌘+F on macOS / etc.) routes
            // through this handler.  Singular `sequence` only binds to
            // the first and Qt 6 warns about the rest.
            sequences: [StandardKey.Find]
            context: Qt.WindowShortcut
            onActivated: toolbar.focusSearch()
        }

        // ── Breadcrumb Navigation ─────────────────────────
        // Pure breadcrumb-driven navigation: no Back / Forward arrows. Users
        // jump anywhere in the path by clicking a segment directly — cleaner
        // and closer to modern cloud-file UX (Google Drive / Dropbox) than
        // the browser-style history buttons the Qt Widgets version had.
        FileManagerBreadcrumb {
            Layout.fillWidth: true
            onNavigateRequested: (folderId, folderName) => {
                page.selectedFolder = folderId.length > 0
                    ? { linkcode: folderId, name: folderName }
                    : null;
                page.selectedFiles = [];
                page.selectedFileData = null;
                if (fileManagerViewModel)
                    fileManagerViewModel.navigateTo(folderId, folderName);
            }
        }

        // Bulk actions have moved into the right-hand preview panel
        // (see detailPanel → "THAO TÁC" section). Removing the old
        // toolbar row keeps the layout from shifting down when files
        // are selected — single/multi actions live in the preview
        // regardless of selection count.

        // ── Main area: list + preview ────────────────────
        Rectangle {
            id: fileManagerContentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            // Horizontal scroller wrapping the [file list | preview] row.
            // The preview panel is always shown at a fixed width — when the
            // window is too narrow to fit both the list's minimum width plus
            // the preview, this Flickable exposes a horizontal scrollbar
            // instead of squeezing the Name column. Vertical flicks pass
            // through to the inner ListView / GridView.
            Flickable {
                id: contentHScroll
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp1
                readonly property int minContentWidth: 320 + 210 + 4 // list min + preview + gap
                contentWidth: Math.max(width, minContentWidth)
                contentHeight: height
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                clip: true
                interactive: contentWidth > width

                ScrollBar.horizontal: ScrollBar {
                    policy: contentHScroll.contentWidth > contentHScroll.width
                            ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                }

            RowLayout {
                width: contentHScroll.contentWidth
                height: contentHScroll.contentHeight
                spacing: 0


                // File list (left, fills remaining space)
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 320 // keeps Name column readable when preview is open

                    // Loading state
                    FsLoadingState {
                        anchors.fill: parent
                        visible: fileManagerViewModel && fileManagerViewModel.isLoading
                        message: qsTr("Loading files...")
                    }

                    // Empty state
                    FsEmptyState {
                        anchors.fill: parent
                        visible: !fileManagerViewModel || (!fileManagerViewModel.isLoading && fileManagerViewModel.totalCount === 0)
                        icon: "📄"
                        title: qsTr("No files in this folder")
                        description: qsTr("Upload files or create a new folder to get started.")
                    }

                    // ── List View (default) ──────────────────────
                    ListView {
                        id: fileListView
                        anchors.fill: parent
                        anchors.margins: AuroraTheme.sp1
                        clip: true
                        visible: fileManagerViewModel && !fileManagerViewModel.isLoading
                                 && fileManagerViewModel.totalCount > 0
                                 && fileManagerViewModel.viewMode === "list"
                        model: fileManagerViewModel ? fileManagerViewModel.fileListModel : null
                        spacing: 0

                        // Header row
                        header: Rectangle {
                            width: fileListView.width
                            height: 32
                            color: AuroraTheme.bg

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: AuroraTheme.sp4
                                anchors.rightMargin: AuroraTheme.sp4
                                spacing: AuroraTheme.sp3

                                Item { Layout.preferredWidth: 24 } // icon space
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Name")
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: 1.4
                                    font.capitalization: Font.AllUppercase
                                    color: AuroraTheme.ink4
                                }
                                Text {
                                    Layout.preferredWidth: 100
                                    text: qsTr("Size")
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: 1.4
                                    font.capitalization: Font.AllUppercase
                                    color: AuroraTheme.ink4
                                    horizontalAlignment: Text.AlignRight
                                }
                                // Modified column removed from the list — the Modified
                                // date is shown inside the preview panel's "Sửa đổi"
                                // field. Keeps the list focused on Name + Size like
                                // Windows Explorer's Details view when the preview pane
                                // is active.
                                Item { Layout.preferredWidth: 80 } // actions — must match delegate
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom; width: parent.width; height: 1
                                color: AuroraTheme.divider
                            }
                        }

                        delegate: Rectangle {
                            id: fileDelegate

                            // Model roles from FileListModel
                            required property string linkcode
                            required property string name
                            required property bool   isFolder
                            required property real   size     // real for files > 2 GB
                            required property bool   secure
                            required property bool   hasPassword
                            required property bool   directlink
                            required property string created
                            required property string modified
                            required property int    downloadCount
                            required property bool   isDownloaded
                            required property string localPath
                            required property string extension
                            required property string fileCategory

                            width: fileListView.width
                            height: 44
                            color: (page.selectedFiles.indexOf(linkcode) >= 0) ? AuroraTheme.accentTint10
                                : (rowMa.containsMouse ? AuroraTheme.bg : "transparent")
                            Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: AuroraTheme.sp4
                                anchors.rightMargin: AuroraTheme.sp4
                                spacing: AuroraTheme.sp3

                                FsFileTypeIcon {
                                    isFolder: fileDelegate.isFolder
                                    fileName: fileDelegate.name
                                    sizePx: 24
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: AuroraTheme.sp2

                                        Text {
                                            Layout.fillWidth: true
                                            text: fileDelegate.name
                                            font.family: AuroraTheme.fontSans
                                            font.pixelSize: 13
                                            font.weight: Font.Medium
                                            color: AuroraTheme.ink1
                                            elide: Text.ElideMiddle
                                        }

                                        // Downloaded indicator
                                        Rectangle {
                                            visible: fileDelegate.isDownloaded
                                            width: 8; height: 8; radius: 4
                                            color: AuroraTheme.success
                                        }

                                        // Indicator badges — only meaningful for files.
                                        // Folders ignore secure/password/direct-link
                                        // flags and the icons clutter the Name column,
                                        // so suppress them for folder rows.
                                        FsIcon {
                                            visible: fileDelegate.secure && !fileDelegate.isFolder
                                            name: "shield"
                                            sizePx: 13
                                            color: AuroraTheme.success
                                        }
                                        FsIcon {
                                            visible: fileDelegate.hasPassword && !fileDelegate.isFolder
                                            name: "key"
                                            sizePx: 13
                                            color: AuroraTheme.warn
                                        }
                                        FsIcon {
                                            visible: fileDelegate.directlink && !fileDelegate.isFolder
                                            name: "link"
                                            sizePx: 13
                                            color: AuroraTheme.accent3
                                        }
                                    }
                                }

                                Text {
                                    Layout.preferredWidth: 100
                                    text: fileDelegate.isFolder ? "—" : FsFormat.bytes(fileDelegate.size)
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11
                                    color: AuroraTheme.ink2
                                    horizontalAlignment: Text.AlignRight
                                }

                                // Row actions — reserve space unconditionally so
                                // unselected rows keep the same column layout as
                                // selected ones. Use opacity+enabled (not
                                // visible) to keep the 80px slot occupied even
                                // when the icons are hidden. Folder rows hide the
                                // action buttons entirely (info/trash/open are
                                // handled from the preview panel for folders).
                                Row {
                                    Layout.preferredWidth: 80
                                    spacing: AuroraTheme.sp1
                                    readonly property bool _active: !fileDelegate.isFolder
                                        && page.selectedFiles.indexOf(fileDelegate.linkcode) >= 0
                                    opacity: _active ? 1 : 0
                                    enabled: _active

                                    // Open in explorer (only for downloaded files)
                                    Rectangle {
                                        visible: fileDelegate.isDownloaded
                                        width: 24; height: 24; radius: AuroraTheme.radiusSm
                                        color: explorerMa.containsMouse ? AuroraTheme.panel : "transparent"
                                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                                        FsIcon { anchors.centerIn: parent; name: "folder"; sizePx: 14; color: AuroraTheme.ink2 }
                                        MouseArea {
                                            id: explorerMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: if (fileManagerViewModel) fileManagerViewModel.openInExplorer(fileDelegate.localPath)
                                        }
                                    }

                                    // Info + Trash icons removed — all actions
                                    // (info details, delete, move, copy, secure,
                                    // password, rename…) now live in the right
                                    // preview panel's THAO TÁC section. This keeps
                                    // the row clean and consistent with the
                                    // preview-driven design.
                                }
                            }

                            MouseArea {
                                id: rowMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onClicked: function(mouse) {
                                    if (mouse.button === Qt.RightButton) {
                                        const fileObj = {
                                            linkcode: fileDelegate.linkcode,
                                            name: fileDelegate.name,
                                            isFolder: fileDelegate.isFolder,
                                            size: fileDelegate.size,
                                            extension: fileDelegate.extension,
                                            fileCategory: fileDelegate.fileCategory,
                                            secure: fileDelegate.secure,
                                            hasPassword: fileDelegate.hasPassword,
                                            directlink: fileDelegate.directlink,
                                            isDownloaded: fileDelegate.isDownloaded,
                                            localPath: fileDelegate.localPath
                                        };
                                        page._ctxFile = fileObj;
                                        fileMenu.actions = page._buildContextActions(fileObj);
                                        const pt = mapToItem(page, mouse.x, mouse.y);
                                        fileMenu.popup(pt.x, pt.y);
                                    } else {
                                        // Single-click selects the row (folder or file). Folders
                                        // are opened only on double-click — same as Windows
                                        // Explorer. Multi-select requires Ctrl (handled
                                        // separately by toggleSelection); a plain click replaces
                                        // the selection with just this row so the action buttons
                                        // show cleanly for the one the user is focusing on.
                                        page.selectedFiles = [fileDelegate.linkcode];
                                        page.selectedFolder = fileDelegate.isFolder
                                            ? { linkcode: fileDelegate.linkcode, name: fileDelegate.name }
                                            : null;
                                        page.selectedFileData = {
                                            linkcode: fileDelegate.linkcode,
                                            name: fileDelegate.name,
                                            isFolder: fileDelegate.isFolder,
                                            size: fileDelegate.size,
                                            extension: fileDelegate.extension,
                                            fileCategory: fileDelegate.fileCategory,
                                            secure: fileDelegate.secure,
                                            hasPassword: fileDelegate.hasPassword,
                                            directlink: fileDelegate.directlink,
                                            downloadCount: fileDelegate.downloadCount,
                                            created: fileDelegate.created,
                                            modified: fileDelegate.modified,
                                            isDownloaded: fileDelegate.isDownloaded,
                                            localPath: fileDelegate.localPath
                                        };
                                    }
                                }
                                onDoubleClicked: {
                                    if (fileDelegate.isFolder) {
                                        page.selectedFolder = { linkcode: fileDelegate.linkcode, name: fileDelegate.name };
                                        page.selectedFiles = [];
                                        if (fileManagerViewModel) fileManagerViewModel.navigateTo(fileDelegate.linkcode, fileDelegate.name);
                                    }
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left; anchors.right: parent.right
                                anchors.bottom: parent.bottom; height: 1
                                color: AuroraTheme.divider
                            }
                        }

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    // ── Medium Icons Grid View ───────────────────
                    GridView {
                        id: mediumGridView
                        anchors.fill: parent
                        anchors.margins: AuroraTheme.sp2
                        clip: true
                        visible: fileManagerViewModel && !fileManagerViewModel.isLoading
                                 && fileManagerViewModel.totalCount > 0
                                 && fileManagerViewModel.viewMode === "medium"
                        model: fileManagerViewModel ? fileManagerViewModel.fileListModel : null
                        cellWidth:  168
                        cellHeight: 178

                        delegate: Item {
                            id: medDelegate
                            width:  mediumGridView.cellWidth
                            height: mediumGridView.cellHeight

                            required property string linkcode
                            required property string name
                            required property string extension
                            required property string fileCategory
                            required property bool   isFolder
                            required property real   size
                            required property bool   secure
                            required property bool   hasPassword
                            required property bool   directlink
                            required property string created
                            required property string modified
                            required property int    downloadCount
                            required property bool   isDownloaded
                            required property string localPath

                            FsFileMediumCard {
                                anchors.centerIn: parent
                                fileName:     medDelegate.name
                                extension:    medDelegate.extension
                                fileCategory: medDelegate.fileCategory
                                isFolder:     medDelegate.isFolder
                                fileSize:     medDelegate.size
                                selected:     page.selectedFiles.indexOf(medDelegate.linkcode) >= 0
                                isDownloaded: medDelegate.isDownloaded
                                localPath:    medDelegate.localPath

                                onClicked: {
                                    // Single-click selects; folders are navigated only on
                                    // double-click (Windows Explorer behaviour).
                                    page.selectedFiles = [medDelegate.linkcode];
                                    page.selectedFolder = medDelegate.isFolder
                                        ? { linkcode: medDelegate.linkcode, name: medDelegate.name }
                                        : null;
                                    page.selectedFileData = {
                                        linkcode: medDelegate.linkcode,
                                        name: medDelegate.name,
                                        isFolder: medDelegate.isFolder,
                                        size: medDelegate.size,
                                        extension: medDelegate.extension,
                                        fileCategory: medDelegate.fileCategory,
                                        secure: medDelegate.secure,
                                        hasPassword: medDelegate.hasPassword,
                                        directlink: medDelegate.directlink,
                                        downloadCount: medDelegate.downloadCount,
                                        created: medDelegate.created,
                                        modified: medDelegate.modified,
                                        isDownloaded: medDelegate.isDownloaded,
                                        localPath: medDelegate.localPath
                                    };
                                }
                                onDoubleClicked: {
                                    if (medDelegate.isFolder) {
                                        page.selectedFolder = { linkcode: medDelegate.linkcode, name: medDelegate.name };
                                        page.selectedFiles = [];
                                        if (fileManagerViewModel) fileManagerViewModel.navigateTo(medDelegate.linkcode, medDelegate.name);
                                    }
                                }
                                onContextMenuRequested: (x, y) => {
                                    const fileObj = {
                                        linkcode: medDelegate.linkcode,
                                        name: medDelegate.name,
                                        isFolder: medDelegate.isFolder,
                                        size: medDelegate.size,
                                        extension: medDelegate.extension,
                                        fileCategory: medDelegate.fileCategory,
                                        secure: medDelegate.secure,
                                        hasPassword: medDelegate.hasPassword,
                                        directlink: medDelegate.directlink,
                                        isDownloaded: medDelegate.isDownloaded,
                                        localPath: medDelegate.localPath
                                    };
                                    page._ctxFile = fileObj;
                                    fileMenu.actions = page._buildContextActions(fileObj);
                                    const pt = medDelegate.mapToItem(page, x, y);
                                    fileMenu.popup(pt.x, pt.y);
                                }
                                onOpenInExplorerClicked: {
                                    if (fileManagerViewModel) fileManagerViewModel.openInExplorer(medDelegate.localPath);
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }
                }

                // ── File Detail Panel (fixed right column, Windows-Explorer preview-pane style) ───
                // Always visible at a fixed width next to the file list. When no
                // file or folder is selected it shows an empty-state hint. Under
                // the outer horizontal Flickable, if the window becomes narrower
                // than (list min + panel width) the whole area scrolls sideways
                // instead of the panel shrinking — the panel width never changes.
                Rectangle {
                    id: detailPanel
                    readonly property int  selCount: page.selectedFiles.length
                    readonly property bool active:   selCount > 0
                    readonly property bool isSingle: selCount === 1 && page.selectedFileData !== null
                    readonly property bool isMulti:  selCount >= 2
                    readonly property int panelWidth: 210 // ≈ 2/3 of 320

                    // Count how many of the current selection are folders vs files,
                    // using FileListModel::getItemAsVariant (Q_INVOKABLE) to look up
                    // each linkcode. Called from the multi-select summary label.
                    function _selectionBreakdown() {
                        const codes = page.selectedFiles;
                        const out = { folders: 0, files: 0 };
                        if (!fileManagerViewModel || !codes || codes.length === 0)
                            return out;
                        const model = fileManagerViewModel.fileListModel;
                        if (!model || typeof model.getItemAsVariant !== "function")
                            return out;
                        for (let i = 0; i < codes.length; ++i) {
                            const obj = model.getItemAsVariant(codes[i]);
                            if (obj && obj.isFolder) ++out.folders; else ++out.files;
                        }
                        return out;
                    }

                    Layout.preferredWidth: panelWidth
                    Layout.fillHeight: true
                    Layout.leftMargin: AuroraTheme.sp1
                    clip: true

                    color: AuroraTheme.panel
                    radius: AuroraTheme.radiusMd
                    border.width: 1
                    border.color: AuroraTheme.border

                    // Empty state — shown when nothing is selected
                    ColumnLayout {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -20
                        width: parent.width - AuroraTheme.sp6 * 2
                        visible: !detailPanel.active
                        spacing: AuroraTheme.sp2

                        FsIcon {
                            Layout.alignment: Qt.AlignHCenter
                            name: "info"; sizePx: 28
                            color: AuroraTheme.ink3
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Chọn file hoặc thư mục\nđể xem thông tin")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 11
                            color: AuroraTheme.ink3
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }
                    }

                    Flickable {
                        anchors.fill: parent
                        anchors.leftMargin: 1
                        contentHeight: detailCol.implicitHeight + AuroraTheme.sp4 * 2
                        clip: true
                        visible: detailPanel.active
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        ColumnLayout {
                            id: detailCol
                            width: parent.width
                            anchors.top: parent.top
                            anchors.topMargin: AuroraTheme.sp3
                            spacing: 0

                            // ── Header: "Thông tin" + Close ──
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.rightMargin: AuroraTheme.sp2

                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Thông tin")
                                    font.family: AuroraTheme.fontSerif
                                    font.pixelSize: 22
                                    font.weight: Font.Normal
                                    color: AuroraTheme.ink1
                                }
                                Rectangle {
                                    width: 28; height: 28; radius: AuroraTheme.radiusSm
                                    color: closePanelMa.containsMouse ? AuroraTheme.bg : "transparent"
                                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                                    FsIcon { anchors.centerIn: parent; name: "x"; sizePx: 14; color: AuroraTheme.ink2 }
                                    MouseArea {
                                        id: closePanelMa; anchors.fill: parent; hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            page.selectedFiles = [];
                                            page.selectedFileData = null;
                                        }
                                    }
                                }
                            }

                            // ── Multi-select summary (shown only when 2+ items selected) ──
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 100
                                Layout.topMargin: AuroraTheme.sp2
                                visible: detailPanel.isMulti

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 72; height: 72
                                    radius: 36 // circle
                                    color: Qt.rgba(AuroraTheme.accent3.r, AuroraTheme.accent3.g, AuroraTheme.accent3.b, 0.14)
                                    border.width: 1; border.color: AuroraTheme.accent3

                                    Text {
                                        anchors.centerIn: parent
                                        text: detailPanel.selCount > 99 ? "99+" : String(detailPanel.selCount)
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 22
                                        font.weight: Font.DemiBold
                                        color: AuroraTheme.accent3
                                    }
                                }
                            }

                            // Multi: "N mục đã chọn"
                            Text {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.rightMargin: AuroraTheme.sp4
                                visible: detailPanel.isMulti
                                text: qsTr("%1 mục đã chọn").arg(detailPanel.selCount)
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                                horizontalAlignment: Text.AlignHCenter
                            }

                            // Multi: breakdown (folders + files)
                            Text {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.rightMargin: AuroraTheme.sp4
                                Layout.topMargin: 2
                                visible: detailPanel.isMulti
                                text: {
                                    if (!detailPanel.isMulti) return "";
                                    const b = detailPanel._selectionBreakdown();
                                    const parts = [];
                                    if (b.folders > 0) parts.push(qsTr("%1 thư mục").arg(b.folders));
                                    if (b.files > 0)   parts.push(qsTr("%1 tệp").arg(b.files));
                                    return parts.join(" · ");
                                }
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 11
                                color: AuroraTheme.ink3
                                horizontalAlignment: Text.AlignHCenter
                            }

                            // ── File icon + name (single selection) ──
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 100
                                Layout.topMargin: AuroraTheme.sp2
                                visible: detailPanel.isSingle

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 72; height: 72
                                    radius: AuroraTheme.radiusLg
                                    color: AuroraTheme.bg
                                    border.width: 1; border.color: AuroraTheme.border

                                    FsFileTypeIcon {
                                        anchors.centerIn: parent
                                        isFolder: page.selectedFileData && page.selectedFileData.isFolder
                                        fileName: page.selectedFileData ? page.selectedFileData.name : ""
                                        sizePx: 36
                                    }
                                }
                            }

                            // File name
                            Text {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.rightMargin: AuroraTheme.sp4
                                visible: detailPanel.isSingle
                                text: page.selectedFileData ? page.selectedFileData.name : ""
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                horizontalAlignment: Text.AlignHCenter
                                maximumLineCount: 3
                                elide: Text.ElideMiddle
                            }

                            // File type label
                            Text {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.rightMargin: AuroraTheme.sp4
                                Layout.topMargin: 2
                                visible: detailPanel.isSingle
                                text: page.selectedFileData
                                    ? (page.selectedFileData.isFolder
                                        ? qsTr("Thư mục")
                                        : (page.selectedFileData.extension || "").toUpperCase() + " file")
                                    : ""
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 11
                                color: AuroraTheme.ink3
                                horizontalAlignment: Text.AlignHCenter
                            }

                            // ── Divider ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.topMargin: AuroraTheme.sp3; Layout.bottomMargin: AuroraTheme.sp2
                                height: 1; color: AuroraTheme.divider
                            }

                            // ── Primary action (single selection only): Open folder / Play media ──
                            // Kept separate from the bulk "THAO TÁC" grid so it reads as
                            // the hero action when one item is selected. Shown for folders
                            // and for any media file (video/audio) — whether the file is
                            // already downloaded or still remote. `_playMediaFile` picks
                            // the right path (local open vs stream-URL → .m3u8 playlist).
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle && page.selectedFileData
                                         && (page.selectedFileData.isFolder
                                             || page._isMediaFile(page.selectedFileData))
                                spacing: AuroraTheme.sp1

                                // Folder: Open
                                FsButton {
                                    Layout.fillWidth: true
                                    visible: page.selectedFileData && page.selectedFileData.isFolder
                                    text: qsTr("Mở thư mục"); variant: "primary"; size: "sm"
                                    onClicked: {
                                        const f = page.selectedFileData;
                                        page.selectedFolder = { linkcode: f.linkcode, name: f.name };
                                        page.selectedFiles = [];
                                        page.selectedFileData = null;
                                        if (fileManagerViewModel)
                                            fileManagerViewModel.navigateTo(f.linkcode, f.name);
                                    }
                                }

                                // Media (video / audio): Play.
                                // "Phát" when the file is already downloaded (local open),
                                // "Xem trực tiếp" when it's still remote (stream via player).
                                FsButton {
                                    Layout.fillWidth: true
                                    visible: page.selectedFileData
                                             && page._isMediaFile(page.selectedFileData)
                                    text: (page.selectedFileData && page.selectedFileData.isDownloaded)
                                          ? qsTr("Phát") : qsTr("Xem trực tiếp")
                                    variant: "primary"; size: "sm"
                                    onClicked: {
                                        if (page.selectedFileData)
                                            page._playMediaFile(page.selectedFileData);
                                    }
                                }
                            }

                            // ── THAO TÁC section label ──
                            // Single source of truth for bulk actions. Always visible
                            // when a selection exists — works for 1 or many items.
                            Text {
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.active
                                text: qsTr("THAO TÁC")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.ink4
                            }

                            // Row 1: [Di chuyển] [Sao chép]
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                visible: detailPanel.active
                                spacing: AuroraTheme.sp1

                                FsButton {
                                    Layout.fillWidth: true
                                    text: qsTr("Di chuyển"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        moveCopyDialog.mode             = "move";
                                        moveCopyDialog.pendingLinkcodes = page.selectedFiles.slice();
                                        moveCopyDialog.open();
                                    }
                                }
                                FsButton {
                                    Layout.fillWidth: true
                                    text: qsTr("Sao chép"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        moveCopyDialog.mode             = "copy";
                                        moveCopyDialog.pendingLinkcodes = page.selectedFiles.slice();
                                        moveCopyDialog.open();
                                    }
                                }
                            }

                            // Row 2: [Sao chép link] full-width
                            FsButton {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: AuroraTheme.sp1
                                visible: detailPanel.active
                                text: qsTr("Sao chép link"); variant: "ghost"; size: "sm"
                                onClicked: {
                                    if (fileManagerViewModel && page.selectedFiles.length > 0)
                                        fileManagerViewModel.copyLinks(page.selectedFiles);
                                }
                            }

                            // Row 3: [Đặt mật khẩu] full-width
                            FsButton {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: AuroraTheme.sp1
                                visible: detailPanel.active
                                text: qsTr("Đặt mật khẩu"); variant: "ghost"; size: "sm"
                                onClicked: {
                                    passwordDialog.targetLinkcodes = page.selectedFiles.slice();
                                    // For single selection with existing password we can
                                    // pre-fill the "change" mode; otherwise treat as new.
                                    passwordDialog.hasExisting = detailPanel.isSingle
                                        && page.selectedFileData
                                        && page.selectedFileData.hasPassword;
                                    passwordDialog.open();
                                }
                            }

                            // Row 4 (single): [Bật/Tắt bảo mật] based on current state
                            FsButton {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle && page.selectedFileData
                                text: (page.selectedFileData && page.selectedFileData.secure)
                                    ? qsTr("Tắt bảo mật") : qsTr("Bật bảo mật")
                                variant: "ghost"; size: "sm"
                                onClicked: {
                                    if (fileManagerViewModel && page.selectedFileData)
                                        fileManagerViewModel.changeSecure(
                                            [page.selectedFileData.linkcode],
                                            !page.selectedFileData.secure);
                                }
                            }

                            // Row 4 (multi): [Bật] [Tắt] split because items may be mixed
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: AuroraTheme.sp1
                                visible: detailPanel.isMulti
                                spacing: AuroraTheme.sp1

                                FsButton {
                                    Layout.fillWidth: true
                                    text: qsTr("Bật bảo mật"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (fileManagerViewModel)
                                            fileManagerViewModel.changeSecure(page.selectedFiles, true);
                                    }
                                }
                                FsButton {
                                    Layout.fillWidth: true
                                    text: qsTr("Tắt bảo mật"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (fileManagerViewModel)
                                            fileManagerViewModel.changeSecure(page.selectedFiles, false);
                                    }
                                }
                            }

                            // ── Quick actions (single selection: copy link + share shortcuts) ──
                            // Kept around as a convenience row — Copy link here is a
                            // duplicate of the THAO TÁC one, but it gives the single-
                            // selection view a secondary action near the file name.
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: AuroraTheme.sp2
                                visible: detailPanel.isSingle && page.selectedFileData
                                         && !page.selectedFileData.isFolder
                                spacing: AuroraTheme.sp1

                                // Share (same as copy link for Fshare)
                                FsButton {
                                    Layout.fillWidth: true
                                    text: qsTr("Chia sẻ"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (fileManagerViewModel && page.selectedFileData)
                                            fileManagerViewModel.copyLinks([page.selectedFileData.linkcode]);
                                        page.showToast(qsTr("Link đã được sao chép — gửi cho người nhận"), "", "success");
                                    }
                                }
                            }

                            // ── Media extra: copy stream link ──
                            // The hero button above handles "play now". This ghost
                            // button is the fallback for users who want to paste the
                            // URL into a different player / downloader by hand.
                            // Shown only when the file is still remote — for
                            // downloaded items the local path is already available.
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle && page.selectedFileData
                                         && page._isMediaFile(page.selectedFileData)
                                         && !page.selectedFileData.isDownloaded
                                spacing: AuroraTheme.sp1

                                FsButton {
                                    Layout.fillWidth: true
                                    text: qsTr("Sao chép link xem trực tiếp")
                                    variant: "ghost"; size: "sm"
                                    onClicked: {
                                        page._pendingStreamLinkcode = "";
                                        page._pendingStreamName     = "";
                                        if (fileManagerViewModel && page.selectedFileData)
                                            fileManagerViewModel.getStreamLink(page.selectedFileData.linkcode);
                                    }
                                }
                            }

                            // ── Divider ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.topMargin: AuroraTheme.sp2; Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle
                                height: 1; color: AuroraTheme.divider
                            }

                            // ── Metadata section ──
                            Text {
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle
                                text: qsTr("CHI TIẾT")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.ink4
                            }

                            // Size
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 26
                                visible: detailPanel.isSingle
                                Text {
                                    Layout.preferredWidth: 80
                                    text: qsTr("Kích thước"); color: AuroraTheme.ink3
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: page.selectedFileData
                                        ? (page.selectedFileData.isFolder ? "—" : FsFormat.bytes(page.selectedFileData.size))
                                        : "—"
                                    color: AuroraTheme.ink2
                                    font.family: AuroraTheme.fontMono; font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            // Created
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 26
                                visible: detailPanel.isSingle
                                Text {
                                    Layout.preferredWidth: 80
                                    text: qsTr("Ngày tạo"); color: AuroraTheme.ink3
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: page.selectedFileData ? FsFormat.dateTime(page.selectedFileData.created) : "—"
                                    color: AuroraTheme.ink2
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            // Modified
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 26
                                visible: detailPanel.isSingle
                                Text {
                                    Layout.preferredWidth: 80
                                    text: qsTr("Sửa đổi"); color: AuroraTheme.ink3
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: page.selectedFileData ? FsFormat.dateTime(page.selectedFileData.modified) : "—"
                                    color: AuroraTheme.ink2
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            // Downloads
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 26
                                visible: detailPanel.isSingle && page.selectedFileData && !page.selectedFileData.isFolder
                                Text {
                                    Layout.preferredWidth: 80
                                    text: qsTr("Lượt tải"); color: AuroraTheme.ink3
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: (page.selectedFileData && page.selectedFileData.downloadCount !== undefined)
                                        ? String(page.selectedFileData.downloadCount) : "—"
                                    color: AuroraTheme.ink2
                                    font.family: AuroraTheme.fontMono; font.pixelSize: 11
                                }
                            }

                            // ── Divider ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.topMargin: AuroraTheme.sp2; Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle
                                height: 1; color: AuroraTheme.divider
                            }

                            // ── Security status (single selection: read-only state chips) ──
                            // Toggles live in THAO TÁC above; this block is purely
                            // informational so the user can see current state at a glance.
                            Text {
                                Layout.leftMargin: AuroraTheme.sp4
                                Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle
                                text: qsTr("BẢO MẬT")
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                color: AuroraTheme.ink4
                            }

                            // Secure status row
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 26
                                visible: detailPanel.isSingle

                                FsIcon {
                                    name: "shield"; sizePx: 14
                                    color: (page.selectedFileData && page.selectedFileData.secure)
                                        ? AuroraTheme.success : AuroraTheme.ink3
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: (page.selectedFileData && page.selectedFileData.secure)
                                        ? qsTr("Secure: Bật") : qsTr("Secure: Tắt")
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                    color: AuroraTheme.ink2
                                }
                            }

                            // Password status row
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 26
                                visible: detailPanel.isSingle

                                FsIcon {
                                    name: "key"; sizePx: 14
                                    color: (page.selectedFileData && page.selectedFileData.hasPassword)
                                        ? AuroraTheme.warn : AuroraTheme.ink3
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: (page.selectedFileData && page.selectedFileData.hasPassword)
                                        ? qsTr("Mật khẩu: Có") : qsTr("Mật khẩu: Không")
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                    color: AuroraTheme.ink2
                                }
                            }

                            // Direct link row (status + toggle — kept interactive since
                            // it's not part of the bulk actions)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.preferredHeight: 32
                                visible: detailPanel.isSingle

                                FsIcon {
                                    name: "link"; sizePx: 14
                                    color: (page.selectedFileData && page.selectedFileData.directlink)
                                        ? AuroraTheme.accent3 : AuroraTheme.ink3
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: (page.selectedFileData && page.selectedFileData.directlink)
                                        ? qsTr("Direct link: Bật") : qsTr("Direct link: Tắt")
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 11
                                    color: AuroraTheme.ink2
                                }
                                FsButton {
                                    text: (page.selectedFileData && page.selectedFileData.directlink) ? qsTr("Tắt") : qsTr("Bật")
                                    variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (fileManagerViewModel && page.selectedFileData)
                                            fileManagerViewModel.setDirectLink(
                                                [page.selectedFileData.linkcode],
                                                !page.selectedFileData.directlink);
                                    }
                                }
                            }

                            // ── Divider ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                Layout.topMargin: AuroraTheme.sp2; Layout.bottomMargin: AuroraTheme.sp1
                                visible: detailPanel.isSingle
                                height: 1; color: AuroraTheme.divider
                            }

                            // ── Danger actions ──
                            // Xoá is always visible (works for 1 or many); the others
                            // are single-item operations so they hide in multi mode.
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: AuroraTheme.sp3
                                Layout.rightMargin: AuroraTheme.sp3
                                Layout.topMargin: detailPanel.isMulti ? AuroraTheme.sp2 : 0
                                Layout.bottomMargin: AuroraTheme.sp4
                                spacing: AuroraTheme.sp1

                                FsButton {
                                    Layout.fillWidth: true
                                    visible: detailPanel.isSingle
                                    text: qsTr("Đổi tên"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (page.selectedFileData) {
                                            page._renameTarget = page.selectedFileData.linkcode;
                                            page._renameOrigName = page.selectedFileData.name;
                                            page._renameIsFolder = page.selectedFileData.isFolder;
                                            renameDialog.open();
                                        }
                                    }
                                }

                                // Downloaded file actions (single only)
                                FsButton {
                                    Layout.fillWidth: true
                                    visible: detailPanel.isSingle && page.selectedFileData && page.selectedFileData.isDownloaded
                                    text: qsTr("Mở file"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (fileManagerViewModel && page.selectedFileData)
                                            fileManagerViewModel.openLocalFile(page.selectedFileData.localPath);
                                    }
                                }
                                FsButton {
                                    Layout.fillWidth: true
                                    visible: detailPanel.isSingle && page.selectedFileData && page.selectedFileData.isDownloaded
                                    text: qsTr("Mở thư mục chứa"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (fileManagerViewModel && page.selectedFileData)
                                            fileManagerViewModel.openInExplorer(page.selectedFileData.localPath);
                                    }
                                }

                                // Xoá — works for both single and multi. Falls back to the
                                // full selectedFiles array so a multi-select delete is one click.
                                FsButton {
                                    Layout.fillWidth: true
                                    visible: detailPanel.active
                                    text: detailPanel.isMulti
                                        ? qsTr("Xoá %1 mục").arg(detailPanel.selCount)
                                        : qsTr("Xoá")
                                    variant: "danger"; size: "sm"
                                    onClicked: {
                                        if (page.selectedFiles && page.selectedFiles.length > 0)
                                            page._confirmDelete(page.selectedFiles.slice());
                                    }
                                }
                            }
                        }
                    }
                }
            } // RowLayout
            } // contentHScroll Flickable
        }
    }

    // ── Context Menu (right-click on file rows) ────────
    // Must be at page level (anchors.fill: parent) for backdrop to work.
    FsContextMenu {
        id: fileMenu
        anchors.fill: parent
        menuWidth: 230
        actions: []   // populated dynamically in _buildContextActions() before each popup()
    }

    // ── Rename Dialog ──────────────────────────────────
    FileRenameDialog {
        id: renameDialog
        targetLinkcode: page._renameTarget
        originalName:   page._renameOrigName
        isFolder:       page._renameIsFolder
        onConfirmed: (newName) => {
            if (fileManagerViewModel)
                fileManagerViewModel.renameFile(page._renameTarget, newName);
        }
    }

    // ── Toast notification (copy link, op feedback) ────
    FsToast {
        id: pageToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: AuroraTheme.sp4
        z: 1001
        visible: false
        onClosed: visible = false
    }

    // ── ViewModel signal connections ───────────────────
    Connections {
        target: fileManagerViewModel
        function onLinksCopied(count) {
            page.showToast(
                count === 1 ? qsTr("Link copied to clipboard")
                            : qsTr("%1 links copied to clipboard").arg(count),
                "", "success");
        }
        function onOperationMessage(msg, isError) {
            page.showToast(msg, "", isError ? "error" : "success");
            // Refresh the detail panel data after a successful operation
            // so security toggles (secure, password, directlink) update immediately
            if (!isError && page.selectedFileData && fileManagerViewModel) {
                const fresh = fileManagerViewModel.fileListModel.getItemAsVariant(
                    page.selectedFileData.linkcode);
                if (fresh && fresh.linkcode)
                    page.selectedFileData = fresh;
            }
        }
        function onStreamLinkReady(linkcode, url) {
            // Two intents share this signal:
            //   1. "Xem trực tiếp" — launch the default media player via .m3u8
            //   2. "Sao chép link xem trực tiếp" — the VM already put the URL
            //      on the clipboard; we just toast.
            // _pendingStreamLinkcode distinguishes them: it's set only by the
            // play path (_playMediaFile).
            if (page._pendingStreamLinkcode === linkcode) {
                const hintName = page._pendingStreamName;
                page._pendingStreamLinkcode = "";
                page._pendingStreamName     = "";
                if (fileManagerViewModel)
                    fileManagerViewModel.playStreamUrl(url, hintName);
                page.showToast(qsTr("Đang phát"),
                               hintName || qsTr("Trình phát mặc định đã khởi chạy"),
                               "success");
            } else {
                page.showToast(qsTr("Stream link đã được sao chép"), url, "success");
            }
        }
        function onStreamLinkError(message) {
            page._pendingStreamLinkcode = "";
            page._pendingStreamName     = "";
            page.showToast(qsTr("Không thể lấy stream link"), message, "error");
        }
    }

    // ── Move / Copy Dialog — Fshare folder tree picker ──
    // Shows the Fshare folder tree (from folderTree) so the user can pick a
    // destination folder. Works for both Move and Copy operations.
    FileMoveCopyDialog {
        id: moveCopyDialog
        folderTreeModel: fileManagerViewModel ? fileManagerViewModel.folderTreeModel : null
        onConfirmed: (mode, linkcodes, destId) => {
            if (!fileManagerViewModel) return;
            if (mode === "move")
                fileManagerViewModel.moveFiles(linkcodes, destId);
            else
                fileManagerViewModel.copyFiles(linkcodes, destId);
            page.selectedFiles = [];
        }
    }

    // ── Set / Remove Password Dialog ───────────────────
    FilePasswordDialog {
        id: passwordDialog
        onConfirmed: (linkcodes, pwd) => {
            if (fileManagerViewModel)
                fileManagerViewModel.setPassword(linkcodes, pwd);
        }
    }

    // ── Delete Confirmation Dialog ────────────────────
    FileDeleteDialog {
        id: deleteConfirmDialog
        onConfirmed: (linkcodes) => {
            if (fileManagerViewModel)
                fileManagerViewModel.deleteFiles(linkcodes);
            page.selectedFiles = [];
            page.selectedFileData = null;
        }
    }

    // Helper: open delete confirmation with the given linkcodes
    function _confirmDelete(linkcodes) {
        if (!linkcodes || linkcodes.length === 0) return;
        deleteConfirmDialog.pendingLinkcodes = linkcodes;
        // Check if any selected item is a folder
        let hasFolder = false;
        if (fileManagerViewModel) {
            for (let i = 0; i < linkcodes.length; i++) {
                const item = fileManagerViewModel.fileListModel.getItemAsVariant(linkcodes[i]);
                if (item && item.isFolder) { hasFolder = true; break; }
            }
        }
        deleteConfirmDialog.hasFolder = hasFolder;
        deleteConfirmDialog.open();
    }

    // ── New Folder Dialog ──────────────────────────────
    //
    // The old dialog silently used `page.selectedFolder?.linkcode || ""` as
    // the parent, which was wrong on two counts:
    //   1. `selectedFolder` is the list-HIGHLIGHT, not the current browsing
    //      location — confusing and surprising to users.
    //   2. An empty parent id hits the Fshare API at root; the server then
    //      rejects with a misleading "không cho phép các ký tự đặc biệt"
    //      error (the legacy megatool normalises empty → "0" to avoid this).
    //
    // The redesigned dialog:
    //   • Shows the destination explicitly ("Tạo trong: <folder>") so users
    //     never guess where the folder lands.
    //   • Offers a one-tap toggle to create inside the HIGHLIGHTED folder
    //     when one is selected, only when that's different from the current
    //     location.
    //   • Validates the name client-side (forbidden chars, reserved names,
    //     duplicates in the destination) with inline error feedback, so the
    //     server's confusing message never reaches the user.
    //   • Submits on Enter in addition to the Create button.
    FsDialog {
        id: newFolderDialog
        title: qsTr("Tạo thư mục mới")
        dialogWidth: 460

        // ── State ────────────────────────────────────────────────
        property string folderName: ""
        property string validationError: ""
        // When true (and a folder is selected), create inside the selected
        // folder rather than the current browsing folder.
        property bool createInsideSelected: false

        // ── Destination resolution ───────────────────────────────
        readonly property bool _hasSelectedFolder:
            page.selectedFolder !== null && page.selectedFolder !== undefined
        readonly property string _currentFolderId:
            fileManagerViewModel ? fileManagerViewModel.currentFolderId : ""
        readonly property string _currentFolderName: {
            if (!fileManagerViewModel) return qsTr("Thư mục gốc");
            const bc = fileManagerViewModel.breadcrumbs;
            return (bc && bc.length > 0) ? bc[bc.length - 1].name
                                         : qsTr("Thư mục gốc");
        }
        // Fshare API quirk: the root folder id must be "0" — passing empty
        // string triggers a misleading "invalid characters" error from the
        // server. Legacy megatool handled this the same way.
        readonly property string _targetParentId: {
            if (createInsideSelected && _hasSelectedFolder)
                return page.selectedFolder.linkcode;
            const cur = _currentFolderId;
            return cur === "" ? "0" : cur;
        }
        readonly property string _targetParentName: {
            if (createInsideSelected && _hasSelectedFolder)
                return page.selectedFolder.name;
            return _currentFolderName;
        }

        // ── Validation ───────────────────────────────────────────
        // Returns true when valid; sets `validationError` otherwise.
        function _validate() {
            const raw  = folderName;
            const name = raw.trim();
            if (name === "") {
                validationError = qsTr("Tên thư mục không được để trống");
                return false;
            }
            if (name.length > 255) {
                validationError = qsTr("Tên thư mục quá dài (tối đa 255 ký tự)");
                return false;
            }
            // Windows-forbidden + Fshare-rejected characters. We allow
            // a superset of what the server accepts so the user only sees
            // one rejection at a time.
            const bad = /[<>:"/\\|?*\u0000-\u001f]/;
            if (bad.test(name)) {
                validationError = qsTr("Không dùng các ký tự: < > : \" / \\ | ? *");
                return false;
            }
            if (name.startsWith(".") || name.endsWith(".")) {
                validationError = qsTr("Tên không được bắt đầu hoặc kết thúc bằng dấu chấm");
                return false;
            }
            // Windows reserved device names — Fshare sync would later fail
            // if the folder is downloaded locally.
            const reserved = /^(con|prn|aux|nul|com[1-9]|lpt[1-9])$/i;
            if (reserved.test(name)) {
                validationError = qsTr("Tên này là tên dành riêng của hệ thống");
                return false;
            }
            // Duplicate check against what's currently visible in the
            // destination. Only meaningful when creating in the current
            // folder (we can't see inside the selected-but-not-entered one).
            if (!createInsideSelected && fileManagerViewModel
                && fileManagerViewModel.fileListModel
                && fileManagerViewModel.fileListModel.hasFolderNamed(name)) {
                validationError = qsTr("Đã tồn tại thư mục cùng tên ở đây");
                return false;
            }
            validationError = "";
            return true;
        }

        function _submit() {
            if (!_validate() || !fileManagerViewModel) return;
            const name = folderName.trim();
            fileManagerViewModel.createFolder(name, _targetParentId);
            // Success / failure toast arrives via the operationMessage
            // connection below, so we close straight away.
            folderName = "";
            validationError = "";
            createInsideSelected = false;
            newFolderDialog.close();
        }

        onOpened: {
            folderName = "";
            validationError = "";
            createInsideSelected = false;
            // Focus the input after the enter animation settles.
            nameFieldFocus.restart();
        }

        Timer {
            id: nameFieldFocus
            interval: 120
            onTriggered: if (nameField.input) nameField.input.forceActiveFocus()
        }

        content: Item {
            width: 460
            implicitHeight: contentCol.implicitHeight + 2 * AuroraTheme.sp5
            // Item has no intrinsic height binding, so expose one manually
            // so FsDialog's childrenRect calculation grows with content.
            height: implicitHeight

            ColumnLayout {
                id: contentCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                anchors.topMargin: AuroraTheme.sp5
                spacing: AuroraTheme.sp3

                // ── Destination card ────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    radius: AuroraTheme.radiusSm
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border
                    implicitHeight: destCol.implicitHeight + 2 * AuroraTheme.sp3

                    ColumnLayout {
                        id: destCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: AuroraTheme.sp3
                        anchors.rightMargin: AuroraTheme.sp3
                        spacing: AuroraTheme.sp1

                        Text {
                            text: qsTr("TẠO TRONG")
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.4
                            font.capitalization: Font.AllUppercase
                            color: AuroraTheme.ink4
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AuroraTheme.sp2
                            FsIcon {
                                name: "folder"
                                sizePx: 16
                                color: AuroraTheme.accent
                            }
                            Text {
                                Layout.fillWidth: true
                                text: newFolderDialog._targetParentName
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                                elide: Text.ElideMiddle
                            }
                        }
                    }
                }

                // ── "Inside selected folder" toggle ─────────────
                // Only offered when a folder is highlighted AND is not the
                // folder we're currently browsing (otherwise the toggle
                // would be a no-op).
                FsCheckbox {
                    Layout.fillWidth: true
                    visible: newFolderDialog._hasSelectedFolder
                             && page.selectedFolder
                             && page.selectedFolder.linkcode !== newFolderDialog._currentFolderId
                    checked: newFolderDialog.createInsideSelected
                    label: page.selectedFolder
                        ? qsTr("Tạo bên trong thư mục đã chọn \"%1\"").arg(page.selectedFolder.name)
                        : ""
                    onToggled: function(c) {
                        newFolderDialog.createInsideSelected = c;
                        // Re-validate if an error is showing (duplicate check
                        // depends on the destination).
                        if (newFolderDialog.validationError !== "")
                            newFolderDialog._validate();
                    }
                }

                // ── Name input ──────────────────────────────────
                FsTextField {
                    id: nameField
                    Layout.fillWidth: true
                    label: qsTr("Tên thư mục")
                    placeholder: qsTr("VD: Tài liệu 2026")
                    text: newFolderDialog.folderName
                    hint: qsTr("Tránh dùng các ký tự < > : \" / \\ | ? *")
                    error: newFolderDialog.validationError
                    onTextChanged: {
                        newFolderDialog.folderName = text;
                        // Clear stale error as the user types; full
                        // validation runs on submit.
                        if (newFolderDialog.validationError !== "")
                            newFolderDialog.validationError = "";
                    }
                    onAccepted: newFolderDialog._submit()
                }
            }
        }

        footer: Item {
            width: 460
            height: 64
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                spacing: AuroraTheme.sp2
                Item { Layout.fillWidth: true }
                FsButton {
                    text: qsTr("Hủy"); variant: "ghost"
                    onClicked: newFolderDialog.close()
                }
                FsButton {
                    text: qsTr("Tạo thư mục"); variant: "primary"
                    enabled: newFolderDialog.folderName.trim().length > 0
                    onClicked: newFolderDialog._submit()
                }
            }
        }
    }
}
              