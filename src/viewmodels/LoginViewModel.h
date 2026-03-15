#ifndef LOGINVIEWMODEL_H
#define LOGINVIEWMODEL_H

#include "BaseViewModel.h"
#include "httprequest_v2.h"

#include <QString>

/**
 * @brief 登录窗口视图模型。
 *
 * 负责封装登录、注册、重置密码与本地账号缓存读取逻辑，
 * 让登录界面只处理窗口展示与 QML 交互，不直接操作网络层或设置层。
 */
class LoginViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    explicit LoginViewModel(QObject* parent = nullptr);

    QString cachedAccount() const;
    QString cachedPassword() const;
    QString cachedUsername() const;

    void requestLogin(const QString& account, const QString& password, bool autoLogin);
    void requestRegister(const QString& account, const QString& password, const QString& username);
    void requestResetPassword(const QString& account, const QString& newPassword);

signals:
    void loginSucceeded(const QString& username);
    void loginFailed(const QString& message);
    void registerCompleted(bool success, const QString& message);
    void resetPasswordCompleted(bool success, const QString& message);
    void switchToLoginModeRequested();

private slots:
    void onUsernameReceived(const QString& username);
    void onLoginFlag(bool success);
    void onRegisterFlag(bool success);
    void onRegisterResult(bool success, const QString& message);
    void onResetPasswordResult(bool success, const QString& message);

private:
    HttpRequestV2 m_request;
    QString m_lastRequestedAccount;
    QString m_lastRequestedPassword;
    bool m_lastRequestIsAutoLogin = false;
    bool m_loginInFlight = false;
};

#endif // LOGINVIEWMODEL_H
