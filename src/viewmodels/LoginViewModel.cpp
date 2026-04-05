#include "LoginViewModel.h"

#include "settings_manager.h"

LoginViewModel::LoginViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_request(this)
{
    connect(&m_request, &HttpRequestV2::signalGetusername,
            this, &LoginViewModel::onUsernameReceived);
    connect(&m_request, &HttpRequestV2::signalLoginFlag,
            this, &LoginViewModel::onLoginFlag);
    connect(&m_request, &HttpRequestV2::signalRegisterFlag,
            this, &LoginViewModel::onRegisterFlag);
    connect(&m_request, &HttpRequestV2::signalRegisterResult,
            this, &LoginViewModel::onRegisterResult);
    connect(&m_request, &HttpRequestV2::signalResetPasswordResult,
            this, &LoginViewModel::onResetPasswordResult);
}

QString LoginViewModel::cachedAccount() const
{
    return SettingsManager::instance().cachedAccount();
}

QString LoginViewModel::cachedPassword() const
{
    return SettingsManager::instance().cachedPassword();
}

QString LoginViewModel::cachedUsername() const
{
    return SettingsManager::instance().cachedUsername();
}

void LoginViewModel::requestLogin(const QString& account, const QString& password, bool autoLogin)
{
    const QString trimmedAccount = account.trimmed();
    const QString trimmedPassword = password.trimmed();
    if (trimmedAccount.isEmpty() || trimmedPassword.isEmpty()) {
        emit loginFailed(QStringLiteral("账号或密码不能为空"));
        return;
    }

    clearError();
    setIsBusy(true);

    m_lastRequestedAccount = trimmedAccount;
    m_lastRequestedPassword = trimmedPassword;
    m_lastRequestIsAutoLogin = autoLogin;
    m_loginInFlight = true;

    m_request.login(trimmedAccount, trimmedPassword);
}

void LoginViewModel::requestRegister(const QString& account, const QString& password, const QString& username)
{
    clearError();
    setIsBusy(true);
    m_request.registerUser(account, password, username);
}

void LoginViewModel::requestResetPassword(const QString& account, const QString& newPassword)
{
    clearError();
    setIsBusy(true);
    m_request.resetPassword(account, newPassword);
}

void LoginViewModel::onUsernameReceived(const QString& username)
{
    if (username.trimmed().isEmpty()) {
        return;
    }

    m_loginInFlight = false;
    setIsBusy(false);
    emit loginSucceeded(username);
}

void LoginViewModel::onLoginFlag(bool success)
{
    if (!m_loginInFlight || success) {
        return;
    }

    m_loginInFlight = false;
    setIsBusy(false);

    const QString message = m_lastRequestIsAutoLogin
            ? QStringLiteral("自动登录失败，请手动登录")
            : QStringLiteral("账号或密码错误");
    setErrorMessage(message);
    emit loginFailed(message);
}

void LoginViewModel::onRegisterFlag(bool success)
{
    if (success) {
        emit switchToLoginModeRequested();
    }
}

void LoginViewModel::onRegisterResult(bool success, const QString& message)
{
    setIsBusy(false);
    if (!success) {
        setErrorMessage(message);
    } else {
        clearError();
    }
    emit registerCompleted(success, message);
}

void LoginViewModel::onResetPasswordResult(bool success, const QString& message)
{
    setIsBusy(false);
    if (!success) {
        setErrorMessage(message);
    } else {
        clearError();
    }
    emit resetPasswordCompleted(success, message);
}
