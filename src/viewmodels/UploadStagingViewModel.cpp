// SPDX-License-Identifier: Proprietary
#include "UploadStagingViewModel.h"
#include "UploadViewModel.h"
#include "core/repositories/SettingsRepository.h"

#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QUrl>

namespace fsnext {

// QSettings keys for persisted staging. Kept under a dedicated group so other
// settings can't accidentally collide. QLatin1String (not QStringLiteral) here
// because QStringLiteral is a macro that needs a string literal, not a
// constexpr variable — and the keys are referenced from two call sites.
static const QLatin1String kKeyFiles    {"UploadStaging/files"};       // JSON array
static const QLatin1String kKeyFolder   {"UploadStaging/folder"};      // "/" or "/Sub"
static const QLatin1String kKeyPassword {"UploadStaging/password"};    // sensitive — see persistNow()
static const QLatin1String kKeySecured  {"UploadStaging/secured"};     // 0|1
static const QLatin1String kKeyDirect   {"UploadStaging/directLink"};  // 0|1

// Normalize "file:///D:/a.mkv", "D:\a.mkv", and percent-encoded variants into a
// single canonical local path. Used both for QFileInfo lookups and for dedupe.
static QString toLocalPath(const QString &urlOrPath)
{
    QString s = urlOrPath;
    if (s.startsWith(QLatin1String("file://"))) {
        s = QUrl(s).toLocalFile();
    } else if (s.contains(QLatin1Char('%'))) {
        s = QUrl::fromPercentEncoding(s.toUtf8());
    }
    return s;
}

// Build a QVariantMap entry for a path. Stat is best-effort: if the file is
// missing/inaccessible, valid=false and size=0 so the UI can render a warning
// without the caller having to filter.
static QVariantMap fileEntry(const QString &urlOrPath)
{
    const QString local = toLocalPath(urlOrPath);
    const QFileInfo fi(local);
    const bool valid = fi.exists() && fi.isFile();
    QVariantMap m;
    m.insert(QStringLiteral("path"),  urlOrPath);  // keep original form (QML uses file:// URLs)
    m.insert(QStringLiteral("name"),  fi.fileName());
    m.insert(QStringLiteral("ext"),   fi.suffix().toLower());
    m.insert(QStringLiteral("size"),  static_cast<qlonglong>(valid ? fi.size() : 0));
    m.insert(QStringLiteral("valid"), valid);
    return m;
}

UploadStagingViewModel::UploadStagingViewModel(UploadViewModel *upload,
                                                SettingsRepository *settings,
                                                QObject *parent)
    : QObject(parent), m_upload(upload), m_settings(settings)
{
    m_persistTimer.setSingleShot(true);
    m_persistTimer.setInterval(200);
    connect(&m_persistTimer, &QTimer::timeout, this,
            [this]() { persistNow(); });
    hydrate();
}

void UploadStagingViewModel::setTargetFolder(const QString &f)
{
    if (m_folder == f) return;
    m_folder = f;
    emit targetFolderChanged();
    schedulePersist();
}

void UploadStagingViewModel::setPassword(const QString &p)
{
    if (m_password == p) return;
    m_password = p;
    emit passwordChanged();
    schedulePersist();
}

void UploadStagingViewModel::setSecured(bool s)
{
    if (m_secured == s) return;
    m_secured = s;
    emit securedChanged();
    schedulePersist();
}

void UploadStagingViewModel::setDirectLink(bool d)
{
    if (m_directLink == d) return;
    m_directLink = d;
    emit directLinkChanged();
    schedulePersist();
}

void UploadStagingViewModel::addFiles(const QStringList &fileUrlsOrPaths)
{
    bool changed = false;
    for (const QString &u : fileUrlsOrPaths) {
        if (u.isEmpty()) continue;
        // Dedupe by canonical local path so dropping the same file twice via
        // different representations ("file:///D:/a.mkv" vs raw path) collapses.
        const QString cand = toLocalPath(u);
        bool dup = false;
        for (const QVariant &v : m_files) {
            if (toLocalPath(v.toMap().value(QStringLiteral("path")).toString()) == cand) {
                dup = true; break;
            }
        }
        if (dup) continue;
        m_files.append(fileEntry(u));
        changed = true;
    }
    if (changed) {
        emit stagedFilesChanged();
        schedulePersist();
        // Mark "the user just added files" so UploadPage knows to auto-pop the
        // dialog (vs a passive sidebar navigation where staging is leftover).
        if (!m_showRequested) {
            m_showRequested = true;
            emit showRequestedChanged();
        }
    }
}

void UploadStagingViewModel::removeFile(int index)
{
    if (index < 0 || index >= m_files.size()) return;
    m_files.removeAt(index);
    emit stagedFilesChanged();
    schedulePersist();
}

void UploadStagingViewModel::clear()
{
    const bool wasEmpty = m_files.isEmpty()
                       && m_password.isEmpty()
                       && m_folder == QStringLiteral("/")
                       && !m_secured && !m_directLink;
    if (wasEmpty) {
        // Nothing to wipe; just dismiss the restore banner if it was up.
        if (m_restoredFromDisk) {
            m_restoredFromDisk = false;
            emit restoredFromDiskChanged();
            // Persist the dismissal so a relaunch doesn't resurrect the banner.
            persistNow();
        }
        return;
    }
    m_files.clear();
    m_password.clear();
    m_folder = QStringLiteral("/");
    m_secured = false;
    m_directLink = false;
    const bool wasRestored = m_restoredFromDisk;
    m_restoredFromDisk = false;
    const bool wasShowReq = m_showRequested;
    m_showRequested = false;
    emit stagedFilesChanged();
    emit passwordChanged();
    emit targetFolderChanged();
    emit securedChanged();
    emit directLinkChanged();
    if (wasRestored) emit restoredFromDiskChanged();
    if (wasShowReq)  emit showRequestedChanged();
    // Sync to disk NOW (not debounced): a crash between clear() and the
    // debounce-flush would otherwise resurrect a batch the user just discarded,
    // which would feel like a ghost.
    persistNow();
}

void UploadStagingViewModel::clearFiles()
{
    if (m_files.isEmpty()) return;
    m_files.clear();
    emit stagedFilesChanged();
    // showRequested only meaningful when there are files to show; reset it so
    // a subsequent addFiles cleanly raises a fresh "show" pulse.
    if (m_showRequested) {
        m_showRequested = false;
        emit showRequestedChanged();
    }
    persistNow();
}

void UploadStagingViewModel::commit()
{
    if (!m_upload || m_files.isEmpty()) return;
    QStringList valid;
    valid.reserve(m_files.size());
    for (const QVariant &v : m_files) {
        const QVariantMap m = v.toMap();
        // Skip files that vanished between sessions — UI flagged them already
        // (the user could have removed them explicitly but we don't enforce
        // that). Commit-time skip keeps the action non-blocking.
        if (!m.value(QStringLiteral("valid"), true).toBool()) continue;
        const QString path = m.value(QStringLiteral("path")).toString();
        if (!path.isEmpty()) valid << path;
    }
    if (!valid.isEmpty()) {
        m_upload->addUpload(valid, m_folder, /*desc*/QString(),
                            m_password, m_secured, m_directLink);
    }
    clear();
}

void UploadStagingViewModel::acknowledgeRestored()
{
    if (!m_restoredFromDisk) return;
    m_restoredFromDisk = false;
    emit restoredFromDiskChanged();
    // Persist so a subsequent relaunch doesn't show the banner again for the
    // same staged batch.
    persistNow();
}

void UploadStagingViewModel::acknowledgeShow()
{
    if (!m_showRequested) return;
    m_showRequested = false;
    emit showRequestedChanged();
}

void UploadStagingViewModel::revalidate()
{
    bool changed = false;
    for (int i = 0; i < m_files.size(); ++i) {
        QVariantMap m = m_files[i].toMap();
        const QString path = m.value(QStringLiteral("path")).toString();
        const QFileInfo fi(toLocalPath(path));
        const bool nowValid = fi.exists() && fi.isFile();
        const qlonglong nowSize = nowValid ? fi.size() : 0;
        if (m.value(QStringLiteral("valid")).toBool() != nowValid
         || m.value(QStringLiteral("size")).toLongLong() != nowSize) {
            m.insert(QStringLiteral("valid"), nowValid);
            m.insert(QStringLiteral("size"),  nowSize);
            m_files[i] = m;
            changed = true;
        }
    }
    if (changed) emit stagedFilesChanged();
}

void UploadStagingViewModel::schedulePersist()
{
    if (!m_persistTimer.isActive()) m_persistTimer.start();
}

void UploadStagingViewModel::persistNow() const
{
    if (!m_settings) return;
    // QSettings can't round-trip QVariantList of QVariantMaps reliably across
    // backends, so we serialize to a compact JSON blob.
    QJsonArray arr;
    for (const QVariant &v : m_files) {
        const QVariantMap m = v.toMap();
        QJsonObject obj;
        obj[QStringLiteral("path")] = m.value(QStringLiteral("path")).toString();
        obj[QStringLiteral("name")] = m.value(QStringLiteral("name")).toString();
        obj[QStringLiteral("ext")]  = m.value(QStringLiteral("ext")).toString();
        obj[QStringLiteral("size")] = m.value(QStringLiteral("size")).toLongLong();
        // valid is re-evaluated on hydrate; persisting it would make a deleted
        // file resurrect as valid until the next stat — confusing.
        arr.append(obj);
    }
    const QString json = QString::fromUtf8(
        QJsonDocument(arr).toJson(QJsonDocument::Compact));
    auto *s = const_cast<SettingsRepository *>(m_settings);
    s->setString(kKeyFiles, json);
    s->setString(kKeyFolder, m_folder);
    // Password is stored as-is. Threat model: same trust level as the saved
    // session token already in QSettings (legacy "rememberMe"). The Fshare
    // upload password is a per-file PIN, not an account password — leaking it
    // unlocks downloads of that one file, nothing else.
    s->setString(kKeyPassword, m_password);
    s->setInt(kKeySecured, m_secured ? 1 : 0);
    s->setInt(kKeyDirect,  m_directLink ? 1 : 0);
}

void UploadStagingViewModel::hydrate()
{
    if (!m_settings) return;
    const QString json = m_settings->getString(kKeyFiles);
    if (!json.isEmpty()) {
        const QJsonArray arr = QJsonDocument::fromJson(json.toUtf8()).array();
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            QVariantMap m;
            const QString path = o.value(QStringLiteral("path")).toString();
            if (path.isEmpty()) continue;
            // Re-stat at hydrate: a file that existed last session may have
            // been deleted/moved since. Keep the entry so the user sees what
            // they had staged, but flag valid=false so the UI can warn and
            // commit() can skip it.
            const QFileInfo fi(toLocalPath(path));
            const bool valid = fi.exists() && fi.isFile();
            m.insert(QStringLiteral("path"),  path);
            m.insert(QStringLiteral("name"),  o.value(QStringLiteral("name")).toString());
            m.insert(QStringLiteral("ext"),   o.value(QStringLiteral("ext")).toString());
            m.insert(QStringLiteral("size"),
                     static_cast<qlonglong>(valid ? fi.size()
                                                  : static_cast<qlonglong>(o.value(QStringLiteral("size")).toInteger())));
            m.insert(QStringLiteral("valid"), valid);
            m_files.append(m);
        }
    }
    m_folder     = m_settings->getString(kKeyFolder, QStringLiteral("/"));
    m_password   = m_settings->getString(kKeyPassword);
    m_secured    = m_settings->getInt(kKeySecured, 0) != 0;
    m_directLink = m_settings->getInt(kKeyDirect,  0) != 0;
    if (!m_files.isEmpty()) {
        m_restoredFromDisk = true;
        qInfo() << "[UploadStaging] Hydrated" << m_files.size()
                << "staged file(s) from previous session.";
        // Defer the change emits so any QML side that connects in
        // Component.onCompleted (after AppContext finishes) sees them.
        QMetaObject::invokeMethod(this, [this]() {
            emit stagedFilesChanged();
            emit restoredFromDiskChanged();
        }, Qt::QueuedConnection);
    }
}

} // namespace fsnext
