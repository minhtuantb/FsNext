// SPDX-License-Identifier: Proprietary
// FsTransferItem — Row component for download/upload transfers
// Per design system spec: 64px height, file icon + filename + progress + actions

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme 1.0
import Fshare.Utils 1.0

Item {
    id: root

    // -- Public API --
    property string transferId: ""
    property string fileName: "filename.ext"
    property string folderPath: ""      // e.g. "My Photos/2024" (empty for single-file downloads)
    property real fileSize: 0           // bytes
    property real bytesTransferred: 0   // bytes
    property real progress: 0           // 0..100
    property real speed: 0              // bytes/sec
    property string eta: ""
    property int status: 0              // TransferState enum: 0=Queued 1=Active 2=Paused 3=Complete 4=Error 5=Cancelled
    property string errorMessage: ""
    property bool showActions: true
    // Enable ↑ ↓ queue-reorder buttons (shown for Queued items only).
    property bool showReorder: false
    // Upload-specific action buttons (shown on Complete items only).
    // Hidden by default so the component stays interchangeable with the
    // download use-case that doesn't need these.
    property bool showOpenInBrowser: false
    property bool showInfoButton:    false
    property bool showDeleteLocal:   false
    // ms-since-epoch when this item entered Complete state (0 = not yet
    // completed). Drives the relative-time hint on just-completed cards.
    property real completedAt: 0
    // Current wall-clock (ms) — bound by the parent page so the relative-
    // time string updates without each row needing its own Timer. If left
    // at 0, the component falls back to "vừa xong" on first paint.
    property real nowMs: 0

    signal pauseClicked()
    signal resumeClicked()
    signal cancelClicked()
    signal openFolderClicked()
    signal copyLinkClicked()
    signal moveUpClicked()
    signal moveDownClicked()
    // Emitted when the user clicks ✕ on a Complete item (soft archive).
    // Distinct from cancelClicked, which aborts an in-progress transfer.
    signal dismissClicked()
    // Upload-only actions (buttons gated on the corresponding show* props).
    signal openInBrowserClicked()
    signal showInfoClicked()
    signal deleteLocalClicked()

    implicitHeight: root.folderPath.length > 0 ? 78 : 64

    // Status color & label
    property color _statusColor: {
        switch (status) {
            case 1: return AuroraTheme.accent;          // Active → brand
            case 2: return AuroraTheme.warn;        // Paused
            case 3: return AuroraTheme.success;        // Complete
            case 4: return AuroraTheme.accent;          // Error (will use 40% opacity in bar)
            case 5: return AuroraTheme.ink3;        // Cancelled
            default: return AuroraTheme.ink3;       // Queued
        }
    }

    property string _statusLabel: {
        switch (status) {
            case 0: return qsTr("Đang chờ");
            case 1: return qsTr("Đang tải");
            case 2: return qsTr("Đã tạm dừng");
            case 3: return qsTr("Hoàn tất");
            case 4: return qsTr("Lỗi");
            case 5: return qsTr("Đã huỷ");
            default: return "";
        }
    }

    // Relative time for just-completed items: "vừa xong" for <60s, then
    // minutes / hours. Kept deliberately quiet — the goal is a gentle
    // confirmation, not a countdown.
    function _relativeTime(completedMs, nowMs) {
        if (!completedMs || completedMs <= 0) return "";
        const anchor = nowMs > 0 ? nowMs : Date.now();
        const diff = Math.max(0, anchor - completedMs);
        if (diff < 60 * 1000)            return qsTr("vừa xong");
        if (diff < 60 * 60 * 1000)       return qsTr("%1 phút trước").arg(Math.floor(diff / 60000));
        return qsTr("%1 giờ trước").arg(Math.floor(diff / 3600000));
    }

    // Hover background — HoverHandler is passive (no event consumption),
    // so child MouseAreas (pause/cancel buttons) won't steal hover and cause flicker.
    HoverHandler {
        id: hoverHandler
    }

    Rectangle {
        anchors.fill: parent
        color: hoverHandler.hovered ? AuroraTheme.panel : "transparent"
        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: AuroraTheme.sp3
        anchors.rightMargin: AuroraTheme.sp3
        spacing: AuroraTheme.sp3

        FsFileTypeIcon {
            fileName: root.fileName
            sizePx: 40
        }

        // Main info column: filename + size/eta + progress bar
        ColumnLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp1

            // Top row: filename + status badge
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                Text {
                    Layout.fillWidth: true
                    text: root.fileName
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.body.pixelSize
                    font.weight: Font.Medium
                    color: AuroraTheme.ink1
                    elide: Text.ElideMiddle
                }

                // Status mini-badge
                Rectangle {
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: statusText.implicitWidth + 10
                    radius: AuroraTheme.radiusPill
                    color: Qt.rgba(root._statusColor.r, root._statusColor.g, root._statusColor.b, 0.10)
                    border.width: 1
                    border.color: Qt.rgba(root._statusColor.r, root._statusColor.g, root._statusColor.b, 0.25)

                    Text {
                        id: statusText
                        anchors.centerIn: parent
                        text: root._statusLabel
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        color: root._statusColor
                    }
                }
            }

            // Folder path row — only shown for files that belong to a folder download
            Text {
                visible: root.folderPath.length > 0
                text: "📁 " + root.folderPath
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink3
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            // Middle row: bytes / total / speed / eta / completion hint
            RowLayout {
                Layout.fillWidth: true
                spacing: AuroraTheme.sp2

                Text {
                    text: FsFormat.bytes(root.bytesTransferred) + " / " + FsFormat.bytes(root.fileSize)
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }

                // Relative time on just-completed items — tasteful "vừa xong".
                Text {
                    visible: root.status === 3 && root.completedAt > 0
                    text: {
                        const rel = root._relativeTime(root.completedAt, root.nowMs);
                        return rel.length > 0 ? "· " + rel : "";
                    }
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: root.status === 1
                    text: FsFormat.speed(root.speed) || "—"
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }

                Text {
                    visible: root.status === 1 && root.eta.length > 0
                    text: "· " + root.eta
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink2
                }

                Text {
                    id: errorText
                    visible: root.status === 4 && root.errorMessage.length > 0
                    text: root.errorMessage
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.accent
                    elide: Text.ElideRight
                    Layout.maximumWidth: 360

                    HoverHandler { id: errorHover }
                    ToolTip.text: root.errorMessage
                    ToolTip.visible: errorHover.hovered && root.errorMessage.length > 0
                    ToolTip.delay: 300
                }
            }

            // Bottom row: progress bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                radius: 2
                color: AuroraTheme.borderStrong

                Rectangle {
                    width: parent.width * Math.max(0, Math.min(100, root.progress)) / 100
                    height: parent.height
                    radius: 2
                    color: root.status === 4 ? Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.40)
                          : root._statusColor
                    Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: 200 } }
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                }
            }
        }

        // Action slot — fixed-width, always present in layout.
        //
        // Layout-shift fix: earlier versions used `visible: hovered`, which
        // removed the Row from the parent RowLayout entirely on mouse leave.
        // The filename column (Layout.fillWidth) then expanded; on hover it
        // collapsed again → visible jitter. Now we always reserve a fixed
        // slot (wide enough for the widest state — 3 buttons × 28 + gaps)
        // and only animate opacity on the inner Row. The filename width is
        // stable regardless of hover state or status transitions.
        Item {
            id: _actionSlot
            // Baseline 100px fits the default 3-button layout (download rows
            // and in-progress upload rows). Upload-Complete rows opt into
            // extra buttons via showOpenInBrowser / showInfoButton /
            // showDeleteLocal, so we widen the slot to avoid clipping.
            Layout.preferredWidth: (root.showOpenInBrowser
                                    || root.showInfoButton
                                    || root.showDeleteLocal) ? 204 : 100
            Layout.preferredHeight: 28
            Layout.alignment: Qt.AlignVCenter

            Row {
                id: _actions
                anchors.centerIn: parent
                spacing: AuroraTheme.sp1
                // Only hide actions when a transfer is actively progressing
                // and the pointer isn't over the row — reduces visual noise
                // while files are downloading. For Paused/Complete/Error
                // states the actions stay visible so Resume/Open/Copy/Dismiss
                // are reachable with no hover-hunting required.
                opacity: (root.showActions && (
                            hoverHandler.hovered ||
                            root.status === 2 ||   // Paused
                            root.status === 3 ||   // Complete
                            root.status === 4      // Error
                         )) ? 1 : 0
                Behavior on opacity {
                    enabled: !AuroraTheme.reduceMotion
                    NumberAnimation { duration: AuroraTheme.durFast }
                }
                enabled: opacity > 0

                // Pause / Resume — Active & Paused only
                Rectangle {
                    visible: root.status === 1 || root.status === 2
                    width: 28; height: 28; radius: 8
                    color: pauseMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: root.status === 2 ? "▶" : "❚❚"
                        font.pixelSize: 11
                        color: AuroraTheme.ink2
                    }
                    MouseArea {
                        id: pauseMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.status === 2 ? root.resumeClicked() : root.pauseClicked()
                    }
                }

                // Open folder — Complete only
                Rectangle {
                    visible: root.status === 3
                    width: 28; height: 28; radius: 8
                    color: folderMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "📂"
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: folderMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openFolderClicked()
                    }
                }

                // Copy link — Complete only (shares link after upload)
                Rectangle {
                    visible: root.status === 3
                    width: 28; height: 28; radius: 8
                    color: copyLinkMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "🔗"
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: copyLinkMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        ToolTip.text: qsTr("Sao chép liên kết")
                        ToolTip.visible: containsMouse
                        ToolTip.delay: 400
                        onClicked: root.copyLinkClicked()
                    }
                }

                // Open share link in browser — Complete only, upload rows
                Rectangle {
                    visible: root.status === 3 && root.showOpenInBrowser
                    width: 28; height: 28; radius: 8
                    color: openBrowserMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "🌐"
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: openBrowserMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        ToolTip.text: qsTr("Mở liên kết trên Fshare")
                        ToolTip.visible: containsMouse
                        ToolTip.delay: 400
                        onClicked: root.openInBrowserClicked()
                    }
                }

                // Show in My Files — Complete only, upload rows
                Rectangle {
                    visible: root.status === 3 && root.showInfoButton
                    width: 28; height: 28; radius: 8
                    color: infoMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "ℹ"
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        color: AuroraTheme.ink2
                    }
                    MouseArea {
                        id: infoMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        ToolTip.text: qsTr("Xem trong My Files")
                        ToolTip.visible: containsMouse
                        ToolTip.delay: 400
                        onClicked: root.showInfoClicked()
                    }
                }

                // Delete local copy — Complete only, upload rows
                Rectangle {
                    visible: root.status === 3 && root.showDeleteLocal
                    width: 28; height: 28; radius: 8
                    color: deleteLocalMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "🗑"
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: deleteLocalMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        ToolTip.text: qsTr("Xoá file cục bộ")
                        ToolTip.visible: containsMouse
                        ToolTip.delay: 400
                        onClicked: root.deleteLocalClicked()
                    }
                }

                // Move Up / Down — Queued only
                Rectangle {
                    visible: root.showReorder && root.status === 0
                    width: 28; height: 28; radius: 8
                    color: moveUpMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "↑"
                        font.pixelSize: 13
                        color: AuroraTheme.ink2
                    }
                    MouseArea {
                        id: moveUpMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.moveUpClicked()
                    }
                }

                Rectangle {
                    visible: root.showReorder && root.status === 0
                    width: 28; height: 28; radius: 8
                    color: moveDownMa.containsMouse ? AuroraTheme.divider : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "↓"
                        font.pixelSize: 13
                        color: AuroraTheme.ink2
                    }
                    MouseArea {
                        id: moveDownMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.moveDownClicked()
                    }
                }

                // Cancel (active) / Dismiss (complete). Renders identically
                // but emits different signals so callers can distinguish
                // "abort an upload" from "archive a finished card".
                Rectangle {
                    width: 28; height: 28; radius: 8
                    color: cancelMa.containsMouse ? AuroraTheme.accentTint10 : "transparent"
                    Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: cancelMa.containsMouse ? AuroraTheme.accent : AuroraTheme.ink2
                        Behavior on color { enabled: !AuroraTheme.reduceMotion; ColorAnimation { duration: AuroraTheme.durFast } }
                    }
                    MouseArea {
                        id: cancelMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        ToolTip.text: root.status === 3 ? qsTr("Lưu trữ") : qsTr("Huỷ")
                        ToolTip.visible: containsMouse
                        ToolTip.delay: 400
                        onClicked: {
                            if (root.status === 3) root.dismissClicked();
                            else                   root.cancelClicked();
                        }
                    }
                }
            }
        }
    }

    // Bottom divider
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: AuroraTheme.divider
    }
}
