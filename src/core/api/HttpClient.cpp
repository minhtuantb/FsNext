#include "HttpClient.h"
#include <curl/curl.h>
#include <QDebug>
#include <QSet>
#include <QUrl>

namespace fsnext {

// Reject anything that isn't a plausible proxy host. We accept bare hostnames,
// IPv4, bracketed IPv6, or scheme-prefixed URLs for the proxy protocols libcurl
// supports (http, https, socks4[a], socks5[h]). Rejecting here stops a
// malicious/typoed setting from letting curl interpret the string as something
// else (e.g. a file:// URL or a shell-meta-containing host).
static bool isSafeProxyHost(const QString &host)
{
    if (host.isEmpty()) return false;
    if (host.size() > 253) return false;   // DNS hostname limit
    for (QChar c : host) {
        const ushort u = c.unicode();
        if (u < 0x20 || u == 0x7F) return false;   // control chars
        if (c.isSpace()) return false;
    }
    const int schemeSep = host.indexOf(QStringLiteral("://"));
    QString hostPart = host;
    if (schemeSep > 0) {
        const QString scheme = host.left(schemeSep).toLower();
        static const QSet<QString> kAllowedSchemes = {
            QStringLiteral("http"),  QStringLiteral("https"),
            QStringLiteral("socks4"), QStringLiteral("socks4a"),
            QStringLiteral("socks5"), QStringLiteral("socks5h"),
        };
        if (!kAllowedSchemes.contains(scheme)) return false;
        hostPart = host.mid(schemeSep + 3);
    }
    // strip optional :port so we don't reject it as invalid char run
    const int lastColon = hostPart.lastIndexOf(QLatin1Char(':'));
    if (lastColon > 0 && !hostPart.startsWith(QLatin1Char('['))) {
        hostPart = hostPart.left(lastColon);
    }
    return !hostPart.isEmpty();
}

HttpClient::HttpClient()
{
    m_defaultHeaders[QStringLiteral("Content-Type")] = QStringLiteral("application/json");

    // Connection-pool primitive: a CURLSH handle that backs the per-thread
    // easy handles.  We share DNS cache, TLS session cache, and the cookie
    // jar — connection reuse on top of all that happens automatically when
    // the easy handle stays alive between requests (engines do this; the
    // sync get/post path still creates a fresh handle but at least skips
    // re-resolving DNS and re-doing the full TLS dance).
    m_share = curl_share_init();
    if (m_share) {
        curl_share_setopt(m_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        curl_share_setopt(m_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
        curl_share_setopt(m_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
        curl_share_setopt(m_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    } else {
        qWarning() << "[HttpClient] curl_share_init failed — running without pooling";
    }
}

HttpClient::~HttpClient()
{
    if (m_share) {
        curl_share_cleanup(m_share);
        m_share = nullptr;
    }
}

void HttpClient::setProxy(const QString &host, int port)
{
    if (host.isEmpty() && port == 0) {
        clearProxy();
        return;
    }
    if (!isSafeProxyHost(host) || port < 1 || port > 65535) {
        qWarning() << "[HttpClient] Rejected invalid proxy config: host="
                   << host << "port=" << port << "— clearing proxy.";
        clearProxy();
        return;
    }
    m_proxyHost = host;
    m_proxyPort = port;
}

void HttpClient::clearProxy()
{
    m_proxyHost.clear();
    m_proxyPort = 0;
}

void HttpClient::setCaPath(const QString &path)
{
    m_caPath = path;
}

void HttpClient::setDefaultHeader(const QString &key, const QString &value)
{
    m_defaultHeaders[key] = value;
}

void HttpClient::removeDefaultHeader(const QString &key)
{
    m_defaultHeaders.remove(key);
}

void HttpClient::setCookie(const QString &cookie)
{
    m_cookie = cookie;
}

QString HttpClient::cookie() const
{
    return m_cookie;
}

// Upper bound for in-memory API response bodies (JSON metadata, OAuth userinfo,
// etc.). File downloads go through DownloadEngine::segmentWriteCallback which
// streams straight to disk — they never hit this callback. 32 MiB is ~100x
// what any Fshare JSON endpoint legitimately returns; anything larger means
// the server is hostile or misconfigured, and we abort instead of OOM'ing.
static constexpr size_t kMaxResponseBytes = 32 * 1024 * 1024;

size_t HttpClient::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *buffer = static_cast<QByteArray *>(userdata);
    const size_t totalSize = size * nmemb;
    // Returning anything other than totalSize signals CURL to abort with
    // CURLE_WRITE_ERROR — the surrounding get()/post() will surface that as
    // a failed HttpResponse rather than exhausting RAM.
    if (static_cast<size_t>(buffer->size()) + totalSize > kMaxResponseBytes) {
        qWarning() << "[HttpClient] Response body exceeded"
                   << kMaxResponseBytes << "bytes — aborting.";
        return 0;
    }
    buffer->append(ptr, static_cast<int>(totalSize));
    return totalSize;
}

size_t HttpClient::headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *headers = static_cast<QMap<QString, QString> *>(userdata);
    size_t totalSize = size * nmemb;
    QString line = QString::fromUtf8(ptr, static_cast<int>(totalSize)).trimmed();

    int colonIdx = line.indexOf(':');
    if (colonIdx > 0) {
        QString key = line.left(colonIdx).trimmed();
        QString value = line.mid(colonIdx + 1).trimmed();
        headers->insert(key, value);
    }
    return totalSize;
}

CURL *HttpClient::createHandle()
{
    CURL *curl = curl_easy_init();
    if (!curl) return nullptr;

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    // Reject TLS < 1.2. Fshare and all supporting OAuth providers (Google,
    // Facebook) serve TLS 1.2+ — if a downgrade MITM serves 1.0/1.1 we want
    // to fail the handshake rather than transmit secrets over weak crypto.
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    // User-Agent must appear in the Fshare server whitelist — unrecognized UAs
    // get 400 from nginx before the request reaches the app layer.
    // Paired with the 2026 app_key rotation in FshareApi.cpp.
    // Do NOT change without testing against the Fshare API gate.
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Fshare_Tool_2026");

    if (!m_caPath.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, m_caPath.toUtf8().constData());
    }

    // Plug into the shared cache so subsequent requests skip DNS lookup and
    // resume the TLS session on the next handshake.
    if (m_share) {
        curl_easy_setopt(curl, CURLOPT_SHARE, m_share);
    }

    applyProxy(curl);

    return curl;
}

void HttpClient::applyHeaders(CURL *curl, const QMap<QString, QString> &extra)
{
    struct curl_slist *headerList = nullptr;

    // Merge default + extra with extra winning on key collision. Critical for
    // OAuth token exchange: defaults include Content-Type: application/json,
    // but that endpoint needs application/x-www-form-urlencoded. The old code
    // appended BOTH headers via curl_slist_append — CURL sent both and Google
    // responded with 400 ("invalid_request: Invalid form-urlencoded input").
    QMap<QString, QString> merged = m_defaultHeaders;
    for (auto it = extra.cbegin(); it != extra.cend(); ++it) {
        merged[it.key()] = it.value();   // override
    }
    for (auto it = merged.cbegin(); it != merged.cend(); ++it) {
        QString header = it.key() + QStringLiteral(": ") + it.value();
        headerList = curl_slist_append(headerList, header.toUtf8().constData());
    }

    // Cookie
    if (!m_cookie.isEmpty()) {
        QString cookieHeader = QStringLiteral("Cookie: ") + m_cookie;
        headerList = curl_slist_append(headerList, cookieHeader.toUtf8().constData());
    }

    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
}

void HttpClient::applyProxy(CURL *curl)
{
    if (!m_proxyHost.isEmpty() && m_proxyPort > 0) {
        QString proxyUrl = m_proxyHost + QStringLiteral(":") + QString::number(m_proxyPort);
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyUrl.toUtf8().constData());
    }
}

HttpResponse HttpClient::get(const QString &url, const QMap<QString, QString> &headers)
{
    HttpResponse response;
    CURL *curl = createHandle();
    if (!curl) {
        response.statusCode = -1;
        return response;
    }

    QByteArray urlBytes = url.toUtf8();
    curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);

    applyHeaders(curl, headers);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
    } else {
        qWarning() << "HTTP GET failed:" << curl_easy_strerror(res) << "url:" << url;
        response.statusCode = -static_cast<int>(res);
    }

    curl_easy_cleanup(curl);
    return response;
}

HttpResponse HttpClient::post(const QString &url, const QByteArray &body,
                               const QMap<QString, QString> &headers)
{
    qDebug().noquote() << "[HTTP] POST" << url << "body-size:" << body.size();
    HttpResponse response;
    CURL *curl = createHandle();
    if (!curl) {
        response.statusCode = -1;
        return response;
    }

    QByteArray urlBytes = url.toUtf8();
    curl_easy_setopt(curl, CURLOPT_URL, urlBytes.constData());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.constData());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);

    applyHeaders(curl, headers);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        qDebug().noquote() << "[HTTP] POST" << url << "→" << httpCode
                           << "(" << response.body.size() << "bytes)";
    } else {
        qWarning() << "[HTTP] POST failed:" << curl_easy_strerror(res) << "url:" << url;
        response.statusCode = -static_cast<int>(res);
    }

    curl_easy_cleanup(curl);
    return response;
}

} // namespace fsnext
