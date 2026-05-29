// SPDX-License-Identifier: Proprietary
//
// Unit tests for SettingsViewModel — property getter/setter round-trips, the
// NOTIFY-signal contract (emit once on change, never on no-op), the
// effectiveDownloadFolder fallback, and the applySettings() persistence hook.
//
// The VM is driven against a REAL SettingsService backed by a REAL
// SettingsRepository. No network and no QtConcurrent are involved: every setter
// forwards synchronously to the service which writes QSettings immediately, so
// the assertions can read state back on the same event-loop tick without any
// QSignalSpy::wait().
//
// HERMETIC ISOLATION: SettingsRepository is hardwired to
// QSettings("FPT","FshareNext"). To avoid touching the developer's real config
// (and to get a deterministic clean-default baseline), initTestCase() forces
// IniFormat and redirects the UserScope ini path to a throwaway QTemporaryDir,
// then init() clears the FPT/FshareNext store. The real app uses NativeFormat
// (Windows registry), so it is never reachable from here regardless. Each test
// thus starts from default-constructed AppSettings.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "viewmodels/SettingsViewModel.h"
#include "core/services/SettingsService.h"
#include "core/repositories/SettingsRepository.h"
#include "core/models/AppSettings.h"

using fsnext::SettingsViewModel;
using fsnext::SettingsService;
using fsnext::SettingsRepository;
using fsnext::AppSettings;

class TestSettingsViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("SettingsViewModelTest"));
        // Redirect the IniFormat/UserScope store (which SettingsRepository's
        // hardwired QSettings("FPT","FshareNext") lands in) to a throwaway dir,
        // so this test never reads or writes the developer's real config.
        QVERIFY(m_tmp.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tmp.path());
    }

    void init()
    {
        // Wipe the repo's own store so every test sees the default-constructed
        // AppSettings baseline through the service.
        QSettings(QStringLiteral("FPT"), QStringLiteral("FshareNext")).clear();

        m_repo = new SettingsRepository;
        m_svc  = new SettingsService(m_repo);
        m_vm   = new SettingsViewModel(m_svc);
    }

    void cleanup()
    {
        QCoreApplication::processEvents();
        delete m_vm;  m_vm  = nullptr;
        delete m_svc; m_svc = nullptr;
        delete m_repo; m_repo = nullptr;
        QSettings(QStringLiteral("FPT"), QStringLiteral("FshareNext")).clear();
    }

    // ATM-0086 / TC-0039 — setDownloadFolder updates value + emits change once.
    // Baseline is the clean default (isolated store), so we still assert the
    // delta (value updated + signal fired exactly once) against a target value
    // derived from the current one — the contract the VM actually guarantees.
    void setDownloadFolderUpdatesAndEmits()
    {
        const QString target = m_vm->downloadFolder() + QStringLiteral("\\unit-test-dl");

        QSignalSpy spy(m_vm, &SettingsViewModel::downloadFolderChanged);
        QVERIFY(spy.isValid());

        m_vm->setDownloadFolder(target);

        QCOMPARE(m_vm->downloadFolder(), target);
        QCOMPARE(spy.count(), 1);
        // Persisted through to the service too.
        QCOMPARE(m_svc->downloadFolder(), target);
    }

    // ATM-0086 (Edge) — setting the same value is a no-op (no extra signal).
    void setDownloadFolderNoOpOnSameValue()
    {
        const QString target = m_vm->downloadFolder() + QStringLiteral("\\unit-test-dl2");
        m_vm->setDownloadFolder(target);
        QSignalSpy spy(m_vm, &SettingsViewModel::downloadFolderChanged);
        m_vm->setDownloadFolder(target);
        QCOMPARE(spy.count(), 0);
    }

    // ATM-0087 / TC-0040 — effectiveDownloadFolder always resolves to a
    // non-empty path (custom folder if set, else the OS default download dir).
    void effectiveDownloadFolderDefaultsWhenUnset()
    {
        const QString eff = m_vm->effectiveDownloadFolder();
        QVERIFY2(!eff.isEmpty(), qPrintable(eff));           // never empty
    }

    // ATM-0087 / TC-0041 (Edge) — once a custom folder is set,
    // effectiveDownloadFolder returns it verbatim and downloadFolderChanged fired.
    void effectiveDownloadFolderReflectsCustom()
    {
        QSignalSpy spy(m_vm, &SettingsViewModel::downloadFolderChanged);
        m_vm->setDownloadFolder(QStringLiteral("E:\\Tai"));

        QCOMPARE(m_vm->effectiveDownloadFolder(), QStringLiteral("E:\\Tai"));
        QCOMPARE(spy.count(), 1);
    }

    // ATM-0094 / TC-0042 — setAutoLogin flips value + emits exactly once.
    // Baseline may be either state (persisted on this machine), so we flip to
    // the opposite of the current value and verify the transition.
    void setAutoLoginUpdatesAndEmits()
    {
        const bool start = m_vm->autoLogin();
        QSignalSpy spy(m_vm, &SettingsViewModel::autoLoginChanged);

        m_vm->setAutoLogin(!start);

        QCOMPARE(m_vm->autoLogin(), !start);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(m_svc->autoLogin(), !start);                // persisted
    }

    // ATM-0094 / TC-0043 (Edge) — re-setting the same bool is a no-op.
    void setAutoLoginNoOpOnSameValue()
    {
        const bool start = m_vm->autoLogin();
        m_vm->setAutoLogin(!start);                          // ensure a known state
        QSignalSpy spy(m_vm, &SettingsViewModel::autoLoginChanged);
        m_vm->setAutoLogin(!start);                          // same value again
        QCOMPARE(spy.count(), 0);
    }

    // ATM-0077 / TC-0038 — applySettings() pushes the VM's current state into
    // the service (saveSettings), which persists it through the repository.
    void applySettingsPersistsCurrentState()
    {
        m_vm->setDownloadThreads(8);
        m_vm->setDownloadFolder(QStringLiteral("D:\\DL"));

        m_vm->applySettings();

        // saveSettings stores the full struct; verify via a fresh load.
        const AppSettings reloaded = m_svc->loadSettings();
        QCOMPARE(reloaded.downloadThreads, 8);
        QCOMPARE(reloaded.downloadFolder, QStringLiteral("D:\\DL"));
    }

    // ATM-0077 (Edge) — applySettings() with no setter calls is harmless: it
    // re-persists the VM's current state verbatim, so a fresh load matches the
    // VM's own getters (no crash, no value drift).
    //
    // NOTE: we compare against the VM's current state, NOT against a
    // default-constructed AppSettings — SettingsRepository::load() applies its
    // own per-key defaults (e.g. Download/threads=2, Upload/threads=2) that
    // intentionally differ from the AppSettings struct initializers, so a
    // struct-default comparison would not reflect the real round-trip.
    void applySettingsWithDefaultsIsHarmless()
    {
        const int dlBefore = m_vm->downloadThreads();
        const int ulBefore = m_vm->uploadThreads();

        m_vm->applySettings();

        const AppSettings reloaded = m_svc->loadSettings();
        QCOMPARE(reloaded.downloadThreads, dlBefore);
        QCOMPARE(reloaded.uploadThreads, ulBefore);
    }

private:
    QTemporaryDir       m_tmp;   // isolates the QSettings ini store for the whole run
    SettingsRepository *m_repo = nullptr;
    SettingsService    *m_svc  = nullptr;
    SettingsViewModel  *m_vm   = nullptr;
};

QTEST_GUILESS_MAIN(TestSettingsViewModel)
#include "test_settings_viewmodel.moc"
