// SPDX-License-Identifier: Proprietary
//
// Unit tests for DownloadViewModel — the bridge between the Download QML page
// and TransferService. These tests build the *real* service graph with
// in-memory / null collaborators and assert only the construction-time and
// fail-fast behaviour that needs NO network:
//
//   * model() / historyModel() are valid, distinct TransferListModel*.
//   * runState defaults to "idle" when no task has been added.
//   * defaultSaveFolder falls back to SettingsService::effectiveDownloadFolder().
//   * extractFshareLinks (pure regex helper) parses / dedups / returns empty.
//   * addDownload fail-fast: empty URL and invalid URL never reach the
//     service (the VM rejects them before enqueue), the protected
//     system-folder branch emits downloadBlocked.
//
// The "happy path" addDownload (a valid fshare.vn/file/... URL) would drive
// TransferService → TransferOrchestrator → DownloadEngine → libcurl, which
// needs a real network round-trip and a live session, so it is QSKIP-ped here
// ("cần mạng — Lô sau").
//
// Graph construction strategy (mirrors test_file_cache_service):
//   * FshareApi is passed as nullptr — TransferService's ctor never touches
//     m_api, and the only code path that would (dispatch) is never reached on
//     the non-network tests. FshareApi.cpp + HttpClient.cpp are still linked
//     because TransferService.cpp ODR-references FshareApi symbols.
//   * TransferOrchestrator runs on its own thread but is never asked to
//     dispatch in these tests.
//   * AuthService is passed as nullptr (DownloadViewModel's auth wiring is an
//     optional logout-flush hook) to keep the link list small.
//   * cleanup() drains the global thread pool BEFORE the fixtures are freed so
//     no stray worker dereferences a dangling collaborator.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>
#include <QThreadPool>
#include <QString>

#include "viewmodels/DownloadViewModel.h"
#include "viewmodels/TransferListModel.h"
#include "core/services/TransferService.h"
#include "core/services/SettingsService.h"
#include "core/transfer/TransferOrchestrator.h"
#include "core/repositories/SettingsRepository.h"
#include "core/repositories/HistoryRepository.h"

using fsnext::DownloadViewModel;
using fsnext::TransferListModel;
using fsnext::TransferService;
using fsnext::SettingsService;
using fsnext::TransferOrchestrator;
using fsnext::SettingsRepository;
using fsnext::HistoryRepository;

class TestDownloadViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("DownloadViewModelTest"));
    }

    void init()
    {
        m_repo  = new SettingsRepository;
        m_settings = new SettingsService(m_repo);
        m_history  = new HistoryRepository;
        m_orch     = new TransferOrchestrator;
        // FshareApi=nullptr: the ctor never dereferences it, and no test here
        // takes a code path that would.
        m_service  = new TransferService(/*api=*/nullptr, m_repo, m_orch, m_history);
        // AuthService=nullptr: only the logout-flush hook needs it.
        m_vm = new DownloadViewModel(m_service, m_settings, /*auth=*/nullptr);
    }

    void cleanup()
    {
        // Drain any in-flight pool task before freeing the graph.
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        delete m_vm;       m_vm       = nullptr;
        delete m_service;  m_service  = nullptr;
        delete m_orch;     m_orch     = nullptr;
        delete m_history;  m_history  = nullptr;
        delete m_settings; m_settings = nullptr;
        delete m_repo;     m_repo     = nullptr;
    }

    // ── model / historyModel ─────────────────────────────────
    // ATM-0016 / TC-0016 — model() is a valid TransferListModel* for the
    // active download list.
    void modelIsValid()
    {
        QVERIFY(m_vm->model() != nullptr);
        // Freshly constructed → no active tasks.
        QCOMPARE(m_vm->model()->count(), 0);
    }

    // ATM-0017 / TC-0017 — historyModel() is valid AND a distinct instance
    // from model() (active vs. history are two separate models).
    void historyModelIsValidAndDistinct()
    {
        QVERIFY(m_vm->historyModel() != nullptr);
        QVERIFY(m_vm->historyModel() != m_vm->model());
        QCOMPARE(m_vm->historyModel()->count(), 0);
    }

    // ── runState ─────────────────────────────────────────────
    // ATM-0021 / TC-0019 — with no tasks the aggregate run-state is "idle".
    void runStateDefaultsToIdle()
    {
        QCOMPARE(m_vm->runState(), QStringLiteral("idle"));
    }

    // ATM-0021 / TC-0018 — the running-state transition is driven by live
    // TransferService task signals (enqueue → Active). Reaching the Active
    // state requires the orchestrator + a real download, so the positive
    // transition is deferred to the network lot.
    void runStateRunningNeedsNetwork()
    {
        QSKIP("runState→\"running\" cần một task Active thực (mạng) — Lô sau");
    }

    // ── defaultSaveFolder ────────────────────────────────────
    // ATM-0016 (companion) — defaultSaveFolder mirrors the settings service's
    // effectiveDownloadFolder() and is never empty (falls back to the OS
    // Downloads / home location).
    void defaultSaveFolderFollowsSettings()
    {
        QCOMPARE(m_vm->defaultSaveFolder(),
                 m_settings->effectiveDownloadFolder());
        QVERIFY(!m_vm->defaultSaveFolder().isEmpty());
    }

    // ── extractFshareLinks ───────────────────────────────────
    // ATM-0029 / TC-0022 — pull fshare file+folder URLs out of surrounding
    // prose, one per line, no prose leakage.
    void extractFshareLinksFromProse()
    {
        const QString out = m_vm->extractFshareLinks(QStringLiteral(
            "Tai ve https://www.fshare.vn/file/ABC va "
            "https://fshare.vn/folder/XYZ nhe"));
        const QStringList lines =
            out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        QCOMPARE(lines.size(), 2);
        QVERIFY(lines.contains(QStringLiteral("https://www.fshare.vn/file/ABC")));
        QVERIFY(lines.contains(QStringLiteral("https://fshare.vn/folder/XYZ")));
        QVERIFY(!out.contains(QStringLiteral("Tai ve")));
        QVERIFY(!out.contains(QStringLiteral("nhe")));
    }

    // ATM-0029 / TC-0023 — duplicate URLs collapse to one line.
    void extractFshareLinksDedups()
    {
        const QString out = m_vm->extractFshareLinks(QStringLiteral(
            "https://www.fshare.vn/file/ABC https://www.fshare.vn/file/ABC"));
        const QStringList lines =
            out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        QCOMPARE(lines.size(), 1);
        QCOMPARE(lines.at(0), QStringLiteral("https://www.fshare.vn/file/ABC"));
    }

    // ATM-0029 / TC-0024 — text with no link yields an empty string.
    void extractFshareLinksEmptyOnNoMatch()
    {
        QVERIFY(m_vm->extractFshareLinks(
                    QStringLiteral("khong co lien ket nao o day")).isEmpty());
        // Empty input is also empty out (early-return guard).
        QVERIFY(m_vm->extractFshareLinks(QString()).isEmpty());
    }

    // ── addDownload fail-fast (no network) ───────────────────
    // ATM-0026 / TC-0020 (negative half) — an empty URL string produces no
    // task and no downloadBlocked: split(SkipEmptyParts) yields nothing, so
    // the loop body never runs and the service is never asked to enqueue.
    void addDownloadEmptyUrlIsNoOp()
    {
        QSignalSpy blocked(m_vm, &DownloadViewModel::downloadBlocked);
        const QString folder = QDir::tempPath();   // a writable, non-system dir
        m_vm->addDownload(QString(), folder);
        QCOMPARE(m_vm->model()->count(), 0);
        QCOMPARE(blocked.count(), 0);
    }

    // ATM-0026 / TC-0020 (negative half) — a non-fshare / malformed URL is
    // rejected by FshareUrl::parse before any enqueue; the VM reports it via
    // downloadBlocked and adds nothing to the active model.
    void addDownloadInvalidUrlBlocks()
    {
        QSignalSpy blocked(m_vm, &DownloadViewModel::downloadBlocked);
        const QString folder = QDir::tempPath();
        m_vm->addDownload(QStringLiteral("not-a-url ::: garbage"), folder);
        QCOMPARE(m_vm->model()->count(), 0);
        QCOMPARE(blocked.count(), 1);
    }

    // ATM-0026 / TC-0021 — downloading into a protected system folder is
    // blocked: downloadBlocked fires and nothing is enqueued. Even a
    // syntactically valid fshare URL must not slip through, because the
    // folder check happens first.
    void addDownloadSystemFolderBlocks()
    {
        QSignalSpy blocked(m_vm, &DownloadViewModel::downloadBlocked);
        // Windows system root — PlatformUtils::isSystemFolder flags it.
        m_vm->addDownload(QStringLiteral("https://www.fshare.vn/file/ABC123"),
                          QStringLiteral("C:\\Windows"));
        QCOMPARE(blocked.count(), 1);
        QCOMPARE(m_vm->model()->count(), 0);
    }

    // ATM-0026 / TC-0020 (positive) — enqueuing a real download drives the
    // orchestrator + DownloadEngine + libcurl; deferred to the network lot.
    void addDownloadValidUrlNeedsNetwork()
    {
        QSKIP("addDownload với URL hợp lệ cần mạng (session + curl) — Lô sau");
    }

private:
    SettingsRepository   *m_repo     = nullptr;
    SettingsService      *m_settings = nullptr;
    HistoryRepository    *m_history  = nullptr;
    TransferOrchestrator *m_orch     = nullptr;
    TransferService      *m_service  = nullptr;
    DownloadViewModel    *m_vm       = nullptr;
};

QTEST_MAIN(TestDownloadViewModel)
#include "test_download_viewmodel.moc"
