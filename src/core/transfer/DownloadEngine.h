#pragma once

#include "SpeedMeter.h"
#include "core/models/TransferTask.h"
#include <QObject>
#include <QString>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

typedef void CURL;

namespace fsnext {

class DownloadEngine : public QObject
{
    Q_OBJECT

public:
    explicit DownloadEngine(QObject *parent = nullptr);
    ~DownloadEngine() override;

    // segments: 1 = single, 2-8 = multi-segment with HTTP Range parallel
    void startDownload(const TransferTask &task);
    void pause();
    void resume();
    void cancel();

    // Full libcurl proxy spec ("host:port" / "scheme://host:port"); empty = no
    // proxy. Must be called before startDownload(). Applied to every curl
    // handle the engine creates (probe / single / multi-segment) so downloads
    // honour the same proxy preference as API calls.
    void setProxy(const QString &proxyUrl) { m_proxyUrl = proxyUrl; }

signals:
    void progressChanged(int64_t bytesDownloaded, int64_t totalBytes, double speed, const QString &eta);
    void completed(const QString &filePath);
    void failed(const QString &error);

private:
    // Single-segment (with resume support)
    bool downloadSingleSegment(const QString &url, const QString &localPath, int64_t fileSize);

    // Multi-segment with HTTP Range (IDM-style parallel chunks)
    struct FileProbe {
        int64_t fileSize = 0;       // Content-Length from server
        bool    rangeSupported = false; // HTTP 206 on Range: 0-0
    };
    FileProbe probeFileInfo(const QString &url);
    bool downloadMultiSegment(const QString &url, const QString &localPath, int64_t fileSize, int nSegments);

    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static int progressCallback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);

    // Apply the configured proxy (if any) to a curl easy handle.
    void applyProxy(CURL *curl) const;

    // Multi-segment context (per-segment state)
    struct SegmentContext {
        DownloadEngine *engine;
        FILE *file;          // shared file pointer (seek + write)
        int64_t segStart;    // first byte of this segment's range (immutable)
        int64_t segEnd;      // last byte of this segment's range (immutable)
        int64_t writeOffset; // current write position for this segment
        int segIndex;
    };

    // ── Resume sidecar (multi-segment) ───────────────────────
    // Per-segment byte progress is journalled to "<localPath>.fsdownload" so an
    // interrupted multi-segment download resumes from where each segment stopped
    // instead of re-fetching the whole pre-allocated file. The file is written
    // from the single curl-multi event-loop thread (no locking needed) and
    // deleted on successful completion.
    struct SegmentState { int64_t start; int64_t end; int64_t cur; };
    static QString sidecarPath(const QString &localPath);
    bool writeSidecar(const QString &localPath, int64_t fileSize,
                      const std::vector<SegmentContext> &ctx) const;
    // Returns true and fills `out` when a valid resume journal exists for a
    // pre-allocated file of exactly `fileSize` bytes; false to start fresh.
    bool readSidecar(const QString &localPath, int64_t fileSize,
                     std::vector<SegmentState> &out) const;
    static size_t segmentWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static int segmentProgressCallback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);

    TransferTask m_task;
    QString m_proxyUrl;   // libcurl CURLOPT_PROXY spec; empty = direct
    SpeedMeter m_meter;
    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_paused{false};
    FILE *m_file = nullptr;
    std::atomic<int64_t> m_resumeOffset{0};
    std::atomic<int64_t> m_totalBytes{0};

    // Multi-segment progress aggregation
    std::vector<int64_t> m_segBytes;  // per-segment absolute downloaded bytes
    std::atomic<bool> m_isMultiSegment{false};

    // Set when a multi-segment attempt made real progress before failing: the
    // partial file + sidecar are kept for a later resume, so startDownload must
    // NOT fall back to single-segment (which would truncate the partial).
    bool m_skipSingleFallback = false;

    // Per-instance file-write mutex: serialises seek+write across HTTP/2 streams
    // that share the same FILE*. Must be per-instance so concurrent DownloadEngine
    // objects don't block each other (a global mutex would serialise all downloads).
    std::mutex m_fileWriteMutex;

    // Pause/resume via condition_variable so resume() wakes the CURL callback
    // immediately instead of waiting up to the msleep() quantum.
    std::condition_variable m_pauseCv;
    std::mutex              m_pauseMx;

    // Helper called from progress callbacks to honour pause/abort.
    // Returns true if the transfer should abort (abort flag set).
    bool waitIfPaused();
};

} // namespace fsnext
