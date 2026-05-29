// SPDX-License-Identifier: Proprietary
//
// Unit tests for LanguageViewModel — the runtime bilingual (vi/en) switcher.
//
// The class is a small, synchronous QObject with no service dependencies: it
// keeps a "vi"|"en" code, derives displayName() from it, exposes a static
// availableLanguages() list, and persists the code to QSettings key
// "UI/language". setLanguage() flips the code, (re)loads a QTranslator, persists,
// and emits languageChanged() + translationReloadNeeded(). All of that runs on
// the calling thread with no QtConcurrent / network — so every assertion reads
// back state on the same event-loop tick, no QSignalSpy::wait() needed.
//
// HERMETIC ISOLATION: LanguageViewModel persists via a default-constructed
// QSettings (the org/app name set on QCoreApplication). The real app uses
// NativeFormat (Windows registry); initTestCase() forces IniFormat + redirects
// the UserScope ini path to a throwaway QTemporaryDir and sets a throwaway
// org/app name, so this test never reads or writes the developer's real config.
// init() clears that store so the ctor's loadPersisted() sees the "vi" default.
//
// Translation loading: switching to "en" tries to load fshare_en.qm from
// <appDir>/translations. In the test binary that file is absent, so the load
// logs a qWarning and falls back to the source language — but m_language is
// still set to "en" (the warning is expected and harmless here). We assert on
// the language code + signals, not on the actual translated strings.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QVariantList>
#include <QVariantMap>

#include "viewmodels/LanguageViewModel.h"

using fsnext::LanguageViewModel;

class TestLanguageViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("LanguageViewModelTest"));
        QVERIFY(m_tmp.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tmp.path());
    }

    void init()
    {
        // Wipe the persisted store so the ctor's loadPersisted() falls back to
        // the "vi" default for every test.
        QSettings().clear();
    }

    // ATM-0074: default language code is "vi" and displayName tracks it.
    void defaultLanguageIsVi()
    {
        LanguageViewModel vm;
        QCOMPARE(vm.language(), QStringLiteral("vi"));
        QCOMPARE(vm.displayName(), QString::fromUtf8("Tiếng Việt"));
    }

    // ATM-0074 / ATM-0075: setLanguage("en") flips code + displayName and emits
    // both languageChanged() and translationReloadNeeded() exactly once.
    void setLanguageSwitchesAndEmits()
    {
        LanguageViewModel vm;
        QSignalSpy changed(&vm, &LanguageViewModel::languageChanged);
        QSignalSpy reload(&vm, &LanguageViewModel::translationReloadNeeded);
        QVERIFY(changed.isValid());
        QVERIFY(reload.isValid());

        vm.setLanguage(QStringLiteral("en"));

        QCOMPARE(vm.language(), QStringLiteral("en"));
        QCOMPARE(vm.displayName(), QStringLiteral("English"));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(reload.count(), 1);
    }

    // ATM-0074: setting the same value is a no-op — no signal, no state churn.
    void setSameLanguageIsNoOp()
    {
        LanguageViewModel vm;                       // starts at "vi"
        QSignalSpy changed(&vm, &LanguageViewModel::languageChanged);
        QSignalSpy reload(&vm, &LanguageViewModel::translationReloadNeeded);

        vm.setLanguage(QStringLiteral("vi"));       // same as current

        QCOMPARE(vm.language(), QStringLiteral("vi"));
        QCOMPARE(changed.count(), 0);
        QCOMPARE(reload.count(), 0);
    }

    // ATM-0074: the chosen code is persisted to QSettings("UI/language") and a
    // freshly constructed VM hydrates from it (en survives a "restart").
    void languagePersistsAcrossInstances()
    {
        {
            LanguageViewModel vm;
            vm.setLanguage(QStringLiteral("en"));
            QCOMPARE(vm.language(), QStringLiteral("en"));
        }
        // Raw store reflects the write.
        QCOMPARE(QSettings().value(QStringLiteral("UI/language")).toString(),
                 QStringLiteral("en"));
        // A new instance picks it up.
        LanguageViewModel vm2;
        QCOMPARE(vm2.language(), QStringLiteral("en"));
        QCOMPARE(vm2.displayName(), QStringLiteral("English"));
    }

    // ATM-0074: loadPersisted() whitelists the code — an unknown persisted value
    // is coerced back to "vi" rather than trusted blindly.
    void unknownPersistedValueFallsBackToVi()
    {
        QSettings().setValue(QStringLiteral("UI/language"), QStringLiteral("fr"));
        LanguageViewModel vm;
        QCOMPARE(vm.language(), QStringLiteral("vi"));
    }

    // ATM-0076: availableLanguages() returns exactly [{vi, Tiếng Việt},
    // {en, English}] in order, each a {code, name} map.
    void availableLanguagesShape()
    {
        LanguageViewModel vm;
        const QVariantList langs = vm.availableLanguages();
        QCOMPARE(langs.size(), 2);

        const QVariantMap vi = langs.at(0).toMap();
        QCOMPARE(vi.value(QStringLiteral("code")).toString(), QStringLiteral("vi"));
        QCOMPARE(vi.value(QStringLiteral("name")).toString(),
                 QString::fromUtf8("Tiếng Việt"));

        const QVariantMap en = langs.at(1).toMap();
        QCOMPARE(en.value(QStringLiteral("code")).toString(), QStringLiteral("en"));
        QCOMPARE(en.value(QStringLiteral("name")).toString(), QStringLiteral("English"));
    }

private:
    QTemporaryDir m_tmp;
};

QTEST_GUILESS_MAIN(TestLanguageViewModel)
#include "test_language_viewmodel.moc"
