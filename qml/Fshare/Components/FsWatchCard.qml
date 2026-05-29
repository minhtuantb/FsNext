// SPDX-License-Identifier: Proprietary
// FsWatchCard — Watched-folder card for Auto Upload feature

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora
import Fshare.Components 1.0

Rectangle {
    id: root

    // ── Data ──────────────────────────────────────────
    property string watchId: ""
    property string localPath: ""
    property string remotePath: ""
    property int status: 0  // 0=Idle 1=Scanning 2=Active 3=Paused 4=Error 5=Disabled

    // Progress (status = 2 Active)
    property real progress: 0.0
    property string currentFile: ""
    property int filesCompleted: 0
    property int filesTotal: 0
    property string remainingSize: ""
    property string speed: ""
    property string eta: ""

    // Stats (status = 0 Idle)
    property int syncedCount: 0
    property string totalSize: ""
    property string lastSyncTime: ""
    property int pendingCount: 0

    // Config display
    property bool deleteAfterUpload: false
    property bool watchSubfolders: true

    // Error (status = 4 Error)
    property string errorMessage: ""
    property int errorCount: 0

    // ── Signals ───────────────────────────────────────
    signal pauseClicked()
    signal resumeClicked()
    signal rescanClicked()
    signal retryClicked()
    signal settingsClicked()
    signal removeClicked()

    // ── Computed ──────────────────────────────────────
    readonly property bool isActive: status === 2
    readonly property bool isPaused: status === 3
    readonly property bool isError: status === 4

    // ── Visual ───────────────────────────────────────
    implicitHeight: content.implicitHeight + AuroraTheme.sp5 * 2
    radius: AuroraTheme.radiusLg
    color: isError
        ? Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.04)
        : AuroraTheme.panel
    border.width: 1
    border.color: {
        if (hoverMa.containsMouse) {
            if (isActive) return AuroraTheme.accent;
            if (isError) return AuroraTheme.warn;
            return AuroraTheme.borderStrong;
        }
        if (isActive) return AuroraTheme.accentTint15;
        if (isError) return Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25);
        return AuroraTheme.border;
    }

    Behavior on border.color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

    MouseArea {
        id: hoverMa
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    ColumnLayout {
        id: content
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: AuroraTheme.sp5
        spacing: 10

        // ── Row 1: Path + Badge ──────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            FsIcon {
                name: "folder"
                sizePx: 20
                color: AuroraTheme.accent
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: root.localPath
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                    elide: Text.ElideMiddle
                }
                Text {
                    Layout.fillWidth: true
                    text: "\u2192 Fshare: " + root.remotePath
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                    elide: Text.ElideMiddle
                }
            }

            FsBadge {
                text: {
                    switch (root.status) {
                    case 0: return qsTr("Hoạt động");
                    case 1: return qsTr("Đang quét");
                    case 2: return qsTr("Đang tải");
                    case 3: return qsTr("Tạm dừng");
                    case 4: return qsTr("Lỗi");
                    case 5: return qsTr("Đã tắt");
                    default: return "";
                    }
                }
                variant: {
                    switch (root.status) {
                    case 0: return "green";
                    case 1: return "blue";
                    case 2: return "red";
                    case 3: return "amber";
                    case 4: return "amber";
                    case 5: return "neutral";
                    default: return "neutral";
                    }
                }
                dot: root.status === 1 || root.status === 2
            }
        }

        // ── Row 2: Progress bar (Active / Paused) ───
        FsProgressBar {
            Layout.fillWidth: true
            visible: root.isActive || root.isPaused
            value: root.progress
            status: root.isPaused ? "warning" : "uploading"
        }

        // ── Row 3a: Active details ──────────────────
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.isActive
            spacing: 2

            Text {
                text: qsTr("Đang tải: %1 (%2/%3 file)").arg(root.currentFile).arg(root.filesCompleted).arg(root.filesTotal)
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                font.weight: Font.DemiBold
                color: AuroraTheme.ink2
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
            RowLayout {
                spacing: AuroraTheme.sp2
                Text {
                    text: qsTr("%1 còn lại").arg(root.remainingSize)
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }
                Text {
                    visible: root.speed.length > 0
                    text: "\u00B7"
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }
                Text {
                    visible: root.speed.length > 0
                    text: root.speed
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }
                Text {
                    visible: root.eta.length > 0
                    text: "\u00B7"
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }
                Text {
                    visible: root.eta.length > 0
                    text: root.eta
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }
            }
        }

        // ── Row 3b: Paused details ──────────────────
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.isPaused
            spacing: 2

            Text {
                text: qsTr("%1 file đang chờ").arg(root.pendingCount)
                    + (root.remainingSize.length > 0 ? (" \u00B7 " + qsTr("%1 còn lại").arg(root.remainingSize)) : "")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink2
            }
        }

        // ── Row 3c: Idle stats ──────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.status === 0
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                Text {
                    Layout.fillWidth: true
                    text: {
                        var s = "\u2713 " + qsTr("%1 file đã đồng bộ").arg(root.syncedCount);
                        if (root.totalSize.length > 0) s += " \u00B7 " + root.totalSize;
                        if (root.pendingCount > 0) s += " \u00B7 " + qsTr("%1 đang chờ").arg(root.pendingCount);
                        return s;
                    }
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }
                Text {
                    text: root.watchSubfolders ? qsTr("Theo dõi thư mục con") : ""
                    visible: root.watchSubfolders
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                Text {
                    Layout.fillWidth: true
                    visible: root.lastSyncTime.length > 0
                    text: qsTr("Cập nhật lần cuối: %1").arg(root.lastSyncTime)
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }
                Text {
                    text: root.deleteAfterUpload ? qsTr("Xóa local: Có") : qsTr("Xóa local: Không")
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }
            }
        }

        // ── Row 3d: Scanning state ──────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.status === 1
            spacing: AuroraTheme.sp2

            // Tiny inline spinner
            Rectangle {
                width: 14; height: 14
                radius: 7
                color: "transparent"
                border.width: 2
                border.color: AuroraTheme.accentTint15

                Rectangle {
                    width: 6; height: 2; radius: 1
                    color: AuroraTheme.accent
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 1
                }

                RotationAnimator on rotation {
                    from: 0; to: 360; duration: 900; loops: Animation.Infinite
                    running: root.status === 1 && !AuroraTheme.reduceMotion
                }
            }
            Text {
                text: qsTr("Đang quét thư mục...")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink2
            }
        }

        // ── Row 3e: Error state ─────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.isError
            spacing: AuroraTheme.sp1

            RowLayout {
                spacing: 6
                Text {
                    text: "\u26A0"
                    font.pixelSize: 13
                    color: AuroraTheme.warn
                }
                Text {
                    Layout.fillWidth: true
                    text: root.errorMessage
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.warn
                    wrapMode: Text.WordWrap
                }
            }
            Text {
                visible: root.errorCount > 0 || root.syncedCount > 0
                text: {
                    var parts = [];
                    if (root.errorCount > 0) parts.push(root.errorCount + " file lỗi");
                    if (root.syncedCount > 0) parts.push(qsTr("%1 file đã đồng bộ").arg(root.syncedCount));
                    return parts.join(" \u00B7 ");
                }
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink3
            }
        }

        // ── Row 3f: Disabled state ──────────────────
        Text {
            Layout.fillWidth: true
            visible: root.status === 5
            text: qsTr("Thư mục không tìm thấy hoặc đã bị tắt.")
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
            color: AuroraTheme.ink3
        }

        // ── Row 4: Actions ──────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp2

            Item { Layout.fillWidth: true }

            // Primary action (varies by status)
            FsButton {
                visible: root.status === 0
                text: qsTr("Quét lại")
                variant: "ghost"; size: "sm"
                onClicked: root.rescanClicked()
            }
            FsButton {
                visible: root.isActive
                text: qsTr("Tạm dừng")
                variant: "ghost"; size: "sm"
                onClicked: root.pauseClicked()
            }
            FsButton {
                visible: root.isPaused
                text: qsTr("Tiếp tục")
                variant: "secondary"; size: "sm"
                onClicked: root.resumeClicked()
            }
            FsButton {
                visible: root.isError
                text: qsTr("Thử lại")
                variant: "secondary"; size: "sm"
                onClicked: root.retryClicked()
            }
            FsButton {
                visible: root.status === 5
                text: qsTr("Bật lại")
                variant: "secondary"; size: "sm"
                onClicked: root.resumeClicked()
            }

            // Settings
            FsButton {
                text: qsTr("Cài đặt")
                variant: "ghost"; size: "sm"
                onClicked: root.settingsClicked()
            }

            // Remove (x)
            Rectangle {
                width: 28; height: 28
                radius: AuroraTheme.radiusSm
                color: removeMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }

                FsIcon {
                    anchors.centerIn: parent
                    name: "x"
                    sizePx: 14
                    color: removeMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink3
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                }
                MouseArea {
                    id: removeMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.removeClicked()
                }
            }
        }
    }
}
