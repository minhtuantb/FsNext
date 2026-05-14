// SPDX-License-Identifier: Proprietary
// ShowcasePage — Visual catalog of design tokens + all components in all states.
// DEV-only: not added to main nav; access via Settings/About in dev build.
//
// Use this page to:
//   1. Visually verify all component variants render correctly
//   2. Iterate on design tokens (color/typography/spacing) and see effects everywhere
//   3. Provide reference for designers/developers when adding new pages

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FsAurora.Theme 1.0
import Fshare.Components 1.0

ScrollView {
    id: page
    clip: true

    ColumnLayout {
        width: page.availableWidth
        spacing: AuroraTheme.sp8

        // ════════════════════════════════════════════════
        //  HEADER
        // ════════════════════════════════════════════════
        ColumnLayout {
            Layout.fillWidth: true
            spacing: AuroraTheme.sp1
            Text {
                text: qsTr("Design System Showcase")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.h2.pixelSize
                font.weight: Font.Bold
                color: AuroraTheme.ink1
            }
            Text {
                text: qsTr("All design tokens and components — for visual review and iteration")
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink2
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 1: COLOR TOKENS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("1. Colors")
            subtitle: qsTr("Brand red used sparingly — only logo, primary button, active progress")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                // Brand red palette
                ColorRow {
                    label: qsTr("Brand")
                    swatches: [
                        { name: "red",         color: AuroraTheme.accent,        hex: "#E8001D" },
                        { name: "redHover",    color: AuroraTheme.accentHover,   hex: "#FF3D54" },
                        { name: "redPressed",  color: AuroraTheme.accentPressed, hex: "#C50018" },
                        { name: "redTint6",    color: AuroraTheme.accentSoft,   hex: "6%" },
                        { name: "redTint10",   color: AuroraTheme.accentTint10,  hex: "10%" },
                        { name: "redTint15",   color: AuroraTheme.accentTint15,  hex: "15%" },
                        { name: "redBorder",   color: AuroraTheme.accentTint15,  hex: "25%" }
                    ]
                }

                ColorRow {
                    label: qsTr("Semantic")
                    swatches: [
                        { name: "green",      color: AuroraTheme.success,      hex: "success" },
                        { name: "greenTint",  color: AuroraTheme.successSoft,  hex: "10%" },
                        { name: "amber",      color: AuroraTheme.warn,      hex: "warning" },
                        { name: "amberTint",  color: AuroraTheme.warnSoft,  hex: "10%" },
                        { name: "blue",       color: AuroraTheme.info,       hex: "info" },
                        { name: "blueTint",   color: AuroraTheme.infoSoft,   hex: "8%" }
                    ]
                }

                ColorRow {
                    label: qsTr("Surfaces")
                    swatches: [
                        { name: "bg0",     color: AuroraTheme.bg,     hex: "page" },
                        { name: "bg1",     color: AuroraTheme.panel,     hex: "secondary" },
                        { name: "bg2",     color: AuroraTheme.divider,     hex: "card" },
                        { name: "bg3",     color: AuroraTheme.borderStrong,     hex: "track" },
                        { name: "surface", color: AuroraTheme.panel, hex: "elevated" }
                    ]
                }

                ColorRow {
                    label: qsTr("Text")
                    swatches: [
                        { name: "text1", color: AuroraTheme.ink1, hex: "primary" },
                        { name: "text2", color: AuroraTheme.ink2, hex: "secondary" },
                        { name: "text3", color: AuroraTheme.ink3, hex: "muted" },
                        { name: "text4", color: AuroraTheme.ink4, hex: "disabled" }
                    ]
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 2: TYPOGRAPHY
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("2. Typography")
            subtitle: qsTr("Font: ") + AuroraTheme.fontSans.split(",")[0].trim()
                + qsTr(" — Mono: ") + AuroraTheme.fontMono.split(",")[0].trim()

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                TypeSample { label: "display";    sample: "Display 32 ExtraBold";  fontSize: AuroraTheme.hero.pixelSize;   weight: AuroraTheme.hero.weight }
                TypeSample { label: "heading1";   sample: "Heading 1 — 20 Bold";   fontSize: AuroraTheme.h2.pixelSize;  weight: AuroraTheme.h2.weight }
                TypeSample { label: "heading2";   sample: "Heading 2 — 15 DemiBold"; fontSize: AuroraTheme.h3.pixelSize;  weight: AuroraTheme.h3.weight }
                TypeSample { label: "body";       sample: "Body — Default text 13px"; fontSize: AuroraTheme.body.pixelSize;     weight: AuroraTheme.body.weight }
                TypeSample { label: "bodyStrong"; sample: "Body Strong — emphasis"; fontSize: AuroraTheme.bodyStrong.pixelSize; weight: AuroraTheme.bodyStrong.weight }
                TypeSample { label: "caption";    sample: "Caption — small text 11px"; fontSize: AuroraTheme.caption.pixelSize;   weight: AuroraTheme.caption.weight }
                TypeSample { label: "label";      sample: "LABEL — UPPERCASE 11"; fontSize: AuroraTheme.label.pixelSize;     weight: AuroraTheme.label.weight }
                TypeSample { label: "mono";       sample: "1.25 GB / 4.7 GB · 12.3 MB/s"; fontSize: AuroraTheme.body.pixelSize; weight: Font.Normal; mono: true }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 3: SPACING & RADIUS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("3. Spacing & Radius")
            subtitle: qsTr("4px grid · pill buttons (100px)")

            content: RowLayout {
                spacing: AuroraTheme.sp6

                ColumnLayout {
                    spacing: AuroraTheme.sp1
                    Text {
                        text: qsTr("Spacing")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.label.pixelSize
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink3
                    }
                    Repeater {
                        model: [
                            { name: "sp4",  size: AuroraTheme.sp1 },
                            { name: "sp8",  size: AuroraTheme.sp2 },
                            { name: "sp12", size: AuroraTheme.sp3 },
                            { name: "sp16", size: AuroraTheme.sp4 },
                            { name: "sp24", size: AuroraTheme.sp6 },
                            { name: "sp32", size: AuroraTheme.sp8 }
                        ]
                        delegate: RowLayout {
                            spacing: AuroraTheme.sp2
                            Rectangle {
                                width: modelData.size; height: 16
                                color: AuroraTheme.accent
                                radius: 2
                            }
                            Text {
                                text: modelData.name + " · " + modelData.size + "px"
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: AuroraTheme.caption.pixelSize
                                color: AuroraTheme.ink2
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: AuroraTheme.sp1
                    Text {
                        text: qsTr("Radius")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.label.pixelSize
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink3
                    }
                    Repeater {
                        model: [
                            { name: "r6",   r: AuroraTheme.radiusSm },
                            { name: "r8",   r: 8 },
                            { name: "r10",  r: AuroraTheme.radiusMd },
                            { name: "r14",  r: AuroraTheme.radiusLg },
                            { name: "r16",  r: 16 },
                            { name: "rPill", r: AuroraTheme.radiusPill }
                        ]
                        delegate: RowLayout {
                            spacing: AuroraTheme.sp2
                            Rectangle {
                                width: 64; height: 24
                                color: AuroraTheme.accentTint10
                                border.width: 1; border.color: AuroraTheme.accentTint15
                                radius: modelData.r
                            }
                            Text {
                                text: modelData.name + " · " + modelData.r + "px"
                                font.family: AuroraTheme.fontSans
                                font.pixelSize: AuroraTheme.caption.pixelSize
                                color: AuroraTheme.ink2
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 4: BUTTONS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("4. Buttons (FsButton)")
            subtitle: qsTr("5 variants × 3 sizes")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                // Variants row
                RowLayout {
                    spacing: AuroraTheme.sp2
                    FsButton { text: "Primary";   variant: "primary" }
                    FsButton { text: "Secondary"; variant: "secondary" }
                    FsButton { text: "Ghost";     variant: "ghost" }
                    FsButton { text: "Danger";    variant: "danger" }
                    FsButton { text: "Success";   variant: "success" }
                }

                // With icons
                RowLayout {
                    spacing: AuroraTheme.sp2
                    FsButton { text: "Add";      icon: "+";  variant: "primary" }
                    FsButton { text: "Edit";     icon: "✎";  variant: "secondary" }
                    FsButton { text: "Cancel";   variant: "ghost" }
                    FsButton { text: "Disabled"; variant: "primary"; enabled: false }
                }

                // Sizes
                RowLayout {
                    spacing: AuroraTheme.sp2
                    FsButton { text: "Small";   variant: "primary"; size: "sm" }
                    FsButton { text: "Default"; variant: "primary"; size: "default" }
                    FsButton { text: "Large";   variant: "primary"; size: "lg" }
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 5: TEXT FIELDS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("5. Text Fields (FsTextField)")
            subtitle: qsTr("Label, placeholder, error, hint, password")

            content: GridLayout {
                columns: 2
                rowSpacing: AuroraTheme.sp4
                columnSpacing: AuroraTheme.sp4

                FsTextField {
                    Layout.preferredWidth: 280
                    label: qsTr("Email")
                    placeholder: qsTr("you@example.com")
                }
                FsTextField {
                    Layout.preferredWidth: 280
                    label: qsTr("Password")
                    placeholder: qsTr("Enter password")
                    echoMode: TextInput.Password
                    text: "secret123"
                }
                FsTextField {
                    Layout.preferredWidth: 280
                    label: qsTr("With error")
                    placeholder: qsTr("Try typing")
                    text: "wrong"
                    error: qsTr("This value is invalid")
                }
                FsTextField {
                    Layout.preferredWidth: 280
                    label: qsTr("With hint")
                    placeholder: qsTr("Enter folder name")
                    hint: qsTr("Will be created in current folder")
                }
                FsTextField {
                    Layout.preferredWidth: 280
                    label: qsTr("Read-only")
                    text: "Cannot edit this"
                    readOnly: true
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 6: CARDS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("6. Cards (FsCard)")
            subtitle: qsTr("Default and accent variants")

            content: RowLayout {
                spacing: AuroraTheme.sp4

                Rectangle {
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 100
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Default Card")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.body.pixelSize
                        color: AuroraTheme.ink1
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 100
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.accentSoft
                    border.width: 1
                    border.color: AuroraTheme.accentTint15
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Accent Card")
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.body.pixelSize
                        font.weight: Font.DemiBold
                        color: AuroraTheme.accent
                    }
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 7: BADGES
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("7. Badges (FsBadge)")
            subtitle: qsTr("7 variants for status indicators")

            content: RowLayout {
                spacing: AuroraTheme.sp2
                Repeater {
                    model: [
                        { color: AuroraTheme.accent,        bg: AuroraTheme.accentTint10,    border: AuroraTheme.accentTint15,     label: "Active" },
                        { color: AuroraTheme.success,  bg: AuroraTheme.successSoft,    border: Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.25),   label: "Complete" },
                        { color: AuroraTheme.warn,  bg: AuroraTheme.warnSoft,    border: Qt.rgba(AuroraTheme.warn.r, AuroraTheme.warn.g, AuroraTheme.warn.b, 0.25),   label: "Paused" },
                        { color: AuroraTheme.info,   bg: AuroraTheme.infoSoft,     border: Qt.rgba(AuroraTheme.info.r, AuroraTheme.info.g, AuroraTheme.info.b, 0.20),    label: "Info" },
                        { color: AuroraTheme.ink2,      bg: AuroraTheme.borderStrong,          border: AuroraTheme.border,        label: "Queued" }
                    ]
                    delegate: Rectangle {
                        Layout.preferredHeight: 22
                        Layout.preferredWidth: badgeText.implicitWidth + AuroraTheme.sp4
                        radius: AuroraTheme.radiusPill
                        color: modelData.bg
                        border.width: 1
                        border.color: modelData.border
                        Text {
                            id: badgeText
                            anchors.centerIn: parent
                            text: modelData.label
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: modelData.color
                        }
                    }
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 8: PROGRESS BARS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("8. Progress Bars (FsProgressBar)")
            subtitle: qsTr("Status-aware fill colors")

            content: ColumnLayout {
                spacing: AuroraTheme.sp4

                ProgressDemo { label: "Active";    progress: 65; barColor: AuroraTheme.accent }
                ProgressDemo { label: "Complete";  progress: 100; barColor: AuroraTheme.success }
                ProgressDemo { label: "Paused";    progress: 42; barColor: AuroraTheme.warn }
                ProgressDemo { label: "Error";     progress: 30; barColor: Qt.rgba(AuroraTheme.accent.r, AuroraTheme.accent.g, AuroraTheme.accent.b, 0.40) }
                ProgressDemo { label: "Queued";    progress: 0;  barColor: AuroraTheme.ink3 }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 9: FILE TYPE ICONS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("9. File Type Icons (FsFileTypeIcon)")
            subtitle: qsTr("Color-coded by file extension")

            content: RowLayout {
                spacing: AuroraTheme.sp3

                Repeater {
                    model: ["video.mp4", "archive.zip", "song.mp3", "photo.jpg", "doc.pdf", "app.exe", "folder", "unknown.xyz"]
                    delegate: ColumnLayout {
                        spacing: AuroraTheme.sp1
                        FsFileTypeIcon {
                            Layout.alignment: Qt.AlignHCenter
                            fileName: modelData
                            sizePx: 48
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: modelData
                            font.family: AuroraTheme.fontSans
                            font.pixelSize: AuroraTheme.caption.pixelSize
                            color: AuroraTheme.ink2
                        }
                    }
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 10: TRANSFER ITEMS
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("10. Transfer Items (FsTransferItem)")
            subtitle: qsTr("All states: queued, active, paused, complete, error")

            content: ColumnLayout {
                spacing: 0

                FsTransferItem {
                    Layout.fillWidth: true
                    fileName: "movie-Captain-America-2026-1080p.mkv"
                    fileSize: 4700000000
                    bytesTransferred: 0
                    progress: 0
                    status: 0  // Queued
                }
                FsTransferItem {
                    Layout.fillWidth: true
                    fileName: "ubuntu-22.04-desktop-amd64.iso"
                    fileSize: 4700000000
                    bytesTransferred: 3055000000
                    progress: 65
                    speed: 12300000  // 12.3 MB/s
                    eta: "00:42"
                    status: 1  // Active
                }
                FsTransferItem {
                    Layout.fillWidth: true
                    fileName: "music-album.zip"
                    fileSize: 850000000
                    bytesTransferred: 357000000
                    progress: 42
                    status: 2  // Paused
                }
                FsTransferItem {
                    Layout.fillWidth: true
                    fileName: "vacation-photos.tar.gz"
                    fileSize: 1280000000
                    bytesTransferred: 1280000000
                    progress: 100
                    status: 3  // Complete
                    showActions: true
                }
                FsTransferItem {
                    Layout.fillWidth: true
                    fileName: "broken-download.exe"
                    fileSize: 50000000
                    bytesTransferred: 12000000
                    progress: 24
                    status: 4  // Error
                    errorMessage: qsTr("Connection timeout")
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 11: EMPTY & LOADING STATES
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("11. State Components")
            subtitle: qsTr("FsEmptyState · FsLoadingState")

            content: RowLayout {
                spacing: AuroraTheme.sp4

                Rectangle {
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 280
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border

                    FsEmptyState {
                        anchors.fill: parent
                        icon: "↓"
                        title: qsTr("No downloads")
                        description: qsTr("Add a Fshare link to get started")
                        actionText: qsTr("Add Download")
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 280
                    radius: AuroraTheme.radiusLg
                    color: AuroraTheme.panel
                    border.width: 1
                    border.color: AuroraTheme.border

                    FsLoadingState {
                        anchors.fill: parent
                        message: qsTr("Loading files…")
                    }
                }
            }
        }

        // ════════════════════════════════════════════════
        //  SECTION 12: DIALOG (interactive)
        // ════════════════════════════════════════════════
        SectionCard {
            title: qsTr("12. Dialog (FsDialog)")
            subtitle: qsTr("Click button to open · scale + opacity transitions")

            content: RowLayout {
                spacing: AuroraTheme.sp2

                FsButton {
                    text: qsTr("Open Dialog")
                    variant: "primary"
                    onClicked: showcaseDialog.open()
                }

                FsButton {
                    text: qsTr("Open Confirm")
                    variant: "secondary"
                    onClicked: confirmDialog.open()
                }
            }
        }

        Item { Layout.preferredHeight: AuroraTheme.sp12 }
    }

    // Dialog instances (anchored to root via FsDialog's anchors.fill: parent)
    FsDialog {
        id: showcaseDialog
        title: qsTr("Sample Dialog")
        dialogWidth: 460
        content: Item {
            width: 460
            height: 120
            Text {
                anchors.centerIn: parent
                text: qsTr("This is a dialog content area.\nAny QML can go here.")
                horizontalAlignment: Text.AlignHCenter
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink2
            }
        }
        footer: Item {
            width: 460
            height: 64
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                spacing: AuroraTheme.sp2
                Item { Layout.fillWidth: true }
                FsButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: showcaseDialog.close() }
                FsButton { text: qsTr("Confirm"); variant: "primary"; onClicked: showcaseDialog.close() }
            }
        }
    }

    FsDialog {
        id: confirmDialog
        title: qsTr("Delete file?")
        dialogWidth: 380
        content: Item {
            width: 380
            height: 80
            Text {
                anchors.centerIn: parent
                text: qsTr("This action cannot be undone.")
                horizontalAlignment: Text.AlignHCenter
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.body.pixelSize
                color: AuroraTheme.ink2
            }
        }
        footer: Item {
            width: 380
            height: 64
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AuroraTheme.sp6
                anchors.rightMargin: AuroraTheme.sp6
                spacing: AuroraTheme.sp2
                Item { Layout.fillWidth: true }
                FsButton { text: qsTr("Cancel"); variant: "ghost"; onClicked: confirmDialog.close() }
                FsButton { text: qsTr("Delete"); variant: "danger"; onClicked: confirmDialog.close() }
            }
        }
    }

    // ══════════════════════════════════════════════════════
    //  Inline helper components
    // ══════════════════════════════════════════════════════

    component SectionCard: Rectangle {
        property string title: ""
        property string subtitle: ""
        property alias content: contentSlot.children

        Layout.fillWidth: true
        Layout.preferredHeight: header.height + contentSlot.childrenRect.height + AuroraTheme.sp6 * 2 + AuroraTheme.sp2
        radius: AuroraTheme.radiusLg
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border

        Item {
            id: header
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp5
            height: 44

            ColumnLayout {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: parent.parent.parent.title
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.h3.pixelSize
                    font.weight: Font.DemiBold
                    color: AuroraTheme.ink1
                }
                Text {
                    visible: parent.parent.parent.subtitle.length > 0
                    text: parent.parent.parent.subtitle
                    font.family: AuroraTheme.fontSans
                    font.pixelSize: AuroraTheme.caption.pixelSize
                    color: AuroraTheme.ink3
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom; width: parent.width; height: 1
                color: AuroraTheme.divider
            }
        }

        Item {
            id: contentSlot
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: header.bottom
            anchors.leftMargin: AuroraTheme.sp6
            anchors.rightMargin: AuroraTheme.sp6
            anchors.topMargin: AuroraTheme.sp4
            height: childrenRect.height
        }
    }

    component ColorRow: ColumnLayout {
        property string label: ""
        property var swatches: []
        spacing: AuroraTheme.sp2

        Text {
            text: label
            font.family: AuroraTheme.fontSans
            font.pixelSize: AuroraTheme.label.pixelSize
            font.weight: Font.DemiBold
            color: AuroraTheme.ink3
        }

        RowLayout {
            spacing: AuroraTheme.sp2
            Repeater {
                model: swatches
                delegate: ColumnLayout {
                    spacing: AuroraTheme.sp1
                    Rectangle {
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 56
                        radius: 8
                        color: modelData.color
                        border.width: 1
                        border.color: AuroraTheme.border
                    }
                    Text {
                        text: modelData.name
                        font.family: AuroraTheme.fontSans
                        font.pixelSize: AuroraTheme.caption.pixelSize
                        font.weight: Font.DemiBold
                        color: AuroraTheme.ink1
                    }
                    Text {
                        text: modelData.hex
                        font.family: AuroraTheme.fontMono
                        font.pixelSize: 10
                        color: AuroraTheme.ink3
                    }
                }
            }
        }
    }

    component TypeSample: RowLayout {
        property string label: ""
        property string sample: ""
        property int fontSize: 13
        property int weight: Font.Normal
        property bool mono: false
        spacing: AuroraTheme.sp4

        Text {
            Layout.preferredWidth: 100
            text: label
            font.family: AuroraTheme.fontMono
            font.pixelSize: AuroraTheme.caption.pixelSize
            color: AuroraTheme.ink3
        }
        Text {
            text: sample
            font.family: mono ? AuroraTheme.fontMono : AuroraTheme.fontSans
            font.pixelSize: fontSize
            font.weight: weight
            color: AuroraTheme.ink1
        }
        Item { Layout.fillWidth: true }
    }

    component ProgressDemo: ColumnLayout {
        property string label: ""
        property real progress: 0
        property color barColor: AuroraTheme.accent
        spacing: AuroraTheme.sp1

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: label
                font.family: AuroraTheme.fontSans
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink2
            }
            Text {
                text: progress.toFixed(0) + "%"
                font.family: AuroraTheme.fontMono
                font.pixelSize: AuroraTheme.caption.pixelSize
                color: AuroraTheme.ink2
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 4
            radius: 2
            color: AuroraTheme.borderStrong
            Rectangle {
                width: parent.width * progress / 100
                height: parent.height
                radius: 2
                color: barColor
                Behavior on width { enabled: !AuroraTheme.reduceMotion; NumberAnimation { duration: AuroraTheme.durSlow } }
            }
        }
    }
}
