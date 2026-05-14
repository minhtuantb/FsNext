#pragma once
#include "core/models/SyncFolder.h"
#include <QSettings>
#include <QString>
#include <QVector>

namespace fsnext {

// Persists the list of sync folders + their file entries to QSettings.
// Layout:
//   Sync/folders       = QStringList of folder IDs
//   Sync/<id>/...      = SyncFolder fields
//   Sync/<id>/files    = QStringList of relPaths
//   Sync/<id>/file/<h>/...  = SyncFileEntry fields (h = percent-encoded relPath)
//
// Scoped per user so switching accounts gives each user their own sync list.
class SyncRepository {
public:
    SyncRepository();

    // Must be called whenever the logged-in user changes. An empty id disables
    // all reads/writes (returns empty lists / no-ops) until a real id is set.
    void setUserId(const QString &userId);
    QString userId() const { return m_userId; }

    // ── Folders ────────────────────────────────────────────────────────────
    QVector<SyncFolder> loadFolders() const;
    void saveFolder(const SyncFolder &folder);
    void deleteFolder(const QString &folderId);

    // ── Global toggle (v6.0+) ──────────────────────────────────────────────
    // Master "auto-sync on/off" — applied across every folder so the user can
    // pause everything when on mobile data / travel without losing per-folder
    // state.  Persisted per user (alongside the folder list) so each account
    // gets its own master switch; logging out doesn't carry the pause over.
    // Default true so the upgrade is invisible for existing users.
    bool autoSyncEnabled() const;
    void setAutoSyncEnabled(bool enabled);

    // ── Files per folder ───────────────────────────────────────────────────
    QVector<SyncFileEntry> loadFiles(const QString &folderId) const;
    void saveFile(const SyncFileEntry &entry);
    void deleteFile(const QString &folderId, const QString &relPath);
    void deleteAllFiles(const QString &folderId);

    // ── Activity log (v6.0+) ───────────────────────────────────────────────
    // Per-user rolling activity feed — newest entries first, capped at
    // `kActivityCap`.  Stored as a packed QString (one entry per
    // tab-separated line) inside one QSettings key for compactness; reads
    // and writes are atomic at the line-level so concurrent writes don't
    // corrupt the file.  Plenty for an activity feed UI — not a SQL
    // analytics store.
    static constexpr int kActivityCap = 50;
    QVector<SyncActivityEntry> loadActivity() const;
    void appendActivity(const SyncActivityEntry &entry);
    void clearActivity();

private:
    mutable QSettings m_settings;
    QString m_userId;

    QString groupPrefix() const;  // returns "SyncByUser/<userId>"
};

} // namespace fsnext
