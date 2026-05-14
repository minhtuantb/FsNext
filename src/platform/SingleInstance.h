#pragma once

#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

namespace fsnext {

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance() override;

    // Returns true if this process acquired the lock (is the primary instance).
    // Returns false if another instance is already running.
    bool tryLock(const QString &appName);

    // Send a message to the already-running primary instance.
    void sendMessage(const QString &msg);

signals:
    void messageReceived(const QString &message);

private slots:
    void onNewConnection();

private:
    QLocalServer *m_server = nullptr;
    QString       m_appName;
};

} // namespace fsnext
