#include "BatchFileResolver.h"

#include "core/api/FshareApi.h"

#include <QDebug>
#include <QtConcurrent>

namespace fsnext {

BatchFileResolver::BatchFileResolver(FshareApi *api, QObject *parent)
    : QObject(parent)
    , m_api(api)
{
}

void BatchFileResolver::resolve(const QStringList &urls, int concurrency)
{
    if (urls.isEmpty()) {
        emit batchCompleted(0, 0, 0);
        return;
    }

    // Cancel previous batch if running
    if (m_running.load())
        cancel();

    QMutexLocker lock(&m_mutex);
    m_pendingUrls = urls;
    m_total       = urls.size();
    m_completed   = 0;
    m_succeeded   = 0;
    m_failed      = 0;
    m_concurrency = qBound(1, concurrency, 8);
    m_inFlight    = 0;
    m_cancelled.store(false);
    m_running.store(true);
    lock.unlock();

    qDebug() << "[BatchFileResolver] Starting batch:" << m_total
             << "urls, concurrency:" << m_concurrency;

    processQueue();
}

void BatchFileResolver::cancel()
{
    m_cancelled.store(true);
    qDebug() << "[BatchFileResolver] Cancelled";

    QMutexLocker lock(&m_mutex);
    m_pendingUrls.clear();
}

void BatchFileResolver::processQueue()
{
    QMutexLocker lock(&m_mutex);

    while (m_inFlight < m_concurrency && !m_pendingUrls.isEmpty()
           && !m_cancelled.load())
    {
        QString url = m_pendingUrls.takeFirst();
        m_inFlight++;

        FshareApi *api = m_api;

        QtConcurrent::run([this, api, url]() {
            if (m_cancelled.load()) {
                QMutexLocker lk(&m_mutex);
                m_inFlight--;
                m_completed++;
                m_failed++;
                lk.unlock();
                scheduleNext();
                return;
            }

            // Blocking API call on thread-pool thread
            auto result = api->getFileInfo(url);

            QMutexLocker lk(&m_mutex);
            m_inFlight--;
            m_completed++;

            if (result.isSuccess()) {
                m_succeeded++;
                FileItem item = result.data();
                int completed = m_completed;
                int total = m_total;
                lk.unlock();

                // Deliver on main thread
                QMetaObject::invokeMethod(this, [this, item, completed, total]() {
                    emit itemResolved(item);
                    emit batchProgress(completed, total);
                }, Qt::QueuedConnection);
            } else {
                m_failed++;
                QString error = result.error().message;
                int completed = m_completed;
                int total = m_total;
                lk.unlock();

                QMetaObject::invokeMethod(this, [this, url, error, completed, total]() {
                    emit itemFailed(url, error);
                    emit batchProgress(completed, total);
                }, Qt::QueuedConnection);
            }

            scheduleNext();
        });
    }
}

void BatchFileResolver::scheduleNext()
{
    QMetaObject::invokeMethod(this, [this]() {
        QMutexLocker lk(&m_mutex);
        bool done = (m_inFlight == 0 && m_pendingUrls.isEmpty());
        int total = m_total, succeeded = m_succeeded, failed = m_failed;
        lk.unlock();

        if (done) {
            m_running.store(false);
            qDebug() << "[BatchFileResolver] Batch complete:"
                     << succeeded << "ok," << failed << "failed";
            emit batchCompleted(total, succeeded, failed);
        } else if (!m_cancelled.load()) {
            processQueue();
        }
    }, Qt::QueuedConnection);
}

} // namespace fsnext
