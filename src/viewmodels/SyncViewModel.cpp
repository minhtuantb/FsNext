#include "SyncViewModel.h"
#include "core/services/SyncService.h"
#include "core/util/FormatUtil.h"

#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QPointer>
#include <QUrl>
#include <QtConcurrent>

namespace fsnext {

// ─────────────────────────────────────────────────────────────────────────────
// SyncFoldersModel
// ─────────────────────────────────────────────────────────────────────────────

SyncFoldersModel::SyncFoldersModel(QObject *parent) : QAbstractListModel(parent) {}

int SyncFoldersModel::rowCount(const QModelIndex &) const { return m_folders.size(); }

QVariant SyncFoldersModel::data(const QModelIndex &i, int role) const
{
    if (!i.isValid() || i.row() < 0 || i.row() >= m_folders.size()) return {};
    const SyncFolder &f = m_folders.at(i.row());
    switch (role) {
    case IdRole:                 return f.id;
    case LocalPathRole:          return f.localPath;
    case FshareFolderNameRole:   return f.fshareFolderName;
    case EnabledRole:            return f.enabled;
    case DeleteAfterUploadRole:  return f.deleteAfterUpload;
    case LastScanAtRole:
        return f.lastScanAt.isValid()
            ? QLocale().toString(f.lastScanAt, QLocale::ShortFormat) : QString{};
    case FileCountRole:          return m_counts.value(f.id).first;
    case SyncedCountRole:        return m_counts.value(f.id).second;
    case FailedCountRole:        return m_failed.value(f.id);
    // v6.0 per-folder settings — expose for dialog pre-population.
    case WatchSubfoldersRole:    return f.watchSubfolders;
    case IgnorePatternsRole:     return f.ignorePatterns;
    case SpeedLimitKBpsRole:
        // 0 = unlimited; otherwise convert bytes → kilobytes for UI which
        // works in KB/s units (matches WatchFolderSettingsDialog).
        return f.speedLimitBps <= 0
            ? 0
            : static_cast<int>(f.speedLimitBps / 1024);
    case StatusRole:
        return m_state.value(f.id,
            static_cast<int>(SyncFoldersModel::SyncFolderUiState::Idle));
    case StatusTextRole: {
        const auto s = static_cast<SyncFoldersModel::SyncFolderUiState>(
            m_state.value(f.id,
                static_cast<int>(SyncFoldersModel::SyncFolderUiState::Idle)));
        switch (s) {
        case SyncFolderUiState::Idle:      return QObject::tr("Đã đồng bộ");
        case SyncFolderUiState::Uploading: return QObject::tr("Đang tải lên");
        case SyncFolderUiState::Paused:    return QObject::tr("Tạm dừng");
        case SyncFolderUiState::Error:     return QObject::tr("Lỗi");
        case SyncFolderUiState::Missing:   return QObject::tr("Mất thư mục");
        }
        return QString{};
    }
    case UploadProgressRole:  return m_progress.value(f.id).ratio;
    case UploadSpeedTextRole: return m_progress.value(f.id).speedText;
    case UploadEtaTextRole:   return m_progress.value(f.id).etaText;
    }
    return {};
}

QHash<int, QByteArray> SyncFoldersModel::roleNames() const
{
    return {
        { IdRole,                 "id" },
        { LocalPathRole,          "localPath" },
        { FshareFolderNameRole,   "fshareFolderName" },
        { EnabledRole,            "enabled" },
        { DeleteAfterUploadRole,  "deleteAfterUpload" },
        { LastScanAtRole,         "lastScanAt" },
        { FileCountRole,          "fileCount" },
        { SyncedCountRole,        "syncedCount" },
        { FailedCountRole,        "failedCount" },
        { WatchSubfoldersRole,    "watchSubfolders" },
        { IgnorePatternsRole,     "ignorePatterns" },
        { SpeedLimitKBpsRole,     "speedLimitKBps" },
        { StatusRole,             "status" },
        { StatusTextRole,         "statusText" },
        { UploadProgressRole,     "uploadProgress" },
        { UploadSpeedTextRole,    "uploadSpeedText" },
        { UploadEtaTextRole,      "uploadEtaText" },
    };
}

void SyncFoldersModel::setFolderProgress(const QString &folderId, qreal ratio,
                                          const QString &speedText,
                                          const QString &etaText)
{
    const int row = m_indexById.value(folderId, -1);
    if (row < 0) return;
    auto &p = m_progress[folderId];
    QVector<int> roles;
    if (!qFuzzyCompare(p.ratio + 1.0, ratio + 1.0)) {
        p.ratio = ratio;
        roles << UploadProgressRole;
    }
    if (p.speedText != speedText) { p.speedText = speedText; roles << UploadSpeedTextRole; }
    if (p.etaText   != etaText)   { p.etaText   = etaText;   roles << UploadEtaTextRole; }
    if (!roles.isEmpty()) emit dataChanged(index(row), index(row), roles);
}

void SyncFoldersModel::setFolders(const QVector<SyncFolder> &folders,
                                    const QHash<QString, QPair<int,int>> &counts,
                                    const QHash<QString, int> &failed,
                                    const QHash<QString, int> &state)
{
    const int oldCount = m_folders.size();

    // Fast path — same set of folder ids in the same order. The common case
    // during active sync is "folder list unchanged; counts updated". A full
    // modelReset would force every QML delegate to re-materialize even for a
    // single synced-count tick, so we diff row-by-row and only emit
    // dataChanged for rows whose user-visible fields actually moved.
    bool sameShape = folders.size() == m_folders.size();
    if (sameShape) {
        for (int i = 0; i < folders.size(); ++i) {
            if (folders[i].id != m_folders[i].id) { sameShape = false; break; }
        }
    }

    if (sameShape) {
        for (int i = 0; i < folders.size(); ++i) {
            const SyncFolder &nf = folders[i];
            const SyncFolder &of = m_folders[i];

            QVector<int> roles;
            if (nf.localPath != of.localPath)              roles << LocalPathRole;
            if (nf.fshareFolderName != of.fshareFolderName) roles << FshareFolderNameRole;
            if (nf.enabled != of.enabled)                   roles << EnabledRole;
            if (nf.deleteAfterUpload != of.deleteAfterUpload) roles << DeleteAfterUploadRole;
            if (nf.lastScanAt != of.lastScanAt)             roles << LastScanAtRole;
            // Per-folder settings — settings dialog edits flow through saveFolder
            // → loadFolders → here.  Without these, a Save in the dialog would
            // persist correctly but the open dialog would still display stale
            // values until something else triggered a full reset.
            if (nf.watchSubfolders != of.watchSubfolders)   roles << WatchSubfoldersRole;
            if (nf.ignorePatterns != of.ignorePatterns)     roles << IgnorePatternsRole;
            if (nf.speedLimitBps != of.speedLimitBps)       roles << SpeedLimitKBpsRole;

            const auto oldPair = m_counts.value(of.id);
            const auto newPair = counts.value(nf.id);
            if (oldPair.first  != newPair.first)            roles << FileCountRole;
            if (oldPair.second != newPair.second)           roles << SyncedCountRole;

            if (m_failed.value(of.id) != failed.value(nf.id)) roles << FailedCountRole;

            // Status pill: cheap to compute, dataChanged only when the int
            // value actually flips so we don't thrash QML's
            // delegate cache on every counts-only update.
            if (m_state.value(of.id) != state.value(nf.id)) {
                roles << StatusRole;
                roles << StatusTextRole;
            }

            if (!roles.isEmpty())
                emit dataChanged(index(i), index(i), roles);
        }
        m_folders = folders;
        m_counts  = counts;
        m_failed  = failed;
        m_state   = state;
        return;
    }

    // Slow path — folders added/removed/reordered. Full reset is cheaper than
    // computing minimal insert/remove ranges for a list that's usually under
    // 10 entries (SyncService::kMaxFolders).
    beginResetModel();
    m_folders = folders;
    m_counts  = counts;
    m_failed  = failed;
    m_state   = state;
    m_indexById.clear();
    m_indexById.reserve(m_folders.size());
    for (int i = 0; i < m_folders.size(); ++i)
        m_indexById.insert(m_folders[i].id, i);
    endResetModel();
    if (oldCount != m_folders.size()) emit countChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// SyncFilesModel
// ─────────────────────────────────────────────────────────────────────────────

SyncFilesModel::SyncFilesModel(QObject *parent) : QAbstractListModel(parent) {}

int SyncFilesModel::rowCount(const QModelIndex &) const { return m_files.size(); }

static QString stateText(SyncFileState s)
{
    switch (s) {
    case SyncFileState::Pending:   return QObject::tr("Chờ");
    case SyncFileState::Uploading: return QObject::tr("Đang tải lên");
    case SyncFileState::Synced:    return QObject::tr("Đã đồng bộ");
    case SyncFileState::Failed:    return QObject::tr("Thất bại");
    case SyncFileState::Missing:   return QObject::tr("Đã xoá local");
    }
    return {};
}

QVariant SyncFilesModel::data(const QModelIndex &i, int role) const
{
    if (!i.isValid() || i.row() < 0 || i.row() >= m_files.size()) return {};
    const SyncFileEntry &e = m_files.at(i.row());
    switch (role) {
    case RelPathRole:        return e.relPath;
    case SizeRole:            return FormatUtil::humanBytes(e.size);
    case MtimeRole:           return QLocale().toString(
        QDateTime::fromSecsSinceEpoch(e.mtime), QLocale::ShortFormat);
    case StateRole:           return static_cast<int>(e.state);
    case StateTextRole:       return stateText(e.state);
    case LinkcodeRole:        return e.linkcode;
    case UploadedAtRole:
        return e.uploadedAt.isValid()
            ? QLocale().toString(e.uploadedAt, QLocale::ShortFormat) : QString{};
    case ErrorMessageRole:    return e.errorMessage;
    }
    return {};
}

QHash<int, QByteArray> SyncFilesModel::roleNames() const
{
    return {
        { RelPathRole,      "relPath" },
        { SizeRole,         "size" },
        { MtimeRole,        "mtime" },
        { StateRole,        "fileState" },
        { StateTextRole,    "stateText" },
        { LinkcodeRole,     "linkcode" },
        { UploadedAtRole,   "uploadedAt" },
        { ErrorMessageRole, "errorMessage" },
    };
}

void SyncFilesModel::setFiles(const QVector<SyncFileEntry> &files)
{
    const int oldCount = m_files.size();

    // Fast path — same relPaths in the same order. Sync scans re-emit the
    // full file list on every state transition (Pending → Uploading →
    // Synced), so for a 1000-file sync folder we'd otherwise fire hundreds
    // of full resets during a single upload burst. Detect the shape-preserved
    // case and emit targeted dataChanged per changed row instead.
    bool sameShape = files.size() == m_files.size();
    if (sameShape) {
        for (int i = 0; i < files.size(); ++i) {
            if (files[i].relPath != m_files[i].relPath) { sameShape = false; break; }
        }
    }

    if (sameShape) {
        for (int i = 0; i < files.size(); ++i) {
            const SyncFileEntry &ne = files[i];
            const SyncFileEntry &oe = m_files[i];

            QVector<int> roles;
            if (ne.size != oe.size)                 roles << SizeRole;
            if (ne.mtime != oe.mtime)               roles << MtimeRole;
            if (ne.state != oe.state) {
                roles << StateRole;
                roles << StateTextRole;
            }
            if (ne.linkcode != oe.linkcode)         roles << LinkcodeRole;
            if (ne.uploadedAt != oe.uploadedAt)     roles << UploadedAtRole;
            if (ne.errorMessage != oe.errorMessage) roles << ErrorMessageRole;

            if (!roles.isEmpty())
                emit dataChanged(index(i), index(i), roles);
        }
        m_files = files;
        return;
    }

    // Slow path — file-set changed (new file discovered, one deleted). The
    // sync engine doesn't yet emit incremental add/remove, so we fall back
    // to a full reset. This is still relatively cold — shape changes only
    // happen on scan, not on every state tick.
    beginResetModel();
    m_files = files;
    m_indexByRelPath.clear();
    m_indexByRelPath.reserve(m_files.size());
    for (int i = 0; i < m_files.size(); ++i)
        m_indexByRelPath.insert(m_files[i].relPath, i);
    endResetModel();
    if (oldCount != m_files.size()) emit countChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// SyncViewModel
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// SyncActivityModel
// ─────────────────────────────────────────────────────────────────────────────

SyncActivityModel::SyncActivityModel(QObject *parent) : QAbstractListModel(parent) {}

int SyncActivityModel::rowCount(const QModelIndex &) const { return m_entries.size(); }

static QString humanAgo(const QDateTime &at)
{
    if (!at.isValid()) return {};
    const qint64 secs = at.secsTo(QDateTime::currentDateTime());
    if (secs < 60)    return QObject::tr("vừa xong");
    if (secs < 3600)  return QObject::tr("%1 phút").arg(secs / 60);
    if (secs < 86400) return QObject::tr("%1 giờ").arg(secs / 3600);
    return QObject::tr("%1 ngày").arg(secs / 86400);
}

QVariant SyncActivityModel::data(const QModelIndex &i, int role) const
{
    if (!i.isValid() || i.row() < 0 || i.row() >= m_entries.size()) return {};
    const SyncActivityEntry &e = m_entries.at(i.row());
    switch (role) {
    case FolderIdRole:    return e.folderId;
    case FolderLabelRole: return e.folderLabel;
    case RelPathRole:     return e.relPath;
    case SizeBytesRole:   return e.sizeBytes;
    case KindRole:        return static_cast<int>(e.kind);
    case KindTextRole:
        switch (e.kind) {
        case SyncActivityKind::Uploaded:      return QObject::tr("Đã tải lên");
        case SyncActivityKind::Failed:        return QObject::tr("Lỗi");
        case SyncActivityKind::DeletedLocal:  return QObject::tr("Đã xoá local");
        case SyncActivityKind::FolderAdded:   return QObject::tr("Thêm thư mục");
        case SyncActivityKind::FolderRemoved: return QObject::tr("Xoá thư mục");
        }
        return QString{};
    case MessageRole:     return e.message;
    case AtRole:
        return e.at.isValid()
            ? QLocale().toString(e.at, QLocale::ShortFormat) : QString{};
    case AtAgoRole:       return humanAgo(e.at);
    }
    return {};
}

QHash<int, QByteArray> SyncActivityModel::roleNames() const
{
    return {
        { FolderIdRole,    "folderId" },
        { FolderLabelRole, "folderLabel" },
        { RelPathRole,     "relPath" },
        { SizeBytesRole,   "sizeBytes" },
        { KindRole,        "kind" },
        { KindTextRole,    "kindText" },
        { MessageRole,     "message" },
        { AtRole,          "at" },
        { AtAgoRole,       "atAgo" },
    };
}

void SyncActivityModel::setEntries(const QVector<SyncActivityEntry> &entries)
{
    // Activity feed is small (≤50) and rewritten in bulk; a full reset is
    // simpler than diffing.  countChanged emits regardless so QML's count
    // binding always refreshes when the list shape moves.
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// SyncViewModel
// ─────────────────────────────────────────────────────────────────────────────

SyncViewModel::SyncViewModel(SyncService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_foldersModel(new SyncFoldersModel(this))
    , m_filesModel(new SyncFilesModel(this))
    , m_activityModel(new SyncActivityModel(this))
{
    if (!m_service) return;

    connect(m_service, &SyncService::foldersChanged, this, [this]() {
        refreshFolders();
        // If the active folder was removed, snap to the first remaining.
        const auto all = m_service->folders();
        bool stillValid = false;
        for (const auto &f : all)
            if (f.id == m_activeFolderId) { stillValid = true; break; }
        if (!stillValid && !all.isEmpty()) {
            setActiveFolderId(all.first().id);
        } else if (all.isEmpty() && !m_activeFolderId.isEmpty()) {
            m_activeFolderId.clear();
            emit activeFolderIdChanged();
            emit activeFolderChanged();
            m_filesModel->setFiles({});
        } else if (stillValid) {
            // Same active folder, but its state (enabled/deleteAfterUpload/…)
            // may have been mutated. Re-emit so QML bindings on
            // activeEnabled / deleteAfterUpload pick up the new value.
            emit activeFolderChanged();
        }
        emit foldersChanged();
    });

    connect(m_service, &SyncService::folderFilesChanged, this, [this](const QString &id) {
        // Counts may have changed even for non-active folders.
        refreshFolders();
        if (id == m_activeFolderId) refreshFiles(id);
    });

    connect(m_service, &SyncService::syncError, this, [this](const QString &, const QString &msg) {
        emit addFolderError(msg);
    });
    connect(m_service, &SyncService::folderMissing, this,
        [this](const QString &, const QString &path) {
            emit addFolderError(QObject::tr("Thư mục không tìm thấy: %1").arg(path));
        });
    connect(m_service, &SyncService::folderSynced, this,
        [this](const QString &id, int count) {
            if (count > 0) emit infoMessage(QObject::tr("Đã đồng bộ %1 file").arg(count));
            if (id == m_activeFolderId) refreshFiles(id);
        });
    // Master toggle changes flow upward so QML's `autoSyncEnabled` binding
    // refreshes (header switch, sidebar badge gating, …).
    connect(m_service, &SyncService::autoSyncEnabledChanged, this,
        [this](bool) { emit autoSyncEnabledChanged(); });
    connect(m_service, &SyncService::activityChanged, this,
        &SyncViewModel::refreshActivity);
    // Per-folder live progress — translate raw bytes/speed to display
    // strings on this thread (avoids QML doing FormatUtil work inside a
    // binding eval) and push into the folders model.
    connect(m_service, &SyncService::folderProgressChanged, this,
        [this](const QString &folderId, qint64 done, qint64 total,
               double speedBps, qint64 etaSecs) {
            qreal ratio = -1.0;
            QString speedText;
            QString etaText;
            if (total > 0) {
                ratio = qBound<qreal>(0.0,
                                      static_cast<qreal>(done) / total,
                                      1.0);
            }
            if (speedBps > 1.0) {
                speedText = FormatUtil::humanBytes(static_cast<qint64>(speedBps))
                          + QStringLiteral("/s");
            }
            if (etaSecs > 0) {
                if (etaSecs < 60)        etaText = QObject::tr("~%1 giây").arg(etaSecs);
                else if (etaSecs < 3600) etaText = QObject::tr("~%1 phút").arg(etaSecs / 60);
                else                     etaText = QObject::tr("~%1 giờ").arg(etaSecs / 3600);
            }
            m_foldersModel->setFolderProgress(folderId, ratio, speedText, etaText);
        });

    refreshFolders();
    refreshActivity();
    const auto all = m_service->folders();
    if (!all.isEmpty()) setActiveFolderId(all.first().id);
}

void SyncViewModel::refreshActivity()
{
    if (!m_service) {
        m_activityModel->setEntries({});
        return;
    }
    m_activityModel->setEntries(m_service->loadActivity());
}

void SyncViewModel::clearActivity()
{
    if (m_service) m_service->clearActivity();
}

void SyncViewModel::pauseFolder(const QString &folderId)
{
    if (m_service) m_service->pauseFolder(folderId);
    emit infoMessage(QObject::tr("Đã tạm dừng tải lên thư mục"));
}

void SyncViewModel::resumeFolder(const QString &folderId)
{
    if (m_service) m_service->resumeFolder(folderId);
    emit infoMessage(QObject::tr("Đã tiếp tục tải lên thư mục"));
}

void SyncViewModel::retryFailed(const QString &folderId)
{
    if (m_service) m_service->retryFailed(folderId);
    emit infoMessage(QObject::tr("Đang thử tải lại các file lỗi..."));
}

int    SyncViewModel::maxFolders()     const { return SyncService::kMaxFolders; }
int    SyncViewModel::speedLimitKBps() const { return SyncService::kSpeedLimitBps / 1024; }
qint64 SyncViewModel::maxFileSize()    const { return SyncService::kMaxFileSize; }

bool SyncViewModel::autoSyncEnabled() const
{
    return m_service ? m_service->autoSyncEnabled() : false;
}

void SyncViewModel::setAutoSyncEnabled(bool enabled)
{
    if (m_service) m_service->setAutoSyncEnabled(enabled);
}

bool SyncViewModel::canAddMore() const
{
    return m_service && m_service->folderCount() < SyncService::kMaxFolders;
}

bool SyncViewModel::activeEnabled() const
{
    if (!m_service) return false;
    for (const auto &f : m_service->folders())
        if (f.id == m_activeFolderId) return f.enabled;
    return false;
}

bool SyncViewModel::activeDeleteAfterUpload() const
{
    if (!m_service) return false;
    for (const auto &f : m_service->folders())
        if (f.id == m_activeFolderId) return f.deleteAfterUpload;
    return false;
}

QString SyncViewModel::activeLocalPath() const
{
    if (!m_service) return {};
    for (const auto &f : m_service->folders())
        if (f.id == m_activeFolderId) return f.localPath;
    return {};
}

QString SyncViewModel::activeFshareFolderName() const
{
    if (!m_service) return {};
    for (const auto &f : m_service->folders())
        if (f.id == m_activeFolderId) return f.fshareFolderName;
    return {};
}

void SyncViewModel::setActiveFolderId(const QString &id)
{
    if (m_activeFolderId == id) return;
    m_activeFolderId = id;
    emit activeFolderIdChanged();
    emit activeFolderChanged();
    refreshFiles(id);
}

void SyncViewModel::addFolder(const QString &localPath)
{
    if (!m_service) return;
    const QString normalized = normalizeLocalPath(localPath);
    QString err;
    if (!m_service->addFolder(normalized, &err)) {
        emit addFolderError(err.isEmpty() ? QObject::tr("Không thể thêm thư mục") : err);
    }
}

void SyncViewModel::addFolderWithSettings(const QString &localPath,
                                          bool watchSubfolders,
                                          const QString &ignorePatterns,
                                          int speedLimitKBps,
                                          bool deleteAfterUpload)
{
    if (!m_service) return;
    const QString normalized = normalizeLocalPath(localPath);
    // UI works in KB/s, service stores bytes/s.  Treat <=0 as "unlimited"
    // so the dialog's "Không giới hạn" radio maps cleanly to 0.
    const qint64 speedBps = speedLimitKBps > 0
        ? static_cast<qint64>(speedLimitKBps) * 1024
        : 0;
    QString err;
    if (!m_service->addFolder(normalized, watchSubfolders, ignorePatterns,
                              speedBps, deleteAfterUpload, &err)) {
        emit addFolderError(err.isEmpty() ? QObject::tr("Không thể thêm thư mục") : err);
    }
}

void SyncViewModel::updateFolderSettings(const QString &folderId,
                                          bool watchSubfolders,
                                          bool deleteAfterUpload,
                                          const QString &ignorePatterns,
                                          int speedLimitKBps)
{
    if (!m_service) return;
    // Each setter is a no-op when the value is unchanged, so we can fire
    // them unconditionally and rely on SyncService to coalesce.  This keeps
    // the QML side simple — the dialog hands back a flat snapshot, we push
    // it through without diffing on the UI thread.
    m_service->setWatchSubfolders(folderId, watchSubfolders);
    m_service->setDeleteAfterUpload(folderId, deleteAfterUpload);
    m_service->setIgnorePatterns(folderId, ignorePatterns);
    const qint64 speedBps = speedLimitKBps > 0
        ? static_cast<qint64>(speedLimitKBps) * 1024
        : 0;
    m_service->setSpeedLimitBps(folderId, speedBps);
    emit infoMessage(QObject::tr("Đã lưu cài đặt thư mục"));
}

void SyncViewModel::removeFolder(const QString &folderId)
{
    if (m_service) m_service->removeFolder(folderId);
}

void SyncViewModel::setEnabled(const QString &folderId, bool enabled)
{
    if (m_service) m_service->setFolderEnabled(folderId, enabled);
}

void SyncViewModel::setDeleteAfterUpload(const QString &folderId, bool on)
{
    if (m_service) m_service->setDeleteAfterUpload(folderId, on);
}

void SyncViewModel::scanNow(const QString &folderId)
{
    if (m_service) m_service->scanFolder(folderId);
}

void SyncViewModel::requestPreview(const QString &localPath,
                                    bool watchSubfolders,
                                    const QString &ignorePatterns)
{
    if (!m_service) {
        emit previewReady(0, 0, QObject::tr("Sync service không khả dụng"));
        return;
    }
    const QString normalized = normalizeLocalPath(localPath);
    // Snapshot the service pointer + this-guard so the worker doesn't touch
    // half-destroyed objects if the user logs out mid-scan.  previewScan()
    // is const + read-only on m_folders so it's thread-safe to call here.
    SyncService *svc = m_service;
    QPointer<SyncViewModel> guard(this);
    QtConcurrent::run([guard, svc, normalized, watchSubfolders, ignorePatterns]() {
        const SyncService::PreviewResult r =
            svc->previewScan(normalized, watchSubfolders, ignorePatterns);
        if (!guard) return;
        QMetaObject::invokeMethod(guard.data(),
            [guard, r]() {
                if (!guard) return;
                emit guard->previewReady(r.fileCount, r.totalBytes, r.errorMessage);
            },
            Qt::QueuedConnection);
    });
}

void SyncViewModel::copyLinkFor(const QString &folderId, const QString &relPath)
{
    if (!m_service) return;
    const auto files = m_service->filesOf(folderId);
    for (const auto &e : files) {
        if (e.relPath == relPath && !e.linkcode.isEmpty()) {
            // Linkcode might already be a full URL (engine parses "url" field).
            // Normalize to Fshare file URL when it's just a code.
            QString url = e.linkcode;
            if (!url.startsWith(QStringLiteral("http"), Qt::CaseInsensitive))
                url = QStringLiteral("https://www.fshare.vn/file/") + url;
            QGuiApplication::clipboard()->setText(url);
            emit infoMessage(QObject::tr("Đã sao chép đường dẫn"));
            return;
        }
    }
}

void SyncViewModel::openLocalFolder(const QString &folderId)
{
    if (!m_service) return;
    for (const auto &f : m_service->folders()) {
        if (f.id == folderId) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(f.localPath));
            return;
        }
    }
}

void SyncViewModel::openFshareLink(const QString &folderId, const QString &relPath)
{
    if (!m_service) return;
    const auto files = m_service->filesOf(folderId);
    for (const auto &e : files) {
        if (e.relPath == relPath && !e.linkcode.isEmpty()) {
            QString url = e.linkcode;
            if (!url.startsWith(QStringLiteral("http"), Qt::CaseInsensitive))
                url = QStringLiteral("https://www.fshare.vn/file/") + url;
            QDesktopServices::openUrl(QUrl(url));
            return;
        }
    }
}

void SyncViewModel::resetFolderCache(const QString &folderId)
{
    if (!m_service) return;
    m_service->resetFolderState(folderId);
    emit infoMessage(QObject::tr("Đã xoá cache. Đang quét lại thư mục…"));
}

QString SyncViewModel::normalizeLocalPath(const QString &maybeUrl) const
{
    // QML FolderDialog passes "file:///D:/Foo" — convert to native path.
    if (maybeUrl.startsWith(QLatin1String("file://")))
        return QUrl(maybeUrl).toLocalFile();
    return maybeUrl;
}

void SyncViewModel::refreshFolders()
{
    if (!m_service) return;
    const auto all = m_service->folders();
    QHash<QString, QPair<int,int>> counts;
    QHash<QString, int> failed;
    QHash<QString, int> state;        // folderId → SyncFolderUiState (int)
    // Aggregate "needs attention" count for the sidebar badge — Pending,
    // Uploading and Failed all live in the same bucket from a user's
    // perspective ("file not yet successfully synced").  Missing/Synced
    // are settled states so they don't contribute.
    int newPending = 0;
    for (const auto &f : all) {
        const auto files = m_service->filesOf(f.id);
        int synced = 0;
        int failedCount = 0;
        int inflight    = 0;          // Pending + Uploading — used by status calc
        for (const auto &e : files) {
            switch (e.state) {
            case SyncFileState::Synced:    ++synced; break;
            case SyncFileState::Failed:    ++failedCount; ++newPending; break;
            case SyncFileState::Pending:
            case SyncFileState::Uploading: ++newPending; ++inflight; break;
            case SyncFileState::Missing:   break;
            }
        }
        counts.insert(f.id, { files.size(), synced });
        failed.insert(f.id, failedCount);

        // Derive the folder-level pill state.  Precedence matters:
        //   Missing > Paused > Uploading > Error > Idle.
        // i.e. a missing local path always reads "Mất thư mục", even if
        // the user previously paused or had failures; once they restore
        // the path the state cascades back through the other branches.
        using S = SyncFoldersModel::SyncFolderUiState;
        S s = S::Idle;
        if (!QFileInfo(f.localPath).isDir())          s = S::Missing;
        else if (!f.enabled)                           s = S::Paused;
        else if (inflight > 0)                         s = S::Uploading;
        else if (failedCount > 0)                      s = S::Error;
        state.insert(f.id, static_cast<int>(s));
    }
    m_foldersModel->setFolders(all, counts, failed, state);
    if (newPending != m_pendingCount) {
        m_pendingCount = newPending;
        emit pendingCountChanged();
    }
}

void SyncViewModel::refreshFiles(const QString &folderId)
{
    if (!m_service || folderId.isEmpty()) {
        m_filesModel->setFiles({});
        return;
    }
    m_filesModel->setFiles(m_service->filesOf(folderId));
}

} // namespace fsnext
