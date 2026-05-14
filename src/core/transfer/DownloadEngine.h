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

    // Multi-segment context (per-segment state)
    struct SegmentContext {
        DownloadEngine *engine;
        FILE *file;          // shared file pointer (seek + write)
        int64_t writeOffset; // current write position for this segment
        int segIndex;
    };
    static size_t segmentWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static int segmentProgressCallback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);

    TransferTask m_task;
    SpeedMeter m_meter;
    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_paused{false};
    FILE *m_file = nullptr;
    std::atomic<int64_t> m_resumeOffset{0};
    std::atomic<int64_t> m_totalBytes{0};

    // Multi-segment progress aggregation
    std::vector<int64_t> m_segBytes;  // per-segment current bytes
    std::atomic<bool> m_isMultiSegment{false};

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
