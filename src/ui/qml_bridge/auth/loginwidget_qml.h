#ifndef LOGINWIDGET_QML_H
#define LOGINWIDGET_QML_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QVariant>
#include <QtDebug>

#include "httprequest_v2.h"
#include "settings_manager.h"

class LoginWidgetQml : public QObject
{
    Q_OBJECT

public:
    explicit LoginWidgetQml(QObject *parent = nullptr)
        : QObject(parent)
        , m_window(nullptr)
        , m_engine(new QQmlApplicationEngine(this))
        , request(new HttpRequestV2(this))
        , isVisible(false)
    {
        qDebug() << "LoginWidgetQml: Initializing...";

        m_engine->load(QUrl("qrc:/qml/components/auth/LoginWindow.qml"));
        if (m_engine->rootObjects().isEmpty()) {
            qWarning() << "LoginWidgetQml: Failed to load LoginWindow.qml";
            return;
        }

        m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
        if (!m_window) {
            qWarning() << "LoginWidgetQml: Failed to get QQuickWindow";
            return;
        }

        connect(m_window, SIGNAL(loginRequested(QString, QString)),
                this, SLOT(onLoginRequested(QString, QString)));
        connect(m_window, SIGNAL(quickLoginRequested(QString, QString)),
                this, SLOT(onQuickLoginRequested(QString, QString)));
        connect(m_window, SIGNAL(registerRequested(QString, QString, QString)),
                this, SLOT(onRegisterRequested(QString, QString, QString)));
        connect(m_window, SIGNAL(resetPasswordRequested(QString, QString)),
                this, SLOT(onResetPasswordRequested(QString, QString)));

        connect(request, &HttpRequestV2::signal_getusername,
                this, &LoginWidgetQml::onUsernameReceived);
        connect(request, &HttpRequestV2::signal_Loginflag,
                this, &LoginWidgetQml::onLoginFlag);
        connect(request, &HttpRequestV2::signal_Registerflag,
                this, &LoginWidgetQml::onRegisterResult);
        connect(request, &HttpRequestV2::signal_RegisterResult,
                this, &LoginWidgetQml::onRegisterResultMessage);
        connect(request, &HttpRequestV2::signal_ResetPasswordResult,
                this, &LoginWidgetQml::onResetPasswordResult);

        connect(m_window, &QQuickWindow::visibleChanged, this, [this]() {
            isVisible = m_window && m_window->isVisible();
        });

        // Keep login page aware of cached account for one-click login.
        const SettingsManager& settings = SettingsManager::instance();
        setSavedAccount(settings.cachedAccount(), settings.cachedPassword(), settings.cachedUsername());

        qDebug() << "LoginWidgetQml: Initialized";
    }

    ~LoginWidgetQml() override = default;

    void show()
    {
        if (!m_window) {
            return;
        }
        m_window->show();
        m_window->raise();
        m_window->requestActivate();
        isVisible = true;
    }

    void close()
    {
        if (!m_window) {
            return;
        }
        m_window->close();
        isVisible = false;
    }

    void setWindowTitle(const QString &title)
    {
        if (m_window) {
            m_window->setTitle(title);
        }
    }

    void requestLogin(const QString& account, const QString& password, bool autoLogin = false)
    {
        performLogin(account, password, autoLogin);
    }

    void setSavedAccount(const QString& account, const QString& password, const QString& username)
    {
        if (!m_window) {
            return;
        }
        QMetaObject::invokeMethod(
            m_window,
            "setSavedAccount",
            Q_ARG(QVariant, QVariant(account)),
            Q_ARG(QVariant, QVariant(password)),
            Q_ARG(QVariant, QVariant(username))
        );
    }

    bool isVisible;

signals:
    void login_(QString username);

private slots:
    void onLoginRequested(const QString &account, const QString &password)
    {
        performLogin(account, password, false);
    }

    void onQuickLoginRequested(const QString &account, const QString &password)
    {
        performLogin(account, password, false);
    }

    void onRegisterRequested(const QString &account, const QString &password, const QString &username)
    {
        qDebug() << "LoginWidgetQml: Register requested for account:" << account;
        request->registerUser(account, password, username);
    }

    void onResetPasswordRequested(const QString &account, const QString &newPassword)
    {
        qDebug() << "LoginWidgetQml: Reset password requested for account:" << account;
        request->resetPassword(account, newPassword);
    }

    void onUsernameReceived(const QString &username)
    {
        if (username.trimmed().isEmpty()) {
            return;
        }

        m_loginInFlight = false;

        emit login_(username);
        qDebug() << "LoginWidgetQml: Login successful, username:" << username;
        close();

        if (m_window) {
            QMetaObject::invokeMethod(m_window, "clearInputs");
        }
    }

    void onLoginFlag(bool success)
    {
        if (!m_loginInFlight) {
            return;
        }

        if (success) {
            return;
        }

        m_loginInFlight = false;
        qWarning() << "LoginWidgetQml: Login failed for account:" << m_lastRequestedAccount;

        if (m_lastRequestIsAutoLogin) {
            SettingsManager::instance().setAutoLoginEnabled(false);
        }

        if (m_window) {
            const QString message = m_lastRequestIsAutoLogin
                    ? QStringLiteral("自动登录失败，请手动登录")
                    : QStringLiteral("账号或密码错误");
            QMetaObject::invokeMethod(
                m_window,
                "onLoginFailed",
                Q_ARG(QVariant, QVariant(message))
            );
        }
    }

    void onRegisterResult(bool success)
    {
        if (success && m_window) {
            QMetaObject::invokeMethod(m_window, "switchToLoginMode");
        }
    }

    void onRegisterResultMessage(bool success, const QString &message)
    {
        if (!m_window) {
            return;
        }
        QMetaObject::invokeMethod(
            m_window,
            "onRegisterResult",
            Q_ARG(QVariant, QVariant(success)),
            Q_ARG(QVariant, QVariant(message))
        );
    }

    void onResetPasswordResult(bool success, const QString &message)
    {
        if (!m_window) {
            return;
        }
        QMetaObject::invokeMethod(
            m_window,
            "onResetPasswordResult",
            Q_ARG(QVariant, QVariant(success)),
            Q_ARG(QVariant, QVariant(message))
        );
    }

private:
    void performLogin(const QString& account, const QString& password, bool autoLogin)
    {
        const QString trimmedAccount = account.trimmed();
        const QString trimmedPassword = password.trimmed();
        if (trimmedAccount.isEmpty() || trimmedPassword.isEmpty()) {
            if (m_window) {
                QMetaObject::invokeMethod(
                    m_window,
                    "onLoginFailed",
                    Q_ARG(QVariant, QVariant(QStringLiteral("账号或密码不能为空")))
                );
            }
            return;
        }

        m_lastRequestedAccount = trimmedAccount;
        m_lastRequestedPassword = trimmedPassword;
        m_lastRequestIsAutoLogin = autoLogin;
        m_loginInFlight = true;

        qDebug() << "LoginWidgetQml: Login requested for account:" << trimmedAccount
                 << "auto:" << autoLogin;
        request->login(trimmedAccount, trimmedPassword);
    }

private:
    QQuickWindow *m_window;
    QQmlApplicationEngine *m_engine;
    HttpRequestV2 *request;

    QString m_lastRequestedAccount;
    QString m_lastRequestedPassword;
    bool m_lastRequestIsAutoLogin = false;
    bool m_loginInFlight = false;
};

#endif // LOGINWIDGET_QML_H
