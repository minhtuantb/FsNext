// SPDX-License-Identifier: Proprietary
// FileManagerHeader — extracted from FileManagerPage.qml under ADR 003 D13.
//
// Pure presentation: kicker mono uppercase + serif mega-title + breadcrumb-aware
// stats line.  Reads `fileManagerViewModel` from the QML root context (same way
// FileManagerPage did), so the extraction is a straight lift — no new props.

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0

ColumnLayout {
    spacing: AuroraTheme.sp2

    Text {
        text: "━━ " + qsTr("My Files")
        font.family: AuroraTheme.fontMono
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 1.4
        font.capitalization: Font.AllUppercase
        color: AuroraTheme.ink4
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 0

        Text {
            text: qsTr("Files")
            font.family: AuroraTheme.fontSerif
            font.pixelSize: 48
            font.weight: Font.Normal
            font.letterSpacing: -1.4
            color: AuroraTheme.ink1
        }
        Text {
            text: "."
            font.family: AuroraTheme.fontSerif
            font.pixelSize: 48
            font.weight: Font.Normal
            font.italic: true
            font.letterSpacing: -1.4
            color: AuroraTheme.accent
        }
        Item { Layout.fillWidth: true }
    }

    // Mini stats line — shown only once data has loaded.  Pulls breadcrumb
    // tail name from the VM so the user sees both the count and the folder
    // they're currently in ("47 mục · Photos").
    Text {
        Layout.topMargin: AuroraTheme.sp1
        visible: fileManagerViewModel && fileManagerViewModel.totalCount > 0
        text: {
            if (!fileManagerViewModel) return "";
            const count = fileManagerViewModel.totalCount;
            const bc    = fileManagerViewModel.breadcrumbs;
            const here  = (bc && bc.length > 0)
                ? bc[bc.length - 1].name
                : qsTr("Thư mục gốc");
            return qsTr("%1 mục").arg(count) + " · " + here;
        }
        font.family: AuroraTheme.fontMono
        font.pixelSize: 12
        color: AuroraTheme.ink3
    }
}
