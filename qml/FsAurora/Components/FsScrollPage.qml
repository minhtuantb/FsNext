// SPDX-License-Identifier: Proprietary
// FsScrollPage — Aurora-themed vertical scroll container for page roots.
//
// One-stop replacement for the ad-hoc mix of ScrollView / Flickable wrappers
// previously seen on SettingsPage, UserInfoPage etc. Provides:
//   • A vertical scrollbar with Aurora chrome (auto-fade thumb, accent-on-hover).
//   • A ColumnLayout content area where children land directly via the default
//     property — no nested ColumnLayout boilerplate per page.
//   • Configurable `contentSpacing` (defaults to AuroraTheme.sp6 — the same
//     spacing currently used by the editorial page sections).
//
// Pages that have their own internal scroll surface (GridView, ListView, multi-
// pane layouts) should NOT wrap themselves in FsScrollPage — keep their root
// as an Item.
//
// Usage:
//   FsScrollPage {
//       FsPageHeader { ... }
//       SettingsCard { ... }
//       SettingsCard { ... }
//   }

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme

ScrollView {
    id: root
    clip: true

    property real contentSpacing: AuroraTheme.sp6
    default property alias content: contentColumn.data

    // Hide the noisy horizontal bar — pages are vertical scroll only.
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy:   ScrollBar.AsNeeded

    // NOTE: Qt 6 ScrollView's ScrollBar.vertical attached property is
    // READ-ONLY — it surfaces the built-in scrollbar so you can tweak its
    // sub-properties (policy etc.), but assigning a whole ScrollBar { … }
    // value to it is treated as "Cannot assign a value directly to a
    // grouped property" and bricks the QML load. Customizing the bar
    // therefore has to go through the attached property's sub-fields
    // below — NOT a wholesale replacement.
    //
    // Aurora-styled vertical scroll thumb. Auto-fades when idle; accent on
    // hover/press; honours reduceMotion by skipping the opacity animation.
    ScrollBar.vertical.contentItem: Rectangle {
        implicitWidth: 4
        radius: 2
        color: root.ScrollBar.vertical.pressed
            ? AuroraTheme.accent
            : (root.ScrollBar.vertical.hovered ? AuroraTheme.borderStrong : AuroraTheme.border)
        opacity: root.ScrollBar.vertical.active ? 0.85 : 0.0
        Behavior on opacity {
            enabled: !AuroraTheme.reduceMotion
            NumberAnimation { duration: AuroraTheme.durFast }
        }
        Behavior on color {
            enabled: !AuroraTheme.reduceMotion
            ColorAnimation { duration: AuroraTheme.durFast }
        }
    }
    ScrollBar.vertical.background: Item { implicitWidth: 8 }

    ColumnLayout {
        id: contentColumn
        width: root.availableWidth
        spacing: root.contentSpacing
    }
}
