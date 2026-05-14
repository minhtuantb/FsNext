#pragma once

#include "HttpClient.h"
#include "core/models/ApiResponse.h"
#include "core/models/FileItem.h"
#include "core/models/User.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <cstdint>

namespace fsnext {

class FshareApi {
public:
    explicit FshareApi(HttpClient *http);

    // Session
    void setSession(const QString &token, const QString &cookie);

    // Auth
    ApiResponse<Session>            login(const QString &email, const QString &password);
    // Social login (Google/Facebook/FPT ID). `service` must be one of the
    // Fshare-recognised strings: "google", "facebook", "fpt-id". The caller
    // already completed the provider-side OAuth flow and passes the provider's
    // access_token + the user's email here.
    ApiResponse<Session>            loginOauth(const QString &service,
                                               const QString &accessToken,
                                               const QString &email);
    ApiResponse<void>               logout();
    ApiResponse<User>               getUserInfo();

    // File listing & search
    ApiResponse<QVector<FileItem>>  listFiles(const QString &folderUrl, int page, int pageSize);
    ApiResponse<QVector<FileItem>>  listFolders(const QString &path);
    ApiResponse<FileItem>           getFileInfo(const QString &url);
    ApiResponse<QVector<FileItem>>  searchFiles(const QString &keyword, int page);

    // File operations
    ApiResponse<void>               renameFile(const QString &linkcode, const QString &newName);
    ApiResponse<void>               deleteFiles(const QStringList &linkcodes);
    ApiResponse<void>               createFolder(const QString &name, const QString &parentId);
    // Same as createFolder but addresses the parent by PATH ("/", "/Foo/Bar")
    // rather than linkcode. Used by Sync to mirror nested local subdirectories
    // on Fshare without needing to resolve ID chains.
    ApiResponse<void>               createFolderInPath(const QString &name, const QString &parentPath);
    ApiResponse<void>               moveFiles(const QStringList &linkcodes, const QString &to);
    ApiResponse<void>               copyFiles(const QStringList &linkcodes, const QString &to);

    // File settings
    ApiResponse<void>               changeSecure(const QStringList &linkcodes, bool secure);
    ApiResponse<void>               setFilePassword(const QStringList &linkcodes, const QString &password);
    ApiResponse<void>               setDirectLink(const QStringList &linkcodes, bool enabled);

    // Favorites
    ApiResponse<void>               changeFavorite(const QString &linkcode, bool add);
    ApiResponse<bool>               isFavorite(const QString &linkcode);
    ApiResponse<QVector<FileItem>>  listFavorites(const QString &extFilter = {});

    // Transfer session creation
    ApiResponse<QString>            createDownloadSession(const QString &url, const QString &password);
    ApiResponse<QString>            createUploadSession(const QString &name, int64_t size,
                                                        const QString &folder, bool secured);

private:
    HttpClient *m_http = nullptr;
    QString     m_appKey;
    QString     m_token;
};

} // namespace fsnext
