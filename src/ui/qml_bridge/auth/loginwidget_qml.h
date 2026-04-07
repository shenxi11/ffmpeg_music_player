#ifndef LOGINWIDGET_QML_H
#define LOGINWIDGET_QML_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QVariant>
#include <QtDebug>

#include "viewmodels/LoginViewModel.h"

class LoginWidgetQml : public QObject
{
    Q_OBJECT

public:
    explicit LoginWidgetQml(QObject *parent = nullptr)
        : QObject(parent)
        , m_window(nullptr)
        , m_engine(new QQmlApplicationEngine(this))
        , m_viewModel(new LoginViewModel(this))
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

        connect(m_viewModel, &LoginViewModel::loginSucceeded,
                this, &LoginWidgetQml::onLoginSucceeded);
        connect(m_viewModel, &LoginViewModel::loginFailed,
                this, &LoginWidgetQml::onLoginFailed);
        connect(m_viewModel, &LoginViewModel::switchToLoginModeRequested,
                this, &LoginWidgetQml::onSwitchToLoginModeRequested);
        connect(m_viewModel, &LoginViewModel::registerCompleted,
                this, &LoginWidgetQml::onRegisterResultMessage);
        connect(m_viewModel, &LoginViewModel::resetPasswordCompleted,
                this, &LoginWidgetQml::onResetPasswordResult);

        connect(m_window, &QQuickWindow::visibleChanged,
                this, &LoginWidgetQml::onWindowVisibleChanged);

        // Keep login page aware of cached account for one-click login.
        setSavedAccount(m_viewModel->cachedAccount(),
                        m_viewModel->cachedPassword(),
                        m_viewModel->cachedUsername());

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

    Q_INVOKABLE void requestLogin(const QString& account, const QString& password, bool autoLogin = false)
    {
        performLogin(account, password, autoLogin);
    }

    Q_INVOKABLE void setSavedAccount(const QString& account, const QString& password, const QString& username)
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
    void login_(QString username, QString avatarUrl, QString onlineSessionToken);

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
        m_viewModel->requestRegister(account, password, username);
    }

    void onResetPasswordRequested(const QString &account, const QString &newPassword)
    {
        qDebug() << "LoginWidgetQml: Reset password requested for account:" << account;
        m_viewModel->requestResetPassword(account, newPassword);
    }

    void onLoginSucceeded(const QString &username,
                          const QString& avatarUrl,
                          const QString& onlineSessionToken)
    {
        if (username.trimmed().isEmpty()) {
            return;
        }

        emit login_(username, avatarUrl, onlineSessionToken);
        qDebug() << "LoginWidgetQml: Login successful, username:" << username;
        close();

        if (m_window) {
            QMetaObject::invokeMethod(m_window, "clearInputs");
        }
    }

    void onLoginFailed(const QString& message)
    {
        if (m_window) {
            QMetaObject::invokeMethod(
                m_window,
                "onLoginFailed",
                Q_ARG(QVariant, QVariant(message))
            );
        }
    }

    void onSwitchToLoginModeRequested()
    {
        if (m_window) {
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

    void onWindowVisibleChanged()
    {
        isVisible = m_window && m_window->isVisible();
    }

private:
    void performLogin(const QString& account, const QString& password, bool autoLogin)
    {
        qDebug() << "LoginWidgetQml: Login requested for account:" << account.trimmed()
                 << "auto:" << autoLogin;
        m_viewModel->requestLogin(account, password, autoLogin);
    }

private:
    QQuickWindow *m_window;
    QQmlApplicationEngine *m_engine;
    LoginViewModel* m_viewModel;
};

#endif // LOGINWIDGET_QML_H
