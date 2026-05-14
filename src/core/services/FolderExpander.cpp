#include "FolderExpander.h"
#include "core/api/FshareApi.h"
#include "core/util/FileNameSanitizer.h"
#include "core/util/FshareUrl.h"
#include <QDir>
#include <QPointer>
#include <QRegularExpression>
#include <QUuid>
#include <QtConcurrent>
#include <QDebug>

namespace fsnext {

static const int     kPageSize    = 50;
static const QString kFolderBase  = QStringLiteral("https://www.fshare.vn/folder/");
static const QString kFileBase    = QStringLiteral("https://www.fshare.vn/file/");

FolderExpander::FolderExpander(FshareApi  *api,
                               const QString &rootUrl,
                               const QString &savePath,
                               const QString &password,
                               int            segmentsPerFile,
                               int            maxDepth,
                               QObject       *parent)
    : QObject(parent)
    , m_api(api)
    , m_rootUrl(rootUrl)
    , m_savePath(savePath)
    , m_password(password)
    , m_segmentsPerFile(segmentsPerFile)
    , m_maxDepth(maxDepth)
    , m_groupId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

void FolderExpander::start()
{
    QPointer<FolderExpander> guard(this);
    QtConcurrent::run([guard]() {
        if (!guard) return;
        guard->run();
    });
}

void FolderExpander::abort()
{
    m_abort.store(true);
}

// ---------------------------------------------------------------------------
// run() — executes on the QtConcurrent thread pool.
// ---------------------------------------------------------------------------
void FolderExpander::run()
{
    // ── Step 1: Resolve root folder name ────────────────────────────────────
    auto infoResp = m_api->getFileInfo(m_rootUrl);
    if (m_abort.load()) {
        emit finished();
        return;
    }
    if (infoResp.isError()) {
        emit failed(infoResp.error().message);
        emit finished();
        return;
    }

    m_rootFolderName = sanitizeName(infoResp.data().name);
    if (m_rootFolderName.isEmpty())
        m_rootFolderName = QStringLiteral("download");

    // ── Step 2: Recursive crawl ──────────────────────────────────────────────
    if (!crawl(m_rootUrl, QString{}, 0)) {
        if (!m_abort.load())
            emit failed(m_lastError.isEmpty()
                        ? QStringLiteral("Lỗi không xác định khi quét folder")
                        : m_lastError);
        emit finished();
        return;
    }

    // ── Step 3: Done ─────────────────────────────────────────────────────────
    if (!m_abort.load()) {
        emit scanProgress(m_foldersScanned, m_tasks.size());
        emit completed(m_tasks);
    }
    emit finished();
}

// ---------------------------------------------------------------------------
// crawl() — paginate one folder level, recurse into sub-folders.
// Returns true on success (or when abort is requested — caller checks abort).
// Returns false on API error (sets m_lastError).
// ---------------------------------------------------------------------------
bool FolderExpander::crawl(const QString &folderUrl, const QString &relPath, int depth)
{
    if (m_abort.load()) return true; // not an error — just stop

    if (depth >= m_maxDepth) {
        qWarning() << "[FolderExpander] Max depth" << m_maxDepth
                   << "reached, skipping deeper levels at:" << folderUrl;
        return true; // skip, not an error
    }

    int page = 0;                        // Fshare API uses 0-indexed pages
    while (true) {
        if (m_abort.load()) return true;

        auto resp = m_api->listFiles(folderUrl, page, kPageSize);
        if (resp.isError()) {
            m_lastError = resp.error().message;
            qWarning() << "[FolderExpander] listFiles error:" << m_lastError
                       << "url:" << folderUrl << "page:" << page;
            return false;
        }

        const QVector<FileItem> items = resp.takeData();
        for (const FileItem &item : items) {
            if (m_abort.load()) return true;

            if (item.isFile()) {
                m_tasks.append(makeTask(item, relPath));
            } else if (item.isFolder() && !item.linkcode.isEmpty()) {
                // Propagate the share-access `?token=` (if any) from the root
                // share URL to sub-folder listings — the API requires the same
                // token to authorize crawling any level of the share tree.
                QString subUrl = kFolderBase + item.linkcode;
                const QString rootToken = FshareUrl::canonicalUrl(m_rootUrl)
                                              .section(QLatin1Char('?'), 1, 1);
                if (!rootToken.isEmpty())
                    subUrl += QLatin1Char('?') + rootToken;

                const QString safeSub    = sanitizeName(item.name);
                const QString subRelPath = relPath + QLatin1Char('/') + safeSub;
                if (!crawl(subUrl, subRelPath, depth + 1))
                    return false;
            }
        }

        emit scanProgress(++m_foldersScanned, m_tasks.size());

        if (items.size() < kPageSize) break; // last page
        ++page;
    }

    return true;
}

// ---------------------------------------------------------------------------
// makeTask() — build a TransferTask for a file found in the folder tree.
//
// task.localPath is set to the TARGET DIRECTORY (not the full file path).
// TransferService::startNextInQueue() appends the real filename obtained from
// the download session URL — same as for regular single-file downloads.
// ---------------------------------------------------------------------------
TransferTask FolderExpander::makeTask(const FileItem &item, const QString &relPath) const
{
    const QString safeName = sanitizeName(item.name);

    // Build target directory:
    //   savePath / rootFolderName [/ relPath]
    // relPath starts with "/" when non-empty (e.g. "/Photos/2024")
    QString dir = m_savePath;
    if (!dir.endsWith(QLatin1Char('/')) && !dir.endsWith(QLatin1Char('\\')))
        dir += QLatin1Char('/');
    dir += m_rootFolderName;
    if (!relPath.isEmpty())
        dir += relPath; // relPath already starts with '/'

    // Display path shown in UI: "rootFolderName[/subpath]"
    // relPath = "/Photos/2024" → strip leading slash → "Photos/2024"
    QString displayFolderPath = m_rootFolderName;
    if (!relPath.isEmpty()) {
        const QString subPart = relPath.startsWith(QLatin1Char('/'))
                                ? relPath.mid(1) : relPath;
        if (!subPart.isEmpty())
            displayFolderPath += QLatin1Char('/') + subPart;
    }

    // Propagate the share-access `?token=` from the root share URL so
    // createDownloadSession succeeds for token-gated file entries.
    QString fileUrl = kFileBase + item.linkcode;
    const QString rootToken = FshareUrl::canonicalUrl(m_rootUrl)
                                  .section(QLatin1Char('?'), 1, 1);
    if (!rootToken.isEmpty())
        fileUrl += QLatin1Char('?') + rootToken;

    TransferTask task;
    task.id         = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.type       = TransferType::Download;
    task.state      = TransferState::Queued;
    task.fileName   = safeName.isEmpty() ? QStringLiteral("file") : safeName;
    task.fileSize   = item.size;
    task.linkcode   = fileUrl;  // full URL for createDownloadSession
    task.password   = m_password;
    task.localPath  = dir;   // directory only; filename appended by startNextInQueue
    task.segments   = m_segmentsPerFile;
    task.groupId    = m_groupId;
    task.folderPath = displayFolderPath;
    return task;
}

// ---------------------------------------------------------------------------
// sanitizeName() — reuse the same rules as file downloads so sub-folder
// names can't produce broken paths on Windows (DOS devices, trailing dot /
// space, >200 chars, embedded NUL, etc.).
// ---------------------------------------------------------------------------
QString FolderExpander::sanitizeName(const QString &name)
{
    return FileNameSanitizer::sanitize(name);
}

} // namespace fsnext
