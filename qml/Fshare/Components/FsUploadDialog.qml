// SPDX-License-Identifier: Proprietary
// FsUploadDialog — Drag-drop + file picker upload dialog.

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import FsAurora.Theme 1.0
FsDialog {
    id: root

    // The authoritative holder for the user's batch — typically
    // uploadStagingViewModel from AppContext. When set, pendingFiles binds to
    // its stagedFiles, and add/remove/clear/commit all route through it so
    // state survives page navigation and app restart. Falsy = legacy
    // self-contained mode (kept so this component isn't tied to one caller).
    property var stagingModel: null

    property string targetFolder: "/"
    // Files currently in the dialog list. When stagingModel is provided this
    // shadows its stagedFiles via the binding below — writes from the UI go
    // through the model methods, not by reassigning this.
    property var    pendingFiles: stagingModel ? stagingModel.stagedFiles : []
    property string password:     ""
    property bool   secured:      false

    // Preferred: a FolderPickerModel (C++) exposing precomputed `labels`
    // and `ids` QStringLists. Pass `fileManagerViewModel.folderPickerModel`.
    // The picker rebuilds itself lazily when the folder tree changes, and
    // its digest guard makes no-op re-emissions effectively free — so QML
    // no longer has to iterate a QVariantList of thousands of folders on
    // every dialog open.
    property var    folderPickerModel: null

    // Fallback (deprecated): live folder tree as a QVariantList of maps.
    // Still honoured when folderPickerModel isn't wired in, so existing
    // callers keep working while we migrate. Empty → "/" root-only.
    property var    folderItems: []

    // Derived label+id pair used by the FsSelect below. When a
    // folderPickerModel is provided we pass straight through to its
    // precomputed arrays (no iteration). Otherwise we fall back to the
    // legacy JS build path over folderItems, which remains gated on
    // `root.visible` to keep it off the critical open path.
    readonly property var _folderOptions: {
        if (folderPickerModel && folderPickerModel.labels)
            return { labels: folderPickerModel.labels, ids: folderPickerModel.ids };

        const labels = [qsTr("/ (Thư mục gốc)")];
        const ids    = ["/"];
        if (!root.visible || !root.folderItems) return { labels, ids };
        for (let i = 0; i < root.folderItems.length; ++i) {
            const f = root.folderItems[i];
            if (!f || f.isFolder === false) continue;
            const path = (f.path || f.name || "").toString();
            if (path.length === 0) continue;
            const depth = Math.max(0, (path.match(/\//g) || []).length - 1);
            const indent = "    ".repeat(depth);
            labels.push(indent + (f.name || path));
            ids.push(f.linkcode || path);
        }
        return { labels, ids };
    }

    signal uploadStarted(var files, string folder)

    title: qsTr("Tải lên file")
    dialogWidth: 500
    // Don't discard the staged files/folder on a stray click outside the box —
    // the user closes via Hủy / ✕ / Esc instead.
    closeOnOverlayClick: false

    // Hydrate local UI state from the staging model so a re-open of the
    // dialog (page nav round-trip, post-restart "Tiếp tục" banner) shows the
    // folder + password + privacy the user had set, instead of resetting to
    // defaults like the old self-contained version did.
    onOpened: {
        if (root.stagingModel) {
            root.targetFolder = root.stagingModel.targetFolder || "/";
            root.password     = root.stagingModel.password || "";
            root.secured      = root.stagingModel.secured;
            // Push the hydrated password into the text field — the field's
            // `text` is only one-way bound (onTextChanged → root.password) so
            // setting root.password doesn't update what the user sees. Setting
            // text directly also breaks the binding, but that's fine here
            // because subsequent edits flow through onTextChanged.
            passwordField.text = root.password;
            // Reflect the hydrated folder in the dropdown.
            const ids = root._folderOptions.ids;
            const idx = ids.indexOf(root.targetFolder);
            folderSel.currentIndex = (idx >= 0) ? idx : 0;
            // The dialog has just told the VM "I'm showing you", so swallow the
            // showRequested flag — a subsequent passive nav back to Upload
            // shouldn't re-pop the dialog.
            if (root.stagingModel.acknowledgeShow) root.stagingModel.acknowledgeShow();
        } else {
            folderSel.currentIndex = 0;
            root.targetFolder = "/";
        }
        // Park focus on the target-folder selector — first decision the user
        // has to make. Deferred so the dialog is fully on-screen first.
        Qt.callLater(() => folderSel.forceActiveFocus());
    }

    FileDialog {
        id: _filePicker
        title:     qsTr("Chọn file để tải lên")
        fileMode:  FileDialog.OpenFiles
        onAccepted: root._addFiles(selectedFiles)
    }

    // Single entry point for adding files via any picker (drop, "chọn từ máy",
    // OS file dialog). When wired to a stagingModel, the model does the dedupe
    // and stat — the dialog just hands paths over. The legacy local-list path
    // (no model) is kept for back-compat callers that haven't migrated yet.
    function _addFiles(urls) {
        if (root.stagingModel) {
            const arr = [];
            for (const u of urls) arr.push(u.toString());
            root.stagingModel.addFiles(arr);
            return;
        }
        const newFiles = [];
        for (const u of urls) {
            const fullUrl = u.toString();
            const decoded = decodeURIComponent(
                fullUrl.replace(/^file:\/\/\//, "").replace(/^file:\/\//, ""));
            const name = decoded.split("/").pop().split("\\").pop();
            const ext  = name.split(".").pop().toLowerCase();
            if (!root.pendingFiles.find(f => f.path === fullUrl))
                newFiles.push({ path: fullUrl, name, ext, size: 0 });
        }
        root.pendingFiles = root.pendingFiles.concat(newFiles);
    }

    content: [
        Item {
            id: body
            width: root.dialogWidth
            height: bodyCol.implicitHeight + AuroraTheme.sp8

            ColumnLayout {
                id: bodyCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                anchors.topMargin: AuroraTheme.sp4
                spacing: AuroraTheme.sp3

                // Drop zone
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: AuroraTheme.radiusMd
                    color:  _dropArea.containsDrag ? AuroraTheme.accentSoft : AuroraTheme.divider
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    border.color: _dropArea.containsDrag ? AuroraTheme.accent
                                  : _dropHov.containsMouse ? AuroraTheme.borderStrong
                                  : AuroraTheme.border
                    border.width: _dropArea.containsDrag ? 2 : 1
                    Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                    HoverHandler { id: _dropHov }

                    DropArea {
                        id: _dropArea
                        anchors.fill: parent
                        onDropped: (drop) => {
                            if (drop.hasUrls) root._addFiles(drop.urls);
                        }
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: AuroraTheme.sp2

                        FsIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            name:   "upload"
                            sizePx: 32
                            color:  _dropArea.containsDrag ? AuroraTheme.accent : AuroraTheme.ink3
                        }
                        Text {
                            text: qsTr("Kéo thả file vào đây")
                            color: _dropArea.containsDrag ? AuroraTheme.accent : AuroraTheme.ink2
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 4
                            Text {
                                text: qsTr("hoặc")
                                color: AuroraTheme.ink3
                                font.pixelSize: 12
                            }
                            Text {
                                text: qsTr("chọn từ máy tính")
                                color: AuroraTheme.accent
                                font.pixelSize: 12
                                font.underline: true
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: _filePicker.open()
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        z: -1
                        onClicked: _filePicker.open()
                    }
                }

                // Pending files
                ColumnLayout {
                    visible: root.pendingFiles.length > 0
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp1

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("%1 file được chọn").arg(root.pendingFiles.length)
                            color: AuroraTheme.ink2
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 11
                            font.weight: Font.Medium
                        }
                        Text {
                            text: qsTr("Xoá tất cả")
                            color: AuroraTheme.accent
                            font.pixelSize: 11
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                // Only wipe files — keep folder/password/secured
                                // so the user doesn't have to reconfigure after
                                // picking a fresh batch.
                                onClicked: {
                                    if (root.stagingModel) root.stagingModel.clearFiles();
                                    else root.pendingFiles = [];
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(root.pendingFiles.length * 36 + 8, 144)
                        radius: AuroraTheme.radiusSm
                        color: AuroraTheme.divider
                        border.color: AuroraTheme.divider
                        border.width: 1
                        clip: true

                        ListView {
                            anchors.fill: parent
                            anchors.margins: 4
                            model: root.pendingFiles
                            spacing: 2
                            clip: true

                            delegate: Item {
                                width: ListView.view ? ListView.view.width : 400
                                height: 32

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    spacing: AuroraTheme.sp2

                                    FsFileTypeIcon {
                                        fileName: modelData.name
                                        sizePx:   24
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: AuroraTheme.ink1
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 11
                                        elide: Text.ElideMiddle
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 20
                                        Layout.preferredHeight: 20
                                        radius: 4
                                        color: rmMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                                        FsIcon {
                                            anchors.centerIn: parent
                                            name:   "x"
                                            sizePx: 12
                                            color:  rmMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
                                        }
                                        MouseArea {
                                            id: rmMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                if (root.stagingModel) {
                                                    root.stagingModel.removeFile(index);
                                                } else {
                                                    const arr = root.pendingFiles.slice();
                                                    arr.splice(index, 1);
                                                    root.pendingFiles = arr;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Target folder
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2
                    Text {
                        text: qsTr("Tải vào:")
                        color: AuroraTheme.ink3
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                    }
                    FsSelect {
                        id: folderSel
                        Layout.fillWidth: true
                        // Bound to the derived option list so any change to
                        // folderItems (e.g. background tree sync finishing)
                        // propagates without re-opening the dialog.
                        model: root._folderOptions.labels
                        currentIndex: 0
                        onActivated: (idx) => {
                            const ids = root._folderOptions.ids;
                            root.targetFolder = ids[idx] ?? "/";
                            if (root.stagingModel)
                                root.stagingModel.targetFolder = root.targetFolder;
                        }
                    }
                }

                // Password — text set on onOpened from the staging model so a
                // re-open of the dialog (page nav round-trip) doesn't blank it.
                FsTextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholder: qsTr("Mật khẩu bảo vệ file (để trống nếu không cần)")
                    echoMode: TextInput.Password
                    onTextChanged: {
                        root.password = text;
                        if (root.stagingModel) root.stagingModel.password = text;
                    }
                }

                // Secured toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AuroraTheme.sp2
                    FsSwitch {
                        checked: root.secured
                        onToggled: {
                            root.secured = checked;
                            if (root.stagingModel) root.stagingModel.secured = checked;
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("File riêng tư (chỉ bạn xem được)")
                        color: AuroraTheme.ink2
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 11
                    }
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

            FsButton {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Hủy"); variant: "ghost"
                onClicked: root.close()
            }
            FsButton {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Bắt đầu tải lên"); variant: "primary"
                enabled: root.pendingFiles.length > 0
                onClicked: {
                    if (root.stagingModel) {
                        // Final push of any UI state the model didn't get the
                        // live update for, then hand the batch to the upload
                        // VM and clear staging. The model handles dedupe,
                        // invalid-file skip, and persistence.
                        root.stagingModel.targetFolder = root.targetFolder;
                        root.stagingModel.password     = root.password;
                        root.stagingModel.secured      = root.secured;
                        root.stagingModel.commit();
                    } else {
                        // Legacy path for callers without a model.
                        root.uploadStarted(root.pendingFiles, root.targetFolder);
                    }
                    root.close();
                }
            }
        }
    ]
}
