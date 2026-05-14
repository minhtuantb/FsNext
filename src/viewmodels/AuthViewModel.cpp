#include "AuthViewModel.h"
#include "core/api/OAuthProvider.h"
#include "core/services/AuthService.h"
#include <QPointer>

namespace fsnext {

AuthViewModel::AuthViewModel(AuthService *auth, QObject *parent)
    : QObject(parent)
    , m_auth(auth)
{
    connect(m_auth, &AuthService::loginSuccess, this, &AuthViewModel::onLoginSuccess);
    connect(m_auth, &AuthService::loginFailed,  this, &AuthViewModel::onLoginFailed);
    connect(m_auth, &AuthService::isLoggedInChanged, this, &AuthViewModel::isLoggedInChanged);
    connect(m_auth, &AuthService::userChanged,       this, &AuthViewModel::userChanged);
    connect(m_auth, &AuthService::sessionExpiredNotice, this, &AuthViewModel::sessionExpiredNotice);

    // Clear credentials from memory when user logs out.
    // QPointer guards against the (rare) case where a queued signal fires
    // during destruction. Passing `this` as receiver to connect() also ensures
    // Qt auto-disconnects on destruction — belt-and-suspenders.
    QPointer<AuthViewModel> guard(this);
    connect(m_auth, &AuthService::isLoggedInChanged, this, [guard]() {
        if (!guard) return;
        auto *self = guard.data();
        if (!self->m_auth || !self->m_auth->isLoggedIn()) {
            if (!self->m_password.isEmpty()) {
                self->m_password.clear();
                self->m_password.squeeze();
                emit self->passwordChanged();
            }
            if (!self->m_errorMessage.isEmpty()) {
                self->m_errorMessage.clear();
                emit self->errorMessageChanged();
            }
        }
    });

    // Load saved state
    m_email = m_auth->savedEmail();
    m_password = m_auth->savedPassword();
    m_rememberMe = m_auth->rememberMe();

#ifdef FSNEXT_DEV_BUILD
    // DEV build only: pre-fill from env vars FSNEXT_DEV_EMAIL / FSNEXT_DEV_PASSWORD
    // (opt-in). Never hardcode real credentials in source — a leaked binary or
    // screenshot would expose them.
    if (m_email.isEmpty()) {
        const QString devEmail    = qEnvironmentVariable("FSNEXT_DEV_EMAIL");
        const QString devPassword = qEnvironmentVariable("FSNEXT_DEV_PASSWORD");
        if (!devEmail.isEmpty() && !devPassword.isEmpty()) {
            m_email = devEmail;
            m_password = devPassword;
            m_rememberMe = true;
        }
    }
#endif

    if (!m_email.isEmpty()) emit emailChanged();
    if (!m_password.isEmpty()) emit passwordChanged();
    if (m_rememberMe) emit rememberMeChanged();
}

// Getters
QString AuthViewModel::email() const { return m_email; }
QString AuthViewModel::password() const { return m_password; }
bool AuthViewModel::isLoading() const { return m_isLoading; }
bool AuthViewModel::isLoggedIn() const { return m_auth ? m_auth->isLoggedIn() : false; }
bool AuthViewModel::rememberMe() const { return m_rememberMe; }
QString AuthViewModel::errorMessage() const { return m_errorMessage; }
QString AuthViewModel::userName() const { return m_auth ? m_auth->currentUser().name : QString{}; }
QString AuthViewModel::userEmail() const { return m_auth ? m_auth->currentUser().email : QString{}; }
int AuthViewModel::userLevel() const { return m_auth ? m_auth->currentUser().level : 0; }

// Setters
void AuthViewModel::setEmail(const QString &v)
{
    if (m_email == v) return;
    m_email = v;
    emit emailChanged();
}

void AuthViewModel::setPassword(const QString &v)
{
    if (m_password == v) return;
    m_password = v;
    emit passwordChanged();
}

void AuthViewModel::setRememberMe(bool v)
{
    if (m_rememberMe == v) return;
    m_rememberMe = v;
    if (m_auth) m_auth->setRememberMe(v);
    emit rememberMeChanged();
}

// Actions
void AuthViewModel::login()
{
    if (!m_auth || m_isLoading) return;
    m_errorMessage.clear();
    emit errorMessageChanged();
    m_isLoading = true;
    emit isLoadingChanged();
    m_auth->login(m_email, m_password);
}

void AuthViewModel::logout()
{
    if (m_auth) m_auth->logout();
}

// ── Social login invokables ─────────────────────────────

void AuthViewModel::loginWithGoogle()
{
    if (!m_auth) return;
    m_errorMessage.clear();
    emit errorMessageChanged();
    m_isLoading = true;
    emit isLoadingChanged();
    m_auth->loginWithProvider(OAuthConfig::google());
}

void AuthViewModel::loginWithFacebook()
{
    if (!m_auth) return;
    m_errorMessage.clear();
    emit errorMessageChanged();
    m_auth->loginWithProvider(OAuthConfig::facebook());
}

void AuthViewModel::loginWithFptId()
{
    if (!m_auth) return;
    m_errorMessage.clear();
    emit errorMessageChanged();
    m_auth->loginWithProvider(OAuthConfig::fptId());
}

// Private slots
void AuthViewModel::onLoginSuccess()
{
    m_isLoading = false;
    emit isLoadingChanged();
}

void AuthViewModel::onLoginFailed(const QString &message)
{
    m_isLoading = false;
    emit isLoadingChanged();
    m_errorMessage = message;
    emit errorMessageChanged();
}

} // namespace fsnext
