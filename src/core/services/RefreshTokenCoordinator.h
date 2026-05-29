#pragma once

// Implements the silent re-authentication contract described in
// docs/refresh-token-spec (section 4) on top of Fshare's
// /api/user/refreshToken endpoint.
//
// Lives next to AuthService but is intentionally separate so FshareApi can
// drive the gate (ensureFreshToken / handleAuthExpired) directly from its
// worker threads without taking a service-layer dependency.
//
// Threading model
// ───────────────
// FshareApi is synchronous and is called from QtConcurrent thread-pool jobs
// kicked off by the services.  Multiple worker threads can therefore hit the
// coordinator simultaneously.  All public methods are therefore protected by
// QMutex + QWaitCondition: at most one HTTP refresh request is ever in flight
// (single-flight, spec 2.1 #2), and the followers block on m_inflightDone
// until the leader is done.
//
// We deliberately do NOT hold the mutex across the HTTP call — only across
// state transitions — so a slow refresh never serialises business calls
// that just need to read m_token.

#include <QObject>
#include <QMutex>
#include <QString>
#include <QWaitCondition>
#include <cstdint>

namespace fsnext {

class HttpClient;
class SettingsRepository;

// Public categorisation of refresh outcomes.  Mirrors spec section 3.4.
//   HardFail  → token will never refresh; UI must force re-login.
//   SoftFail  → transient (timeout / network / unknown 5xx); keep old token,
//               show a non-blocking toast, retry on next gesture.
//   Success   → token + cookie updated in place; replay the original call.
enum class RefreshResultKind {
    Success,
    HardFail,
    SoftFail,
};

struct RefreshResult {
    RefreshResultKind kind = RefreshResultKind::SoftFail;
    QString           newToken;        // populated on Success only
    QString           newSessionId;    // populated on Success only
    // Optional cookie rebuild — populated on Success when the server sent
    // a Set-Cookie header.  Already includes the ";key=<app_key>" suffix
    // matching the format login() uses, so the caller can just pass this
    // straight to HttpClient::setCookie().
    QString           newCookie;
    QString           message;         // user-visible reason (already localised)
};

class RefreshTokenCoordinator : public QObject {
    Q_OBJECT
public:
    // Spec constants — keep in sync with documented values so any
    // cross-platform parity check passes.
    static constexpr qint64 kTokenSoftMaxAgeMs   = 50LL * 60 * 1000;     // 50 minutes
    static constexpr qint64 kRefreshHardWindowMs = 7LL  * 24 * 3600 * 1000; // 7 days
    static constexpr qint64 kRefreshBackoffMs    = 30LL * 1000;           // 30 seconds
    static constexpr int    kMaxConsecutiveFailures = 3;

    explicit RefreshTokenCoordinator(HttpClient *http,
                                     SettingsRepository *settings,
                                     QObject *parent = nullptr);
    ~RefreshTokenCoordinator() override = default;

    // The platform app_key for the desktop client.  Exposed so FshareApi can
    // reuse the same XOR-decoded key without duplicating the table.  Returns
    // a fresh QString each call (cheap).
    static QString appKey();

    // ── Session ownership ─────────────────────────────────────────────────
    //
    // The coordinator owns the canonical (token, sessionId, cookie) tuple.
    // FshareApi calls token()/cookie() before every request to read the
    // current pair atomically — this is what makes the "rotation in flight"
    // scenario in spec §6 #3 safe: a request issued just before refresh and
    // a request issued just after will see consistent values.
    QString token() const;
    QString sessionId() const;
    QString cookie() const;

    // Called by AuthService after any successful login (password/OAuth) to
    // seed the coordinator and reset failure counters.  This is the ONLY
    // way m_token can transition from empty → non-empty.
    void onLoginSuccess(const QString &token,
                        const QString &sessionId,
                        const QString &cookie);

    // Wipe all state — invoked on explicit logout and on hard-fail expiry.
    void onLogout();

    // Cold-start helper.  Reads the persisted lastRefreshAt timestamp from
    // SettingsRepository and returns false when it is older than the 7-day
    // refresh window: caller should drop the saved token without even
    // attempting a refresh (spec §2.1 #8 + §6 edge case #1).
    //
    // Does NOT load the token/cookie itself — those still live in the
    // SettingsRepository alongside the encrypted password.  AuthService
    // remains responsible for credential persistence; we only own the
    // freshness window.
    bool isPersistedSessionWithinWindow() const;

    // ── Refresh gates ─────────────────────────────────────────────────────
    //
    // Spec §4.6 — call at the start of every authenticated FshareApi method.
    // If the token is older than kTokenSoftMaxAgeMs, blocks briefly while a
    // refresh runs in the foreground.  Returns true when the caller now has
    // a usable token (either the refresh succeeded, or we never needed to
    // refresh).  Returns false when the refresh hard-failed — the caller
    // should surface AppError::auth() and abort.
    //
    // Soft fails (network/timeout) also return true: we keep the old token
    // and let the API call try anyway — the server might still accept it.
    bool ensureFreshToken();

    // Spec §4.7 — call after an authenticated request returned HTTP 201
    // (or any auth-category error).  Drives a blocking refresh and returns
    // a Success/HardFail/SoftFail verdict so the caller can decide whether
    // to retry the original request.
    //
    // Concurrent callers see exactly one network round-trip thanks to the
    // single-flight gate.  All of them wake with the same RefreshResult.
    RefreshResult handleAuthExpired();

signals:
    // UI hooks.  Always emitted on the QObject's affinity thread (the GUI
    // thread for typical usage) via queued connections from the refresh
    // worker thread.
    void tokenRefreshing();
    void tokenRefreshed();
    void tokenRefreshFailed(const QString &message); // soft-fail toast
    void sessionExpired(const QString &message);     // hard-fail → kick to Login

private:
    // Returns the time since the last successful refresh in milliseconds,
    // or -1 if we have never refreshed in this process and have no
    // persisted timestamp either.
    qint64 tokenAgeMs() const;

    // Spec §3.4 — classify response status + body code as hard vs soft.
    static bool isHardAuthFailure(int httpStatus, int apiCode);

    // Drive the actual /api/user/refreshToken round-trip.  Caller MUST hold
    // m_mutex AND must have transitioned the state to InFlight first.
    // Releases m_mutex during the HTTP call (passing it in is purely so we
    // can do the relock atomically before mutating state on return).
    RefreshResult performRefresh(QMutexLocker<QMutex> &lock);

    // Spec §4.5 helper — single-shot guard so a runaway refresh storm only
    // fires sessionExpired once until login resets the flag.
    void emitSessionExpiredOnce(const QString &msg);

    // Persist lastRefreshAt only.  Token + cookie persistence stays with
    // AuthService so we don't duplicate the DPAPI encryption logic.
    void persistLastRefreshAt();
    qint64 loadPersistedLastRefreshAt() const;

    HttpClient         *m_http     = nullptr;
    SettingsRepository *m_settings = nullptr;

    // State machine.  Idle ↔ InFlight on every refresh; Failed is sticky
    // until either onLoginSuccess() or a successful refresh resets it.
    enum class State { Idle, InFlight, Failed };

    mutable QMutex   m_mutex;
    QWaitCondition   m_inflightDone;

    QString          m_token;
    QString          m_sessionId;
    QString          m_cookie;

    State            m_state = State::Idle;
    qint64           m_lastRefreshAtMs       = 0;
    qint64           m_lastRefreshFailedAtMs = 0;
    int              m_consecutiveFailures   = 0;
    bool             m_sessionExpiredEmitted = false;

    // Cached result of the last refresh, broadcast to followers blocked on
    // m_inflightDone.  Spec §2.1 #2: every concurrent caller observes the
    // SAME outcome.
    RefreshResult    m_lastResult;
};

} // namespace fsnext
