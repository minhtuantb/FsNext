#include "PriorityScheduler.h"
#include <algorithm>

namespace fsnext {

bool PriorityScheduler::enqueue(const QString &id, TransferPriority prio)
{
    std::lock_guard<std::mutex> lk(m_mx);
    // Duplicate across ALL priority queues
    for (int p = 0; p < 4; ++p) {
        for (const auto &q : m_queues[p]) {
            if (q == id) return false;
        }
    }
    m_queues[static_cast<int>(prio)].push_back(id);
    return true;
}

std::optional<QString> PriorityScheduler::popFront(TransferPriority prio)
{
    std::lock_guard<std::mutex> lk(m_mx);
    auto &q = m_queues[static_cast<int>(prio)];
    if (q.empty()) return std::nullopt;
    QString id = std::move(q.front());
    q.pop_front();
    return id;
}

bool PriorityScheduler::removeById(const QString &id)
{
    std::lock_guard<std::mutex> lk(m_mx);
    for (int p = 0; p < 4; ++p) {
        auto &q = m_queues[p];
        auto it = std::find(q.begin(), q.end(), id);
        if (it != q.end()) {
            q.erase(it);
            return true;
        }
    }
    return false;
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
