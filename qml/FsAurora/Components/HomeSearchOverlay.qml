// SPDX-License-Identifier: Proprietary
// HomeSearchOverlay — Spotlight-style floating results panel anchored
// below the homepage search pill.
//
// State binding contract (homeSearchViewModel context property):
//   state              — current classification (we only show when Keyword=5)
//   isSearching        — true while a debounced API call is in flight
//   resultsModel       — FileListModel populated by FshareApi::searchFiles
//   noResultsHit       — true when the API responded with 0 items
//   resultsForKeyword  — the keyword the model currently reflects (for header)
//
// Keyboard: ↑/↓ to move highlight, Enter to open the file detail surface for
// the highlighted row. The TextField inside the search pill keeps focus —
// this overlay is purely presentational; key events are forwarded from
// HomePage via the Keys.onPressed handler attached to the TextField.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Fshare.Components 1.0
import Fshare.Utils 1.0
import FsAurora.Theme 1.0

Item {
    id: root

    property var vm: homeSearchViewModel
    // The overlay is mounted by HomePage and laid out by its parent — we
    // expose visibility logic via a derived property instead of forcing
    // anchors here so HomePage can position us under the search pill.
    readonly property bool _hasModel: vm && vm.resultsModel
    readonly property int  _count:    _hasModel ? vm.resultsModel.count : 0
    readonly property bool _showLoading:  vm && vm.isSearching && _count === 0
    readonly property bool _showEmpty:    vm && !vm.isSearching && vm.noResultsHit
    readonly property bool _showResults:  _count > 0

    // True when the panel should be on-screen. HomePage already gates this
    // by Keyword-state; the additional guards here just hide the panel
    // during the brief intervals when there's literally nothing to show.
    readonly property bool active: _showLoading || _showEmpty || _showResults

    // Emitted when the user activates a row — HomePage routes this to
    // remoteShareViewModel.openFile() and shows FileDetailSheet, reusing
    // the Phase-2 surface.
    signal fileActivated(string linkcode, string name)

    // Keyboard-driven highlight. -1 = none (mouse-only mode).
    property int highlightedIndex: -1
    function moveHighlight(delta) {
        if (_count === 0) { highlightedIndex = -1; return; }
        if (highlightedIndex < 0) highlightedIndex = delta > 0 ? 0 : _count - 1;
        else                       highlightedIndex = Math.max(0, Math.min(_count - 1, highlightedIndex + delta));
        // Keep the row in view.
        if (highlightedIndex >= 0)
            listView.positionViewAtIndex(highlightedIndex, ListView.Contain);
    }
    function activateHighlighted() {
        if (highlightedIndex < 0 || highlightedIndex >= _count) return;
        const item = vm.resultsModel.data(vm.resultsModel.index(highlightedIndex, 0), Qt.UserRole + 2);   // NameRole
        const lc   = vm.resultsModel.data(vm.resultsModel.index(highlightedIndex, 0), Qt.UserRole + 1);   // LinkcodeRole
        if (lc && lc.length > 0) root.fileActivated(lc, item || "");
    }

    // Reset highlight whenever the keyword changes.
    Connections {
        target: vm
        function onSearchProgressChanged() { root.highlightedIndex = -1; }
    }

    // Visual scaffolding ─────────────────────────────────────────────────
    width: parent ? parent.width : 480
    height: shellCol.implicitHeight + AuroraTheme.sp3 * 2
    visible: opacity > 0.01
    opacity: active ? 1.0 : 0.0
    Behavior on opacity { enabled: !AuroraTheme.reduceMotion
        NumberAnimation { duration: AuroraTheme.durFast } }

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: AuroraTheme.radiusLg
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border
        // Drop shadow simulated with a wider, faded duplicate behind. Qt 6
        // doesn't ship a built-in drop-shadow effect in QtQuick.Controls;
        // a single offset background is cheap and visually sufficient.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            z: -1
            radius: parent.radius + 1
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(0, 0, 0, 0.06)
        }

        ColumnLayout {
            id: shellCol
            anchors.fill: parent
            anchors.margins: AuroraTheme.sp3
            spacing: AuroraTheme.sp2

            // Header — what we're showing
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                Text {
                    text: {
                        if (root._showLoading) return qsTr("Đang tìm…");
                        if (root._showEmpty)   return qsTr("Không có kết quả");
                        return qsTr("Kết quả tìm kiếm");
                    }
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    font.letterSpacing: 1.4
                    font.capitalization: Font.AllUppercase
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink4
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: root._showResults
                    text: qsTr("%1 mục").arg(root._count)
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 10
                    color: AuroraTheme.ink4
                }
            }

            // ── Loading skeleton ──────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                visible: root._showLoading
                spacing: 6
                Repeater {
                    model: 3
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: AuroraTheme.radiusMd
                        color: AuroraTheme.divider
                        opacity: 0.5
                        // Subtle shimmer — single-pulse loop is enough; full
                        // gradient sweep would be overkill for a 250-500ms
                        // typical wait window.
                        SequentialAnimation on opacity {
                            running: parent.visible && !AuroraTheme.reduceMotion
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.35; to: 0.65; duration: 700 }
                            NumberAnimation { from: 0.65; to: 0.35; duration: 700 }
                        }
                    }
                }
            }

            // ── Empty state ──────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                visible: root._showEmpty
                spacing: 4
                Layout.topMargin: 8
                Layout.bottomMargin: 8

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: vm && vm.resultsForKeyword
                          ? qsTr("Không tìm thấy \"%1\"").arg(vm.resultsForKeyword)
                          : qsTr("Không tìm thấy kết quả")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink2
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Thử từ khoá khác hoặc dán link Fshare.")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: 11
                    color: AuroraTheme.ink4
                }
            }

            // ── Results list ─────────────────────────────────
            ListView {
                id: listView
                visible: root._showResults
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(360, _count * 52 + 4)
                clip: true
                spacing: 2
                interactive: true
                model: root._hasModel ? vm.resultsModel : null

                // Infinite-scroll: when the user is near the bottom and
                // the VM says there are more pages, kick off the next
                // fetch. The VM's request-sequence guard prevents
                // duplicates when the user is also editing the query.
                onContentYChanged: {
                    if (!vm || !vm.hasMorePages || vm.isSearching) return;
                    const dist = contentHeight - (contentY + height);
                    if (dist < height * 0.5) vm.loadMore();
                }

                delegate: Rectangle {
                    id: rowItem
                    width: listView.width
                    height: 48
                    radius: AuroraTheme.radiusMd

                    required property int    index
                    required property string linkcode
                    required property string name
                    required property var    size
                    required property bool   isFolder
                    required property string fileCategory

                    readonly property bool _highlight:
                        rowMa.containsMouse || root.highlightedIndex === index

                    color: _highlight ? AuroraTheme.accentTint10 : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion
                        ColorAnimation { duration: AuroraTheme.durFast } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: AuroraTheme.sp3
                        anchors.rightMargin: AuroraTheme.sp3
                        spacing: AuroraTheme.sp3

                        FsFileTypeIcon {
                            fileName: rowItem.name
                            isFolder: rowItem.isFolder
                            sizePx: 32
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: rowItem.name
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                color: AuroraTheme.ink1
                                elide: Text.ElideMiddle
                            }
                            Text {
                                text: rowItem.isFolder
                                      ? qsTr("Thư mục")
                                      : FsFormat.bytes(rowItem.size)
                                font.family: AuroraTheme.fontMono
                                font.pixelSize: 10
                                color: AuroraTheme.ink3
                            }
                        }

                        // Open / preview chevron — same affordance as the
                        // FavoritesPage row clicks.
                        FsIcon {
                            name: "chevron-right"
                            sizePx: 14
                            color: rowItem._highlight ? AuroraTheme.accent : AuroraTheme.ink4
                        }
                    }

                    MouseArea {
                        id: rowMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Folders in the overlay aren't browsable in
                            // Phase 3 (the share-folder browser is built for
                            // arbitrary URLs, not in-account folders);
                            // surface them as detail too. The user can still
                            // click "Open on fshare.vn" from the sheet.
                            root.fileActivated(rowItem.linkcode, rowItem.name);
                        }
                    }
                }
            }
        }
    }
}
