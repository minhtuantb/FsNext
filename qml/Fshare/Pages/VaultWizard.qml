// SPDX-License-Identifier: Proprietary
// VaultWizard — WIZ-1..7 create-vault flow (Flow A). Modal overlay.
//
// Backend reality (VaultManager): a passphrase is always the primary key
// (KEK ← Argon2id(passphrase)); a keyfile is an OPTIONAL recovery wrap. So the
// method choice is Passphrase / Passphrase + recovery keyfile (the prototype's
// "keyfile-only" is not offered — it would mislead). Recovery keyfile is
// exported AFTER the vault is created (export requires an unlocked vault).
//
// Binds to the `vaultViewModel` context property.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import Fshare.Components 1.0
import FsAurora.Theme 1.0

Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 1200

    property var vm: (typeof vaultViewModel !== "undefined") ? vaultViewModel : null
    signal finished()

    // ── wizard state ──
    property int    step: 1
    property string method: "pass"      // "pass" | "both"
    property string pass: ""
    property string pass2: ""
    property bool   generated: false
    property bool   savePass: true
    property bool   hasRecovery: false
    property string recoveryPath: ""
    property string vName: qsTr("Personal Vault")
    property string location: root.vm ? root.vm.vaultDir : ""
    property string level: "medium"     // high | medium | convenient
    property bool   autoDelete: false
    property bool   ackA: false
    property bool   ackB: false
    property bool   ackC: false
    property bool   ackD: false
    property bool   showAdv: false
    property int    kdfStrength: 0      // 0 balanced · 1 strong · 2 maximum
    property bool   creating: false
    property string errorText: ""

    readonly property bool _passOk: pass.length >= 12 && /[a-zA-Z]/.test(pass)
                                     && /[0-9]/.test(pass) && /[^a-zA-Z0-9]/.test(pass)
    readonly property bool _matchOk: pass.length > 0 && pass === pass2
    readonly property bool _allAck: ackA && ackB && ackC && ackD

    function open() {
        step = 1; method = "pass"; pass = ""; pass2 = ""; generated = false; savePass = true;
        hasRecovery = false; recoveryPath = ""; vName = qsTr("Personal Vault");
        location = root.vm ? root.vm.vaultDir : ""; level = "medium"; autoDelete = false;
        ackA = ackB = ackC = ackD = false; showAdv = false; kdfStrength = 0;
        creating = false; errorText = "";
        visible = true;
    }
    function close() { visible = false }

    function _next() { if (step < 7) step++ }
    function _back() { if (step > 1) step-- }

    function _storageLevelInt() {
        return level === "high" ? 1 : (level === "convenient" ? 3 : 2);
    }

    function _genPassphrase() {
        var words = ["Tiger","River","Cloud","Amber","Stone","Maple","Cobalt","Ember",
                     "Lotus","Comet","Harbor","Willow","Quartz","Raven","Saffron","Cedar"];
        function pick() { return words[Math.floor(Math.random() * words.length)]; }
        var num = String(10 + Math.floor(Math.random() * 89));
        var syms = "!@#$%&?";
        var sym = syms.charAt(Math.floor(Math.random() * syms.length));
        return pick() + "-" + pick() + "-" + pick() + "-" + pick() + "-" + num + sym;
    }

    function _doCreate() {
        if (!root.vm) return;
        root.errorText = "";
        root.creating = true;
        if (root.location.length > 0) root.vm.setVaultDir(root.location);
        root.vm.createVault(root.vName, root.pass, root.kdfStrength, root._storageLevelInt());
    }

    Connections {
        target: root.vm
        function onOperationFinished(op, ok, error) {
            if (!root.creating) return;
            if (op === "create") {
                root.creating = false;
                if (ok) {
                    if (root.hasRecovery && root.recoveryPath.length > 0)
                        root.vm.exportRecoveryKeyfile(root.recoveryPath);
                    root.step = 7;
                } else {
                    root.errorText = (error === "AlreadyExists")
                        ? qsTr("Vault đã tồn tại tại vị trí này.")
                        : qsTr("Không thể tạo Vault. Vui lòng thử lại.");
                }
            }
        }
    }

    // Hidden helper for clipboard copy of the generated passphrase.
    TextEdit { id: clip; visible: false }
    FileDialog {
        id: keyfileDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Tệp khóa (*.key)"), qsTr("Tất cả (*)")]
        onAccepted: { root.recoveryPath = selectedFile.toString(); root.hasRecovery = true; }
    }
    FolderDialog {
        id: folderDialog
        onAccepted: root.location = selectedFolder.toString().replace("file:///", "")
    }

    // ── Overlay ──
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.42)
        MouseArea { anchors.fill: parent }   // swallow clicks
    }

    // ── Wizard box ──
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(720, parent.width - AuroraTheme.sp8)
        height: Math.min(implicitH, parent.height - AuroraTheme.sp8)
        readonly property int implicitH: stepperBox.Layout.preferredHeight + bodyFlick.contentH + footer.Layout.preferredHeight + 2
        radius: 18
        color: AuroraTheme.panel
        border.width: 1
        border.color: AuroraTheme.border
        clip: true
        MouseArea { anchors.fill: parent }   // don't let clicks fall through

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Stepper ──
            Item {
                id: stepperBox
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.rightMargin: AuroraTheme.sp6
                    spacing: 0
                    Repeater {
                        model: [qsTr("Giới thiệu"), qsTr("Bảo vệ"), qsTr("Passphrase"),
                                qsTr("Lưu ý"), qsTr("Khôi phục"), qsTr("Thiết lập"), qsTr("Hoàn tất")]
                        delegate: RowLayout {
                            id: stepDel
                            required property int index
                            required property string modelData
                            readonly property int n: index + 1
                            readonly property bool done: n < root.step
                            readonly property bool cur: n === root.step
                            Layout.fillWidth: true
                            spacing: 0
                            ColumnLayout {
                                spacing: 5
                                Layout.alignment: Qt.AlignTop
                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.preferredWidth: 26; Layout.preferredHeight: 26
                                    radius: 13
                                    color: stepDel.done ? AuroraTheme.accent
                                          : stepDel.cur ? AuroraTheme.accentSoft : AuroraTheme.sunk
                                    border.width: stepDel.cur ? 1.5 : 0
                                    border.color: AuroraTheme.accent
                                    FsIcon {
                                        visible: stepDel.done
                                        anchors.centerIn: parent; name: "check"; sizePx: 13; color: "#FFFFFF"
                                    }
                                    Text {
                                        visible: !stepDel.done
                                        anchors.centerIn: parent
                                        text: stepDel.n
                                        font.family: AuroraTheme.fontMono; font.pixelSize: 11; font.weight: Font.Bold
                                        color: stepDel.cur ? AuroraTheme.accent : AuroraTheme.ink4
                                    }
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: stepDel.modelData
                                    font.family: AuroraTheme.fontMono; font.pixelSize: 9
                                    font.weight: stepDel.cur ? Font.Bold : Font.Normal
                                    color: stepDel.cur ? AuroraTheme.ink1 : AuroraTheme.ink4
                                }
                            }
                            Rectangle {
                                visible: stepDel.index < 6
                                Layout.fillWidth: stepDel.index < 6
                                Layout.preferredHeight: 1.5
                                Layout.bottomMargin: 18
                                Layout.leftMargin: 4; Layout.rightMargin: 4
                                color: stepDel.n < root.step ? AuroraTheme.accent : AuroraTheme.border
                            }
                        }
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
            }

            // ── Body (scrollable) ──
            Flickable {
                id: bodyFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property int contentH: Math.min(440, bodyCol.implicitHeight)
                contentHeight: bodyCol.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: bodyCol
                    width: bodyFlick.width
                    // Steps are mutually exclusive via `visible` (Column skips
                    // hidden children, so only the active step occupies space).

                    // ── WIZ-1 Welcome ──
                    ColumnLayout {
                        visible: root.step === 1
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp4
                        WizHero { iconName: "shield-key"; title: qsTr("Tạo Vault mã hóa")
                            body: qsTr("Một không gian riêng tư cho tài liệu nhạy cảm — mã hóa ngay trên máy bạn trước khi rời thiết bị.") }
                        Repeater {
                            model: [
                                { i: "lock", t: qsTr("Mã hóa diễn ra ngay trên máy của bạn.") },
                                { i: "eye-off", t: qsTr("Server không đọc được nội dung file.") },
                                { i: "key", t: qsTr("Chỉ bạn giữ chìa khóa để mở lại.") },
                                { i: "cloud-up", t: qsTr("Dùng được cả khi đang ngoại tuyến.") }
                            ]
                            delegate: Rectangle {
                                id: vline
                                required property var modelData
                                Layout.fillWidth: true
                                radius: AuroraTheme.radiusMd
                                color: AuroraTheme.sunk
                                implicitHeight: 52
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: AuroraTheme.sp3; spacing: AuroraTheme.sp3
                                    Rectangle {
                                        Layout.preferredWidth: 30; Layout.preferredHeight: 30; radius: 8
                                        color: AuroraTheme.panel; border.width: 1; border.color: AuroraTheme.border
                                        FsIcon { anchors.centerIn: parent; name: vline.modelData.i; sizePx: 15; color: AuroraTheme.accent }
                                    }
                                    Text { Layout.fillWidth: true; text: vline.modelData.t; wrapMode: Text.WordWrap
                                        font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink2 }
                                }
                            }
                        }
                    }

                    // ── WIZ-2 Method ──
                    ColumnLayout {
                        visible: root.step === 2
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp3
                        WizTitle { title: qsTr("Bạn muốn bảo vệ Vault bằng cách nào?")
                            sub: qsTr("Passphrase là bắt buộc. Bạn có thể thêm tệp khóa khôi phục để phòng quên passphrase.") }
                        Repeater {
                            model: [
                                { id: "pass", icon: "key", t: qsTr("Passphrase"),
                                  d: qsTr("Một câu mật khẩu chỉ bạn biết."), badge: "rec" },
                                { id: "both", icon: "shield-check", t: qsTr("Passphrase + tệp khóa (2 lớp)"),
                                  d: qsTr("Thêm tệp khóa khôi phục để mở Vault nếu quên passphrase."), badge: "adv" }
                            ]
                            delegate: Rectangle {
                                id: mCard
                                required property var modelData
                                readonly property bool sel: root.method === mCard.modelData.id
                                Layout.fillWidth: true
                                radius: AuroraTheme.radiusLg
                                color: sel ? AuroraTheme.accentSoft : AuroraTheme.panel
                                border.width: 1.5
                                border.color: sel ? AuroraTheme.accent : AuroraTheme.border
                                implicitHeight: 72
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: AuroraTheme.sp4; spacing: AuroraTheme.sp3
                                    Rectangle {
                                        Layout.preferredWidth: 42; Layout.preferredHeight: 42; radius: 11
                                        color: mCard.sel ? AuroraTheme.accent : AuroraTheme.sunk
                                        FsIcon { anchors.centerIn: parent; name: mCard.modelData.icon; sizePx: 20
                                            color: mCard.sel ? "#FFFFFF" : AuroraTheme.ink3 }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 2
                                        RowLayout {
                                            spacing: AuroraTheme.sp2
                                            Text { text: mCard.modelData.t; font.family: AuroraTheme.fontSans
                                                font.pixelSize: 14; font.weight: Font.Bold; color: AuroraTheme.ink1 }
                                            FsBadge { text: mCard.modelData.badge === "rec" ? qsTr("Khuyến nghị") : qsTr("Nâng cao")
                                                variant: mCard.modelData.badge === "rec" ? "success" : "accent" }
                                        }
                                        Text { Layout.fillWidth: true; text: mCard.modelData.d; wrapMode: Text.WordWrap
                                            font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink3 }
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 20; Layout.preferredHeight: 20; radius: 10
                                        color: "transparent"; border.width: 1.5
                                        border.color: mCard.sel ? AuroraTheme.accent : AuroraTheme.borderStrong
                                        Rectangle { anchors.centerIn: parent; width: 10; height: 10; radius: 5
                                            visible: mCard.sel; color: AuroraTheme.accent }
                                    }
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: root.method = mCard.modelData.id }
                            }
                        }
                    }

                    // ── WIZ-3 Passphrase ──
                    ColumnLayout {
                        visible: root.step === 3
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp3
                        WizTitle { title: qsTr("Tạo passphrase của bạn")
                            sub: qsTr("Đây là chìa khóa mở Vault. Tự đặt một câu, hoặc để FsNext tạo giúp một passphrase mạnh.") }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.fillWidth: true; text: qsTr("Passphrase"); font.family: AuroraTheme.fontSans
                                font.pixelSize: 12; font.weight: Font.DemiBold; color: AuroraTheme.ink2 }
                            FsButton { text: root.generated ? qsTr("Tạo lại") : qsTr("Tạo tự động"); variant: "link"; icon: "sparkle"
                                onClicked: { var p = root._genPassphrase(); root.pass = p; root.pass2 = p; root.generated = true;
                                             w3pass.text = p; w3confirm.text = p; } }
                        }
                        FsTextField {
                            id: w3pass
                            Layout.fillWidth: true
                            echoMode: TextInput.Password; reveal: true; leadingIcon: "key"; mono: true
                            placeholder: qsTr("Nhập passphrase…")
                            text: root.pass
                            onTextChanged: { if (text !== root.pass) { root.pass = text; root.generated = false; } }
                        }
                        FsStrengthMeter { Layout.fillWidth: true; password: root.pass }
                        FsTextField {
                            id: w3confirm
                            visible: !root.generated
                            Layout.fillWidth: true
                            label: qsTr("Nhập lại passphrase")
                            echoMode: TextInput.Password; reveal: true; leadingIcon: "key"; mono: true
                            text: root.pass2
                            onTextChanged: root.pass2 = text
                            valid: root.pass2.length > 0 && root._matchOk
                            error: (root.pass2.length > 0 && !root._matchOk) ? qsTr("Passphrase chưa khớp") : ""
                        }
                        FsCallout {
                            Layout.fillWidth: true
                            tone: "info"; compact: true
                            body: qsTr("FsNext không lưu passphrase ở đâu cả. Hãy ghi nhớ hoặc sao chép vào trình quản lý mật khẩu.")
                        }
                    }

                    // ── WIZ-4 Loss warning ──
                    ColumnLayout {
                        visible: root.step === 4
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp3
                        FsCallout {
                            Layout.fillWidth: true
                            tone: "danger"
                            title: qsTr("Đọc kỹ trước khi tiếp tục")
                            body: qsTr("Passphrase do bạn tự đặt và chỉ mình bạn biết — FsNext không lưu lại. Nếu quên, chúng tôi không thể khôi phục giúp bạn, và file trong Vault sẽ không mở lại được. Đây cũng chính là điều làm nên sự riêng tư: không ai khác — kể cả Fshare — chạm được vào dữ liệu của bạn.")
                        }
                        FsCallout {
                            Layout.fillWidth: true
                            tone: "info"; compact: true
                            body: qsTr("Mã hóa chạy trên máy bạn; FsNext không giữ khóa. Hãy luôn giữ thêm một bản sao lưu dữ liệu quan trọng ở nơi khác.")
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: AuroraTheme.sp1
                            radius: AuroraTheme.radiusLg
                            color: "transparent"; border.width: 1; border.color: AuroraTheme.border
                            implicitHeight: ackCol.implicitHeight + AuroraTheme.sp4 * 2
                            ColumnLayout {
                                id: ackCol
                                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                                anchors.margins: AuroraTheme.sp4
                                spacing: AuroraTheme.sp3
                                FsCheckbox { Layout.fillWidth: true; checked: root.ackA; onToggled: root.ackA = checked
                                    label: qsTr("Tôi hiểu Fshare không thể khôi phục passphrase giúp tôi.") }
                                FsCheckbox { Layout.fillWidth: true; checked: root.ackB; onToggled: root.ackB = checked
                                    label: qsTr("Tôi hiểu nếu mất passphrase, các file trong Vault sẽ không mở lại được.") }
                                FsCheckbox { Layout.fillWidth: true; checked: root.ackC; onToggled: root.ackC = checked
                                    label: qsTr("Tôi đã lưu passphrase ở nơi an toàn (hoặc sẽ làm ngay).") }
                                FsCheckbox { Layout.fillWidth: true; checked: root.ackD; onToggled: root.ackD = checked
                                    label: qsTr("Tôi đã đọc và hiểu các lưu ý trên.") }
                            }
                        }
                    }

                    // ── WIZ-5 Recovery keyfile ──
                    ColumnLayout {
                        visible: root.step === 5
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp4
                        WizHero { iconName: "key-file"; title: qsTr("Tải tệp khóa khôi phục")
                            body: root.method === "both"
                                ? qsTr("Phương thức 2 lớp cần tệp khóa khôi phục. Hãy chọn nơi lưu và cất thật cẩn thận.")
                                : qsTr("Tệp này giúp bạn mở Vault nếu quên passphrase. Lưu ở USB hoặc nơi tách biệt máy tính.") }
                        Rectangle {
                            visible: !root.hasRecovery
                            Layout.fillWidth: true
                            radius: AuroraTheme.radiusLg
                            color: AuroraTheme.sunk
                            border.width: 1.5; border.color: AuroraTheme.borderStrong
                            implicitHeight: 96
                            ColumnLayout {
                                anchors.centerIn: parent; spacing: AuroraTheme.sp2
                                Rectangle { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 46; Layout.preferredHeight: 46
                                    radius: 12; color: AuroraTheme.accentSoft
                                    FsIcon { anchors.centerIn: parent; name: "save"; sizePx: 22; color: AuroraTheme.accent } }
                                Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("Chọn nơi lưu tệp khóa (.key)")
                                    font.family: AuroraTheme.fontSans; font.pixelSize: 14; font.weight: Font.Bold; color: AuroraTheme.ink1 }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: keyfileDialog.open() }
                        }
                        ColumnLayout {
                            visible: root.hasRecovery
                            Layout.fillWidth: true; spacing: AuroraTheme.sp2
                            Rectangle {
                                Layout.fillWidth: true; radius: AuroraTheme.radiusLg
                                color: AuroraTheme.successSoft
                                border.width: 1; border.color: Qt.rgba(AuroraTheme.success.r, AuroraTheme.success.g, AuroraTheme.success.b, 0.2)
                                implicitHeight: 56
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: AuroraTheme.sp4; spacing: AuroraTheme.sp3
                                    FsIcon { name: "shield-check"; sizePx: 22; color: AuroraTheme.success }
                                    ColumnLayout { Layout.fillWidth: true; spacing: 1
                                        Text { text: qsTr("Sẽ tạo tệp khóa sau khi tạo Vault"); font.family: AuroraTheme.fontSans
                                            font.pixelSize: 13; font.weight: Font.DemiBold; color: AuroraTheme.ink1 }
                                        Text { Layout.fillWidth: true; text: root.recoveryPath; elide: Text.ElideMiddle
                                            font.family: AuroraTheme.fontMono; font.pixelSize: 11; color: AuroraTheme.ink3 }
                                    }
                                }
                            }
                            FsCallout { Layout.fillWidth: true; tone: "warn"; compact: true
                                body: qsTr("Đừng để chung với máy đang dùng. Cất ở USB hoặc nơi tách biệt.") }
                        }
                    }

                    // ── WIZ-6 Name & location ──
                    ColumnLayout {
                        visible: root.step === 6
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp4
                        WizTitle { title: qsTr("Đặt tên & vị trí Vault") }
                        FsTextField { Layout.fillWidth: true; label: qsTr("Tên Vault"); leadingIcon: "vault"
                            text: root.vName; onTextChanged: root.vName = text }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: AuroraTheme.sp1
                            Text { text: qsTr("Thư mục Vault"); font.family: AuroraTheme.fontSans; font.pixelSize: 12
                                font.weight: Font.DemiBold; color: AuroraTheme.ink2 }
                            RowLayout {
                                Layout.fillWidth: true; spacing: AuroraTheme.sp2
                                Rectangle {
                                    Layout.fillWidth: true; Layout.preferredHeight: 44; radius: AuroraTheme.radiusMd
                                    color: AuroraTheme.sunk; border.width: 1; border.color: AuroraTheme.border
                                    RowLayout { anchors.fill: parent; anchors.leftMargin: AuroraTheme.sp3; anchors.rightMargin: AuroraTheme.sp3; spacing: AuroraTheme.sp2
                                        FsIcon { name: "folder"; sizePx: 16; color: AuroraTheme.ink3 }
                                        Text { Layout.fillWidth: true; text: root.location; elide: Text.ElideMiddle
                                            font.family: AuroraTheme.fontMono; font.pixelSize: 12; color: AuroraTheme.ink2 } }
                                }
                                FsButton { text: qsTr("Đổi…"); variant: "ghost"; onClicked: folderDialog.open() }
                            }
                        }
                        // Key storage level
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: AuroraTheme.sp2
                            Text { text: qsTr("Cấp lưu khóa"); font.family: AuroraTheme.fontSans; font.pixelSize: 12
                                font.weight: Font.DemiBold; color: AuroraTheme.ink2 }
                            Repeater {
                                model: [
                                    { id: "high", t: qsTr("Cao"), d: qsTr("Nhập passphrase mỗi lần mở app (an toàn nhất)."), badge: "" },
                                    { id: "medium", t: qsTr("Vừa"), d: qsTr("Nhớ qua Windows (DPAPI), tự khóa khi đóng app."), badge: "def" },
                                    { id: "convenient", t: qsTr("Tiện lợi"), d: qsTr("Không tự khóa."), badge: "warn" }
                                ]
                                delegate: Rectangle {
                                    id: lvlCard
                                    required property var modelData
                                    readonly property bool sel: root.level === lvlCard.modelData.id
                                    Layout.fillWidth: true
                                    radius: AuroraTheme.radiusMd
                                    color: sel ? AuroraTheme.accentSoft : "transparent"
                                    border.width: 1.5; border.color: sel ? AuroraTheme.accent : AuroraTheme.border
                                    implicitHeight: 54
                                    RowLayout {
                                        anchors.fill: parent; anchors.margins: AuroraTheme.sp3; spacing: AuroraTheme.sp3
                                        Rectangle { Layout.preferredWidth: 18; Layout.preferredHeight: 18; radius: 9
                                            color: "transparent"; border.width: 1.5; border.color: lvlCard.sel ? AuroraTheme.accent : AuroraTheme.borderStrong
                                            Rectangle { anchors.centerIn: parent; width: 9; height: 9; radius: 4.5; visible: lvlCard.sel; color: AuroraTheme.accent } }
                                        ColumnLayout { Layout.fillWidth: true; spacing: 1
                                            RowLayout { spacing: AuroraTheme.sp2
                                                Text { text: lvlCard.modelData.t; font.family: AuroraTheme.fontSans; font.pixelSize: 13
                                                    font.weight: Font.DemiBold; color: AuroraTheme.ink1 }
                                                FsBadge { visible: lvlCard.modelData.badge === "def"; text: qsTr("Mặc định"); variant: "accent" }
                                                FsBadge { visible: lvlCard.modelData.badge === "warn"; text: qsTr("Cảnh báo"); variant: "warn" } }
                                            Text { Layout.fillWidth: true; text: lvlCard.modelData.d; wrapMode: Text.WordWrap
                                                font.family: AuroraTheme.fontSans; font.pixelSize: 12; color: AuroraTheme.ink3 } }
                                    }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.level = lvlCard.modelData.id }
                                }
                            }
                            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap
                                text: qsTr("Dù chọn mức này, hãy vẫn ghi nhớ/sao lưu passphrase — cần khi đổi hoặc cài lại máy.")
                                font.family: AuroraTheme.fontSans; font.pixelSize: 11; color: AuroraTheme.ink4 }
                        }
                        // Advanced (key strength)
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: AuroraTheme.sp2
                            FsButton { text: qsTr("Phương pháp mã hóa (Nâng cao)"); variant: "link"
                                icon: root.showAdv ? "chevron-down" : "chevron-right"; onClicked: root.showAdv = !root.showAdv }
                            ColumnLayout {
                                visible: root.showAdv
                                Layout.fillWidth: true; spacing: AuroraTheme.sp2
                                FsCallout { Layout.fillWidth: true; tone: "info"; compact: true
                                    body: qsTr("FsNext tự chọn cách mã hóa tối ưu theo kích thước file. Mặc định đã an toàn cho hầu hết mọi người.") }
                                Text { text: qsTr("Độ mạnh khóa"); font.family: AuroraTheme.fontSans; font.pixelSize: 11; color: AuroraTheme.ink4 }
                                FsSegmentedControl {
                                    options: [ { value: 0, label: qsTr("Cân bằng") },
                                               { value: 1, label: qsTr("Mạnh") },
                                               { value: 2, label: qsTr("Tối đa") } ]
                                    selectedValue: root.kdfStrength
                                    onSelectionChanged: (value) => root.kdfStrength = value
                                }
                            }
                        }
                    }

                    // ── WIZ-7 Done ──
                    ColumnLayout {
                        visible: root.step === 7
                        Layout.fillWidth: true
                        Layout.margins: AuroraTheme.sp6
                        spacing: AuroraTheme.sp4
                        WizHero { iconName: "shield-check"; success: true; title: qsTr("Vault đã sẵn sàng!")
                            body: qsTr("Từ giờ bạn có thể thêm file vào Vault và mọi thứ sẽ được mã hóa ngay trên máy.") }
                        Rectangle {
                            Layout.fillWidth: true; radius: AuroraTheme.radiusLg
                            color: "transparent"; border.width: 1; border.color: AuroraTheme.border
                            implicitHeight: doneCol.implicitHeight
                            ColumnLayout {
                                id: doneCol
                                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                                spacing: 0
                                Repeater {
                                    model: [
                                        { k: qsTr("Tên Vault"), v: root.vName },
                                        { k: qsTr("Vị trí"), v: root.location },
                                        { k: qsTr("Cấp bảo mật"), v: root.level === "high" ? qsTr("Cao") : (root.level === "convenient" ? qsTr("Tiện lợi") : qsTr("Vừa")) },
                                        { k: qsTr("Khóa khôi phục"), v: root.hasRecovery ? qsTr("Có") : qsTr("Chưa có") }
                                    ]
                                    delegate: Item {
                                        id: drow
                                        required property int index
                                        required property var modelData
                                        Layout.fillWidth: true
                                        implicitHeight: 44
                                        RowLayout { anchors.fill: parent; anchors.leftMargin: AuroraTheme.sp4; anchors.rightMargin: AuroraTheme.sp4; spacing: AuroraTheme.sp3
                                            Text { Layout.preferredWidth: 120; text: drow.modelData.k; font.family: AuroraTheme.fontSans
                                                font.pixelSize: 12; color: AuroraTheme.ink3 }
                                            Text { Layout.fillWidth: true; text: drow.modelData.v; elide: Text.ElideMiddle
                                                font.family: AuroraTheme.fontSans; font.pixelSize: 13; font.weight: Font.DemiBold; color: AuroraTheme.ink1 } }
                                        Rectangle { visible: drow.index < 3; anchors.bottom: parent.bottom; width: parent.width; height: 1; color: AuroraTheme.divider }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Footer ──
            Item {
                id: footer
                Layout.fillWidth: true
                Layout.preferredHeight: 68
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: AuroraTheme.border }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AuroraTheme.sp6
                    anchors.rightMargin: AuroraTheme.sp6
                    spacing: AuroraTheme.sp2

                    FsButton { visible: root.step > 1 && root.step < 7; text: qsTr("Quay lại"); variant: "ghost"; icon: "arrow-left"
                        enabled: !root.creating; onClicked: root._back() }
                    FsButton { visible: root.step < 7; text: qsTr("Hủy"); variant: "ghost"
                        enabled: !root.creating; onClicked: root.close() }
                    Item { Layout.fillWidth: true }

                    // Per-step primary action
                    Text {
                        visible: root.step === 4 && !root._allAck
                        text: qsTr("Hãy xác nhận cả 4 mục.")
                        font.family: AuroraTheme.fontSans; font.pixelSize: 11; color: AuroraTheme.ink4
                    }
                    Text {
                        visible: root.errorText !== ""
                        text: root.errorText; color: AuroraTheme.danger
                        font.family: AuroraTheme.fontSans; font.pixelSize: 12
                    }
                    FsButton {
                        visible: root.step === 1
                        text: qsTr("Bắt đầu"); variant: "primary"
                        onClicked: root._next()
                    }
                    FsButton {
                        visible: root.step === 2
                        text: qsTr("Tiếp tục"); variant: "primary"
                        onClicked: root._next()
                    }
                    FsButton {
                        visible: root.step === 3
                        text: qsTr("Tiếp tục"); variant: "primary"
                        enabled: root._passOk && (root.generated || root._matchOk)
                        onClicked: root._next()
                    }
                    FsButton {
                        visible: root.step === 4
                        text: qsTr("Tôi hiểu, tiếp tục"); variant: "primary"
                        enabled: root._allAck
                        onClicked: root._next()
                    }
                    FsButton {
                        visible: root.step === 5 && root.method !== "both" && !root.hasRecovery
                        text: qsTr("Bỏ qua"); variant: "ghost"
                        onClicked: root._next()
                    }
                    FsButton {
                        visible: root.step === 5
                        text: qsTr("Tiếp tục"); variant: "primary"
                        enabled: !(root.method === "both" && !root.hasRecovery)
                        onClicked: root._next()
                    }
                    FsButton {
                        visible: root.step === 6
                        text: root.creating ? qsTr("Đang tạo…") : qsTr("Tạo Vault")
                        variant: "primary"; icon: "shield-check"
                        enabled: !root.creating && root.vName.length > 0
                        onClicked: root._doCreate()
                    }
                    FsButton {
                        visible: root.step === 7
                        text: qsTr("Đến trang Vault"); variant: "primary"; icon: "vault"
                        onClicked: { root.close(); root.finished(); }
                    }
                }
            }
        }
    }

    // ── Local sub-components ──
    component WizHero: ColumnLayout {
        id: wizHero
        property string iconName: ""
        property string title: ""
        property string body: ""
        property bool success: false
        Layout.fillWidth: true
        spacing: AuroraTheme.sp3
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 72; Layout.preferredHeight: 72; radius: 20
            color: wizHero.success ? AuroraTheme.successSoft : AuroraTheme.accentSoft
            FsIcon { anchors.centerIn: parent; name: wizHero.iconName; sizePx: 34
                color: wizHero.success ? AuroraTheme.success : AuroraTheme.accent }
        }
        Text { Layout.alignment: Qt.AlignHCenter; text: wizHero.title; horizontalAlignment: Text.AlignHCenter
            font.family: AuroraTheme.fontSans; font.pixelSize: AuroraTheme.h2.pixelSize; font.weight: Font.Bold; color: AuroraTheme.ink1 }
        Text { Layout.fillWidth: true; visible: wizHero.body !== ""; text: wizHero.body; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
            font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink3 }
    }
    component WizTitle: ColumnLayout {
        id: wizTitle
        property string title: ""
        property string sub: ""
        Layout.fillWidth: true
        spacing: 2
        Text { text: wizTitle.title; font.family: AuroraTheme.fontSans; font.pixelSize: AuroraTheme.h2.pixelSize
            font.weight: Font.Bold; color: AuroraTheme.ink1 }
        Text { Layout.fillWidth: true; visible: wizTitle.sub !== ""; text: wizTitle.sub; wrapMode: Text.WordWrap
            font.family: AuroraTheme.fontSans; font.pixelSize: 13; color: AuroraTheme.ink3 }
    }
}
