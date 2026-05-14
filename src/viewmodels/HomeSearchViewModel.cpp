// SPDX-License-Identifier: Proprietary
#include "HomeSearchViewModel.h"

#include "core/api/FshareApi.h"
#include "core/util/BadWordFilter.h"
#include "viewmodels/DownloadViewModel.h"

#include <QPointer>
#include <QStringList>
#include <QtConcurrent>

namespace fsnext {

HomeSearchViewModel::HomeSearchViewModel(BadWordFilter     *filter,
                                          DownloadViewModel *downloadVm,
                                          FshareApi         *api,
                                          QObject           *parent)
    : QObject(parent)
    , m_filter(filter)
    , m_downloadVm(downloadVm)
    , m_api(api)
    , m_resultsModel(new FileListModel(this))
{
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(kDebounceMs);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &HomeSearchViewModel::runKeywordSearchNow);
}

void HomeSearchViewModel::setState(State s, const QString &hint,
                                    const QString &hitWord, const QString &keyword)
{
    if (m_state == s && m_hint == hint && m_hitWord == hitWord && m_keyword == keyword)
        return;
    m_state   = s;
    m_hint    = hint;
    m_hitWord = hitWord;
    m_keyword = keyword;
    emit stateChanged();
}

void HomeSearchViewModel::classify(const QString &text)
{
    const QString trimmed = text.trimmed();

    if (trimmed.isEmpty()) {
        setState(Idle, QString{}, QString{}, QString{});
        clearResults();
        return;
    }

    // URL detection first — URLs are usually > 3 chars but we still want
    // them to bypass the bad-word filter and the length minimum.
    if (m_downloadVm) {
        const QString extracted = m_downloadVm->extractFshareLinks(trimmed);
        if (!extracted.isEmpty()) {
            const QStringList urls = extracted.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            // A URL classification supersedes any in-flight keyword search.
            clearResults();
            if (urls.size() >= 2) {
                setState(UrlMultiple, tr("Phát hiện %1 link · Nhấn Enter để mở tải hàng loạt")
                                          .arg(urls.size()),
                         QString{}, extracted);
                return;
            }
            const QString one = urls.first();
            // /folder/ vs /file/ — case-insensitive, since the regex tolerates both.
            const bool isFolder = one.contains(QStringLiteral("/folder/"), Qt::CaseInsensitive);
            if (isFolder) {
                setState(UrlFolder, tr("Link folder · Nhấn Enter để duyệt"),
                         QString{}, one);
            } else {
                setState(UrlFile, tr("Link file · Nhấn Enter để xem chi tiết"),
                         QString{}, one);
            }
            return;
        }
    }

    // Plain keyword path — enforce the minimum length first so we don't
    // spend cycles in the bad-word filter for a 1-2 char fragment.
    if (trimmed.size() < kMinLength) {
        setState(TooShort, tr("Nhập tối thiểu %1 ký tự để tìm kiếm").arg(kMinLength),
                 QString{}, trimmed);
        clearResults();
        return;
    }

    // Bad-word check. Fail-open: if the filter is null or empty, treat
    // every input as clean (logged once at startup).
    if (m_filter) {
        QString hit;
        if (m_filter->check(trimmed, &hit) == BadWordFilter::Blocked) {
            setState(Blocked,
                     tr("Từ khoá chứa nội dung không phù hợp"),
                     hit, trimmed);
            clearResults();
            return;
        }
    }

    setState(Keyword, tr("Nhấn Enter để tìm kiếm"), QString{}, trimmed);
    scheduleKeywordSearch(trimmed);
}

int HomeSearchViewModel::submit(const QString &text)
{
    classify(text);
    switch (m_state) {
    case Idle:
        return m_state;
    case TooShort:
        emit rejectedTooShort();
        return m_state;
    case Blocked:
        emit rejectedBadWord(m_hitWord);
        return m_state;
    case UrlFile:
        emit routeFileUrl(m_keyword);
        return m_state;
    case UrlFolder:
        emit routeFolderUrl(m_keyword);
        return m_state;
    case UrlMultiple:
        emit routeMultipleUrls(m_keyword);
        return m_state;
    case Keyword:
        // Skip the debounce wait — submit is an explicit "search now"
        // signal from the user. If a search is already in flight for the
        // same keyword, runKeywordSearchNow short-circuits.
        m_debounceTimer.stop();
        runKeywordSearchNow();
        emit routeKeyword(m_keyword);
        return m_state;
    }
    return m_state;
}

// ─── Phase-3 inline keyword search ─────────────────────────────────────────

void HomeSearchViewModel::scheduleKeywordSearch(const QString &keyword)
{
    m_pendingKeyword = keyword;
    // Reset the timer so each keystroke pushes the fetch out kDebounceMs.
    m_debounceTimer.start();

    // Visually surface "loading" right away if we're going to issue a
    // fresh search — keeps the overlay from flashing empty between
    // keystrokes when the previous request already returned.
    if (!m_searching) setSearching(true);
}

void HomeSearchViewModel::runKeywordSearchNow()
{
    if (!m_api || m_pendingKeyword.isEmpty()) return;

    const QString keyword = m_pendingKeyword;
    if (keyword == m_inflightKeyword && m_searching) return;  // already fetching this exact term

    m_inflightKeyword = keyword;
    const quint64 mySeq = ++m_requestSeq;
    setSearching(true);

    FshareApi *api = m_api;
    QPointer<HomeSearchViewModel> guard(this);
    QtConcurrent::run([api, keyword, guard, mySeq]() {
        if (!guard) return;
        auto resp = api->searchFiles(keyword, /*page=*/1);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, keyword, mySeq]() {
            if (!guard) return;
            // Stale response — a newer keystroke already kicked off another
            // request. Drop the data quietly so we don't overwrite fresher
            // results with older ones.
            if (mySeq != guard->m_requestSeq) return;

            guard->setSearching(false);

            if (resp.isError()) {
                // Soft failure: keep the existing results model intact so the
                // user doesn't lose what they had. Just clear the
                // "currentlyShown keyword" marker. The overlay's empty/loading
                // state will then reflect the absence of data.
                guard->m_resultsForKeyword = keyword;
                guard->m_noResultsHit = false;
                emit guard->searchProgressChanged();
                return;
            }

            const auto items = resp.data();
            guard->m_resultsModel->resetItems(items);
            guard->m_resultsForKeyword = keyword;
            guard->m_noResultsHit = items.isEmpty();
            emit guard->searchProgressChanged();
        });
    });
}

void HomeSearchViewModel::clearResults()
{
    m_debounceTimer.stop();
    m_pendingKeyword.clear();
    // Don't bump m_inflightKeyword — let any in-flight callback notice the
    // request-sequence mismatch and discard itself.
    if (m_resultsModel->count() > 0) m_resultsModel->resetItems({});
    m_resultsForKeyword.clear();
    m_noResultsHit = false;
    setSearching(false);
}

void HomeSearchViewModel::setSearching(bool v)
{
    if (m_searching == v) return;
    m_searching = v;
    emit searchProgressChanged();
}

} // namespace fsnext
