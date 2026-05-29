#include "PriorityScheduler.h"
#include <algorithm>

namespace fsnext {

bool PriorityScheduler::enqueue(const QString &id, TransferPriority prio)
{
    std::lock_guard<std::mutex> lk(m_mx);
    // O(1) dup-check via the side index — the old linear scan across all
    // four deques was the bottleneck when bulk-pasting (folder expander) or
    // bulk-resuming a session with thousands of queued items.
    if (m_indexed.contains(id)) return false;
    const int p = static_cast<int>(prio);
    m_queues[p].push_back(id);
    m_indexed.insert(id, p);
    return true;
}

std::optional<QString> PriorityScheduler::popFront(TransferPriority prio)
{
    std::lock_guard<std::mutex> lk(m_mx);
    auto &q = m_queues[static_cast<int>(prio)];
    if (q.empty()) return std::nullopt;
    QString id = std::move(q.front());
    q.pop_front();
    m_indexed.remove(id);
    return id;
}

bool PriorityScheduler::removeById(const QString &id)
{
    std::lock_guard<std::mutex> lk(m_mx);
    const auto it = m_indexed.constFind(id);
    if (it == m_indexed.cend()) return false;
    const int p = it.value();
    m_indexed.erase(it);
    auto &q = m_queues[p];
    const auto dit = std::find(q.begin(), q.end(), id);
    if (dit != q.end()) q.erase(dit);
    return true;
}

void PriorityScheduler::pendingCounts(int out[4]) const
{
    std::lock_guard<std::mutex> lk(m_mx);
    for (int p = 0; p < 4; ++p)
        out[p] = static_cast<int>(m_queues[p].size());
}

bool PriorityScheduler::isEmpty() const
{
    std::lock_guard<std::mutex> lk(m_mx);
    for (int p = 0; p < 4; ++p)
        if (!m_queues[p].empty()) return false;
    return true;
}

int PriorityScheduler::totalPending() const
{
    std::lock_guard<std::mutex> lk(m_mx);
    int t = 0;
    for (int p = 0; p < 4; ++p) t += static_cast<int>(m_queues[p].size());
    return t;
}

} // namespace fsnext
