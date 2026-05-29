#include "SingleInstance.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QPointer>
#include <QDebug>

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

    // Wrap the raw pointer in QPointer so the readyRead lambda can detect a
    // socket that's already been deleteLater()'d via the disconnected
    // signal — even if Qt queued readyRead first, between dispatch and
    // execution the socket can become invalid (browser closes the tab
    // mid-write, OS reaps the connection, etc.).
    QPointer<QLocalSocket> sockGuard(socket);

    connect(socket, &QLocalSocket::readyRead, this, [this, sockGuard]() {
        if (!sockGuard) return;
        QDataStream stream(sockGuard.data());
        stream.setVersion(QDataStream::Qt_6_0);
        QString msg;
        stream >> msg;
        // Reject malformed/truncated framing.  Without this guard,
        // QDataStream silently leaves msg empty and we'd emit
        // messageReceived("") — which Main.qml maps to the "open download
        // dialog with no link" path, surprising the user.  Tear the socket
        // down so a buggy / hostile peer can't keep us busy.
        if (stream.status() != QDataStream::Ok) {
            qWarning() << "[SingleInstance] malformed framing on local socket — closing";
            sockGuard->disconnectFromServer();
            sockGuard->deleteLater();
            return;
        }
        emit messageReceived(msg);
    });

    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

} // namespace fsnext
