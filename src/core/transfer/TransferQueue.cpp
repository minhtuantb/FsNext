#include "TransferQueue.h"
#include "core/models/TransferState.h"

namespace fsnext {

TransferQueue::TransferQueue(QObject *parent)
    : QObject(parent)
{
}

// ---------------------------------------------------------------------------
// Queue management
// ---------------------------------------------------------------------------

void TransferQueue::enqueue(const TransferTask &task)
{
    m_queue.append(task);
    tryDispatch();
}

TransferTask TransferQueue::dequeue()
{
    if (m_queue.isEmpty()) return {};
    return m_queue.takeFirst();
}

void TransferQueue::remove(const QString &id)
{
    for (int i = 0; i < m_queue.size(); ++i) {
        if (m_queue[i].id == id) {
            const bool wasActive = (m_queue[i].state == TransferState::Active ||
                                    m_queue[i].state == TransferState::Paused);
            m_queue.remove(i);
            if (wasActive && m_activeCount > 0) {
                --m_activeCount;
            }
            tryDispatch();
            return;
        }
    }
}

void TransferQueue::moveUp(const QString &id)
{
    for (int i = 1; i < m_queue.size(); ++i) {
        if (m_queue[i].id == id) {
            m_queue.swapItemsAt(i, i - 1);
            return;
        }
    }
}

void TransferQueue::moveDown(const QString &id)
{
    for (int i = 0; i < m_queue.size() - 1; ++i) {
        if (m_queue[i].id == id) {
            m_queue.swapItemsAt(i, i + 1);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Concurrency
// ---------------------------------------------------------------------------

void TransferQueue::setMaxConcurrent(int n)
{
    m_maxConcurrent = qMax(1, n);
    tryDispatch();
}

int TransferQueue::maxConcurrent() const
{
    return m_maxConcurrent;
}

// ---------------------------------------------------------------------------
// Counts
// ---------------------------------------------------------------------------

int TransferQueue::activeCount() const
{
    return m_activeCount;
}

int TransferQueue::queueCount() const
{
    return m_queue.size();
}

// ---------------------------------------------------------------------------
// Inspection
// ---------------------------------------------------------------------------

QVector<TransferTask> TransferQueue::allTasks() const
{
    return m_queue;
}

bool TransferQueue::contains(const QString &id) const
{
    for (const TransferTask &t : m_queue) {
        if (t.id == id) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Private: dispatch queued tasks up to the concurrency limit
// ---------------------------------------------------------------------------

void TransferQueue::tryDispatch()
{
    for (auto &task : m_queue) {
        if (m_activeCount >= m_maxConcurrent) break;

        if (task.state == TransferState::Queued) {
            task.state = (task.type == TransferType::Download)
                         ? TransferState::Active
                         : TransferState::Active;
            ++m_activeCount;
            emit taskStarted(task.id);
            // Actual worker launch is handled by the controller/service layer
        }
    }
}

} // namespace fsnext
