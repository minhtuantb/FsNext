#include "DownloadEngine.h"
#include <curl/curl.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QDebug>
#include <QThread>

#ifdef _WIN32
#include <io.h>
#define FSEEK64(f, o, w) _fseeki64(f, static_cast<__int64>(o), w)
#else
#include <unistd.h>
#define FSEEK64(f, o, w) fseeko(f, static_cast<off_t>(o), w)
#endif

namespace fsnext {

// Open a file path that may contain non-ASCII characters. Plain fopen() on
// Windows uses the current ANSI codepage for the path, which drops Vietnamese
// chars — use _wfopen with UTF-16 instead. On POSIX fopen with UTF-8 works.
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

// Minimum file size to enable multi-segment (2 MB).
// Below this the segment setup overhead exceeds the throughput gain.
static constexpr int64_t MIN_SEGMENTED_BYTES = 2LL * 1024 * 1024;

// Minimum bytes each segment should be responsible for.  Used to clamp the
// user-configured segment count down for smaller files so we don't spend more
// time on HTTP Range handshakes than on data transfer.
//
// Example: with default 16 segments configured, a 5 MB file would otherwise
// get 16 × ~312 KB chunks — mostly handshake overhead.  Clamped to 2 MB/seg
// that becomes 2 segments of ~2.5 MB each.  Large files (≥ 32 MB) still get
// the full 16-way parallelism.
static constexpr int64_t MIN_BYTES_PER_SEGMENT = 2LL * 1024 * 1024;

DownloadEngine::DownloadEngine(QObject *parent)
    : QObject(parent)
{
}

DownloadEngine::~DownloadEngine()
{
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

// ── Pause helper ─────────────────────────────────────────
// Called from CURL progress callbacks. Blocks until resumed or aborted.
// Returns true if the caller should abort the transfer.
bool DownloadEngine::waitIfPaused()
{
    if (!m_paused.load())
        return m_abort.load();

    std::unique_lock<std::mutex> lk(m_pauseMx);
    // Wait until either m_paused is cleared (resume) or m_abort is set (cancel).
    m_pauseCv.wait(lk, [this] { return !m_paused.load() || m_abort.load(); });
    return m_abort.load();
}

// ── Single-segment callbacks ──────────────────────────────

size_t DownloadEngine::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *engine = static_cast<DownloadEngine *>(userdata);
    if (engine->m_abort.load())
        return 0;
    if (!engine->m_file)
        return 0;
    return fwrite(ptr, size, nmemb, engine->m_file);
}

int DownloadEngine::progressCallback(void *clientp, int64_t dltotal, int64_t dlnow,
                                      int64_t /*ultotal*/, int64_t /*ulnow*/)
{
    auto *engine = static_cast<DownloadEngine *>(clientp);
    if (engine->m_abort.load())
        return 1;

    if (engine->waitIfPaused())
        return 1;  // aborted while waiting

    const int64_t resumeOffset = engine->m_resumeOffset.load();
    int64_t totalBytes = engine->m_totalBytes.load();

    if (dltotal > 0 || totalBytes > 0) {
        const int64_t absBytes = resumeOffset + dlnow;
        int64_t absTotal = totalBytes > 0 ? totalBytes : (resumeOffset + dltotal);

        if (totalBytes <= 0 && dltotal > 0) {
            engine->m_totalBytes.store(resumeOffset + dltotal);
            absTotal = resumeOffset + dltotal;
        }

        engine->m_meter.markProgress(absBytes);
        emit engine->progressChanged(absBytes, absTotal,
                                      engine->m_meter.speed(), engine->m_meter.eta());
    }
    return 0;
}

// ── Multi-segment callbacks ──────────────────────────────

size_t DownloadEngine::segmentWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *ctx = static_cast<SegmentContext *>(userdata);
    if (!ctx || !ctx->engine || ctx->engine->m_abort.load())
        return 0;
    if (!ctx->file)
        return 0;

    const size_t total = size * nmemb;

    // All HTTP/2 streams for THIS download share one FILE*.
    // Use the per-engine mutex so concurrent downloads don't block each other
    // (a global mutex would serialise writes across all active downloads).
    std::lock_guard<std::mutex> lock(ctx->engine->m_fileWriteMutex);
    if (FSEEK64(ctx->file, ctx->writeOffset, SEEK_SET) != 0)
        return 0;
    const size_t wrote = fwrite(ptr, 1, total, ctx->file);
    if (wrote > 0) {
        fflush(ctx->file);   // commit to OS immediately — prevents stale buffer
        ctx->writeOffset += static_cast<int64_t>(wrote);
    }
    return wrote;
}

int DownloadEngine::segmentProgressCallback(void *clientp, int64_t /*dltotal*/, int64_t dlnow,
                                             int64_t /*ultotal*/, int64_t /*ulnow*/)
{
    auto *ctx = static_cast<SegmentContext *>(clientp);
    if (!ctx || !ctx->engine)
        return 0;
    if (ctx->engine->m_abort.load())
        return 1;

    if (ctx->engine->waitIfPaused())
        return 1;

    // Update this segment's bytes
    const int idx = ctx->segIndex;
    if (idx >= 0 && idx < static_cast<int>(ctx->engine->m_segBytes.size())) {
        ctx->engine->m_segBytes[idx] = dlnow;
    }

    // Aggregate total bytes
    int64_t total = 0;
    for (int64_t b : ctx->engine->m_segBytes) total += b;

    const int64_t fileTotal = ctx->engine->m_totalBytes.load();
    ctx->engine->m_meter.markProgress(total);
    emit ctx->engine->progressChanged(total, fileTotal,
                                       ctx->engine->m_meter.speed(),
                                       ctx->engine->m_meter.eta());
    return 0;
}

// ── File info probe (Range support + Content-Length) ─────

DownloadEngine::FileProbe DownloadEngine::probeFileInfo(const QString &url)
{
    FileProbe result;

    CURL *curl = curl_easy_init();
    if (!curl) return result;

    QByteArray urlBytes = url.toUtf8();
    curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    // Discard body — we only need headers
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char *, size_t s, size_t n, void *) -> size_t { return s * n; });

    // Single HEAD request to get Content-Length + check Range support.
    // Previously we did Range:0-0 (downloads 1 byte) + a second HEAD — but
    // Fshare CDN URLs may be single/limited-use, and the 2-request probe was
    // consuming the token before the real download started. A HEAD request
    // doesn't transfer body bytes, so it's less likely to invalidate the URL.
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    // Include Accept-Ranges in the response we inspect (standard HTTP header)
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

    // Capture response headers to check Accept-Ranges
    QByteArray headerBuf;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
        +[](char *p, size_t sz, size_t nm, void *ud) -> size_t {
            auto *buf = static_cast<QByteArray *>(ud);
            buf->append(p, static_cast<int>(sz * nm));
            return sz * nm;
        });
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuf);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (res == CURLE_OK && (httpCode == 200 || httpCode == 206)) {
        curl_off_t cl = 0;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
        if (cl > 0)
            result.fileSize = static_cast<int64_t>(cl);

        // Check Accept-Ranges header (case-insensitive).
        // "Accept-Ranges: bytes" → server supports byte-range requests.
        const QString headers = QString::fromUtf8(headerBuf);
        result.rangeSupported = headers.contains(
            QStringLiteral("Accept-Ranges: bytes"), Qt::CaseInsensitive);

        // Some CDNs don't include Accept-Ranges but still support Range.
        // If Content-Length > 0 and status is 200, assume Range works —
        // the actual multi-segment download will fail fast if it doesn't.
        if (!result.rangeSupported && result.fileSize > 0)
            result.rangeSupported = true;
    }

    curl_easy_cleanup(curl);

    qDebug() << "[DownloadEngine] probe:" << url.left(80) << "→ size="
             << result.fileSize << "range=" << result.rangeSupported;
    return result;
}

// ── Multi-segment downloader ─────────────────────────────

bool DownloadEngine::downloadMultiSegment(const QString &url, const QString &localPath,
                                           int64_t fileSize, int nSegments)
{
    qDebug() << "[DownloadEngine] Multi-segment:" << nSegments
             << "segments for" << fileSize << "bytes";

    // Pre-allocate the file at full size
    {
        QFile qf(localPath);
        if (qf.exists()) qf.remove();
        if (!qf.open(QIODevice::WriteOnly)) {
            emit failed(QStringLiteral("Cannot create output file"));
            return false;
        }
        if (!qf.resize(fileSize)) {
            qf.close();
            emit failed(QStringLiteral("Cannot allocate file size"));
            return false;
        }
        qf.close();
    }

    // Open in r+b for concurrent writes at offsets. Use Unicode-safe helper
    // so Vietnamese / non-ASCII paths work on Windows (toLocal8Bit dropped bytes).
    m_file = openFileUnicode(localPath, "r+b");
    if (!m_file) {
        emit failed(QStringLiteral("Không thể mở file để ghi: %1").arg(localPath));
        return false;
    }
    // Disable stdio buffering so fseek+fwrite in the write callback is
    // direct-to-OS. Without this, interleaved seeks from different HTTP/2
    // streams can confuse stdio's internal buffer position tracking:
    //   Stream 0: fseek(A) → fwrite(data) → data in stdio buffer at pos A
    //   Stream 1: fseek(B) → flushes stream 0's buffer... but internal
    //             position may have drifted → data lands at wrong offset
    // With _IONBF every fwrite goes straight through to the kernel.
    setvbuf(m_file, nullptr, _IONBF, 0);

    // Setup per-segment contexts
    m_segBytes.assign(nSegments, 0);
    std::vector<SegmentContext> contexts(nSegments);
    std::vector<std::string> rangeBuffers(nSegments);  // each stores "start-end"
    std::vector<CURL *> handles(nSegments, nullptr);

    CURLM *multi = curl_multi_init();
    if (!multi) {
        fclose(m_file); m_file = nullptr;
        emit failed(QStringLiteral("CURL multi init failed"));
        return false;
    }

    // ── HTTP/2 multiplexing ─────────────────────────────────
    // Instead of 8 separate TCP connections (each with its own TLS handshake
    // and TCP slow-start), multiplex all byte-range streams over a SINGLE TCP
    // connection using HTTP/2 framing. This:
    //
    //   • Saves 7 × TLS handshakes (~200 ms each) = ~1.4 s start-up saved
    //   • Single congestion window ramps faster than 8 independent windows
    //   • CDN sees 1 connection → less likely to trigger per-connection shaping
    //   • Header compression (HPACK) reduces overhead for repeated Range headers
    //
    // CURLPIPE_MULTIPLEX tells the multi-handle to reuse connections and
    // interleave HTTP/2 streams when the server supports it. Falls back
    // gracefully to HTTP/1.1 separate connections if server doesn't support h2.
    curl_multi_setopt(multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    QByteArray urlBytes = url.toUtf8();

    for (int i = 0; i < nSegments; ++i) {
        const int64_t segStart = (static_cast<int64_t>(i) * fileSize) / nSegments;
        const int64_t segEnd = (i == nSegments - 1) ? (fileSize - 1)
            : (((static_cast<int64_t>(i) + 1) * fileSize) / nSegments - 1);

        rangeBuffers[i] = std::to_string(segStart) + "-" + std::to_string(segEnd);

        contexts[i].engine = this;
        contexts[i].file = m_file;
        contexts[i].writeOffset = segStart;
        contexts[i].segIndex = i;

        CURL *h = curl_easy_init();
        if (!h) {
            for (CURL *e : handles) if (e) { curl_multi_remove_handle(multi, e); curl_easy_cleanup(e); }
            curl_multi_cleanup(multi);
            fclose(m_file); m_file = nullptr;
            emit failed(QStringLiteral("CURL handle init failed"));
            return false;
        }

        curl_easy_setopt(h, CURLOPT_URL, urlBytes.constData());
        curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(h, CURLOPT_RANGE, rangeBuffers[i].c_str());
        curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, segmentWriteCallback);
        curl_easy_setopt(h, CURLOPT_WRITEDATA, &contexts[i]);
        curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(h, CURLOPT_XFERINFOFUNCTION, segmentProgressCallback);
        curl_easy_setopt(h, CURLOPT_XFERINFODATA, &contexts[i]);
        curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(h, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 30L);

        // ── HTTP/2 per-handle settings ──────────────────────
        // Request HTTP/2 (falls back to HTTP/1.1 if server/proxy doesn't
        // support it — no failure mode). PIPEWAIT tells this handle to wait
        // for the multiplexed connection from handle 0 instead of opening a
        // new TCP socket, ensuring all streams share the same socket.
        curl_easy_setopt(h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(h, CURLOPT_PIPEWAIT, 1L);

        // ── TCP / throughput tuning (match IDM behaviour) ───
        curl_easy_setopt(h, CURLOPT_BUFFERSIZE, 256L * 1024);
        curl_easy_setopt(h, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(h, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(h, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(h, CURLOPT_TCP_KEEPINTVL, 60L);

        curl_multi_add_handle(multi, h);
        handles[i] = h;
    }

    // ── Multi-perform event loop ────────────────────────────
    // Tight poll interval (10 ms) vs old 250 ms. The OS wakes us when data
    // arrives on any socket, so 10 ms is not a busy-spin — it's just the
    // maximum sleep between data-available checks. IDM and other accelerators
    // use similarly tight loops. The 250 ms original value was measurably
    // bottlenecking throughput on fast links: at 100 MB/s, 250 ms of latency
    // per poll iteration = 25 MB of missed buffering.
    int still = 0;
    bool hadError = false;
    do {
        if (m_abort.load()) break;
        curl_multi_perform(multi, &still);

        // Drain finished messages
        CURLMsg *msg = nullptr;
        int msgsLeft = 0;
        while ((msg = curl_multi_info_read(multi, &msgsLeft)) != nullptr) {
            if (msg->msg != CURLMSG_DONE) continue;
            if (msg->data.result != CURLE_OK) {
                qWarning() << "[DownloadEngine] Segment failed:" << curl_easy_strerror(msg->data.result);
                hadError = true;
            }
        }

        if (still > 0)
            curl_multi_wait(multi, nullptr, 0, 10, nullptr);
    } while (still > 0 && !m_abort.load() && !hadError);

    // Cleanup
    for (CURL *h : handles) {
        if (h) {
            curl_multi_remove_handle(multi, h);
            curl_easy_cleanup(h);
        }
    }
    curl_multi_cleanup(multi);
    fclose(m_file); m_file = nullptr;

    if (m_abort.load()) return false;
    if (hadError) {
        QFile::remove(localPath);
        return false;
    }

    // Verify final file size
    QFileInfo fi(localPath);
    if (!fi.exists() || fi.size() < fileSize) {
        emit failed(QStringLiteral("File incomplete after multi-segment download"));
        return false;
    }

    return true;
}

// ── Entry point ──────────────────────────────────────────

void DownloadEngine::startDownload(const TransferTask &task)
{
    m_task = task;
    m_abort.store(false);
    m_paused.store(false);

    QDir dir(QFileInfo(task.localPath).absolutePath());
    if (!dir.exists()) dir.mkpath(".");

    if (task.realUrl.isEmpty()) {
        emit failed(QStringLiteral("No download URL available"));
        return;
    }

    // ── Probe the download URL for file size + Range support ─────
    // fileSize is typically 0 at this point (the Fshare createDownloadSession
    // API only returns a URL, not metadata). We probe with a HEAD + Range:0-0
    // request to discover both, which is the same technique IDM / legacy
    // Fshare Tool use before starting a multi-part download.
    int64_t fileSize = task.fileSize;
    bool rangeSupported = false;
    {
        FileProbe probe = probeFileInfo(task.realUrl);
        if (probe.fileSize > 0)
            fileSize = probe.fileSize;
        rangeSupported = probe.rangeSupported;
    }

    m_meter.start(fileSize);
    m_totalBytes.store(fileSize);

    bool success = false;
    const int requestedSegments = qBound(1, task.segments, 32);

    // Adaptive segment count — never assign less than MIN_BYTES_PER_SEGMENT
    // of work per segment.  For small files this collapses N configured
    // segments down to whatever fits the file without starving any segment
    // below the overhead break-even point.  Falls back to requestedSegments
    // verbatim when the probe couldn't resolve fileSize (fileSize <= 0).
    int effectiveSegments = requestedSegments;
    if (fileSize > 0) {
        const int fittable = static_cast<int>(fileSize / MIN_BYTES_PER_SEGMENT);
        effectiveSegments = qBound(1, qMin(requestedSegments, fittable), 32);
    }

    QFileInfo fi(task.localPath);
    const bool isResume = fi.exists() && fi.size() > 0 && fileSize > 0 && fi.size() < fileSize;

    // ── Disk-space preflight ───────────────────────────────────
    // Done AFTER probe so we know the real target size (task.fileSize is 0
    // until the Fshare API + HEAD probe have completed). Failing fast here
    // turns a confusing mid-transfer "qf.resize() failed" into a clear
    // up-front error and spares the user a long wait on a doomed transfer.
    // We need (target - already-downloaded) bytes; on resume that is less
    // than the full fileSize.
    if (fileSize > 0) {
        const qint64 needed = isResume ? (fileSize - fi.size()) : fileSize;
        const QString parentDir = QFileInfo(task.localPath).absolutePath();
        const QStorageInfo storage(parentDir);
        if (storage.isValid() && storage.isReady()) {
            const qint64 available = storage.bytesAvailable();
            // Reserve 16 MiB headroom so we don't fill the disk to the last
            // byte — the OS and other apps need room to breathe.
            static constexpr qint64 kHeadroomBytes = 16 * 1024 * 1024;
            if (available < needed + kHeadroomBytes) {
                const auto mib = [](qint64 b){ return b / (1024 * 1024); };
                emit failed(QStringLiteral("Không đủ dung lượng ổ đĩa tại %1: "
                                           "cần %2 MiB, còn trống %3 MiB.")
                            .arg(parentDir)
                            .arg(mib(needed))
                            .arg(mib(available)));
                return;
            }
        }
        // If storage is invalid / network volume with unknown free space,
        // fall through and let the write path surface the error later.
    }

    // ── Decide: multi-segment (IDM-style) or single ─────────────
    // Multi-segment requires ALL of:
    //   • not resuming an existing partial file
    //   • user configured > 1 segment (default 16 in TransferTask)
    //   • file >= 2 MB (too small = not worth the overhead)
    //   • server supports HTTP Range (returns Accept-Ranges: bytes)
    //   • file size is known (probe succeeded)
    if (!isResume && effectiveSegments > 1
        && fileSize >= MIN_SEGMENTED_BYTES && rangeSupported) {
        qDebug() << "[DownloadEngine] Range supported, using" << effectiveSegments
                 << "segments for" << fileSize << "bytes"
                 << "(requested:" << requestedSegments << ")";
        m_isMultiSegment.store(true);
        success = downloadMultiSegment(task.realUrl, task.localPath, fileSize, effectiveSegments);
    } else {
        qDebug() << "[DownloadEngine] Single segment — size:" << fileSize
                 << "range:" << rangeSupported << "resume:" << isResume
                 << "effective/requested segments:" << effectiveSegments
                 << "/" << requestedSegments;
    }

    if (!success && !m_abort.load()) {
        m_isMultiSegment.store(false);
        success = downloadSingleSegment(task.realUrl, task.localPath, task.fileSize);
    }

    if (m_file) { fclose(m_file); m_file = nullptr; }

    if (m_abort.load()) return;

    // On failure, the single/multi-segment routines already emit a specific
    // `failed(...)` with the underlying CURL / HTTP / I/O error. Emitting a
    // generic "Download failed" here OVERWRITES that specific message in the
    // UI (taskFailed subscribers receive both). Only emit completed() on
    // success — let the specific error stand.
    if (success)
        emit completed(task.localPath);
}

bool DownloadEngine::downloadSingleSegment(const QString &url, const QString &localPath, int64_t fileSize)
{
    m_file = openFileUnicode(localPath, "wb");
    if (!m_file) {
        emit failed(QStringLiteral("Không thể tạo file: %1").arg(localPath));
        return false;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        emit failed(QStringLiteral("CURL init failed"));
        return false;
    }

    QByteArray urlBytes = url.toUtf8();
    curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    // TCP tuning — same as multi-segment
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 256L * 1024);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

    m_totalBytes.store(fileSize);
    m_resumeOffset.store(0);

    QFileInfo fi(localPath);
    if (fi.exists() && fi.size() > 0 && fi.size() < fileSize) {
        fclose(m_file);
        m_file = openFileUnicode(localPath, "ab");
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(fi.size()));
        m_resumeOffset.store(fi.size());
    }

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res == CURLE_ABORTED_BY_CALLBACK) return false;
    if (res != CURLE_OK) {
        emit failed(QString::fromUtf8(curl_easy_strerror(res)));
        return false;
    }
    if (httpCode != 200 && httpCode != 206) {
        emit failed(QStringLiteral("HTTP error: %1").arg(httpCode));
        return false;
    }

    return true;
}

void DownloadEngine::pause()
{
    m_paused.store(true);
    // No cv notification needed on pause — the callback will naturally
    // enter waitIfPaused() on its next invocation.
}

void DownloadEngine::resume()
{
    m_paused.store(false);
    // Wake the callback thread immediately so it doesn't sit in wait().
    m_pauseCv.notify_all();
}

void DownloadEngine::cancel()
{
    m_abort.store(true);
    m_paused.store(false);
    // Wake any thread blocked in waitIfPaused() so it sees m_abort=true.
    m_pauseCv.notify_all();
}

} // namespace fsnext
