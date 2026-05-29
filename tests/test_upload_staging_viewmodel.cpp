// SPDX-License-Identifier: Proprietary
//
// Unit tests for UploadStagingViewModel — the session-and-restart-stable holder
// for the upload "staging" batch (files the user selected but hasn't uploaded).
//
// The VM's ctor takes (UploadViewModel*, SettingsRepository*). UploadViewModel
// itself needs a TransferService, but we construct it with a null service:
// UploadViewModel::addUpload() short-circuits on (!m_service), so commit() can
// run without any network. That lets us assert the *staging-side* contract
// (files staged / deduped / removed / cleared on commit) deterministically.
// The "addUpload received exactly the valid files" half of TC-0052/TC-0053
// needs a real TransferService (the model only fills from service signals), so
// that observation is QSKIP'd — we still assert staging is emptied after commit.
//
// SettingsRepository is backed by real QSettings, so init() wipes the
// UploadStaging/* keys before each test to give hydrate() a clean slate (the VM
// hydrates from disk in its ctor). No QtConcurrent work is kicked off by this
// VM, but cleanup() still drains the global pool before freeing the fixtures, in
// case UploadViewModel's timers or any pool task is in flight.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>
#include <QThreadPool>
#include <QTemporaryDir>
#include <QVariantList>
#include <QVariantMap>

#include "viewmodels/UploadStagingViewModel.h"
#include "viewmodels/UploadViewModel.h"
#include "core/repositories/SettingsRepository.h"

using fsnext::UploadStagingViewModel;
using fsnext::UploadViewModel;
using fsnext::SettingsRepository;

class TestUploadStagingViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("UploadStagingVMTest"));
        // Two real on-disk files so addFiles/commit see valid=true entries.
        QVERIFY(m_tmp.isValid());
        m_pathA = m_tmp.filePath(QStringLiteral("a.txt"));
        m_pathB = m_tmp.filePath(QStringLiteral("b.txt"));
        writeFile(m_pathA);
        writeFile(m_pathB);
    }

    void init()
    {
        m_repo = new SettingsRepository;
        // Clean slate — no persisted staging leaking in via hydrate().
        clearStagingKeys();
        m_upload = new UploadViewModel(/*service*/nullptr, /*auth*/nullptr);
        m_vm = new UploadStagingViewModel(m_upload, m_repo);
    }

    void cleanup()
    {
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        delete m_vm;     m_vm = nullptr;
        delete m_upload; m_upload = nullptr;
        delete m_repo;   m_repo = nullptr;
    }

    // ATM-0109 / TC-0044 — stagedFiles emits stagedFilesChanged on addFiles.
    void stagedFilesEmitsOnAdd()
    {
        QSignalSpy spy(m_vm, &UploadStagingViewModel::stagedFilesChanged);
        QVERIFY(m_vm->stagedFiles().isEmpty());

        m_vm->addFiles(QStringList{ m_pathA });

        QCOMPARE(m_vm->stagedFiles().size(), 1);
        QVERIFY(spy.count() >= 1);
        QCOMPARE(m_vm->stagedFiles().at(0).toMap().value(QStringLiteral("path")).toString(),
                 m_pathA);
    }

    // ATM-0110 / TC-0045 — hasStaged true after add, false after clearFiles.
    void hasStagedReflectsList()
    {
        QVERIFY(!m_vm->hasStaged());

        m_vm->addFiles(QStringList{ m_pathA });
        QVERIFY(m_vm->hasStaged());

        QSignalSpy spy(m_vm, &UploadStagingViewModel::stagedFilesChanged);
        m_vm->clearFiles();
        QVERIFY(!m_vm->hasStaged());
        QVERIFY(spy.count() >= 1);   // stagedFilesChanged fires on clearFiles
    }

    // ATM-0111 / TC-0046 — setTargetFolder updates value and emits.
    void setTargetFolderEmits()
    {
        QSignalSpy spy(m_vm, &UploadStagingViewModel::targetFolderChanged);
        m_vm->setTargetFolder(QStringLiteral("/Phim"));

        QCOMPARE(m_vm->targetFolder(), QStringLiteral("/Phim"));
        QCOMPARE(spy.count(), 1);
    }

    // ATM-0111 (Edge) — setTargetFolder with the same value is a no-op (no emit).
    void setTargetFolderSameValueNoEmit()
    {
        m_vm->setTargetFolder(QStringLiteral("/Phim"));
        QSignalSpy spy(m_vm, &UploadStagingViewModel::targetFolderChanged);
        m_vm->setTargetFolder(QStringLiteral("/Phim"));
        QCOMPARE(spy.count(), 0);
    }

    // ATM-0112 / TC-0047 — setPassword updates value and emits.
    void setPasswordEmits()
    {
        QSignalSpy spy(m_vm, &UploadStagingViewModel::passwordChanged);
        m_vm->setPassword(QStringLiteral("pw123"));

        QCOMPARE(m_vm->password(), QStringLiteral("pw123"));
        QCOMPARE(spy.count(), 1);
    }

    // ATM-0117 / TC-0048 — addFiles dedupes by canonical path.
    void addFilesDedupes()
    {
        QSignalSpy spy(m_vm, &UploadStagingViewModel::stagedFilesChanged);
        m_vm->addFiles(QStringList{ m_pathA, m_pathB, m_pathA });

        QCOMPARE(m_vm->stagedFiles().size(), 2);   // a + b, dup a collapsed
        QVERIFY(spy.count() >= 1);
    }

    // ATM-0117 (Edge) / TC-0049 — file:// URL and raw path for the same file
    // collapse into a single entry.
    void addFilesCollapsesUrlAndRawPath()
    {
        const QString fileUrl = QUrl::fromLocalFile(m_pathA).toString(); // file:///...
        m_vm->addFiles(QStringList{ fileUrl });
        QCOMPARE(m_vm->stagedFiles().size(), 1);

        m_vm->addFiles(QStringList{ m_pathA });   // raw path, same canonical file
        QCOMPARE(m_vm->stagedFiles().size(), 1);  // not added twice
    }

    // ATM-0118 / TC-0050 — removeFile drops the entry at a valid index.
    void removeFileValidIndex()
    {
        m_vm->addFiles(QStringList{ m_pathA, m_pathB });
        QCOMPARE(m_vm->stagedFiles().size(), 2);

        QSignalSpy spy(m_vm, &UploadStagingViewModel::stagedFilesChanged);
        m_vm->removeFile(0);   // drop a → b remains

        QCOMPARE(m_vm->stagedFiles().size(), 1);
        QCOMPARE(m_vm->stagedFiles().at(0).toMap().value(QStringLiteral("path")).toString(),
                 m_pathB);
        QCOMPARE(spy.count(), 1);
    }

    // ATM-0118 (Negative) / TC-0051 — removeFile with out-of-range index is a
    // no-op (no change, no crash, no signal).
    void removeFileOutOfRangeNoOp()
    {
        m_vm->addFiles(QStringList{ m_pathA });
        QSignalSpy spy(m_vm, &UploadStagingViewModel::stagedFilesChanged);

        m_vm->removeFile(5);
        m_vm->removeFile(-1);

        QCOMPARE(m_vm->stagedFiles().size(), 1);
        QCOMPARE(spy.count(), 0);
    }

    // ATM-0121 / TC-0052 — commit() hands valid files off and clears staging.
    // The "addUpload received exactly [a,b]" assertion needs a real
    // TransferService (the model only populates from service signals), so that
    // half is QSKIP'd; here we assert the deterministic staging-side contract:
    // a non-empty valid batch is consumed (staging emptied) and the signal fires.
    void commitClearsStaging()
    {
        m_vm->setTargetFolder(QStringLiteral("/Up"));
        m_vm->addFiles(QStringList{ m_pathA, m_pathB });
        QVERIFY(m_vm->hasStaged());

        QSignalSpy spy(m_vm, &UploadStagingViewModel::stagedFilesChanged);
        m_vm->commit();

        // commit() -> addUpload(valid...) -> clear(): staging is emptied and
        // the destination folder reset to the default "/".
        QVERIFY(!m_vm->hasStaged());
        QVERIFY(m_vm->stagedFiles().isEmpty());
        QCOMPARE(m_vm->targetFolder(), QStringLiteral("/"));
        QVERIFY(spy.count() >= 1);

        QSKIP("addUpload receiving exactly the valid batch needs a real "
              "TransferService (model fills from service signals) — Lo sau");
    }

    // ATM-0121 (Negative) / TC-0053 — commit() with a mix of valid + invalid
    // (a path that doesn't exist → valid=false) still clears staging and does
    // not crash; the invalid entry is skipped at commit time. Verifying that
    // ONLY the valid file reached addUpload needs a real TransferService, so
    // that observation is QSKIP'd.
    void commitSkipsInvalidEntries()
    {
        const QString missing = m_tmp.filePath(QStringLiteral("gone.txt")); // never created
        m_vm->addFiles(QStringList{ m_pathA, missing });
        QCOMPARE(m_vm->stagedFiles().size(), 2);

        // Confirm the staged entries carry the expected validity flags.
        bool sawValid = false, sawInvalid = false;
        for (const QVariant &v : m_vm->stagedFiles()) {
            const QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("path")).toString() == m_pathA)
                sawValid = m.value(QStringLiteral("valid")).toBool();
            else if (m.value(QStringLiteral("path")).toString() == missing)
                sawInvalid = !m.value(QStringLiteral("valid")).toBool();
        }
        QVERIFY(sawValid);
        QVERIFY(sawInvalid);

        m_vm->commit();   // skips the invalid entry; must not crash
        QVERIFY(!m_vm->hasStaged());

        QSKIP("Asserting only the valid file reached addUpload needs a real "
              "TransferService — Lo sau");
    }

private:
    static void writeFile(const QString &path)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
    }

    void clearStagingKeys()
    {
        m_repo->setString(QStringLiteral("UploadStaging/files"), QString());
        m_repo->setString(QStringLiteral("UploadStaging/folder"), QStringLiteral("/"));
        m_repo->setString(QStringLiteral("UploadStaging/password"), QString());
        m_repo->setInt(QStringLiteral("UploadStaging/secured"), 0);
        m_repo->setInt(QStringLiteral("UploadStaging/directLink"), 0);
    }

    QTemporaryDir            m_tmp;
    QString                  m_pathA;
    QString                  m_pathB;
    SettingsRepository      *m_repo   = nullptr;
    UploadViewModel         *m_upload = nullptr;
    UploadStagingViewModel  *m_vm     = nullptr;
};

QTEST_MAIN(TestUploadStagingViewModel)
#include "test_upload_staging_viewmodel.moc"
