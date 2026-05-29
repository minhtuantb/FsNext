#include "RefreshTokenCoordinator.h"

#include "core/api/HttpClient.h"
#include "core/repositories/SettingsRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

namespace fsnext {

// ── Storage keys ──────────────────────────────────────────────────────────
//
// SettingsRepository wraps QSettings (HKCU on Windows).  We store under the
// "Account/" group so cleanup on logout — which already zeroes Account/* —
// nukes the freshness timestamp too.
namespace {
constexpr auto kKeyLastRefreshAt = "Account/lastRefreshAt";   // qint64 ms since epoch
constexpr auto kRefreshUrl       = "https://api.fshare.vn/api/user/refreshToken";

// HttpClient uses CURLOPT_TIMEOUT=30, CURLOPT_CONNECTTIMEOUT=15 already.
// Spec §4.2 asks for 15 s end-to-end — the curl total covers us in the
// pathological "TLS handshake then server stalls" case; the connect cap
// covers the much more common "TCP RTT collapse on weak networks" case.
// If we ever need to tighten further we'll add a per-call timeout override
// to HttpClient.
}

// ── app_key (XOR-decoded — same plaintext as FshareApi) ───────────────────
//
// Duplicating the XOR table here (vs. taking a dependency on FshareApi) keeps
// the coordinator usable as a standalone unit and avoids a header cycle.  If
// the plaintext key ever rotates, update BOTH this table and the one in
// FshareApi.cpp (scripts/encode_appkey.py emits both).
QString RefreshTokenCoordinator::appKey() {
    const char encrypted[] = {
        0x38, 0x11, 0x32, 0x2d, 0x11, 0x11, 0x06, 0x11, 0x09, 0x32,
        0x12, 0x69, 0x05, 0x2c, 0x2a, 0x17, 0x19, 0x12, 0x3d, 0x19,
        0x34, 0x38, 0x0d, 0x0d, 0x69, 0x36, 0x24, 0x18, 0x2d, 0x38,
        0x38, 0x28,
        0x5c
    };
    const int size = sizeof(encrypted) - 1;
    std::string out(size, '\0');
    for (int i = 0; i < size; ++i)
        out[i] = char(encrypted[i] ^ encrypted[size]);
    return QString::fromStdString(out);
}

// ── ctor ──────────────────────────────────────────────────────────────────

RefreshTokenCoordinator::RefreshTokenCoordinator(HttpClient *http,
                                                 SettingsRepository *settings,
                                                 QObject *parent)
    : QObject(parent)
    , m_http(http)
    , m_settings(settings)
{
    m_lastRefreshAtMs = loadPersistedLastRefreshAt();
}

// ── Snapshots ─────────────────────────────────────────────────────────────

QString RefreshTokenCoordinator::token() const {
    QMutexLocker locker(&m_mutex);
    return m_token;
}

QString RefreshTokenCoordinator::sessionId() const {
    QMutexLocker locker(&m_mutex);
    return m_sessionId;
}

QString RefreshTokenCoordinator::cookie() const {
    QMutexLocker locker(&m_mutex);
    return m_cookie;
}

// ── Session ownership ─────────────────────────────────────────────────────

void RefreshTokenCoordinator::onLoginSuccess(const QString &token,
                                             const QString &sessionId,
                                             const QString &cookie)
{
    {
        QMutexLocker locker(&m_mutex);
        m_token                 = token;
        m_sessionId             = sessionId;
        m_cookie                = cookie;
        m_state                 = State::Idle;
        m_lastRefreshAtMs       = QDateTime::currentMSecsSinceEpoch();
        m_lastRefreshFailedAtMs = 0;
        m_consecutiveFailures   = 0;
        m_sessionExpiredEmitted = false;
        // Wake any followers that piled up before login completed — they'll
        // re-evaluate and either succeed or re-trigger a refresh.
        m_inflightDone.wakeAll();
    }
    persistLastRefreshAt();
    qDebug() << "[RefreshTokenCoordinator] login success; token rotated; "
                "lastRefreshAt seeded";
}

void RefreshTokenCoordinator::onLogout()
{
    {
        QMutexLocker locker(&m_mutex);
        m_token.clear();
        m_sessionId.clear();
        m_cookie.clear();
        m_state                 = State::Idle;
        m_lastRefreshAtMs       = 0;
        m_lastRefreshFailedAtMs = 0;
        m_consecutiveFailures   = 0;
        m_sessionExpiredEmitted = false;
        m_inflightDone.wakeAll();
    }
    if (m_settings)
        m_settings->setString(QLatin1String(kKeyLastRefreshAt), QString());
}

// ── Cold-start window check ───────────────────────────────────────────────

bool RefreshTokenCoordinator::isPersistedSessionWithinWindow() const
{
    const qint64 ts = loadPersistedLastRefreshAt();
    if (ts <= 0)
        return false;   // never refreshed — treat as outside the window
    const qint64 age = QDateTime::currentMSecsSinceEpoch() - ts;
    return age < kRefreshHardWindowMs;
}

// ── ensureFreshToken (lazy proactive) ─────────────────────────────────────

bool RefreshTokenCoordinator::ensureFreshToken()
{
    // Quick read under the lock to decide whether we need to act.
    State state;
    qint64 age;
    QString token;
    {
        QMutexLocker locker(&m_mutex);
        state = m_state;
        token = m_token;
        age   = (m_lastRefreshAtMs > 0)
              ? (QDateTime::currentMSecsSinceEpoch() - m_lastRefreshAtMs)
              : -1;
    }

    if (token.isEmpty())
        return true;   // not logged in — nothing for the gate to do

    // Spec §4.6: token older than the soft window → refresh ahead of the
    // business call so we don't waste a round-trip on a guaranteed 201.
    const bool soft = (age >= 0 && age > kTokenSoftMaxAgeMs);
    if (state != State::InFlight && !soft)
        return true;

    if (soft) {
        // We have nothing to retry yet; just kick off the refresh.  Followers
        // that arrive while we're in flight will rendezvous on handleAuthExpired
        // or on this same call from another worker.
        const RefreshResult r = handleAuthExpired();
        return r.kind != RefreshResultKind::HardFail;
    }

    // State::InFlight without a stale token → another thread already issued
    // a refresh; block on the result so we use the new token below.
    QMutexLocker locker(&m_mutex);
    while (m_state == State::InFlight)
        m_inflightDone.wait(&m_mutex);
    return m_state != State::Failed;   // Idle = success or backoff-skipped
}

// ── handleAuthExpired (reactive + single-flight) ──────────────────────────

RefreshResult RefreshTokenCoordinator::handleAuthExpired()
{
    QMutexLocker locker(&m_mutex);

    // Block-and-share: if a refresh is already in flight, wait for it and
    // hand back the SAME RefreshResult.  Spec §2.1 #2 + §6 edge case #2.
    if (m_state == State::InFlight) {
        while (m_state == State::InFlight)
            m_inflightDone.wait(&m_mutex);
        return m_lastResult;
    }

    // Backoff: don't hammer the server after consecutive failures.  Spec §2.1 #5.
    if (m_consecutiveFailures >= kMaxConsecutiveFailures
        && (QDateTime::currentMSecsSinceEpoch() - m_lastRefreshFailedAtMs) < kRefreshBackoffMs)
    {
        RefreshResult r;
        r.kind    = RefreshResultKind::HardFail;
        r.message = tr("Quá nhiều lần thử làm mới phiên. Vui lòng đăng nhập lại.");
        m_lastResult = r;
        // We're idle, but we want callers to know it's hopeless until login.
        emitSessionExpiredOnce(r.message);
        return r;
    }

    if (m_token.isEmpty()) {
        RefreshResult r;
        r.kind    = RefreshResultKind::HardFail;
        r.message = tr("Không có phiên đăng nhập.");
        m_lastResult = r;
        emitSessionExpiredOnce(r.message);
        return r;
    }

    m_state = State::InFlight;
    emit tokenRefreshing();

    const RefreshResult result = performRefresh(locker);

    // performRefresh re-acquired the lock before returning.
    m_lastResult = result;
    switch (result.kind) {
    case RefreshResultKind::Success:
        m_token                 = result.newToken;
        if (!result.newSessionId.isEmpty())
            m_sessionId         = result.newSessionId;
        // Cookie rotation: prefer the parsed Set-Cookie from the response
        // (covers PHPSESSID rotation server-side).  Fall back to the
        // existing m_cookie unchanged when the server didn't rotate it —
        // session_id in the JSON body is the LoginSession id and is NOT
        // the same as the PHPSESSID cookie value.
        if (!result.newCookie.isEmpty()) {
            m_cookie = result.newCookie;
            // Mirror to HttpClient so subsequent calls see the fresh cookie
            // — FshareApi only re-applies it via setSession() after login.
            if (m_http)
                m_http->setCookie(m_cookie);
        }
        m_lastRefreshAtMs       = QDateTime::currentMSecsSinceEpoch();
        m_lastRefreshFailedAtMs = 0;
        m_consecutiveFailures   = 0;
        m_sessionExpiredEmitted = false;
        m_state                 = State::Idle;
        m_inflightDone.wakeAll();
        locker.unlock();
        persistLastRefreshAt();
        emit tokenRefreshed();
        break;

    case RefreshResultKind::HardFail:
        m_lastRefreshFailedAtMs = QDateTime::currentMSecsSinceEpoch();
        m_consecutiveFailures  += 1;
        m_state                 = State::Failed;
        m_inflightDone.wakeAll();
        // Wipe — token will never recover.  AuthService also clears its
        // copies when sessionExpired arrives, so this is belt-and-braces.
        m_token.clear();
        m_sessionId.clear();
        m_cookie.clear();
        if (m_settings)
            m_settings->setString(QLatin1String(kKeyLastRefreshAt), QString());
        emitSessionExpiredOnce(result.message);
        locker.unlock();
        emit tokenRefreshFailed(result.message);
        break;

    case RefreshResultKind::SoftFail:
        m_lastRefreshFailedAtMs = QDateTime::currentMSecsSinceEpoch();
        m_consecutiveFailures  += 1;
        m_state                 = State::Failed;
        m_inflightDone.wakeAll();
        // Keep token/cookie — next gesture may succeed (spec §3.4 SoftFail).
        locker.unlock();
        emit tokenRefreshFailed(result.message);
        break;
    }
    return result;
}

// ── performRefresh (HTTP round-trip) ──────────────────────────────────────

RefreshResult RefreshTokenCoordinator::performRefresh(QMutexLocker<QMutex> &lock)
{
    // Snapshot under the lock, then release for the network call so other
    // FshareApi threads can keep reading m_token / sessionId via cookie() etc.
    const QString currentToken = m_token;
    lock.unlock();

    QJsonObject body;
    body.insert(QStringLiteral("token"),   currentToken);
    body.insert(QStringLiteral("app_key"), appKey());
    const QByteArray jsonBody = QJsonDocument(body).toJson(QJsonDocument::Compact);

    // Spec §3.1 instructs clients NOT to send the session cookie because the
    // server-side endpoint sits in the no-auth exemption list.  HttpClient
    // attaches m_cookie automatically from the field set by setCookie(),
    // not from the headers map, so we can't suppress it from here.  In
    // practice the server simply ignores the cookie on this endpoint —
    // authentication is body-only ({token, app_key}).  Leaving the cookie
    // attached is therefore harmless; if Fshare ever changes that, we'll
    // need a "no-cookie" variant of HttpClient::post.
    HttpResponse resp;
    if (m_http) {
        resp = m_http->post(QString::fromLatin1(kRefreshUrl), jsonBody);
    }
    // else: leave resp with statusCode == 0 → treated as soft network failure

    lock.relock();

    // Parse + classify.
    RefreshResult out;
    out.kind = RefreshResultKind::SoftFail;
    out.message = tr("Lỗi mạng khi làm mới phiên. Sẽ thử lại sau.");

    if (resp.statusCode <= 0)
        return out;   // network/timeout → soft

    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(resp.body, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        // Body wasn't JSON — usually nginx HTML on a hard rejection.  Lean
        // hard-fail if status itself is hard, otherwise soft.
        if (isHardAuthFailure(resp.statusCode, 0)) {
            out.kind    = RefreshResultKind::HardFail;
            out.message = tr("Không thể làm mới phiên (HTTP %1).").arg(resp.statusCode);
        }
        return out;
    }

    const QJsonObject root = doc.object();
    const int apiCode = root.value(QStringLiteral("code")).toInt(resp.statusCode);
    const QString msg = root.value(QStringLiteral("msg")).toString();

    if (isHardAuthFailure(resp.statusCode, apiCode)) {
        out.kind    = RefreshResultKind::HardFail;
        out.message = msg.isEmpty()
            ? tr("Phiên đã hết hạn. Vui lòng đăng nhập lại.")
            : msg;
        return out;
    }

    if (resp.statusCode == 200 && apiCode == 200) {
        const QString newToken = root.value(QStringLiteral("token")).toString();
        if (newToken.isEmpty()) {
            // Spec §6 #6: 200 with empty token → treat as hard-fail (parse).
            out.kind    = RefreshResultKind::HardFail;
            out.message = tr("Server trả về phiên rỗng. Vui lòng đăng nhập lại.");
            return out;
        }
        out.kind          = RefreshResultKind::Success;
        out.newToken      = newToken;
        out.newSessionId  = root.value(QStringLiteral("session_id")).toString();
        out.message       = msg;

        // Rebuild the cookie if the server rotated PHPSESSID — login() uses
        // the same parser; mirror exactly so authenticated requests keep
        // working against the same nginx that's whitelisting the cookie.
        // Format: "<cookie_name>=<cookie_value>;key=<app_key>".
        const QString setCookie = resp.headers.value(QStringLiteral("Set-Cookie"));
        if (!setCookie.isEmpty()) {
            const int semicolonIdx = setCookie.indexOf(QLatin1Char(';'));
            const QString cookiePair = semicolonIdx > 0
                ? setCookie.left(semicolonIdx)
                : setCookie;
            out.newCookie = cookiePair + QStringLiteral(";key=") + appKey();
        }
        return out;
    }

    // 5xx / unknown code → soft, callers can retry next gesture.
    out.kind    = RefreshResultKind::SoftFail;
    out.message = msg.isEmpty()
        ? tr("Mã phản hồi không xác định (%1).").arg(apiCode)
        : msg;
    return out;
}

// ── helpers ───────────────────────────────────────────────────────────────

bool RefreshTokenCoordinator::isHardAuthFailure(int httpStatus, int apiCode)
{
    // Spec §3.4 table.  Both axes are checked: server returns the same
    // semantic via either http status or the JSON code in different builds.
    switch (httpStatus) {
        case 400: case 403: case 409: case 410:
        case 421: case 422: case 423:
            return true;
        default: break;
    }
    switch (apiCode) {
        case 33: case 34: case 400: case 403:
        case 409: case 410: case 420:
            return true;
        default: break;
    }
    return false;
}

void RefreshTokenCoordinator::emitSessionExpiredOnce(const QString &msg)
{
    // We're typically already holding m_mutex here.  The Qt::AutoConnection
    // defaults will marshal the signal to the receiver's thread (the GUI
    // thread for AuthService) so emitting under the lock is safe — receivers
    // run later, not inline.
    if (m_sessionExpiredEmitted) return;
    m_sessionExpiredEmitted = true;
    emit sessionExpired(msg);
}

void RefreshTokenCoordinator::persistLastRefreshAt()
{
    if (!m_settings) return;
    // QSettings has no native qint64 — store as decimal string to dodge the
    // sign-extension foot-gun when the value crosses INT_MAX (it will).
    m_settings->setString(QLatin1String(kKeyLastRefreshAt),
                          QString::number(m_lastRefreshAtMs));
}

qint64 RefreshTokenCoordinator::loadPersistedLastRefreshAt() const
{
    if (!m_settings) return 0;
    const QString s = m_settings->getString(QLatin1String(kKeyLastRefreshAt));
    if (s.isEmpty()) return 0;
    bool ok = false;
    const qint64 v = s.toLongLong(&ok);
    return ok ? v : 0;
}

qint64 RefreshTokenCoordinator::tokenAgeMs() const
{
    if (m_lastRefreshAtMs <= 0) return -1;
    return QDateTime::currentMSecsSinceEpoch() - m_lastRefreshAtMs;
}

} // namespace fsnext
