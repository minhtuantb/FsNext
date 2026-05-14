// SPDX-License-Identifier: Proprietary
// SyncPage (Aurora) — editorial header + folder pair rows + file list.
//
// Matches handoff `aurora-screens.jsx → AuroraSync` adapted for the
// one-way backup model the VM actually implements (local → Fshare):
//   • Top panel: serif mega-number folder count + mini stats + "Thêm folder".
//   • Folder pairs: Local → Fshare rows, click to activate; state badge
//     (syncing / idle / missing / failed).
//   • Active folder: compact icon toolbar (scan / open / reset / remove)
//     + inline switches (Enable, Delete after upload) + file list.
//
// All VM wiring preserved:
//   syncViewModel.folders, files, maxFolders, activeFolderId,
//   activeLocalPath, activeFshareFolderName, activeEnabled,
//   deleteAfterUpload, canAddMore, speedLimitKBps;
//   addFolder, removeFolder, scanNow, resetFolderCache, setEnabled,
//   setDeleteAfterUpload, openLocalFolder, copyLinkFor, openFshareLink;
//   signals onAddFolderError, onInfoMessage.

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Fshare.Components 1.0
import Fshare.Dialogs 1.0
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Item {
    id: page

    readonly property bool hasVM: typeof syncViewModel !== "undefined" && syncViewModel !== null
    readonly property int folderCount: hasVM ? syncViewModel.folders.count : 0
    readonly property int maxFolders: hasVM ? syncViewModel.maxFolders : 5
    readonly property string activeId: hasVM ? syncViewModel.activeFolderId : ""
    readonly property int activeFileCount: hasVM ? syncViewModel.files.count : 0

    // Aggregated counts across all folders (for mini stats).
    readonly property int totalFiles: {
        if (!hasVM) return 0;
        let n = 0;
        for (let i = 0; i < syncViewModel.folders.count; ++i)
            n += syncViewModel.folders.get(i).fileCount;
        return n;
    }
    readonly property int totalSynced: {
        if (!hasVM) return 0;
        let n = 0;
        for (let i = 0; i < syncViewModel.folders.count; ++i)
            n += syncViewModel.folders.get(i).syncedCount;
        return n;
    }
    readonly property int totalFailed: {
        if (!hasVM) return 0;
        let n = 0;
        for (let i = 0; i < syncViewModel.folders.count; ++i)
            n += syncViewModel.folders.get(i).failedCount;
        return n;
    }

    // Compact 32×32 icon button (Aurora hover tints).
    component IconBtn : Rectangle {
        id: ib
        property string iconName: ""
        property string tooltip: ""
        property bool danger: false
        property bool enabledBtn: true
        signal clicked()

        // Tab-stop + keyboard activation — without these IconBtn was
        // mouse-only; now Space/Enter triggers `clicked()`.  Disabled
        // buttons drop out of the focus chain so a Tab-rush doesn't park
        // on a no-op.
        activeFocusOnTab: ib.enabledBtn
        Accessible.role: Accessible.Button
        Accessible.name: ib.tooltip
        Accessible.onPressAction: if (ib.enabledBtn) ib.clicked()
        Keys.onPressed: function(event) {
            if (!ib.enabledBtn) return;
            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
                || event.key === Qt.Key_Enter) {
                ib.clicked();
                event.accepted = true;
            }
        }

        width: 32; height: 32
        radius: AuroraTheme.radiusSm
        color: !ib.enabledBtn ? "transparent"
             : ibMa.pressed ? (ib.danger ? AuroraTheme.dangerSoft : AuroraTheme.accentTint15)
             : (ibMa.containsMouse || ib.activeFocus)
                ? (ib.danger ? Qt.rgba(AuroraTheme.danger.r, AuroraTheme.danger.g, AuroraTheme.danger.b, 0.08) : AuroraTheme.accentTint10)
             : "transparent"
        opacity: ib.enabledBtn ? 1.0 : 0.35
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }

        Aurora.FsIcon {
            anchors.centerIn: parent
            name: ib.iconName
            sizePx: 16
            color: ib.danger ? AuroraTheme.danger : AuroraTheme.ink2
        }

        // Focus ring — matches the FsButton pattern.  Kept under the icon
        // (z: -1) so the icon glyph stays crisp.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: AuroraTheme.radiusSm + 3
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                                   AuroraTheme.accent.b, 0.55)
            visible: ib.activeFocus
            z: -1
        }

        // Tooltip shows on hover OR keyboard focus — without the activeFocus
        // branch the tooltip was effectively mouse-only, so a Tab user
        // couldn't tell what each pause/resume/retry icon did.
        ToolTip.visible: ib.enabledBtn && ib.tooltip.length > 0
                         && (ibMa.containsMouse || ib.activeFocus)
        ToolTip.text: ib.tooltip
        ToolTip.delay: ib.activeFocus ? 0 : 400      // instant on focus, deferred on hover

        MouseArea {
            id: ibMa
            anchors.fill: parent
            hoverEnabled: ib.enabledBtn
            cursorShape: ib.enabledBtn ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (ib.enabledBtn) ib.clicked()
        }
    }

    // ── Toasts ─────────────────────────────────────────────
    FsToast {
        id: errToast
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: AuroraTheme.sp4
        anchors.rightMargin: AuroraTheme.sp4
        visible: false
        variant: "danger"
        autoCloseMs: 5000
        onClosed: visible = false
    }
    FsToast {
        id: infoToast
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: AuroraTheme.sp4
        anchors.rightMargin: AuroraTheme.sp4
        visible: false
        variant: "success"
        autoCloseMs: 3500
        onClosed: visible = false
    }

    Connections {
        target: hasVM ? syncViewModel : null
        function onAddFolderError(msg) {
            errToast.title = qsTr("Không thể thêm");
            errToast.desc = msg;
            errToast.visible = true;
        }
        function onInfoMessage(msg) {
            infoToast.title = msg;
            infoToast.desc = "";
            infoToast.visible = true;
        }
    }

    // Rich add-folder wizard: picks the local path, surfaces the per-folder
    // toggles (subfolder scan, ignore patterns, delete-after-upload) up
    // front so the very first scan honours them.  Remote destination stays
    // auto-derived from the local folder leaf (matches SyncService — see
    // remoteFolderEditable note in the dialog).  Speed limit is intentionally
    // not editable at creation time; user tunes it post-create via
    // WatchFolderSettingsDialog where the spinner UI lives.
    AddWatchFolderDialog {
        id: addWatchDialog
        remoteFolderEditable: false
        onAccepted: function(localPath, remoteFolderId, remoteFolderPath,
                             watchSub, deleteAfter, ignorePat) {
            if (!page.hasVM) return;
            // 5120 KB/s = 5 MiB/s = legacy SyncService::kSpeedLimitBps; matches
            // what every pre-v6 folder runs at, so new folders inherit the
            // same "safe default" until the user explicitly raises it.
            syncViewModel.addFolderWithSettings(localPath, watchSub, ignorePat,
                                                /*speedKBps=*/ 5120, deleteAfter);
        }
        // Step-2 "Sẽ quét N file" preview — fire-and-forget request when the
        // user taps "Tiếp tục".  The matching previewReady signal handler
        // below pushes the result back into dialog properties when it lands.
        onPreviewRequested: function(localPath, watchSub, ignorePat) {
            if (!page.hasVM) return;
            previewLoading = true;
            previewFileCount = 0;
            syncViewModel.requestPreview(localPath, watchSub, ignorePat);
        }
    }

    // Drives the dry-run preview hint box inside AddWatchFolderDialog Step 2.
    // Kept at page scope (not inside the dialog) because the dialog is
    // VM-agnostic by design — it just consumes flat property values.
    Connections {
        target: page.hasVM ? syncViewModel : null
        function onPreviewReady(fileCount, totalBytes, errorMessage) {
            addWatchDialog.previewLoading = false;
            addWatchDialog.previewFileCount = fileCount;
            // Surface error via the existing addFolderError toast — the
            // dialog itself shows a 0-count preview which the user can read
            // as "nothing to upload" and just go back to Step 1.
            if (errorMessage && errorMessage.length > 0) {
                errToast.title = qsTr("Không thể quét thư mục");
                errToast.desc = errorMessage;
                errToast.visible = true;
                return;
            }
            // Format bytes inline — no FsFormat import on the SyncPage scope
            // and the value is single-use, so the few lines pay off here.
            const units = ["B", "KB", "MB", "GB", "TB"];
            let n = totalBytes;
            let u = 0;
            while (n >= 1024 && u < units.length - 1) { n /= 1024; ++u; }
            addWatchDialog.previewTotalSize =
                (u === 0 ? n.toString() : n.toFixed(1)) + " " + units[u];
            // Quota check is best-effort — userInfoViewModel may not be
            // available before login; default to "ok" so the user isn't
            // blocked by an unknown.
            if (typeof userInfoViewModel !== "undefined" && userInfoViewModel) {
                const free = userInfoViewModel.webspaceTotal
                           - userInfoViewModel.webspaceUsed;
                addWatchDialog.previewQuotaOk = free <= 0 || totalBytes <= free;
            } else {
                addWatchDialog.previewQuotaOk = true;
            }
        }
    }

    // Per-folder settings editor — opened from the active folder toolbar.
    // Mirrors the data shape SyncFoldersModel exposes (watchSubfolders,
    // ignorePatterns, speedLimitKBps roles) so the form opens pre-populated.
    WatchFolderSettingsDialog {
        id: settingsDialog
        onSaved: function(watchId, watchSub, deleteAfter, ignorePat, speedLimitKBps) {
            if (!page.hasVM) return;
            syncViewModel.updateFolderSettings(watchId, watchSub, deleteAfter,
                                                ignorePat, speedLimitKBps);
        }

        // Helper: pull the active folder's current settings out of the
        // SyncFoldersModel and hand them to the dialog before showing it.
        // Done here (page scope) instead of inside the dialog so the dialog
        // stays VM-agnostic — it just consumes/emits flat values.
        function openFor(folderId) {
            if (!page.hasVM) return;
            const folders = syncViewModel.folders;
            for (let i = 0; i < folders.count; ++i) {
                const f = folders.get(i);
                if (f && f.id === folderId) {
                    watchId           = folderId;
                    localPath         = f.localPath;
                    remotePath        = "/" + (f.fshareFolderName || "");
                    watchSubfolders   = f.watchSubfolders;
                    deleteAfterUpload = f.deleteAfterUpload;
                    ignorePatterns    = f.ignorePatterns || "";
                    speedLimitKBps    = f.speedLimitKBps > 0 ? f.speedLimitKBps : 5120;
                    speedLimitMode    = f.speedLimitKBps > 0 ? 1 : 0;
                    open();
                    return;
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: AuroraTheme.sp4

        // ═════════════════════════════════════════════════
        //  EDITORIAL HEADER PANEL
        // ═════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: syncHeaderCol.implicitHeight + AuroraTheme.sp6 * 2
            radius: AuroraTheme.radiusLg
            color: AuroraTheme.panel
            border.width: 1
            border.color: AuroraTheme.border

            ColumnLayout {
                id: syncHeaderCol
                anchors.fill: parent
                anchors.margins: AuroraTheme.sp6
                spacing: AuroraTheme.sp4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp6

                    ColumnLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            text: "━━ Đồng bộ · " + page.folderCount + "/" + page.maxFolders + " folder"
                            color: AuroraTheme.ink4
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            font.letterSpacing: 2.0
                            font.capitalization: Font.AllUppercase
                        }

                        RowLayout {
                            spacing: 8
                            Layout.bottomMargin: 2

                            Text {
                                text: page.folderCount > 0 ? String(page.folderCount) : "—"
                                color: AuroraTheme.ink1
                                font.family: AuroraTheme.fontSerif
                                font.pixelSize: 56
                                font.letterSpacing: -1.8
                                lineHeight: 1.0
                            }

                            Text {
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: 10
                                text: qsTr("folder đang sync.")
                                color: AuroraTheme.accent
                                font.family: AuroraTheme.fontSerif
                                font.italic: true
                                font.pixelSize: 22
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        visible: page.folderCount > 0
                        spacing: AuroraTheme.sp6
                        Layout.alignment: Qt.AlignVCenter

                        Repeater {
                            model: [
                                {
                                    label: "Đã sao lưu",
                                    value: page.totalSynced + "/" + page.totalFiles
                                },
                                {
                                    label: "Lỗi",
                                    value: String(page.totalFailed)
                                },
                                {
                                    label: "Giới hạn",
                                    value: (page.hasVM ? (syncViewModel.speedLimitKBps / 1024).toFixed(0) : "5") + " MB/s"
                                }
                            ]
                            delegate: ColumnLayout {
                                spacing: 2
                                Text {
                                    text: modelData.label
                                    color: AuroraTheme.ink4
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 10
                                    font.letterSpacing: 1.4
                                    font.capitalization: Font.AllUppercase
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: modelData.value
                                    color: AuroraTheme.ink1
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 16
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }

                    // Master "auto-sync" toggle — global pause that overrides
                    // every per-folder enabled flag.  Lives in the header so
                    // it's the FIRST thing the user sees, matching the
                    // AutoUpload design doc's intent.  Uses Aurora token
                    // tints for the chip background so it reads "settings",
                    // not "destructive".
                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredHeight: 36
                        Layout.preferredWidth: masterRow.implicitWidth + AuroraTheme.sp4 * 2
                        radius: AuroraTheme.radiusMd
                        color: page.hasVM && syncViewModel.autoSyncEnabled
                            ? AuroraTheme.accentTint10
                            : Qt.rgba(AuroraTheme.ink4.r, AuroraTheme.ink4.g,
                                       AuroraTheme.ink4.b, 0.08)
                        border.width: 1
                        border.color: page.hasVM && syncViewModel.autoSyncEnabled
                            ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g,
                                       AuroraTheme.accent.b, 0.22)
                            : AuroraTheme.border
                        Behavior on color { enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast } }

                        RowLayout {
                            id: masterRow
                            anchors.fill: parent
                            anchors.leftMargin: AuroraTheme.sp4
                            anchors.rightMargin: AuroraTheme.sp4
                            spacing: AuroraTheme.sp2

                            Aurora.FsIcon {
                                name: "power"
                                sizePx: 14
                                color: page.hasVM && syncViewModel.autoSyncEnabled
                                    ? AuroraTheme.accent : AuroraTheme.ink3
                            }
                            Text {
                                text: page.hasVM && syncViewModel.autoSyncEnabled
                                    ? qsTr("Tự động đồng bộ") : qsTr("Đã tạm dừng")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: AuroraTheme.body.pixelSize
                                font.weight: Font.DemiBold
                                color: page.hasVM && syncViewModel.autoSyncEnabled
                                    ? AuroraTheme.ink1 : AuroraTheme.ink2
                            }
                            FsSwitch {
                                checked: page.hasVM && syncViewModel.autoSyncEnabled
                                onToggled: function(c) {
                                    if (page.hasVM) syncViewModel.autoSyncEnabled = c;
                                }
                            }
                        }
                    }

                    Aurora.FsButton {
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Thêm folder sync")
                        icon: "plus"
                        variant: "primary"
                        size: "md"
                        enabled: page.hasVM && syncViewModel.canAddMore
                        onClicked: addWatchDialog.open()
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  EMPTY STATE (no folders)
        // ═════════════════════════════════════════════════
        FsEmptyState {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.folderCount === 0
            icon: "↻"
            title: qsTr("Chưa có thư mục đồng bộ")
            description: qsTr("Tự động sao lưu tối đa %1 thư mục, giới hạn %2 MB/s để không ảnh hưởng mạng. Có thể tự động xoá bản local sau khi tải xong.")
                .arg(page.maxFolders)
                .arg(page.hasVM ? (syncViewModel.speedLimitKBps / 1024).toFixed(0) : 5)
            actionText: qsTr("Thêm thư mục đầu tiên")
            onActionClicked: addWatchDialog.open()
        }

        // ═════════════════════════════════════════════════
        //  FOLDER PAIRS
        // ═════════════════════════════════════════════════
        ColumnLayout {
            Layout.fillWidth: true
            visible: page.folderCount > 0
            spacing: AuroraTheme.sp2

            Text {
                Layout.leftMargin: AuroraTheme.sp2
                text: "━ Cặp thư mục · " + page.folderCount
                color: AuroraTheme.ink4
                font.family: AuroraTheme.fontMono
                font.pixelSize: 11
                font.letterSpacing: 2.0
                font.capitalization: Font.AllUppercase
                font.weight: Font.DemiBold
            }

            Repeater {
                model: page.hasVM ? syncViewModel.folders : null
                delegate: Rectangle {
                    id: pair
                    required property string id
                    required property string localPath
                    required property string fshareFolderName
                    required property bool   enabled
                    required property int    fileCount
                    required property int    syncedCount
                    required property int    failedCount
                    // v6.0: VM-side aggregate status — replaces the local
                    // computed stateKey so Missing folders surface their own
                    // pill and the QML side becomes a pure renderer.
                    required property int    status        // SyncFolderUiState
                    required property string statusText
                    // v6.0 Phase 3 — live progress.  ratio < 0 = idle (bar
                    // hidden); 0..1 fills the strip along the bottom edge.
                    required property real   uploadProgress
                    required property string uploadSpeedText
                    required property string uploadEtaText

                    readonly property bool active: id === page.activeId

                    // Map the enum to design-system colours.  Order matches
                    // SyncFoldersModel::SyncFolderUiState exactly:
                    //   0 Idle, 1 Uploading, 2 Paused, 3 Error, 4 Missing.
                    readonly property color stateColor: {
                        switch (status) {
                        case 1: return AuroraTheme.accent;       // Uploading
                        case 0: return AuroraTheme.success;       // Idle
                        case 3: return AuroraTheme.danger;        // Error
                        case 4: return AuroraTheme.warn;          // Missing
                        default: return AuroraTheme.ink4;          // Paused
                        }
                    }
                    readonly property color stateBg: {
                        switch (status) {
                        case 1: return AuroraTheme.accentTint15;
                        case 0: return Qt.rgba(AuroraTheme.success.r,
                                                AuroraTheme.success.g,
                                                AuroraTheme.success.b, 0.12);
                        case 3: return AuroraTheme.dangerSoft;
                        case 4: return AuroraTheme.warnSoft;
                        default: return Qt.rgba(AuroraTheme.ink4.r,
                                                 AuroraTheme.ink4.g,
                                                 AuroraTheme.ink4.b, 0.08);
                        }
                    }
                    readonly property string stateLabel: {
                        // Glyph prefix per state — gives quick visual scan
                        // before the user reads the label.
                        switch (status) {
                        case 1: return "↻ " + statusText;
                        case 0: return "✓ " + statusText;
                        case 3: return "⚠ " + statusText;
                        case 4: return "✕ " + statusText;
                        default: return "⏸ " + statusText;
                        }
                    }

                    Layout.fillWidth: true
                    Layout.preferredHeight: pairRow.implicitHeight + AuroraTheme.sp5 * 2
                    radius: AuroraTheme.radiusLg
                    color: active
                        ? AuroraTheme.accentTint10
                        : (pairMa.containsMouse ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.04) : AuroraTheme.panel)
                    border.width: active ? 2 : 1
                    border.color: active ? AuroraTheme.accent : AuroraTheme.border
                    Behavior on color { enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast } }

                    MouseArea {
                        id: pairMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (page.hasVM) syncViewModel.activeFolderId = pair.id
                    }

                    RowLayout {
                        id: pairRow
                        anchors.fill: parent
                        anchors.margins: AuroraTheme.sp5
                        spacing: AuroraTheme.sp4

                        // Local side
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            spacing: 2

                            Text {
                                text: qsTr("Máy tính")
                                color: AuroraTheme.ink4
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                font.weight: Font.DemiBold
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: AuroraTheme.sp2
                                Aurora.FsIcon {
                                    name: "folder"
                                    sizePx: 15
                                    color: pair.enabled ? AuroraTheme.ink2 : AuroraTheme.ink4
                                    opacity: pair.enabled ? 1.0 : 0.6
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: pair.localPath
                                    color: AuroraTheme.ink1
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    elide: Text.ElideMiddle
                                }
                            }
                        }

                        // Arrow indicator
                        ColumnLayout {
                            Layout.preferredWidth: 24
                            spacing: 2
                            Aurora.FsIcon {
                                Layout.alignment: Qt.AlignHCenter
                                name: "arrow-up"
                                sizePx: 12
                                color: pair.status === 1 ? AuroraTheme.accent : AuroraTheme.ink4   // 1 = Uploading
                            }
                            Aurora.FsIcon {
                                Layout.alignment: Qt.AlignHCenter
                                name: "arrow-down"
                                sizePx: 12
                                color: AuroraTheme.ink4
                                opacity: 0.45
                            }
                        }

                        // Fshare side
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            spacing: 2

                            Text {
                                text: qsTr("Fshare Cloud")
                                color: AuroraTheme.ink4
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.letterSpacing: 1.4
                                font.capitalization: Font.AllUppercase
                                font.weight: Font.DemiBold
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: AuroraTheme.sp2
                                Aurora.FsIcon {
                                    name: "upload"
                                    sizePx: 15
                                    color: AuroraTheme.accent
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: "/" + pair.fshareFolderName
                                    color: AuroraTheme.ink1
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    elide: Text.ElideMiddle
                                }
                            }
                        }

                        // State + counts
                        ColumnLayout {
                            Layout.preferredWidth: 150
                            spacing: 4
                            Rectangle {
                                Layout.alignment: Qt.AlignRight
                                implicitWidth: badgeTxt.implicitWidth + 16
                                implicitHeight: 22
                                radius: 11
                                color: pair.stateBg
                                Text {
                                    id: badgeTxt
                                    anchors.centerIn: parent
                                    text: pair.stateLabel
                                    font.family: AuroraTheme.fontSans
                                    font.pixelSize: 11
                                    font.weight: Font.Bold
                                    color: pair.stateColor
                                }
                            }
                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: pair.syncedCount + " / " + pair.fileCount + qsTr(" file")
                                color: AuroraTheme.ink4
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                            }
                            // Speed + ETA mono line — appears only when the
                            // folder has an active upload (uploadProgress
                            // crosses 0).  Matches the same "small mono"
                            // visual the count line uses so the two read as
                            // a pair of stats, not competing widgets.
                            Text {
                                Layout.alignment: Qt.AlignRight
                                visible: pair.uploadProgress >= 0
                                         && pair.uploadSpeedText.length > 0
                                text: pair.uploadSpeedText
                                      + (pair.uploadEtaText.length > 0
                                          ? " · " + pair.uploadEtaText
                                          : "")
                                color: AuroraTheme.accent
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                        }
                    }

                    // Thin progress strip along the bottom edge — same
                    // pattern as FsProgressBar but inlined here so it can
                    // anchor to the parent Rectangle's corners and inherit
                    // its rounded radius.  Hidden when ratio < 0 (idle).
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 3
                        color: Qt.rgba(AuroraTheme.accent.r,
                                        AuroraTheme.accent.g,
                                        AuroraTheme.accent.b, 0.10)
                        visible: pair.uploadProgress >= 0
                        radius: 1.5
                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: Math.max(0, pair.uploadProgress) * parent.width
                            radius: 1.5
                            color: AuroraTheme.accent
                            Behavior on width { enabled: !AuroraTheme.reduceMotion
                                NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                        }
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  ACTIVE FOLDER DETAIL: TOOLBAR + FILE LIST
        // ═════════════════════════════════════════════════
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.folderCount > 0 && page.activeId.length > 0
            spacing: AuroraTheme.sp3

            // Compact toolbar row — icon actions + inline toggles
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: toolbarRow.implicitHeight + AuroraTheme.sp4 * 2
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border

                RowLayout {
                    id: toolbarRow
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp4
                    spacing: AuroraTheme.sp3

                    IconBtn {
                        iconName: "sync"
                        tooltip: qsTr("Đồng bộ ngay")
                        onClicked: confirmScan.open()
                    }
                    // Pause/Resume the active folder's in-flight uploads.
                    // Two side-by-side buttons (instead of one toggle) so the
                    // intent is unambiguous — TransferService::pauseTask /
                    // resumeTask are idempotent so a duplicate click is safe.
                    // Both stay enabled regardless of progress so a user can
                    // pre-emptively pause before files even start.
                    IconBtn {
                        iconName: "pause"
                        tooltip: qsTr("Tạm dừng tải lên")
                        enabledBtn: page.activeId.length > 0
                        onClicked: if (page.hasVM) syncViewModel.pauseFolder(page.activeId)
                    }
                    IconBtn {
                        iconName: "play"
                        tooltip: qsTr("Tiếp tục tải lên")
                        enabledBtn: page.activeId.length > 0
                        onClicked: if (page.hasVM) syncViewModel.resumeFolder(page.activeId)
                    }
                    // Retry — only visible when there are failed files to
                    // retry; reduces toolbar noise during normal operation.
                    IconBtn {
                        iconName: "refresh"
                        tooltip: qsTr("Thử lại file lỗi (%1)").arg(page.totalFailed)
                        visible: page.totalFailed > 0
                        onClicked: if (page.hasVM) syncViewModel.retryFailed(page.activeId)
                    }
                    IconBtn {
                        iconName: "folder-open"
                        tooltip: qsTr("Mở thư mục")
                        onClicked: if (page.hasVM) syncViewModel.openLocalFolder(page.activeId)
                    }
                    IconBtn {
                        iconName: "refresh"
                        tooltip: qsTr("Xoá cache và quét lại")
                        onClicked: confirmReset.open()
                    }
                    IconBtn {
                        iconName: "gear"
                        tooltip: qsTr("Cài đặt thư mục")
                        // Disabled when no folder is active — opens
                        // WatchFolderSettingsDialog pre-populated with the
                        // active folder's current per-folder settings.
                        enabledBtn: page.activeId.length > 0
                        onClicked: settingsDialog.openFor(page.activeId)
                    }
                    IconBtn {
                        iconName: "trash"
                        tooltip: qsTr("Xoá khỏi danh sách đồng bộ")
                        danger: true
                        onClicked: confirmRemove.open()
                    }

                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 22
                        Layout.leftMargin: AuroraTheme.sp3
                        Layout.rightMargin: AuroraTheme.sp3
                        color: AuroraTheme.divider
                    }

                    RowLayout {
                        spacing: AuroraTheme.sp2
                        Aurora.FsIcon { name: "power"; sizePx: 14; color: AuroraTheme.ink2 }
                        Text {
                            text: qsTr("Bật đồng bộ")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 12
                            color: AuroraTheme.ink2
                        }
                        FsSwitch {
                            checked: page.hasVM && syncViewModel.activeEnabled
                            onToggled: function(c) {
                                if (page.hasVM) syncViewModel.setEnabled(page.activeId, c);
                            }
                        }
                    }

                    RowLayout {
                        spacing: AuroraTheme.sp2
                        Aurora.FsIcon { name: "trash"; sizePx: 14; color: AuroraTheme.ink2 }
                        Text {
                            text: qsTr("Xoá local sau khi tải")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 12
                            color: AuroraTheme.ink2
                        }
                        FsSwitch {
                            checked: page.hasVM && syncViewModel.deleteAfterUpload
                            onToggled: function(c) {
                                if (page.hasVM) syncViewModel.setDeleteAfterUpload(page.activeId, c);
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // File list card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Header: local → cloud destination
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        Layout.leftMargin: AuroraTheme.sp5
                        Layout.rightMargin: AuroraTheme.sp5
                        spacing: AuroraTheme.sp3

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Text {
                                Layout.fillWidth: true
                                text: page.hasVM ? syncViewModel.activeLocalPath : ""
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: AuroraTheme.ink1
                                elide: Text.ElideMiddle
                            }
                            Text {
                                Layout.fillWidth: true
                                text: page.hasVM
                                    ? qsTr("→ /%1 · %2 MB/s · %3 file")
                                        .arg(syncViewModel.activeFshareFolderName)
                                        .arg((syncViewModel.speedLimitKBps / 1024).toFixed(0))
                                        .arg(page.activeFileCount)
                                    : ""
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 11
                                color: AuroraTheme.ink4
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: qsTr("Trạng thái")
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            font.letterSpacing: 1.4
                            font.capitalization: Font.AllUppercase
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink4
                            Layout.preferredWidth: 90
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: AuroraTheme.divider }

                    ListView {
                        id: filesList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !(page.hasVM && page.activeFileCount === 0)
                        clip: true
                        model: page.hasVM ? syncViewModel.files : null
                        spacing: 0

                        ScrollBar.vertical: ScrollBar {}

                        delegate: Rectangle {
                            id: rowDelegate
                            required property string relPath
                            required property string size
                            required property string mtime
                            required property int    fileState
                            required property string stateText
                            required property string linkcode
                            required property string uploadedAt
                            required property string errorMessage

                            width: filesList.width
                            implicitHeight: 54
                            color: rowMa.containsMouse
                                ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.04)
                                : "transparent"
                            Behavior on color { enabled: !AuroraTheme.reduceMotion
                                ColorAnimation { duration: AuroraTheme.durFast } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: AuroraTheme.sp5
                                anchors.rightMargin: AuroraTheme.sp5
                                spacing: AuroraTheme.sp3

                                Aurora.FsIcon {
                                    name: {
                                        switch (rowDelegate.fileState) {
                                        case 2: return "check"     // Synced
                                        case 1: return "sync"      // Uploading
                                        case 3: return "x"         // Failed
                                        case 4: return "folder"    // Missing
                                        default: return "pause"    // Pending
                                        }
                                    }
                                    sizePx: 18
                                    color: {
                                        switch (rowDelegate.fileState) {
                                        case 2: return AuroraTheme.success
                                        case 3: return AuroraTheme.danger
                                        case 4: return AuroraTheme.ink4
                                        case 1: return AuroraTheme.accent
                                        default: return AuroraTheme.ink3
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Text {
                                        Layout.fillWidth: true
                                        text: rowDelegate.relPath
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 13
                                        color: AuroraTheme.ink1
                                        elide: Text.ElideMiddle
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: {
                                            if (rowDelegate.errorMessage.length > 0) return rowDelegate.errorMessage;
                                            if (rowDelegate.uploadedAt.length > 0)
                                                return qsTr("%1 · %2 · đã sao lưu %3")
                                                    .arg(rowDelegate.size).arg(rowDelegate.mtime).arg(rowDelegate.uploadedAt);
                                            return qsTr("%1 · %2").arg(rowDelegate.size).arg(rowDelegate.mtime);
                                        }
                                        font.family: AuroraTheme.fontMono
                                        font.pixelSize: 11
                                        color: rowDelegate.errorMessage.length > 0 ? AuroraTheme.danger : AuroraTheme.ink4
                                        elide: Text.ElideRight
                                    }
                                }

                                // State badge
                                Rectangle {
                                    Layout.preferredHeight: 22
                                    implicitWidth: stTxt.implicitWidth + 14
                                    radius: 11
                                    color: {
                                        switch (rowDelegate.fileState) {
                                        case 2: return Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.12);
                                        case 3: return AuroraTheme.dangerSoft;
                                        case 1: return AuroraTheme.accentTint15;
                                        default: return Qt.rgba(AuroraTheme.ink4.r, AuroraTheme.ink4.g, AuroraTheme.ink4.b, 0.08);
                                        }
                                    }
                                    Text {
                                        id: stTxt
                                        anchors.centerIn: parent
                                        text: rowDelegate.stateText
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 11
                                        font.weight: Font.Bold
                                        color: {
                                            switch (rowDelegate.fileState) {
                                            case 2: return AuroraTheme.success
                                            case 3: return AuroraTheme.danger
                                            case 1: return AuroraTheme.accent
                                            default: return AuroraTheme.ink3
                                            }
                                        }
                                    }
                                }

                                IconBtn {
                                    visible: rowDelegate.linkcode.length > 0
                                    iconName: "copy"
                                    tooltip: qsTr("Sao chép link Fshare")
                                    onClicked: if (page.hasVM) syncViewModel.copyLinkFor(page.activeId, rowDelegate.relPath)
                                }
                                IconBtn {
                                    visible: rowDelegate.linkcode.length > 0
                                    iconName: "external-link"
                                    tooltip: qsTr("Mở link Fshare")
                                    onClicked: if (page.hasVM) syncViewModel.openFshareLink(page.activeId, rowDelegate.relPath)
                                }
                            }

                            MouseArea {
                                id: rowMa
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.NoButton
                            }
                        }
                    }

                    // Empty state inside list
                    FsEmptyState {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: page.hasVM && page.activeFileCount === 0 && page.folderCount > 0
                        icon: "↻"
                        title: qsTr("Chưa có file nào được theo dõi")
                        description: qsTr("Bấm \"Đồng bộ ngay\" để quét thư mục và bắt đầu sao lưu.")
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        //  RECENT ACTIVITY (last 50, newest first)
        // ═════════════════════════════════════════════════
        ColumnLayout {
            Layout.fillWidth: true
            visible: page.folderCount > 0
            spacing: AuroraTheme.sp2

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AuroraTheme.sp2
                Layout.rightMargin: AuroraTheme.sp2
                spacing: AuroraTheme.sp3

                Text {
                    text: "━ " + qsTr("Hoạt động gần đây") + " · "
                          + (page.hasVM ? syncViewModel.activity.count : 0)
                    color: AuroraTheme.ink4
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 11
                    font.letterSpacing: 2.0
                    font.capitalization: Font.AllUppercase
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                // Subtle ghost link to wipe — only visible when there's
                // something to clear so the affordance isn't noise.
                Text {
                    visible: page.hasVM && syncViewModel.activity.count > 0
                    text: qsTr("Xoá lịch sử")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: clearMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink4
                    Behavior on color { enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast } }
                    MouseArea {
                        id: clearMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (page.hasVM) syncViewModel.clearActivity()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(activityList.contentHeight + AuroraTheme.sp2 * 2,
                                                  300)
                radius: AuroraTheme.radiusLg
                color: AuroraTheme.panel
                border.width: 1
                border.color: AuroraTheme.border
                clip: true
                visible: page.hasVM && syncViewModel.activity.count > 0

                ListView {
                    id: activityList
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp2
                    model: page.hasVM ? syncViewModel.activity : null
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: RowLayout {
                        // Each row is 36px tall to match Aurora list-row token
                        // (AuroraTheme.heightListRow=44 is too tall for a
                        // dense feed; this matches Fshare DownloadPage's
                        // history list).
                        width: ListView.view ? ListView.view.width : 0
                        spacing: AuroraTheme.sp2

                        required property string folderLabel
                        required property string relPath
                        required property int    sizeBytes
                        required property int    kind
                        required property string kindText
                        required property string message
                        required property string atAgo

                        // Status glyph in semantic colour — single character
                        // keeps the row dense; tooltip from the kindText
                        // gives the textual label on hover.
                        readonly property color glyphColor: {
                            switch (kind) {
                            case 0: return AuroraTheme.success;  // Uploaded
                            case 1: return AuroraTheme.danger;   // Failed
                            case 2: return AuroraTheme.success;  // DeletedLocal
                            case 3: return AuroraTheme.accent;   // FolderAdded
                            case 4: return AuroraTheme.warn;     // FolderRemoved
                            default: return AuroraTheme.ink3;
                            }
                        }
                        readonly property string glyph: {
                            switch (kind) {
                            case 0: return "✓";
                            case 1: return "✕";
                            case 2: return "↺";
                            case 3: return "＋";
                            case 4: return "－";
                            default: return "·";
                            }
                        }

                        Text {
                            Layout.preferredWidth: 16
                            horizontalAlignment: Text.AlignHCenter
                            text: glyph
                            color: glyphColor
                            font.pixelSize: 12
                            font.weight: Font.Bold
                        }
                        Text {
                            // Primary line — file name (or folder action),
                            // truncated middle so extension stays visible.
                            Layout.fillWidth: true
                            text: relPath.length > 0
                                ? relPath
                                : (kindText + ": " + folderLabel)
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 12
                            color: AuroraTheme.ink1
                            elide: Text.ElideMiddle
                        }
                        Text {
                            // Secondary line — folder label (only for
                            // file-scoped entries).  Caption tone so it
                            // recedes behind the primary text.
                            visible: relPath.length > 0
                            text: folderLabel
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 11
                            color: AuroraTheme.ink4
                            elide: Text.ElideRight
                            Layout.preferredWidth: 120
                        }
                        Text {
                            // Time-ago column — fixed width so the column
                            // visually aligns across rows.
                            Layout.preferredWidth: 72
                            horizontalAlignment: Text.AlignRight
                            text: atAgo
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            color: AuroraTheme.ink4
                        }
                    }
                }
            }
        }
    }

    // ── Confirm dialogs ──────────────────────────────────
    FsConfirmDialog {
        id: confirmRemove
        title: qsTr("Xoá khỏi danh sách đồng bộ?")
        message: qsTr("File đã tải lên Fshare sẽ KHÔNG bị xoá. FsNext chỉ ngừng theo dõi thư mục này.")
        primaryLabel: qsTr("Xoá khỏi danh sách")
        dangerAction: true
        onConfirmed: if (page.hasVM) syncViewModel.removeFolder(page.activeId)
    }

    FsConfirmDialog {
        id: confirmScan
        title: qsTr("Đồng bộ ngay?")
        message: qsTr("FsNext sẽ quét toàn bộ thư mục và tải mọi file mới hoặc đã sửa đổi lên Fshare.")
        primaryLabel: qsTr("Bắt đầu quét")
        dangerAction: false
        onConfirmed: if (page.hasVM) syncViewModel.scanNow(page.activeId)
    }

    FsConfirmDialog {
        id: confirmReset
        title: qsTr("Xoá cache đồng bộ?")
        message: qsTr("Toàn bộ trạng thái local của thư mục này sẽ bị xoá và FsNext sẽ quét lại từ đầu. File trên Fshare KHÔNG bị ảnh hưởng.")
        primaryLabel: qsTr("Xoá cache")
        dangerAction: true
        onConfirmed: if (page.hasVM) syncViewModel.resetFolderCache(page.activeId)
    }
}
