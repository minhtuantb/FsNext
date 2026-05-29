#include "UserInfoViewModel.h"
#include "core/services/AuthService.h"
#include "core/util/FormatUtil.h"

namespace fsnext {

UserInfoViewModel::UserInfoViewModel(AuthService *auth, QObject *parent)
    : QObject(parent)
    , m_auth(auth)
{
    connect(m_auth, &AuthService::userChanged, this, &UserInfoViewModel::userInfoChanged);
}

// ── Identity ──────────────────────────────────────────────────────────────

QString UserInfoViewModel::userId() const
{
    return m_auth ? m_auth->currentUser().id : QString{};
}

QString UserInfoViewModel::userName() const
{
    return m_auth ? m_auth->currentUser().name : QString{};
}

QString UserInfoViewModel::userEmail() const
{
    return m_auth ? m_auth->currentUser().email : QString{};
}

int UserInfoViewModel::userLevel() const
{
    return m_auth ? m_auth->currentUser().level : 0;
}

QString UserInfoViewModel::levelLabel() const
{
    if (!m_auth) return QString{};
    const auto &u = m_auth->currentUser();
    return levelToLabel(u.level, u.accountType);
}

bool UserInfoViewModel::isVip() const
{
    // Free tier is level 0-2 ("Thành viên thường"). Levels 3+ map to VIP,
    // Promo, Bundle, FSUB*, Forever, Storage etc. — any of those is a paid
    // tier and warrants the gradient VIP card treatment.
    return m_auth && m_auth->currentUser().level >= 3;
}

QString UserInfoViewModel::accountType() const
{
    return m_auth ? m_auth->currentUser().accountType : QString{};
}

QString UserInfoViewModel::joinDate() const
{
    return m_auth ? FormatUtil::formatDate(m_auth->currentUser().joinDate) : QString{};
}

QString UserInfoViewModel::vipExpiry() const
{
    if (!m_auth) return {};
    const auto &u = m_auth->currentUser();
    // Level 18 = Forever — never expires
    if (u.level == 18) return tr("Vĩnh viễn");
    return FormatUtil::formatDate(u.expireVip);
}

QString UserInfoViewModel::avatarUrl() const
{
    return m_auth ? m_auth->currentUser().avatarUrl : QString{};
}

// ── Points & quota ────────────────────────────────────────────────────────

int UserInfoViewModel::totalPoints() const
{
    return m_auth ? m_auth->currentUser().totalPoints : 0;
}

int UserInfoViewModel::dlTimeAvail() const
{
    return m_auth ? m_auth->currentUser().dlTimeAvail : 0;
}

// ── Non-secure webspace ───────────────────────────────────────────────────

qint64 UserInfoViewModel::webspaceTotal() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().webspace) : 0;
}

qint64 UserInfoViewModel::webspaceUsed() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().webspaceUsed) : 0;
}

qint64 UserInfoViewModel::webspaceFree() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().webspaceFree()) : 0;
}

// ── Secure webspace ───────────────────────────────────────────────────────

qint64 UserInfoViewModel::secureTotal() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().webspaceSecure) : 0;
}

qint64 UserInfoViewModel::secureUsed() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().webspaceSecureUsed) : 0;
}

qint64 UserInfoViewModel::secureFree() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().webspaceSecureFree()) : 0;
}

// ── Combined ──────────────────────────────────────────────────────────────

qint64 UserInfoViewModel::storageTotalAll() const
{
    return m_auth
        ? static_cast<qint64>(m_auth->currentUser().webspace +
                               m_auth->currentUser().webspaceSecure)
        : 0;
}

// ── Traffic ───────────────────────────────────────────────────────────────

qint64 UserInfoViewModel::trafficTotal() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().traffic) : 0;
}

qint64 UserInfoViewModel::trafficUsed() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().trafficUsed) : 0;
}

qint64 UserInfoViewModel::trafficFree() const
{
    return m_auth ? static_cast<qint64>(m_auth->currentUser().trafficFree()) : 0;
}

// ── Refresh ───────────────────────────────────────────────────────────────

void UserInfoViewModel::refresh()
{
    if (m_auth)
        m_auth->fetchUserInfo();
}

// ── Level → label mapping (Fshare account levels) ──────────

QString UserInfoViewModel::levelToLabel(int level, const QString &accountTypeFallback)
{
    switch (level) {
    case 0:
    case 1:
    case 2:  return QStringLiteral("Thành viên thường");
    case 3:
    case 4:
    case 5:
    case 6:  return QStringLiteral("VIP");
    case 7:  return QStringLiteral("Promo");
    case 8:  return QStringLiteral("PromoPlus");
    case 9:  return QStringLiteral("FSUB15");
    case 10: return QStringLiteral("FSUB10");
    case 11: return QStringLiteral("BUNDLE");
    case 16: return QStringLiteral("FSUB20");
    case 17: return QStringLiteral("Storage");
    case 18: return QStringLiteral("Forever");
    default:
        // Use server-provided account_type as fallback
        return accountTypeFallback.isEmpty()
            ? QStringLiteral("Thành viên thường")
            : accountTypeFallback;
    }
}

} // namespace fsnext
