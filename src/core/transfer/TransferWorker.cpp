#include "TransferWorker.h"
#include "DownloadEngine.h"
#include "UploadEngine.h"

namespace fsnext {

TransferWorker::TransferWorker(const TransferTask &task, QObject *parent)
    : QThread(parent)
    , m_task(task)
{
}

TransferWorker::~TransferWorker()
{
    // Ensure thread is stopped before destruction
    cancel();
    wait();
}

void TransferWorker::setEngine(DownloadEngine *engine)
{
    m_downloadEngine = engine;
    m_uploadEngine   = nullptr;
}

void TransferWorker::setEngine(UploadEngine *engine)
{
    m_uploadEngine   = engine;
    m_downloadEngine = nullptr;
}

void TransferWorker::pause()
{
    if (m_downloadEngine) m_downloadEngine->pause();
    if (m_uploadEngine)   m_uploadEngine->pause();
}

void TransferWorker::resume()
{
    if (m_downloadEngine) m_downloadEngine->resume();
    if (m_uploadEngine)   m_uploadEngine->resume();
}

void TransferWorker::cancel()
{
    if (m_downloadEngine) m_downloadEngine->cancel();
    if (m_uploadEngine)   m_uploadEngine->cancel();
}

// ---------------------------------------------------------------------------
// run() — executes on the worker thread
// ---------------------------------------------------------------------------

void TransferWorker::run()
{
    const QString id = m_task.id;

    if (m_downloadEngine) {
        // Wire signals from engine to this worker's signals (queued across thread boundary)
        connect(m_downloadEngine, &DownloadEngine::progressChanged,
                this, [this, id](int64_t bytes, int64_t total, double speed, const QString &eta) {
                    emit progressChanged(id, bytes, total, speed, eta);
                }, Qt::DirectConnection);

        connect(m_downloadEngine, &DownloadEngine::completed,
                this, [this, id]() {
                    emit finished(id);
                }, Qt::DirectConnection);

        connect(m_downloadEngine, &DownloadEngine::failed,
                this, [this, id](const QString &error) {
                    emit failed(id, error);
                }, Qt::DirectConnection);

        m_downloadEngine->startDownload(m_task);

    } else if (m_uploadEngine) {
        connect(m_uploadEngine, &UploadEngine::progressChanged,
                this, [this, id](int64_t bytes, int64_t total, double speed, const QString &eta) {
                    emit progressChanged(id, bytes, total, speed, eta);
                }, Qt::DirectConnection);

        connect(m_uploadEngine, &UploadEngine::completed,
                this, [this, id](const QString & /*linkcode*/) {
                    emit finished(id);
                }, Qt::DirectConnection);

        connect(m_uploadEngine, &UploadEngine::failed,
                this, [this, id](const QString &error) {
                    emit failed(id, error);
                }, Qt::DirectConnection);

        m_uploadEngine->startUpload(m_task);

    } else {
        emit failed(id, QStringLiteral("TransferWorker: no engine set"));
    }
}

} // namespace fsnext
