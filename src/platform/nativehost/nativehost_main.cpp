// SPDX-License-Identifier: Proprietary
// fsharenativeapp.exe — tiny Chrome native-messaging host.
//
// Lifecycle (Chrome ↔ host):
//   1. Chrome (extension popup) calls chrome.runtime.sendNativeMessage(
//          "com.fshare.tool", { url: "..." })
//   2. Chrome spawns this binary, pipes JSON over stdin / stdout using the
//      4-byte little-endian length prefix protocol.
//   3. We read one message, forward the URL to the running FsNext.exe via the
//      single-instance QLocalSocket, write a tiny ACK back to stdout, exit.
//
// Why a separate binary:
//   • Cold-starting full FsNext.exe for every Chrome message is ~2 s and
//     spawns Qt's renderer; users pasting 5 links would flood the desktop.
//   • Native-messaging hosts run with stdin/stdout in BINARY mode under
//     Chrome's accounting, which is awkward for a GUI process.
//
// Wire-up at install time:
//   manifest.json (this folder) is registered at
//     HKCU\Software\Google\Chrome\NativeMessagingHosts\com.fshare.tool
//   pointing at the absolute path of fsharenativeapp.exe (set by the
//   installer / scripts/register_native_host.bat).

#include <QCoreApplication>
#include <QDataStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QtEndian>
#include <cstdio>
#include <cstring>

#ifdef Q_OS_WIN
#  include <fcntl.h>
#  include <io.h>
#  include <stdio.h>
#endif

namespace {

constexpr char kSingleInstanceServerName[] = "fshare.singleinstance";
constexpr int  kMaxMessageBytes            = 1 << 20;       // 1 MiB hard cap

// Set stdin / stdout to binary so Chrome's framing isn't corrupted by
// CRLF translation on Windows.
void enterBinaryStdMode()
{
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
}

// Read one Chrome-framed message: 4-byte LE length + payload.  Returns the
// payload; empty QByteArray on EOF / framing error.
QByteArray readOne()
{
    quint32 len = 0;
    if (std::fread(&len, sizeof(len), 1, stdin) != 1) return {};
    len = qFromLittleEndian(len);
    if (len == 0 || len > kMaxMessageBytes) return {};
    QByteArray buf(static_cast<int>(len), 0);
    if (std::fread(buf.data(), 1, len, stdin) != len) return {};
    return buf;
}

// Write one Chrome-framed message back.  Used for an ACK so the extension
// can show "sent to Fshare" feedback.
void writeOne(const QByteArray &payload)
{
    const quint32 len = qToLittleEndian(static_cast<quint32>(payload.size()));
    std::fwrite(&len, sizeof(len), 1, stdout);
    std::fwrite(payload.constData(), 1, payload.size(), stdout);
    std::fflush(stdout);
}

bool forwardToFsNext(const QString &url)
{
    QLocalSocket sock;
    sock.connectToServer(QString::fromLatin1(kSingleInstanceServerName));
    if (!sock.waitForConnected(800)) return false;
    // SingleInstance::onNewConnection on the receiving side decodes one
    // QString via QDataStream (version Qt_6_0).  Match the framing exactly —
    // sending plain UTF-8 bytes makes the receiver read the first 4 bytes as
    // a string length prefix and get garbage.
    QDataStream stream(&sock);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << url;
    sock.flush();
    sock.waitForBytesWritten(800);
    sock.disconnectFromServer();
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    enterBinaryStdMode();

    const QByteArray raw = readOne();
    if (raw.isEmpty()) return 1;

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) {
        writeOne(R"({"ok":false,"error":"bad-json"})");
        return 2;
    }
    const QJsonObject obj = doc.object();
    const QString url = obj.value(QStringLiteral("url")).toString();
    if (url.isEmpty()) {
        writeOne(R"({"ok":false,"error":"missing-url"})");
        return 3;
    }

    const bool delivered = forwardToFsNext(url);
    QJsonObject ack;
    ack[QStringLiteral("ok")]        = delivered;
    ack[QStringLiteral("delivered")] = delivered;
    if (!delivered)
        ack[QStringLiteral("error")] = QStringLiteral("fsnext-not-running");
    writeOne(QJsonDocument(ack).toJson(QJsonDocument::Compact));
    return delivered ? 0 : 4;
}
