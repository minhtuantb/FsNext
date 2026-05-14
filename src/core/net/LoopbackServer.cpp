// SPDX-License-Identifier: Proprietary
#include "LoopbackServer.h"

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

namespace fsnext {

// Page rendered in the user's browser after a redirect arrives. Centered
// card, brand colors, auto-close countdown — makes the short window the user
// sees look polished despite the raw localhost:port URL in the address bar.
static const char kSuccessHtml[] =
    "<!doctype html><html lang=\"vi\"><head><meta charset=\"utf-8\">"
    "<title>FsNext — Đăng nhập thành công</title>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<style>"
    "*{box-sizing:border-box}"
    "body{font-family:'Segoe UI Variable','Segoe UI',-apple-system,BlinkMacSystemFont,sans-serif;"
    "background:linear-gradient(135deg,#FAF8F7 0%,#F5F2F1 100%);color:#1A1010;"
    "display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;padding:24px}"
    ".card{text-align:center;padding:48px 56px;background:#fff;border-radius:14px;"
    "box-shadow:0 12px 40px rgba(232,0,29,.08),0 4px 12px rgba(0,0,0,.06);"
    "max-width:440px;width:100%}"
    ".logo{width:56px;height:56px;border-radius:14px;background:#E8001D;"
    "color:#fff;font-size:28px;font-weight:700;display:inline-flex;"
    "align-items:center;justify-content:center;margin-bottom:20px;"
    "box-shadow:0 6px 18px rgba(232,0,29,.25)}"
    ".check{width:28px;height:28px;border-radius:50%;background:#00C48C;"
    "color:#fff;display:inline-flex;align-items:center;justify-content:center;"
    "font-size:16px;margin-right:8px;vertical-align:middle}"
    "h1{color:#1A1010;margin:0 0 10px;font-size:20px;font-weight:600;letter-spacing:-.01em}"
    "p{margin:6px 0;color:#6B5555;font-size:14px;line-height:1.5}"
    ".hint{margin-top:20px;padding:12px 16px;background:#FAF8F7;border-radius:10px;"
    "font-size:12px;color:#9E8888}"
    "#count{font-weight:600;color:#E8001D}"
    "</style></head><body>"
    "<div class=\"card\">"
    "<div class=\"logo\">F</div>"
    "<h1><span class=\"check\">&#10003;</span>Đăng nhập thành công</h1>"
    "<p>Bạn có thể đóng cửa sổ này và quay lại FsNext.</p>"
    "<div class=\"hint\">Cửa sổ sẽ tự đóng sau <span id=\"count\">3</span> giây…</div>"
    "</div>"
    "<script>"
    "let n=3;const el=document.getElementById('count');"
    "const t=setInterval(()=>{n--;if(el)el.textContent=n;"
    "if(n<=0){clearInterval(t);try{window.close();}catch(e){}}},1000);"
    "</script>"
    "</body></html>";

static const char kErrorHtml[] =
    "<!doctype html><html lang=\"vi\"><head><meta charset=\"utf-8\">"
    "<title>FsNext — Đăng nhập thất bại</title>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<style>"
    "*{box-sizing:border-box}"
    "body{font-family:'Segoe UI Variable','Segoe UI',-apple-system,BlinkMacSystemFont,sans-serif;"
    "background:linear-gradient(135deg,#FAF8F7 0%,#F5F2F1 100%);color:#1A1010;"
    "display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;padding:24px}"
    ".card{text-align:center;padding:48px 56px;background:#fff;border-radius:14px;"
    "box-shadow:0 12px 40px rgba(232,0,29,.08),0 4px 12px rgba(0,0,0,.06);"
    "max-width:440px;width:100%}"
    ".logo{width:56px;height:56px;border-radius:14px;background:#E8001D;"
    "color:#fff;font-size:28px;font-weight:700;display:inline-flex;"
    "align-items:center;justify-content:center;margin-bottom:20px}"
    "h1{color:#E8001D;margin:0 0 10px;font-size:20px;font-weight:600}"
    "p{margin:6px 0;color:#6B5555;font-size:14px;line-height:1.5}"
    "</style></head><body>"
    "<div class=\"card\"><div class=\"logo\">F</div>"
    "<h1>Đăng nhập thất bại</h1>"
    "<p>Đã có lỗi khi kết nối tài khoản. Bạn có thể đóng cửa sổ này và thử lại trong ứng dụng.</p>"
    "</div></body></html>";

LoopbackServer::LoopbackServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &LoopbackServer::onNewConnection);
}

LoopbackServer::~LoopbackServer() = default;

bool LoopbackServer::start(int timeoutSec, int preferredPort)
{
    // Bind to 127.0.0.1 (NOT QHostAddress::Any) — the OAuth redirect goes
    // specifically to localhost and we don't want to expose this server on
    // the LAN. preferredPort == 0 → OS picks a free port.
    const quint16 port = preferredPort > 0 ? static_cast<quint16>(preferredPort) : 0;
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "[LoopbackServer] listen() failed on port" << port
                   << ":" << m_server->errorString();
        return false;
    }
    qDebug() << "[LoopbackServer] listening on 127.0.0.1:" << m_server->serverPort();

    // Safety net: if the user never completes the flow (closed browser, etc.),
    // emit timedOut() so the caller can clean up instead of leaking this object.
    QTimer::singleShot(timeoutSec * 1000, this, [this]() {
        if (m_handled) return;
        m_handled = true;
        qWarning() << "[LoopbackServer] timeout waiting for OAuth redirect";
        m_server->close();
        emit timedOut();
        deleteLater();
    });

    return true;
}

int LoopbackServer::port() const
{
    return m_server ? static_cast<int>(m_server->serverPort()) : 0;
}

void LoopbackServer::onNewConnection()
{
    QTcpSocket *sock = m_server->nextPendingConnection();
    if (!sock) return;

    // Parse a single HTTP request line ("GET /callback?code=...&state=... HTTP/1.1")
    // then close. We deliberately don't implement a full HTTP server — one request,
    // one response, done.
    connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
        if (m_handled) return;

        const QByteArray data = sock->readAll();
        // First line only; query string is what we want.
        const int nlIdx = data.indexOf('\n');
        const QByteArray firstLine = nlIdx > 0 ? data.left(nlIdx) : data;

        // Format: "GET /callback?code=X&state=Y HTTP/1.1"
        const QList<QByteArray> parts = firstLine.split(' ');
        if (parts.size() < 2) {
            respondAndClose(sock, QByteArray::fromRawData(kErrorHtml, int(sizeof(kErrorHtml) - 1)));
            return;
        }

        const QUrl url(QStringLiteral("http://localhost") + QString::fromUtf8(parts[1]));
        const QUrlQuery q(url);

        const QString code        = q.queryItemValue(QStringLiteral("code"));
        const QString state       = q.queryItemValue(QStringLiteral("state"));
        const QString error       = q.queryItemValue(QStringLiteral("error"));
        const QString errorDesc   = q.queryItemValue(QStringLiteral("error_description"));

        m_handled = true;
        // Write raw UTF-8 bytes directly — no QString round-trip which was
        // previously using fromLatin1 and double-encoding Vietnamese chars.
        if (error.isEmpty())
            respondAndClose(sock, QByteArray::fromRawData(kSuccessHtml, int(sizeof(kSuccessHtml) - 1)));
        else
            respondAndClose(sock, QByteArray::fromRawData(kErrorHtml, int(sizeof(kErrorHtml) - 1)));

        emitResult(code, state, error, errorDesc);
    });

    connect(sock, &QTcpSocket::disconnected, sock, &QTcpSocket::deleteLater);
}

void LoopbackServer::respondAndClose(QTcpSocket *sock, const QByteArray &utf8HtmlBody)
{
    QByteArray resp;
    resp.append("HTTP/1.1 200 OK\r\n");
    resp.append("Content-Type: text/html; charset=utf-8\r\n");
    resp.append("Content-Length: " + QByteArray::number(utf8HtmlBody.size()) + "\r\n");
    resp.append("Connection: close\r\n\r\n");
    resp.append(utf8HtmlBody);
    sock->write(resp);
    sock->flush();
    sock->disconnectFromHost();
}

void LoopbackServer::emitResult(const QString &code, const QString &state,
                                 const QString &error, const QString &errorDesc)
{
    m_server->close();
    if (!error.isEmpty()) {
        emit errorReceived(error, errorDesc);
    } else {
        emit codeReceived(code, state);
    }
    deleteLater();
}

} // namespace fsnext
