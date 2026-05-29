// SPDX-License-Identifier: Proprietary
//
// Unit tests for UploadViewModel — the non-network surface only.
//
// UploadViewModel's ctor takes (TransferService*, AuthService*). The real
// TransferService drags in the whole transfer graph (engines, orchestrator,
// FshareApi, history DB) and every meaningful path through addUpload/pause/
// resume ultimately needs a live server. We therefore construct the VM with
// BOTH dependencies null — exactly the "service-less" branch the production
// code guards for (`if (!m_service) return;`). That lets us assert the parts
// that stand alone:
//   • model()/historyModel() are always constructed (new TransferListModel(this))
//   • the two models are distinct instances
//   • runState defaults to "idle" with no tasks
//   • addUpload is a fail-fast no-op for an empty file list (and with a null
//     service even a non-empty list enqueues nothing — no crash)
//
// Anything that requires a real upload round-trip (a task actually reaching
// TransferService, progress/state signals, the quota pre-flight against a
// logged-in User) is QSKIP'd here and deferred to an integration lot that can
// stand up a TransferService against a fake engine.
//
// The VM owns two QTimers (sweep + progress-flush) but no worker threads of
// its own; still, cleanup() drains the global pool before freeing the fixture
// to stay consistent with the async-VM test convention.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QStringList>
#include <QThreadPool>

#include "viewmodels/UploadViewModel.h"
#include "viewmodels/TransferListModel.h"

using fsnext::UploadViewModel;
using fsnext::TransferListModel;

class TestUploadViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("UploadViewModelTest"));
    }

    void init()
    {
        // No TransferService, no AuthService — the guarded "service-less" path.
        m_vm = new UploadViewModel(nullptr, nullptr);
    }

    void cleanup()
    {
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        delete m_vm;
        m_vm = nullptr;
    }

    // ATM-0122 / TC-0054 — model() is a valid TransferListModel for the
    // active (in-flight) upload list, even with no service injected.
    void modelIsNonNull()
    {
        QVERIFY(m_vm->model() != nullptr);
        QCOMPARE(m_vm->model()->count(), 0);   // nothing enqueued yet
    }

    // ATM-0123 / TC-0055 — historyModel() is non-null AND a different
    // instance from model() (active vs. completed are separate lists).
    void historyModelIsDistinct()
    {
        QVERIFY(m_vm->historyModel() != nullptr);
        QVERIFY(m_vm->historyModel() != m_vm->model());
        QCOMPARE(m_vm->historyModel()->count(), 0);
    }

    // ATM-0126 / TC-0056 — runState defaults to "idle" with no tasks.
    // The Positive case (a task flipping the VM to "running") is driven by
    // TransferService::taskAdded/taskStateChanged, which require a live
    // service; that branch is exercised in the integration lot — here we
    // only pin the documented default and verify the signal exists.
    void runStateDefaultsIdle()
    {
        QCOMPARE(m_vm->runState(), QStringLiteral("idle"));

        // The signal must be wired even though nothing fires it here.
        QSignalSpy runSpy(m_vm, &UploadViewModel::runStateChanged);
        QVERIFY(runSpy.isValid());

        QSKIP("runState -> \"running\" requires a live TransferService emitting "
              "taskAdded/taskStateChanged — covered in the integration lot.");
    }

    // ATM-0127 / TC-0058 — addUpload with an empty file list creates no task.
    // With no service injected the VM short-circuits; nothing is enqueued and
    // the active model stays empty. No uploadError is emitted for an empty
    // batch (pre-flight only fires on quota/permission failures).
    void addUploadEmptyListIsNoOp()
    {
        QSignalSpy errSpy(m_vm, &UploadViewModel::uploadError);

        m_vm->addUpload(QStringList{}, QStringLiteral("0"));

        QCOMPARE(m_vm->model()->count(), 0);
        QCOMPARE(errSpy.count(), 0);
    }

    // ATM-0127 / TC-0058 (Edge) — even a NON-empty list enqueues nothing when
    // there is no service. This guards the fail-fast `if (!m_service) return;`
    // branch: no crash, no task, no spurious error signal.
    void addUploadWithoutServiceEnqueuesNothing()
    {
        QSignalSpy errSpy(m_vm, &UploadViewModel::uploadError);

        const QStringList files{
            QStringLiteral("D:\\a.txt"),
            QStringLiteral("D:\\b.txt"),
        };
        m_vm->addUpload(files, QStringLiteral("0"));

        QCOMPARE(m_vm->model()->count(), 0);
        QCOMPARE(errSpy.count(), 0);
    }

    // ATM-0127 / TC-0057 — the real "task created + handed to TransferService
    // with the right folderId" assertion needs a TransferService (and, for the
    // quota branch, a logged-in AuthService). Deferred to the integration lot.
    void addUploadDispatchesToService()
    {
        QSKIP("addUpload -> TransferService::addUpload requires a live "
              "TransferService — covered in the integration lot.");
    }

    // ATM-0128 / TC-0059, ATM-0129 / TC-0060, ATM-0130 / TC-0061,
    // ATM-0131 / TC-0062, ATM-0132 / TC-0063 — pause/resume/cancel/pauseAll/
    // resumeAll all forward straight to TransferService. With a null service
    // they are pure guarded no-ops (asserted not to crash below); the actual
    // state transitions belong to the integration lot.
    void taskControlsAreSafeNoOpsWithoutService()
    {
        // None of these should touch a null service or crash.
        m_vm->pauseTask(QStringLiteral("u1"));
        m_vm->resumeTask(QStringLiteral("u1"));
        m_vm->cancelTask(QStringLiteral("u1"));
        m_vm->pauseAll();
        m_vm->resumeAll();

        QCOMPARE(m_vm->model()->count(), 0);
        QCOMPARE(m_vm->runState(), QStringLiteral("idle"));

        QSKIP("pause/resume/cancel/pauseAll/resumeAll state transitions require "
              "a live TransferService — covered in the integration lot.");
    }

private:
    UploadViewModel *m_vm = nullptr;
};

QTEST_MAIN(TestUploadViewModel)
#include "test_upload_viewmodel.moc"
