// SPDX-License-Identifier: Proprietary
// UploadStagingViewModel — session-and-restart-stable holder for the upload
// "staging" state (files the user has selected but not yet started uploading).
//
// Lives on AppContext for the lifetime of the app, surviving page-Loader
// teardowns that destroy UploadPage when the user navigates away. Previously
// staging lived on FsUploadDialog (inside UploadPage), so every page switch lost
// the user's batch and a race during teardown could crash the app.
//
// Persists to SettingsRepository on every mutation (debounced 200 ms) so a
// crash or a deliberate restart preserves the batch. Hydrate revalidates each
// path: a file the user deleted between sessions stays in the list but with
// valid=false so the UI can flag it (commit silently skips invalid entries).

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

namespace fsnext {

class UploadViewModel;
class SettingsRepository;

class UploadStagingViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList stagedFiles READ stagedFiles NOTIFY stagedFilesChanged)
    Q_PROPERTY(bool hasStaged READ hasStaged NOTIFY stagedFilesChanged)
    Q_PROPERTY(QString targetFolder READ targetFolder WRITE setTargetFolder NOTIFY targetFolderChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool secured READ secured WRITE setSecured NOTIFY securedChanged)
    Q_PROPERTY(bool directLink READ directLink WRITE setDirectLink NOTIFY directLinkChanged)
    // True for ONE staging cycle after hydrate-from-disk on app start. UploadPage
    // uses it to gate the dialog: instead of auto-popping a modal on launch
    // (jarring), it shows an inline banner "Bạn còn N file từ phiên trước
    // [Tiếp tục] [Bỏ]". Cleared by acknowledgeRestored() or clear().
    Q_PROPERTY(bool restoredFromDisk READ restoredFromDisk NOTIFY restoredFromDiskChanged)
    // True for one tick after addFiles() makes an actual addition — UploadPage
    // uses this to decide "did the user just drop/pick a file, or did they just
    // navigate here?". Avoids auto-popping the dialog on every sidebar click
    // back to Upload when there's staging left over. Cleared by
    // acknowledgeShow() once the dialog is open.
    Q_PROPERTY(bool showRequested READ showRequested NOTIFY showRequestedChanged)

public:
    explicit UploadStagingViewModel(UploadViewModel *upload,
                                    SettingsRepository *settings,
                                    QObject *parent = nullptr);

    QVariantList stagedFiles() const { return m_files; }
    bool hasStaged() const { return !m_files.isEmpty(); }

    QString targetFolder() const { return m_folder; }
    void setTargetFolder(const QString &f);

    QString password() const { return m_password; }
    void setPassword(const QString &p);

    bool secured() const { return m_secured; }
    void setSecured(bool s);

    bool directLink() const { return m_directLink; }
    void setDirectLink(bool d);

    bool restoredFromDisk() const { return m_restoredFromDisk; }
    bool showRequested() const { return m_showRequested; }

    // Append files (URL "file:///..." OR raw path). Dedupes by canonical path so
    // dragging the same file twice via different forms collapses cleanly.
    // Each entry stores {path, name, ext, size, valid} — valid is re-evaluated
    // here so an immediate add of a missing file is flagged the same way as a
    // hydrated-then-deleted one.
    Q_INVOKABLE void addFiles(const QStringList &fileUrlsOrPaths);

    // Remove the entry at the given index. Out-of-range is a no-op.
    Q_INVOKABLE void removeFile(int index);

    // Clear all staging (files + folder/password/secured/directLink + restore
    // flag). Persists immediately so a crash before the debounced save can't
    // resurrect a batch the user just discarded.
    Q_INVOKABLE void clear();

    // Clear ONLY the files list, leaving folder/password/secured/directLink in
    // place. Powers the dialog's "Xoá tất cả" button — the user expects to drop
    // the file selection but keep their destination + privacy settings.
    Q_INVOKABLE void clearFiles();

    // Hand the staged batch to UploadViewModel.addUpload(...) and clear. Invalid
    // entries (file gone between sessions) are skipped — caller must have
    // surfaced those visually before commit.
    Q_INVOKABLE void commit();

    // Called by UploadPage when the user clicks "Tiếp tục" on the post-restart
    // banner. Marks the staging as accepted by the user so subsequent emits
    // behave like a normal in-session staging (auto-open dialog).
    Q_INVOKABLE void acknowledgeRestored();

    // Called by UploadPage once it has acted on a showRequested == true state
    // (opened the dialog). Clears the flag so subsequent sidebar visits don't
    // re-trigger an auto-open of the dialog.
    Q_INVOKABLE void acknowledgeShow();

signals:
    void stagedFilesChanged();
    void targetFolderChanged();
    void passwordChanged();
    void securedChanged();
    void directLinkChanged();
    void restoredFromDiskChanged();
    void showRequestedChanged();

private:
    // Re-stat every entry's path and update valid + size. Cheap (files are
    // usually local). Called from hydrate; could be called from a tray-resume
    // hook in the future.
    void revalidate();
    // Debounced (200 ms) wrapper around persistNow — protects QSettings I/O
    // when the user rapid-fires drops or types into the password field.
    void schedulePersist();
    void persistNow() const;
    // Load JSON-encoded staging from SettingsRepository at construction.
    void hydrate();

    UploadViewModel    *m_upload   = nullptr;
    SettingsRepository *m_settings = nullptr;
    QVariantList        m_files;
    QString             m_folder   = QStringLiteral("/");
    QString             m_password;
    bool                m_secured     = false;
    bool                m_directLink  = false;
    bool                m_restoredFromDisk = false;
    bool                m_showRequested    = false;
    QTimer              m_persistTimer;
};

} // namespace fsnext
