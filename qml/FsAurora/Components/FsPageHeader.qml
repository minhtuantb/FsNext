// SPDX-License-Identifier: Proprietary
// FsPageHeader — Aurora editorial page header.
//
// Renders the recurring three-line composition:
//
//   ━━ KICKER MONO UPPERCASE                 (ink4, 11px, letter-spaced)
//   TitleFirst TitleAccent.                  (serif 48/56px; accent italic last)
//   subtitle text (optional)                 (mono 12px ink3)
//
// Variants:
//   • framed = true  → wraps in panel-color rectangle with 1px border (used by
//     SettingsPage / FavoritesPage and any page where the header lives in its
//     own card).
//   • framed = false → transparent container (UserInfoPage / inline headers).
//
// Trailing content (filter inputs, action buttons) declared inside the header
// goes into the right-aligned slot via the default property:
//
//   FsPageHeader {
//       framed: true
//       kicker: qsTr("Yêu thích · bộ sưu tập cá nhân")
//       title: qsTr("Yêu");  accentWord: qsTr("thích.")
//       subtitle: qsTr("12 mục đã gắn sao")
//
//       Rectangle { /* search box, appears on the right */ }
//   }
//
// Notes:
//   • `kicker` is prefixed with "━━ " automatically — pass just the text.
//   • `accentWord` is optional; when empty, only `title` is rendered.
//   • `titlePixelSize` lets pages opt into the 56px hero variant (default 48px).

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme

Rectangle {
    id: root

    property string kicker: ""
    property string title: ""
    property string accentWord: ""
    property string subtitle: ""
    property int    titlePixelSize: 48
    property real   titleLetterSpacing: -1.4
    property bool   framed: false

    // Compact variant — drops the "━━ KICKER" rail entirely (the sidebar
    // already tells the user which surface they're on), shrinks the serif
    // hero from 48-56 px down to a usable 22 px, and inlines the subtitle on
    // the same row as the title with a "·" separator. The editorial 48-56 px
    // hero ate 100-130 px of vertical real estate on every page; that was a
    // lot of chrome for a workspace where users want the list/grid front and
    // centre. Pages that still want the hero (HomePage, marketing surfaces)
    // simply don't opt in.
    property bool   compact: false
    readonly property int    _kickerVisible: !compact && kicker.length > 0
    readonly property int    _resolvedTitleSize: compact ? 22 : titlePixelSize
    readonly property real   _resolvedLetterSpacing: compact ? -0.4 : titleLetterSpacing
    readonly property int    _resolvedPadding:
        framed ? (compact ? AuroraTheme.sp4 : AuroraTheme.sp6) : 0

    // Auto-scale the serif title on narrow windows so the editorial hero
    // doesn't overflow / wrap into the trailing slot when the user resizes
    // below the FsNext minimum width (800px). Threshold matches Aurora's
    // narrow-desktop breakpoint. Set to false on a per-instance basis if a
    // page wants pixel-fixed sizing. Compact variant doesn't need scaling —
    // 22 px already fits any sane window width.
    property bool   responsive: true
    readonly property int _effectiveTitleSize:
        (!compact && responsive && width > 0 && width < 720)
            ? Math.round(_resolvedTitleSize * 0.7)
            : _resolvedTitleSize

    // Trailing items (filter box, primary CTA) get appended to a right-aligned
    // RowLayout inside the header. Declared as `data` so consumers don't need
    // to set RowLayout.* attached properties manually.
    default property alias trailing: trailingRow.data

    Layout.fillWidth: true
    implicitHeight: contentRow.implicitHeight + (framed ? root._resolvedPadding * 2 : 0)

    radius: framed ? AuroraTheme.radiusLg : 0
    color: framed ? AuroraTheme.panel : "transparent"
    border.width: framed ? 1 : 0
    border.color: framed ? AuroraTheme.border : "transparent"

    RowLayout {
        id: contentRow
        anchors.fill: parent
        anchors.margins: root.framed ? root._resolvedPadding : 0
        spacing: AuroraTheme.sp4

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: root.compact ? 2 : AuroraTheme.sp2

            Text {
                visible: root._kickerVisible
                text: "━━ " + root.kicker
                font.family: AuroraTheme.fontMono
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                font.capitalization: Font.AllUppercase
                color: AuroraTheme.ink4
            }

            RowLayout {
                Layout.bottomMargin: root.compact ? 0 : 2
                spacing: root.compact ? 6 : 8

                Text {
                    text: root.title
                    color: AuroraTheme.ink1
                    // Compact uses sans-DemiBold (the serif at 22 looks like
                    // an under-styled mistake); editorial keeps the serif hero.
                    font.family: root.compact ? AuroraTheme.fontSans : AuroraTheme.fontSerif
                    font.pixelSize: root._effectiveTitleSize
                    font.letterSpacing: root._resolvedLetterSpacing
                    font.weight: root.compact ? Font.DemiBold : Font.Normal
                    lineHeight: 1.0
                }
                Text {
                    visible: root.accentWord.length > 0
                    text: root.accentWord
                    color: AuroraTheme.accent
                    font.family: root.compact ? AuroraTheme.fontSans : AuroraTheme.fontSerif
                    font.italic: true
                    font.pixelSize: root._effectiveTitleSize
                    font.letterSpacing: root._resolvedLetterSpacing
                    font.weight: root.compact ? Font.DemiBold : Font.Normal
                    lineHeight: 1.0
                }
                // Compact: subtitle inline with the title (separator "·") so
                // the entire header is a single ~28 px row. Editorial: subtitle
                // sits on its own row below the hero.
                Text {
                    visible: root.compact && root.subtitle.length > 0
                    Layout.leftMargin: 8
                    Layout.bottomMargin: 2
                    Layout.alignment: Qt.AlignBaseline
                    text: "· " + root.subtitle
                    color: AuroraTheme.ink3
                    font.family: AuroraTheme.fontMono
                    font.pixelSize: 12
                }
                Item { Layout.fillWidth: true }
            }

            Text {
                Layout.topMargin: AuroraTheme.sp1
                visible: !root.compact && root.subtitle.length > 0
                text: root.subtitle
                color: AuroraTheme.ink3
                font.family: AuroraTheme.fontMono
                font.pixelSize: 12
            }
        }

        Item { Layout.fillWidth: true }

        RowLayout {
            id: trailingRow
            Layout.alignment: Qt.AlignVCenter
            spacing: AuroraTheme.sp2
        }
    }
}
