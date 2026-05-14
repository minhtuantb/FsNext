// SPDX-License-Identifier: Proprietary
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "viewmodels/FileListModel.h"

namespace fsnext {

class BadWordFilter;
class DownloadViewModel;
class FshareApi;

// HomeSearchViewModel — stateful classifier for the homepage search input.
//
// Walks every keystroke / submit through the pipeline:
//   1. Trim + length check (must be ≥ 3 chars for a keyword search).
//   2. Detect Fshare share URLs (one or many file/folder links).
//   3. Bad-word filter (only when the input is a keyword, not a URL).
//   4. Otherwise: keyword ready for API search.
//
// The current state is reflected through Q_PROPERTYs so the HomePage QML
// can bind hint text, border colour, and submit-button enablement without
// re-implementing the rule set. submit() additionally emits a routing
// signal for the parent (Main.qml) so the actual action — open a folder
// browser, open a file detail sheet, kick off downloads, or query the API
// — happens at the page-host level.
class HomeSearchViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int     state    READ state    NOTIFY stateChanged)
    Q_PROPERTY(QString hint     READ hint     NOTIFY stateChanged)
    Q_PROPERTY(QString hitWord  READ hitWord  NOTIFY stateChanged)
    Q_PROPERTY(QString keyword  READ keyword  NOTIFY stateChanged)
    Q_PROPERTY(int     minLength READ minLength CONSTANT)

    // Phase-3: inline overlay for keyword results.
    Q_PROPERTY(FileListModel *resultsModel READ resultsModel CONSTANT)
    Q_PROPERTY(bool isSearching   READ isSearching   NOTIFY searchProgressChanged)
    Q_PROPERTY(bool hasResults    READ hasResults    NOTIFY searchProgressChanged)
    Q_PROPERTY(bool noResultsHit  READ noResultsHit  NOTIFY searchProgressChanged)
    Q_PROPERTY(QString resultsForKeyword READ resultsForKeyword NOTIFY searchProgressChanged)
public:
    // Mirrors the QML side. Order matters: the higher the value, the more
    // urgent the visual treatment HomePage.qml gives it.
    enum State {
        Idle           = 0,   // Empty input
        TooShort       = 1,   // 1-2 chars — show inline hint, block submit
        UrlFile        = 2,   // Exactly one fshare.vn/file/<code> URL
        UrlFolder      = 3,   // Exactly one fshare.vn/folder/<code> URL
        UrlMultiple    = 4,   // 2+ Fshare URLs — bulk download path
        Keyword        = 5,   // Clean, ≥ 3 chars — API search
        Blocked        = 6,   // Hits the bad-word filter
    };
    Q_ENUM(State)

    explicit HomeSearchViewModel(BadWordFilter     *filter,
                                  DownloadViewModel *downloadVm,
                                  FshareApi         *api,
                                  QObject           *parent = nullptr);

    int     state()   const { return m_state; }
    QString hint()    const { return m_hint; }
    QString hitWord() const { return m_hitWord; }
    QString keyword() const { return m_keyword; }
    int     minLength() const { return kMinLength; }

    FileListModel *resultsModel() const { return m_resultsModel; }
    bool isSearching()   const { return m_searching; }
    bool hasResults()    const { return m_resultsModel && m_resultsModel->count() > 0; }
    bool noResultsHit()  const { return m_noResultsHit; }
    QString resultsForKeyword() const { return m_resultsForKeyword; }

    // Re-classify live as the user types. Called by HomePage on every
    // TextField change so the chip/hint UI stays accurate. Does NOT emit
    // routing signals — see submit().
    Q_INVOKABLE void classify(const QString &text);

    // Called when the user hits Enter (or presses the submit button).
    // Emits exactly one of the route* signals — or none, if state is
    // Idle/TooShort/Blocked. Returns the resulting state.
    Q_INVOKABLE int submit(const QString &text);

    // Clear results model + cancel any in-flight search. Called by QML
    // when the search box is emptied or the overlay is dismissed.
    Q_INVOKABLE void clearResults();

signals:
    void stateChanged();
    void searchProgressChanged();

    // Routing — Main.qml hooks these to open the correct surface.
    void routeFileUrl     (const QString &url);
    void routeFolderUrl   (const QString &url);
    void routeMultipleUrls(const QString &newlineJoined);
    void routeKeyword     (const QString &keyword);

    // Soft feedback when the input is rejected. UI shows a toast / inline
    // banner; nothing else fires.
    void rejectedTooShort();
    void rejectedBadWord(const QString &hitWord);

private:
    static constexpr int kMinLength      = 3;
    static constexpr int kDebounceMs     = 250;   // 250ms keystroke quiet window
    static constexpr int kPerPage        = 30;    // first-page results to fetch

    void setState(State s, const QString &hint, const QString &hitWord, const QString &keyword);

    // Phase-3 helpers ─────────────────────────────────────────
    void scheduleKeywordSearch(const QString &keyword);
    void runKeywordSearchNow();
    void setSearching(bool v);

    BadWordFilter     *m_filter      = nullptr;
    DownloadViewModel *m_downloadVm  = nullptr;
    FshareApi         *m_api         = nullptr;
    FileListModel     *m_resultsModel = nullptr;

    int     m_state   = Idle;
    QString m_hint;
    QString m_hitWord;
    QString m_keyword;

    // Inline-search state
    QTimer  m_debounceTimer;
    QString m_pendingKeyword;       // value to fetch on debounce fire
    QString m_inflightKeyword;      // value currently being fetched (for cancel-tracking)
    QString m_resultsForKeyword;    // keyword the resultsModel currently reflects
    bool    m_searching     = false;
    bool    m_noResultsHit  = false;
    // Monotonically increasing request ID — lets late-arriving async
    // responses know whether they're still relevant or were superseded
    // by a newer keystroke.
    quint64 m_requestSeq    = 0;
};

} // namespace fsnext
