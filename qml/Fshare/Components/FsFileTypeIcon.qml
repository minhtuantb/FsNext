// SPDX-License-Identifier: Proprietary
// FsFileTypeIcon — File type indicator with semantic color

import QtQuick
import FsAurora.Theme 1.0
Rectangle {
    id: root

    property string fileName: ""
    property bool   isFolder: false
    property int    sizePx: 40

    width: sizePx
    height: sizePx
    radius: AuroraTheme.radiusSm

    // Determine type from extension
    function fileExtension(name) {
        const dot = name.lastIndexOf(".");
        if (dot < 0) return "";
        return name.substring(dot + 1).toLowerCase();
    }

    function categoryColor(ext) {
        // Video
        if (["mp4","mkv","avi","mov","wmv","flv","webm","m4v","3gp"].indexOf(ext) >= 0)
            return AuroraTheme.accent;
        // Archive
        if (["zip","rar","7z","tar","gz","bz2"].indexOf(ext) >= 0)
            return AuroraTheme.warn;
        // Audio
        if (["mp3","flac","wav","aac","ogg","m4a","wma"].indexOf(ext) >= 0)
            return AuroraTheme.info;
        // Image
        if (["jpg","jpeg","png","gif","bmp","svg","webp","psd","tif","tiff"].indexOf(ext) >= 0)
            return AuroraTheme.success;
        // Document
        if (["pdf","doc","docx","xls","xlsx","ppt","pptx","txt","rtf"].indexOf(ext) >= 0)
            return AuroraTheme.ink2;
        // App / disc
        if (["exe","msi","dmg","iso","img","apk"].indexOf(ext) >= 0)
            return AuroraTheme.ink3;
        // Default: warm gray
        return AuroraTheme.ink3;
    }

    function categoryLabel(ext) {
        if (["mp4","mkv","avi","mov","wmv","flv","webm","m4v","3gp"].indexOf(ext) >= 0) return "VID";
        if (["zip","rar","7z","tar","gz","bz2"].indexOf(ext) >= 0) return "ZIP";
        if (["mp3","flac","wav","aac","ogg","m4a","wma"].indexOf(ext) >= 0) return "AUD";
        if (["jpg","jpeg","png","gif","bmp","svg","webp","psd","tif","tiff"].indexOf(ext) >= 0) return "IMG";
        if (["pdf","doc","docx","xls","xlsx","ppt","pptx","txt","rtf"].indexOf(ext) >= 0) return "DOC";
        if (["exe","msi","dmg","iso","img","apk"].indexOf(ext) >= 0) return "APP";
        return ext.substring(0, 3).toUpperCase();
    }

    // Folder gets a bold solid amber chip so it pops out from file icons
    // (which use a pale tinted square with a small text label).
    color: {
        if (root.isFolder)
            return AuroraTheme.warn;
        const ext = fileExtension(fileName);
        const c = categoryColor(ext);
        return Qt.rgba(c.r, c.g, c.b, 0.12);
    }
    border.color: root.isFolder ? AuroraTheme.warn : "transparent"
    border.width: root.isFolder ? 1 : 0

    // Folder glyph: tabbed silhouette drawn in white on amber background for
    // maximum contrast. Much more distinct than a text label.
    Item {
        visible: root.isFolder
        anchors.centerIn: parent
        width:  Math.round(root.sizePx * 0.70)
        height: Math.round(root.sizePx * 0.58)

        // Folder tab (back flap)
        Rectangle {
            id: tab
            width:  Math.round(parent.width * 0.48)
            height: Math.round(parent.height * 0.28)
            radius: Math.max(1, Math.round(root.sizePx * 0.05))
            color:  "white"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: Math.round(parent.width * 0.02)
        }

        // Folder body (main pocket)
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: Math.round(parent.height * 0.82)
            radius: Math.max(2, Math.round(root.sizePx * 0.08))
            color:  "white"

            // Front-lip line giving the folder a "pocket" look
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: Math.round(parent.height * 0.22)
                height: Math.max(1, Math.round(root.sizePx * 0.045))
                color: Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g,
                               AuroraTheme.warn.b, 0.35)
            }
        }
    }

    // File type label for non-folder items
    Text {
        visible: !root.isFolder
        anchors.centerIn: parent
        text: root.categoryLabel(root.fileExtension(root.fileName))
        font.family: AuroraTheme.fontSans
        font.pixelSize: Math.round(root.sizePx * 0.27)
        font.weight: Font.Bold
        color: root.categoryColor(root.fileExtension(root.fileName))
    }
}
