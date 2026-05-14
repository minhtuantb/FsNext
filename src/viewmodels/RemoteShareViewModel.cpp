// SPDX-License-Identifier: Proprietary
#include "RemoteShareViewModel.h"

#include "core/api/FshareApi.h"
#include "core/util/FormatUtil.h"
#include "core/util/FshareUrl.h"
#include "platform/PlatformUtils.h"
#include "viewmodels/DownloadViewModel.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPointer>
#include <QUrl>
#include <QtConcurrent>

namespace fsnext {

namespace {

constexpr const char *kFolderBase = "https://www.fshare.vn/folder/";
constexpr const char *kFileBase   = "https://www.fshare.vn/file/";

// Compute a coarse file-category bucket for the QML detail sheet — mirrors
// HomePage's _categoryOf so the user gets the same "video/audio/document/
// image/other" classification everywhere. Folders fall through to "folder".
QString categoryOf(const FileItem &it)
{
    if (it.isFolder()) return QStringLiteral("folder");
    const QString ext = QFileInfo(it.name).suffix().toLower();
    static const QSet<QString> kVideo {
        "mp4","mkv","avi","mov","wmv","flv","webm","m4v","3gp","ts","mpg","mpeg"
    };
    static const QSet<QString> kAudio {
        "mp3","wav","flac","m4a","aac","ogg","wma","opus"
    };
    static const QSet<QString> kImage {
        "jpg","jpeg","png","gif","bmp","svg","webp","tif","tiff","heic","ico"
    };
    static const QSet<QString> kDoc {
        "pdf","doc","docx","xls","xlsx","ppt","pptx","txt","rtf","odt","csv"
    };
    if (kVideo.contains(ext)) return QStringLiteral("video");
    if (kAudio.contains(ext)) return QStringLiteral("audio");
    if (kImage.contains(ext)) return QStringLiteral("image");
    if (kDoc.contains(ext))   return QStringLiteral("document");
    return QStringLiteral("other");
}

} // namespace

RemoteShareViewModel::RemoteShareViewModel(FshareApi         *api,
                                            DownloadViewModel *downloadVm,
                                            QObject           *parent)
    : QObject(parent)
    , m_api(api)
    , m_downloadVm(downloadVm)
    , m_fileListModel(new FileListModel(this))
{
}

// ─── Property setters ──────────────────────────────────────────────────────

void RemoteShareViewModel::setLoading(bool v)
{
    if (m_loading == v) return;
    m_loading = v;
    emit isLoadingChanged();
}

void RemoteShareViewModel::setMode(Mode m)
{
    if (m_mode == m) return;
    m_mode = m;
    emit modeChanged();
}

void RemoteShareViewModel::setError(const QString &msg)
{
    if (m_errorText == msg) return;
    m_errorText = msg;
    emit errorTextChanged();
    if (!msg.isEmpty())
        emit operationMessage(msg, /*isError=*/true);
}

QVariantList RemoteShareViewModel::breadcrumbs() const
{
    QVariantList out;
    out.reserve(m_folderStack.size());
    for (const auto &e : m_folderStack) {
        QVariantMap m;
        m[QStringLiteral("linkcode")] = e.linkcode;
        m[QStringLiteral("name")]     = e.name;
        out.append(m);
    }
    return out;
}

// ─── URL helpers ───────────────────────────────────────────────────────────

QString RemoteShareViewModel::tokenizedFolderUrl(const QString &linkcode) const
{
    QString u = QString::fromLatin1(kFolderBase) + linkcode;
    if (!m_rootToken.isEmpty()) u += QLatin1Char('?') + m_rootToken;
    return u;
}

QString RemoteShareViewModel::tokenizedFileUrl(const QString &linkcode) const
{
    QString u = QString::fromLatin1(kFileBase) + linkcode;
    if (!m_rootToken.isEmpty()) u += QLatin1Char('?') + m_rootToken;
    return u;
}

// ─── Entry points ──────────────────────────────────────────────────────────

void RemoteShareViewModel::close()
{
    setMode(ModeNone);
    m_fileListModel->resetItems({});
    m_folderStack.clear();
    m_rootToken.clear();
    m_currentPage = 0;
    m_totalCount  = 0;
    m_hasMore     = false;
    m_currentFile.clear();
    m_currentFileUrl.clear();
    m_errorText.clear();
    setLoading(false);
    emit folderChanged();
    emit fileChanged();
    emit errorTextChanged();
}

void RemoteShareViewModel::openFolder(const QString &rawUrl)
{
    const QString canonical = FshareUrl::canonicalUrl(rawUrl);
    const auto parsed = FshareUrl::parse(canonical);
    if (parsed.kind != FshareUrl::Kind::Folder || parsed.linkcode.isEmpty()) {
        setError(tr("Đường dẫn folder không hợp lệ"));
        return;
    }

    // Reset, then capture the share-access token (if any) — every child
    // folder / file URL we synthesise during navigation must echo it back
    // to the API to stay authorised.
    close();
    setMode(ModeFolder);
    m_rootToken = canonical.section(QLatin1Char('?'), 1, 1);

    m_folderStack.append({ parsed.linkcode, parsed.linkcode, canonical });
    emit folderChanged();

    // Resolve the root folder's display name in parallel with the first
    // page listing — the name only affects the breadcrumb label, not the
    // listing itself, so we don't block the list on it.
    QPointer<RemoteShareViewModel> guard(this);
    setLoading(true);
    QtConcurrent::run([this, guard, canonical]() {
        if (!guard) return;
        auto info = m_api->getFileInfo(canonical);
        QMetaObject::invokeMethod(guard.data(), [guard, info]() {
            if (!guard || guard->m_folderStack.isEmpty()) return;
            if (info.isSuccess()) {
                const QString name = info.data().name;
                if (!name.isEmpty()) {
                    guard->m_folderStack.first().name = name;
                    emit guard->folderChanged();
                }
            }
            // getFileInfo failure isn't fatal for folder mode — the listing
            // still works and the breadcrumb just shows the linkcode.
        });
    });

    fetchFolderPage(0);
}

// Used when the user is browsing a folder share and taps a file row —
// we keep the same ?token= that authorised the parent listing so the
// downstream getFileInfo / createDownloadSession calls don't 401.
void RemoteShareViewModel::openFileFromCurrentFolder(const QString &linkcode)
{
    if (linkcode.isEmpty()) return;
    const QString token = m_rootToken;     // preserve before close() clears it
    const QString fileUrl = tokenizedFileUrl(linkcode);
    openFile(fileUrl);
    // openFile() reset m_rootToken to whatever it parsed from the URL we just
    // synthesised — which is the same `token`, so we're good. (Guard kept
    // for clarity in case the synthesis logic changes.)
    if (m_rootToken.isEmpty() && !token.isEmpty()) m_rootToken = token;
}

void RemoteShareViewModel::openFile(const QString &rawUrl)
{
    const QString canonical = FshareUrl::canonicalUrl(rawUrl);
    const auto parsed = FshareUrl::parse(canonical);
    if (parsed.kind != FshareUrl::Kind::File || parsed.linkcode.isEmpty()) {
        setError(tr("Đường dẫn file không hợp lệ"));
        return;
    }

    close();
    setMode(ModeFile);
    m_rootToken      = canonical.section(QLatin1Char('?'), 1, 1);
    m_currentFileUrl = canonical;

    QPointer<RemoteShareViewModel> guard(this);
    setLoading(true);
    QtConcurrent::run([this, guard, canonical]() {
        if (!guard) return;
        auto resp = m_api->getFileInfo(canonical);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            guard->setLoading(false);
            if (resp.isError()) {
                guard->setError(resp.error().message.isEmpty()
                                    ? tr("Không lấy được thông tin file")
                                    : resp.error().message);
                guard->m_currentFile.clear();
                emit guard->fileChanged();
                return;
            }
            guard->m_currentFile = fileItemToMap(resp.data());
            emit guard->fileChanged();
        });
    });
}

// ─── Folder navigation ─────────────────────────────────────────────────────

void RemoteShareViewModel::navigateInto(const QString &linkcode, const QString &name)
{
    if (m_mode != ModeFolder || linkcode.isEmpty()) return;
    m_folderStack.append({ linkcode, name.isEmpty() ? linkcode : name,
                            tokenizedFolderUrl(linkcode) });
    m_fileListModel->resetItems({});
    m_currentPage = 0;
    m_totalCount  = 0;
    m_hasMore     = false;
    emit folderChanged();
    fetchFolderPage(0);
}

void RemoteShareViewModel::navigateBack()
{
    if (m_folderStack.size() <= 1) return;
    m_folderStack.removeLast();
    m_fileListModel->resetItems({});
    m_currentPage = 0;
    m_totalCount  = 0;
    m_hasMore     = false;
    emit folderChanged();
    fetchFolderPage(0);
}

void RemoteShareViewModel::loadMore()
{
    if (!m_hasMore || m_loading) return;
    fetchFolderPage(m_currentPage + 1);
}

void RemoteShareViewModel::fetchFolderPage(int page)
{
    if (m_folderStack.isEmpty()) return;
    const QString folderUrl = m_folderStack.last().url;

    QPointer<RemoteShareViewModel> guard(this);
    setLoading(true);
    QtConcurrent::run([this, guard, folderUrl, page]() {
        if (!guard) return;
        auto resp = m_api->listFiles(folderUrl, page, kPageSize);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, page]() {
            if (!guard) return;
            guard->setLoading(false);
            if (resp.isError()) {
                guard->setError(resp.error().message.isEmpty()
                                    ? tr("Không tải được danh sách thư mục")
                                    : resp.error().message);
                return;
            }
            const QVector<FileItem> items = resp.data();
            if (page == 0)
                guard->m_fileListModel->resetItems(items);
            else
                guard->m_fileListModel->mergeItems(items);

            guard->m_currentPage = page;
            guard->m_totalCount  = guard->m_fileListModel->count();
            guard->m_hasMore     = items.size() >= kPageSize;
            emit guard->folderChanged();
        });
    });
}

// ─── File-mode actions ─────────────────────────────────────────────────────

void RemoteShareViewModel::downloadCurrentFile(const QString &password)
{
    if (m_mode != ModeFile || m_currentFileUrl.isEmpty() || !m_downloadVm) return;
    const QString folder = m_downloadVm->defaultSaveFolder();
    m_downloadVm->addDownload(m_currentFileUrl, folder, password);
    const QString name = m_currentFile.value(QStringLiteral("name")).toString();
    emit downloadQueued(name);
    emit operationMessage(tr("Đã thêm vào danh sách tải"), false);
}

// playCurrentFile — same chain as FavoritesViewModel::getStreamLink /
// playStreamUrl, but inlined because RemoteShareVM doesn't share the
// FileCacheService dependency that wraps getDownloadUrl().
void RemoteShareViewModel::playCurrentFile(const QString &password)
{
    if (m_mode != ModeFile || m_currentFileUrl.isEmpty()) return;
    const QString name = m_currentFile.value(QStringLiteral("name")).toString();

    QPointer<RemoteShareViewModel> guard(this);
    setLoading(true);
    const QString url = m_currentFileUrl;
    QtConcurrent::run([this, guard, url, password, name]() {
        if (!guard) return;
        auto resp = m_api->createDownloadSession(url, password);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, name]() {
            if (!guard) return;
            guard->setLoading(false);
            if (resp.isError()) {
                guard->setError(resp.error().message.isEmpty()
                                    ? tr("Không tạo được phiên phát trực tiếp")
                                    : resp.error().message);
                return;
            }
            const bool ok = PlatformUtils::playStreamUrl(resp.data(), name);
            if (!ok) {
                guard->setError(tr("Không mở được trình phát mặc định"));
            } else {
                emit guard->operationMessage(QObject::tr("Đang mở trình phát…"), false);
            }
        });
    });
}

void RemoteShareViewModel::copyCurrentFileLink()
{
    if (m_currentFileUrl.isEmpty()) return;
    if (QClipboard *cb = QGuiApplication::clipboard()) {
        cb->setText(m_currentFileUrl);
        emit linkCopied();
        emit operationMessage(tr("Đã sao chép link"), false);
    }
}

void RemoteShareViewModel::openCurrentFileOnFshare()
{
    if (m_currentFileUrl.isEmpty()) return;
    QDesktopServices::openUrl(QUrl(m_currentFileUrl));
}

// ─── Folder-row actions ────────────────────────────────────────────────────

void RemoteShareViewModel::downloadFolderItem(const QString &linkcode, bool isFolder,
                                                const QString &password)
{
    if (linkcode.isEmpty() || !m_downloadVm) return;
    const QString url = isFolder ? tokenizedFolderUrl(linkcode)
                                  : tokenizedFileUrl(linkcode);
    const QString folder = m_downloadVm->defaultSaveFolder();
    m_downloadVm->addDownload(url, folder, password);
    emit operationMessage(tr("Đã thêm vào danh sách tải"), false);
}

void RemoteShareViewModel::copyFolderItemLink(const QString &linkcode, bool isFolder)
{
    if (linkcode.isEmpty()) return;
    const QString url = isFolder ? tokenizedFolderUrl(linkcode)
                                  : tokenizedFileUrl(linkcode);
    if (QClipboard *cb = QGuiApplication::clipboard()) {
        cb->setText(url);
        emit linkCopied();
        emit operationMessage(tr("Đã sao chép link"), false);
    }
}

// ─── Static helpers ────────────────────────────────────────────────────────

QVariantMap RemoteShareViewModel::fileItemToMap(const FileItem &it)
{
    QVariantMap m;
    m[QStringLiteral("linkcode")]      = it.linkcode;
    m[QStringLiteral("name")]          = it.name;
    m[QStringLiteral("size")]          = static_cast<qlonglong>(it.size);
    m[QStringLiteral("isFolder")]      = it.isFolder();
    m[QStringLiteral("hasPassword")]   = it.hasPassword;
    m[QStringLiteral("downloadCount")] = it.downloadCount;
    m[QStringLiteral("created")]       = it.created;
    m[QStringLiteral("description")]   = it.description;
    m[QStringLiteral("fileCategory")]  = categoryOf(it);
    return m;
}

} // namespace fsnext
