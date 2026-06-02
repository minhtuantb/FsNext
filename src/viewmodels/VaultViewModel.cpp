// SPDX-License-Identifier: Proprietary
#include "viewmodels/VaultViewModel.h"

#include "core/crypto/Crypto.h"
#include "core/crypto/FencFormat.h"
#include "core/crypto/Key.h"
#include "core/crypto/KdfParams.h"
#include "core/services/SettingsService.h"
#include "core/services/VaultManager.h"

#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPointer>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>
#include <QtConcurrent>

#include <array>
#include <cstring>
#include <memory>

namespace fsnext {

using crypto::CryptoError;

namespace {
crypto::KdfParams kdfFromPreset(int preset)
{
    switch (preset) {
    case 1:  return crypto::KdfParams::strong();
    case 2:  return crypto::KdfParams::maximum();
    default: return crypto::KdfParams::balanced();
    }
}

QString humanBytes(quint64 b)
{
    static const char *u[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(b);
    int i = 0;
    while (v >= 1024.0 && i < 4) { v /= 1024.0; ++i; }
    return QString::number(v, 'f', i == 0 ? 0 : 1) + " " + QString::fromLatin1(u[i]);
}

QString fpHex(const std::array<std::uint8_t, 8> &fp)
{
    QString h;
    for (int i = 0; i < 8; ++i) {
        h += QString("%1").arg(fp[i], 2, 16, QLatin1Char('0'));
        if (i % 2 == 1 && i < 7)
            h += QLatin1Char(' ');
    }
    return h;  // e.g. "9f3a 71c0 4e8d b21c"
}

// A standalone master-key snapshot so a worker can encrypt/decrypt even if the
// vault auto-locks mid-operation (its own SecureBytes, independent of the VM).
std::shared_ptr<crypto::Key> snapshotKey(VaultManager *mgr)
{
    if (!mgr || !mgr->masterKey().valid())
        return nullptr;
    return std::make_shared<crypto::Key>(
        crypto::Key::fromBytes(mgr->masterKey().data(), crypto::Key::Size));
}
} // namespace

VaultViewModel::VaultViewModel(QObject *parent)
    : QObject(parent), vault_(std::make_unique<VaultManager>())
{
    aesAvailable_ = crypto::aes256GcmAvailable();
    autoLockTimer_.setSingleShot(true);
    connect(&autoLockTimer_, &QTimer::timeout, this, [this] {
        if (isUnlocked())
            lock();
    });
    // Sweep aged decrypted temp files every minute (30-min TTL, see sweepTemps).
    tempSweepTimer_.setInterval(60 * 1000);
    connect(&tempSweepTimer_, &QTimer::timeout, this, [this] { sweepTemps(false); });
    tempSweepTimer_.start();
}

VaultViewModel::~VaultViewModel()
{
    // Best-effort secure wipe of any decrypted temp files left on disk.
    sweepTemps(true);
}

void VaultViewModel::setServices(SettingsService *settings, Uploader uploader)
{
    settings_ = settings;
    uploader_ = std::move(uploader);
    emit vaultPrefsChanged();
}

QString VaultViewModel::toLocalPath(const QString &s)
{
    return s.startsWith(QStringLiteral("file:")) ? QUrl(s).toLocalFile() : s;
}

// ── Vault prefs (proxied to SettingsService) ──
int VaultViewModel::maxEncryptMb() const { return settings_ ? settings_->vaultMaxEncryptMb() : 0; }
void VaultViewModel::setMaxEncryptMb(int mb)
{
    if (!settings_ || settings_->vaultMaxEncryptMb() == mb) return;
    settings_->setVaultMaxEncryptMb(mb);
    emit vaultPrefsChanged();
}

int VaultViewModel::overLimitBehavior() const
{
    return settings_ ? settings_->vaultOverLimitBehavior() : 0;
}
void VaultViewModel::setOverLimitBehavior(int b)
{
    if (!settings_ || settings_->vaultOverLimitBehavior() == b) return;
    settings_->setVaultOverLimitBehavior(b);
    emit vaultPrefsChanged();
}

bool VaultViewModel::autoUploadDefault() const
{
    return settings_ ? settings_->vaultAutoUpload() : false;
}
void VaultViewModel::setAutoUploadDefault(bool on)
{
    if (!settings_ || settings_->vaultAutoUpload() == on) return;
    settings_->setVaultAutoUpload(on);
    emit vaultPrefsChanged();
}

QString VaultViewModel::uploadFolder() const
{
    return settings_ ? settings_->vaultUploadFolder() : QStringLiteral("/");
}
void VaultViewModel::setUploadFolder(const QString &path)
{
    if (!settings_ || settings_->vaultUploadFolder() == path) return;
    settings_->setVaultUploadFolder(path);
    emit vaultPrefsChanged();
}

qint64 VaultViewModel::encryptCapBytes() const
{
    const int mb = maxEncryptMb();
    if (mb > 0)
        return static_cast<qint64>(mb) * 1024 * 1024;
    // No user cap → use the natural 1 GiB "large file" boundary for the prompt.
    return static_cast<qint64>(crypto::EncryptionEngine::LargeFileThreshold);
}

int VaultViewModel::overLimitPolicy() const { return overLimitBehavior(); }

void VaultViewModel::setVaultDir(const QString &dir)
{
    const QString local = toLocalPath(dir);
    if (local == vaultDir_)
        return;
    vaultDir_ = local;
    vault_->setVaultDir(local);
    keyRegistry_.load((vaultDir_ + QStringLiteral("/.key-registry.json")).toStdString());
    refreshState();
    refreshFiles();
    refreshKeys();
}

void VaultViewModel::setBusy(bool b)
{
    if (busy_ == b)
        return;
    busy_ = b;
    emit busyChanged();
}

void VaultViewModel::refreshState()
{
    const int newLock = static_cast<int>(vault_->state());  // NoVault=0, Locked=1, Unlocked=2
    vaultExists_   = vault_->vaultExists();
    vaultName_     = vault_->vaultName();
    hasRecovery_   = vault_->hasRecoveryKey();
    storageLevel_  = vault_->storageLevelValue();
    const int alm  = vault_->vaultExists() ? vault_->autoLockMinutes() : autoLockMin_;
    lockState_     = newLock;
    emit lockStateChanged();
    if (alm != autoLockMin_) {
        autoLockMin_ = alm;
        emit autoLockMinutesChanged();
    }
}

void VaultViewModel::armAutoLock()
{
    autoLockTimer_.stop();
    if (isUnlocked() && storageLevel_ != 3 && autoLockMin_ > 0)
        autoLockTimer_.start(autoLockMin_ * 60 * 1000);
}

void VaultViewModel::noteActivity()
{
    if (isUnlocked())
        armAutoLock();
}

void VaultViewModel::setAutoLockMinutes(int minutes)
{
    const int m = minutes < 0 ? 0 : minutes;
    if (m == autoLockMin_ && (!vaultExists_ || vault_->autoLockMinutes() == m))
        return;
    autoLockMin_ = m;
    if (vaultExists_)
        vault_->setAutoLockMinutes(m);
    emit autoLockMinutesChanged();
    armAutoLock();
}

void VaultViewModel::setFileBusy(bool b)
{
    if (fileBusy_ == b)
        return;
    fileBusy_ = b;
    emit fileBusyChanged();
}

void VaultViewModel::finish(const QString &op, CryptoError e)
{
    refreshState();
    refreshFiles();
    refreshKeys();
    const bool ok = (e == CryptoError::Ok);
    if (ok)
        armAutoLock();
    emit operationFinished(op, ok, ok ? QString() : QString::fromLatin1(crypto::toString(e)));
}

void VaultViewModel::refreshFiles()
{
    QVariantList list;
    if (isUnlocked() && !vaultDir_.isEmpty()) {
        QDir dir(vaultDir_);
        const QFileInfoList entries =
            dir.entryInfoList(QStringList{QStringLiteral("*.fshenc")}, QDir::Files, QDir::Time);
        for (const QFileInfo &fi : entries) {
            crypto::FencHeader h;
            QString  name  = fi.completeBaseName();   // fallback: strip ".fshenc"
            quint64  osize = static_cast<quint64>(fi.size());
            if (engine_.readHeader(fi.absoluteFilePath().toStdString(), h) == CryptoError::Ok) {
                if (!h.filename.empty())
                    name = QString::fromStdString(h.filename);
                osize = h.originalSize;
            }
            QVariantMap m;
            m.insert(QStringLiteral("path"), fi.absoluteFilePath());
            m.insert(QStringLiteral("name"), name);
            m.insert(QStringLiteral("sizeText"), humanBytes(osize));
            list.append(m);
        }
    }
    vaultFiles_ = list;
    emit vaultFilesChanged();
}

void VaultViewModel::addFiles(const QStringList &paths)
{
    // Quick-add (button / drag-drop) honours the user's auto-upload default.
    beginEncrypt(paths, autoUploadDefault(), uploadFolder());
}

void VaultViewModel::addFilesUpload(const QStringList &paths, bool autoUpload,
                                    const QString &cloudFolder)
{
    beginEncrypt(paths, autoUpload, cloudFolder.isEmpty() ? QStringLiteral("/") : cloudFolder);
}

void VaultViewModel::beginEncrypt(const QStringList &paths, bool autoUpload,
                                  const QString &cloudFolder)
{
    if (!isUnlocked() || fileBusy_)
        return;

    const qint64 cap = encryptCapBytes();
    QStringList small, large;
    for (const QString &p : paths) {
        const QString lp = toLocalPath(p);
        if (lp.isEmpty())
            continue;
        const QFileInfo fi(lp);
        if (!fi.isFile())
            continue;
        if (fi.size() > cap)
            large << lp;
        else
            small << lp;
    }
    if (small.isEmpty() && large.isEmpty())
        return;

    if (large.isEmpty()) {
        runEncryptBatch(small, 0, autoUpload, cloudFolder);
        return;
    }

    switch (overLimitPolicy()) {
    case 1:  // skip large files
        runEncryptBatch(small, large.size(), autoUpload, cloudFolder);
        return;
    case 2:  // copy large files to a plain (unencrypted) folder outside the vault
        for (const QString &f : large)
            copyToPlainFolder(f);
        runEncryptBatch(small, 0, autoUpload, cloudFolder);
        return;
    default: // 0 = ask: stash and let the UI resolve via DLG-LARGEFILE
        break;
    }

    pendingSmall_      = small;
    pendingLarge_      = large;
    pendingAutoUpload_ = autoUpload;
    pendingFolder_     = cloudFolder;
    QVariantList list;
    for (const QString &f : large) {
        const QFileInfo fi(f);
        const quint64 sz = static_cast<quint64>(fi.size());
        QVariantMap m;
        m.insert(QStringLiteral("path"), f);
        m.insert(QStringLiteral("name"), fi.fileName());
        m.insert(QStringLiteral("sizeText"), humanBytes(sz));
        // Rough estimate at ~200 MB/s; deliberately conservative, UI shows "~Ns".
        const int secs = static_cast<int>(sz / (200ull * 1024 * 1024)) + 1;
        m.insert(QStringLiteral("estText"), QString::number(secs) + QStringLiteral("s"));
        list.append(m);
    }
    emit largeFilesPending(list);
}

void VaultViewModel::resolveLargeFiles(const QString &disposition)
{
    const QStringList small  = pendingSmall_;
    const QStringList large  = pendingLarge_;
    const bool        au     = pendingAutoUpload_;
    const QString     folder = pendingFolder_;
    pendingSmall_.clear();
    pendingLarge_.clear();

    QStringList toEncrypt = small;
    int skipped = 0;
    if (disposition == QStringLiteral("encrypt")) {
        toEncrypt += large;
    } else if (disposition == QStringLiteral("plain")) {
        for (const QString &f : large)
            copyToPlainFolder(f);
    } else {  // "skip" / cancel-but-keep-small
        skipped = large.size();
    }

    if (toEncrypt.isEmpty()) {
        refreshFiles();
        emit fileOpFinished(QStringLiteral("encrypt"), true,
                            QStringLiteral("0/") + QString::number(skipped), QString());
        return;
    }
    runEncryptBatch(toEncrypt, skipped, au, folder);
}

void VaultViewModel::runEncryptBatch(const QStringList &paths, int skipped, bool autoUpload,
                                     const QString &cloudFolder)
{
    if (!isUnlocked() || fileBusy_ || paths.isEmpty())
        return;
    std::shared_ptr<crypto::Key> key = snapshotKey(vault_.get());
    if (!key)
        return;

    noteActivity();
    setFileBusy(true);
    cancelBatch_.store(false);
    const QString outDir = vaultDir_;
    QPointer<VaultViewModel> guard(this);
    auto future = QtConcurrent::run(
        [this, guard, paths, key, outDir, skipped, autoUpload, cloudFolder]() {
            crypto::EncryptionEngine eng;  // local instance — worker thread
            int ok = 0;
            QString firstErr;
            QStringList okOutputs;
            QStringList failedInputs;
            const int total = paths.size();
            for (int i = 0; i < total; ++i) {
                if (cancelBatch_.load()) {
                    // Remaining files count as not-done (eligible for retry).
                    for (int j = i; j < total; ++j)
                        failedInputs << paths.at(j);
                    break;
                }
                const QString in = paths.at(i);
                const QFileInfo fi(in);
                const QString out =
                    outDir + QStringLiteral("/") + fi.fileName() + QStringLiteral(".fshenc");
                const CryptoError e = eng.encryptFile(in.toStdString(), out.toStdString(), *key);
                if (e == CryptoError::Ok) {
                    ++ok;
                    okOutputs << out;
                } else {
                    failedInputs << in;
                    if (firstErr.isEmpty())
                        firstErr = QString::fromLatin1(crypto::toString(e));
                }
                const QString name = fi.fileName();
                const int done = i + 1;
                QMetaObject::invokeMethod(
                    this,
                    [this, guard, done, total, name]() {
                        if (guard)
                            emit batchProgress(done, total, name);
                    },
                    Qt::QueuedConnection);
            }
            QMetaObject::invokeMethod(
                this,
                [this, guard, ok, total, skipped, firstErr, okOutputs, failedInputs, autoUpload,
                 cloudFolder]() {
                    if (!guard)
                        return;
                    setFileBusy(false);
                    refreshFiles();
                    lastFailed_     = failedInputs;
                    lastAutoUpload_ = autoUpload;
                    lastFolder_     = cloudFolder;
                    if (autoUpload && uploader_) {
                        for (const QString &out : okOutputs) {
                            uploader_(QStringList{out}, cloudFolder);
                            emit fileUploadQueued(QFileInfo(out).completeBaseName());
                        }
                    }
                    const int denom = total + skipped;
                    emit fileOpFinished(QStringLiteral("encrypt"), ok == total,
                                        QString::number(ok) + "/" + QString::number(denom),
                                        firstErr);
                },
                Qt::QueuedConnection);
        });
    Q_UNUSED(future);
}

void VaultViewModel::addFolderUpload(const QString &dir, bool autoUpload, const QString &cloudFolder)
{
    const QString lp = toLocalPath(dir);
    if (lp.isEmpty())
        return;
    QStringList files;
    QDirIterator it(lp, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        files << it.next();
    if (!files.isEmpty())
        beginEncrypt(files, autoUpload, cloudFolder.isEmpty() ? QStringLiteral("/") : cloudFolder);
}

void VaultViewModel::retryFailed()
{
    if (lastFailed_.isEmpty() || fileBusy_)
        return;
    const QStringList retry = lastFailed_;
    lastFailed_.clear();
    runEncryptBatch(retry, 0, lastAutoUpload_, lastFolder_);
}

void VaultViewModel::cancelBatch()
{
    cancelBatch_.store(true);
}

QString VaultViewModel::copyToPlainFolder(const QString &src)
{
    const QFileInfo fi(src);
    if (!fi.isFile())
        return {};
    // A sibling of the vault dir, clearly OUTSIDE the vault — never present an
    // unencrypted file as "protected" (encryption-plan §3.1 safety rule).
    QDir parent(vaultDir_);
    parent.cdUp();
    const QString plainDir = parent.absoluteFilePath(QStringLiteral("FsNext Unencrypted"));
    QDir().mkpath(plainDir);
    QString dest = plainDir + QStringLiteral("/") + fi.fileName();
    for (int n = 1; QFile::exists(dest); ++n) {
        const QString suffix = fi.suffix().isEmpty() ? QString() : QStringLiteral(".") + fi.suffix();
        dest = plainDir + QStringLiteral("/") + fi.completeBaseName()
               + QStringLiteral(" (") + QString::number(n) + QStringLiteral(")") + suffix;
    }
    return QFile::copy(src, dest) ? dest : QString();
}

void VaultViewModel::addFolder(const QString &dir)
{
    const QString lp = toLocalPath(dir);
    if (lp.isEmpty())
        return;
    QStringList files;
    QDirIterator it(lp, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        files << it.next();
    if (!files.isEmpty())
        addFiles(files);  // flat: each file → <basename>.fshenc in the vault folder
}

void VaultViewModel::decryptAndOpen(const QString &fshencPath)
{
    if (!isUnlocked() || fileBusy_)
        return;
    const QString in = toLocalPath(fshencPath);

    crypto::FencHeader h;
    QString name = QFileInfo(in).completeBaseName();
    if (engine_.readHeader(in.toStdString(), h) == CryptoError::Ok && !h.filename.empty())
        name = QString::fromStdString(h.filename);

    const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                           + QStringLiteral("/FsNextVault/")
                           + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir().mkpath(tmpDir);
    const QString out = tmpDir + QStringLiteral("/") + name;

    std::shared_ptr<crypto::Key> key = snapshotKey(vault_.get());
    if (!key)
        return;

    noteActivity();
    setFileBusy(true);
    QPointer<VaultViewModel> guard(this);
    auto future = QtConcurrent::run([this, guard, in, out, name, tmpDir, key]() {
        crypto::EncryptionEngine eng;
        const CryptoError e = eng.decryptFile(in.toStdString(), out.toStdString(), *key);
        QMetaObject::invokeMethod(
            this,
            [this, guard, e, out, name, tmpDir]() {
                if (!guard)
                    return;
                setFileBusy(false);
                const bool ok = (e == CryptoError::Ok);
                if (ok) {
                    // Track for secure-delete (30-min TTL, or immediately on lock).
                    trackTemp(out, tmpDir);
                    QDesktopServices::openUrl(QUrl::fromLocalFile(out));
                } else {
                    QDir(tmpDir).removeRecursively();  // nothing useful to keep
                }
                emit fileOpFinished(QStringLiteral("decrypt"), ok, name,
                                    ok ? QString() : QString::fromLatin1(crypto::toString(e)));
            },
            Qt::QueuedConnection);
    });
    Q_UNUSED(future);
}

void VaultViewModel::runAsync(const QString &op, std::function<CryptoError()> work)
{
    if (busy_) {
        emit operationFinished(op, false, QStringLiteral("Busy"));
        return;
    }
    setBusy(true);
    QPointer<VaultViewModel> guard(this);
    auto future = QtConcurrent::run([this, guard, op, work]() {
        const CryptoError e = work();  // worker thread — VaultManager is a plain class
        QMetaObject::invokeMethod(
            this,
            [this, guard, op, e]() {
                if (!guard)
                    return;
                setBusy(false);
                finish(op, e);
            },
            Qt::QueuedConnection);
    });
    Q_UNUSED(future);
}

void VaultViewModel::createVault(const QString &name, const QString &passphrase, int kdfPreset,
                                 int storageLevel)
{
    VaultManager *mgr = vault_.get();
    const crypto::KdfParams kdf = kdfFromPreset(kdfPreset);
    const auto level = static_cast<VaultManager::StorageLevel>(storageLevel);
    runAsync(QStringLiteral("create"),
             [mgr, name, passphrase, kdf, level] { return mgr->createVault(name, passphrase, kdf, level); });
}

void VaultViewModel::unlock(const QString &passphrase)
{
    VaultManager *mgr = vault_.get();
    runAsync(QStringLiteral("unlock"), [mgr, passphrase] { return mgr->unlock(passphrase); });
}

void VaultViewModel::changePassphrase(const QString &oldPass, const QString &newPass, int kdfPreset)
{
    VaultManager *mgr = vault_.get();
    const crypto::KdfParams kdf = kdfFromPreset(kdfPreset);
    runAsync(QStringLiteral("changePassphrase"),
             [mgr, oldPass, newPass, kdf] { return mgr->changePassphrase(oldPass, newPass, kdf); });
}

void VaultViewModel::unlockWithKeyfile(const QString &path)
{
    if (busy_) {
        emit operationFinished(QStringLiteral("unlockKeyfile"), false, QStringLiteral("Busy"));
        return;
    }
    finish(QStringLiteral("unlockKeyfile"), vault_->unlockWithKeyfile(toLocalPath(path)));
}

void VaultViewModel::lock()
{
    // Don't touch VaultManager while a worker op is mutating it.
    if (busy_)
        return;
    autoLockTimer_.stop();
    vault_->lock();
    sweepTemps(true);  // a locked vault should leave no decrypted temps behind
    finish(QStringLiteral("lock"), CryptoError::Ok);
}

void VaultViewModel::exportRecoveryKeyfile(const QString &path)
{
    if (busy_) {
        emit operationFinished(QStringLiteral("export"), false, QStringLiteral("Busy"));
        return;
    }
    finish(QStringLiteral("export"), vault_->exportRecoveryKeyfile(toLocalPath(path)));
}

void VaultViewModel::deleteVault()
{
    if (busy_) {
        emit operationFinished(QStringLiteral("delete"), false, QStringLiteral("Busy"));
        return;
    }
    autoLockTimer_.stop();
    vault_->deleteVault();
    finish(QStringLiteral("delete"), CryptoError::Ok);
}

bool VaultViewModel::tryAutoUnlock()
{
    if (busy_)
        return false;
    const bool ok = vault_->tryAutoUnlock();
    finish(QStringLiteral("autoUnlock"), ok ? CryptoError::Ok : CryptoError::NotUnlocked);
    return ok;
}

void VaultViewModel::refreshKeys()
{
    QVariantList list;
    if (isUnlocked() && vaultExists_ && vault_->masterKey().valid()) {
        QVariantMap v;
        v.insert(QStringLiteral("id"), QStringLiteral("vault"));
        v.insert(QStringLiteral("label"), vaultName_.isEmpty() ? tr("Vault") : vaultName_);
        v.insert(QStringLiteral("fingerprint"), fpHex(vault_->masterKey().fingerprint()));
        v.insert(QStringLiteral("type"), QStringLiteral("vault"));
        v.insert(QStringLiteral("backedUp"), hasRecovery_);
        list.append(v);
        if (hasRecovery_) {
            QVariantMap r;
            r.insert(QStringLiteral("id"), QStringLiteral("recovery"));
            r.insert(QStringLiteral("label"), tr("Khóa khôi phục"));
            r.insert(QStringLiteral("fingerprint"), QStringLiteral("—"));
            r.insert(QStringLiteral("type"), QStringLiteral("recovery"));
            r.insert(QStringLiteral("backedUp"), true);
            list.append(r);
        }
    }
    for (const auto &e : keyRegistry_.entries()) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), QString::fromStdString(e.id));
        m.insert(QStringLiteral("label"), QString::fromStdString(e.label));
        m.insert(QStringLiteral("fingerprint"), QString::fromStdString(e.fingerprint));
        m.insert(QStringLiteral("type"), QString::fromStdString(e.type));
        m.insert(QStringLiteral("backedUp"), true);
        list.append(m);
    }
    keys_ = list;
    emit keysChanged();
}

void VaultViewModel::copyFingerprint(const QString &fingerprint)
{
    if (auto *gui = qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
        gui->clipboard()->setText(fingerprint);
}

void VaultViewModel::importKeyfile(const QString &path)
{
    const QString lp = toLocalPath(path);
    QFile f(lp);
    if (!f.open(QIODevice::ReadOnly)) {
        emit operationFinished(QStringLiteral("importKey"), false, QStringLiteral("IoError"));
        return;
    }
    const QByteArray raw = f.read(64);
    f.close();
    if (raw.size() != static_cast<int>(crypto::Key::Size)) {
        emit operationFinished(QStringLiteral("importKey"), false, QStringLiteral("InvalidFormat"));
        return;
    }
    crypto::Key k = crypto::Key::fromBytes(
        reinterpret_cast<const std::uint8_t *>(raw.constData()), crypto::Key::Size);
    crypto::KeyEntry e;
    e.id          = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    e.label       = QFileInfo(lp).fileName().toStdString();
    e.fingerprint = fpHex(k.fingerprint()).toStdString();
    e.type        = "keyfile";
    e.createdAt   = QDateTime::currentSecsSinceEpoch();
    keyRegistry_.add(e);
    keyRegistry_.save((vaultDir_ + QStringLiteral("/.key-registry.json")).toStdString());
    refreshKeys();
    emit operationFinished(QStringLiteral("importKey"), true, QString());
}

void VaultViewModel::removeKey(const QString &id)
{
    if (keyRegistry_.remove(id.toStdString())) {
        keyRegistry_.save((vaultDir_ + QStringLiteral("/.key-registry.json")).toStdString());
        refreshKeys();
    }
}

// ── Secure-delete of decrypted temp files ──

void VaultViewModel::trackTemp(const QString &path, const QString &dir)
{
    temps_.push_back({path, dir, QDateTime::currentMSecsSinceEpoch()});
}

void VaultViewModel::secureDeleteFile(const QString &path)
{
    QFile f(path);
    if (!f.exists())
        return;
    // Best-effort overwrite before unlink. On SSDs wear-levelling may preserve
    // the original blocks, so this is belt-and-braces rather than a guarantee
    // (the UI help text says as much); on spinning disks it defeats trivial
    // undelete.
    if (f.open(QIODevice::ReadWrite)) {
        const qint64 sz = f.size();
        constexpr qint64 kBuf = 64 * 1024;
        QByteArray buf(kBuf, '\0');
        auto *rng = QRandomGenerator::global();
        qint64 remaining = sz;
        f.seek(0);
        while (remaining > 0) {
            const qint64 n = qMin(kBuf, remaining);
            for (qint64 i = 0; i < n; i += 4) {
                const quint32 r = rng->generate();
                std::memcpy(buf.data() + i, &r, static_cast<size_t>(qMin<qint64>(4, n - i)));
            }
            f.write(buf.constData(), n);
            remaining -= n;
        }
        f.flush();
        f.close();
    }
    QFile::remove(path);
}

void VaultViewModel::sweepTemps(bool force)
{
    if (temps_.empty())
        return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    constexpr qint64 kTtlMs = 30 * 60 * 1000;  // 30 minutes
    int cleaned = 0;
    std::vector<TempFile> kept;
    kept.reserve(temps_.size());
    for (const TempFile &t : temps_) {
        if (force || (now - t.createdMs) >= kTtlMs) {
            secureDeleteFile(t.path);
            if (!t.dir.isEmpty())
                QDir(t.dir).removeRecursively();
            ++cleaned;
        } else {
            kept.push_back(t);
        }
    }
    temps_.swap(kept);
    if (cleaned > 0)
        emit tempCleaned(cleaned);
}

} // namespace fsnext
