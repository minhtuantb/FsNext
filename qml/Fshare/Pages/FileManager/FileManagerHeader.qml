// SPDX-License-Identifier: Proprietary
// FileManagerHeader — Aurora editorial header for the My Files page.
//
// Thin wrapper around Aurora.FsPageHeader that injects the breadcrumb-aware
// stats subtitle ("47 mục · Photos") fed by the FileManager ViewModel.

import QtQuick
import QtQuick.Layouts
import FsAurora.Theme 1.0
import FsAurora.Components 1.0 as Aurora

Aurora.FsPageHeader {
    framed: false
    kicker: qsTr("My Files")
    title: qsTr("Files")
    accentWord: "."
    subtitle: {
        if (!fileManagerViewModel || fileManagerViewModel.totalCount <= 0)
            return "";
        const count = fileManagerViewModel.totalCount;
        const bc    = fileManagerViewModel.breadcrumbs;
        const here  = (bc && bc.length > 0)
            ? bc[bc.length - 1].name
            : qsTr("Thư mục gốc");
        return qsTr("%1 mục").arg(count) + " · " + here;
    }
}
