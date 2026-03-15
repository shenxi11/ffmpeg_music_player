#include "loginwidget.h"

#include <QMessageBox>

/*
模块名称: LoginWidget 连接配置
功能概述: 统一管理登录页按钮、模式切换与 ViewModel 回调连接。
对外接口: setupAuthConnections
维护说明: 登录视图仅做输入采集与状态展示，鉴权流程由 ViewModel 承载。
*/

void LoginWidget::setupAuthConnections(QLabel* welcomeLabel)
{
    m_welcomeLabel = welcomeLabel;

    connect(login, &QPushButton::clicked, this, &LoginWidget::handleSubmitClicked);
    connect(Register, &QLabel::linkActivated, this, &LoginWidget::handleModeLinkActivated);

    connect(m_viewModel, &LoginViewModel::loginSucceeded, this, &LoginWidget::handleLoginSucceeded);
    connect(m_viewModel, &LoginViewModel::loginFailed, this, &LoginWidget::handleLoginFailed);
    connect(m_viewModel, &LoginViewModel::switchToLoginModeRequested,
            this, &LoginWidget::handleSwitchToLoginModeRequested);
    connect(m_viewModel, &LoginViewModel::registerCompleted, this, &LoginWidget::handleRegisterCompleted);
}

void LoginWidget::handleSubmitClicked()
{
    const QString accountText = account ? account->text() : QString();
    const QString passwordText = password ? password->text() : QString();

    if (!m_viewModel) {
        return;
    }

    if (isLogin) {
        m_viewModel->requestLogin(accountText, passwordText, false);
        return;
    }

    const QString usernameText = username ? username->text() : QString();
    m_viewModel->requestRegister(accountText, passwordText, usernameText);
}

void LoginWidget::handleModeLinkActivated(const QString& link)
{
    Q_UNUSED(link);
    isLogin = !isLogin;

    if (isLogin) {
        login->setText(QStringLiteral("登录"));
        Register->setText(QStringLiteral("<a href=\"#\" style='color: white; text-decoration: none;'>还没有账号？点击注册</a>"));
        if (m_welcomeLabel) {
            m_welcomeLabel->setText(QStringLiteral("欢迎回来"));
        }
    } else {
        login->setText(QStringLiteral("注册"));
        Register->setText(QStringLiteral("<a href=\"#\" style='color: white; text-decoration: none;'>已有账号？点击登录</a>"));
        if (m_welcomeLabel) {
            m_welcomeLabel->setText(QStringLiteral("创建新账号"));
        }
    }
    if (username) {
        username->setVisible(!isLogin);
    }
}

void LoginWidget::handleLoginSucceeded(const QString& userName)
{
    emit login_(userName);
    qDebug() << "登录成功，用户名:" << userName;
}

void LoginWidget::handleLoginFailed(const QString& message)
{
    QMessageBox::critical(this, QStringLiteral("错误"), message, QMessageBox::Ok);
}

void LoginWidget::handleSwitchToLoginModeRequested()
{
    if (isLogin) {
        return;
    }

    isLogin = true;
    login->setText(QStringLiteral("登录"));
    Register->setText(QStringLiteral("<a href=\"#\" style='color: white; text-decoration: none;'>还没有账号？点击注册</a>"));
    if (m_welcomeLabel) {
        m_welcomeLabel->setText(QStringLiteral("欢迎回来"));
    }
    if (username) {
        username->setVisible(false);
        username->clear();
    }
}

void LoginWidget::handleRegisterCompleted(bool success, const QString& message)
{
    if (!success && !message.trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), message, QMessageBox::Ok);
    }
}
