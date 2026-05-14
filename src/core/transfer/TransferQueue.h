#pragma once
#include "core/models/TransferTask.h"
#include <QObject>
#include <QVector>
#include <QString>

namespace fsnext {

class TransferQueue : public QObject {
    Q_OBJECT

public:
    explicit TransferQueue(QObject *parent = nullptr);
    ~TransferQueue() override = default;

    // Queue management
    void enqueue(const TransferTask &task);
    TransferTask dequeue();            // removes and returns front item
    void remove(const QString &id);
    void moveUp(const QString &id);    // swap with previous item
    void moveDown(const QString &id);  // swap with next item

    // Concurrency
    void setMaxConcurrent(int n);
    int  maxConcurrent() const;

    // Counts
    int activeCount() const;
    int queueCount()  const;

    // Inspection
    QVector<TransferTask> allTasks() const;
    bool contains(const QString &id) const;

signals:
    void taskStarted(const QString &taskId);
    void taskFinished(const QString &taskId);

private:
    void tryDispatch();  // start tasks up to m_maxConcurrent

    QVector<TransferTask> m_queue;
    int m_maxConcurrent = 2;
    int m_activeCount   = 0;
};

} // namespace fsnext
