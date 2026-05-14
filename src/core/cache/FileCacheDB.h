#pragma once

#include "core/models/FileItem.h"
#include <QHash>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

namespace fsnext {

struct FolderSyncState {
    bool    exists     = false;
    bool    isComplete = false;
    qint64  syncedAt   = 0;   // unix seconds
};

// SQLite-backed metadata cache for file manager.
// All methods must be called from the thread that called open() (typically the main thread).
// Uses WAL mode for performance; FTS5 virtual table for full-text search with a LIKE fallback.
class FileCacheDB : public QObject
{
    Q_OBJECT

public:
    explicit FileCacheDB(QObject *parent = nullptr);
    ~FileCacheDB() override;

    // Open (or create) the database at dbPath.  Returns false on failure.
    bool open(const QString &dbPath);
    bool isOpen() const;

    // ── File metadata ────────────────────────────────────────
    void upsertFiles(const QString &userId, const QVector<FileItem> &items);

    QVector<FileItem> queryFiles(const QString &userId,
                                 const QString &parentId,
                                 const QString &sortKey    = QStringLiteral("name"),
                                 bool           sortAsc    = true,
                                 const QString &typeFilter = QStringLiteral("all"),
                                 int            limit      = 2000,
                                 int            offset     = 0);

    int countFiles(const QString &userId,
                   const QString &parentId,
                   const QString &typeFilter = QStringLiteral("all"));

    // ── Full-text search (FTS5, falls back to LIKE) ──────────
    QVector<FileItem> searchFts(const QString &userId,
                                const QString &keyword,
                                int            limit = 200);

    // ── Folder tree (folders only, whole account) ────────────
    QVector<FileItem> getFolderTree(const QString &userId);

    // ── Sync state ───────────────────────────────────────────
    FolderSyncState getSyncState(const QString &userId, const QString &folderId);
    void setFolderSyncComplete(const QString &userId, const QString &folderId);
    void setFolderSyncPartial(const QString &userId,  const QString &folderId);

    // Drop the sync-state row for one folder. Used by the manual Refresh
    // action so the next visit re-fetches from the API instead of serving
    // stale cache.
    void clearFolderSync(const QString &userId, const QString &folderId);

    // ── Mutations (applied optimistically after write ops) ───
    void removeFiles(const QStringList &linkcodes);
    void renameFile(const QString &linkcode, const QString &newName);
    void updateParent(const QStringList &linkcodes, const QString &newParentId);

    // ── Local file tracking (download/upload → local path) ────
    void upsertLocalFile(const QString &userId, const QString &linkcode,
                         const QString &localPath, const QString &fileName,
                         int64_t fileSize, const QString &transferType);

    QString getLocalPath(const QString &userId, const QString &linkcode);

    QHash<QString, QString> getLocalPaths(const QString &userId,
                                          const QStringList &linkcodes);

    void removeLocalFile(const QString &userId, const QString &linkcode);

    // Check if the local file still exists on disk; removes the row if not.
    // Returns true when the file is still present.
    bool verifyLocalFile(const QString &userId, const QString &linkcode);

    // ── Maintenance ──────────────────────────────────────────
    void clearUserCache(const QString &userId);

private:
    bool createSchema();

    bool         m_hasFts5 = false;
    QSqlDatabase m_db;
};

} // namespace fsnext
