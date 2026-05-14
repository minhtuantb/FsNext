#pragma once

#include "SpeedMeter.h"
#include "core/models/TransferTask.h"
#include <QObject>
#include <QString>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace fsnext {

class UploadEngine : public QObject
{
    Q_OBJECT

public:
    explicit UploadEngine(QObject *parent = nullptr);
    ~UploadEngine() override;

    void startUpload(const TransferTask &task);
    void pause();
    void resume();
    void cancel();

signals:
    void progressChanged(int64_t bytesUploaded, int64_t totalBytes, double speed, const QString &eta);
    void completed(const QString &linkcode);
    void failed(const QString &error);
    // Server invalidated the upload session mid-transfer (INVALID_UPLOAD_SESSION).
    // TransferService handles this by re-creating the session and re-queuing.
    void sessionExpired();

private:
    // Per-chunk read context
    struct ChunkContext {
        UploadEngine *engine;
        FILE *file;
        int64_t bytesRemaining;  // bytes remaining IN THIS CHUNK

        // Throttle (sync-only): rolling-window token bucket. windowStart is
        // zero-initialized so the default represents "no window started yet".
        std::chrono::steady_clock::time_point windowStart{};
        int64_t bytesThisWindow = 0;
    };

    // Query how many bytes the server has already received for this upload session.
    // Returns the byte offset to resume from (0 = start over).
    int64_t queryResumeOffset(const QString &url, int64_t totalSize);

    static size_t chunkReadCallback(char *buffer, size_t size, size_t nmemb, void *userdata);
    static int chunkProgressCallback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);

    TransferTask m_task;
    SpeedMeter m_meter;
    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_paused{false};

    // Total bytes uploaded across all completed chunks (for progress aggregation)
    std::atomic<int64_t> m_totalUploaded{0};
};

} // namespace fsnext
