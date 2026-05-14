#pragma once

#include "core/models/FileItem.h"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>

namespace fsnext {

class FshareApi;

/// Resolves Fshare file URLs with bounded concurrency (2–4 parallel API calls).
///
/// Each resolved FileItem is emitted via itemResolved(). The caller is
/// responsible for caching — typically by connecting itemResolved to
/// FileCacheService or FileListModel.
///
/// Usage:
///   resolver->resolve(urls, /*concurrency=*/3);
///   connect(resolver, &BatchFileResolver::itemResolved, ...);
///   connect(resolver, &BatchFileResolver::batchCompleted, ...);
class BatchFileResolver : public QObject
{
    Q_OBJECT

public:
    explicit BatchFileResolver(FshareApi *api,
                               QObject   *parent = nullptr);
    ~BatchFileResolver() override = default;

    /// Start resolving a list of Fshare URLs.
    /// @param urls         list of Fshare URLs (e.g. "https://www.fshare.vn/file/XXXX")
    /// @param concurrency  maximum concurrent API calls (2–4 recommended)
    void resolve(const QStringList &urls, int concurrency = 3);

    /// Cancel any in-progress resolution.
    void cancel();

    /// Returns true while a batch is being resolved.
    bool isRunning() const { return m_running.load(); }

signals:
    /// Emitted for each successfully resolved file.
    void itemResolved(const fsnext::FileItem &item);

    /// Emitted when a single URL fails to resolve.
    void itemFailed(const QString &url, const QString &error);

    /// Progress update: @p completed out of @p total items processed.
    void batchProgress(int completed, int total);

    /// All items have been processed.
    void batchCompleted(int total, int succeeded, int failed);

private:
    void processQueue();
    void scheduleNext();

    FshareApi *m_api = nullptr;

    // Batch state — guarded by m_mutex
    QMutex       m_mutex;
    QStringList  m_pendingUrls;
    int          m_total       = 0;
    int          m_completed   = 0;
    int          m_succeeded   = 0;
    int          m_failed      = 0;
    int          m_concurrency = 3;
    int          m_inFlight    = 0;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancelled{false};
};

} // namespace fsnext
