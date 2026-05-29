#pragma once
#include "core/models/ApiResponse.h"
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QMutex>
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
    // Virtual so tests can subclass with a FakeHttpClient that returns canned
    // HttpResponses (e.g. RefreshTokenCoordinator single-flight tests) without
    // a real network round-trip. Production callers hold HttpClient* and are
    // unaffected by the extra vtable indirection.
    virtual ~HttpClient();

    // Configuration
    void setProxy(const QString &host, int port);
    // Set a complete proxy spec ("host:port" or "scheme://host:port"). Empty
    // string clears the proxy. Used by the central settings→proxy wiring so
    // system-proxy URLs (which already embed a scheme) pass through verbatim.
    void setProxyUrl(const QString &proxyUrl);
    void clearProxy();
    void setCaPath(const QString &path);
    void setDefaultHeader(const QString &key, const QString &value);
    void removeDefaultHeader(const QString &key);

    // Synchronous requests (virtual — see ~HttpClient note; tests override).
    virtual HttpResponse get(const QString &url, const QMap<QString, QString> &headers = {});
    virtual HttpResponse post(const QString &url, const QByteArray &body,
                              const QMap<QString, QString> &headers = {});

    // Cookie management
    virtual void setCookie(const QString &cookie);
    QString cookie() const;

private:
    CURL *createHandle();
    void applyHeaders(CURL *curl, const QMap<QString, QString> &extra);
    void applyProxy(CURL *curl);

    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t headerCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

    // ── Concurrency ──────────────────────────────────────────────────────
    // Every QString/QMap field below is touched by both worker threads
    // (executeAuthed lambdas running on QtConcurrent) AND by mutators that
    // can fire from the GUI thread or from RefreshTokenCoordinator after a
    // silent token rotation. QString is implicit-shared with non-atomic
    // d-pointer assignment — concurrent read/write races on a refcount
    // pointer swap, which has caused crashes in the field.
    //
    // We snapshot under the lock at the start of every request (see
    // applyHeaders / applyProxy / createHandle) so the network call itself
    // stays lock-free; only the brief copy is serialised.
    mutable QMutex m_mutex;

    // Full proxy spec passed straight to CURLOPT_PROXY ("host:port" or
    // "scheme://host:port"). Empty = no proxy. Snapshotted under m_mutex at
    // request time so a concurrent settings change can't tear the QString.
    QString m_proxyUrl;
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
