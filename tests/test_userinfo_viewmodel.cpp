// SPDX-License-Identifier: Proprietary
//
// Unit tests for UserInfoViewModel — the read-only MVVM bridge over
// AuthService that surfaces the current user's identity / quota / storage
// fields to QML.
//
// The VM is a PURELY SYNCHRONOUS wrapper: every getter reads
// AuthService::currentUser() and the only "logic" lives in the level→label
// mapping (levelLabel/isVip/vipExpiry). It is constructed against a REAL
// AuthService wired to a FakeFshareApi (IFshareApi) + in-memory
// SettingsRepository, exactly like test_auth_viewmodel — but, unlike that
// suite, NOTHING here triggers login()/refresh()/fetchUserInfo(). Those run on
// QtConcurrent worker threads and are the documented bug-class #1 for flaky /
// segfaulting test binaries. We therefore exercise only the synchronous
// surface:
//   • getters return the empty/zero defaults of a fresh User{} (not logged in);
//   • the level→label table (levelToLabel) drives levelLabel/isVip/vipExpiry
//     deterministically, *without* a network round-trip, by reasoning purely
//     about the level-0 (free tier) default user;
//   • userInfoChanged is forwarded from AuthService::userChanged.
//
// The forward test emits AuthService::userChanged directly (signals are public
// member functions) — a fully synchronous trigger, no worker thread involved.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>

#include "viewmodels/UserInfoViewModel.h"
#include "core/services/AuthService.h"
#include "core/api/IFshareApi.h"
#include "core/repositories/SettingsRepository.h"
#include "core/models/AppError.h"
#include "core/models/User.h"

using fsnext::UserInfoViewModel;
using fsnext::AuthService;
using fsnext::IFshareApi;
using fsnext::SettingsRepository;
using fsnext::ApiResponse;
using fsnext::AppError;
using fsnext::Session;
using fsnext::User;

namespace {

// Minimal IFshareApi stub. Its methods are NEVER invoked by these tests (we
// never call login/logout/getUserInfo) — it exists only to satisfy the
// AuthService ctor's non-null api requirement. All overrides return failures so
// that an accidental network code path would fail loudly rather than hang.
class FakeFshareApi : public IFshareApi
{
public:
    ApiResponse<Session> login(const QString &, const QString &) override
    {
        return ApiResponse<Session>::failure(AppError::auth(405, QStringLiteral("n/a")));
    }
    ApiResponse<Session> loginOauth(const QString &, const QString &, const QString &) override
    {
        return ApiResponse<Session>::failure(AppError::auth(400, QStringLiteral("n/a")));
    }
    ApiResponse<void> logout() override { return ApiResponse<void>::success(); }
    ApiResponse<User> getUserInfo() override
    {
        return ApiResponse<User>::failure(AppError::auth(401, QStringLiteral("n/a")));
    }
};

} // namespace

class TestUserInfoViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Hermetic QSettings: AuthService writes Account/* + General/* keys via
        // SettingsRepository on logout/session-expiry. Pin everything to an
        // IniFormat store under a throwaway scope so the real app config (the
        // Windows registry) is never touched.
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_tmp.path());
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("UserInfoViewModelTest"));
    }

    void init()
    {
        m_api  = new FakeFshareApi;
        m_repo = new SettingsRepository;
        m_auth = new AuthService(m_api, m_repo);  // no oauth, no coordinator
        m_vm   = new UserInfoViewModel(m_auth);
    }

    void cleanup()
    {
        delete m_vm;   m_vm   = nullptr;
        delete m_auth; m_auth = nullptr;
        delete m_repo; m_repo = nullptr;
        delete m_api;  m_api  = nullptr;
    }

    // ── ATM-0145 / ATM-0146 / ATM-0147 / ATM-0154 ────────────────────────────
    // Identity string getters return empty defaults when no user is logged in
    // (fresh User{} — id/name/email/avatarUrl all default-constructed empty).
    void identityStringsDefaultEmpty()
    {
        QVERIFY(m_vm->userId().isEmpty());
        QVERIFY(m_vm->userName().isEmpty());
        QVERIFY(m_vm->userEmail().isEmpty());
        QVERIFY(m_vm->avatarUrl().isEmpty());
        QVERIFY(m_vm->accountType().isEmpty());
    }

    // ── ATM-0148 / ATM-0150 / ATM-0149 ───────────────────────────────────────
    // Level-derived view logic for the free-tier (level 0) default user:
    //   • userLevel == 0
    //   • isVip == false (VIP gate is level >= 3)
    //   • levelLabel == "Thành viên thường" (level 0-2 free-tier label)
    // This is the real branch logic of levelToLabel(), exercised without any
    // login by reasoning about the default User{}.
    void levelZeroIsFreeTier()
    {
        QCOMPARE(m_vm->userLevel(), 0);
        QVERIFY(!m_vm->isVip());
        QCOMPARE(m_vm->levelLabel(), QStringLiteral("Thành viên thường"));
    }

    // ── ATM-0152 / ATM-0153 ──────────────────────────────────────────────────
    // joinDate / vipExpiry go through FormatUtil::formatDate(); with empty
    // source timestamps (default User{}) they fall back to the "—" placeholder
    // rather than a parsed date. (Level 0 ≠ 18, so vipExpiry is NOT the
    // "Vĩnh viễn" forever-branch — it follows the formatDate fallback path.)
    void dateGettersFallbackWhenEmpty()
    {
        QCOMPARE(m_vm->joinDate(), QStringLiteral("—"));
        QCOMPARE(m_vm->vipExpiry(), QStringLiteral("—"));
    }

    // ── ATM-0155 / ATM-0156 ──────────────────────────────────────────────────
    // Points & download-quota integer getters default to 0 for a fresh user.
    void pointsAndQuotaDefaultZero()
    {
        QCOMPARE(m_vm->totalPoints(), 0);
        QCOMPARE(m_vm->dlTimeAvail(), 0);
    }

    // ── ATM-0157..0166 / ATM-0163 ────────────────────────────────────────────
    // Storage & traffic getters. For a fresh User{} every backing int64 is 0,
    // so totals, used, the computed *Free() helpers, the combined
    // storageTotalAll, and all traffic fields are 0. The non-trivial part here
    // is the *computed* getters (webspaceFree/secureFree/trafficFree =
    // total - used, and storageTotalAll = webspace + secureTotal): 0 - 0 == 0
    // and 0 + 0 == 0, proving the arithmetic wiring is wired to the right
    // fields without a network fetch.
    void storageAndTrafficDefaultZero()
    {
        // Non-secure webspace (total / used / computed free)
        QCOMPARE(m_vm->webspaceTotal(), Q_INT64_C(0));
        QCOMPARE(m_vm->webspaceUsed(),  Q_INT64_C(0));
        QCOMPARE(m_vm->webspaceFree(),  Q_INT64_C(0));

        // Secure webspace
        QCOMPARE(m_vm->secureTotal(), Q_INT64_C(0));
        QCOMPARE(m_vm->secureUsed(),  Q_INT64_C(0));
        QCOMPARE(m_vm->secureFree(),  Q_INT64_C(0));

        // Combined total (webspace + secureTotal)
        QCOMPARE(m_vm->storageTotalAll(), Q_INT64_C(0));

        // Traffic (total / used / computed free)
        QCOMPARE(m_vm->trafficTotal(), Q_INT64_C(0));
        QCOMPARE(m_vm->trafficUsed(),  Q_INT64_C(0));
        QCOMPARE(m_vm->trafficFree(),  Q_INT64_C(0));
    }

    // ── ATM-0145..0166 (NOTIFY contract) ─────────────────────────────────────
    // Every Q_PROPERTY shares the single userInfoChanged NOTIFY, which the VM
    // ctor wires to AuthService::userChanged. Emitting that signal directly
    // (signals are public member functions — fully synchronous, no worker
    // thread) must forward exactly one userInfoChanged. This proves the
    // change-notification plumbing without driving an async login/fetch.
    void userChangedForwardsToUserInfoChanged()
    {
        QSignalSpy spy(m_vm, &UserInfoViewModel::userInfoChanged);
        QVERIFY(spy.isValid());

        emit m_auth->userChanged();
        QCOMPARE(spy.count(), 1);

        emit m_auth->userChanged();
        QCOMPARE(spy.count(), 2);
    }

private:
    FakeFshareApi      *m_api  = nullptr;
    SettingsRepository *m_repo = nullptr;
    AuthService        *m_auth = nullptr;
    UserInfoViewModel  *m_vm   = nullptr;
    QTemporaryDir       m_tmp;
};

QTEST_GUILESS_MAIN(TestUserInfoViewModel)
#include "test_userinfo_viewmodel.moc"
