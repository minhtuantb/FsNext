#pragma once

#include "core/models/FileItem.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace fsnext {

class FshareApi;

class FileService : public QObject
{
    Q_OBJECT

public:
    explicit FileService(FshareApi *api, QObject *parent = nullptr);
    ~FileService() override = default;

    // Tree & listing
    Q_INVOKABLE void loadFolderTree();
    Q_INVOKABLE void listFiles(const QString &folderId, int page = 0);

    // CRUD operations. Declared virtual so tests can substitute a fake that
    // synthesises operationComplete/Failed without hitting the network.
    Q_INVOKABLE virtual void createFolder(const QString &name, const QString &parentId);
    Q_INVOKABLE virtual void renameFile(const QString &linkcode, const QString &newName);
    Q_INVOKABLE virtual void deleteFiles(const QStringList &linkcodes);
    Q_INVOKABLE virtual void moveFiles(const QStringList &linkcodes, const QString &targetFolderId);
    Q_INVOKABLE virtual void copyFiles(const QStringList &linkcodes, const QString &targetFolderId);

    // Search & info
    Q_INVOKABLE void searchFiles(const QString &keyword, int page = 1);
    Q_INVOKABLE virtual void getFileInfo(const QString &url);

    // File settings. Virtual for the same reason as the CRUD ops above —
    // they share the operationComplete signal, so tests need to substitute.
    Q_INVOKABLE virtual void changeSecure(const QStringList &linkcodes, bool secure);
    Q_INVOKABLE virtual void setPassword(const QStringList &linkcodes, const QString &password);
    Q_INVOKABLE virtual void setDirectLink(const QStringList &linkcodes, bool enabled);

    // Accessors
    QVector<FileItem> currentFiles() const { return m_currentFiles; }
    QVector<FileItem> folderTree() const { return m_folderTree; }

signals:
    void folderTreeLoaded(const QVector<fsnext::FileItem> &folders);
    void fileListLoaded(const QVector<fsnext::FileItem> &files);
    void operationComplete(const QString &message);
    void operationFailed(const QString &error);
    void isLoadingChanged(bool loading);

private:
    FshareApi *m_api = nullptr;
    QVector<FileItem> m_currentFiles;
    QVector<FileItem> m_folderTree;
};

} // namespace fsnext
