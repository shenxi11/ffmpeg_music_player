#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "viewmodels/LoginViewModel.h"

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(QWidget* parent = nullptr);

    bool isVisible = false;

signals:
    void login_(QString username);

protected:
    void closeEvent(QCloseEvent* event) override
    {
        this->isVisible = false;
    }
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    // 连接拆分：统一管理登录页交互与 ViewModel 回调绑定。
    void setupAuthConnections(QLabel* welcomeLabel);
    void handleSubmitClicked();
    void handleModeLinkActivated(const QString& link);
    void handleLoginSucceeded(const QString& userName);
    void handleLoginFailed(const QString& message);
    void handleSwitchToLoginModeRequested();
    void handleRegisterCompleted(bool success, const QString& message);

    QLineEdit* account = nullptr;
    QLineEdit* password = nullptr;
    QLineEdit* username = nullptr;
    QPushButton* login = nullptr;
    QLabel* Register = nullptr;
    QLabel* m_welcomeLabel = nullptr;

    bool isLogin = true;
    bool mousePressed = false;
    QPoint mouseStartPoint = QPoint(0, 0);
    QPoint windowStartPoint = QPoint(0, 0);

    LoginViewModel* m_viewModel = nullptr;
};

#endif // LOGINWIDGET_H
