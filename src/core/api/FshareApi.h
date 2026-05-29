#pragma once

#include "HttpClient.h"
#include "core/models/ApiResponse.h"
#include "core/models/FileItem.h"
#include "core/models/User.h"

#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVector>
#include <cstdint>
#include <type_traits>

namespace fsnext {

class RefreshTokenCoordinator;

class FshareApi {
public:
    explicit FshareApi(HttpClient *http);

    // Session
    void setSession(const QString &token, const QString &cookie);

    // Inject the silent-refresh coordinator (created by AppContext after the
    // SettingsRepository exists).  Until this is wired the API behaves like
    // the legacy build — no proactive refresh, 201s surface as auth errors
    // to the caller.  Safe to call exactly once during init; later changes
    // are unsupported and would race with in-flight calls.
    void setRefreshCoordinator(RefreshTokenCoordinator *coord);

    // Auth
    ApiResponse<Session>            login(const QString &email, const QString &password);
    // Social login (Google/Facebook/FPT ID). `service` must be one of the
    // Fshare-recognised strings: "google", "facebook", "fpt-id". The caller
    // already completed the provider-side OAuth flow and passes the provider's
    // access_token + the user's email here.
    ApiResponse<Session>            loginOauth(const QString &service,
                                               const QString &accessToken,
                                               const QString &email);
    ApiResponse<void>               logout();
    ApiResponse<User>               getUserInfo();

    // File listing & search
    ApiResponse<QVector<FileItem>>  listFiles(const QString &folderUrl, int page, int pageSize);
    ApiResponse<QVector<FileItem>>  listFolders(const QString &path);
    ApiResponse<FileItem>           getFileInfo(const QString &url);
    ApiResponse<QVector<FileItem>>  searchFiles(const QString &keyword, int page);

    // File operations
    ApiResponse<void>               renameFile(const QString &linkcode, const QString &newName);
    ApiResponse<void>               deleteFiles(const QStringList &linkcodes);
    ApiResponse<void>               createFolder(const QString &name, const QString &parentId);
    // Same as createFolder but addresses the parent by PATH ("/", "/Foo/Bar")
    // rather than linkcode. Used by Sync to mirror nested local subdirectories
    // on Fshare without needing to resolve ID chains.
    ApiResponse<void>               createFolderInPath(const QString &name, const QString &parentPath);
    ApiResponse<void>               moveFiles(const QStringList &linkcodes, const QString &to);
    ApiResponse<void>               copyFiles(const QStringList &linkcodes, const QString &to);

    // File settings
    ApiResponse<void>               changeSecure(const QStringList &linkcodes, bool secure);
    ApiResponse<void>               setFilePassword(const QStringList &linkcodes, const QString &password);
    ApiResponse<void>               setDirectLink(const QStringList &linkcodes, bool enabled);

    // Favorites
    ApiResponse<void>               changeFavorite(const QString &linkcode, bool add);
    ApiResponse<bool>               isFavorite(const QString &linkcode);
    ApiResponse<QVector<FileItem>>  listFavorites(const QString &extFilter = {});

    // Transfer session creation
    ApiResponse<QString>            createDownloadSession(const QString &url, const QString &password);
    ApiResponse<QString>            createUploadSession(const QString &name, int64_t size,
                                                        const QString &folder, bool secured);

private:
    // Spec §4.6 — gate run at the start of every authenticated public method.
    // Returns false when the coordinator has already hard-failed: caller
    // should short-circuit with an auth error.
    bool ensureFresh();

    // Re-read the canonical token snapshot from the refresh coordinator.
    // Cheap (one mutex lock) — called at the start of every call so a refresh
    // that just rotated the token is observed before the next request goes
    // out, and again right after a refresh-and-retry.
    void syncTokenFromCoordinator();

    // Spec §4.7 — when an auth-category error came back, drive a refresh and
    // tell the caller whether they should retry the original lambda exactly
    // once.  HardFail / SoftFail → return false; the original error stands.
    bool refreshAfterAuthError();

    // ── Auth-aware wrapper ───────────────────────────────────────────────
    //
    // Routes every authenticated FshareApi method through:
    //   1. ensureFresh()           — lazy proactive refresh, spec §4.6
    //   2. syncTokenFromCoordinator()
    //   3. fn()                    — caller's HTTP round-trip
    //   4. on Auth error → handleAuthExpired() + retry fn() ONCE
    //
    // Implemented inline so we don't need to enumerate every ApiResponse<T>
    // specialisation in a .cpp.  Fn is any callable returning an
    // ApiResponse<T> (incl. ApiResponse<void>).
    template <typename Fn>
    auto executeAuthed(Fn &&fn) -> decltype(fn())
    {
        using ResultT = decltype(fn());

        if (!ensureFresh()) {
            // Hard-fail from the coordinator — sessionExpired was already
            // emitted by it; surface an auth error so the caller path
            // (TransferService etc.) still routes through its
            // ErrorCategory::Auth handler for cleanup.
            return ResultT::failure(
                AppError::auth(420,
                    QStringLiteral("Phiên đăng nhập đã hết hạn. Vui lòng đăng nhập lại.")));
        }
        syncTokenFromCoordinator();

        ResultT result = fn();
        if (result.isSuccess())
            return result;
        if (result.error().category != ErrorCategory::Auth)
            return result;

        // Reactive refresh — single-flight inside the coordinator; concurrent
        // callers fold into the same network round-trip and see the same
        // outcome.  Replay the original lambda exactly once on success.
        if (!refreshAfterAuthError())
            return result;
        syncTokenFromCoordinator();
        return fn();
    }

    // Thread-safe snapshot of m_token.  Returned by value so the lambda can
    // hold a local copy across the entire HTTP round-trip without holding
    // the lock.  See `m_tokenMutex` below for why.
    QString tokenSnapshot() const;

    HttpClient              *m_http    = nullptr;
    RefreshTokenCoordinator *m_refresh = nullptr;
    QString                  m_appKey;
    QString                  m_token;

    // ── Token concurrency ────────────────────────────────────────────────
    // m_token is touched by:
    //   - setSession()                  — login completion (GUI thread)
    //   - syncTokenFromCoordinator()    — top of every executeAuthed call
    //                                     (worker thread, called per-request)
    //   - logout() and the m_token.clear() inside refresh hard-fail
    //   - every executeAuthed lambda's `body["token"] = m_token.toStdString()`
    //
    // Without serialisation, worker A's lambda reading m_token can collide
    // with worker B's syncTokenFromCoordinator() rotating m_token after a
    // silent refresh — QString d-pointer assignment is not atomic and we
    // saw rare crashes attributable to a torn implicit-shared refcount.
    //
    // The lock is held only across the snapshot/copy; never across the
    // HTTP call itself.
    mutable QMutex           m_tokenMutex;
};

} // namespace fsnext
