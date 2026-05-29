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

    // Auto-scale the serif title on narrow windows so the editorial hero
    // doesn't overflow / wrap into the trailing slot when the user resizes
    // below the FsNext minimum width (800px). Threshold matches Aurora's
    // narrow-desktop breakpoint. Set to false on a per-instance basis if a
    // page wants pixel-fixed sizing.
    property bool   responsive: true
    readonly property int _effectiveTitleSize:
        (responsive && width > 0 && width < 720)
            ? Math.round(titlePixelSize * 0.7)
            : titlePixelSize

    // Trailing items (filter box, primary CTA) get appended to a right-aligned
    // RowLayout inside the header. Declared as `data` so consumers don't need
    // to set RowLayout.* attached properties manually.
    default property alias trailing: trailingRow.data

    Layout.fillWidth: true
    implicitHeight: contentRow.implicitHeight + (framed ? AuroraTheme.sp6 * 2 : 0)

    radius: framed ? AuroraTheme.radiusLg : 0
    color: framed ? AuroraTheme.panel : "transparent"
    border.width: framed ? 1 : 0
    border.color: framed ? AuroraTheme.border : "transparent"

    RowLayout {
        id: contentRow
        anchors.fill: parent
        anchors.margins: root.framed ? AuroraTheme.sp6 : 0
        spacing: AuroraTheme.sp4

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: AuroraTheme.sp2

            Text {
                visible: root.kicker.length > 0
                text: "━━ " + root.kicker
                font.family: AuroraTheme.fontMono
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                font.capitalization: Font.AllUppercase
                color: AuroraTheme.ink4
            }

            RowLayout {
                Layout.bottomMargin: 2
                spacing: 8

                Text {
                    text: root.title
                    color: AuroraTheme.ink1
                    font.family: AuroraTheme.fontSerif
                    font.pixelSize: root._effectiveTitleSize
                    font.letterSpacing: root.titleLetterSpacing
                    lineHeight: 1.0
                }
                Text {
                    visible: root.accentWord.length > 0
                    text: root.accentWord
                    color: AuroraTheme.accent
                    font.family: AuroraTheme.fontSerif
                    font.italic: true
                    font.pixelSize: root._effectiveTitleSize
                    font.letterSpacing: root.titleLetterSpacing
                    lineHeight: 1.0
                }
                Item { Layout.fillWidth: true }
            }

            Text {
                Layout.topMargin: AuroraTheme.sp1
                visible: root.subtitle.length > 0
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
