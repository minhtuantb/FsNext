#include "LanguageViewModel.h"

#include <QCoreApplication>
#include <QSettings>
#include <QVariantMap>

namespace fsnext {

LanguageViewModel::LanguageViewModel(QObject *parent)
    : QObject(parent)
    , m_language(loadPersisted())
{
    // Install translator on startup so the first QML render already has the
    // correct language. main.cpp creates AppContext (which creates us) BEFORE
    // creating the QQmlApplicationEngine, so the translator is in place when
    // QML loads its first frame.
    loadTranslation(m_language);
}

void LanguageViewModel::setLanguage(const QString &lang)
{
    if (m_language == lang) return;
    m_language = lang;
    loadTranslation(lang);
    persist(lang);
    emit languageChanged();
    // main.cpp connects this to engine.retranslate() so all qsTr() in QML re-fires
    emit translationReloadNeeded();
}

QString LanguageViewModel::displayName() const
{
    return (m_language == QStringLiteral("en"))
        ? QStringLiteral("English")
        : QStringLiteral("Tiếng Việt");
}

QVariantList LanguageViewModel::availableLanguages() const
{
    return {
        QVariantMap{ { QStringLiteral("code"), QStringLiteral("vi") },
                     { QStringLiteral("name"), QStringLiteral("Tiếng Việt") } },
        QVariantMap{ { QStringLiteral("code"), QStringLiteral("en") },
                     { QStringLiteral("name"), QStringLiteral("English") } }
    };
}

void LanguageViewModel::loadTranslation(const QString &lang)
{
    if (!m_translator.isEmpty())
        QCoreApplication::removeTranslator(&m_translator);

    // "vi" = source language → source strings (Vietnamese) display as-is, no file needed.
    if (lang == QStringLiteral("vi") || lang.isEmpty())
        return;

    const QString dir = QCoreApplication::applicationDirPath()
                        + QStringLiteral("/translations");
    const QString qm  = QStringLiteral("fshare_") + lang + QStringLiteral(".qm");

    if (m_translator.load(qm, dir)) {
        QCoreApplication::installTranslator(&m_translator);
    }
    // If the .qm file is missing, falls back gracefully to Vietnamese source strings.
}

void LanguageViewModel::persist(const QString &lang)
{
    QSettings s;
    s.setValue(QStringLiteral("UI/language"), lang);
}

QString LanguageViewModel::loadPersisted() const
{
    QSettings s;
    const QString saved = s.value(QStringLiteral("UI/language"),
                                  QStringLiteral("vi")).toString();
    // Whitelist: only support "vi" and "en"
    return (saved == QStringLiteral("en")) ? QStringLiteral("en") : QStringLiteral("vi");
}

} // namespace fsnext
