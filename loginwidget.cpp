#include "loginwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QMessageBox>
#include <QDebug>

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
    this->resize(400, 300);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint); // 无边框窗口
    this->setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    isLogin = true; // 默认是登录模式

    auto request = HttpRequest::getInstance();

    // 自定义标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet("background-color: #FF4766; border-top-left-radius: 10px; border-top-right-radius: 10px;");

    QLabel *titleLabel = new QLabel("网易云音乐登录", titleBar);
    titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);

    QPushButton *closeButton = new QPushButton("×", titleBar);
    closeButton->setFixedSize(30, 30);
    closeButton->setStyleSheet("background: transparent; color: white; font-size: 18px; border: none;");
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(closeButton);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    // 主内容区
    account = new QLineEdit(this);
    account->setPlaceholderText("请输入账号");
    account->setFixedHeight(30);
    account->setStyleSheet("border: 1px solid #D9D9D9; border-radius: 4px; padding: 4px;");

    password = new QLineEdit(this);
    password->setEchoMode(QLineEdit::Password);
    password->setPlaceholderText("请输入密码");
    password->setFixedHeight(30);
    password->setStyleSheet("border: 1px solid #D9D9D9; border-radius: 4px; padding: 4px;");

    // 用户名输入框 (仅在注册时显示)
    username = new QLineEdit(this);
    username->setPlaceholderText("请输入用户名");
    username->setFixedHeight(30);
    username->setStyleSheet("border: 1px solid #D9D9D9; border-radius: 4px; padding: 4px;");
    username->setVisible(false);  // 默认隐藏用户名输入框

    login = new QPushButton("登录", this);
    login->setFixedHeight(35);
    login->setStyleSheet("background-color: #FF4766; color: white; border-radius: 4px; font-size: 16px;");

    Register = new QLabel("<a href=\"#\">注册</a>", this);
    Register->setStyleSheet("color: #1DB954; font-size: 14px;");
    Register->setTextFormat(Qt::RichText);
    Register->setTextInteractionFlags(Qt::TextBrowserInteraction);
    Register->setOpenExternalLinks(false);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(account);
    mainLayout->addWidget(password);
    mainLayout->addWidget(username);  // 显示用户名输入框
    mainLayout->addWidget(login);
    mainLayout->addWidget(Register, 0, Qt::AlignCenter);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    setLayout(mainLayout);

    // 信号与槽
    connect(login, &QPushButton::clicked, this, [=]() {
        QString Account = account->text();
        QString Password = password->text();

        if (Account.isEmpty() || Password.isEmpty()) {
            qDebug() << "账号或密码不能为空！";
            return;
        }

        if (isLogin) {
            // 登录逻辑
            qDebug() << "尝试登录: 账号:" << Account << "密码:" << Password;
            request->Login(Account, Password); // 调用 HttpRequest 的登录方法
        } else {
            // 注册逻辑
            QString Username = username->text();  // 从用户名输入框获取用户名
            if (Username.isEmpty()) {
                qDebug() << "用户名不能为空！";
                return;
            }
            qDebug() << "尝试注册: 账号:" << Account << "密码:" << Password << "用户名:" << Username;
            request->Register(Account, Password, Username); // 调用 HttpRequest 的注册方法
        }
    });

    connect(Register, &QLabel::linkActivated, this, [=]() {
        // 切换登录和注册模式
        isLogin = !isLogin;
        login->setText(isLogin ? "登录" : "注册");
        Register->setText(isLogin ? "<a href=\"#\">注册</a>" : "<a href=\"#\">登录</a>");
        username->setVisible(!isLogin);  // 切换用户名输入框的显示与隐藏
    });

    connect(request, &HttpRequest::signal_getusername, this, [=](QString username) {
        if(username.size() > 0)
        {
            emit login_(username);
            qDebug() << "登录成功，用户名:" << username;
        }
        else
        {
            QMessageBox::critical(nullptr,
                                      "错误",
                                      "账号或密码错误",
                                      QMessageBox::Ok);
        }
    });

    connect(request, &HttpRequest::signal_Registerflag, this, [=](bool success) {
        if (success && !isLogin) {
            qDebug() << "注册成功！";
            isLogin = true;
            login->setText("登录");
            Register->setText("<a href=\"#\">注册</a>");
            username->clear();  // 注册成功后清空用户名输入框
        } else if (!success) {
            qDebug() << "注册失败！";
        }
    });
}

void LoginWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressed = true;
        mouseStartPoint = event->globalPos();
        windowStartPoint = this->frameGeometry().topLeft();
    }
}

void LoginWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (mousePressed) {
        QPoint distance = event->globalPos() - mouseStartPoint;
        this->move(windowStartPoint + distance);
    }
}

void LoginWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    mousePressed = false;
}
