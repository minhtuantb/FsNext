#pragma once

#include <QObject>
#include <QString>

namespace fsnext {

class AuthService;

class UserInfoViewModel : public QObject
{
    Q_OBJECT

    // ── Identity ──────────────────────────────────────────────────────────
    Q_PROPERTY(QString userId          READ userId          NOTIFY userInfoChanged)
    Q_PROPERTY(QString userName        READ userName        NOTIFY userInfoChanged)
    Q_PROPERTY(QString userEmail       READ userEmail       NOTIFY userInfoChanged)
    Q_PROPERTY(int     userLevel       READ userLevel       NOTIFY userInfoChanged)
    Q_PROPERTY(QString levelLabel      READ levelLabel      NOTIFY userInfoChanged)
    // Convenience flag for QML: true for any paid/VIP tier (level 3+), false
    // for free tier (level 0-2 "Thành viên thường"). Lets the sidebar / VIP
    // card swap to a lighter visual variant for free users instead of showing
    // the orange gradient hero unconditionally.
    Q_PROPERTY(bool    isVip           READ isVip           NOTIFY userInfoChanged)
    Q_PROPERTY(QString accountType     READ accountType     NOTIFY userInfoChanged)
    Q_PROPERTY(QString joinDate        READ joinDate        NOTIFY userInfoChanged)
    Q_PROPERTY(QString vipExpiry       READ vipExpiry       NOTIFY userInfoChanged)
    Q_PROPERTY(QString avatarUrl       READ avatarUrl       NOTIFY userInfoChanged)

    // ── Points & download quota ───────────────────────────────────────────
    Q_PROPERTY(int     totalPoints     READ totalPoints     NOTIFY userInfoChanged)
    Q_PROPERTY(int     dlTimeAvail     READ dlTimeAvail     NOTIFY userInfoChanged)

    // ── Non-secure (regular) webspace ─────────────────────────────────────
    Q_PROPERTY(qint64  webspaceTotal   READ webspaceTotal   NOTIFY userInfoChanged)
    Q_PROPERTY(qint64  webspaceUsed    READ webspaceUsed    NOTIFY userInfoChanged)
    Q_PROPERTY(qint64  webspaceFree    READ webspaceFree    NOTIFY userInfoChanged)

    // ── Secure webspace ───────────────────────────────────────────────────
    Q_PROPERTY(qint64  secureTotal     READ secureTotal     NOTIFY userInfoChanged)
    Q_PROPERTY(qint64  secureUsed      READ secureUsed      NOTIFY userInfoChanged)
    Q_PROPERTY(qint64  secureFree      READ secureFree      NOTIFY userInfoChanged)

    // ── Combined totals ───────────────────────────────────────────────────
    Q_PROPERTY(qint64  storageTotalAll READ storageTotalAll NOTIFY userInfoChanged)

    // ── Traffic ───────────────────────────────────────────────────────────
    Q_PROPERTY(qint64  trafficTotal    READ trafficTotal    NOTIFY userInfoChanged)
    Q_PROPERTY(qint64  trafficUsed     READ trafficUsed     NOTIFY userInfoChanged)
    Q_PROPERTY(qint64  trafficFree     READ trafficFree     NOTIFY userInfoChanged)

public:
    explicit UserInfoViewModel(AuthService *auth, QObject *parent = nullptr);
    ~UserInfoViewModel() override = default;

    // Identity
    QString userId() const;
    QString userName() const;
    QString userEmail() const;
    int     userLevel() const;
    QString levelLabel() const;
    bool    isVip() const;
    QString accountType() const;
    QString joinDate() const;
    QString vipExpiry() const;
    QString avatarUrl() const;

    // Points & quota
    int totalPoints() const;
    int dlTimeAvail() const;

    // Non-secure webspace
    qint64 webspaceTotal() const;
    qint64 webspaceUsed() const;
    qint64 webspaceFree() const;

    // Secure webspace
    qint64 secureTotal() const;
    qint64 secureUsed() const;
    qint64 secureFree() const;

    // Combined
    qint64 storageTotalAll() const;

    // Traffic
    qint64 trafficTotal() const;
    qint64 trafficUsed() const;
    qint64 trafficFree() const;

    Q_INVOKABLE void refresh();

signals:
    void userInfoChanged();

private:
    AuthService *m_auth = nullptr;

    // Map numeric level → human-readable Vietnamese label.
    static QString levelToLabel(int level, const QString &accountTypeFallback);
};

} // namespace fsnext
