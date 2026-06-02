// SPDX-License-Identifier: Proprietary
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "core/crypto/CryptoError.h"
#include "core/crypto/EncryptionEngine.h"
#include "core/crypto/KeyRegistry.h"

namespace fsnext {

class VaultManager;
class SettingsService;

/// QML-facing bridge over VaultManager. Runs the expensive passphrase
/// operations (Argon2id) off the main thread so the UI never freezes, owns the
/// inactivity auto-lock timer, and exposes vault state as bindable properties.
///
/// lockState: 0 = no vault, 1 = locked, 2 = unlocked.
class VaultViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int lockState READ lockState NOTIFY lockStateChanged)
    Q_PROPERTY(bool isUnlocked READ isUnlocked NOTIFY lockStateChanged)
    Q_PROPERTY(bool vaultExists READ vaultExists NOTIFY lockStateChanged)
    Q_PROPERTY(QString vaultName READ vaultName NOTIFY lockStateChanged)
    Q_PROPERTY(bool hasRecoveryKey READ hasRecoveryKey NOTIFY lockStateChanged)
    Q_PROPERTY(int storageLevel READ storageLevel NOTIFY lockStateChanged)
    Q_PROPERTY(int autoLockMinutes READ autoLockMinutes WRITE setAutoLockMinutes NOTIFY autoLockMinutesChanged)
    Q_PROPERTY(bool aesAvailable READ aesAvailable CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString vaultDir READ vaultDir WRITE setVaultDir NOTIFY lockStateChanged)
    // Encrypted files in the vault folder. Each entry: { path, name, sizeText }.
    Q_PROPERTY(QVariantList vaultFiles READ vaultFiles NOTIFY vaultFilesChanged)
    // Separate busy flag for file encrypt/decrypt (distinct from create/unlock).
    Q_PROPERTY(bool fileBusy READ fileBusy NOTIFY fileBusyChanged)
    // Key Manager list: vault + recovery (live) + imported keyfiles (registry).
    // Each entry: { id, label, fingerprint, type, backedUp }.
    Q_PROPERTY(QVariantList keys READ keys NOTIFY keysChanged)
    // ── Vault prefs (proxied to SettingsService; see SET-VAULT) ──
    // Auto-encrypt cap in MB; 0 = no cap.
    Q_PROPERTY(int maxEncryptMb READ maxEncryptMb WRITE setMaxEncryptMb NOTIFY vaultPrefsChanged)
    // Over-cap behavior: 0 = ask, 1 = skip, 2 = plain folder.
    Q_PROPERTY(int overLimitBehavior READ overLimitBehavior WRITE setOverLimitBehavior NOTIFY vaultPrefsChanged)
    // Default "auto-upload .fshenc to Fshare" toggle.
    Q_PROPERTY(bool autoUploadDefault READ autoUploadDefault WRITE setAutoUploadDefault NOTIFY vaultPrefsChanged)
    // Default cloud destination folder path ("/" = root).
    Q_PROPERTY(QString uploadFolder READ uploadFolder WRITE setUploadFolder NOTIFY vaultPrefsChanged)
public:
    explicit VaultViewModel(QObject *parent = nullptr);
    ~VaultViewModel() override;

    /// Sink for auto-upload: receives (localPaths, cloudFolderPath). AppContext
    /// wires it to UploadViewModel::addUpload so this VM keeps no compile-time
    /// dependency on the transfer stack (and tests stay light).
    using Uploader = std::function<void(const QStringList &, const QString &)>;

    /// Inject size-cap policy (SettingsService) + the upload sink. Called from
    /// AppContext once collaborators exist. Both may be unset in tests.
    void setServices(SettingsService *settings, Uploader uploader);

    int     lockState() const { return lockState_; }
    bool    isUnlocked() const { return lockState_ == 2; }
    bool    vaultExists() const { return vaultExists_; }
    QString vaultName() const { return vaultName_; }
    bool    hasRecoveryKey() const { return hasRecovery_; }
    int     storageLevel() const { return storageLevel_; }
    int     autoLockMinutes() const { return autoLockMin_; }
    void    setAutoLockMinutes(int minutes);
    bool    aesAvailable() const { return aesAvailable_; }
    bool    busy() const { return busy_; }
    QString vaultDir() const { return vaultDir_; }
    // Q_INVOKABLE so QML (VaultWizard) can call vm.setVaultDir(path) directly,
    // not only via the vaultDir property-write binding.
    Q_INVOKABLE void setVaultDir(const QString &dir);

    /// For sibling VMs that need the master key (e.g. file encrypt/decrypt).
    /// Valid only while unlocked.
    VaultManager *manager() const { return vault_.get(); }

    // Heavy (Argon2id) → run asynchronously, result via operationFinished.
    Q_INVOKABLE void createVault(const QString &name, const QString &passphrase, int kdfPreset,
                                 int storageLevel);
    Q_INVOKABLE void unlock(const QString &passphrase);
    Q_INVOKABLE void changePassphrase(const QString &oldPass, const QString &newPass,
                                      int kdfPreset);
    // Fast → run synchronously.
    Q_INVOKABLE void unlockWithKeyfile(const QString &path);
    Q_INVOKABLE void lock();
    Q_INVOKABLE void exportRecoveryKeyfile(const QString &path);
    Q_INVOKABLE void deleteVault();
    Q_INVOKABLE bool tryAutoUnlock();
    Q_INVOKABLE void noteActivity();

    // ── Vault prefs ──
    int     maxEncryptMb() const;
    void    setMaxEncryptMb(int mb);
    int     overLimitBehavior() const;
    void    setOverLimitBehavior(int b);
    bool    autoUploadDefault() const;
    void    setAutoUploadDefault(bool on);
    QString uploadFolder() const;
    void    setUploadFolder(const QString &path);

    // ── File operations (require an unlocked vault) ──
    QVariantList vaultFiles() const { return vaultFiles_; }
    bool         fileBusy() const { return fileBusy_; }
    Q_INVOKABLE void refreshFiles();
    Q_INVOKABLE void addFiles(const QStringList &paths);   // encrypt into the vault (no auto-upload)
    Q_INVOKABLE void addFolder(const QString &dir);        // encrypt all files in a folder (recursive)
    // DLG-BATCH entry: encrypt a list, optionally auto-uploading each .fshenc
    // to `cloudFolder` (path, "/" = root) after it is written.
    Q_INVOKABLE void addFilesUpload(const QStringList &paths, bool autoUpload,
                                    const QString &cloudFolder);
    // DLG-BATCH folder entry: recurse `dir`, then encrypt (+ optional upload).
    Q_INVOKABLE void addFolderUpload(const QString &dir, bool autoUpload,
                                     const QString &cloudFolder);
    // Re-run the files that failed in the most recent batch ("Thử lại file lỗi").
    Q_INVOKABLE void retryFailed();
    // Cooperative cancel of the running batch (best-effort, between files).
    Q_INVOKABLE void cancelBatch();
    // Resolve a pending DLG-LARGEFILE prompt for the files emitted via
    // largeFilesPending(). disposition: "encrypt" | "plain" | "skip".
    Q_INVOKABLE void resolveLargeFiles(const QString &disposition);
    Q_INVOKABLE void decryptAndOpen(const QString &fshencPath);

    // ── Key manager ──
    QVariantList keys() const { return keys_; }
    Q_INVOKABLE void copyFingerprint(const QString &fingerprint);
    Q_INVOKABLE void importKeyfile(const QString &path);
    Q_INVOKABLE void removeKey(const QString &id);

signals:
    void lockStateChanged();
    void autoLockMinutesChanged();
    void busyChanged();
    void vaultFilesChanged();
    void fileBusyChanged();
    void keysChanged();
    void vaultPrefsChanged();
    /// Files exceeding the encrypt cap that need a user decision (policy=ask).
    /// Each entry: { path, name, sizeText, estText }. Resolve via resolveLargeFiles().
    void largeFilesPending(const QVariantList &files);
    /// A .fshenc was handed to the uploader. name = original display name.
    void fileUploadQueued(const QString &name);
    /// Secure-deleted N temporary decrypted files. UI shows a reassuring toast.
    void tempCleaned(int count);
    /// Per-file batch progress for DLG-BATCH. done/total processed, current name.
    void batchProgress(int done, int total, const QString &currentName);
    /// op: "encrypt" | "decrypt". name = "ok/total" (encrypt) or file name (decrypt).
    void fileOpFinished(const QString &op, bool ok, const QString &name, const QString &error);
    /// op: "create"|"unlock"|"unlockKeyfile"|"changePassphrase"|"export"|"lock"|"autoUnlock".
    /// On failure, `error` is the CryptoError name (UI should map to generic copy).
    void operationFinished(const QString &op, bool ok, const QString &error);

private:
    void setBusy(bool b);
    void setFileBusy(bool b);
    void refreshState();
    void refreshKeys();
    void armAutoLock();
    void finish(const QString &op, crypto::CryptoError e);
    void runAsync(const QString &op, std::function<crypto::CryptoError()> work);
    static QString toLocalPath(const QString &s);

    // ── Encrypt pipeline / size-cap policy ──
    qint64 encryptCapBytes() const;     // resolved cap (settings cap, else 1 GiB)
    int    overLimitPolicy() const;     // 0 ask · 1 skip · 2 plain
    void   beginEncrypt(const QStringList &paths, bool autoUpload, const QString &cloudFolder);
    void   runEncryptBatch(const QStringList &paths, int skipped, bool autoUpload,
                           const QString &cloudFolder);
    QString copyToPlainFolder(const QString &src);   // returns dest or empty on error

    // ── Secure-delete of decrypted temp files ──
    void trackTemp(const QString &path, const QString &dir);
    void sweepTemps(bool force);        // force = delete all regardless of age
    void sweepOrphanTemps();            // wipe temps left behind by a crashed prior session
    static void secureDeleteFile(const QString &path);

    std::unique_ptr<VaultManager> vault_;
    SettingsService *settings_ = nullptr;
    Uploader        uploader_;
    QTimer  autoLockTimer_;
    QTimer  tempSweepTimer_;
    QString vaultDir_;
    int     lockState_    = 0;
    bool    vaultExists_  = false;
    QString vaultName_;
    bool    hasRecovery_  = false;
    int     storageLevel_ = 2;
    int     autoLockMin_  = 15;
    bool    aesAvailable_ = false;
    bool    busy_         = false;
    bool    fileBusy_     = false;
    QVariantList vaultFiles_;
    QVariantList keys_;
    crypto::KeyRegistry keyRegistry_;
    crypto::EncryptionEngine engine_;   // stateless; used on the main thread only

    // Decrypted temp files awaiting secure-delete (age-based + on-lock sweep).
    struct TempFile { QString path; QString dir; qint64 createdMs; };
    std::vector<TempFile> temps_;

    // Pending DLG-LARGEFILE resolution (set when largeFilesPending is emitted).
    QStringList pendingSmall_;
    QStringList pendingLarge_;
    bool        pendingAutoUpload_ = false;
    QString     pendingFolder_;

    // Batch state: cooperative cancel + "retry failed" memory.
    std::atomic<bool> cancelBatch_{false};
    QStringList lastFailed_;
    bool        lastAutoUpload_ = false;
    QString     lastFolder_;
};

} // namespace fsnext
