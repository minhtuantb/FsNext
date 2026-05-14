#pragma once

#include <QObject>
#include <QTranslator>
#include <QVariantList>

namespace fsnext {

/**
 * LanguageViewModel — runtime bilingual switching (Tiếng Việt / English).
 *
 * Vietnamese is the source language: qsTr() source strings are Vietnamese.
 * No translator file is needed for VI — source strings display as-is.
 * Switching to EN loads fshare_en.qm from <appDir>/translations/ and emits
 * translationReloadNeeded so main.cpp can call QQmlApplicationEngine::retranslate().
 *
 * Persisted to QSettings key "UI/language" ("vi" | "en").
 *
 * QML usage:
 *   languageViewModel.language              // "vi" or "en"
 *   languageViewModel.language = "en"       // switch at runtime
 *   languageViewModel.displayName           // "Tiếng Việt" or "English"
 *   languageViewModel.availableLanguages    // [{ code, name }, ...]
 */
class LanguageViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY languageChanged)
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages CONSTANT)

public:
    explicit LanguageViewModel(QObject *parent = nullptr);

    QString language() const { return m_language; }
    void setLanguage(const QString &lang);

    QString displayName() const;
    QVariantList availableLanguages() const;

signals:
    void languageChanged();

    // main.cpp connects this to QQmlApplicationEngine::retranslate()
    // so every qsTr() binding in QML is re-evaluated on language change.
    void translationReloadNeeded();

private:
    void loadTranslation(const QString &lang);
    void persist(const QString &lang);
    QString loadPersisted() const;

    QString m_language;
    QTranslator m_translator;
};

} // namespace fsnext
