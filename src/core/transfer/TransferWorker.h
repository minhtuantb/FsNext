#pragma once
#include "core/models/TransferTask.h"
#include <QThread>
#include <QString>
#include <cstdint>

namespace fsnext {

class DownloadEngine;
class UploadEngine;

class TransferWorker : public QThread {
    Q_OBJECT

public:
    explicit TransferWorker(const TransferTask &task, QObject *parent = nullptr);
    ~TransferWorker() override;

    // Call before start(). Ownership is NOT transferred.
    void setEngine(DownloadEngine *engine);
    void setEngine(UploadEngine   *engine);

    void pause();
    void resume();
    void cancel();

signals:
    void progressChanged(const QString &taskId, int64_t bytes, int64_t totalBytes, double speed, const QString &eta);
    void finished(const QString &taskId);
    void failed(const QString &taskId, const QString &error);

protected:
    void run() override;

private:
    TransferTask    m_task;
    DownloadEngine *m_downloadEngine = nullptr;
    UploadEngine   *m_uploadEngine   = nullptr;
};

} // namespace fsnext
