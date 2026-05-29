#pragma once
// SPDX-License-Identifier: Proprietary
// FolderExpander — async recursive Fshare folder crawler.
//
// Given a folder URL (https://www.fshare.vn/folder/<code>), crawls the full
// sub-tree up to maxDepth levels, paginating each folder, and emits a flat
// list of TransferTask objects (one per file) ready to be enqueued by
// TransferService.
//
// Threading contract:
//   • Created on the main thread.
//   • start() launches a QtConcurrent worker to do all API work.
//   • All signals are emitted from the worker thread; Qt's auto-connection
//     delivers them to main-thread slots via the event loop (queued).
//   • finished() is ALWAYS emitted last — use it for cleanup (deleteLater).
//   • abort() sets an atomic flag; the worker stops at the next safe point
//     (after the current listFiles() call returns).

#include "core/models/FileItem.h"
#include "core/models/TransferTask.h"
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <atomic>

namespace fsnext {

class FshareApi;

class FolderExpander : public QObject
{
    Q_OBJECT

public:
    explicit FolderExpander(FshareApi  *api,
                            const QString &rootUrl,
                            const QString &savePath,
                            const QString &password,
                            int            segmentsPerFile,
                            int            maxDepth = 20,
                            QObject       *parent   = nullptr);

    // Begin the async crawl (call once).
    void start();

    // Request cancellation; finished() will still be emitted.
    void abort();

    QString groupId() const { return m_groupId; }
    QString rootUrl() const { return m_rootUrl;  }

signals:
    // Periodic progress update (folders scanned so far, files found so far).
    void scanProgress(int foldersScanned, int filesFound);

    // Emitted on successful completion — tasks list is ready.
    void completed(QVector<TransferTask> tasks);

    // Emitted when a (non-abort) error stops the crawl.
    void failed(const QString &error);

    // Always emitted last regardless of outcome — use to trigger cleanup.
    void finished();

private:
    void run();
    bool crawl(const QString &folderUrl, const QString &relPath, int depth);
    TransferTask makeTask(const FileItem &item, const QString &relPath) const;
    static QString sanitizeName(const QString &name);

    FshareApi *m_api;
    QString    m_rootUrl;
    QString    m_savePath;
    QString    m_password;
    int        m_segmentsPerFile;
    int        m_maxDepth;

    QString    m_groupId;
    QString    m_rootFolderName;
    QString    m_lastError;

    QVector<TransferTask> m_tasks;
    int m_foldersScanned = 0;
    // Linkcodes already descended into — cycle guard for crawl() (CRASH_AUDIT H9).
    QSet<QString> m_visited;

    std::atomic<bool> m_abort{false};
};

} // namespace fsnext
