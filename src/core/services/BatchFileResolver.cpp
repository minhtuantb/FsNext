#include "BatchFileResolver.h"

#include "core/api/FshareApi.h"

#include <QDebug>
#include <QPointer>
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
        // QPointer guard — defends against the resolver being destroyed while
        // a worker is still in the blocking getFileInfo call. The resolver is
        // app-lifetime in normal flow (owned by AppContext) but the defensive
        // guard costs nothing and makes "delete me right now" scenarios safe
        // (e.g. a future test harness, or a hard logout that recreates the
        // batch surface).
        QPointer<BatchFileResolver> guard(this);

        QtConcurrent::run([guard, api, url]() {
            if (!guard || guard->m_cancelled.load()) {
                if (auto *self = guard.data()) {
                    QMutexLocker lk(&self->m_mutex);
                    self->m_inFlight--;
                    self->m_completed++;
                    self->m_failed++;
                    lk.unlock();
                    self->scheduleNext();
                }
                return;
            }

            // Blocking API call on thread-pool thread. `api` is captured by
            // value (raw pointer is safe — AppContext owns FshareApi for app
            // lifetime; main.cpp's waitForDone(5000) drains every worker
            // before the api object is destroyed at exit).
            auto result = api->getFileInfo(url);

            // Re-check after the blocking call returned: the resolver may
            // have been destroyed while getFileInfo was outstanding.
            auto *self = guard.data();
            if (!self) return;

            QMutexLocker lk(&self->m_mutex);
            self->m_inFlight--;
            self->m_completed++;

            if (result.isSuccess()) {
                self->m_succeeded++;
                FileItem item = result.data();
                int completed = self->m_completed;
                int total = self->m_total;
                lk.unlock();

                // Deliver on main thread — invokeMethod silently no-ops if
                // the receiver is gone, but the inner lambda still copies the
                // guard so we don't dereference a dangling self there either.
                QMetaObject::invokeMethod(self, [guard, item, completed, total]() {
                    if (auto *s = guard.data()) {
                        emit s->itemResolved(item);
                        emit s->batchProgress(completed, total);
                    }
                }, Qt::QueuedConnection);
            } else {
                self->m_failed++;
                QString error = result.error().message;
                int completed = self->m_completed;
                int total = self->m_total;
                lk.unlock();

                QMetaObject::invokeMethod(self, [guard, url, error, completed, total]() {
                    if (auto *s = guard.data()) {
                        emit s->itemFailed(url, error);
                        emit s->batchProgress(completed, total);
                    }
                }, Qt::QueuedConnection);
            }

            self->scheduleNext();
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
