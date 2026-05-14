#include "FshareApi.h"
#include "core/models/AppError.h"

#include <json/json.h>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

namespace fsnext {

static const QString API_BASE = QStringLiteral("https://api.fshare.vn");

// App key — XOR-encoded with the last byte of the array (0x5c).
// To re-encode a new key, use FsNext/scripts/encode_appkey.py (reproducible).
// Current plaintext (2026 rotation): "dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt"
static QString getAppKey() {
    const char encrypted_key[] = {
        0x38, 0x11, 0x32, 0x2d, 0x11, 0x11, 0x06, 0x11, 0x09, 0x32,
        0x12, 0x69, 0x05, 0x2c, 0x2a, 0x17, 0x19, 0x12, 0x3d, 0x19,
        0x34, 0x38, 0x0d, 0x0d, 0x69, 0x36, 0x24, 0x18, 0x2d, 0x38,
        0x38, 0x28,
        0x5c
    };
    const int size = sizeof(encrypted_key) - 1;
    std::string result(size, '\0');
    for (int i = 0; i < size; i++)
        result[i] = char(encrypted_key[i] ^ encrypted_key[size]);
    return QString::fromStdString(result);
}

// ── Helpers ──────────────────────────────────────────────

static QByteArray jsonBody(const QJsonObject &obj) {
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

static Json::Value parseJson(const QByteArray &data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream stream(data.toStdString());
    Json::parseFromStream(builder, stream, &root, &errs);
    return root;
}

static AppError checkApiResponse(const HttpResponse &resp, const Json::Value &root) {
    if (resp.statusCode <= 0)
        return AppError::network(resp.statusCode, QStringLiteral("Connection failed"));

    // Fshare API convention: the JSON body's `code` field is the authoritative
    // error indicator. The HTTP status often mirrors it (login fail returns
    // both HTTP 405 AND {"code":405}), so trust the JSON first when present.
    //
    // Known codes (verified against 2026 API):
    //   • 200      — success
    //   • 302      — redirect (download URL handoff)
    //   • 201/202 — session expired / re-login required
    //   • 405      — "Authenticate fail!" (wrong credentials or bad app_key)
    //   • 502       — bad gateway (retryable server-side)
    //   • 400       — nginx whitelist rejection (UA not recognized)
    if (root.isObject() && root.isMember("code")) {
        int code = root.get("code", 200).asInt();
        if (code == 200) return AppError::none();

        QString msg = QString::fromStdString(root.get("msg", "").asString());

        // 201/202 — session expired
        if (code == 201 || code == 202) {
            if (msg.isEmpty())
                msg = QStringLiteral("Phiên đăng nhập hết hạn. Vui lòng đăng nhập lại.");
            return AppError::auth(code, msg);
        }
        // 405 — bad credentials (wrong email/password)
        if (code == 405) {
            if (msg.isEmpty())
                msg = QStringLiteral("Email hoặc mật khẩu không đúng.");
            return AppError::auth(code, msg);
        }
        // Any other non-200 JSON code
        if (msg.isEmpty())
            msg = QStringLiteral("Server error (code %1)").arg(code);
        return AppError::server(code, msg);
    }

    // No JSON body (non-JSON response, e.g. nginx HTML error page) — use HTTP status
    if (resp.statusCode == 201 || resp.statusCode == 202)
        return AppError::auth(resp.statusCode,
            QStringLiteral("Phiên đăng nhập hết hạn. Vui lòng đăng nhập lại."));
    if (resp.statusCode != 200 && resp.statusCode != 302)
        return AppError::server(resp.statusCode, QStringLiteral("HTTP %1").arg(resp.statusCode));

    return AppError::none();
}

// Safe string-to-int conversion that handles both JSON numbers and strings
static int64_t safeInt64(const Json::Value &v, int64_t defaultValue = 0) {
    if (v.isNull()) return defaultValue;
    if (v.isIntegral()) return v.asInt64();
    if (v.isString()) {
        const auto s = v.asString();
        if (s.empty()) return defaultValue;
        try { return std::stoll(s); } catch (...) { return defaultValue; }
    }
    return defaultValue;
}

static int safeInt(const Json::Value &v, int defaultValue = 0) {
    return static_cast<int>(safeInt64(v, defaultValue));
}

static bool safeBool(const Json::Value &v, bool defaultValue = false) {
    if (v.isNull()) return defaultValue;
    if (v.isBool()) return v.asBool();
    if (v.isIntegral()) return v.asInt() != 0;
    if (v.isString()) {
        const auto s = v.asString();
        return s == "1" || s == "true";
    }
    return defaultValue;
}

static User parseUser(const Json::Value &root) {
    User u;
    u.id = QString::fromStdString(root.get("id", "").asString());
    u.email = QString::fromStdString(root.get("email", "").asString());
    u.name = QString::fromStdString(root.get("name", "").asString());
    u.level = safeInt(root.get("level", 0));
    u.expireVip = QString::fromStdString(root.get("expire_vip", "").asString());
    u.joinDate = QString::fromStdString(root.get("joindate", "").asString());
    u.traffic = safeInt64(root.get("traffic", 0));
    u.trafficUsed = safeInt64(root.get("traffic_used", 0));
    u.webspace = safeInt64(root.get("webspace", 0));
    u.webspaceUsed = safeInt64(root.get("webspace_used", 0));
    u.webspaceSecure = safeInt64(root.get("webspace_secure", 0));
    u.webspaceSecureUsed = safeInt64(root.get("webspace_secure_used", 0));
    int totalPoints = safeInt(root.get("totalpoints", 0));
    int amount = safeInt(root.get("amount", 0));
    u.totalPoints = totalPoints > 0 ? totalPoints : amount;
    u.dlTimeAvail = safeInt(root.get("dl_time_avail", 0));
    u.accountType = QString::fromStdString(root.get("account_type", "").asString());
    return u;
}

static FileItem parseFileItem(const Json::Value &item) {
    FileItem f;
    f.id = static_cast<uint64_t>(safeInt64(item.get("id", 0)));
    f.linkcode = QString::fromStdString(item.get("linkcode", "").asString());
    f.name = QString::fromStdString(item.get("name", "").asString());
    // Some responses use "type", others use "file_type". Fshare encodes both
    // as "0" = folder, "1" = file. Normalise to the literal strings
    // "folder" / "file" so the rest of the app (FileItem::isFolder(),
    // queryFiles' ORDER BY CASE, type filters, cached rows) can treat it
    // uniformly.
    const auto typeVal = item.get("type", "").asString();
    const auto fileTypeVal = item.get("file_type", "").asString();
    const QString raw = QString::fromStdString(typeVal.empty() ? fileTypeVal : typeVal);
    if (raw == QStringLiteral("0"))      f.type = QStringLiteral("folder");
    else if (raw == QStringLiteral("1")) f.type = QStringLiteral("file");
    else                                 f.type = raw;
    f.size = safeInt64(item.get("size", 0));
    f.path = QString::fromStdString(item.get("path", "").asString());
    f.secure = safeBool(item.get("secure", false));
    f.isPublic = safeBool(item.get("public", false));
    f.directlink = safeBool(item.get("directlink", false));
    f.deleted = safeBool(item.get("deleted", false));
    f.copied = safeBool(item.get("copied", false));
    f.shared = safeBool(item.get("shared", false));
    f.hashIndex = QString::fromStdString(item.get("hash_index", "").asString());
    f.ownerId = QString::fromStdString(item.get("owner_id", "").asString());
    f.parentId = QString::fromStdString(item.get("pid", "").asString());
    // Fshare uses "0" or "" interchangeably for the account root. Normalise to
    // empty string so cache queries keyed on parent_id='' match items the API
    // reports with pid="0".
    if (f.parentId == QStringLiteral("0"))
        f.parentId.clear();
    f.downloadCount = safeInt(item.get("downloadcount", 0));
    f.description = QString::fromStdString(item.get("description", "").asString());
    f.created = QString::fromStdString(item.get("created", "").asString());
    f.modified = QString::fromStdString(item.get("modified", "").asString());
    f.lastDownload = QString::fromStdString(item.get("lastdownload", "").asString());
    f.tIndex = QString::fromStdString(item.get("t_index", "").asString());
    auto pwd = item.get("pwd", "");
    f.hasPassword = pwd.isBool() ? pwd.asBool() : (!pwd.asString().empty() && pwd.asString() != "0");
    return f;
}

static Json::Value itemsArray(const QStringList &linkcodes) {
    Json::Value arr(Json::arrayValue);
    for (const auto &lc : linkcodes)
        arr.append(lc.toStdString());
    return arr;
}

// ── Constructor ──────────────────────────────────────────

FshareApi::FshareApi(HttpClient *http)
    : m_http(http)
    , m_appKey(getAppKey())
{
}

void FshareApi::setSession(const QString &token, const QString &cookie)
{
    m_token = token;
    if (m_http)
        m_http->setCookie(cookie);
}

// ── Auth ──────────────────────────────────────────────────

ApiResponse<Session> FshareApi::login(const QString &email, const QString &password)
{
    QJsonObject body;
    body[QStringLiteral("app_key")] = m_appKey;
    body[QStringLiteral("user_email")] = email;
    body[QStringLiteral("password")] = password;

    HttpResponse resp = m_http->post(API_BASE + QStringLiteral("/api/user/login"), jsonBody(body));
    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError())
        return ApiResponse<Session>::failure(err, resp.statusCode);

    Session session;
    session.token = QString::fromStdString(root.get("token", "").asString());
    session.sessionId = QString::fromStdString(root.get("session_id", "").asString());

    // Extract cookie from Set-Cookie header
    QString setCookie = resp.headers.value(QStringLiteral("Set-Cookie"));
    if (!setCookie.isEmpty()) {
        // Format: PHPSESSID=xxx; path=/; ...
        int semicolonIdx = setCookie.indexOf(';');
        QString phpSession = semicolonIdx > 0 ? setCookie.left(semicolonIdx) : setCookie;
        session.cookie = phpSession + QStringLiteral(";key=") + m_appKey;
    }

    setSession(session.token, session.cookie);
    return ApiResponse<Session>::success(session);
}

ApiResponse<Session> FshareApi::loginOauth(const QString &service,
                                           const QString &accessToken,
                                           const QString &email)
{
    // Fshare API contract (OAuth login):
    //   POST /api/user/oauth
    //   body: {app_key, service, access_token, user_email}
    //   → same Session response shape as /api/user/login
    QJsonObject body;
    body[QStringLiteral("app_key")]      = m_appKey;
    body[QStringLiteral("service")]      = service;
    body[QStringLiteral("access_token")] = accessToken;
    body[QStringLiteral("user_email")]   = email;

    HttpResponse resp = m_http->post(API_BASE + QStringLiteral("/api/user/oauth"), jsonBody(body));
    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError())
        return ApiResponse<Session>::failure(err, resp.statusCode);

    Session session;
    session.token     = QString::fromStdString(root.get("token", "").asString());
    session.sessionId = QString::fromStdString(root.get("session_id", "").asString());

    const QString setCookie = resp.headers.value(QStringLiteral("Set-Cookie"));
    if (!setCookie.isEmpty()) {
        const int semicolonIdx = setCookie.indexOf(';');
        const QString cookiePair = semicolonIdx > 0 ? setCookie.left(semicolonIdx) : setCookie;
        session.cookie = cookiePair + QStringLiteral(";key=") + m_appKey;
    }

    // Fill in the email we already know — Fshare often returns it in userinfo,
    // but having it up-front lets the UI show the account immediately.
    session.user.email = email;

    setSession(session.token, session.cookie);
    return ApiResponse<Session>::success(session);
}

ApiResponse<void> FshareApi::logout()
{
    HttpResponse resp = m_http->get(API_BASE + QStringLiteral("/api/user/logout"));
    m_token.clear();
    m_http->setCookie({});
    return ApiResponse<void>::success(resp.statusCode);
}

ApiResponse<User> FshareApi::getUserInfo()
{
    HttpResponse resp = m_http->get(API_BASE + QStringLiteral("/api/user/get"));
    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError())
        return ApiResponse<User>::failure(err, resp.statusCode);

    return ApiResponse<User>::success(parseUser(root));
}

// ── File listing & search ──────────────────────────────────

ApiResponse<QVector<FileItem>> FshareApi::listFiles(const QString &folderUrl, int page, int pageSize)
{
    // Root listing: empty URL means "account root" — getFolderListPaging requires
    // a URL, so fall back to /api/fileops/list/home (same endpoint family used
    // by listFolders, but targeted at root with dirOnly=0 so we get files
    // AND folders at level 1). The `/home` suffix is required by the Fshare
    // API to address the account root; without it the endpoint returns empty.
    if (folderUrl.isEmpty()) {
        const QString url = API_BASE + QStringLiteral("/api/fileops/list/home")
                          + QStringLiteral("?pageIndex=") + QString::number(page)
                          + QStringLiteral("&dirOnly=0")
                          + QStringLiteral("&limit=") + QString::number(pageSize);
        qInfo().noquote() << "[FshareApi] list/home GET url=" << url;
        HttpResponse resp = m_http->get(url);
        qInfo().noquote() << "[FshareApi] list/home status=" << resp.statusCode
                          << "bodyBytes=" << resp.body.size()
                          << "body(first 500)=" << QString::fromUtf8(resp.body.left(500));
        if (resp.statusCode <= 0)
            return ApiResponse<QVector<FileItem>>::failure(AppError::network(resp.statusCode, {}));
        if (resp.statusCode != 200)
            return ApiResponse<QVector<FileItem>>::failure(AppError::server(resp.statusCode, {}));

        Json::Value root = parseJson(resp.body);
        if (root.isObject() && root.isMember("code")) {
            int code = root.get("code", 200).asInt();
            if (code != 200) {
                QString msg = QString::fromStdString(root.get("msg", "").asString());
                return ApiResponse<QVector<FileItem>>::failure(AppError::server(code, msg));
            }
        }
        QVector<FileItem> items;
        if (root.isArray()) {
            for (const auto &item : root)
                items.append(parseFileItem(item));
        }
        qInfo().noquote() << "[FshareApi] list/home parsed items=" << items.size();
        return ApiResponse<QVector<FileItem>>::success(items);
    }

    // Fshare's getFolderListPaging expects a full folder URL in the "url" field,
    // not a bare linkcode. Passing a bare linkcode makes the server return 200
    // with an empty body, which silently shows folders as empty. Normalise any
    // caller-supplied linkcode to the canonical fshare.vn folder URL.
    QString normalisedUrl = folderUrl;
    if (!normalisedUrl.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        && !normalisedUrl.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        normalisedUrl = QStringLiteral("https://www.fshare.vn/folder/") + normalisedUrl;
    }

    Json::Value body;
    body["token"] = m_token.toStdString();
    body["url"] = normalisedUrl.toStdString();
    body["page_index"] = page;
    body["page_size"] = pageSize;

    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, body);
    HttpResponse resp = m_http->post(
        API_BASE + QStringLiteral("/api/fileops/getFolderListPaging"),
        QByteArray::fromStdString(jsonStr));

    qInfo().noquote() << "[FshareApi] getFolderListPaging url=" << normalisedUrl
                      << "page=" << page << "status=" << resp.statusCode
                      << "bodyBytes=" << resp.body.size();

    if (resp.statusCode <= 0)
        return ApiResponse<QVector<FileItem>>::failure(AppError::network(resp.statusCode, {}));
    if (resp.statusCode != 200)
        return ApiResponse<QVector<FileItem>>::failure(AppError::server(resp.statusCode, {}));

    if (resp.body.isEmpty()) {
        // Genuinely empty body on 200 — treat as empty folder.
        return ApiResponse<QVector<FileItem>>::success({});
    }

    qInfo().noquote() << "[FshareApi] getFolderListPaging body(first 500 chars)="
                      << QString::fromUtf8(resp.body.left(500));

    Json::Value root = parseJson(resp.body);

    // If response is an object with error code, surface it
    if (root.isObject() && root.isMember("code")) {
        int code = root.get("code", 200).asInt();
        if (code != 200) {
            QString msg = QString::fromStdString(root.get("msg", "").asString());
            return ApiResponse<QVector<FileItem>>::failure(AppError::server(code, msg));
        }
    }

    QVector<FileItem> items;
    if (root.isArray()) {
        for (const auto &item : root)
            items.append(parseFileItem(item));
    }
    return ApiResponse<QVector<FileItem>>::success(items);
}

ApiResponse<QVector<FileItem>> FshareApi::listFolders(const QString &path)
{
    QString url = API_BASE + QStringLiteral("/api/fileops/list");
    if (!path.isEmpty())
        url += QStringLiteral("/") + path;
    url += QStringLiteral("?dirOnly=1&limit=1000&pageIndex=0");

    QVector<FileItem> allItems;
    int pageIndex = 0;

    while (true) {
        QString pageUrl = url;
        if (pageIndex > 0)
            pageUrl = API_BASE + QStringLiteral("/api/fileops/list")
                + (path.isEmpty() ? QString() : QStringLiteral("/") + path)
                + QStringLiteral("?dirOnly=1&limit=1000&pageIndex=") + QString::number(pageIndex);

        HttpResponse resp = m_http->get(pageUrl);
        if (resp.statusCode <= 0)
            return ApiResponse<QVector<FileItem>>::failure(AppError::network(resp.statusCode, {}));
        if (resp.statusCode == 502 && pageIndex == 0) {
            // Legacy has 502 retry logic for bad gateway — simple retry here
            QThread::msleep(500);
            resp = m_http->get(pageUrl);
        }
        if (resp.statusCode != 200)
            return ApiResponse<QVector<FileItem>>::failure(AppError::server(resp.statusCode, {}));

        Json::Value root = parseJson(resp.body);
        // Handle error responses that come as objects instead of arrays
        if (root.isObject() && root.isMember("code")) {
            int code = root.get("code", 200).asInt();
            if (code != 200) {
                QString msg = QString::fromStdString(root.get("msg", "").asString());
                return ApiResponse<QVector<FileItem>>::failure(AppError::server(code, msg));
            }
        }
        if (!root.isArray() || root.empty())
            break;

        for (const auto &item : root)
            allItems.append(parseFileItem(item));

        if (static_cast<int>(root.size()) < 1000)
            break;
        pageIndex++;
    }

    return ApiResponse<QVector<FileItem>>::success(allItems);
}

ApiResponse<FileItem> FshareApi::getFileInfo(const QString &url)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["url"] = url.toStdString();

    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, body);
    HttpResponse resp = m_http->post(
        API_BASE + QStringLiteral("/api/fileops/get"),
        QByteArray::fromStdString(jsonStr));

    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError())
        return ApiResponse<FileItem>::failure(err, resp.statusCode);

    return ApiResponse<FileItem>::success(parseFileItem(root));
}

ApiResponse<QVector<FileItem>> FshareApi::searchFiles(const QString &keyword, int page)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["keyword"] = keyword.toStdString();

    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, body);
    QString url = API_BASE + QStringLiteral("/api/fileops/search?pageIndex=") + QString::number(page);
    HttpResponse resp = m_http->post(url, QByteArray::fromStdString(jsonStr));

    if (resp.statusCode <= 0)
        return ApiResponse<QVector<FileItem>>::failure(AppError::network(resp.statusCode, {}));
    if (resp.statusCode != 200)
        return ApiResponse<QVector<FileItem>>::failure(AppError::server(resp.statusCode, {}));

    Json::Value root = parseJson(resp.body);

    if (root.isObject() && root.isMember("code")) {
        int code = root.get("code", 200).asInt();
        if (code != 200) {
            QString msg = QString::fromStdString(root.get("msg", "").asString());
            return ApiResponse<QVector<FileItem>>::failure(AppError::server(code, msg));
        }
    }

    QVector<FileItem> items;
    if (root.isArray()) {
        for (const auto &item : root)
            items.append(parseFileItem(item));
    }
    return ApiResponse<QVector<FileItem>>::success(items);
}

// ── File operations ──────────────────────────────────────

static ApiResponse<void> postSimple(HttpClient *http, const QString &endpoint,
                                     const Json::Value &body)
{
    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, body);
    HttpResponse resp = http->post(API_BASE + endpoint, QByteArray::fromStdString(jsonStr));
    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError()) {
        // Log the raw server body on failure — Fshare sometimes returns useful
        // details (e.g. "Folder name already exists") that we otherwise drop.
        qInfo().noquote() << "[FshareApi]" << endpoint
                          << "failed status=" << resp.statusCode
                          << "body=" << QString::fromUtf8(resp.body.left(500));
        return ApiResponse<void>::failure(err, resp.statusCode);
    }
    return ApiResponse<void>::success(resp.statusCode);
}

ApiResponse<void> FshareApi::renameFile(const QString &linkcode, const QString &newName)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["file"] = linkcode.toStdString();
    body["new_name"] = newName.toStdString();
    return postSimple(m_http, QStringLiteral("/api/fileops/rename"), body);
}

ApiResponse<void> FshareApi::deleteFiles(const QStringList &linkcodes)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["items"] = itemsArray(linkcodes);
    return postSimple(m_http, QStringLiteral("/api/fileops/delete"), body);
}

ApiResponse<void> FshareApi::createFolder(const QString &name, const QString &parentId)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["name"] = name.toStdString();
    body["in_dir"] = parentId.toStdString();
    return postSimple(m_http, QStringLiteral("/api/fileops/createFolder"), body);
}

ApiResponse<void> FshareApi::createFolderInPath(const QString &name, const QString &parentPath)
{
    // Fshare expects parentPath as a slash-rooted path: "/" for the root, or
    // "/Foo/Bar" for a nested destination. Normalise so callers can hand us
    // bare segments without worrying about the leading slash.
    QString p = parentPath.trimmed();
    if (p.isEmpty()) p = QStringLiteral("/");
    else if (!p.startsWith(QLatin1Char('/'))) p.prepend(QLatin1Char('/'));

    Json::Value body;
    body["token"]  = m_token.toStdString();
    body["name"]   = name.toStdString();
    body["in_dir"] = p.toStdString();
    return postSimple(m_http, QStringLiteral("/api/fileops/createFolderInPath"), body);
}

ApiResponse<void> FshareApi::moveFiles(const QStringList &linkcodes, const QString &to)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["items"] = itemsArray(linkcodes);
    body["to"] = to.toStdString();
    return postSimple(m_http, QStringLiteral("/api/fileops/move"), body);
}

ApiResponse<void> FshareApi::copyFiles(const QStringList &linkcodes, const QString &to)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["items"] = itemsArray(linkcodes);
    body["to"] = to.toStdString();
    return postSimple(m_http, QStringLiteral("/api/fileops/copy"), body);
}

// ── File settings ──────────────────────────────────────

ApiResponse<void> FshareApi::changeSecure(const QStringList &linkcodes, bool secure)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["items"] = itemsArray(linkcodes);
    body["status"] = secure ? 1 : 0;
    return postSimple(m_http, QStringLiteral("/api/fileops/changeSecure"), body);
}

ApiResponse<void> FshareApi::setFilePassword(const QStringList &linkcodes, const QString &password)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["items"] = itemsArray(linkcodes);
    body["pass"] = password.toStdString();
    return postSimple(m_http, QStringLiteral("/api/fileops/createFilePass"), body);
}

ApiResponse<void> FshareApi::setDirectLink(const QStringList &linkcodes, bool enabled)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    body["items"] = itemsArray(linkcodes);
    body["status"] = enabled ? 1 : 0;
    return postSimple(m_http, QStringLiteral("/api/share/SetDirectLink"), body);
}

// ── Favorites ──────────────────────────────────────────────

ApiResponse<void> FshareApi::changeFavorite(const QString &linkcode, bool add)
{
    Json::Value body;
    body["token"] = m_token.toStdString();
    Json::Value items(Json::arrayValue);
    items.append(linkcode.toStdString());
    body["items"] = items;
    body["status"] = add ? 1 : 0;
    return postSimple(m_http, QStringLiteral("/api/fileops/changeFavorite"), body);
}

ApiResponse<bool> FshareApi::isFavorite(const QString &linkcode)
{
    QString url = API_BASE + QStringLiteral("/api/fileops/isFavorite?linkcode=") + linkcode;
    HttpResponse resp = m_http->get(url);
    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError())
        return ApiResponse<bool>::failure(err, resp.statusCode);
    bool fav = safeBool(root.get("isFavorite", false));
    return ApiResponse<bool>::success(fav);
}

ApiResponse<QVector<FileItem>> FshareApi::listFavorites(const QString &extFilter)
{
    QString url = API_BASE + QStringLiteral("/api/fileops/listFavorite");
    if (!extFilter.isEmpty())
        url += QStringLiteral("?ext=") + extFilter;

    HttpResponse resp = m_http->get(url);
    if (resp.statusCode <= 0)
        return ApiResponse<QVector<FileItem>>::failure(AppError::network(resp.statusCode, {}));

    Json::Value root = parseJson(resp.body);

    // 404 = no favorites (empty list, not an error)
    if (root.isObject() && root.isMember("code")) {
        int code = root.get("code", 200).asInt();
        if (code == 404)
            return ApiResponse<QVector<FileItem>>::success({});
        if (code != 200) {
            QString msg = QString::fromStdString(root.get("msg", "").asString());
            return ApiResponse<QVector<FileItem>>::failure(AppError::server(code, msg));
        }
    }

    QVector<FileItem> items;
    if (root.isArray()) {
        for (const auto &item : root)
            items.append(parseFileItem(item));
    }
    return ApiResponse<QVector<FileItem>>::success(items);
}

// ── Transfer session creation ──────────────────────────────

ApiResponse<QString> FshareApi::createDownloadSession(const QString &url, const QString &password)
{
    // Use QJsonObject + jsonBody() helper (same as login) so UTF-8 encoding
    // flows through QJsonDocument — NOT jsoncpp + std::string which has
    // occasionally lost bytes on Windows.
    //
    // Field types must match the legacy Fshare client byte-for-byte:
    //   - zipflag: BOOL (legacy sessionapi.cpp:59 uses `false`, not 0)
    //   - password: omitted when empty (some builds choke on "password":"")
    QJsonObject body;
    body[QStringLiteral("token")]   = m_token;
    body[QStringLiteral("url")]     = url;
    body[QStringLiteral("zipflag")] = false;
    if (!password.isEmpty())
        body[QStringLiteral("password")] = password;

    HttpResponse resp = m_http->post(
        API_BASE + QStringLiteral("/api/session/download"), jsonBody(body));

    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError()) {
        qWarning().noquote() << "[FshareApi] createDownloadSession failed HTTP"
                             << resp.statusCode << "body:" << resp.body;
        return ApiResponse<QString>::failure(err, resp.statusCode);
    }

    QString location = QString::fromStdString(root.get("location", "").asString());
    if (location.isEmpty()) {
        qWarning().noquote() << "[FshareApi] createDownloadSession OK but no location, body:"
                             << resp.body;
        return ApiResponse<QString>::failure(AppError::server(0, QStringLiteral("No download URL returned")));
    }

    return ApiResponse<QString>::success(location);
}

ApiResponse<QString> FshareApi::createUploadSession(const QString &name, int64_t size,
                                                     const QString &folder, bool secured)
{
    // Fshare's upload API takes a folder PATH ("/", "/My Folder"), NOT a
    // folder ID. Passing "0" makes the server reject the request with a
    // misleading "không cho phép ký tự đặc biệt" message even though the
    // file has never been uploaded. Legacy dialogupload.cpp normalises to
    // "/" for root and prepends "/" when missing — mirror that here.
    QString folderForApi = folder.trimmed();
    if (folderForApi.isEmpty()
        || folderForApi == QStringLiteral("0")
        || folderForApi == QStringLiteral("/")) {
        folderForApi = QStringLiteral("/");
    } else if (!folderForApi.startsWith(QLatin1Char('/'))) {
        folderForApi = QLatin1Char('/') + folderForApi;
    }

    QJsonObject body;
    body[QStringLiteral("token")]   = m_token;
    body[QStringLiteral("name")]    = name;
    body[QStringLiteral("size")]    = QString::number(size);   // legacy sends as STRING
    body[QStringLiteral("path")]    = folderForApi;
    body[QStringLiteral("secured")] = secured ? 1 : 0;         // legacy uses int 0/1 here

    // Redact token for safe logging — show only prefix/suffix so session issues
    // are still traceable without leaking credentials.
    const QString tokenPreview = m_token.size() > 10
        ? m_token.left(4) + QStringLiteral("…") + m_token.right(4)
        : QStringLiteral("<empty>");
    const QByteArray reqBody = jsonBody(body);
    QByteArray reqBodyForLog = reqBody;
    if (!m_token.isEmpty())
        reqBodyForLog.replace(m_token.toUtf8(), tokenPreview.toUtf8());

    qDebug().noquote() << "[FshareApi] createUploadSession REQUEST"
                       << "name:" << name
                       << "size:" << size
                       << "path:" << folderForApi
                       << "secured:" << secured
                       << "token:" << tokenPreview
                       << "body:" << reqBodyForLog;

    HttpResponse resp = m_http->post(
        API_BASE + QStringLiteral("/api/session/upload"), reqBody);

    Json::Value root = parseJson(resp.body);
    AppError err = checkApiResponse(resp, root);
    if (err.isError()) {
        qWarning().noquote() << "[FshareApi] createUploadSession FAILED HTTP"
                             << resp.statusCode
                             << "name:" << name << "folder:" << folderForApi
                             << "token:" << tokenPreview
                             << "reqBody:" << reqBodyForLog
                             << "respBody:" << resp.body;
        return ApiResponse<QString>::failure(err, resp.statusCode);
    }

    QString location = QString::fromStdString(root.get("location", "").asString());
    if (location.isEmpty()) {
        qWarning().noquote() << "[FshareApi] createUploadSession OK but no location, body:"
                             << resp.body;
        return ApiResponse<QString>::failure(AppError::server(0, QStringLiteral("No upload URL returned")));
    }

    return ApiResponse<QString>::success(location);
}

} // namespace fsnext
