#pragma once
#include "core/models/ApiResponse.h"
#include <QString>
#include <QByteArray>
#include <QMap>
#include <functional>
#include <memory>

struct curl_slist;
typedef void CURL;
typedef void CURLSH;

namespace fsnext {

struct HttpResponse {
    int statusCode = 0;
    QByteArray body;
    QMap<QString, QString> headers;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Configuration
    void setProxy(const QString &host, int port);
    void clearProxy();
    void setCaPath(const QString &path);
    void setDefaultHeader(const QString &key, const QString &value);
    void removeDefaultHeader(const QString &key);

    // Synchronous requests
    HttpResponse get(const QString &url, const QMap<QString, QString> &headers = {});
    HttpResponse post(const QString &url, const QByteArray &body,
                      const QMap<QString, QString> &headers = {});

    // Cookie management
    void setCookie(const QString &cookie);
    QString cookie() const;

private:
    CURL *createHandle();
    void applyHeaders(CURL *curl, const QMap<QString, QString> &extra);
    void applyProxy(CURL *curl);

    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

    QString m_proxyHost;
    int m_proxyPort = 0;
    QString m_caPath;
    QString m_cookie;
    QMap<QString, QString> m_defaultHeaders;

    // CURL share handle — reuses DNS cache, SSL session cache, and the cookie
    // jar across every easy handle this client creates.  TLS handshake cost
    // for the second+ call to the same host drops from ~200 ms to ~20 ms
    // (resumed session) on Windows.  See ADR 003 D8.
    //
    // Owned for the lifetime of HttpClient.  Locking callbacks are no-ops
    // because we only ever call CURL from threads that have their own easy
    // handle; the share itself is touched on connection completion.
    CURLSH *m_share = nullptr;
};

} // namespace fsnext
