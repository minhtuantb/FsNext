// SPDX-License-Identifier: Proprietary
// FavoritesPage (Aurora) — editorial header + list/preview layout.
//
// Handoff reference: `aurora-screens.jsx → AuroraFavorites`. Keeps the
// existing list-plus-preview layout (since the VM is shape-compatible with
// My Files), re-skinned with Aurora tokens + editorial header. Grid view
// from the handoff is out of scope — our VM streams into a list model.
//
// All VM wiring preserved:
//   favoritesViewModel.extFilter, loadFavorites, isInFolder, fileListModel,
//   totalCount, isLoading, breadcrumbs, navigateBack/To*, selectedLinkcodes,
//   selectAll, clearSelection, toggleSelection, copyLinks,
//   removeFromFavorite, deleteFiles, openLocalFile, openInExplorer,
//   getStreamLink, playStreamUrl; signals onLinksCopied, onOperationMessage,
//   onStreamLinkReady, onStreamLinkError.

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Fshare.Components 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: page

    // ── Selection state ─────────────────────────────────────
    property var    selectedFiles: []
    property var    selectedFileData: null
    property var    _ctxFile: null

    property string _pendingStreamLinkcode: ""
    property string _pendingStreamName: ""

    // ── Helpers ─────────────────────────────────────────────
    function showToast(title, desc, variant) {
        pageToast.title   = title;
        pageToast.desc    = desc   || "";
        pageToast.variant = variant || "info";
        pageToast.visible = true;
        pageToast.show();
    }

    function _isMediaFile(file) {
        if (!file || file.isFolder) return false;
        const c = (file.fileCategory || "").toLowerCase();
        return c === "video" || c === "audio";
    }

    function _fshareUrlFor(file) {
        if (!file || !file.linkcode) return "";
        return "https://www.fshare.vn/" + (file.isFolder ? "folder" : "file") + "/" + file.linkcode;
    }

    function _openFshareUrl(file) {
        const url = _fshareUrlFor(file);
        if (url.length > 0) Qt.openUrlExternally(url);
    }

    function _downloadItem(file) {
        if (!file || !downloadViewModel) return;
        const url = _fshareUrlFor(file);
        if (url.length === 0) return;
        const folder = downloadViewModel.defaultSaveFolder || "";
        downloadViewModel.addDownload(url, folder, "");
        page.showToast(qsTr("Đã thêm vào tải về"), file.name || "", "success");
    }

    function _playMediaFile(file) {
        if (!file || !favoritesViewModel) return;
        if (file.isDownloaded && file.localPath && file.localPath.length > 0) {
            favoritesViewModel.openLocalFile(file.localPath);
            return;
        }
        page._pendingStreamLinkcode = file.linkcode;
        page._pendingStreamName     = file.name || "";
        favoritesViewModel.getStreamLink(file.linkcode);
        page.showToast(qsTr("Đang lấy link xem trực tiếp…"), "", "info");
    }

    function _confirmDelete(linkcodes) {
        if (!linkcodes || linkcodes.length === 0) return;
        deleteConfirmDialog.pendingLinkcodes = linkcodes;
        deleteConfirmDialog.open();
    }

    function _buildContextActions(file) {
        if (!file) return [];
        const actions = [];

        if (file.isFolder) {
            actions.push({
                label: qsTr("Mở thư mục"),
                icon: "📂",
                onTriggered: function() {
                    if (!favoritesViewModel) return;
                    page.selectedFiles = [];
                    page.selectedFileData = null;
                    favoritesViewModel.navigateToFolder(file.linkcode, file.name);
                }
            });
        } else if (page._isMediaFile(file)) {
            actions.push({
                label: qsTr("Xem trực tiếp"),
                icon: "▶",
                onTriggered: function() { page._playMediaFile(file); }
            });
        }

        if (!file.isFolder && file.isDownloaded) {
            actions.push({
                label: qsTr("Mở file"),
                icon: "▶",
                onTriggered: function() {
                    if (favoritesViewModel) favoritesViewModel.openLocalFile(file.localPath);
                }
            });
            actions.push({
                label: qsTr("Mở thư mục chứa"),
                icon: "📂",
                onTriggered: function() {
                    if (favoritesViewModel) favoritesViewModel.openInExplorer(file.localPath);
                }
            });
        }

        actions.push({ separator: true });
        actions.push({
            label: qsTr("Sao chép link"),
            icon: "⎘",
            onTriggered: function() {
                if (favoritesViewModel) favoritesViewModel.copyLinks([file.linkcode]);
            }
        });
        actions.push({
            label: qsTr("Mở trên Fshare"),
            icon: "↗",
            onTriggered: function() { page._openFshareUrl(file); }
        });
        actions.push({
            label: qsTr("Tải về"),
            icon: "⬇",
            onTriggered: function() { page._downloadItem(file); }
        });

        if (favoritesViewModel && !favoritesViewModel.isInFolder) {
            actions.push({ separator: true });
            actions.push({
                label: qsTr("Bỏ yêu thích"),
                icon: "✕",
                danger: true,
                onTriggered: function() {
                    if (favoritesViewModel) favoritesViewModel.removeFromFavorite(file.linkcode);
                }
            });
        }

        return actions;
    }

    Component.onCompleted: {
        if (favoritesViewModel) favoritesViewModel.loadFavorites();
    }

    // Ctrl+F focuses + selects the extension filter — same convention as
    // File Manager, browsers, etc.  WindowShortcut scope is fine because
    // Main.qml hides inactive pages so the shortcut won't fire when the
    // user is on a different page.
    Shortcut {
        // Use `sequences` to attach to every binding StandardKey.Find
        // expands to across platforms (avoids the "binding to one of
        // multiple key bindings" Qt 6 warning).
        sequences: [StandardKey.Find]
        context: Qt.WindowShortcut
        onActivated: {
            extFilterInput.forceActiveFocus();
            extFilterInput.selectAll();
        }
    }

    // Sync ListView.currentIndex → page-level selection by reading the
    // materialised delegate at that index.  Mirrors the same helper in
    // FileManagerPage so the two file-list pages share keyboard ergonomics.
    function _selectByListIndex(idx) {
        if (!favListView || idx < 0) return;
        if (idx >= favListView.count) return;
        favListView.positionViewAtIndex(idx, ListView.Contain);
        favListView.currentIndex = idx;
        const it = favListView.currentItem;
        if (!it) return;
        page.selectedFiles = [it.linkcode];
        if (typeof favoritesViewModel !== "undefined" && favoritesViewModel)
            favoritesViewModel.setSelected([it.linkcode]);
    }

    // ── Keyboard shortcuts ─────────────────────────────
    focus: true
    Keys.onPressed: function(event) {
        // Arrow keys → walk the list cursor.  Wired here (not on ListView)
        // so the same key handler that already owns Ctrl+C / Delete /
        // Ctrl+A keeps a single source of truth.
        if (event.key === Qt.Key_Down) {
            page._selectByListIndex(favListView.currentIndex < 0 ? 0
                : Math.min(favListView.count - 1, favListView.currentIndex + 1));
            event.accepted = true; return;
        }
        if (event.key === Qt.Key_Up) {
            page._selectByListIndex(Math.max(0, favListView.currentIndex - 1));
            event.accepted = true; return;
        }
        if (event.key === Qt.Key_Home) {
            page._selectByListIndex(0); event.accepted = true; return;
        }
        if (event.key === Qt.Key_End) {
            page._selectByListIndex(favListView.count - 1);
            event.accepted = true; return;
        }
        if (event.key === Qt.Key_PageDown) {
            page._selectByListIndex(Math.min(favListView.count - 1,
                favListView.currentIndex + 10));
            event.accepted = true; return;
        }
        if (event.key === Qt.Key_PageUp) {
            page._selectByListIndex(Math.max(0, favListView.currentIndex - 10));
            event.accepted = true; return;
        }
        // Menu key / Shift+F10 → context menu on selected favorite.  Mirrors
        // the FileManagerPage shortcut so the two file-list pages share
        // keyboard ergonomics.  Builds the action list from selectedFileData
        // exactly like the right-click handler does.
        if (event.key === Qt.Key_Menu
            || (event.key === Qt.Key_F10 && (event.modifiers & Qt.ShiftModifier))) {
            if (page.selectedFileData) {
                page._ctxFile = page.selectedFileData;
                fileMenu.actions = page._buildContextActions(page.selectedFileData);
                const it = favListView.currentItem;
                const anchor = it
                    ? it.mapToItem(page, it.width * 0.5, it.height)
                    : { x: page.width * 0.5, y: page.height * 0.5 };
                fileMenu.popup(anchor.x, anchor.y);
                event.accepted = true;
            }
            return;
        }
        if (event.key === Qt.Key_C && (event.modifiers & Qt.ControlModifier)) {
            if (page.selectedFiles.length > 0 && favoritesViewModel) {
                favoritesViewModel.copyLinks(page.selectedFiles);
                event.accepted = true;
            }
            return;
        }
        if (event.key === Qt.Key_Delete) {
            if (favoritesViewModel && !favoritesViewModel.isInFolder
                && page.selectedFiles.length > 0) {
                for (let i = 0; i < page.selectedFiles.length; ++i)
                    favoritesViewModel.removeFromFavorite(page.selectedFiles[i]);
                page.selectedFiles = [];
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (page.selectedFileData && page.selectedFileData.isFolder && favoritesViewModel) {
                favoritesViewModel.navigateToFolder(page.selectedFileData.linkcode,
                                                   page.selectedFileData.name);
                page.selectedFiles = [];
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
        if (event.key === Qt.Key_Backspace) {
            if (favoritesViewModel && favoritesViewModel.isInFolder) {
                page.selectedFiles = [];
                page.selectedFileData = null;
                favoritesViewModel.navigateBack();
                event.accepted = true;
            }
            return;
        }
        if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
            if (favoritesViewModel) {
                favoritesViewModel.selectAll();
                page.selectedFiles = favoritesViewModel.selectedLinkcodes;
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
        if (event.key === Qt.Key_Escape) {
            if (page.selectedFiles.length > 0) {
                page.selectedFiles = [];
                page.selectedFileData = null;
                event.accepted = true;
            }
            return;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: AuroraTheme.sp4

        // ═════════════════════════════════════════════════
        //  EDITORIAL HEADER PANEL
        // ═════════════════════════════════════════════════
        Aurora.FsPageHeader {
            framed: true
            kicker: qsTr("Bộ sưu tập cá nhân")
            title: qsTr("Yêu")
            accentWord: qsTr("thích.")
            titlePixelSize: 56
            titleLetterSpacing: -1.8
            subtitle: {
                if (!favoritesViewModel) return "—";
                const n = favoritesViewModel.totalCount;
                if (n === 0) return qsTr("Chưa có mục nào");
                return qsTr("%1 mục đã gắn sao").arg(n);
            }

            // Filter textbox
            Rectangle {
                Layout.preferredWidth: 260
                Layout.preferredHeight: 36
                Layout.alignment: Qt.AlignVCenter
                radius: AuroraTheme.radiusMd
                color: AuroraTheme.bg
                border.width: 1
                border.color: extFilterInput.activeFocus ? AuroraTheme.accent : AuroraTheme.border
                Behavior on border.color { enabled: !AuroraTheme.reduceMotion
                    ColorAnimation { duration: AuroraTheme.durFast } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp3
                    anchors.rightMargin: AuroraTheme.sp3
                    spacing: AuroraTheme.sp2

                    FsIcon { name: "search"; sizePx: 14; color: AuroraTheme.ink4 }
                    TextInput {
                        id: extFilterInput
                        Layout.fillWidth: true
                        verticalAlignment: TextInput.AlignVCenter
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        color: AuroraTheme.ink1
                        text: favoritesViewModel ? favoritesViewModel.extFilter : ""
                        onAccepted: {
                            if (favoritesViewModel) favoritesViewModel.extFilter = text;
                        }

                        Text {
                            visible: parent.text.length === 0
                            text: qsTr("Lọc theo đuôi file (vd: mp4, pdf)...")
                            color: AuroraTheme.ink4
                            font: parent.font
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        visible: extFilterInput.text.length > 0
                        width: 20; height: 20; radius: 10
                        color: clearFilterMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                        FsIcon { anchors.centerIn: parent; name: "x"; sizePx: 12; color: AuroraTheme.ink3 }
                        MouseArea {
                            id: clearFilterMa
                            anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                extFilterInput.text = "";
                                if (favoritesViewModel) favoritesViewModel.extFilter = "";
                            }
                        }
                    }
                }
            }

            Aurora.FsButton {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Làm mới")
                icon: "refresh"
                variant: "ghost"
                size: "md"
                onClicked: if (favoritesViewModel) favoritesViewModel.loadFavorites()
            }
        }

        // ═════════════════════════════════════════════════
        //  BREADCRUMB
        // ═════════════════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp2
            visible: favoritesViewModel !== null

            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                contentWidth: bcPathRow.implicitWidth
                clip: true
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds

                onContentWidthChanged: {
                    if (contentWidth > width)
                        contentX = contentWidth - width;
                }

                Row {
                    id: bcPathRow
                    spacing: AuroraTheme.sp2
                    height: 28

                    Rectangle {
                        id: bcRootChip
                        readonly property bool atRoot: !favoritesViewModel || !favoritesViewModel.isInFolder
                        width: 28; height: 28; radius: AuroraTheme.radiusSm
                        color: atRoot
                            ? AuroraTheme.accentTint10
                            : (bcRootMa.containsMouse ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.06) : "transparent")
                        Behavior on color { enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast } }
                        anchors.verticalCenter: parent.verticalCenter

                        FsIcon {
                            anchors.centerIn: parent
                            name: "heart"
                            sizePx: 16
                            color: bcRootChip.atRoot
                                ? AuroraTheme.accent
                                : (bcRootMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2)
                            Behavior on color { enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast } }
                        }

                        MouseArea {
                            id: bcRootMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: bcRootChip.atRoot ? Qt.ArrowCursor : Qt.PointingHandCursor
                            onClicked: {
                                if (!bcRootChip.atRoot) {
                                    page.selectedFiles = [];
                                    page.selectedFileData = null;
                                    if (favoritesViewModel) favoritesViewModel.navigateToRoot();
                                }
                            }
                        }

                        ToolTip.visible: bcRootMa.containsMouse && !bcRootChip.atRoot
                        ToolTip.text: qsTr("Về Yêu thích")
                        ToolTip.delay: 500
                    }

                    Repeater {
                        model: favoritesViewModel ? favoritesViewModel.breadcrumbs : []

                        Row {
                            spacing: AuroraTheme.sp2
                            anchors.verticalCenter: parent.verticalCenter

                            FsIcon {
                                name: "chevron-right"; sizePx: 14; color: AuroraTheme.ink4
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                property bool isLast: index === (favoritesViewModel ? favoritesViewModel.breadcrumbs.length - 1 : 0)
                                text: modelData.name || ""
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: isLast ? Font.DemiBold : Font.Medium
                                color: isLast ? AuroraTheme.ink1
                                              : (bcSegMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2)
                                Behavior on color { enabled: !AuroraTheme.reduceMotion
                                    ColorAnimation { duration: AuroraTheme.durFast } }
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                anchors.verticalCenter: parent.verticalCenter

                                MouseArea {
                                    id: bcSegMa; anchors.fill: parent; hoverEnabled: true
                                    cursorShape: parent.isLast ? Qt.ArrowCursor : Qt.PointingHandCursor
                                    onClicked: {
                                        if (!parent.isLast && favoritesViewModel) {
                                            const crumbs = favoritesViewModel.breadcrumbs;
                                            const targetDepth = index + 1;
                                            const currentDepth = crumbs.length;
                                            for (let i = 0; i < currentDepth - targetDepth; i++)
                                                favoritesViewModel.navigateBack();
                                            page.selectedFiles = [];
                                            page.selectedFileData = null;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  LIST + PREVIEW
        // ═════════════════════════════════════════════════
        Rectangle {
            id: favContentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            Flickable {
                id: contentHScroll
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp2
                readonly property int minContentWidth: 320 + 220 + 4
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

                    // ── File list column ────────────────────────────────
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 320

                        FsLoadingState {
                            anchors.fill: parent
                            visible: favoritesViewModel && favoritesViewModel.isLoading
                            message: qsTr("Đang tải danh sách yêu thích...")
                        }

                        FsEmptyState {
                            anchors.fill: parent
                            visible: !favoritesViewModel
                                     || (!favoritesViewModel.isLoading
                                         && favoritesViewModel.totalCount === 0)
                            icon: "❤"
                            title: favoritesViewModel && favoritesViewModel.isInFolder
                                ? qsTr("Thư mục trống")
                                : qsTr("Chưa có mục yêu thích")
                            description: favoritesViewModel && favoritesViewModel.isInFolder
                                ? qsTr("Thư mục này chưa có file nào.")
                                : qsTr("Đánh dấu file hoặc thư mục yêu thích từ trang Quản lý file.")
                        }

                        ListView {
                            id: favListView
                            anchors.fill: parent
                            anchors.margins: AuroraTheme.sp2
                            clip: true
                            visible: favoritesViewModel && !favoritesViewModel.isLoading
                                     && favoritesViewModel.totalCount > 0
                            model: favoritesViewModel ? favoritesViewModel.fileListModel : null
                            spacing: 0

                            header: Rectangle {
                                width: favListView.width
                                height: 32
                                color: AuroraTheme.bg

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: AuroraTheme.sp5
                                    anchors.rightMargin: AuroraTheme.sp5
                                    spacing: AuroraTheme.sp4

                                    Item { Layout.preferredWidth: 24 }
                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr("Tên")
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 10
                                        font.letterSpacing: 1.4
                                        font.capitalization: Font.AllUppercase
                                        font.weight: Font.DemiBold
                                        color: AuroraTheme.ink4
                                    }
                                    Text {
                                        Layout.preferredWidth: 100
                                        text: qsTr("Kích thước")
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 10
                                        font.letterSpacing: 1.4
                                        font.capitalization: Font.AllUppercase
                                        font.weight: Font.DemiBold
                                        color: AuroraTheme.ink4
                                        horizontalAlignment: Text.AlignRight
                                    }
                                    Item { Layout.preferredWidth: 80 }
                                }

                                Rectangle {
                                    anchors.bottom: parent.bottom; width: parent.width; height: 1
                                    color: AuroraTheme.divider
                                }
                            }

                            delegate: Rectangle {
                                id: favDelegate

                                required property string linkcode
                                required property string name
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
                                required property string extension
                                required property string fileCategory

                                width: favListView.width
                                height: 44
                                color: (page.selectedFiles.indexOf(linkcode) >= 0)
                                    ? AuroraTheme.accentTint10
                                    : (rowMa.containsMouse ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.04) : "transparent")
                                Behavior on color { enabled: !AuroraTheme.reduceMotion
                                    ColorAnimation { duration: AuroraTheme.durFast } }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: AuroraTheme.sp5
                                    anchors.rightMargin: AuroraTheme.sp5
                                    spacing: AuroraTheme.sp4

                                    FsFileTypeIcon {
                                        isFolder: favDelegate.isFolder
                                        fileName: favDelegate.name
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
                                                text: favDelegate.name
                                                font.family: AuroraTheme.fontSans
                                                font.pixelSize: 13
                                                font.weight: Font.Medium
                                                color: AuroraTheme.ink1
                                                elide: Text.ElideMiddle
                                            }

                                            Rectangle {
                                                visible: favDelegate.isDownloaded
                                                width: 8; height: 8; radius: 4
                                                color: AuroraTheme.success
                                            }

                                            FsIcon {
                                                visible: favDelegate.hasPassword && !favDelegate.isFolder
                                                name: "key"
                                                sizePx: 13
                                                color: AuroraTheme.warn
                                            }
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: 100
                                        text: favDelegate.isFolder ? "—" : FsFormat.bytes(favDelegate.size)
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 11
                                        color: AuroraTheme.ink3
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    Row {
                                        Layout.preferredWidth: 80
                                        spacing: AuroraTheme.sp2
                                        readonly property bool _active: !favDelegate.isFolder
                                            && page.selectedFiles.indexOf(favDelegate.linkcode) >= 0
                                        opacity: _active ? 1 : 0
                                        enabled: _active

                                        Rectangle {
                                            visible: favDelegate.isDownloaded
                                            width: 24; height: 24; radius: AuroraTheme.radiusSm
                                            color: explorerMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                                            Behavior on color { enabled: !AuroraTheme.reduceMotion
                                                ColorAnimation { duration: AuroraTheme.durFast } }
                                            FsIcon { anchors.centerIn: parent; name: "folder"; sizePx: 14; color: AuroraTheme.ink2 }
                                            MouseArea {
                                                id: explorerMa
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: if (favoritesViewModel) favoritesViewModel.openInExplorer(favDelegate.localPath)
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: rowMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                                    onClicked: function(mouse) {
                                        page.focus = true;

                                        const fileObj = {
                                            linkcode: favDelegate.linkcode,
                                            name: favDelegate.name,
                                            isFolder: favDelegate.isFolder,
                                            size: favDelegate.size,
                                            extension: favDelegate.extension,
                                            fileCategory: favDelegate.fileCategory,
                                            secure: favDelegate.secure,
                                            hasPassword: favDelegate.hasPassword,
                                            directlink: favDelegate.directlink,
                                            downloadCount: favDelegate.downloadCount,
                                            created: favDelegate.created,
                                            modified: favDelegate.modified,
                                            isDownloaded: favDelegate.isDownloaded,
                                            localPath: favDelegate.localPath
                                        };

                                        if (mouse.button === Qt.RightButton) {
                                            page._ctxFile = fileObj;
                                            fileMenu.actions = page._buildContextActions(fileObj);
                                            const pt = mapToItem(page, mouse.x, mouse.y);
                                            fileMenu.popup(pt.x, pt.y);
                                            return;
                                        }

                                        const idx = page.selectedFiles.indexOf(favDelegate.linkcode);
                                        let newSel;
                                        if (mouse.modifiers & Qt.ControlModifier) {
                                            newSel = page.selectedFiles.slice();
                                            if (idx >= 0) newSel.splice(idx, 1);
                                            else          newSel.push(favDelegate.linkcode);
                                        } else {
                                            newSel = [favDelegate.linkcode];
                                        }
                                        page.selectedFiles = newSel;
                                        page.selectedFileData = (newSel.length === 1) ? fileObj : null;

                                        if (favoritesViewModel) {
                                            favoritesViewModel.clearSelection();
                                            for (const lc of newSel)
                                                favoritesViewModel.toggleSelection(lc);
                                        }
                                    }

                                    onDoubleClicked: {
                                        if (favDelegate.isFolder && favoritesViewModel) {
                                            page.selectedFiles = [];
                                            page.selectedFileData = null;
                                            favoritesViewModel.navigateToFolder(favDelegate.linkcode,
                                                                                favDelegate.name);
                                        } else if (page._isMediaFile({
                                                        fileCategory: favDelegate.fileCategory,
                                                        isFolder: favDelegate.isFolder
                                                    })) {
                                            page._playMediaFile({
                                                linkcode: favDelegate.linkcode,
                                                name: favDelegate.name,
                                                isFolder: favDelegate.isFolder,
                                                fileCategory: favDelegate.fileCategory,
                                                isDownloaded: favDelegate.isDownloaded,
                                                localPath: favDelegate.localPath
                                            });
                                        } else if (favDelegate.isDownloaded && favoritesViewModel) {
                                            favoritesViewModel.openLocalFile(favDelegate.localPath);
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
                    }

                    // ── Preview Panel (fixed right column) ───────────────
                    Rectangle {
                        id: detailPanel
                        readonly property int  selCount: page.selectedFiles.length
                        readonly property bool active:   selCount > 0
                        readonly property bool isSingle: selCount === 1 && page.selectedFileData !== null
                        readonly property bool isMulti:  selCount >= 2
                        readonly property int panelWidth: 220

                        function _selectionBreakdown() {
                            const codes = page.selectedFiles;
                            const out = { folders: 0, files: 0 };
                            if (!favoritesViewModel || !codes || codes.length === 0) return out;
                            const model = favoritesViewModel.fileListModel;
                            if (!model || typeof model.getItemAsVariant !== "function") return out;
                            for (let i = 0; i < codes.length; ++i) {
                                const obj = model.getItemAsVariant(codes[i]);
                                if (obj && obj.isFolder) ++out.folders; else ++out.files;
                            }
                            return out;
                        }

                        Layout.preferredWidth: panelWidth
                        Layout.fillHeight: true
                        Layout.leftMargin: AuroraTheme.sp2
                        clip: true

                        color: AuroraTheme.bg
                        radius: AuroraTheme.radiusMd
                        border.width: 1
                        border.color: AuroraTheme.border

                        ColumnLayout {
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: -20
                            width: parent.width - AuroraTheme.sp6 * 2
                            visible: !detailPanel.active
                            spacing: AuroraTheme.sp2

                            FsIcon {
                                Layout.alignment: Qt.AlignHCenter
                                name: "heart"; sizePx: 28
                                color: AuroraTheme.ink4
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Chọn file hoặc thư mục\nđể xem thông tin")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 12
                                color: AuroraTheme.ink4
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

                                // Header
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4
                                    Layout.rightMargin: AuroraTheme.sp2

                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr("THÔNG TIN")
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 10
                                        font.letterSpacing: 1.6
                                        font.capitalization: Font.AllUppercase
                                        font.weight: Font.Bold
                                        color: AuroraTheme.ink4
                                    }
                                    Rectangle {
                                        width: 24; height: 24; radius: AuroraTheme.radiusSm
                                        color: closePanelMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                                        Behavior on color { enabled: !AuroraTheme.reduceMotion
                                            ColorAnimation { duration: AuroraTheme.durFast } }
                                        FsIcon { anchors.centerIn: parent; name: "x"; sizePx: 12; color: AuroraTheme.ink2 }
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

                                // Multi-select count bubble
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 100
                                    Layout.topMargin: AuroraTheme.sp3
                                    visible: detailPanel.isMulti

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 72; height: 72
                                        radius: 36
                                        color: AuroraTheme.accentTint15
                                        border.width: 1; border.color: AuroraTheme.accent

                                        Text {
                                            anchors.centerIn: parent
                                            text: detailPanel.selCount > 99 ? "99+" : String(detailPanel.selCount)
                                            font.family: AuroraTheme.fontSerif
                                            font.pixelSize: 28
                                            font.weight: Font.DemiBold
                                            color: AuroraTheme.accent
                                        }
                                    }
                                }

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
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11
                                    color: AuroraTheme.ink4
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                // Single selection: icon + name
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 100
                                    Layout.topMargin: AuroraTheme.sp3
                                    visible: detailPanel.isSingle

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 72; height: 72
                                        radius: AuroraTheme.radiusLg
                                        color: AuroraTheme.panel
                                        border.width: 1; border.color: AuroraTheme.border

                                        FsFileTypeIcon {
                                            anchors.centerIn: parent
                                            isFolder: page.selectedFileData && page.selectedFileData.isFolder
                                            fileName: page.selectedFileData ? page.selectedFileData.name : ""
                                            sizePx: 36
                                        }
                                    }
                                }

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
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11
                                    color: AuroraTheme.ink4
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.topMargin: AuroraTheme.sp3; Layout.bottomMargin: AuroraTheme.sp2
                                    height: 1; color: AuroraTheme.divider
                                }

                                // Hero action
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp3
                                    Layout.rightMargin: AuroraTheme.sp3
                                    Layout.bottomMargin: AuroraTheme.sp2
                                    visible: detailPanel.isSingle && page.selectedFileData
                                             && (page.selectedFileData.isFolder
                                                 || page._isMediaFile(page.selectedFileData)
                                                 || (page.selectedFileData.isDownloaded))
                                    spacing: AuroraTheme.sp2

                                    Aurora.FsButton {
                                        Layout.fillWidth: true
                                        visible: page.selectedFileData && page.selectedFileData.isFolder
                                        text: qsTr("Mở thư mục"); variant: "primary"; size: "sm"
                                        onClicked: {
                                            const f = page.selectedFileData;
                                            page.selectedFiles = [];
                                            page.selectedFileData = null;
                                            if (favoritesViewModel)
                                                favoritesViewModel.navigateToFolder(f.linkcode, f.name);
                                        }
                                    }

                                    Aurora.FsButton {
                                        Layout.fillWidth: true
                                        visible: page.selectedFileData
                                                 && !page.selectedFileData.isFolder
                                                 && page._isMediaFile(page.selectedFileData)
                                        text: qsTr("Xem trực tiếp"); icon: "play"; variant: "primary"; size: "sm"
                                        onClicked: page._playMediaFile(page.selectedFileData)
                                    }

                                    Aurora.FsButton {
                                        Layout.fillWidth: true
                                        visible: page.selectedFileData
                                                 && !page.selectedFileData.isFolder
                                                 && !page._isMediaFile(page.selectedFileData)
                                                 && page.selectedFileData.isDownloaded
                                        text: qsTr("Mở file"); variant: "primary"; size: "sm"
                                        onClicked: {
                                            if (favoritesViewModel && page.selectedFileData)
                                                favoritesViewModel.openLocalFile(page.selectedFileData.localPath);
                                        }
                                    }
                                }

                                Text {
                                    Layout.leftMargin: AuroraTheme.sp4
                                    Layout.topMargin: AuroraTheme.sp3
                                    Layout.bottomMargin: AuroraTheme.sp2
                                    visible: detailPanel.active
                                    text: qsTr("THAO TÁC")
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 10
                                    font.letterSpacing: 1.6
                                    font.capitalization: Font.AllUppercase
                                    font.weight: Font.Bold
                                    color: AuroraTheme.ink4
                                }

                                Aurora.FsButton {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp3
                                    Layout.rightMargin: AuroraTheme.sp3
                                    visible: detailPanel.active
                                    text: qsTr("Sao chép link"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (favoritesViewModel && page.selectedFiles.length > 0)
                                            favoritesViewModel.copyLinks(page.selectedFiles);
                                    }
                                }

                                Aurora.FsButton {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp3
                                    Layout.rightMargin: AuroraTheme.sp3
                                    Layout.topMargin: AuroraTheme.sp2
                                    visible: detailPanel.isSingle
                                    text: qsTr("Mở trên Fshare"); variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (page.selectedFileData) page._openFshareUrl(page.selectedFileData);
                                    }
                                }

                                Aurora.FsButton {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp3
                                    Layout.rightMargin: AuroraTheme.sp3
                                    Layout.topMargin: AuroraTheme.sp2
                                    visible: detailPanel.active
                                    text: detailPanel.isMulti
                                        ? qsTr("Tải về %1 mục").arg(detailPanel.selCount)
                                        : qsTr("Tải về")
                                    variant: "ghost"; size: "sm"
                                    onClicked: {
                                        if (detailPanel.isSingle && page.selectedFileData) {
                                            page._downloadItem(page.selectedFileData);
                                            return;
                                        }
                                        if (!favoritesViewModel || !downloadViewModel) return;
                                        const model = favoritesViewModel.fileListModel;
                                        const folder = downloadViewModel.defaultSaveFolder || "";
                                        let queued = 0;
                                        for (let i = 0; i < page.selectedFiles.length; ++i) {
                                            const obj = model ? model.getItemAsVariant(page.selectedFiles[i]) : null;
                                            if (!obj) continue;
                                            const url = page._fshareUrlFor(obj);
                                            if (url.length > 0) {
                                                downloadViewModel.addDownload(url, folder, "");
                                                ++queued;
                                            }
                                        }
                                        if (queued > 0)
                                            page.showToast(qsTr("Đã thêm vào tải về"),
                                                           qsTr("%1 mục đã xếp hàng").arg(queued),
                                                           "success");
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.topMargin: AuroraTheme.sp3; Layout.bottomMargin: AuroraTheme.sp2
                                    visible: detailPanel.isSingle
                                    height: 1; color: AuroraTheme.divider
                                }

                                Text {
                                    Layout.leftMargin: AuroraTheme.sp4
                                    Layout.bottomMargin: AuroraTheme.sp2
                                    visible: detailPanel.isSingle
                                    text: qsTr("CHI TIẾT")
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 10
                                    font.letterSpacing: 1.6
                                    font.capitalization: Font.AllUppercase
                                    font.weight: Font.Bold
                                    color: AuroraTheme.ink4
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.preferredHeight: 26
                                    visible: detailPanel.isSingle
                                    Text {
                                        Layout.preferredWidth: 80
                                        text: qsTr("Kích thước"); color: AuroraTheme.ink4
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

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.preferredHeight: 26
                                    visible: detailPanel.isSingle
                                    Text {
                                        Layout.preferredWidth: 80
                                        text: qsTr("Ngày tạo"); color: AuroraTheme.ink4
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

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.preferredHeight: 26
                                    visible: detailPanel.isSingle
                                    Text {
                                        Layout.preferredWidth: 80
                                        text: qsTr("Sửa đổi"); color: AuroraTheme.ink4
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

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.preferredHeight: 26
                                    visible: detailPanel.isSingle && page.selectedFileData && !page.selectedFileData.isFolder
                                    Text {
                                        Layout.preferredWidth: 80
                                        text: qsTr("Lượt tải"); color: AuroraTheme.ink4
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

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp4; Layout.rightMargin: AuroraTheme.sp4
                                    Layout.topMargin: AuroraTheme.sp3; Layout.bottomMargin: AuroraTheme.sp2
                                    visible: detailPanel.isSingle
                                    height: 1; color: AuroraTheme.divider
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: AuroraTheme.sp3
                                    Layout.rightMargin: AuroraTheme.sp3
                                    Layout.topMargin: detailPanel.isMulti ? AuroraTheme.sp3 : 0
                                    Layout.bottomMargin: AuroraTheme.sp4
                                    spacing: AuroraTheme.sp2

                                    Aurora.FsButton {
                                        Layout.fillWidth: true
                                        visible: detailPanel.isSingle && page.selectedFileData && page.selectedFileData.isDownloaded
                                        text: qsTr("Mở thư mục chứa"); variant: "ghost"; size: "sm"
                                        onClicked: {
                                            if (favoritesViewModel && page.selectedFileData)
                                                favoritesViewModel.openInExplorer(page.selectedFileData.localPath);
                                        }
                                    }

                                    Aurora.FsButton {
                                        Layout.fillWidth: true
                                        visible: detailPanel.active
                                                 && favoritesViewModel
                                                 && !favoritesViewModel.isInFolder
                                        text: detailPanel.isMulti
                                            ? qsTr("Bỏ yêu thích %1 mục").arg(detailPanel.selCount)
                                            : qsTr("Bỏ yêu thích")
                                        variant: "danger"; size: "sm"
                                        onClicked: {
                                            if (!favoritesViewModel) return;
                                            const codes = page.selectedFiles.slice();
                                            for (let i = 0; i < codes.length; ++i)
                                                favoritesViewModel.removeFromFavorite(codes[i]);
                                            page.selectedFiles = [];
                                            page.selectedFileData = null;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Context menu ────────────────────────────────────
    FsContextMenu {
        id: fileMenu
        anchors.fill: parent
        menuWidth: 230
        actions: []
    }

    // ── Delete confirmation ─────────────────────────────
    FsConfirmDialog {
        id: deleteConfirmDialog
        property var pendingLinkcodes: []

        title: qsTr("Xác nhận xóa")
        message: qsTr("Bạn có chắc chắn muốn xóa %1 mục đã chọn? Thao tác này không thể hoàn tác.")
                    .arg(pendingLinkcodes.length)
        primaryLabel: qsTr("Xóa")
        dangerAction: true

        onConfirmed: {
            if (favoritesViewModel && pendingLinkcodes.length > 0)
                favoritesViewModel.deleteFiles(pendingLinkcodes);
            page.selectedFiles = [];
            page.selectedFileData = null;
            pendingLinkcodes = [];
        }
    }

    // ── Toast ───────────────────────────────────────────
    FsToast {
        id: pageToast
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: AuroraTheme.sp4
        anchors.rightMargin: AuroraTheme.sp4
        visible: false
        autoCloseMs: 4000
        onClosed: visible = false
    }

    // ── Signal handlers ─────────────────────────────────
    Connections {
        target: favoritesViewModel

        function onLinksCopied(count) {
            showToast(qsTr("Đã sao chép"),
                      qsTr("%1 link đã được sao chép vào clipboard").arg(count),
                      "success");
        }

        function onOperationMessage(msg, isError) {
            showToast(isError ? qsTr("Lỗi") : qsTr("Thành công"), msg, isError ? "error" : "success");
            if (!isError && page.selectedFileData && favoritesViewModel) {
                page.selectedFileData = favoritesViewModel.fileListModel
                    .getItemAsVariant(page.selectedFileData.linkcode);
            }
        }

        function onStreamLinkReady(linkcode, url) {
            if (page._pendingStreamLinkcode === linkcode) {
                const hintName = page._pendingStreamName;
                page._pendingStreamLinkcode = "";
                page._pendingStreamName     = "";
                if (favoritesViewModel)
                    favoritesViewModel.playStreamUrl(url, hintName);
                showToast(qsTr("Đang phát"),
                          qsTr("Đã mở trình phát mặc định"),
                          "success");
            } else {
                showToast(qsTr("Link tải sẵn sàng"),
                          qsTr("Đã sao chép vào clipboard"),
                          "success");
            }
        }

        function onStreamLinkError(message) {
            page._pendingStreamLinkcode = "";
            page._pendingStreamName     = "";
            showToast(qsTr("Lỗi"), message, "error");
        }
    }
}
