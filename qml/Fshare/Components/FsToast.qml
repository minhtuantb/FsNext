// SPDX-License-Identifier: Proprietary
// FsToast — Slide-in notification (Aurora-native)
//
// Usage:
//   FsToast { title: "Tải lên thành công"; desc: "file.mp4"; variant: "success" }

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

Rectangle {
    id: root

    property string title: ""
    property string desc: ""
    property string variant: "info"     // success | warning | error | info

    // Don't render the card if there's no message to show. An earlier
    // regression routed empty-string error signals to this component,
    // producing a "ghost" toast with only an icon + close button.
    readonly property bool _hasContent: (title !== "") || (desc !== "")
    property string iconText: {
        switch (variant) {
        case "success": return "✓"
        case "warning": return "⚠"
        case "error":   return "✕"
        default:        return "ℹ"
        }
    }
    property bool autoClose: true
    property int autoCloseMs: 5000

    // Opaque token. When non-empty, the toast body becomes clickable: clicking
    // (anywhere except the ✕) emits activated() and then closed(), letting the
    // host route the user to whatever surface the action describes (e.g.,
    // "go.download" → switch to the Download page). Empty = pure notification.
    property string action: ""

    signal closed()
    signal activated()

    implicitWidth: _hasContent ? 380 : 0
    implicitHeight: _hasContent ? (row.implicitHeight + AuroraTheme.sp4 * 2) : 0
    visible: _hasContent
    radius: AuroraTheme.radiusMd
    color: AuroraTheme.panel
    border.width: 1
    border.color: variant === "error" ? Qt.rgba(AuroraTheme.danger.r, AuroraTheme.danger.g, AuroraTheme.danger.b, 0.25)
                                       : AuroraTheme.border

    // Screen-reader announcement: "AlertMessage" makes NVDA / Narrator read
    // the toast aloud as soon as it appears.  Equivalent to ARIA
    // role="alert" / aria-live="assertive" — see Aurora handoff a11y spec.
    Accessible.role: variant === "error" ? Accessible.AlertMessage : Accessible.StaticText
    Accessible.name: title
    Accessible.description: desc

    property color _accentColor: {
        switch (variant) {
        case "success": return AuroraTheme.success
        case "warning": return AuroraTheme.warn
        case "error":   return AuroraTheme.danger
        default:        return AuroraTheme.info
        }
    }
    property color _accentTint: {
        switch (variant) {
        case "success": return AuroraTheme.successSoft
        case "warning": return AuroraTheme.warnSoft
        case "error":   return AuroraTheme.dangerSoft
        default:        return AuroraTheme.infoSoft
        }
    }
    property color _accentText: _accentColor

    // Body-click handler — declared BEFORE the RowLayout so it sits BEHIND
    // the close-button MouseArea in stacking order: clicking the ✕ still
    // dismisses without firing activated(). Disabled when no action is set,
    // so plain notifications keep their idle cursor and don't surprise the
    // user with a "click to go somewhere" affordance.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.action.length > 0
        cursorShape: root.action.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: { root.activated(); root.closed(); }
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: AuroraTheme.sp4
        spacing: AuroraTheme.sp3

        // Icon
        Rectangle {
            width: 34; height: 34
            radius: AuroraTheme.radiusSm
            color: root._accentTint

            Text {
                anchors.centerIn: parent
                text: root.iconText
                font.pixelSize: 16
                color: root._accentText
            }
        }

        // Text
        Column {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: root.title
                font.family: AuroraTheme.fontSans
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: AuroraTheme.ink1
                width: parent.width
                elide: Text.ElideRight
            }
            Text {
                visible: root.desc !== ""
                text: root.desc
                font.family: AuroraTheme.fontSans
                font.pixelSize: 11
                color: AuroraTheme.ink3
                width: parent.width
                elide: Text.ElideRight
            }
        }

        // Close
        Text {
            text: "✕"
            font.pixelSize: 14
            color: AuroraTheme.ink4

            MouseArea {
                anchors.fill: parent
                anchors.margins: -4
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = AuroraTheme.ink1
                onExited: parent.color = AuroraTheme.ink4
                onClicked: root.closed()
            }
        }
    }

    // Slide-in via opacity (safe — no position animation loop)
    opacity: 0
    Component.onCompleted: {
        opacity = 1
        if (autoClose) autoCloseTimer.start()
    }
    Behavior on opacity { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow; easing.type: Easing.OutCubic } }

    // Call show() to re-display and restart the auto-close timer after the
    // toast has been hidden (e.g. after a previous onClosed: visible = false).
    function show() {
        opacity = 1;
        if (autoClose) autoCloseTimer.restart();
    }

    Timer {
        id: autoCloseTimer
        interval: root.autoCloseMs
        onTriggered: root.closed()
    }
}
