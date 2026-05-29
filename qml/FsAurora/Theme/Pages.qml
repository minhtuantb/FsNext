// SPDX-License-Identifier: Proprietary
// Pages — singleton mapping symbolic names to the integer indices used by
// Main.qml's currentPage / sidebar nav / command palette routing.
//
// Use these constants instead of literal numbers so adding a new page (or
// reordering) only updates this one file.
//
//   import FsAurora.Theme
//   root.currentPage = Pages.home
//   title: Pages.titles[currentPage]
//
// `titles` is a lookup map (int → string) so callers don't have to know the
// underlying numeric layout. Wrapped in qsTr() so translations pick them up.

pragma Singleton
import QtQuick

QtObject {
    readonly property int download:  0
    readonly property int upload:    1
    readonly property int sync:      2
    readonly property int files:     3
    readonly property int favorites: 4
    readonly property int account:   5
    readonly property int settings:  6
    readonly property int showcase:  7
    readonly property int home:      8

    readonly property var titles: ({
        0: qsTr("Tải xuống"),
        1: qsTr("Tải lên"),
        2: qsTr("Đồng bộ"),
        3: qsTr("File"),
        4: qsTr("Yêu thích"),
        5: qsTr("Tài khoản"),
        6: qsTr("Cài đặt"),
        7: qsTr("Showcase"),
        8: qsTr("Trang chủ")
    })
}
