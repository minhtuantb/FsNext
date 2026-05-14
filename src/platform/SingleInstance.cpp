#include "SingleInstance.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>

namespace fsnext {

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
}

SingleInstance::~SingleInstance()
{
    if (m_server) {
        m_server->close();
    }
}

bool SingleInstance::tryLock(const QString &appName)
{
    m_appName = appName;

    // Try to connect to an existing server. If it succeeds, another instance is running.
    QLocalSocket probe;
    probe.connectToServer(appName);
    if (probe.waitForConnected(500)) {
        probe.disconnectFromServer();
        return false; // secondary instance
    }

    // No existing server — become the primary instance.
    m_server = new QLocalServer(this);
    QLocalServer::removeServer(appName); // remove stale socket if any
    if (!m_server->listen(appName)) {
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::onNewConnection);
    return true;
}

void SingleInstance::sendMessage(const QString &msg)
{
    QLocalSocket socket;
    socket.connectToServer(m_appName);
    if (!socket.waitForConnected(1000))
        return;

    QDataStream stream(&socket);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << msg;
    socket.flush();
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
}

void SingleInstance::onNewConnection()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket)
        return;

    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        QDataStream stream(socket);
        stream.setVersion(QDataStream::Qt_6_0);
        QString msg;
        stream >> msg;
        emit messageReceived(msg);
    });

    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

} // namespace fsnext
