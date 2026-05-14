// SPDX-License-Identifier: Proprietary
#pragma once

#include <QString>

namespace fsnext::FshareUrl {

enum class Kind {
    Invalid,
    File,
    Folder,
};

// Inspect a pasted URL and return its kind plus the extracted linkcode.
// Tolerates: case-insensitive host & path, optional scheme/www prefix,
// trailing slash, query string, fragment, and surrounding whitespace.
struct ParsedLink {
    Kind    kind{Kind::Invalid};
    QString linkcode;   // just the ID segment after /file/ or /folder/
};

ParsedLink parse(const QString &rawUrl);

// Canonical URL used for API calls. Scheme/host/casing are normalized; the
// share-access token (`?token=<value>`) — required by Fshare to list
// password-less share folders / access share files — is PRESERVED. All other
// query params and fragments are dropped.
//   https://www.fshare.vn/folder/ABC?token=123       → preserved
//   https://fshare.vn/file/ABC?utm_source=x#frag     → …/file/ABC
QString canonicalUrl(const QString &rawUrl);

// Extract just the alphanumeric linkcode — suitable as a stable cache key
// (file-cache, history) regardless of whether the original URL had a token.
QString linkcodeOf(const QString &rawUrl);

inline bool isFshareUrl(const QString &rawUrl) { return parse(rawUrl).kind != Kind::Invalid; }
inline bool isFolderUrl(const QString &rawUrl) { return parse(rawUrl).kind == Kind::Folder; }
inline bool isFileUrl  (const QString &rawUrl) { return parse(rawUrl).kind == Kind::File;   }

} // namespace fsnext::FshareUrl
