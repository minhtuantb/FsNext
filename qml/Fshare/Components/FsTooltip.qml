// SPDX-License-Identifier: Proprietary
// FsTooltip — Hover tooltip anchored to a target.
//
// Usage:
//   Button {
//       id: myBtn
//       text: "Hi"
//       FsTooltip { text: qsTr("Details…"); target: myBtn }
//   }
//
// Implementation note:
//   Previous revision hand-rolled the floating bubble by reparenting to
//   `target.Window.contentItem` and computing x/y via `mapToItem`. That
//   binding doesn't re-evaluate when an ancestor of the target moves
//   (e.g. when the target lives inside a modal FsDialog and the dialog
//   plays its open animation), so the tooltip appeared at stale
//   coordinates. Migrated to Controls.ToolTip whose positioning is owned
//   by Qt's own popup machinery — it always resolves to the right place
//   regardless of whether the target is on a page or inside a dialog.

import QtQuick
import QtQuick.Controls
import FsAurora.Theme 1.0

Item {
    id: root

    property string text: ""
    property Item target: parent
    property int delay: 500
    // "top" | "bottom" — placement relative to target.
    property string position: "top"

    // Decorative wrapper. Sized 0x0 so it doesn't affect layout; the
    // actual bubble lives in the attached ToolTip below.
    width: 0
    height: 0

    HoverHandler {
        id: hov
        target: root.target
    }

    // ToolTip is reused (one instance per app); ToolTip.show / hide picks
    // the right Overlay and reuses Qt's positioning code that already
    // handles "what window am I in", modality, and ancestor transforms.
    Timer {
        id: showTimer
        interval: root.delay
        onTriggered: {
            if (!hov.hovered || root.text.length === 0 || !root.target)
                return;
            const pos = root.target.mapToGlobal(root.target.width / 2, 0);
            // Show via the per-item attached ToolTip on the target so the
            // bubble inherits the target's coordinate system and Qt handles
            // overlay parenting / z-order automatically.
            tip.parent = root.target;
            tip.x = (root.target.width - tip.implicitWidth) / 2;
            tip.y = root.position === "bottom"
                ? root.target.height + 6
                : -tip.implicitHeight - 6;
            tip.open();
        }
    }

    Connections {
        target: hov
        function onHoveredChanged() {
            if (hov.hovered) showTimer.restart();
            else { showTimer.stop(); tip.close(); }
        }
    }

    ToolTip {
        id: tip
        text: root.text
        delay: 0   // we already gate via showTimer above
        timeout: -1
        // Themed bubble. Qt's default ToolTip background uses native style;
        // we override to keep the Aurora look (ink1 dark on light, white on
        // dark).
        background: Rectangle {
            radius: AuroraTheme.radiusSm
            color: AuroraTheme.ink1
            opacity: 0.92
        }
        contentItem: Text {
            text: tip.text
            color: AuroraTheme.panel
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.caption.pixelSize
        }
    }
}
