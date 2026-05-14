#pragma once
#include <QString>
#include <cstdint>

namespace fsnext {

struct FileItem {
    uint64_t id = 0;
    QString linkcode;
    QString name;
    QString type;             // "file" or "folder"
    int64_t size = 0;
    QString path;
    bool secure = false;
    bool isPublic = false;
    bool directlink = false;
    bool hasPassword = false;
    bool deleted = false;
    bool copied = false;
    bool shared = false;
    QString hashIndex;
    QString ownerId;
    QString parentId;
    int downloadCount = 0;
    QString description;
    QString created;
    QString modified;
    QString lastDownload;
    QString tIndex;           // Temporal index from API

    bool isFolder() const { return type == QStringLiteral("folder"); }
    bool isFile() const { return !isFolder(); }
};

} // namespace fsnext
