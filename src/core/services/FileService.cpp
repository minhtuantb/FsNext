#include "FileService.h"
#include "core/api/FshareApi.h"
#include <QtConcurrent>
#include <QPointer>
#include <QDebug>

namespace fsnext {

FileService::FileService(FshareApi *api, QObject *parent)
    : QObject(parent)
    , m_api(api)
{
}

// Helper: safely run an API call on thread pool, route result back to service on main thread.
// If the service is destroyed before the result arrives, QMetaObject::invokeMethod drops the call.
#define FSNEXT_ASYNC_API(work_lambda, result_lambda) \
    do { \
        auto *api = m_api; \
        QPointer<FileService> guard(this); \
        QtConcurrent::run([api, guard, work_lambda, result_lambda]() mutable { \
            auto resp = work_lambda(api); \
            QMetaObject::invokeMethod(guard.data() ? guard.data() : nullptr, \
                [guard, resp, result_lambda]() mutable { \
                    if (!guard) return; \
                    result_lambda(guard.data(), resp); \
                }); \
        }); \
    } while (0)

void FileService::loadFolderTree()
{
    emit isLoadingChanged(true);
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard]() mutable {
        auto resp = api->listFolders({});
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            emit guard->isLoadingChanged(false);
            if (resp.isSuccess()) {
                guard->m_folderTree = resp.data();
                qDebug() << "[FsNext] Folder tree loaded," << guard->m_folderTree.size() << "folders";
                emit guard->folderTreeLoaded(guard->m_folderTree);
            } else {
                qWarning() << "[FsNext] Folder tree failed:" << resp.error().message;
                emit guard->operationFailed(resp.error().message);
            }
        });
    });
}

void FileService::listFiles(const QString &folderId, int page)
{
    emit isLoadingChanged(true);
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, folderId, page]() mutable {
        auto resp = api->listFiles(folderId, page, 50);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            emit guard->isLoadingChanged(false);
            if (resp.isSuccess()) {
                guard->m_currentFiles = resp.data();
                emit guard->fileListLoaded(guard->m_currentFiles);
            } else {
                emit guard->operationFailed(resp.error().message);
            }
        });
    });
}

void FileService::createFolder(const QString &name, const QString &parentId)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, name, parentId]() mutable {
        auto resp = api->createFolder(name, parentId);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, name]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(QStringLiteral("Folder '%1' created").arg(name));
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::renameFile(const QString &linkcode, const QString &newName)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcode, newName]() mutable {
        auto resp = api->renameFile(linkcode, newName);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(QStringLiteral("Renamed"));
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::deleteFiles(const QStringList &linkcodes)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcodes]() mutable {
        auto resp = api->deleteFiles(linkcodes);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, linkcodes]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(QStringLiteral("Deleted %1 items").arg(linkcodes.size()));
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::moveFiles(const QStringList &linkcodes, const QString &targetFolderId)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcodes, targetFolderId]() mutable {
        auto resp = api->moveFiles(linkcodes, targetFolderId);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(QStringLiteral("Moved"));
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::copyFiles(const QStringList &linkcodes, const QString &targetFolderId)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcodes, targetFolderId]() mutable {
        auto resp = api->copyFiles(linkcodes, targetFolderId);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(QStringLiteral("Copied"));
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::searchFiles(const QString &keyword, int page)
{
    emit isLoadingChanged(true);
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, keyword, page]() mutable {
        auto resp = api->searchFiles(keyword, page);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            emit guard->isLoadingChanged(false);
            if (resp.isSuccess()) {
                guard->m_currentFiles = resp.data();
                emit guard->fileListLoaded(guard->m_currentFiles);
            } else {
                emit guard->operationFailed(resp.error().message);
            }
        });
    });
}

void FileService::getFileInfo(const QString &url)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, url]() mutable {
        auto resp = api->getFileInfo(url);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(resp.data().name);
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::changeSecure(const QStringList &linkcodes, bool secure)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcodes, secure]() mutable {
        auto resp = api->changeSecure(linkcodes, secure);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, secure]() {
            if (!guard) return;
            if (resp.isSuccess())
                emit guard->operationComplete(secure ? QStringLiteral("Secured") : QStringLiteral("Unsecured"));
            else
                emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::setPassword(const QStringList &linkcodes, const QString &password)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcodes, password]() mutable {
        auto resp = api->setFilePassword(linkcodes, password);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            if (resp.isSuccess()) emit guard->operationComplete(QStringLiteral("Password set"));
            else emit guard->operationFailed(resp.error().message);
        });
    });
}

void FileService::setDirectLink(const QStringList &linkcodes, bool enabled)
{
    auto *api = m_api;
    QPointer<FileService> guard(this);
    QtConcurrent::run([api, guard, linkcodes, enabled]() mutable {
        auto resp = api->setDirectLink(linkcodes, enabled);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, enabled]() {
            if (!guard) return;
            if (resp.isSuccess())
                emit guard->operationComplete(enabled ? QStringLiteral("Direct link enabled") : QStringLiteral("Direct link disabled"));
            else
                emit guard->operationFailed(resp.error().message);
        });
    });
}

} // namespace fsnext
