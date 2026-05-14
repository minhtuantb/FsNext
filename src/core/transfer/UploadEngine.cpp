#include "UploadEngine.h"
#include <curl/curl.h>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <QThread>
#include <json/json.h>
#include <chrono>
#include <cstring>

namespace fsnext {

// Chunk size constants — see ADR 003 D11.
// Retry budget: 5 attempts with 1/2/4/8/16-s exponential backoff before
// halving the chunk size and resuming from the current offset.  Auth/quota
// errors (FshareApi reports them via HTTP 401/507 mapped to specific strings)
// propagate immediately and are NOT counted against the retry budget.
static constexpr int64_t DEFAULT_UPLOAD_CHUNK = 20LL * 1024 * 1024;   // 20MB initial chunk
static constexpr int64_t MIN_UPLOAD_CHUNK     =  5LL * 1024 * 1024;   //  5MB floor (D11)
static constexpr int64_t MAX_UPLOAD_CHUNK     = 1024LL * 1024 * 1024; // 1GB max chunk
static constexpr int     MAX_CHUNK_RETRIES    = 5;                     // per chunk
static constexpr double  TARGET_CHUNK_SECS    = 60.0;                  // aim for 60s per chunk

#ifdef _WIN32
#define FSEEKO64 _fseeki64
#else
#define FSEEKO64 fseeko
#endif

// Unicode-safe file open. Plain fopen() uses the ANSI codepage on Windows and
// fails for paths containing Vietnamese / non-Latin-1 chars. See sibling helper
// in DownloadEngine.cpp.
static FILE *openFileUnicode(const QString &path, const char *mode)
{
#ifdef _WIN32
    const std::wstring wp = path.toStdWString();
    const std::wstring wmode(mode, mode + std::strlen(mode));
    return _wfopen(wp.c_str(), wmode.c_str());
#else
    return std::fopen(path.toUtf8().constData(), mode);
#endif
}

UploadEngine::UploadEngine(QObject *parent)
    : QObject(parent)
{
}

UploadEngine::~UploadEngine() = default;

// ── Chunk read callback ──────────────────────────────────

size_t UploadEngine::chunkReadCallback(char *buffer, size_t size, size_t nmemb, void *userdata)
{
    auto *ctx = static_cast<ChunkContext *>(userdata);
    if (!ctx || !ctx->engine || ctx->engine->m_abort.load())
        return CURL_READFUNC_ABORT;

    while (ctx->engine->m_paused.load() && !ctx->engine->m_abort.load())
        QThread::msleep(100);

    int64_t maxBytes = static_cast<int64_t>(size) * static_cast<int64_t>(nmemb);

    // Throttle: for sync-marked tasks with speedLimitBps>0, cap how many bytes
    // we hand to libcurl each callback and sleep between calls so the average
    // write rate stays at or below the limit. Token-bucket with window-reset
    // every 200 ms — small enough to feel responsive, large enough to amortize
    // the syscall / context-switch overhead of short sleeps.
    const int64_t cap = ctx->engine->m_task.speedLimitBps;
    if (ctx->engine->m_task.isSyncTask && cap > 0) {
        constexpr int kWindowMs = 200;
        const int64_t perWindow = (cap * kWindowMs) / 1000;  // bytes allowed per window

        using clock = std::chrono::steady_clock;
        const auto now = clock::now();
        if (ctx->windowStart.time_since_epoch().count() == 0) {
            ctx->windowStart = now;
            ctx->bytesThisWindow = 0;
        }
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - ctx->windowStart).count();
        if (elapsedMs >= kWindowMs) {
            ctx->windowStart = now;
            ctx->bytesThisWindow = 0;
        } else if (ctx->bytesThisWindow >= perWindow) {
            const int sleepMs = static_cast<int>(kWindowMs - elapsedMs);
            if (sleepMs > 0) QThread::msleep(sleepMs);
            ctx->windowStart = clock::now();
            ctx->bytesThisWindow = 0;
        }

        const int64_t remainingInWindow = perWindow - ctx->bytesThisWindow;
        if (remainingInWindow > 0 && remainingInWindow < maxBytes)
            maxBytes = remainingInWindow;
    }

    const int64_t toRead = qMin(maxBytes, ctx->bytesRemaining);
    if (toRead <= 0) return 0;

    const size_t actuallyRead = fread(buffer, 1, static_cast<size_t>(toRead), ctx->file);
    ctx->bytesRemaining -= static_cast<int64_t>(actuallyRead);
    if (ctx->engine->m_task.isSyncTask && cap > 0)
        ctx->bytesThisWindow += static_cast<int64_t>(actuallyRead);
    return actuallyRead;
}

int UploadEngine::chunkProgressCallback(void *clientp, int64_t /*dltotal*/, int64_t /*dlnow*/,
                                         int64_t /*ultotal*/, int64_t ulnow)
{
    auto *ctx = static_cast<ChunkContext *>(clientp);
    if (!ctx || !ctx->engine) return 0;
    if (ctx->engine->m_abort.load()) return 1;

    // total = (already-completed chunks) + (current chunk progress)
    const int64_t totalBytes = ctx->engine->m_task.fileSize;
    const int64_t baseUploaded = ctx->engine->m_totalUploaded.load();
    const int64_t absUploaded = baseUploaded + ulnow;

    ctx->engine->m_meter.markProgress(absUploaded);
    emit ctx->engine->progressChanged(absUploaded, totalBytes,
                                       ctx->engine->m_meter.speed(),
                                       ctx->engine->m_meter.eta());
    return 0;
}

// ── Entry point ──────────────────────────────────────────

void UploadEngine::startUpload(const TransferTask &task)
{
    m_task = task;
    m_abort.store(false);
    m_paused.store(false);
    m_totalUploaded.store(0);

    QFileInfo fi(task.sourcePath);
    if (!fi.exists()) {
        emit failed(QStringLiteral("Source file not found: ") + task.sourcePath);
        return;
    }
    if (task.realUrl.isEmpty()) {
        emit failed(QStringLiteral("No upload URL"));
        return;
    }

    const int64_t fileSize = fi.size();
    m_meter.start(fileSize);

    FILE *srcFile = openFileUnicode(task.sourcePath, "rb");
    if (!srcFile) {
        emit failed(QStringLiteral("Không thể mở file nguồn: %1").arg(task.sourcePath));
        return;
    }

    // --- P2-2: Query the server for how many bytes are already received.
    // This allows resuming mid-upload after a pause or crash without re-sending
    // bytes the server confirmed. Falls back to 0 if the query fails.
    const int64_t resumeOffset = queryResumeOffset(task.realUrl, fileSize);
    int64_t fromTarget = resumeOffset;
    if (fromTarget > 0) {
        qDebug() << "[UploadEngine] Resuming from byte" << fromTarget;
        m_totalUploaded.store(fromTarget);
    }

    QByteArray urlBytes = task.realUrl.toUtf8();
    QString lastResponse;

    // --- P3-1: Dynamic chunk size — starts at DEFAULT_UPLOAD_CHUNK and adjusts
    // after each successful chunk to aim for TARGET_CHUNK_SECS per chunk.
    int64_t currentChunkSize = DEFAULT_UPLOAD_CHUNK;

    qDebug() << "[UploadEngine] Starting chunked upload:" << task.fileName
             << "(" << fileSize << "bytes, initial chunk=" << currentChunkSize << ")";

    while (fromTarget < fileSize && !m_abort.load()) {
        // Calculate chunk range
        int64_t toTarget = fromTarget + currentChunkSize;
        if (toTarget > fileSize) toTarget = fileSize;
        const int64_t chunkLen = toTarget - fromTarget;

        // Seek to chunk start
        if (FSEEKO64(srcFile, fromTarget, SEEK_SET) != 0) {
            fclose(srcFile);
            emit failed(QStringLiteral("Failed to seek source file"));
            return;
        }

        // Build Content-Range header: bytes <from>-<to-1>/<total>
        QString rangeHeader = QStringLiteral("Content-Range: bytes %1-%2/%3")
            .arg(fromTarget).arg(toTarget - 1).arg(fileSize);

        // Per-chunk retry loop
        bool chunkOk = false;
        QString chunkError;
        const auto chunkStartTime = std::chrono::steady_clock::now();

        for (int retry = 0; retry < MAX_CHUNK_RETRIES && !m_abort.load(); ++retry) {
            CURL *curl = curl_easy_init();
            if (!curl) { chunkError = QStringLiteral("CURL init failed"); break; }

            ChunkContext ctx;
            ctx.engine = this;
            ctx.file = srcFile;
            ctx.bytesRemaining = chunkLen;

            QByteArray responseBody;

            curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, chunkReadCallback);
            curl_easy_setopt(curl, CURLOPT_READDATA, &ctx);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(chunkLen));
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, chunkProgressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);  // No total timeout — chunk can be slow

            // Capture response
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                +[](char *ptr, size_t s, size_t n, void *ud) -> size_t {
                    auto *buf = static_cast<QByteArray *>(ud);
                    buf->append(ptr, static_cast<int>(s * n));
                    return s * n;
                });
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

            // Custom headers
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
            headers = curl_slist_append(headers, "Expect:");
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, rangeHeader.toUtf8().constData());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Reset seek for retry
            if (retry > 0) {
                FSEEKO64(srcFile, fromTarget, SEEK_SET);
                ctx.bytesRemaining = chunkLen;
            }

            CURLcode res = curl_easy_perform(curl);
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (m_abort.load()) {
                fclose(srcFile);
                return;
            }

            if (res == CURLE_OK && (httpCode == 200 || httpCode == 201 || httpCode == 206)) {
                lastResponse = QString::fromUtf8(responseBody);

                // Server invalidated the session mid-transfer.
                // Signal TransferService to re-create the session and re-queue
                // the task rather than showing an error to the user.
                if (lastResponse.contains(QStringLiteral("INVALID_UPLOAD_SESSION"))) {
                    fclose(srcFile);
                    emit sessionExpired();
                    return;
                }

                chunkOk = true;
                break;
            } else {
                chunkError = QStringLiteral("HTTP %1: %2")
                    .arg(httpCode).arg(QString::fromUtf8(curl_easy_strerror(res)));
                qWarning() << "[UploadEngine] Chunk failed (retry" << (retry + 1)
                           << "of" << MAX_CHUNK_RETRIES << "):" << chunkError;
                // Auth (401/403) / quota (507) → no point retrying with the
                // same credentials; surface the error to the caller, which
                // routes through TransferService → AuthService::tryRefreshSession
                // → optional retry with a refreshed session.  See ADR 003 D9/D11.
                if (httpCode == 401 || httpCode == 403 || httpCode == 507) break;

                // Exponential backoff: 1s, 2s, 4s, 8s, 16s — gives the network
                // a chance to recover from a route flap without burning bandwidth.
                if (retry < MAX_CHUNK_RETRIES - 1) {
                    const int delaySec = (1 << retry);  // 1, 2, 4, 8, 16
                    QThread::msleep(static_cast<unsigned long>(delaySec) * 1000UL);
                }
            }
        }

        if (!chunkOk) {
            fclose(srcFile);
            if (m_abort.load()) return;
            emit failed(chunkError.isEmpty() ? QStringLiteral("Chunk upload failed") : chunkError);
            return;
        }

        // Advance position
        fromTarget = toTarget;
        m_totalUploaded.store(fromTarget);

        // --- P3-1: Adjust chunk size based on elapsed time for this chunk.
        const auto chunkEndTime = std::chrono::steady_clock::now();
        const double elapsedSec = std::chrono::duration<double>(chunkEndTime - chunkStartTime).count();
        if (elapsedSec > 0.1) {  // guard against near-zero elapsed for tiny files
            const double bytesPerSec = static_cast<double>(chunkLen) / elapsedSec;
            const int64_t targetSize = static_cast<int64_t>(bytesPerSec * TARGET_CHUNK_SECS);
            currentChunkSize = qBound(DEFAULT_UPLOAD_CHUNK, targetSize, MAX_UPLOAD_CHUNK);
        }

        // Emit progress for completed chunks
        emit progressChanged(fromTarget, fileSize,
                             m_meter.speed(), m_meter.eta());
    }

    fclose(srcFile);

    if (m_abort.load()) return;

    // Parse final response for linkcode
    QString linkcode;
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream stream(lastResponse.toStdString());
    if (Json::parseFromStream(builder, stream, &root, &errs)) {
        // Try common field names
        if (root.isMember("url"))
            linkcode = QString::fromStdString(root["url"].asString());
        else if (root.isMember("linkcode"))
            linkcode = QString::fromStdString(root["linkcode"].asString());
        else if (root.isMember("link"))
            linkcode = QString::fromStdString(root["link"].asString());
    }
    if (linkcode.isEmpty()) linkcode = lastResponse.trimmed();

    qDebug() << "[UploadEngine] Upload complete:" << task.fileName << "→" << linkcode;
    emit completed(linkcode);
}

// ---------------------------------------------------------------------------
// queryResumeOffset — GCS-compatible resume query.
// Sends:  PUT {url}  Content-Range: bytes */{totalSize}  Content-Length: 0
// Server: 308 + Range: 0-{X}  → X+1 bytes received, resume from X+1
//         200/201              → upload already complete
//         other / error        → start from byte 0
// ---------------------------------------------------------------------------
int64_t UploadEngine::queryResumeOffset(const QString &url, int64_t totalSize)
{
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    QByteArray urlBytes = url.toUtf8();
    const QString rangeHdr = QStringLiteral("Content-Range: bytes */%1").arg(totalSize);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Length: 0");
    headers = curl_slist_append(headers, rangeHdr.toUtf8().constData());

    QByteArray respHeaders;
    curl_easy_setopt(curl, CURLOPT_URL,        urlBytes.constData());
    curl_easy_setopt(curl, CURLOPT_UPLOAD,     1L);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
        +[](char *ptr, size_t sz, size_t nmemb, void *ud) -> size_t {
            static_cast<QByteArray *>(ud)->append(ptr, static_cast<int>(sz * nmemb));
            return sz * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &respHeaders);
    // Discard body
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char *, size_t s, size_t n, void *) -> size_t { return s * n; });
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    // Fshare's nginx rejects requests with unrecognised User-Agent (HTTP 400
    // before the app layer). Must match HttpClient::createHandle — otherwise
    // resume detection silently fails and restart-from-byte-0 happens every
    // time, wasting bandwidth on large files.
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Fshare_Tool_2026");

    curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (httpCode == 200 || httpCode == 201) return totalSize;  // already complete

    if (httpCode == 308) {
        // Parse "Range: 0-{lastByte}" header (case-insensitive).
        // Defensive: a hostile or buggy server could return a value that is
        // negative, unparseable, or beyond totalSize — seeking there would
        // corrupt the upload or produce confusing I/O errors. Fall back to
        // restarting from byte 0 in every such case.
        const QString hdrs = QString::fromLatin1(respHeaders);
        static const QRegularExpression reRange(
            QStringLiteral("\\brange:\\s*0-(\\d+)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto m = reRange.match(hdrs);
        if (m.hasMatch()) {
            bool ok = false;
            const qint64 lastByte = m.captured(1).toLongLong(&ok);
            if (ok && lastByte >= 0 && lastByte < totalSize) {
                return lastByte + 1;
            }
            qWarning() << "[UploadEngine] Ignoring invalid resume offset"
                       << m.captured(1) << "(totalSize=" << totalSize
                       << "), restarting from 0.";
        }
    }

    return 0;  // start from beginning
}

void UploadEngine::pause()  { m_paused.store(true); }
void UploadEngine::resume() { m_paused.store(false); }
void UploadEngine::cancel() { m_abort.store(true); m_paused.store(false); }

} // namespace fsnext
