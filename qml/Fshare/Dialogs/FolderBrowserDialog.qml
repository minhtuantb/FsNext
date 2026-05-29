// SPDX-License-Identifier: Proprietary
// FolderBrowserDialog — Multi-level Fshare share-folder browser.
//
// Opened from HomePage when the user pastes a https://www.fshare.vn/folder/…
// URL into the search box. State is fully owned by `remoteShareViewModel`
// (a context property registered by AppContext) so the QML side only deals
// with presentation: breadcrumb, list rows, pagination scroll, row actions.
//
// Per product spec: folder URLs themselves are never password-protected;
// individual files inside can be, and password entry happens inline on each
// per-row action (download / open detail).

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0

Item {
    id: root

    // Auto-bound to the registered context property; falling back to null
    // makes standalone QML previews degrade gracefully.
    property var vm: remoteShareViewModel

    signal closed()
    signal fileRowOpened(string linkcode, var fileRow)   // routed back to host → FileDetailSheet

    function open()  { root.visible = true; root.forceActiveFocus(); }
    function close() {
        root.visible = false;
        // vm.close() is the host's responsibility (Main.qml). See
        // FileDetailSheet.close() for the same rationale — keeps the
        // folder→file→folder roundtrip possible.
        root.closed();
    }

    anchors.fill: parent
    visible: false
    z: 1000

    focus: visible
    activeFocusOnTab: false
    Keys.onEscapePressed: function(event) { root.close(); event.accepted = true; }

    Accessible.role: Accessible.Dialog
    Accessible.name: qsTr("Duyệt folder chia sẻ")

    // ── Backdrop ───────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, root.visible ? 0.35 : 0)
        Behavior on color { enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast } }
        MouseArea { anchors.fill: parent; onClicked: root.close() }
    }

    // ── Dialog body — large modal so the list has room to breathe ─────────
    Rectangle {
        id: dialogBody
        width: Math.min(840, parent.width  - 64)
        height: Math.min(640, parent.height - 64)
        anchors.centerIn: parent
        radius: 16
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border
        scale:   root.visible ? 1.0  : 0.95
        opacity: root.visible ? 1.0  : 0.0
        Behavior on scale   { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durSlow; easing.type: Easing.OutBack } }
        Behavior on opacity { enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durFast } }
        MouseArea { anchors.fill: parent }    // swallow clicks so backdrop MA doesn't fire

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Header ─────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.rightMargin: AuroraTheme.sp4
                    spacing: AuroraTheme.sp3

                    Rectangle {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        radius: AuroraTheme.radiusMd
                        color: AuroraTheme.accentTint10
                        FsIcon {
                            anchors.centerIn: parent
                            name: "folder-open"; sizePx: 18
                            color: AuroraTheme.accent
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: qsTr("Folder chia sẻ")
                            font.family: AuroraTheme.fontMono
                            font.pixelSize: 10
                            font.letterSpacing: 1.6
                            font.capitalization: Font.AllUppercase
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink4
                        }
                        Text {
                            Layout.fillWidth: true
                            text: vm && vm.currentFolderName.length > 0
                                  ? vm.currentFolderName
                                  : qsTr("Đang tải…")
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                            color: AuroraTheme.ink1
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: AuroraTheme.radiusSm
                        color: closeMa.containsMouse ? AuroraTheme.accentSoft : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "✕"; font.pixelSize: 16
                            color: closeMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
                        }
                        MouseArea {
                            id: closeMa; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
            }

            // ── Breadcrumb + back button ───────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.rightMargin: AuroraTheme.sp4
                    spacing: AuroraTheme.sp3

                    // Back chip — disabled at root level (canGoBack==false)
                    Rectangle {
                        Layout.preferredWidth: backRow.implicitWidth + AuroraTheme.sp3 * 2
                        Layout.preferredHeight: 28
                        radius: AuroraTheme.radiusPill
                        color: backMa.containsMouse && (vm && vm.canGoBack)
                               ? AuroraTheme.accentTint10
                               : AuroraTheme.panel
                        border.width: 1
                        border.color: AuroraTheme.border
                        opacity: vm && vm.canGoBack ? 1.0 : 0.4
                        RowLayout {
                            id: backRow
                            anchors.centerIn: parent
                            spacing: 4
                            FsIcon { name: "arrow-left"; sizePx: 12; color: AuroraTheme.ink2 }
                            Text {
                                text: qsTr("Quay lại")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 11
                                color: AuroraTheme.ink2
                            }
                        }
                        MouseArea {
                            id: backMa; anchors.fill: parent
                            hoverEnabled: true
                            enabled: vm && vm.canGoBack
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: if (vm) vm.navigateBack()
                        }
                    }

                    FsBreadcrumb {
                        Layout.fillWidth: true
                        // breadcrumbs is a QVariantList<{linkcode,name}> — map
                        // to the {label,linkcode} shape the component expects.
                        segments: {
                            const segs = (vm && vm.breadcrumbs) ? vm.breadcrumbs : [];
                            const out = [];
                            for (let i = 0; i < segs.length; ++i) {
                                out.push({ label: segs[i].name || segs[i].linkcode,
                                           linkcode: segs[i].linkcode });
                            }
                            return out;
                        }
                        onSegmentClicked: function(index, _seg) {
                            if (!vm) return;
                            const target = vm.breadcrumbs.length - 1 - index;
                            for (let i = 0; i < target; ++i) vm.navigateBack();
                        }
                    }

                    Text {
                        text: vm ? qsTr("%1 mục").arg(vm.totalCount) : ""
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 11
                        color: AuroraTheme.ink4
                    }

                    // "Select all files" chip — small, appears whenever
                    // we have at least one file row in view (folders only
                    // would make the chip useless because selectAllFiles
                    // skips them).
                    Rectangle {
                        Layout.preferredWidth: selAllRow.implicitWidth + AuroraTheme.sp3 * 2
                        Layout.preferredHeight: 24
                        radius: AuroraTheme.radiusPill
                        color: selAllMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                        border.width: 1
                        border.color: AuroraTheme.border
                        visible: vm && vm.totalCount > 0
                        RowLayout {
                            id: selAllRow
                            anchors.centerIn: parent
                            spacing: 4
                            FsIcon { name: "check"; sizePx: 11; color: AuroraTheme.ink2 }
                            Text {
                                text: qsTr("Chọn tất cả file")
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 11
                                color: AuroraTheme.ink2
                            }
                        }
                        MouseArea {
                            id: selAllMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (vm) vm.selectAllFiles()
                        }
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
            }

            // ── Selection toolbar (visible only when 1+ files selected) ─
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: vm && vm.hasSelection ? 40 : 0
                visible: vm && vm.hasSelection
                color: AuroraTheme.accentTint10

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.rightMargin: AuroraTheme.sp4
                    spacing: AuroraTheme.sp3

                    FsIcon { name: "check"; sizePx: 14; color: AuroraTheme.accent }
                    Text {
                        Layout.fillWidth: true
                        text: vm ? qsTr("Đã chọn %1 file").arg(vm.selectedCount) : ""
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: AuroraTheme.accent
                    }
                    IconBtn { icon: "x";        tooltip: qsTr("Bỏ chọn")
                        onActivated: if (vm) vm.clearSelection()
                    }
                    IconBtn { icon: "download"; tooltip: qsTr("Tải các file đã chọn")
                        onActivated: if (vm) vm.downloadSelected("")
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
            }

            // ── File / folder list ─────────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: listView
                    anchors.fill: parent
                    anchors.margins: AuroraTheme.sp3
                    clip: true
                    spacing: 2
                    model: vm ? vm.fileListModel : null

                    // Infinite-scroll: when the user is within 1.5 viewport
                    // heights of the bottom, request the next page. The VM
                    // guards against re-entry while a fetch is in flight.
                    onContentYChanged: {
                        if (!vm || !vm.hasMorePages || vm.isLoading) return;
                        const dist = contentHeight - (contentY + height);
                        if (dist < height * 0.5) vm.loadMore();
                    }

                    delegate: Rectangle {
                        id: rowItem
                        width: listView.width
                        height: 56
                        radius: AuroraTheme.radiusMd
                        color: rowMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                        Behavior on color { enabled: !AuroraTheme.reduceMotion
                            ColorAnimation { duration: AuroraTheme.durFast } }

                        required property string linkcode
                        required property string name
                        required property var    size
                        required property bool   isFolder
                        required property bool   hasPassword
                        required property string fileCategory

                        // Live binding to the VM's selection set. selectionChanged
                        // fires whenever the set membership changes; we re-query
                        // isSelected for THIS row. We don't bind to vm.selectedCount
                        // (which would fire for every row on every change) — checking
                        // isSelected per-row scales linearly with visible rows only.
                        property bool _selected: vm ? vm.isSelected(rowItem.linkcode) : false
                        Connections {
                            target: vm
                            function onSelectionChanged() {
                                rowItem._selected = vm.isSelected(rowItem.linkcode);
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: AuroraTheme.sp3
                            anchors.rightMargin: AuroraTheme.sp3
                            spacing: AuroraTheme.sp3

                            // Per-row checkbox — only meaningful for files
                            // (folders aren't multi-selectable). A spacer
                            // replaces it on folder rows to keep alignment.
                            Rectangle {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                radius: 4
                                border.width: 1
                                border.color: rowItem._selected
                                              ? AuroraTheme.accent
                                              : AuroraTheme.borderStrong
                                color: rowItem._selected
                                       ? AuroraTheme.accent
                                       : "transparent"
                                visible: !rowItem.isFolder
                                FsIcon {
                                    anchors.centerIn: parent
                                    visible: rowItem._selected
                                    name: "check"; sizePx: 10
                                    color: "#FFFFFF"
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (vm) vm.toggleSelection(rowItem.linkcode)
                                }
                            }
                            Item {
                                visible: rowItem.isFolder
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                            }

                            FsFileTypeIcon {
                                fileName: rowItem.name
                                isFolder: rowItem.isFolder
                                sizePx: 36
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Text {
                                        Layout.fillWidth: true
                                        text: rowItem.name
                                        font.family: AuroraTheme.fontSans
                                        font.pixelSize: 13
                                        font.weight: Font.Medium
                                        color: AuroraTheme.ink1
                                        elide: Text.ElideMiddle
                                    }
                                    Rectangle {
                                        visible: rowItem.hasPassword
                                        Layout.preferredWidth: pwdLabel.implicitWidth + 10
                                        Layout.preferredHeight: 16
                                        radius: 8
                                        color: AuroraTheme.warnSoft
                                        Text {
                                            id: pwdLabel
                                            anchors.centerIn: parent
                                            text: "🔒 " + qsTr("Mật khẩu")
                                            font.pixelSize: 9
                                            font.weight: Font.DemiBold
                                            color: AuroraTheme.warn
                                        }
                                    }
                                }
                                Text {
                                    text: rowItem.isFolder
                                          ? qsTr("Thư mục")
                                          : FsFormat.bytes(rowItem.size)
                                    font.family: AuroraTheme.fontMono
                                    font.pixelSize: 11
                                    color: AuroraTheme.ink3
                                }
                            }

                            // Per-row actions — only relevant for files.
                            // Folders just navigate on click.
                            RowLayout {
                                spacing: 4
                                visible: !rowItem.isFolder

                                IconBtn { icon: "external-link"; tooltip: qsTr("Xem chi tiết")
                                    onActivated: {
                                        // Hand back to host so it can open the
                                        // FileDetailSheet (which also resets
                                        // VM mode — done at parent level).
                                        root.fileRowOpened(rowItem.linkcode, ({
                                            name: rowItem.name,
                                            size: rowItem.size,
                                            hasPassword: rowItem.hasPassword,
                                            fileCategory: rowItem.fileCategory
                                        }));
                                    }
                                }
                                IconBtn { icon: "download"; tooltip: qsTr("Tải về")
                                    onActivated: if (vm) vm.downloadFolderItem(rowItem.linkcode, false, "")
                                }
                                IconBtn { icon: "copy"; tooltip: qsTr("Sao chép link")
                                    onActivated: if (vm) vm.copyFolderItemLink(rowItem.linkcode, false)
                                }
                            }

                            // Download-folder button (only shown for folder rows)
                            IconBtn {
                                visible: rowItem.isFolder
                                icon: "download"; tooltip: qsTr("Tải cả folder")
                                onActivated: if (vm) vm.downloadFolderItem(rowItem.linkcode, true, "")
                            }
                        }

                        MouseArea {
                            id: rowMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            propagateComposedEvents: false
                            onClicked: {
                                if (!vm) return;
                                if (rowItem.isFolder) {
                                    vm.navigateInto(rowItem.linkcode, rowItem.name);
                                } else {
                                    root.fileRowOpened(rowItem.linkcode, ({
                                        name: rowItem.name,
                                        size: rowItem.size,
                                        hasPassword: rowItem.hasPassword,
                                        fileCategory: rowItem.fileCategory
                                    }));
                                }
                            }
                        }
                    }

                    // Loading spinner — shown when fetching the first page
                    // (m_loading + empty model). Subsequent page-loads keep
                    // the existing rows visible while a small footer spinner
                    // would appear (skipped here to keep the surface lean).
                    Item {
                        anchors.centerIn: parent
                        visible: vm && vm.isLoading && (!vm.fileListModel || vm.fileListModel.count === 0)
                        width: 40; height: 40
                        FsSpinner { anchors.centerIn: parent; running: parent.visible }
                    }
                }

                // Empty state — only after a successful empty page (not
                // during the very first fetch).
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: vm && !vm.isLoading
                             && vm.fileListModel && vm.fileListModel.count === 0
                             && (vm.errorText || "").length === 0
                    spacing: AuroraTheme.sp3

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 56; height: 56; radius: 28
                        color: AuroraTheme.accentTint10
                        FsIcon { anchors.centerIn: parent; name: "folder"; sizePx: 24; color: AuroraTheme.accent }
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Folder trống")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink2
                    }
                }

                // Error state
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: vm && (vm.errorText || "").length > 0
                    spacing: AuroraTheme.sp3

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 56; height: 56; radius: 28
                        color: AuroraTheme.dangerSoft
                        Text { anchors.centerIn: parent; text: "⚠"; font.pixelSize: 22; color: AuroraTheme.danger }
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: vm ? vm.errorText : ""
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 13
                        color: AuroraTheme.ink2
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.maximumWidth: 360
                    }
                }
            }
        }
    }

    // ── Compact per-row icon-button component ──────────────────
    component IconBtn: Rectangle {
        id: ib
        property string icon: ""
        property string tooltip: ""
        signal activated()

        Layout.preferredWidth: 28
        Layout.preferredHeight: 28
        radius: AuroraTheme.radiusSm
        color: ibMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
        border.width: 1
        border.color: ibMa.containsMouse ? AuroraTheme.accent : AuroraTheme.border
        FsIcon {
            anchors.centerIn: parent
            name: ib.icon; sizePx: 12
            color: ibMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2
        }
        MouseArea {
            id: ibMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: ib.activated()
            ToolTip.visible: containsMouse && ib.tooltip.length > 0
            ToolTip.text: ib.tooltip
            ToolTip.delay: 400
        }
    }
}
