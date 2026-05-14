// SPDX-License-Identifier: Proprietary
#pragma once

#include <QObject>
#include <QTcpServer>
#include <QString>

namespace fsnext {

/// Minimal HTTP/1.1 server bound to 127.0.0.1 on an OS-chosen port, used to
/// capture the OAuth redirect in the RFC 8252 loopback flow.
///
/// Lifecycle:
///     auto *srv = new LoopbackServer(this);
///     if (!srv->start(/*timeoutSec=*/120)) return;
///     const int port = srv->port();                 // use in redirect_uri
///     // Build auth URL with redirect_uri=http://127.0.0.1:<port>/callback
///     QDesktopServices::openUrl(authUrl);
///     connect(srv, &LoopbackServer::codeReceived, ...);
///     connect(srv, &LoopbackServer::errorReceived, ...);
///     connect(srv, &LoopbackServer::timedOut,     ...);
///
/// The server closes itself after the first complete request. It owns its own
/// QTcpServer and will schedule itself for deletion via deleteLater() once the
/// result is emitted — callers don't need to manage its lifetime beyond
/// parenting it for automatic cleanup on shutdown.
class LoopbackServer : public QObject
{
    Q_OBJECT

public:
    explicit LoopbackServer(QObject *parent = nullptr);
    ~LoopbackServer() override;

    /// Start listening on 127.0.0.1. With `preferredPort == 0` the OS picks a
    /// free port (RFC 8252 recommendation). With a non-zero value the server
    /// binds that exact port — required by providers that exact-match the
    /// redirect URI (Facebook, enterprise SSO). Returns false if the fixed
    /// port is already in use.
    /// @param timeoutSec Abort and emit timedOut() after this many seconds.
    /// @return true on successful bind.
    bool start(int timeoutSec = 120, int preferredPort = 0);

    /// Actual port assigned by the OS. Undefined before start() succeeds.
    int port() const;

signals:
    /// Emitted when the browser redirects with ?code=&state=... query params.
    /// `state` is returned verbatim for the caller to validate against the
    /// value sent in the authorization URL (CSRF protection).
    void codeReceived(const QString &code, const QString &state);

    /// Emitted when the browser redirects with ?error=... instead of ?code=.
    void errorReceived(const QString &error, const QString &description);

    /// Emitted when no redirect arrives within the timeout.
    void timedOut();

private slots:
    void onNewConnection();

private:
    // Takes raw UTF-8 bytes, NOT a QString. The HTML literals already hold
    // UTF-8 bytes (thanks to MSVC /utf-8); a QString round-trip would require
    // QString::fromUtf8 and was a previous source of double-encoded mojibake.
    void respondAndClose(class QTcpSocket *sock, const QByteArray &utf8HtmlBody);
    void emitResult(const QString &code, const QString &state,
                    const QString &error, const QString &errorDesc);

    QTcpServer *m_server = nullptr;
    bool        m_handled = false;
};

} // namespace fsnext
