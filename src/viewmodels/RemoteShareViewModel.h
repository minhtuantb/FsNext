// SPDX-License-Identifier: Proprietary
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "core/models/FileItem.h"
#include "viewmodels/FileListModel.h"

namespace fsnext {

class FshareApi;
class DownloadViewModel;

// ViewModel powering the "open a share URL from the homepage search" surface.
// Two presentation modes:
//
//   Folder mode — user pasted /folder/<code>?token=…
//                 The dialog shows a paginated, multi-level browser.
//                 Folder URLs never require a password in the Fshare API
//                 (per product spec — share-folder password protection is
//                 not used). Files inside the folder can still be
//                 password-protected and are downloaded / played using
//                 the same password the user enters once for that file.
//
//   File mode   — user pasted /file/<code>?token=…
//                 The dialog turns into a single-file detail sheet with
//                 Play (video/audio only) + Download + Copy-link buttons.
//
// All Fshare API calls are issued on QtConcurrent's pool — exactly the same
// pattern as FileCacheService / FavoritesViewModel — and re-marshalled
// back to the GUI thread with QMetaObject::invokeMethod so the
// FileListModel updates happen on the UI thread.
class RemoteShareViewModel : public QObject {
    Q_OBJECT

    // ── Common ────────────────────────────────────────────────
    Q_PROPERTY(FileListModel *fileListModel READ fileListModel CONSTANT)
    Q_PROPERTY(int  mode          READ mode          NOTIFY modeChanged)        // see Mode enum
    Q_PROPERTY(bool isLoading     READ isLoading     NOTIFY isLoadingChanged)
    Q_PROPERTY(QString errorText  READ errorText     NOTIFY errorTextChanged)

    // ── Folder mode ───────────────────────────────────────────
    Q_PROPERTY(QString  currentFolderName READ currentFolderName NOTIFY folderChanged)
    Q_PROPERTY(QString  currentFolderUrl  READ currentFolderUrl  NOTIFY folderChanged)
    Q_PROPERTY(QVariantList breadcrumbs   READ breadcrumbs       NOTIFY folderChanged)
    Q_PROPERTY(bool     canGoBack         READ canGoBack         NOTIFY folderChanged)
    Q_PROPERTY(bool     hasMorePages      READ hasMorePages      NOTIFY folderChanged)
    Q_PROPERTY(int      totalCount        READ totalCount        NOTIFY folderChanged)

    // ── File mode ─────────────────────────────────────────────
    Q_PROPERTY(QVariantMap currentFile    READ currentFile       NOTIFY fileChanged)

public:
    enum Mode {
        ModeNone   = 0,
        ModeFolder = 1,
        ModeFile   = 2,
    };
    Q_ENUM(Mode)

    explicit RemoteShareViewModel(FshareApi         *api,
                                   DownloadViewModel *downloadVm,
                                   QObject           *parent = nullptr);

    // ── Property getters ─────────────────────────────────────
    FileListModel *fileListModel() const { return m_fileListModel; }
    int  mode()       const { return m_mode; }
    bool isLoading()  const { return m_loading; }
    QString errorText() const { return m_errorText; }

    QString currentFolderName() const { return m_folderStack.isEmpty() ? QString{} : m_folderStack.last().name; }
    QString currentFolderUrl()  const { return m_folderStack.isEmpty() ? QString{} : m_folderStack.last().url; }
    QVariantList breadcrumbs() const;
    bool canGoBack()    const { return m_folderStack.size() > 1; }
    bool hasMorePages() const { return m_hasMore; }
    int  totalCount()   const { return m_totalCount; }

    QVariantMap currentFile() const { return m_currentFile; }

    // ── Entry points (called from QML) ───────────────────────
    Q_INVOKABLE void openFolder(const QString &url);
    Q_INVOKABLE void openFile(const QString &url);

    // Switch from folder-browse to file-detail for one of the rows the
    // user just clicked. The current ?token= is preserved on the
    // synthesised file URL so password-less share folders that gate
    // access via token keep working without the user re-pasting the URL.
    Q_INVOKABLE void openFileFromCurrentFolder(const QString &linkcode);

    // Reset state — call when the host dialog closes.
    Q_INVOKABLE void close();

    // ── Folder navigation ────────────────────────────────────
    Q_INVOKABLE void navigateInto(const QString &linkcode, const QString &name);
    Q_INVOKABLE void navigateBack();
    Q_INVOKABLE void loadMore();

    // ── Actions on the currently displayed item / row ────────
    // password is optional — required only for password-protected files;
    // QML passes "" when the user hasn't entered one.
    Q_INVOKABLE void downloadCurrentFile(const QString &password);
    Q_INVOKABLE void playCurrentFile    (const QString &password);
    Q_INVOKABLE void copyCurrentFileLink();
    Q_INVOKABLE void openCurrentFileOnFshare();

    // Folder-row actions — issued from inside the folder browser.
    Q_INVOKABLE void downloadFolderItem(const QString &linkcode, bool isFolder,
                                         const QString &password = QString{});
    Q_INVOKABLE void copyFolderItemLink(const QString &linkcode, bool isFolder);

signals:
    void modeChanged();
    void isLoadingChanged();
    void errorTextChanged();
    void folderChanged();
    void fileChanged();

    // Surfaced to the dialog for toasts / status text.
    void operationMessage(const QString &msg, bool isError);
    void downloadQueued(const QString &fileName);
    void linkCopied();

private:
    struct FolderEntry {
        QString linkcode;   // bare ID — used as breadcrumb key
        QString name;       // display label
        QString url;        // canonical URL with ?token preserved
    };

    void setLoading(bool v);
    void setMode(Mode m);
    void setError(const QString &msg);

    // Build a sub-folder URL that inherits the share-access ?token= of the
    // root entry, mirroring what FolderExpander does for downloads.
    QString tokenizedFolderUrl(const QString &linkcode) const;
    QString tokenizedFileUrl  (const QString &linkcode) const;

    void fetchFolderPage(int page);     // async — appends rows on success
    static QVariantMap fileItemToMap(const FileItem &it);

    FshareApi         *m_api        = nullptr;
    DownloadViewModel *m_downloadVm = nullptr;
    FileListModel     *m_fileListModel = nullptr;

    Mode    m_mode    = ModeNone;
    bool    m_loading = false;
    QString m_errorText;

    // Folder mode state. The root token is stored once (extracted from the
    // URL the user pasted) and reused for every child folder/file URL we
    // build during navigation.
    QVector<FolderEntry> m_folderStack;
    QString m_rootToken;        // already includes "token=…" — empty when none
    int     m_currentPage = 0;
    int     m_totalCount  = 0;
    bool    m_hasMore     = false;
    static constexpr int kPageSize = 50;

    // File mode state — captured from getFileInfo so the QML can render
    // the detail sheet without having to model FileItem itself.
    QVariantMap m_currentFile;
    QString     m_currentFileUrl;       // canonical URL with ?token=
};

} // namespace fsnext
