// SPDX-License-Identifier: Proprietary
#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

namespace fsnext {

// BadWordFilter — classifies a free-text search keyword as "clean" or
// "blocked" using a packaged JSON dictionary
// (qrc:/data/search_bad_words.json). The dictionary mixes English and
// Vietnamese entries; the latter appear in two forms (with and without
// diacritics). Matching is tokenized + n-gram (1..3) to catch multi-word
// phrases like "phim sex" while keeping single tokens such as "essex" safe
// thanks to word-boundary splitting.
//
// Exposed to QML as a context property so the homepage search input can
// gate user keywords before issuing any API call.
class BadWordFilter : public QObject {
    Q_OBJECT
public:
    enum Result {
        Clean   = 0,
        Blocked = 1,
    };
    Q_ENUM(Result)

    explicit BadWordFilter(QObject *parent = nullptr);

    // Load the bundled JSON dictionary. Returns true when at least one
    // entry was parsed; false on parse error or empty file (in which case
    // every input is treated as Clean — fail-open is intentional so a
    // packaging mishap can never lock users out of search).
    bool loadFromResource(const QString &resourcePath = QStringLiteral(":/data/search_bad_words.json"));

    // Convenience for tests / future override file support.
    bool loadFromFile(const QString &filePath);

    // Apply a runtime override file from %APPDATA%/FsNext/. Format:
    //   { "add":    ["word1", "word2_with_underscore"],
    //     "remove": ["word_to_unblock"] }
    // Both keys optional. Words are normalised the same way as the
    // bundled dictionary (lowercase + spaces→'_'); diacritic variants
    // get auto-added to the stripped set. Missing file is not an error.
    // Returns the number of words actually added + removed.
    int applyOverridesFromAppData();

    // Classify a single user keyword. Returns Result + the offending word
    // (only meaningful for Blocked). URLs are NEVER passed through here —
    // the caller is expected to short-circuit on fshare.vn URLs before
    // invoking the filter.
    Q_INVOKABLE int check(const QString &keyword, QString *hitWord = nullptr) const;

    // QML-friendly variant: returns the hit word ("" when clean).
    Q_INVOKABLE QString checkHit(const QString &keyword) const;

    // Exact-phrase variant of check(): the whole phrase is normalised once
    // (lower + spaces→'_' + diacritic-strip) and looked up as a SINGLE key.
    // No tokenisation, no n-gram expansion — used by the homepage search
    // "quoted mode" where the user wraps the keyword in "…" to opt out of
    // substring-style splitting that might wrongly reject legitimate words
    // sharing a stem with a dictionary entry (e.g. searching "lồng tiếng"
    // when the dictionary contains "lồn"). A quoted phrase that EXACTLY
    // matches a bad-word entry is still rejected — quoting is a precision
    // flag, not a bypass.
    Q_INVOKABLE int checkExactPhrase(const QString &phrase,
                                     QString *hitWord = nullptr) const;

    // Diagnostics
    int entryCount() const { return m_raw.size() + m_stripped.size(); }

private:
    // Two parallel sets:
    //   m_raw      — keys with diacritics preserved (e.g. "cặc", "hiếp_dâm")
    //   m_stripped — same keys after Unicode NFD + combining-mark removal
    //                + lowercase (e.g. "cac", "hiep_dam"). Some JSON entries
    //                are already in this form (e.g. "hiep_dam") — they're
    //                indexed in m_stripped only.
    QSet<QString> m_raw;
    QSet<QString> m_stripped;

    // Cap matches at trigram — every dictionary entry in the bundled file
    // is ≤ 2 tokens, but we leave headroom for future additions.
    static constexpr int kMaxNgram = 3;

    static QString stripDiacritics(const QString &s);
    static QString normalizeSpaces(const QString &s);  // collapse to single '_'
};

} // namespace fsnext
