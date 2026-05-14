#pragma once

#include <QObject>
#include <QString>

namespace fsnext {

class AuthService;

class AuthViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString email        READ email        WRITE setEmail        NOTIFY emailChanged)
    Q_PROPERTY(QString password     READ password     WRITE setPassword     NOTIFY passwordChanged)
    Q_PROPERTY(bool isLoading       READ isLoading                          NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isLoggedIn      READ isLoggedIn                         NOTIFY isLoggedInChanged)
    Q_PROPERTY(bool rememberMe      READ rememberMe   WRITE setRememberMe   NOTIFY rememberMeChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage                       NOTIFY errorMessageChanged)
    Q_PROPERTY(QString userName     READ userName                           NOTIFY userChanged)
    Q_PROPERTY(QString userEmail    READ userEmail                          NOTIFY userChanged)
    Q_PROPERTY(int userLevel        READ userLevel                          NOTIFY userChanged)

public:
    explicit AuthViewModel(AuthService *auth, QObject *parent = nullptr);
    ~AuthViewModel() override = default;

    // Property getters
    QString email() const;
    QString password() const;
    bool isLoading() const;
    bool isLoggedIn() const;
    bool rememberMe() const;
    QString errorMessage() const;
    QString userName() const;
    QString userEmail() const;
    int userLevel() const;

    // Property setters
    void setEmail(const QString &email);
    void setPassword(const QString &password);
    void setRememberMe(bool rememberMe);

    Q_INVOKABLE void login();
    Q_INVOKABLE void logout();

    // Social login — each opens the user's default browser to the provider's
    // authorization page, then completes the RFC 8252 loopback flow. Non-blocking.
    Q_INVOKABLE void loginWithGoogle();
    Q_INVOKABLE void loginWithFacebook();
    Q_INVOKABLE void loginWithFptId();

signals:
    void emailChanged();
    void passwordChanged();
    void isLoadingChanged();
    void isLoggedInChanged();
    void rememberMeChanged();
    void errorMessageChanged();
    void userChanged();

    // Relayed from AuthService — carries the "Session expired" message
    // so QML can show a toast and route the user back to login.
    void sessionExpiredNotice(const QString &message);

private:
    void onLoginSuccess();
    void onLoginFailed(const QString &message);

    AuthService *m_auth         = nullptr;
    QString      m_email;
    QString      m_password;
    QString      m_errorMessage;
    bool         m_isLoading    = false;
    bool         m_rememberMe   = false;
};

} // namespace fsnext
