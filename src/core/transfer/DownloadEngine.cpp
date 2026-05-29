#include "DownloadEngine.h"
#include <curl/curl.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QDebug>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <chrono>

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

// Per-segment transient-failure retry budget for multi-segment downloads.
// A segment that errors (connection reset, CDN hiccup) is re-issued for its
// REMAINING byte range up to this many times before the whole download is
// declared failed.  Mirrors the single-segment retry budget below.
static constexpr int MAX_SEGMENT_RETRIES = 5;
// Retry budget for the single-segment path (transient curl errors only).
static constexpr int MAX_SINGLE_RETRIES  = 5;
// Flush the resume journal to disk at most this often during a transfer.
static constexpr qint64 SIDECAR_FLUSH_MS = 1000;

// Short Vietnamese description for the few HTTP statuses our error messages
// surface. Anything outside the map falls back to "Mã HTTP %1".
static QString httpCodeDescription(long httpCode)
{
    switch (httpCode) {
    case 0:   return {};
    case 401: return QStringLiteral("phiên đăng nhập hết hạn");
    case 403: return QStringLiteral("bị từ chối truy cập — link tải có thể đã hết hạn");
    case 404: return QStringLiteral("không tìm thấy file trên máy chủ");
    case 408: return QStringLiteral("máy chủ hết thời gian chờ");
    case 416: return QStringLiteral("máy chủ không hỗ trợ tiếp tục (range)");
    case 429: return QStringLiteral("đã vượt quá giới hạn yêu cầu (rate-limit)");
    case 500: return QStringLiteral("lỗi nội bộ máy chủ");
    case 502: return QStringLiteral("cổng máy chủ trung gian lỗi (bad gateway)");
    case 503: return QStringLiteral("dịch vụ máy chủ không khả dụng");
    case 504: return QStringLiteral("gateway hết thời gian chờ");
    default:  return {};
    }
}

// Build a human-readable failure line that names both the underlying CURL
// error (if any) AND the HTTP status. `segIdx` ≥ 0 includes "(segment #N)".
static QString formatDownloadError(CURLcode curl, long httpCode, int segIdx = -1)
{
    QString out;
    const QString httpDesc = httpCodeDescription(httpCode);
    if (httpCode > 0) {
        out = QStringLiteral("HTTP %1").arg(httpCode);
        if (!httpDesc.isEmpty()) out += QStringLiteral(" — ") + httpDesc;
    }
    if (curl != CURLE_OK) {
        const QString why = QString::fromUtf8(curl_easy_strerror(curl));
        if (!out.isEmpty()) out += QStringLiteral("; ");
        out += QStringLiteral("CURL: %1 (mã %2)").arg(why).arg(static_cast<int>(curl));
    }
    if (segIdx >= 0)
        out += QStringLiteral(" [segment #%1]").arg(segIdx);
    if (out.isEmpty())
        out = QStringLiteral("Lỗi không xác định");
    return out;
}

QString DownloadEngine::sidecarPath(const QString &localPath)
{
    return localPath + QStringLiteral(".fsdownload");
}

bool DownloadEngine::writeSidecar(const QString &localPath, int64_t fileSize,
                                  const std::vector<SegmentContext> &ctx) const
{
    QJsonObject root;
    root[QStringLiteral("v")]    = 1;
    root[QStringLiteral("size")] = static_cast<double>(fileSize);
    QJsonArray segs;
    for (const SegmentContext &c : ctx) {
        QJsonObject s;
        s[QStringLiteral("s")] = static_cast<double>(c.segStart);
        s[QStringLiteral("e")] = static_cast<double>(c.segEnd);
        s[QStringLiteral("c")] = static_cast<double>(c.writeOffset);
        segs.append(s);
    }
    root[QStringLiteral("segs")] = segs;

    QFile f(sidecarPath(localPath));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    return true;
}

bool DownloadEngine::readSidecar(const QString &localPath, int64_t fileSize,
                                 std::vector<SegmentState> &out) const
{
    // The journal is only trustworthy if the pre-allocated data file still
    // exists at exactly the expected size — otherwise the byte offsets in it
    // point into a file that no longer matches, so we start fresh.
    QFileInfo dataInfo(localPath);
    if (!dataInfo.exists() || dataInfo.size() != fileSize)
        return false;

    QFile f(sidecarPath(localPath));
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray raw = f.readAll();
    f.close();

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject())
        return false;
    const QJsonObject root = doc.object();
    if (static_cast<int64_t>(root.value(QStringLiteral("size")).toDouble(0)) != fileSize)
        return false;
    const QJsonArray segs = root.value(QStringLiteral("segs")).toArray();
    if (segs.isEmpty())
        return false;

    out.clear();
    for (const QJsonValue &v : segs) {
        const QJsonObject s = v.toObject();
        SegmentState st;
        st.start = static_cast<int64_t>(s.value(QStringLiteral("s")).toDouble(-1));
        st.end   = static_cast<int64_t>(s.value(QStringLiteral("e")).toDouble(-1));
        st.cur   = static_cast<int64_t>(s.value(QStringLiteral("c")).toDouble(-1));
        // Reject anything malformed: ranges must be ordered, cur within
        // [start, end+1].  A single bad entry voids the whole journal so we
        // never seek to a garbage offset and corrupt the file.
        if (st.start < 0 || st.end < st.start || st.cur < st.start || st.cur > st.end + 1) {
            out.clear();
            return false;
        }
        out.push_back(st);
    }
    return true;
}

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

void DownloadEngine::applyProxy(CURL *curl) const
{
    if (!m_proxyUrl.isEmpty())
        curl_easy_setopt(curl, CURLOPT_PROXY, m_proxyUrl.toUtf8().constData());
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
        // The stream is opened unbuffered (_IONBF), so every fwrite goes
        // straight to the OS — an explicit fflush here is redundant syscall
        // overhead. The kernel buffer is what makes the bytes visible to the
        // resume journal / a subsequent reopen after an app crash.
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

    // Update this segment's ABSOLUTE downloaded bytes from the write offset
    // (segStart + bytes written so far), not curl's per-request dlnow. On a
    // resumed segment dlnow restarts at 0 for the remaining range, so dlnow
    // alone would under-count by the bytes already on disk.
    const int idx = ctx->segIndex;
    if (idx >= 0 && idx < static_cast<int>(ctx->engine->m_segBytes.size())) {
        ctx->engine->m_segBytes[idx] = ctx->writeOffset - ctx->segStart;
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
    applyProxy(curl);
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
    // ── Resume detection ────────────────────────────────────
    // A valid journal next to a full-size pre-allocated file means we can pick
    // each segment back up from its recorded offset instead of re-fetching the
    // whole file. The segment layout comes from the journal in that case (it
    // may differ from the current `nSegments` setting).
    std::vector<SegmentState> resumeSegs;
    const bool resuming = readSidecar(localPath, fileSize, resumeSegs);
    if (resuming) {
        nSegments = static_cast<int>(resumeSegs.size());
        qDebug() << "[DownloadEngine] Multi-segment RESUME:" << nSegments
                 << "segments for" << fileSize << "bytes";
    } else {
        qDebug() << "[DownloadEngine] Multi-segment:" << nSegments
                 << "segments for" << fileSize << "bytes";
        // Pre-allocate the file at full size and drop any stale journal.
        QFile qf(localPath);
        if (qf.exists()) qf.remove();
        if (!qf.open(QIODevice::WriteOnly)) {
            emit failed(QStringLiteral("Không thể tạo file đích: %1").arg(localPath));
            return false;
        }
        if (!qf.resize(fileSize)) {
            qf.close();
            // Phrasing kept aligned with TransferService::isTransientDownloadError
            // so a permanent filesystem failure isn't retried in a loop.
            emit failed(QStringLiteral("Không thể tạo file đích (cấp phát %1 byte thất bại)").arg(fileSize));
            return false;
        }
        qf.close();
        QFile::remove(sidecarPath(localPath));
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
    // streams can confuse stdio's internal buffer position tracking.
    setvbuf(m_file, nullptr, _IONBF, 0);

    // Per-segment state. contexts hands pointers to curl, so it must NOT be
    // resized after this point.
    m_segBytes.assign(nSegments, 0);
    std::vector<SegmentContext> contexts(nSegments);
    std::vector<std::string> rangeBuffers(nSegments);   // "start-end" per segment
    std::vector<CURL *> handles(nSegments, nullptr);
    std::vector<int>     segRetries(nSegments, 0);
    std::vector<qint64>  segRetryAtMs(nSegments, -1);    // >=0 → awaiting re-add

    for (int i = 0; i < nSegments; ++i) {
        int64_t s, e, c;
        if (resuming) { s = resumeSegs[i].start; e = resumeSegs[i].end; c = resumeSegs[i].cur; }
        else {
            s = (static_cast<int64_t>(i) * fileSize) / nSegments;
            e = (i == nSegments - 1) ? (fileSize - 1)
                : (((static_cast<int64_t>(i) + 1) * fileSize) / nSegments - 1);
            c = s;
        }
        contexts[i].engine = this;
        contexts[i].file = m_file;
        contexts[i].segStart = s;
        contexts[i].segEnd = e;
        contexts[i].writeOffset = c;
        contexts[i].segIndex = i;
        m_segBytes[i] = c - s;
    }

    CURLM *multi = curl_multi_init();
    if (!multi) {
        fclose(m_file); m_file = nullptr;
        emit failed(QStringLiteral("Khởi tạo libcurl multi thất bại"));
        return false;
    }
    // HTTP/2 multiplexing — all byte-range streams share ONE TCP connection
    // (1 TLS handshake, single congestion window). Falls back to HTTP/1.1
    // separate connections if the server doesn't speak h2.
    curl_multi_setopt(multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    const QByteArray urlBytes = url.toUtf8();

    // Build a fresh easy handle for segment `i` covering [from .. segEnd].
    auto makeHandle = [&](int i, int64_t from) -> CURL * {
        CURL *h = curl_easy_init();
        if (!h) return nullptr;
        rangeBuffers[i] = std::to_string(from) + "-" + std::to_string(contexts[i].segEnd);
        contexts[i].writeOffset = from;
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
        applyProxy(h);
        // Request HTTP/2; PIPEWAIT makes this handle reuse handle-0's socket
        // instead of opening a new TCP connection.
        curl_easy_setopt(h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(h, CURLOPT_PIPEWAIT, 1L);
        curl_easy_setopt(h, CURLOPT_BUFFERSIZE, 256L * 1024);
        curl_easy_setopt(h, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(h, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(h, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(h, CURLOPT_TCP_KEEPINTVL, 60L);
        return h;
    };

    // Initial handle set — skip segments already complete from a prior run.
    bool initError = false;
    for (int i = 0; i < nSegments; ++i) {
        if (contexts[i].writeOffset > contexts[i].segEnd)
            continue;
        CURL *h = makeHandle(i, contexts[i].writeOffset);
        if (!h) { initError = true; break; }
        curl_multi_add_handle(multi, h);
        handles[i] = h;
    }
    if (initError) {
        for (CURL *e : handles) if (e) { curl_multi_remove_handle(multi, e); curl_easy_cleanup(e); }
        curl_multi_cleanup(multi);
        fclose(m_file); m_file = nullptr;
        emit failed(QStringLiteral("Khởi tạo handle libcurl cho luồng tải thất bại"));
        return false;
    }

    QElapsedTimer clock; clock.start();
    qint64 lastFlushMs = 0;
    bool terminal    = false;   // a segment exhausted its retry budget
    bool rangeBroken = false;   // server ignored Range (200 instead of 206)

    // Diagnostics — captured from the last segment failure so the user-facing
    // error message can name the actual cause (curl text + HTTP code +
    // segment index) instead of a generic "tải đa luồng thất bại".
    CURLcode lastFailCurl = CURLE_OK;
    long     lastFailHttp = 0;
    int      lastFailSeg  = -1;

    auto anyPending = [&]() {
        for (qint64 t : segRetryAtMs) if (t >= 0) return true;
        return false;
    };

    int still = 0;
    do {
        if (m_abort.load()) break;
        curl_multi_perform(multi, &still);

        // Drain finished messages — classify each as success / retryable error.
        CURLMsg *msg = nullptr;
        int msgsLeft = 0;
        while ((msg = curl_multi_info_read(multi, &msgsLeft)) != nullptr) {
            if (msg->msg != CURLMSG_DONE) continue;
            CURL *eh = msg->easy_handle;
            const CURLcode res = msg->data.result;
            long httpCode = 0;
            curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &httpCode);
            int idx = -1;
            for (int i = 0; i < nSegments; ++i) if (handles[i] == eh) { idx = i; break; }
            curl_multi_remove_handle(multi, eh);
            curl_easy_cleanup(eh);
            if (idx >= 0) handles[idx] = nullptr;

            if (res == CURLE_OK) {
                // A 200 (instead of 206) means the server served the whole file
                // on a single stream and ignored Range — the parallel writes
                // have corrupted each other. Bail to the single-stream fallback.
                if (httpCode != 206 && httpCode != 200) {
                    terminal = true;
                    lastFailCurl = CURLE_OK;
                    lastFailHttp = httpCode;
                    lastFailSeg  = idx;
                }
                else if (httpCode == 200) { rangeBroken = true; terminal = true; }
                continue;
            }
            if (idx < 0) { terminal = true; lastFailCurl = res; lastFailHttp = httpCode; continue; }
            if (segRetries[idx] >= MAX_SEGMENT_RETRIES) {
                qWarning() << "[DownloadEngine] Segment" << idx << "exhausted retries:"
                           << curl_easy_strerror(res) << "HTTP" << httpCode;
                terminal = true;
                lastFailCurl = res;
                lastFailHttp = httpCode;
                lastFailSeg  = idx;
                continue;
            }
            const int n = ++segRetries[idx];
            const qint64 backoff = qMin<qint64>(1000LL << (n - 1), 16000);  // 1,2,4,8,16s
            segRetryAtMs[idx] = clock.elapsed() + backoff;
            qWarning() << "[DownloadEngine] Segment" << idx << "failed ("
                       << curl_easy_strerror(res) << ") retry" << n << "in" << backoff << "ms";
        }

        // Re-arm segments whose backoff elapsed (over their remaining range).
        if (!terminal && !m_abort.load()) {
            const qint64 nowMs = clock.elapsed();
            for (int i = 0; i < nSegments; ++i) {
                if (segRetryAtMs[i] < 0 || nowMs < segRetryAtMs[i]) continue;
                if (contexts[i].writeOffset > contexts[i].segEnd) { segRetryAtMs[i] = -1; continue; }
                CURL *h = makeHandle(i, contexts[i].writeOffset);
                if (!h) { terminal = true; break; }
                curl_multi_add_handle(multi, h);
                handles[i] = h;
                segRetryAtMs[i] = -1;
            }
        }

        // Journal flush so a crash/kill resumes near the last committed offset.
        const qint64 nowMs = clock.elapsed();
        if (nowMs - lastFlushMs >= SIDECAR_FLUSH_MS) {
            writeSidecar(localPath, fileSize, contexts);
            lastFlushMs = nowMs;
        }

        if (still > 0)        curl_multi_wait(multi, nullptr, 0, 10, nullptr);
        else if (anyPending()) QThread::msleep(50);   // backoff wait — don't busy-spin
    } while ((still > 0 || anyPending()) && !m_abort.load() && !terminal);

    // Detach + free any still-attached handles.
    for (CURL *h : handles)
        if (h) { curl_multi_remove_handle(multi, h); curl_easy_cleanup(h); }
    curl_multi_cleanup(multi);
    fclose(m_file); m_file = nullptr;

    // Aborted (pause-to-stop / cancel): keep file + journal for a later resume.
    if (m_abort.load()) {
        writeSidecar(localPath, fileSize, contexts);
        return false;
    }

    if (terminal) {
        int64_t got = 0; for (int64_t b : m_segBytes) got += b;
        // Range genuinely unsupported, or a fresh attempt that got nowhere:
        // discard and let startDownload fall back to a plain single stream.
        if (rangeBroken || (!resuming && got == 0)) {
            QFile::remove(localPath);
            QFile::remove(sidecarPath(localPath));
            return false;
        }
        // Otherwise keep the partial + journal so a later retry resumes, and
        // tell startDownload NOT to truncate it via the single-segment path.
        writeSidecar(localPath, fileSize, contexts);
        m_skipSingleFallback = true;
        const QString detail = formatDownloadError(lastFailCurl, lastFailHttp, lastFailSeg);
        const int64_t mib = (got > 0) ? (got / (1024 * 1024)) : 0;
        emit failed(QStringLiteral(
            "Tải đa luồng thất bại sau %1 lần thử mỗi luồng — %2. "
            "Đã giữ %3 MiB để tiếp tục lần sau.")
                    .arg(MAX_SEGMENT_RETRIES)
                    .arg(detail)
                    .arg(mib));
        return false;
    }

    // Success — verify exact size and drop the journal.
    QFileInfo fi(localPath);
    if (!fi.exists() || fi.size() != fileSize) {
        emit failed(QStringLiteral(
            "File tải về không đầy đủ: mong %1 byte, thực tế %2 byte.")
                    .arg(fileSize).arg(fi.exists() ? fi.size() : qint64(0)));
        return false;
    }
    QFile::remove(sidecarPath(localPath));
    return true;
}

// ── Entry point ──────────────────────────────────────────

void DownloadEngine::startDownload(const TransferTask &task)
{
    m_task = task;
    m_abort.store(false);
    m_paused.store(false);
    m_skipSingleFallback = false;

    QDir dir(QFileInfo(task.localPath).absolutePath());
    if (!dir.exists()) dir.mkpath(".");

    if (task.realUrl.isEmpty()) {
        emit failed(QStringLiteral("Không có URL tải xuống (tạo session thất bại)"));
        return;
    }

    // ── Probe the download URL for file size + Range support ─────
    // fileSize is typically 0 at this point (the Fshare createDownloadSession
    // API only returns a URL, not metadata). probeFileInfo() issues a single
    // HEAD request to read Content-Length + Accept-Ranges without consuming a
    // single-use CDN token (a body-fetching probe could invalidate the URL).
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
    // A multi-segment resume is keyed off the journal, not the file size (the
    // data file is pre-allocated to full size, so a size check can't detect it).
    const bool hasSidecar = QFileInfo::exists(sidecarPath(task.localPath));
    // Single-segment resume: a partial file shorter than the target with NO
    // multi-segment journal beside it.
    const bool isResume = !hasSidecar && fi.exists() && fi.size() > 0
                          && fileSize > 0 && fi.size() < fileSize;

    // ── Disk-space preflight ───────────────────────────────────
    // Done AFTER probe so we know the real target size (task.fileSize is 0
    // until the Fshare API + HEAD probe have completed). Failing fast here
    // turns a confusing mid-transfer "qf.resize() failed" into a clear
    // up-front error and spares the user a long wait on a doomed transfer.
    // We need (target - already-downloaded) bytes; on resume that is less
    // than the full fileSize.
    if (fileSize > 0) {
        // On any resume the bytes already on disk don't need to be re-reserved.
        // A multi-segment resume's file is pre-allocated to full size, so
        // (fileSize - fi.size()) is ~0 there; a single-segment partial needs
        // only the remainder.
        const qint64 onDisk = (isResume || hasSidecar) ? static_cast<qint64>(fi.size()) : 0;
        const qint64 needed = qMax<qint64>(0, fileSize - onDisk);
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
    // Use multi-segment when:
    //   • there is a multi-segment resume journal to continue (hasSidecar), OR
    //   • a fresh start qualifies: not a single-segment resume, > 1 segment,
    //     file >= 2 MB, server honours Range, and the size is known.
    // downloadMultiSegment detects + continues the journal internally.
    const bool freshMulti = !isResume && effectiveSegments > 1
                            && fileSize >= MIN_SEGMENTED_BYTES && rangeSupported;
    if (hasSidecar || freshMulti) {
        qDebug() << "[DownloadEngine] Multi-segment (" << effectiveSegments
                 << "seg, sidecar=" << hasSidecar << ") for" << fileSize << "bytes";
        m_isMultiSegment.store(true);
        success = downloadMultiSegment(task.realUrl, task.localPath, fileSize, effectiveSegments);
    } else {
        qDebug() << "[DownloadEngine] Single segment — size:" << fileSize
                 << "range:" << rangeSupported << "resume:" << isResume
                 << "effective/requested segments:" << effectiveSegments
                 << "/" << requestedSegments;
    }

    // Fall back to a single stream only when the multi-segment attempt left no
    // resumable partial (m_skipSingleFallback guards against truncating one).
    // Pass the PROBED fileSize (task.fileSize is usually 0 here) so the
    // single-segment path can detect + continue a partial via Range.
    if (!success && !m_abort.load() && !m_skipSingleFallback) {
        m_isMultiSegment.store(false);
        success = downloadSingleSegment(task.realUrl, task.localPath, fileSize);
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
    m_totalBytes.store(fileSize);
    const QByteArray urlBytes = url.toUtf8();

    // Retry transient failures from the current on-disk offset with exponential
    // backoff. A still-valid URL recovers here; an expired CDN URL fails with a
    // permanent HTTP code and is handled one level up (TransferService re-resolves
    // the session and re-dispatches).
    for (int attempt = 0; attempt <= MAX_SINGLE_RETRIES && !m_abort.load(); ++attempt) {
        const QFileInfo fi(localPath);
        const int64_t resumeFrom =
            (fi.exists() && fi.size() > 0 && fileSize > 0 && fi.size() < fileSize)
                ? static_cast<int64_t>(fi.size()) : 0;
        m_resumeOffset.store(resumeFrom);

        // Open BEFORE deciding the mode — opening "wb" would truncate an
        // existing partial, so only use it for a genuine fresh start.
        m_file = openFileUnicode(localPath, resumeFrom > 0 ? "ab" : "wb");
        if (!m_file) {
            emit failed(QStringLiteral("Không thể tạo file: %1").arg(localPath));
            return false;
        }

        CURL *curl = curl_easy_init();
        if (!curl) {
            fclose(m_file); m_file = nullptr;
            emit failed(QStringLiteral("Khởi tạo libcurl thất bại"));
            return false;
        }
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
        applyProxy(curl);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 256L * 1024);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
        if (resumeFrom > 0)
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(resumeFrom));

        const CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
        fclose(m_file); m_file = nullptr;

        // Paused-to-stop / cancelled — leave the partial for a later resume.
        if (m_abort.load() || res == CURLE_ABORTED_BY_CALLBACK)
            return false;

        if (res == CURLE_OK) {
            if (httpCode == 206 || (httpCode == 200 && resumeFrom == 0))
                return true;
            if (httpCode == 200 && resumeFrom > 0) {
                // Server ignored the resume offset and streamed the whole file,
                // which got appended after the existing partial → corrupt.
                // Discard and let the next attempt start clean from byte 0.
                QFile::remove(localPath);
                qWarning() << "[DownloadEngine] Server ignored resume (HTTP 200);"
                              " restarting single-segment from scratch.";
            } else if (httpCode == 403 || httpCode == 404 ||
                       httpCode == 416 || httpCode == 401) {
                // Permanent — retrying the same URL won't help.
                emit failed(formatDownloadError(CURLE_OK, httpCode));
                return false;
            }
        }

        if (attempt >= MAX_SINGLE_RETRIES) {
            emit failed(QStringLiteral("Tải xuống thất bại sau %1 lần thử — %2")
                        .arg(MAX_SINGLE_RETRIES + 1)
                        .arg(formatDownloadError(res, httpCode)));
            return false;
        }

        // Interruptible exponential backoff (1,2,4,8,16s) so pause/cancel during
        // the wait stays responsive.
        const int delayMs = qMin(1000 << attempt, 16000);
        qWarning() << "[DownloadEngine] Single-segment attempt" << (attempt + 1)
                   << "failed; retry in" << delayMs << "ms";
        for (int slept = 0; slept < delayMs && !m_abort.load(); slept += 100)
            QThread::msleep(100);
    }
    return false;
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
