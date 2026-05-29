#pragma once

// PriorityScheduler — owns the pending-task queues for ONE TransferClass and
// answers "what is the next task of priority P that wants to run?"
//
// There is one PriorityScheduler per class (Download / Upload / Metadata).
// Within a class, four FIFO queues (Interactive / Normal / Background /
// Metadata-priority) are drained highest-first, subject to the floor quota
// enforced by BudgetManager.
//
// Thread-safety
// -------------
// All methods are thread-safe (internal mutex).  Lives inside the
// TransferOrchestrator thread and is poked by enqueue / release events.

#include "TransferPriority.h"
#include <QHash>
#include <QString>
#include <deque>
#include <mutex>
#include <optional>

namespace fsnext {

class PriorityScheduler {
public:
    PriorityScheduler() = default;

    // Add a task to its priority queue.  Duplicate ids (already queued in ANY
    // priority) are rejected and return false.
    bool enqueue(const QString &id, TransferPriority prio);

    // Pop and return the head of a specific priority queue.  nullopt if empty.
    // Called by TransferOrchestrator AFTER BudgetManager grants the slot.
    std::optional<QString> popFront(TransferPriority prio);

    // Remove a task by id, regardless of priority.  Used by cancelQueued().
    // Returns true if found-and-removed.
    bool removeById(const QString &id);

    // Per-priority pending counts (ordered Interactive / Normal / Background
    // / Metadata).  Fed to BudgetManager so floor enforcement knows which
    // lower-priority queues have work waiting.
    void pendingCounts(int out[4]) const;

    bool isEmpty() const;
    int  totalPending() const;

private:
    mutable std::mutex  m_mx;
    // Four FIFO queues, indexed by TransferPriority int value.
    std::deque<QString> m_queues[4];
    // Side index of every id currently queued, regardless of priority.
    // Maps id → priority of the queue it sits in.  Lets enqueue() do an
    // O(1) "already queued?" check instead of scanning all four deques
    // (O(N) per enqueue, O(N²) for a 1000-link paste).  removeById and
    // popFront keep this index in sync.
    QHash<QString, int> m_indexed;
};

} // namespace fsnext
