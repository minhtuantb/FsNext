// SPDX-License-Identifier: Proprietary
// FsFontLoader — registers the Aurora fonts from qrc:/fonts/ into the Qt
// font database. Ship one instance somewhere in the Main.qml scope graph
// (ApplicationWindow root) so the FontLoaders stay alive for the session.
//
// If scripts/fetch-aurora-fonts.ps1 hasn't been run, these load attempts
// silently fail and the design falls back to system sans / serif. The
// visual difference is: Geist → Segoe UI / SF Pro; Instrument Serif →
// Georgia. Glyphs are similar, metrics differ slightly.

import QtQuick

Item {
    // No visual representation; purely side-effect loaders.
    visible: false
    width: 0
    height: 0

    FontLoader { source: "qrc:/fonts/Geist-Variable.ttf" }
    FontLoader { source: "qrc:/fonts/GeistMono-Variable.ttf" }
    FontLoader { source: "qrc:/fonts/InstrumentSerif-Regular.ttf" }
    FontLoader { source: "qrc:/fonts/InstrumentSerif-Italic.ttf" }
    FontLoader { source: "qrc:/fonts/BeVietnamPro-Regular.ttf" }
    FontLoader { source: "qrc:/fonts/BeVietnamPro-Medium.ttf" }
    FontLoader { source: "qrc:/fonts/BeVietnamPro-SemiBold.ttf" }
    FontLoader { source: "qrc:/fonts/BeVietnamPro-Bold.ttf" }
}
