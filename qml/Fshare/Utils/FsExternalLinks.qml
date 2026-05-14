// SPDX-License-Identifier: Proprietary
// FsExternalLinks — Single source of truth for every URL we hand off to
// the system browser via Qt.openUrlExternally().
//
// Why a singleton: account-management features (đổi mật khẩu, 2FA, VIP
// history…) intentionally live on fshare.vn instead of being re-implemented
// in the desktop client.  Keeping every URL here means:
//   • One file to update when fshare.vn restructures (no grep across QML)
//   • Easy audit of what we redirect for ("liệt kê chỗ nào dẫn ra ngoài")
//   • Comments per URL flag CONFIRMED vs UNVERIFIED (search "UNVERIFIED").
//
// Usage:
//   import Fshare.Utils 1.0
//   onClicked: Qt.openUrlExternally(FsExternalLinks.profile)
//
// Product decision (2026-05-14): every per-feature account URL collapses
// onto a single `/account/profile` landing page.  Sub-features (2FA,
// password change, VIP history, payments) live as sections on that page —
// so the desktop hands off with the right intent label, but the user
// always lands somewhere known on fshare.vn.

pragma Singleton
import QtQuick

QtObject {
    // ── Brand / marketing (confirmed) ──────────────────────────────────────
    readonly property string home:        "https://www.fshare.vn/"
    readonly property string login:       "https://www.fshare.vn/login"
    readonly property string upgrade:     "https://www.fshare.vn/upgrade"

    // ── Auth flows the desktop doesn't host itself ─────────────────────────
    // Per product decision (2026-05-14): signup + forgot-password both
    // route to the fshare.vn homepage.  The web header surfaces the
    // "Đăng ký" / "Quên mật khẩu" entry points from there, so we don't
    // hardcode a deep slug that could break when web restructures auth.
    readonly property string signup:           "https://www.fshare.vn/"
    readonly property string forgotPassword:   "https://www.fshare.vn/"

    // ── Account management — all collapse to /account/profile ──────────────
    // Per product decision: one canonical landing page covers every
    // sub-feature (đổi mật khẩu, 2FA, đổi email, VIP, payment).  Labels
    // in the UI keep the user's intent visible; the destination doesn't
    // change.  If fshare.vn later splits these into separate pages, this
    // is the only file that needs touching.
    readonly property string profile:          "https://www.fshare.vn/account/profile"
    readonly property string changePassword:   "https://www.fshare.vn/account/profile"
    readonly property string changeEmail:      "https://www.fshare.vn/account/profile"
    readonly property string editProfile:      "https://www.fshare.vn/account/profile"
    readonly property string twoFactor:        "https://www.fshare.vn/account/profile"
    readonly property string vipHistory:       "https://www.fshare.vn/account/profile"
    readonly property string paymentHistory:   "https://www.fshare.vn/account/profile"

    // ── Help & support — point at homepage for now ─────────────────────────
    // Until Fshare publishes dedicated /terms /privacy /support pages,
    // we land users on the homepage where the global footer can route them.
    readonly property string support:          "https://www.fshare.vn/"
    readonly property string termsOfService:   "https://www.fshare.vn/"
    readonly property string privacyPolicy:    "https://www.fshare.vn/"

    // ── Public file / folder share URLs (templated) ────────────────────────
    // These are built by callers using the linkcode; functions return a
    // ready-to-open URL so callers don't reassemble strings inline.
    function fileUrl(linkcode)   { return "https://www.fshare.vn/file/"   + linkcode; }
    function folderUrl(linkcode) { return "https://www.fshare.vn/folder/" + linkcode; }
}
